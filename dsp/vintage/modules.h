#pragma once
// Building blocks for the vintage suite (hardware of 1970-1990). Audio-thread safe after
// configure(): no allocation in process(), any block size, deterministic for a seed.
#include "common/rng.h"
#include <cstdint>
#include <vector>

namespace cs {

// Converter / tape-head bandwidth: a second-order Butterworth low-pass, cutoff live.
class Bandwidth {
public:
    void configure(double fs) { fs_ = fs; set(20000.0); reset(); }
    void reset() { z1_ = z2_ = 0.f; }
    void set(double cutoffHz);
    void process(const float* in, float* out, int n);
private:
    double fs_ = 48000;
    float b0_ = 1, b1_ = 0, b2_ = 0, a1_ = 0, a2_ = 0, z1_ = 0, z2_ = 0;
};

// Early digital converters: word length and sample rate. Zero-order hold at the target rate (no
// anti-alias filter, as the cheap ones had none worth the name), rounding to `bits` with a little
// dither, then a one-pole smoothing at half the target rate so the steps do not hiss.
class Quantizer {
public:
    void configure(double fs, uint64_t seed) { fs_ = fs; rng_ = Rng(seed); set(16.0, fs); reset(); }
    void reset() { hold_ = 0.f; acc_ = 0.0; lp_ = 0.f; }
    void set(double bits, double rateHz);
    void process(const float* in, float* out, int n);
private:
    double fs_ = 48000, step_ = 1.0, acc_ = 0.0;
    float q_ = 32767.f, hold_ = 0.f, lp_ = 0.f, lpK_ = 1.f, dither_ = 0.f;
    Rng rng_;
};

// A chain of first-order all-passes: flat magnitude, frequency-dependent delay. This is what makes
// a spring chirp and a plate ring. Coefficient and active stage count are live.
class DispersionChain {
public:
    void configure(int maxStages) { x1_.assign((size_t)maxStages, 0.f); y1_.assign((size_t)maxStages, 0.f); max_ = maxStages; stages_ = maxStages; }
    void reset() { std::fill(x1_.begin(), x1_.end(), 0.f); std::fill(y1_.begin(), y1_.end(), 0.f); }
    void set(int stages, float coef) { stages_ = stages < 0 ? 0 : stages > max_ ? max_ : stages; a_ = coef; }
    inline float tick(float x) {
        for (int i = 0; i < stages_; ++i) { const float y = -a_ * x + x1_[(size_t)i] + a_ * y1_[(size_t)i]; x1_[(size_t)i] = x; y1_[(size_t)i] = y; x = y; }
        return x;
    }
    void process(const float* in, float* out, int n) { for (int i = 0; i < n; ++i) out[i] = tick(in[i]); }
private:
    std::vector<float> x1_, y1_;
    int max_ = 0, stages_ = 0; float a_ = -0.6f;
};

// A spring tank: up to three springs per channel, each a delay loop holding a dispersion chain
// and a low-pass, with feedback set from the decay time. Mono in, mono out.
class SpringTank {
public:
    void configure(double fs, uint64_t seed, double size);
    void reset();
    void set(int springs, double boing, double tensionHz, double t60);   // boing 0..1: chain length and coefficient
    void process(const float* in, float* out, int n);
private:
    struct Spring { std::vector<float> buf; int len = 1, pos = 0; DispersionChain chain; float lp = 0.f, g = 0.9f, lpK = 0.5f; };
    double fs_ = 48000;
    Spring s_[3];
    int count_ = 2;
};

// Dense multitap burst behind a few diffusers: the AMS-style nonlinear and reverse programs, and
// (with recirculation) the Ursa Major-style multitap cloud. Stereo in, stereo out.
class Burst {
public:
    void configure(double fs, uint64_t seed, double maxSec);
    void reset();
    // durationSec: span of the taps. taps: how many (density). shape: 0 flat, 1 rising (reverse),
    // -1 falling (decaying cloud). feedback 0..0.9 recirculates the tap sum. lpHz: in the loop and output.
    void set(double durationSec, int taps, double shape, double feedback, double lpHz);
    void process(const float* inL, const float* inR, float* outL, float* outR, int n);
private:
    struct Ap { std::vector<float> w; int d = 1, pos = 0; float g = 0.62f; float tick(float in) { const float wd = w[(size_t)pos]; const float wn = in + g * wd; w[(size_t)pos] = wn; if (++pos >= d) pos = 0; return wd - g * wn; } };
    double fs_ = 48000;
    std::vector<float> buf_; int size_ = 1, wp_ = 0;
    Ap ap_[4];
    std::vector<int> tapDelay_; std::vector<float> tapL_, tapR_; int taps_ = 0;
    std::vector<float> rnd_;            // per-tap randoms (position jitter, sign, pan)
    float fb_ = 0.f, lpK_ = 1.f, lpState_ = 0.f, outLpL_ = 0.f, outLpR_ = 0.f;
};

}  // namespace cs
