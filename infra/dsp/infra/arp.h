#pragma once
// ORBIT ARP: tempo-synced arpeggiator with a 16-step lane (velocity / rest, transpose, ratchet) and probability.
// Times are absolute sample counts from the engine's clock. Steps are generated lazily, when the engine reaches
// them, so key changes inside a block are heard on the next step. No allocation after prepare().
#include "blocks.h"
#include "params.h"
#include <array>
#include <cstdint>

namespace infra {

struct ArpEvent {
    int64_t time = 0;
    bool on = false;
    int note = 60;
    float velocity = 1.f;
    float stepValue = 1.f;       // the step's lane velocity, for the Arp Step mod source
    uint32_t id = 0;
};

class Arpeggiator {
public:
    void reset();
    // Called at the start of every block. ppq is the host position (quarter notes) when playing.
    void beginBlock(const ArpParams& p, int64_t blockStart, double fs, double bpm, bool hostPlaying, double ppq);
    void endBlock(int n) { internalPpq_ = ppq0_ + n * bps_; expectPpq_ = internalPpq_; }
    void keyOn(int note, float velocity, int64_t now);
    void keyOff(int note, int64_t now);
    void clearKeys();            // all keys up and latch released
    // Time of the next thing to do (a step to generate or a queued note event); INT64_MAX when nothing.
    int64_t nextTime() const;
    // Generate the step due at or before `now` (if any) and pop every queued event with time <= now.
    // Returns the number of events written to out (at most maxOut).
    int process(int64_t now, ArpEvent* out, int maxOut);
    // Stop: drop pending note-ons, move pending note-offs to `now`.
    void stop(int64_t now);

    bool running() const { return running_; }
    int currentLane() const { return lane_; }          // -1 when idle
    int heldCount() const { return heldN_; }

private:
    void rebuildSequence();
    void fireStep(int64_t t);
    bool push(const ArpEvent& e);
    double beatAt(int64_t t) const { return ppq0_ + (double)(t - t0_) * bps_; }
    int64_t sampleAt(double beat) const { return t0_ + (int64_t)std::ceil((beat - ppq0_) / bps_ - 1e-9); }
    double stepBeats() const { return kDivisionBeats[std::clamp(p_.division, 0, kNumDivisions - 1)]; }
    double swungBeat(double grid) const;

    ArpParams p_{};
    // Held notes in the order they were played.
    struct Held { int note; float vel; };
    std::array<Held, 32> held_{};
    int heldN_ = 0, physical_ = 0;
    // Expanded sequence for the current mode.
    std::array<Held, 160> seq_{};
    int seqN_ = 0, seqPos_ = 0;
    bool dirty_ = true;
    ArpMode builtMode_ = ArpMode::Up; int builtOct_ = 1;

    // Clock.
    double fs_ = 48000.0, bps_ = 120.0 / 60.0 / 48000.0, ppq0_ = 0.0, internalPpq_ = 0.0, expectPpq_ = -1.0;
    int64_t t0_ = 0, blockLen_ = 0;
    bool hostPlaying_ = false;
    bool running_ = false;
    int64_t immediate_ = -1;     // a step to fire at this time (key press that started the arp)
    double nextGrid_ = 0.0;      // next step on the unswung grid, in beats
    int64_t stepCount_ = 0, playCount_ = 0;
    int lane_ = -1;
    uint32_t nextId_ = 1;
    Rng rng_;

    std::array<ArpEvent, 512> queue_{};
    int queueN_ = 0;
};

}  // namespace infra
