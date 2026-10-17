#include "reverb.h"
#include "fdn/decay_design.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static std::vector<float> render(const fr::ReverbParams& p, double fs, int len, int block, bool impulse, unsigned seed = 1, float amp = 1.f) {
    fr::Reverb rv; rv.configure(p, fs, block);
    std::vector<float> in(len, 0.f), out(2 * len);
    if (impulse) in[0] = amp;
    else { unsigned s = seed; for (float& x : in) { s = s * 1664525u + 1013904223u; x = ((s >> 8) * (1.0f / 8388608.f) - 1.f) * amp; } }
    std::vector<float> oL(len), oR(len);
    for (int off = 0; off < len; off += block) {
        int n = std::min(block, len - off);
        rv.process(&in[off], &in[off], &oL[off], &oR[off], n);
    }
    for (int i = 0; i < len; ++i) { out[2 * i] = oL[i]; out[2 * i + 1] = oR[i]; }
    return out;
}

int main() {
    const double fs = 48000;
    // 1. Decay filter design hits the target T60 curve (within 0.3 dB per pass).
    {
        double t60[3] = {3.0, 2.0, 1.0}, delay = 1500;
        fr::DecayFilter d = fr::designDecayFilter(delay, t60, fs);
        for (double f : {125., 250., 500., 1000., 2000., 4000., 8000.}) {
            double want = -60.0 * delay / (fs * fr::t60Target(f, t60));
            CHECK(std::fabs(fr::decayFilterDb(d, f, fs) - want) < 0.3, "f=%g got %g want %g", f, fr::decayFilterDb(d, f, fs), want);
        }
        for (double f : {20., 100., 1000., 10000., 20000.}) CHECK(fr::decayFilterDb(d, f, fs) < 0.0, "filter gain >= 1 at %g Hz", f);
    }
    // 2. Silence in -> exact silence out (Q8).
    {
        fr::ReverbParams p; p.erLevel = 0.5f;
        auto y = render(p, fs, 48000, 128, false, 1, 0.f);
        float m = 0; for (float v : y) m = std::max(m, std::fabs(v));
        CHECK(m == 0.f, "silent input gave %g", m);
    }
    // 3. Block-size independence: same output for any host block size.
    {
        fr::ReverbParams p; p.erLevel = 0.5f;
        auto a = render(p, fs, 20000, 64, false, 7, 0.3f);
        auto b = render(p, fs, 20000, 509, false, 7, 0.3f);
        auto c = render(p, fs, 20000, 1, false, 7, 0.3f);
        bool same = a == b && a == c;
        CHECK(same, "output depends on block size");
    }
    // 4. Determinism (Q11).
    {
        fr::ReverbParams p; p.erLevel = 0.5f;
        CHECK(render(p, fs, 20000, 128, false, 3, 0.3f) == render(p, fs, 20000, 128, false, 3, 0.3f), "not deterministic");
    }
    // 5. Stability: no NaN/Inf and bounded energy across extreme settings; decay after input stops.
    {
        const double sizes[] = {0.3, 1.0, 3.0};
        const double t60s[][3] = {{0.2, 0.2, 0.1}, {3, 2, 1}, {20, 20, 20}};
        const fr::Quality qs[] = {fr::Quality::Eco, fr::Quality::Normal, fr::Quality::High};
        for (double size : sizes) for (auto& t : t60s) for (auto q : qs) {
            fr::ReverbParams p; p.size = size; p.quality = q; p.erLevel = 0.5f;
            for (int k = 0; k < 3; ++k) p.t60[k] = t[k];
            p.modDepth = 4.0;
            auto y = render(p, fs, 24000, 256, false, 5, 0.9f);
            bool ok = true; float m = 0;
            for (float v : y) { if (!std::isfinite(v)) ok = false; m = std::max(m, std::fabs(v)); }
            CHECK(ok && m < 20.f, "size %g t60 %g q %d peak %g", size, t[0], (int)q, m);
        }
    }
    // 6. Decay: tail energy at 1.5 s is far below the first 100 ms (T60 3 s => about 30 dB down).
    {
        fr::ReverbParams p;
        auto y = render(p, fs, 96000, 128, true);
        auto e = [&](int a, int b) { double s = 0; for (int i = a; i < b; ++i) s += (double)y[2 * i] * y[2 * i]; return s; };
        double early = e(0, 4800), late = e(72000, 76800);
        CHECK(10 * std::log10(late / early) < -20, "decay too slow: %g dB", 10 * std::log10(late / early));
    }
    // 7. Live controls: setDecay to identical values changes nothing; a decay change and a
    //    mix change are free of jumps; the new decay takes effect.
    {
        const int len = 96000, block = 128;
        auto run = [&](bool changeDecay, bool sameDecay, bool changeMix) {
            fr::ReverbParams p; p.erLevel = 0.5f;
            fr::Reverb rv; rv.configure(p, fs, block);
            fr::ReverbParams q = p; if (changeDecay) q.t60[0] = q.t60[1] = q.t60[2] = 0.4;
            fr::DecaySet ds = fr::Reverb::designDecay(sameDecay ? p : q, fs);
            std::vector<float> in(len), oL(len), oR(len);
            unsigned s = 9; for (float& x : in) { s = s * 1664525u + 1013904223u; x = ((s >> 8) * (1.0f / 8388608.f) - 1.f) * 0.3f; }
            std::fill(in.begin() + 30000, in.end(), 0.f);   // let the tail ring out
            for (int off = 0; off < len; off += block) {
                if (off == 24064 && (changeDecay || sameDecay)) rv.setDecay(ds, 4800);
                if (off == 24064 && changeMix) rv.setMix(0.f, 0.f, 0.5f, 1.f, 960);
                rv.process(&in[off], &in[off], &oL[off], &oR[off], block);
            }
            return oL;
        };
        auto base = run(false, false, false), same = run(false, true, false);
        double maxDiff = 0; for (int i = 0; i < len; ++i) maxDiff = std::max(maxDiff, (double)std::fabs(base[i] - same[i]));
        CHECK(maxDiff < 1e-4, "setDecay with identical values changed output by %g", maxDiff);
        auto dec = run(true, false, false);
        double maxStep = 0, maxStepBase = 0;
        for (int i = 24064; i < 32000; ++i) { maxStep = std::max(maxStep, (double)std::fabs(dec[i] - dec[i - 1])); maxStepBase = std::max(maxStepBase, (double)std::fabs(base[i] - base[i - 1])); }
        CHECK(maxStep < 2.0 * maxStepBase + 1e-3, "decay change made a jump: %g vs %g", maxStep, maxStepBase);
        auto e = [&](const std::vector<float>& y, int a, int b) { double s = 0; for (int i = a; i < b; ++i) s += (double)y[i] * y[i]; return s; };
        CHECK(e(dec, 60000, 96000) < 0.5 * e(base, 60000, 96000), "new decay not applied");
        auto mx = run(false, false, true);
        double ms = 0, mb = 0; for (int i = 24064; i < 26000; ++i) { ms = std::max(ms, (double)std::fabs(mx[i] - mx[i - 1])); mb = std::max(mb, (double)std::fabs(base[i] - base[i - 1])); }
        CHECK(ms < 1.5 * mb, "mix change jump %g vs %g", ms, mb);
        CHECK(e(mx, 40000, 96000) == 0.0 || e(mx, 40000, 96000) < 1e-9, "wet=0 not silent");
    }
    // 8. Freeze: the tail is held (no decay, no growth), the input is ignored, and unfreezing restores the decay.
    {
        const int len = 48000 * 12, block = 128;
        fr::ReverbParams p; p.erLevel = 0.5f;
        fr::Reverb rv; rv.configure(p, fs, block);
        std::vector<float> in(len), oL(len), oR(len);
        unsigned s = 5; for (float& x : in) { s = s * 1664525u + 1013904223u; x = ((s >> 8) * (1.0f / 8388608.f) - 1.f) * 0.3f; }
        std::fill(in.begin() + 24000, in.end(), 0.f);                  // 0.5 s of noise, then silence
        std::vector<float> in2 = in;                                    // second run: loud noise while frozen
        for (int i = 96000; i < 144000; ++i) in2[i] = 0.9f * (i % 2 ? 1.f : -1.f);
        auto run = [&](const std::vector<float>& src) {
            fr::Reverb r; r.configure(p, fs, block);
            std::vector<float> o(len), o2(len);
            for (int off = 0; off < len; off += block) {
                if (off == 31360) r.setFreeze(true, 2400);              // 0.65 s
                if (off == 480000 - 128 * 0) r.setFreeze(false, 2400);  // 10 s
                r.process(&src[off], &src[off], &o[off], &o2[off], block);
            }
            return o;
        };
        auto a = run(in), b = run(in2);
        auto e = [&](const std::vector<float>& y, int x0, int x1) { double v = 0; for (int i = x0; i < x1; ++i) v += (double)y[i] * y[i]; return v / (x1 - x0); };
        const double held0 = e(a, 48000, 52800), held1 = e(a, 400000, 404800);
        CHECK(held1 > 0.5 * held0 && held1 < 1.5 * held0, "frozen tail changed: %g -> %g", held0, held1);
        CHECK(std::fabs(10 * std::log10(e(b, 400000, 404800) / held1)) < 1.0, "input leaked into the frozen tail");
        CHECK(e(a, 560000, 576000) < 0.01 * held1, "no decay after unfreeze: %g vs %g", e(a, 560000, 576000), held1);
        bool fin = true; for (float v : a) fin = fin && std::isfinite(v);
        CHECK(fin, "non-finite while frozen");
    }
    std::printf(failures ? "%d failure(s)\n" : "all tests passed\n", failures);
    return failures ? 1 : 0;
}
