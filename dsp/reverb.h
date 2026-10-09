#pragma once
#include "fdn/fdn.h"
#include "er/early_reflections.h"
#include <vector>

namespace fr {

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
    uint64_t seed = 1;
};

// Full reverb: pre-delay -> (ER) and (input diffusion -> FDN tail) -> mix.
// configure() allocates (call off the audio thread); process() does not allocate,
// takes any block size, and reports zero latency.
class Reverb {
public:
    void configure(const ReverbParams& p, double fs, int maxBlock = 512);
    void reset();
    void process(const float* inL, const float* inR, float* outL, float* outR, int n);

private:
    struct Allpass {
        std::vector<float> x, y;
        int d = 1, pos = 0;
        float g = 0.6f;
        float tick(float in) {
            float xd = x[pos], yd = y[pos];
            float out = -g * in + xd + g * yd;
            x[pos] = in; y[pos] = out;
            if (++pos >= d) pos = 0;
            return out;
        }
    };
    void processChunk(const float* inL, const float* inR, float* outL, float* outR, int n);

    ReverbParams p_;
    int maxBlock_ = 512, preSize_ = 1, prePos_ = 0, preDelay_ = 0;
    std::vector<Allpass> apL_, apR_;
    std::vector<float> preL_, preR_;
    std::vector<float> dL_, dR_, tL_, tR_, eL_, eR_;
    Fdn fdn_;
    EarlyReflections er_;
};

}  // namespace fr
