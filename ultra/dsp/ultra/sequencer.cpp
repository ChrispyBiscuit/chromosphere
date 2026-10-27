#include "sequencer.h"
#include "blocks.h"
#include <algorithm>
#include <cmath>

namespace ultra {

namespace {
constexpr double kBarBeats = 4.0;
inline int64_t floorDiv(int64_t a, int64_t b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

bool condition(Cond c, int64_t loop, int64_t firstLoop, bool fill) {
    auto ab = [&](int a, int b) { return (int)(((loop % b) + b) % b) == a - 1; };
    switch (c) {
        case Cond::Always: return true;
        case Cond::Fill: return fill;
        case Cond::NotFill: return !fill;
        case Cond::First: return loop == firstLoop;
        case Cond::NotFirst: return loop != firstLoop;
        case Cond::C12: return ab(1, 2);
        case Cond::C22: return ab(2, 2);
        case Cond::C13: return ab(1, 3);
        case Cond::C23: return ab(2, 3);
        case Cond::C33: return ab(3, 3);
        case Cond::C14: return ab(1, 4);
        case Cond::C24: return ab(2, 4);
        case Cond::C34: return ab(3, 4);
        case Cond::C44: return ab(4, 4);
    }
    return true;
}
}  // namespace

void Sequencer::reset() {
    running_ = false; internalWas_ = false;
    internalPpq_ = startPpq_ = endPpq_ = 0.0;
    queued_ = -1;
    for (auto& s : step_) s = -1;
}

void Sequencer::requestPattern(int p) {
    p = std::clamp(p, 0, kPatterns - 1);
    if (!running_) { current_ = p; queued_ = -1; }
    else queued_ = p == current_ ? -1 : p;
}

double Sequencer::swingWarp(double grid, double swing) {
    if (swing <= 0.5001) return grid;
    const double pair = 0.5, base = std::floor(grid / pair) * pair, q = grid - base;
    return q < 0.25 ? base + q * 2.0 * swing : base + swing * 0.5 + (q - 0.25) * (2.0 - 2.0 * swing);
}

int Sequencer::collect(const Pattern& pat, const KitParams& kit, double p0, double spb, int sFrom, int sTo, float swing,
                       float humanize, float lockScale, int n, Hit* out, int count, int maxHits) {
    (void)n;
    if (sTo <= sFrom) return count;
    const double a = p0 + sFrom / spb, b = p0 + sTo / spb;
    const double beatsPerSec = bpm_ / 60.0;
    for (int t = 0; t < kTracks; ++t) {
        if (kit.tracks[(size_t)t].mute) continue;
        const auto& tp = pat.tracks[(size_t)t];
        const int len = std::clamp(tp.length, 1, kMaxSteps);
        const int rate = std::clamp(tp.rate, 0, kNumRates - 1);
        const double sb = kRateBeats[rate];
        const bool swings = rateSwings(rate);
        const int64_t kmin = std::max<int64_t>(0, (int64_t)std::floor(a / sb) - 3);
        const int64_t kmax = (int64_t)std::floor(b / sb) + 1;
        const int64_t firstLoop = floorDiv((int64_t)std::floor(startPpq_ / sb + 1e-9), len);
        for (int64_t k = kmin; k <= kmax; ++k) {
            const int s = (int)(k % len);
            const Step& st = tp.steps[(size_t)s];
            if (st.vel == 0) continue;
            const int64_t loop = k / len;
            if (!condition((Cond)std::min<int>(st.cond, kNumConds - 1), loop, firstLoop, kit.fill)) continue;
            if (st.chance < 100 && hash01(0xC4A2u + (uint32_t)t, (uint32_t)k, (uint32_t)(k >> 32)) * 100.f >= (float)st.chance) continue;
            double grid = (double)k * sb;
            if (swings) grid = swingWarp(grid, swing);
            double at = grid + std::clamp((int)st.nudge, -50, 50) * 0.01 * sb;
            float velMul = 1.f;
            if (humanize > 0.f) {
                at += (hash01(0x5EEDu + (uint32_t)t, (uint32_t)k, 77u) * 2.f - 1.f) * 0.008 * beatsPerSec * humanize;
                velMul = 1.f + (hash01(0x5EEDu + (uint32_t)t, (uint32_t)k, 91u) * 2.f - 1.f) * 0.15f * humanize;
            }
            const int rat = std::clamp((int)st.ratchet, 1, 4);
            for (int r = 0; r < rat; ++r) {
                const double tr = at + r * sb / rat;
                const double S = (tr - p0) * spb;
                const int off = (int)std::ceil(S - 0.01);   // tolerance: float dials (swing) and accumulated positions
                if (off < sFrom || off >= sTo) continue;
                if (count >= maxHits) return count;
                Hit h;
                h.offset = off; h.track = t;
                h.velocity = std::clamp(st.vel / 127.f * (1.f - 0.1f * r) * velMul, 0.f, 1.f);
                h.pitch = std::clamp((int)st.pitch, -24, 24);
                h.lock = std::clamp(st.lock * 0.01f * lockScale, -1.f, 1.f);
                out[count++] = h;
            }
        }
    }
    return count;
}

int Sequencer::process(const PatternBank& bank, const KitParams& kit, float swing, float humanize, float lockScale,
                       const Transport& t, bool internalPlay, int n, Hit* out, int maxHits) {
    if (kit.pattern != lastParam_) { lastParam_ = kit.pattern; requestPattern(kit.pattern); }
    const bool host = t.playing;
    const bool run = host || internalPlay;
    bpm_ = std::clamp(t.hasTempo ? t.bpm : (double)kit.tempo, 20.0, 999.0);
    const double spb = fs_ * 60.0 / bpm_;              // samples per beat
    double p0;
    if (host) { p0 = t.ppq; internalWas_ = false; }
    else if (internalPlay) {
        if (!internalWas_) { internalPpq_ = 0.0; internalWas_ = true; }
        p0 = internalPpq_;
    } else { internalWas_ = false; p0 = 0.0; }
    if (run && !running_) {
        startPpq_ = p0;
        if (queued_ >= 0) { current_ = queued_; queued_ = -1; }
    }
    running_ = run;
    if (!run) { for (auto& s : step_) s = -1; endPpq_ = p0; return 0; }

    int count = 0;
    int split = n;
    if (queued_ >= 0) {
        const double sw = std::ceil(p0 / kBarBeats - 1e-9) * kBarBeats;
        split = std::clamp((int)std::ceil((sw - p0) * spb - 0.01), 0, n + 1);
    }
    const Pattern& cur = bank.patterns[(size_t)current_];
    count = collect(cur, kit, p0, spb, 0, std::min(split, n), swing, humanize, lockScale, n, out, count, maxHits);
    if (split < n) {
        current_ = queued_; queued_ = -1;
        count = collect(bank.patterns[(size_t)current_], kit, p0, spb, split, n, swing, humanize, lockScale, n, out, count, maxHits);
    }
    // Insertion sort, stable and allocation free (std::stable_sort may allocate).
    for (int i = 1; i < count; ++i) {
        const Hit h = out[i];
        int j = i - 1;
        while (j >= 0 && out[j].offset > h.offset) { out[j + 1] = out[j]; --j; }
        out[j + 1] = h;
    }

    const double p1 = p0 + n / spb;
    endPpq_ = p1;
    if (!host) internalPpq_ = p1;
    const Pattern& now = bank.patterns[(size_t)current_];
    for (int tr = 0; tr < kTracks; ++tr) {
        const auto& tp = now.tracks[(size_t)tr];
        const double sb = kRateBeats[std::clamp(tp.rate, 0, kNumRates - 1)];
        const int64_t k = (int64_t)std::floor(p1 / sb - 1e-9);
        step_[tr] = k < 0 ? 0 : (int)(k % std::clamp(tp.length, 1, kMaxSteps));
    }
    return count;
}

}  // namespace ultra
