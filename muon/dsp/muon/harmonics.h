#pragma once
// The harmonics section of one band, one channel (PRD 4.4). Runs oversampled.
//
// Order bank: the source stream's analytic signal a + j b (Hilbert pair at the base rate, both parts upsampled)
// gives an instantaneous amplitude A = |a + j b| and phase. A * T_n(a / A) = A cos(n phase) is the source moved
// to n times its frequency, for any level and at once (no envelope follower to ripple). The weighted sum over
// n = 2..8 is added to the band; the H1 weight scales the source itself (its dry fundamental).
// Drive: one of six curves with first-order antiderivative anti-aliasing, bias, tone tilt and mix.
// What the section adds (the difference from its input) passes a 5 Hz DC blocker, so a neutral section is just
// the up/down sampling.
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
    DriveCurve curve = DriveCurve::Soft;
    float bias = 0.f, tone = 0.5f;
};

class HarmonicBand {
public:
    void prepare(double fs, int osFactor, bool linear, int maxBlock);
    void reset();
    int osFactor() const { return up_[0].factor(); }
    // xSrc: the stream the section works on; xOther: the rest of the band (added back untouched). n <= maxBlock.
    void process(const float* xSrc, const float* xOther, float* out, int n, const HarmControl& c);

private:
    double fsHi_ = 192000.0;
    Hilbert hil_;
    Resampler up_[4];                    // source, other, a, b (each used for its up direction only)
    Resampler down_;
    std::vector<float> a_, b_, upS_, upO_, upA_, upB_, y_;
    Adaa adaa_;
    OnePole tape_, tilt_;
    DcBlock dc_;
    DriveCurve lastCurve_ = DriveCurve::Soft;
    bool bankWasOn_ = false;
};

}  // namespace muon
