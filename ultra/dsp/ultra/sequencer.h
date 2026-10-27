#pragma once
// The step sequencer (PRD Section 6). A step's time is a pure function of the song position in quarter notes, so
// loops and bounces repeat exactly. process() turns one block into a list of hits at sample offsets.
#include "params.h"
#include <cstdint>

namespace ultra {

struct Transport {
    double bpm = 120.0;
    bool hasTempo = false;       // the host reported a tempo
    bool playing = false;        // the host transport runs
    double ppq = 0.0;            // host position at the start of the block, in quarter notes (when playing)
};

struct Hit {
    int offset = 0;              // sample in the block
    int track = 0;
    float velocity = 1.f;        // 0..1
    int pitch = 0;               // semitones
    float lock = 0.f;            // -1..1
};

class Sequencer {
public:
    void prepare(double fs) { fs_ = fs; reset(); }
    void reset();
    // Pattern requests from MIDI (notes 24..31); the Pattern parameter requests through process().
    void requestPattern(int p);
    // swing: effective 0.5..0.75; humanize 0..1; lockScale multiplies the lock lane. Returns the number of hits,
    // sorted by offset.
    int process(const PatternBank& bank, const KitParams& kit, float swing, float humanize, float lockScale,
                const Transport& t, bool internalPlay, int n, Hit* out, int maxHits);

    bool running() const { return running_; }
    int currentPattern() const { return current_; }
    int queuedPattern() const { return queued_; }
    double position() const { return endPpq_; }
    double bpm() const { return bpm_; }
    // The step each track is on (at the end of the last block), or -1 when stopped.
    int stepOf(int track) const { return step_[track]; }

    // Swing warp of a grid time (exposed for tests).
    static double swingWarp(double grid, double swing);

private:
    int collect(const Pattern& pat, const KitParams& kit, double p0, double spb, int sFrom, int sTo, float swing,
                float humanize, float lockScale, int n, Hit* out, int count, int maxHits);
    double fs_ = 48000.0;
    bool running_ = false, internalWas_ = false;
    double internalPpq_ = 0.0, startPpq_ = 0.0, endPpq_ = 0.0, bpm_ = 120.0;
    int current_ = 0, queued_ = -1, lastParam_ = -1;
    int step_[kTracks] = {-1, -1, -1, -1, -1, -1, -1, -1};
};

}  // namespace ultra
