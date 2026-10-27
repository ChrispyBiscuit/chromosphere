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

// Accurate, branch-free versions (relative error about 3e-9 for exp2, absolute 3e-8 for log2) that the compiler
// can vectorise. Used where the antiderivative anti-aliasing needs float precision.
inline float vexp2(float x) {
    x = std::min(std::max(x, -126.f), 126.f);
    const float t = (float)(int)x, fi = t - (t > x ? 1.f : 0.f), f = x - fi;
    const float p = 1.000000002587f + f * (0.693146928693f + f * (0.240230502045f + f * (0.055480426326f
                    + f * (0.009684580451f + f * (0.001238782149f + f * 0.000218775047f)))));
    const int32_t e = ((int32_t)fi + 127) << 23;
    float sc; std::memcpy(&sc, &e, 4);
    return sc * p;
}
inline float vlog2(float x) {     // x > 0, normal
    uint32_t i; std::memcpy(&i, &x, 4);
    const float e = (float)((int32_t)(i >> 23) - 127);
    i = (i & 0x007FFFFF) | 0x3F800000;
    float m; std::memcpy(&m, &i, 4);
    const float u = m - 1.f;
    return e + u * (1.442694998388f + u * (-0.721340328852f + u * (0.480695475319f + u * (-0.35842849238f
                + u * (0.275719668448f + u * (-0.197285224406f + u * (0.11305198986f + u * (-0.042687192891f + u * 0.007579137513f))))))));
}

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
// Firs half-band taps (Kaiser-windowed): the odd offsets 1, 3, ... up to halfLen - 1, scaled so both polyphase
// branches have unity DC gain (centre tap 0.5).
std::vector<float> designFirHalfband(int halfLen);
// Stage k of the chain runs between rates 2^k fs and 2^(k+1) fs.
struct StageSpec { double tbw; int firHalf; };
inline constexpr StageSpec kStageSpecs[3] = {{0.04, 80}, {0.15, 22}, {0.30, 20}};
// Round-trip (up then down) latency in base-rate samples: FIR only; IIR reports 0. FIR half-lengths are chosen so
// it is an integer (H1 even, H2 a multiple of 2, H3 a multiple of 4).
int resamplerLatency(int factor, bool linear);

