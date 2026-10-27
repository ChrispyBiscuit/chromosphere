#pragma once
// SUB (PRD 4.4.3): sub-octaves from a flip-flop octave divider, and psychoacoustic bass ("Phantom").
// Works on one mono signal (the processed signal's low end is made mono by the engine).
//  - The input is band-limited to 25 Hz .. Range. Zero crossings with hysteresis (10% of the envelope) and a
//    minimum period toggle a flip-flop at half the input frequency, and a second one at a quarter.
//  - Clean output: the band-limited input multiplied by the flip-flop (a ring modulation whose strongest product
//    is the octave below), low-passed. Gritty output: the flip-flop square times the envelope. Purity blends them.
//  - Phantom: harmonics 2 to 4 of the band-limited input, from its analytic signal (the order-bank method).
#include "filters.h"
#include "params.h"

namespace muon {

class SubGenerator {
public:
    void prepare(double fs);
    void reset();
    // x: mono input; out: generated sub + phantom (mono), replaces out[0..n). Pitch of the last octave-down cycle
    // in Hz (0 = none) is kept for tests.
    void process(const float* x, float* out, int n, const Params& p);
    float lastPeriodHz() const { return lastHz_; }
private:
    double fs_ = 48000.0;
    Svf hp_, lpA_, lpB_;          // input band
    Svf clean1_[2], clean2_[2];   // output low-passes (4th order each)
    OnePole env_;
    Hilbert hil_;
    float range_ = -1.f;
    float ff1_ = 1.f, ff2_ = 1.f;
    bool armed_ = false;
    int since_ = 0;
    long long lastCross_ = 0;
    float lastHz_ = 0.f;
    long long t_ = 0;
    float gate_ = 0.f, gateEnv_ = 0.f, gateRel_ = 0.999f;
};

}  // namespace muon
