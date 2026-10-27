#include "arp.h"
#include <climits>
#include <cstdint>

namespace infra {

void Arpeggiator::reset() {
    heldN_ = physical_ = 0; seqN_ = seqPos_ = 0; dirty_ = true;
    running_ = false; immediate_ = -1; nextGrid_ = 0.0; stepCount_ = playCount_ = 0; lane_ = -1;
    queueN_ = 0; internalPpq_ = 0.0; expectPpq_ = -1.0; hostPlaying_ = false;
    rng_.seed(0x5eed1234u);
}

double Arpeggiator::swungBeat(double grid) const {
    const double st = stepBeats();
    const long long idx = std::llround(grid / st);
    return grid + ((idx & 1) ? std::clamp(p_.swing, 0.f, 1.f) * 0.5 * st : 0.0);
}

void Arpeggiator::beginBlock(const ArpParams& p, int64_t blockStart, double fs, double bpm, bool hostPlaying, double ppq) {
    const bool divChanged = p.division != p_.division;
    if (p.mode != builtMode_ || p.octaves != builtOct_) dirty_ = true;
    const bool latchReleased = p_.latch && !p.latch;
    p_ = p;
    fs_ = fs;
    bps_ = std::max(1.0, bpm) / 60.0 / fs;
    const double st = stepBeats();
    t0_ = blockStart;
    if (hostPlaying) {
        const bool jumped = !hostPlaying_ || std::abs(ppq - expectPpq_) > 1e-3;
        ppq0_ = ppq;
        if (running_ && (jumped || divChanged)) nextGrid_ = std::ceil(ppq / st - 1e-9) * st;
    } else {
        ppq0_ = internalPpq_;
        if (running_ && divChanged) nextGrid_ = std::ceil(ppq0_ / st - 1e-9) * st;
    }
    hostPlaying_ = hostPlaying;
    // Releasing latch with no keys down lets the latched chord go.
    if (latchReleased && physical_ == 0) clearKeys();
}

void Arpeggiator::keyOn(int note, float velocity, int64_t now) {
    if (p_.latch && physical_ == 0) { heldN_ = 0; }       // a new chord replaces the latched one
    ++physical_;
    for (int i = 0; i < heldN_; ++i) if (held_[(size_t)i].note == note) { held_[(size_t)i].vel = velocity; return; }
    if (heldN_ < (int)held_.size()) held_[(size_t)heldN_++] = {note, velocity};
    dirty_ = true;
    if (!running_) {
        running_ = true;
        stepCount_ = playCount_ = 0; seqPos_ = 0;
        const double st = stepBeats();
        if (hostPlaying_) {
            const double b = beatAt(now);
            nextGrid_ = std::ceil((b + 0.5 * st) / st) * st;   // no stub step right after the first one
        } else {
            // Restart the internal clock so the first step is on a grid line.
            ppq0_ = -(double)(now - t0_) * bps_;
            nextGrid_ = st;
        }
        immediate_ = now;
    }
}

void Arpeggiator::keyOff(int note, int64_t now) {
    physical_ = std::max(0, physical_ - 1);
    if (p_.latch) return;
    for (int i = 0; i < heldN_; ++i)
        if (held_[(size_t)i].note == note) {
            for (int j = i; j + 1 < heldN_; ++j) held_[(size_t)j] = held_[(size_t)j + 1];
            --heldN_; dirty_ = true; break;
        }
    if (heldN_ == 0) stop(now);
}

void Arpeggiator::clearKeys() { heldN_ = 0; physical_ = 0; dirty_ = true; }

void Arpeggiator::stop(int64_t now) {
    running_ = false; immediate_ = -1; lane_ = -1;
    int w = 0;
    for (int i = 0; i < queueN_; ++i) {
        ArpEvent e = queue_[(size_t)i];
        if (e.on) continue;
        e.time = std::min(e.time, now);
        queue_[(size_t)w++] = e;
    }
    queueN_ = w;
    std::sort(queue_.begin(), queue_.begin() + queueN_, [](const ArpEvent& a, const ArpEvent& b) { return a.time < b.time; });
}

int64_t Arpeggiator::nextTime() const {
    int64_t t = INT64_MAX;
    if (running_) {
        if (immediate_ >= 0) t = std::min(t, immediate_);
        t = std::min(t, sampleAt(swungBeat(nextGrid_)));
    }
    if (queueN_ > 0) t = std::min(t, queue_[0].time);
    return t;
}

bool Arpeggiator::push(const ArpEvent& e) {
    if (queueN_ >= (int)queue_.size()) return false;
    int i = queueN_++;
    while (i > 0 && queue_[(size_t)i - 1].time > e.time) { queue_[(size_t)i] = queue_[(size_t)i - 1]; --i; }
    queue_[(size_t)i] = e;
    return true;
}

void Arpeggiator::rebuildSequence() {
    dirty_ = false; builtMode_ = p_.mode; builtOct_ = p_.octaves;
    seqN_ = 0;
    if (heldN_ == 0) return;
    // Sorted copy of the held notes.
    std::array<Held, 32> sorted{};
    for (int i = 0; i < heldN_; ++i) sorted[(size_t)i] = held_[(size_t)i];
    std::sort(sorted.begin(), sorted.begin() + heldN_, [](const Held& a, const Held& b) { return a.note < b.note; });
    const int oct = std::clamp(p_.octaves, 1, 4);
    std::array<Held, 128> up{};
    int upN = 0;
    for (int o = 0; o < oct; ++o) for (int i = 0; i < heldN_; ++i) up[(size_t)upN++] = {sorted[(size_t)i].note + 12 * o, sorted[(size_t)i].vel};
    auto add = [&](const Held& h) { if (seqN_ < (int)seq_.size()) seq_[(size_t)seqN_++] = h; };
    auto addRange = [&](const std::array<Held, 128>& a, int n, bool reverse, bool dropEnds) {
        for (int k = 0; k < n; ++k) {
            const int i = reverse ? n - 1 - k : k;
            if (dropEnds && (k == 0 || k == n - 1)) continue;
            add(a[(size_t)i]);
        }
    };
    std::array<Held, 128> conv{};
    int convN = 0;
    auto makeConverge = [&]() {
        int l = 0, r = upN - 1;
        while (l <= r) { conv[(size_t)convN++] = up[(size_t)l]; if (l != r) conv[(size_t)convN++] = up[(size_t)r]; ++l; --r; }
    };
    switch (p_.mode) {
        case ArpMode::Up: case ArpMode::Random: addRange(up, upN, false, false); break;
        case ArpMode::Down: addRange(up, upN, true, false); break;
        case ArpMode::UpDown: addRange(up, upN, false, false); if (upN > 2) addRange(up, upN, true, true); break;
        case ArpMode::DownUp: addRange(up, upN, true, false); if (upN > 2) addRange(up, upN, false, true); break;
        case ArpMode::Played:
            for (int o = 0; o < oct; ++o) for (int i = 0; i < heldN_; ++i) add({held_[(size_t)i].note + 12 * o, held_[(size_t)i].vel});
            break;
        case ArpMode::Converge: makeConverge(); addRange(conv, convN, false, false); break;
        case ArpMode::Diverge: makeConverge(); addRange(conv, convN, true, false); break;
        case ArpMode::Orbit: makeConverge(); addRange(conv, convN, false, false); if (convN > 2) addRange(conv, convN, true, true); break;
        case ArpMode::Pulsar:
            if (upN == 1) add(up[0]);
            for (int i = 1; i < upN; ++i) { add(up[0]); add(up[(size_t)i]); }
            break;
        case ArpMode::Chord: for (int i = 0; i < heldN_; ++i) add(sorted[(size_t)i]); break;
    }
    if (seqN_ > 0) seqPos_ %= seqN_;
}

void Arpeggiator::fireStep(int64_t t) {
    if (heldN_ == 0) { running_ = false; lane_ = -1; return; }
    if (dirty_) rebuildSequence();
    if (seqN_ == 0) return;
    const int len = std::clamp(p_.length, 1, kArpSteps);
    const int lane = (int)(stepCount_ % len);
    ++stepCount_;
    lane_ = lane;
    const ArpStep& st = p_.steps[(size_t)lane];
    if (st.velocity <= 0.001f) return;
    if (p_.probability < 0.999f && rng_.uni() >= p_.probability) return;
    const double stepSamples = stepBeats() / bps_;
    const int r = std::clamp(st.ratchet, 1, 4);
    const double sub = stepSamples / r;
    const int64_t gateLen = std::max<int64_t>(1, (int64_t)(std::clamp(p_.gate, 0.02f, 2.f) * sub));
    auto emit = [&](int note, float vel) {
        note = std::clamp(note + st.transpose, 0, 127);
        for (int i = 0; i < r; ++i) {
            if (queueN_ + 2 > (int)queue_.size()) return;
            const int64_t on = t + (int64_t)(i * sub);
            const uint32_t id = nextId_;
            nextId_ = (nextId_ + 1) & 0x3fffffffu; if (nextId_ == 0) nextId_ = 1;
            ArpEvent e; e.time = on; e.on = true; e.note = note; e.velocity = std::clamp(vel * st.velocity, 0.02f, 1.f); e.stepValue = st.velocity; e.id = id;
            push(e);
            e.on = false; e.time = on + gateLen;
            push(e);
        }
    };
    if (p_.mode == ArpMode::Chord) {
        const int o = (int)(playCount_ % std::clamp(p_.octaves, 1, 4));
        for (int i = 0; i < seqN_; ++i) emit(seq_[(size_t)i].note + 12 * o, seq_[(size_t)i].vel);
    } else {
        const Held& h = p_.mode == ArpMode::Random ? seq_[(size_t)(rng_.nextU() % (uint32_t)seqN_)] : seq_[(size_t)(seqPos_++ % seqN_)];
        if (seqPos_ >= seqN_) seqPos_ = 0;
        emit(h.note, h.vel);
    }
    ++playCount_;
}

int Arpeggiator::process(int64_t now, ArpEvent* out, int maxOut) {
    if (running_) {
        if (immediate_ >= 0 && immediate_ <= now) { const int64_t t = immediate_; immediate_ = -1; fireStep(t); }
        int guard = 0;
        while (running_ && guard++ < 64) {
            const int64_t t = sampleAt(swungBeat(nextGrid_));
            if (t > now) break;
            fireStep(t);
            nextGrid_ += stepBeats();
        }
    }
    int n = 0, i = 0;
    while (i < queueN_ && queue_[(size_t)i].time <= now && n < maxOut) out[n++] = queue_[(size_t)i++];
    if (i > 0) { for (int j = i; j < queueN_; ++j) queue_[(size_t)(j - i)] = queue_[(size_t)j]; queueN_ -= i; }
    return n;
}

}  // namespace infra