// One 2x stage for L interleaved signals ([sample][lane]); the lanes run in step so the compiler vectorises them.
template <int L>
class Stage2x {
public:
    void setupIir(const std::vector<double>& c) {
        fir_ = false; c0_.clear(); c1_.clear();
        for (size_t i = 0; i < c.size(); ++i) (i % 2 == 0 ? c0_ : c1_).push_back((float)c[i]);
        s0_.assign(c0_.size(), {}); s1_.assign(c1_.size(), {});
        reset();
    }
    void setupFir(int halfLen) {
        fir_ = true; half_ = halfLen; taps_ = designFirHalfband(halfLen);
        histLen_ = 2 * half_ + 2; hist_.assign((size_t)histLen_ * 2 * L, 0.f);
        lowLen_ = half_ + 1; histLow_.assign((size_t)lowLen_ * 2 * L, 0.f);
        reset();
    }
    void reset() {
        for (auto& v : s0_) v = {};
        for (auto& v : s1_) v = {};
        for (auto& o : odd_) o = 0.f;
        std::fill(hist_.begin(), hist_.end(), 0.f); std::fill(histLow_.begin(), histLow_.end(), 0.f);
        pos_ = posLow_ = 0;
    }
    // n frames in, 2n frames out.
    void up(const float* in, float* out, int n) {
        if (!fir_) {
            for (int i = 0; i < n; ++i) {
                float y0[L], y1[L];
                for (int l = 0; l < L; ++l) y0[l] = y1[l] = in[i * L + l];
                chain(c0_, s0_, y0); chain(c1_, s1_, y1);
                for (int l = 0; l < L; ++l) { out[(2 * i) * L + l] = y0[l]; out[(2 * i + 1) * L + l] = y1[l]; }
            }
            return;
        }
        const int nt = (int)taps_.size();
        for (int i = 0; i < n; ++i) {
            posLow_ = (posLow_ == 0 ? lowLen_ : posLow_) - 1;
            for (int l = 0; l < L; ++l) histLow_[(size_t)posLow_ * L + l] = histLow_[(size_t)(posLow_ + lowLen_) * L + l] = in[i * L + l];
            const float* h = &histLow_[(size_t)posLow_ * L];        // h[k * L + l] = x_l[n - k]
            float acc[L] = {};
            for (int t = 0; t < nt; ++t) {
                const int d = 2 * t + 1;
                const float* a = h + ((half_ - d - 1) / 2) * L; const float* b = h + ((half_ + d - 1) / 2) * L;
                for (int l = 0; l < L; ++l) acc[l] += taps_[(size_t)t] * (a[l] + b[l]);
            }
            for (int l = 0; l < L; ++l) { out[(2 * i) * L + l] = h[(half_ / 2) * L + l]; out[(2 * i + 1) * L + l] = 2.f * acc[l]; }
        }
    }
    // 2n frames in, n frames out.
    void down(const float* in, float* out, int n) {
        if (!fir_) {
            for (int i = 0; i < n; ++i) {
                float y0[L], y1[L];
                for (int l = 0; l < L; ++l) { y0[l] = in[(2 * i) * L + l]; y1[l] = odd_[l]; odd_[l] = in[(2 * i + 1) * L + l]; }
                chain(c0_, s0_, y0); chain(c1_, s1_, y1);
                for (int l = 0; l < L; ++l) out[i * L + l] = 0.5f * (y0[l] + y1[l]);
            }
            return;
        }
        const int nt = (int)taps_.size();
        for (int i = 0; i < n; ++i) {
            for (int k = 0; k < 2; ++k) {
                pos_ = (pos_ == 0 ? histLen_ : pos_) - 1;
                for (int l = 0; l < L; ++l) hist_[(size_t)pos_ * L + l] = hist_[(size_t)(pos_ + histLen_) * L + l] = in[(2 * i + k) * L + l];
            }
            const float* h = &hist_[(size_t)(pos_ + 1) * L];          // h[j * L + l] = v_l[2n - j]
            float acc[L];
            for (int l = 0; l < L; ++l) acc[l] = 0.5f * h[half_ * L + l];
            for (int t = 0; t < nt; ++t) {
                const int d = 2 * t + 1;
                const float* a = h + (half_ - d) * L; const float* b = h + (half_ + d) * L;
                for (int l = 0; l < L; ++l) acc[l] += taps_[(size_t)t] * (a[l] + b[l]);
            }
            for (int l = 0; l < L; ++l) out[i * L + l] = acc[l];
        }
    }
private:
    // First-order all-pass sections at the lower rate, (c + z^-1) / (1 + c z^-1), applied to every lane.
    struct St { float x1[L] = {}, y1[L] = {}; };
    static inline void chain(const std::vector<float>& c, std::vector<St>& s, float* y) {
        for (size_t k = 0; k < c.size(); ++k) {
            const float ck = c[k]; St& st = s[k];
            for (int l = 0; l < L; ++l) { const float o = ck * (y[l] - st.y1[l]) + st.x1[l]; st.x1[l] = y[l]; st.y1[l] = o; y[l] = o; }
        }
    }
    bool fir_ = false;
    std::vector<float> c0_, c1_;
    std::vector<St> s0_, s1_;
    float odd_[L] = {};
    int half_ = 0, pos_ = 0, histLen_ = 0, posLow_ = 0, lowLen_ = 0;
    std::vector<float> taps_, hist_, histLow_;
};

