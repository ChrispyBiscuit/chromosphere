#pragma once
#include "common/simd.h"
#include <cstdint>
#include <vector>

namespace fr {

struct FdnConfig {
    int lines = 32;                 // 16 (Eco), 32 (Normal), 64 (High)
    double size = 1.0;              // scales all delay lengths
    double minMs = 15.0, maxMs = 80.0;
    double t60[3] = {3.0, 2.0, 1.0};  // seconds at 125 Hz, 1 kHz, 8 kHz
    double modDepth = 1.0;          // samples (std dev of the delay offset)
    double modRateHz = 0.7;
    uint64_t seed = 1;
};

// Decay filter coefficients for every line, designed off the audio thread and
// handed to Fdn::setDecay() (no allocation there).
struct DecaySet {
    std::vector<float> g0, b0, b1, a1;   // g0: [line]; others: [section][line]
    double t60mid = 2.0;                 // seconds at 1 kHz; the reverb uses it to set the input-diffusion gains
};

// Feedback delay network: Householder matrix, per-line 4-shelf decay filter,
// slow random delay modulation with cubic Lagrange reads.
// configure() allocates and may be slow; process() never allocates.
class Fdn {
public:
    void configure(const FdnConfig& cfg, double fs);
    void reset();
    // Stereo in -> stereo out, in-place safe. n may be any size >= 0.
    void process(const float* inL, const float* inR, float* outL, float* outR, int n);
    int lines() const { return n_; }
    // Delay lengths (samples) a configuration will use. Pure function.
    static std::vector<int> computeDelays(const FdnConfig& cfg, double fs);
    // Slow (tens of ms); allocates. Pure function: call on a worker thread.
    static DecaySet designDecaySet(const std::vector<int>& delays, const double t60[3], double fs);
    DecaySet designDecay(const double t60[3]) const { return designDecaySet(delay_, t60, fs_); }
    // Audio-thread safe. Interpolates from the current to the new coefficients. While frozen the set is
    // remembered and takes effect on unfreeze.
    void setDecay(const DecaySet& d, int rampSamples);
    // Hold the tail: every line gets a flat loop gain of 0.99999 (the tail lasts for hours and cannot grow).
    // Audio-thread safe. The caller mutes the input while frozen.
    void setFreeze(bool on, int rampSamples);
    bool frozen() const { return frozen_; }
    // Audio-thread safe; takes effect at the next control tick.
    void setModulation(double depthSamples, double rateHz);
    const std::vector<int>& delays() const { return delay_; }

private:
    static constexpr int kCtrl = 16;     // modulation control interval (samples)
    static constexpr int kSections = 4;
    static constexpr int kGuard = kCtrl + 4;  // mirrored samples past the ring end: a chunk's 4-point reads never wrap

    void refreshModulation();
    template <int N> void processN(const float* inL, const float* inR, float* outL, float* outR, int count);
    void rampTo(const DecaySet& d, int rampSamples);

    int n_ = 0, stride_ = 0, lineLen_ = 0, wp_ = 0, ctrlPos_ = 0, rampTicks_ = 0;   // stride_: ring length; lineLen_: ring + mirror
    double fs_ = 48000;
    float twoOverN_ = 0;
    double modK_ = 0, modNorm_ = 0, modDepth_ = 0;
    std::vector<int> delay_;
    std::vector<float> buf_;                        // n_ * lineLen_
    std::vector<float> g0_, b0_, b1_, a1_, st_;     // SoA: [section][line]
    DecaySet target_, step_;                        // coefficient ramp (preallocated)
    DecaySet normal_, freeze_;                      // decay in use when not frozen / the frozen set
    bool frozen_ = false;
    std::vector<float> inL_, inR_, tapL_, tapR_;    // per-line gains
    std::vector<float> modCur_, modStep_, modNext_, modState_, y_, z_;
    std::vector<float> delayF_, yBlk_, vBlk_;           // chunk buffers, [t][line]
    std::vector<float> wa_, wb_, wc_, wd_;              // interpolation weights for the current control period
    std::vector<int> di_;
    std::vector<uint64_t> rngState_;
};

}  // namespace fr
