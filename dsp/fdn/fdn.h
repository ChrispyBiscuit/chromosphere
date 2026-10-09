#pragma once
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
    const std::vector<int>& delays() const { return delay_; }

private:
    static constexpr int kCtrl = 16;     // modulation control interval (samples)
    static constexpr int kSections = 4;

    void refreshModulation();

    int n_ = 0, stride_ = 0, wp_ = 0, ctrlPos_ = 0;
    float twoOverN_ = 0;
    double modK_ = 0, modNorm_ = 0, modDepth_ = 0;
    std::vector<int> delay_;
    std::vector<float> buf_;                        // n_ * stride_
    std::vector<float> g0_, b0_, b1_, a1_, st_;     // SoA: [section][line]
    std::vector<float> inL_, inR_, tapL_, tapR_;    // per-line gains
    std::vector<float> modCur_, modStep_, modNext_, modState_, y_, z_;
    std::vector<uint64_t> rngState_;
};

}  // namespace fr