// Up and down sampling by 1, 2, 4 or 8 for L interleaved signals. Up and down have separate states.
template <int L>
class ResamplerN {
public:
    // factor 1, 2, 4 or 8; linear = FIR stages. maxBlock: largest base-rate block (frames).
    void prepare(int factor, bool linear, int maxBlock) {
        nStages_ = factor >= 8 ? 3 : factor >= 4 ? 2 : factor >= 2 ? 1 : 0;
        factor_ = 1 << nStages_;
        for (int s = 0; s < nStages_; ++s) {
            if (linear) { upSt_[s].setupFir(kStageSpecs[s].firHalf); downSt_[s].setupFir(kStageSpecs[s].firHalf); }
            else { const auto c = designHalfband(100.0, kStageSpecs[s].tbw); upSt_[s].setupIir(c); downSt_[s].setupIir(c); }
        }
        latency_ = resamplerLatency(factor_, linear);
        tmpA_.assign((size_t)maxBlock * 8 * L, 0.f); tmpB_.assign((size_t)maxBlock * 8 * L, 0.f);
    }
    void reset() { for (int s = 0; s < 3; ++s) { upSt_[s].reset(); downSt_[s].reset(); } }
    int factor() const { return factor_; }
    int roundTripLatency() const { return latency_; }
    void up(const float* in, float* out, int n) {          // out: n * factor frames
        if (nStages_ == 0) { std::memcpy(out, in, sizeof(float) * (size_t)n * L); return; }
        const float* src = in; int len = n;
        for (int s = 0; s < nStages_; ++s) {
            float* dst = s == nStages_ - 1 ? out : (s % 2 == 0 ? tmpA_.data() : tmpB_.data());
            upSt_[s].up(src, dst, len); src = dst; len *= 2;
        }
    }
    void down(const float* in, float* out, int n) {        // in: n * factor frames
        if (nStages_ == 0) { std::memcpy(out, in, sizeof(float) * (size_t)n * L); return; }
        const float* src = in; int len = n << nStages_;
        for (int s = nStages_ - 1; s >= 0; --s) {
            len /= 2;
            float* dst = s == 0 ? out : (s % 2 == 0 ? tmpA_.data() : tmpB_.data());
            downSt_[s].down(src, dst, len); src = dst;
        }
    }
private:
    int factor_ = 1, latency_ = 0, nStages_ = 0;
    Stage2x<L> upSt_[3], downSt_[3];
    std::vector<float> tmpA_, tmpB_;
};
using Resampler = ResamplerN<1>;

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

// Float versions for the audio path.
inline float curveFf(Curve c, float x) {
    const float a = std::fabs(x);
    switch (c) {
        case Curve::Tanh: { const float t = 1.f - 2.f / (1.f + vexp2(2.8853900818f * a)); return x < 0.f ? -t : t; }
        case Curve::Rational: return x / (1.f + a);
        case Curve::Clip: return std::min(std::max(x, -1.f), 1.f);
        case Curve::Sine: return std::sin(x);
        case Curve::Rectify: return a;
    }
    return x;
}
inline float curveF1f(Curve c, float x) {
    const float a = std::fabs(x);
    switch (c) {
        case Curve::Tanh: return a + 0.69314718f * vlog2(1.f + vexp2(-2.8853900818f * a)) - 0.69314718f;
        case Curve::Rational: return a - 0.69314718f * vlog2(1.f + a);
        case Curve::Clip: return a <= 1.f ? 0.5f * x * x : a - 0.5f;
        case Curve::Sine: return -std::cos(x);
        case Curve::Rectify: return 0.5f * x * a;
    }
    return 0.5f * x * x;
}

// First-order ADAA of a curve: y[n] = (F1(x[n]) - F1(x[n-1])) / (x[n] - x[n-1]), in float. Where the step is too
// small for float precision (relative to the level) it falls back to the curve at the midpoint. Adds half a sample
// of delay.
struct Adaa {
    float x1 = 0.f, f1 = 0.f;
    Curve curve = Curve::Tanh;
    void reset() { x1 = 0.f; f1 = curveF1f(curve, 0.f); }
    void setCurve(Curve c) { if (c != curve) { curve = c; f1 = curveF1f(c, x1); } }
    inline float tick(float x) {
        const float F = curveF1f(curve, x), d = x - x1;
        const float y = std::fabs(d) > 1e-3f * std::max(1.f, std::fabs(x)) ? (F - f1) / d : curveFf(curve, 0.5f * (x + x1));
        x1 = x; f1 = F;
        return y;
    }
};

}  // namespace muon
