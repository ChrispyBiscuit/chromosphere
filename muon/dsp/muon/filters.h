#pragma once
// Small filters and helpers for the MUON engine. Everything here is real-time safe after prepare()/setup.
//  - Polyphase IIR half-band design (elliptic, after Laurent de Soras' HIIR), used by the 2x up/down sampling
//    stages and, frequency-shifted, by the Hilbert pair.
//  - Up/down samplers by 1, 2, 4 or 8: minimum-phase IIR half-bands, or linear-phase FIR half-bands with an
//    integer latency in base-rate samples.
//  - Hilbert pair: two all-pass chains whose outputs are 90 degrees apart from 20 Hz to 20 kHz.
//  - TPT state-variable filter, one-pole, DC blocker, delay line, fast log2/exp2.
//  - Memoryless curves with their antiderivatives, for first-order antiderivative anti-aliasing (ADAA).
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace muon {

constexpr double kPi = 3.14159265358979323846;

// ---- Fast approximations (about 1e-4 relative error; used for envelope detection and gain) ---------------
inline float fastLog2(float x) {
    union { float f; uint32_t i; } v{x};
    const float e = (float)(int)((v.i >> 23) & 255) - 127.f;
    v.i = (v.i & 0x007FFFFF) | 0x3F800000;     // mantissa in [1, 2)
    const float m = v.f;
    return e + (-0.34484843f * m + 2.02466578f) * m - 0.67487759f;   // max error about 0.005 (0.03 dB)
}
// dB of a linear magnitude, floored.
inline float toDb(float x) { return 6.0206f * fastLog2(std::max(x, 1e-6f)); }
inline float fastExp2(float x) {
    x = std::clamp(x, -60.f, 60.f);
    const float fi = std::floor(x), f = x - fi;
    // 2^f on [0,1): cubic fit
    const float p = 1.f + f * (0.69583356f + f * (0.22606716f + f * 0.07944023f));
    union { uint32_t i; float f; } v{(uint32_t)((int)fi + 127) << 23};
    return v.f * p;
}
inline float dbToGain(float db) { return fastExp2(db * 0.16609640f); }

// ---- Half-band design ------------------------------------------------------------------------------------
// Coefficients of the two all-pass paths of a polyphase IIR half-band low-pass with stopband attenuation `att`
// (dB) and transition width `tbw` (relative to the higher rate; passband edge at 0.25 - tbw/2). Ascending;
// even indices go to path 0, odd indices to path 1.
std::vector<double> designHalfband(double attDb, double tbw);

// ---- Hilbert pair --------------------------------------------------------------------------------------
// a + j b is the analytic signal of the input up to an all-pass phase common to both: for cos(wt) the outputs
// are cos(wt + p) and sin(wt + p). Accurate to 0.01 degree from 15 Hz to fs/2 - 15 Hz.
class Hilbert {
public:
    void prepare(double fs);
    void reset();
    inline void process(float x, float& a, float& b) {
        float y = x;
        for (int i = 0; i < n0_; ++i) { Sec& s = s0_[i]; const float o = s.c * (y + s.y2) - s.x2; s.x2 = s.x1; s.x1 = y; s.y2 = s.y1; s.y1 = o; y = o; }
        a = y;
        y = x;
        for (int i = 0; i < n1_; ++i) { Sec& s = s1_[i]; const float o = s.c * (y + s.y2) - s.x2; s.x2 = s.x1; s.x1 = y; s.y2 = s.y1; s.y1 = o; y = o; }
        b = d_; d_ = y;                          // path 1 is one sample later
    }
private:
    struct Sec { float c = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0; };
    static constexpr int kMax = 12;
    Sec s0_[kMax], s1_[kMax];
    int n0_ = 0, n1_ = 0;
    float d_ = 0.f;
};

// ---- Up/down sampling -----------------------------------------------------------------------------------
// One 2x stage, IIR (minimum phase) or FIR (linear phase).
class Stage2x {
public:
    void setupIir(const std::vector<double>& coefs);
    void setupFir(int halfLen, double tbw);       // taps = 2 * halfLen + 1
    void reset();
    int firDelay() const { return fir_ ? half_ : 0; }   // in samples at the higher rate
    // Upsample n samples into 2n.
    void up(const float* in, float* out, int n);
    // Downsample 2n samples into n.
    void down(const float* in, float* out, int n);
private:
    bool fir_ = false;
    // IIR: first-order all-pass sections at the lower rate, (c + z^-1) / (1 + c z^-1).
    struct Ap { float c = 0, x1 = 0, y1 = 0; };
    std::vector<Ap> p0_, p1_;
    float odd_ = 0.f;                             // down: the previous odd input sample
    // FIR.
    int half_ = 0;
    std::vector<float> taps_;                     // nonzero odd-offset taps h[centre +- (2k+1)], k = 0..
    std::vector<float> hist_;                     // history ring (2x length, mirrored)
    int pos_ = 0, histLen_ = 0;
    std::vector<float> histLow_;                  // FIR up: history at the lower rate
    int posLow_ = 0, lowLen_ = 0;
};

// Up and down sampling by 1, 2, 4 or 8. Both directions in one object (separate states).
class Resampler {
public:
    // factor 1, 2, 4 or 8; linear = FIR stages. maxBlock: largest base-rate block.
    void prepare(int factor, bool linear, int maxBlock);
    void reset();
    int factor() const { return factor_; }
    // Latency in base-rate samples of an up followed by a down (FIR only; IIR reports 0).
    int roundTripLatency() const { return latency_; }
    void up(const float* in, float* out, int n);       // out: n * factor
    void down(const float* in, float* out, int n);     // in: n * factor
private:
    int factor_ = 1, latency_ = 0, nStages_ = 0;
    Stage2x upSt_[3], downSt_[3];
    std::vector<float> tmpA_, tmpB_;
};
// Shared description of the stage chain (so up-only and down-only users agree on latency).
int resamplerLatency(int factor, bool linear);

// ---- Basic filters --------------------------------------------------------------------------------------
// Topology-preserving state-variable filter (Zavalishin). k = 1/Q.
struct Svf {
    float g = 0, k = 1.41421356f, a1 = 0, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0;
    void set(double fs, double hz, double k_) {
        g = (float)std::tan(kPi * std::clamp(hz, 1.0, 0.49 * fs) / fs); k = (float)k_;
        a1 = 1.f / (1.f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    void reset() { ic1 = ic2 = 0.f; }
    // Returns low; band and high through references.
    inline void tick(float x, float& lp, float& bp, float& hp) {
        const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2;
        lp = v2; bp = v1; hp = x - k * v1 - v2;
    }
    inline float lp(float x) { float l, b, h; tick(x, l, b, h); return l; }
    inline float hp(float x) { float l, b, h; tick(x, l, b, h); return h; }
    inline float ap(float x) { float l, b, h; tick(x, l, b, h); return x - 2.f * k * b; }
};

struct OnePole {
    float a = 0.f, z = 0.f;
    void setTime(double fs, double seconds) { a = seconds <= 0 ? 0.f : (float)std::exp(-1.0 / (seconds * fs)); }
    void setHz(double fs, double hz) { a = (float)std::exp(-2.0 * kPi * hz / fs); }
    inline float lp(float x) { z = x + a * (z - x); return z; }
    void reset(float v = 0.f) { z = v; }
};

struct DcBlock {
    float r = 0.9995f, x1 = 0, y1 = 0;
    void setHz(double fs, double hz) { r = (float)std::exp(-2.0 * kPi * hz / fs); }
    inline float tick(float x) { const float y = x - x1 + r * y1; x1 = x; y1 = y; return y; }
    void reset() { x1 = y1 = 0.f; }
};

// Integer delay line with a fixed maximum (power-of-two ring).
class Delay {
public:
    void prepare(int maxDelay) {
        int n = 1; while (n < maxDelay + 1) n <<= 1;
        buf_.assign((size_t)n, 0.f); mask_ = n - 1; pos_ = 0;
    }
    void reset() { std::fill(buf_.begin(), buf_.end(), 0.f); }
    inline float tick(float x, int d) {
        buf_[(size_t)pos_] = x;
        const float y = buf_[(size_t)((pos_ - d) & mask_)];
        pos_ = (pos_ + 1) & mask_;
        return y;
    }
    // Fractional read at d samples behind the last written sample (call after write()).
    inline void write(float x) { buf_[(size_t)pos_] = x; pos_ = (pos_ + 1) & mask_; }
    inline float readFrac(float d) const {
        const int i = (int)d; const float f = d - (float)i;
        const float a = buf_[(size_t)((pos_ - 1 - i) & mask_)], b = buf_[(size_t)((pos_ - 2 - i) & mask_)];
        return a + (b - a) * f;
    }
    int capacity() const { return mask_; }
private:
    std::vector<float> buf_;
    int mask_ = 0, pos_ = 0;
};

// ---- Curves with antiderivatives (ADAA) -----------------------------------------------------------------
// log(cosh(x)), stable for large |x|.
inline double logCosh(double x) { const double a = std::fabs(x); return a + std::log1p(std::exp(-2.0 * a)) - 0.69314718055994531; }

enum class Curve { Tanh, Rational, Clip, Sine, Rectify };

inline double curveF(Curve c, double x) {
    switch (c) {
        case Curve::Tanh: return std::tanh(x);
        case Curve::Rational: return x / (1.0 + std::fabs(x));
        case Curve::Clip: return std::clamp(x, -1.0, 1.0);
        case Curve::Sine: return std::sin(x);
        case Curve::Rectify: return std::fabs(x);
    }
    return x;
}
inline double curveF1(Curve c, double x) {   // antiderivative
    switch (c) {
        case Curve::Tanh: return logCosh(x);
        case Curve::Rational: { const double a = std::fabs(x); return a - std::log1p(a); }
        case Curve::Clip: { const double a = std::fabs(x); return a <= 1.0 ? 0.5 * x * x : a - 0.5; }
        case Curve::Sine: return -std::cos(x);
        case Curve::Rectify: return 0.5 * x * std::fabs(x);
    }
    return 0.5 * x * x;
}

// First-order ADAA of a curve: y[n] = (F1(x[n]) - F1(x[n-1])) / (x[n] - x[n-1]). Adds half a sample of delay.
struct Adaa {
    double x1 = 0.0, f1 = 0.0;
    Curve curve = Curve::Tanh;
    void reset() { x1 = 0.0; f1 = curveF1(curve, 0.0); }
    void setCurve(Curve c) { if (c != curve) { curve = c; f1 = curveF1(c, x1); } }
    inline float tick(float xf) {
        const double x = xf, F = curveF1(curve, x), d = x - x1;
        const double y = std::fabs(d) > 1e-5 ? (F - f1) / d : curveF(curve, 0.5 * (x + x1));
        x1 = x; f1 = F;
        return (float)y;
    }
};

}  // namespace muon
