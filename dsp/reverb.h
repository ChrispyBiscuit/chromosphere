#pragma once
#include "fdn/fdn.h"
#include "er/early_reflections.h"
#include <vector>

namespace cs {

enum class Quality { Eco, Normal, High };

struct ReverbParams {
    Quality quality = Quality::Normal;
    double size = 1.0;
    double t60[3] = {3.0, 2.0, 1.0};   // seconds at 125 Hz, 1 kHz, 8 kHz
    double modDepth = 1.0, modRateHz = 0.7;
    double preDelayMs = 0.0;
    float erLevel = 0.5f, tailLevel = 1.0f;
    float dry = 0.0f, wet = 1.0f;
    bool diffusion = true;
    uint64_t seed = 9;   // picked by tools/seed_sweep.py: best Q1/Q2/Q7 margins for all three qualities
};

// Full reverb: pre-delay -> (ER) and (input diffusion -> FDN tail) -> mix.
// configure() allocates (call off the audio thread); process() does not allocate,
// takes any block size, and reports zero latency.
class Reverb {
public:
    void configure(const ReverbParams& p, double fs, int maxBlock = 512);
    void reset();
    // Live controls (audio-thread safe, no allocation).
    static FdnConfig fdnConfigFor(const ReverbParams& p);
    static DecaySet designDecay(const ReverbParams& p, double fs) {   // worker thread
        return Fdn::designDecaySet(Fdn::computeDelays(fdnConfigFor(p), fs), p.t60, fs);
    }
    void setDecay(const DecaySet& d, int rampSamples) { fdn_.setDecay(d, rampSamples); setDiffusionDecay(d.t60mid, rampSamples); }
    void setModulation(double depth, double rateHz) { fdn_.setModulation(depth, rateHz); }
    void setDiffusionDecay(double t60mid, int rampSamples);   // allpass gains follow the decay time
    // Hold the tail. The input is muted while frozen (an open input would build up without limit).
    void setFreeze(bool on, int rampSamples) { fdn_.setFreeze(on, rampSamples); gate_.set(on ? 0.f : 1.f, rampSamples); }
    void setMix(float dry, float wet, float er, float tail, int rampSamples);
    void process(const float* inL, const float* inR, float* outL, float* outR, int n);

private:
    struct Allpass {
        std::vector<float> w;      // single state buffer: w[n] = x[n] + g w[n-d]; y = w[n-d] - g w[n]
        int d = 1, pos = 0;
        float g = 0.6f, gTarget = 0.6f, gStep = 0.f; int gLeft = 0;   // g ramps toward gTarget, advanced per chunk
        float tick(float in) {
            float wd = w[pos];
            float wn = in + g * wd;
            w[pos] = wn;
            if (++pos >= d) pos = 0;
            return wd - g * wn;
        }
    };
    struct Ramp {
        float cur = 0, target = 0, step = 0; int left = 0;
        void set(float t, int n) { target = t; left = n > 0 ? n : 0; if (left == 0) cur = t; else step = (t - cur) / (float)left; }
        float next() { if (left > 0) { cur += step; if (--left == 0) cur = target; } return cur; }
    };
    Ramp dry_, wet_, er_g_, tail_g_, gate_;
    void processChunk(const float* inL, const float* inR, float* outL, float* outR, int n);

    double fs_ = 48000;
    static float diffusionGain(double apMs, double t60mid);
    ReverbParams p_;
    int maxBlock_ = 512, preSize_ = 1, prePos_ = 0, preDelay_ = 0;
    std::vector<Allpass> apL_, apR_;
    std::vector<float> preL_, preR_;
    std::vector<float> dL_, dR_, tL_, tR_, eL_, eR_;
    Fdn fdn_;
    EarlyReflections er_;
};

}  // namespace cs
