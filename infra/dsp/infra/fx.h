#pragma once
// The effects rack: Drive -> Aurora (chorus) -> Echo (delay) -> Chromosphere reverb -> width, level, limiter.
// All stereo, in place. No allocation after prepare().
#include "blocks.h"
#include "params.h"
#include <atomic>
#include <memory>
#include <vector>

namespace fr { class EngineHost; }

namespace infra {

// Effective effect settings for one block (after the global modulation).
struct FxState {
    DriveMode driveMode = DriveMode::Soft;
    float drive = 0.f, driveTone = 0.7f, driveMix = 1.f;
    float driveLowCut = 0.f;                     // Hz: below it the drive passes the signal clean (0 = drive everything)
    float squash = 0.f;                          // OTT-style three-band compression amount, 0..1
    float monoBelow = 0.f;                       // Hz: stereo content below it is removed (0 = off)
    float chorusRate = 0.4f, chorusDepth = 0.5f, chorusMix = 0.f;
    double delaySamples = 0.0; float delayFeedback = 0.3f, delayDamp = 0.4f, delayMix = 0.f; bool pingPong = true;
    float reverbMix = 0.f;
    float width = 1.f, outGain = 1.f;
};

class Drive {
public:
    void prepare(double fs) { fs_ = (float)fs; reset(); }
    void reset() { for (auto& c : xlo_) for (auto& s : c) s.reset(); for (auto& c : xhi_) for (auto& s : c) s.reset(); lp_[0] = lp_[1] = 0.f; dcx_[0] = dcx_[1] = dcy_[0] = dcy_[1] = 0.f; hold_[0] = hold_[1] = 0.f; holdCount_ = 0.f; }
    void process(float* L, float* R, int n, const FxState& s);
private:
    float fs_ = 48000.f, lp_[2] = {}, dcx_[2] = {}, dcy_[2] = {}, hold_[2] = {}, holdCount_ = 0.f;
    Svf xlo_[2][2], xhi_[2][2];                  // sub-clean split: Linkwitz-Riley 4th order (two Butterworth stages)
};

// Squash: three-band upward and downward compression in the manner of OTT, the density and presence tool of modern
// bass and lead sound design. Bands split at 120 Hz and 2.5 kHz with complementary filters (they sum back exactly);
// each band is pushed down above its threshold and lifted below it (the lift is capped and stops below -60 dBFS, so
// silence stays silent), then made up and mixed in by the amount.
class Squash {
public:
    void prepare(double fs);
    void reset();
    void process(float* L, float* R, int n, float amount);
private:
    float fs_ = 48000.f, aLo_ = 0.f, aHi_ = 0.f, att_ = 0.f, rel_ = 0.f, mixSmooth_ = 0.f;
    float lo_[2][2] = {}, hi_[2][2] = {};        // split filter states
    float env_[3] = {};                           // band envelopes (stereo-linked)
};

class Chorus {
public:
    void prepare(double fs);
    void reset();
    void process(float* L, float* R, int n, const FxState& s);
    void skip(int n, const FxState& s);          // n samples of silence, cheaply (engine asleep): keeps the LFO in time
private:
    float fs_ = 48000.f, mixSmooth_ = 0.f;
    double phase_ = 0.0;                         // double: the same after n single steps as after one step of n (skip)
    std::vector<float> buf_[2];
    int pos_ = 0;
};

class Echo {
public:
    void prepare(double fs);
    void reset();
    void process(float* L, float* R, int n, const FxState& s);
private:
    float fs_ = 48000.f;
    std::vector<float> buf_[2];
    int pos_ = 0;
    double delay_ = -1.0;            // current (slewed) delay in samples
    float lp_[2] = {}, hp_[2] = {}, mixSmooth_ = 0.f;
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
    void process(float* L, float* R, int n, float mix);
private:
    std::unique_ptr<fr::EngineHost> host_;
    std::atomic<float> size_{1.f}, decay_{2.5f}, damp_{0.4f}, pre_{10.f};
    std::vector<float> inL_, inR_, wetL_, wetR_;
    float mixSmooth_ = 0.f;
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

}  // namespace infra
