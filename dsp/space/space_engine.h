#pragma once
// Acoustic engine: a physical room (space/room_model) rendered as
//   early part  - the room's first `earlyMs` (image sources, scattering, air, directivity), true stereo,
//                 through the zero-latency partitioned convolver;
//   late field  - an FDN whose delays follow the room's mean free path and whose decay follows the
//                 room's Eyring T60 per band, level- and spectrum-matched to the statistical late energy;
//   room modes  - below the Schroeder frequency of small rooms, a bank of resonators at the room's
//                 modal frequencies with the source/listener coupling of each mode (the tail is
//                 high-passed there);
//   direct      - the four source/microphone paths (delay, 1/r, air, directivity), at its own level.
// configure() allocates and is slow (worker thread); process() never allocates.
#include "space/room_model.h"
#include "conv/partitioned_convolver.h"
#include "common/delay_line.h"
#include "fdn/fdn.h"
#include <vector>

namespace cs {

struct SpaceParams {
    RoomSpec room;
    int lines = 32;              // FDN size (16 Eco, 32 Normal, 64 High)
    double preDelayMs = 0;       // extra delay of the reverb (not the direct sound)
    uint64_t seed = 9;
};

class SpaceEngine {
public:
    void configure(const SpaceParams& p, double fs, int maxBlock);
    void process(const float* inL, const float* inR, float* outL, float* outR, int n);
    void setLevels(float er, float tail, int ramp) { er_.set(er, ramp); tail_.set(tail, ramp); }
    void setDirect(float g, int ramp) { direct_.set(g, ramp); }
    void setModulation(double depth, double rate) { fdn_.setModulation(depth, rate); }
    void setDecay(const DecaySet& d, int ramp) { fdn_.setDecay(d, ramp); }
    void setFreeze(bool on, int ramp) { fdn_.setFreeze(on, ramp); gate_.set(on ? 0.f : 1.f, ramp); }
    const RoomRender& render() const { return render_; }

    static FdnConfig fdnConfig(const SpaceParams& p, const RoomRender& r);
    // Decay set from the room's T60 curve (worker thread). Pure.
    static DecaySet designDecay(const SpaceParams& p, double fs);

private:
    struct Ramp {
        float cur = 1, target = 1, step = 0; int left = 0;
        void set(float t, int n) { target = t; left = n > 0 ? n : 0; if (left == 0) cur = t; else step = (t - cur) / (float)left; }
        float next() { if (left > 0) { cur += step; if (--left == 0) cur = target; } return cur; }
    };
    struct Biquad { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; float z1[2] = {}, z2[2] = {}; };
    void chunk(const float* inL, const float* inR, float* outL, float* outR, int n);
    void calibrateTail(const SpaceParams& p, const DecaySet& ds);

    double fs_ = 48000;
    int maxBlock_ = 512;
    RoomRender render_;
    PartitionedConvolver conv_;
    DelayLine pre_, feed_;
    Fdn fdn_;
    float tailMatch_ = 1.f;                  // FDN gain that matches the statistical late energy
    float shLow_[3] = {1, 0, 0}, shHigh_[3] = {1, 0, 0};   // first-order shelves b0, b1, a1 (spectral match)
    float shState_[2][2] = {};
    bool highPass_ = false;
    Biquad hp_[2];                          // two cascaded 2nd-order sections: 4th-order Butterworth, applied to both channels
    float mixA_ = 1.f, mixB_ = 0.f;         // late-field coherence matrix
    // Modes (SoA).
    int nModes_ = 0;
    std::vector<float> mRe_, mIm_, mCr_, mCi_, mInL_, mInR_, mOutL_, mOutR_;
    // Direct paths: integer delays from a short history, one-pole air filter per path.
    int dirDelay_[4] = {}, histLen_ = 1, histPos_ = 0;
    float dirGain_[4] = {}, dirLp_[4] = {}, dirState_[4] = {};
    std::vector<float> histL_, histR_;
    Ramp er_, tail_, direct_, gate_;
    std::vector<float> gL_, gR_, xL_, xR_, eL_, eR_, fL_, fR_, tL_, tR_;
};

}  // namespace cs
