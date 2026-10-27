#pragma once
// The master bus: Drive -> Glue on the dry mix; Echo and the Chromosphere reverb as send effects; then EQ, width,
// level and the limiter. Drive, Echo, ReverbStage and Limiter come from INFRA (infra/dsp/infra/fx.*); Echo and
// ReverbStage here return only the wet signal, since they sit on sends. All stereo. No allocation after prepare().
#include "blocks.h"
#include "params.h"
#include <atomic>
#include <memory>
#include <vector>

namespace fr { class EngineHost; }

namespace ultra {

class Drive {
public:
    void prepare(double fs) { fs_ = (float)fs; reset(); }
    void reset() { lp_[0] = lp_[1] = 0.f; dcx_[0] = dcx_[1] = dcy_[0] = dcy_[1] = 0.f; hold_[0] = hold_[1] = 0.f; holdCount_ = 0.f; }
    void process(float* L, float* R, int n, DriveMode mode, float amount, float tone, float mix);
private:
    float fs_ = 48000.f, lp_[2] = {}, dcx_[2] = {}, dcy_[2] = {}, hold_[2] = {}, holdCount_ = 0.f;
};

// Feed-forward bus compressor with a soft knee; the detector follows the louder channel.
class Glue {
public:
    void prepare(double fs) { fs_ = (float)fs; reset(); }
    void reset() { envDb_ = -120.f; grDb_ = 0.f; }
    void process(float* L, float* R, int n, float threshDb, float ratio, float attackMs, float releaseMs, float makeupDb, float mix);
    float reductionDb() const { return grDb_; }
private:
    float fs_ = 48000.f, envDb_ = -120.f, grDb_ = 0.f;
};

class Echo {
public:
    void prepare(double fs);
    void reset();
    // in: the send bus; out: the wet signal only (may alias in).
    void process(const float* inL, const float* inR, float* outL, float* outR, int n,
                 double delaySamples, float feedback, float damp, bool pingPong);
private:
    float fs_ = 48000.f;
    std::vector<float> buf_[2];
    int pos_ = 0;
    double delay_ = -1.0;
    float lp_[2] = {}, hp_[2] = {};
};

// The Chromosphere FDN reverb (algorithmic mode) through fr::EngineHost, which rebuilds engines on its worker
// thread when size or pre-delay change and designs decay changes there too.
class ReverbStage {
public:
    ReverbStage();
    ~ReverbStage();
    void prepare(double fs, int maxBlock);
    void release();
    void setShape(float size, float decay, float damp, float preDelayMs);   // audio thread; read by the worker
    // Wet only, out may alias in.
    void process(const float* inL, const float* inR, float* outL, float* outR, int n);
private:
    std::unique_ptr<fr::EngineHost> host_;
    std::atomic<float> size_{1.f}, decay_{1.6f}, damp_{0.4f}, pre_{8.f};
    bool prepared_ = false;
};

class Limiter {
public:
    void prepare(double fs) { release_ = (float)std::exp(-1.0 / (0.08 * fs)); gain_ = 1.f; }
    void process(float* L, float* R, int n) {
        constexpr float thr = 0.891f;            // -1 dBFS
        for (int i = 0; i < n; ++i) {
            const float pk = std::max(std::abs(L[i]), std::abs(R[i]));
            const float target = pk > thr ? thr / pk : 1.f;
            gain_ = target < gain_ ? target : target + (gain_ - target) * release_;
            L[i] *= gain_; R[i] *= gain_;
        }
    }
    float reduction() const { return gain_; }
private:
    float gain_ = 1.f, release_ = 0.999f;
};

}  // namespace ultra
