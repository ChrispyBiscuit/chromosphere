#pragma once
// The harmonics section of one band, both channels (PRD 4.4). Runs oversampled.
//
// Order bank: the source stream's analytic signal a + j b (Hilbert pair at the base rate, both parts upsampled)
// gives an instantaneous amplitude A = |a + j b| and phase. A * T_n(a / A) = A cos(n phase) is the source moved
// to n times its frequency, for any level and at once (no envelope follower to ripple). The weighted sum over
// n = 2..8 is added to the band; the H1 weight scales the source itself (its dry fundamental).
// Drive: one of six curves with first-order antiderivative anti-aliasing, bias, tone tilt and mix.
// What the section adds (the difference from its input) passes a 5 Hz DC blocker, so a neutral section is just
// the up/down sampling.
// The eight upsampled signals (source, other, a, b for each channel) run as one interleaved 8-lane chain, and the
// two outputs as a 2-lane chain, so the serial all-pass filters vectorise.
#include "filters.h"
#include "params.h"

namespace muon {

// Per-sample control values for one block (base rate), filled by the engine.
struct HarmControl {
    const float* amount = nullptr;        // bank amount (smoothed)
    const float* drive = nullptr;         // drive 0..1 (smoothed)
    const float* driveMix = nullptr;
    std::array<float, kOrders> weights{}; // signed weights, H1..H8 (H1 relative: 1 = unchanged)
    bool bank = false, driveOn = false, hasOther = false;
    bool active[2] = {true, true};        // per channel (mid/side: the side passes untouched)
    DriveCurve curve = DriveCurve::Soft;
    float bias = 0.f, tone = 0.5f;
};

class HarmonicBand {
public:
    void prepare(double fs, int osFactor, bool linear, int maxBlock);
    void reset();
    int osFactor() const { return up_.factor(); }
    // src[c]: the stream the section works on; oth[c]: the rest of the band (added back untouched). n <= maxBlock.
    void process(const float* const src[2], const float* const oth[2], float* const out[2], int n, const HarmControl& c);

private:
    static constexpr int kLanes = 8;     // srcL srcR othL othR aL aR bL bR
    double fsHi_ = 192000.0;
    Hilbert hil_[2];
    ResamplerN<kLanes> up_;
    ResamplerN<2> down_;
    std::vector<float> in_, hi_, y_, out_, gain_, post_, s_, x_, f_, d_, a_, b_, ctl_;
    Adaa adaa_[2];
    OnePole tape_[2], tilt_[2];
    DcBlock dc_[2];
    bool bankWasOn_ = false;
};

}  // namespace muon
