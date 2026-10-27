#pragma once
// Small DSP building blocks. Rng, fastTanh, the SVF and the voice filter come from INFRA (infra/dsp/infra/blocks.h);
// the filter is mono here (a drum voice is mono until it is panned) and has an Off position.
#include "params.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace ultra {

constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

struct Rng {
    uint32_t s = 0x9e3779b9u;
    void seed(uint32_t v) { s = v ? v : 0x9e3779b9u; }
    uint32_t nextU() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (nextU() >> 8) * (1.f / 16777216.f); }     // 0..1
    float bi() { return uni() * 2.f - 1.f; }                          // -1..1
};

// Stateless hash to 0..1, for decisions that must depend only on position (chance, humanize).
inline float hash01(uint32_t a, uint32_t b, uint32_t c) {
    uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u ^ (c + 0x165667B1u) * 0xC2B2AE3Du;
    h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
    return (h >> 8) * (1.f / 16777216.f);
}

// Rational tanh approximation, accurate to about 1e-3 and bounded to +-1.
inline float fastTanh(float x) {
    x = x < -3.f ? -3.f : x > 3.f ? 3.f : x;        // the rational form reaches exactly +-1 at +-3
    const float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}

// sin(2 pi p) for a phase p in [0, 1): a parabola with one correction step, error about 1e-3.
inline float fastSin(float p) {
    const float t = 2.f * p - 1.f;                   // -1..1; sin(2 pi p) = -sin(pi t)
    float y = 4.f * t * (1.f - std::abs(t));
    y += 0.225f * (y * std::abs(y) - y);
    return -y;
}

// PolyBLEP residual for a discontinuity at phase 0, phase increment dt.
inline float polyBlep(float t, float dt) {
    if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}
// Band-limited square, phase p in [0, 1).
inline float blepSquare(float p, float dt) {
    float y = p < 0.5f ? 1.f : -1.f;
    y += polyBlep(p, dt);
    float q = p + 0.5f; if (q >= 1.f) q -= 1.f;
    y -= polyBlep(q, dt);
    return y;
}

inline float wrap(float p) { return p - (float)(int)p; }          // p >= 0
inline float dbToGain(float db) { return std::pow(10.f, db * 0.05f); }
// Per-sample multiplier that falls 60 dB in t60 seconds.
inline float decayCoef(float t60, float fs) { return std::exp(-6.9078f / (std::max(t60, 1e-4f) * fs)); }
// Log map of a 0..1 dial onto lo..hi.
inline float logMap(float x, float lo, float hi) { return lo * std::pow(hi / lo, std::clamp(x, 0.f, 1.f)); }

// ---- Topology-preserving state variable filter (Simper / Zavalishin). ----
struct Svf {
    float ic1 = 0.f, ic2 = 0.f;
    void reset() { ic1 = ic2 = 0.f; }
    struct Coef { float a1, a2, a3, k; };
    static Coef make(float g, float k) { const float a1 = 1.f / (1.f + g * (g + k)); return {a1, g * a1, g * g * a1, k}; }
    static Coef lowpass(float hz, float q, float fs) { return make(std::tan(kPi * std::clamp(hz, 10.f, 0.47f * fs) / fs), 1.f / q); }
    inline void tick(float x, const Coef& c, float& lp, float& bp, float& hp) {
        const float v3 = x - ic2;
        const float v1 = c.a1 * ic1 + c.a2 * v3;
        const float v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
        ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2;
        lp = v2; bp = v1; hp = x - c.k * v1 - v2;
    }
    inline float bandpass(float x, const Coef& c) { float lp, bp, hp; tick(x, c, lp, bp, hp); return bp; }
    inline float highpass(float x, const Coef& c) { float lp, bp, hp; tick(x, c, lp, bp, hp); return hp; }
    inline float lowpassTick(float x, const Coef& c) { float lp, bp, hp; tick(x, c, lp, bp, hp); return lp; }
};

// ---- Per-voice mono filter: Off plus INFRA's eight types. Coefficients are set at control rate. ----
class VoiceFilter {
public:
    void prepare(double fs) {
        fs_ = (float)fs;
        comb_.assign((size_t)std::ceil(fs / 20.0) + 4, 0.f);
        reset();
    }
    void reset() {
        svfA_.reset(); svfB_.reset();
        for (auto& s : form_) s.reset();
        for (auto& s : ladder_) s = 0.f;
        std::fill(comb_.begin(), comb_.end(), 0.f);
        combPos_ = 0; combLp_ = 0.f;
    }
    void set(FilterType type, float cutoffHz, float res) {
        if (type == type_ && cutoffHz == lastFc_ && res == lastRes_) return;
        type_ = type; lastFc_ = cutoffHz; lastRes_ = res;
        if (type == FilterType::Off) return;
        res = std::clamp(res, 0.f, 1.f);
        const float fc = std::clamp(cutoffHz, 16.f, 0.47f * fs_);
        const float g = std::tan(kPi * fc / fs_);
        switch (type) {
            case FilterType::LP24: {
                G_ = g / (1.f + g); beta_ = 1.f / (1.f + g);
                k_ = 3.98f * res;
                comp_ = 1.f + 0.35f * k_;
                const float G2 = G_ * G_;
                lc_[0] = beta_ * G2 * G_; lc_[1] = beta_ * G2; lc_[2] = beta_ * G_; lc_[3] = beta_;
                lInv_ = 1.f / (1.f + k_ * G2 * G2);
                break;
            }
            case FilterType::LP12: case FilterType::HP12: case FilterType::Notch:
                ca_ = Svf::make(g, 1.414f * (1.f - res) + 0.03f * res); break;
            case FilterType::BP12:
                ca_ = Svf::make(g, 1.2f * (1.f - res) + 0.06f * res); break;
            case FilterType::HP24:
                ca_ = Svf::make(g, 1.848f); cb_ = Svf::make(g, 0.765f * (1.f - res) + 0.03f * res); break;
            case FilterType::Comb: {
                const float f = std::clamp(cutoffHz, 20.f, 0.25f * fs_);
                combDelay_ = std::min((float)comb_.size() - 3.f, fs_ / f);
                combFb_ = 0.2f + 0.77f * res;
                combOut_ = 1.f - 0.75f * combFb_;
                break;
            }
            case FilterType::Formant: {
                // Cutoff picks the vowel (A E I O U across 80 Hz .. 8 kHz), resonance narrows the formants.
                const float pos = std::clamp(std::log2(std::max(fc, 80.f) / 80.f) / std::log2(100.f), 0.f, 1.f) * 4.f;
                const int i = std::min(3, (int)pos);
                const float fr = pos - (float)i;
                static const float vow[5][3] = {{800, 1150, 2900}, {550, 1770, 2490}, {280, 2250, 2890}, {450, 800, 2830}, {325, 700, 2530}};
                const float k = 0.32f - 0.26f * res;
                for (int j = 0; j < 3; ++j) {
                    const float f = std::exp(std::log(vow[i][j]) * (1.f - fr) + std::log(vow[i + 1][j]) * fr);
                    cf_[j] = Svf::make(std::tan(kPi * std::min(f, 0.45f * fs_) / fs_), k);
                }
                formGain_ = k * 2.2f;
                break;
            }
            case FilterType::Off: break;
        }
    }
    inline float process(float x) {
        float lp, bp, hp;
        switch (type_) {
            case FilterType::Off: return x;
            case FilterType::LP24: {
                const float sigma = lc_[0] * ladder_[0] + lc_[1] * ladder_[1] + lc_[2] * ladder_[2] + lc_[3] * ladder_[3];
                float u = fastTanh((x * comp_ - k_ * sigma) * lInv_);
                float y = 0.f;
                for (int s = 0; s < 4; ++s) { const float v = (u - ladder_[s]) * G_; y = v + ladder_[s]; ladder_[s] = y + v; u = y; }
                return y;
            }
            case FilterType::LP12: svfA_.tick(x, ca_, lp, bp, hp); return lp;
            case FilterType::BP12: svfA_.tick(x, ca_, lp, bp, hp); return bp * ca_.k * 1.4f;
            case FilterType::HP12: svfA_.tick(x, ca_, lp, bp, hp); return hp;
            case FilterType::Notch: svfA_.tick(x, ca_, lp, bp, hp); return x - ca_.k * bp;
            case FilterType::HP24: {
                svfA_.tick(x, ca_, lp, bp, hp);
                float lp2, bp2, hp2; svfB_.tick(hp, cb_, lp2, bp2, hp2);
                return hp2;
            }
            case FilterType::Comb: {
                const int size = (int)comb_.size();
                float rp = (float)combPos_ - combDelay_;
                if (rp < 0.f) rp += (float)size;
                const int i0 = (int)rp; const float fr = rp - (float)i0;
                const int i1 = i0 + 1 >= size ? 0 : i0 + 1;
                const float d = comb_[(size_t)i0] + (comb_[(size_t)i1] - comb_[(size_t)i0]) * fr;
                combLp_ += (d - combLp_) * 0.6f;
                const float y = x + combFb_ * combLp_;
                comb_[(size_t)combPos_] = fastTanh(y * 0.5f) * 2.f;
                if (++combPos_ >= size) combPos_ = 0;
                return y * combOut_;
            }
            case FilterType::Formant: {
                float sum = 0.f;
                static const float gains[3] = {1.f, 0.7f, 0.45f};
                for (int j = 0; j < 3; ++j) { form_[j].tick(x, cf_[j], lp, bp, hp); sum += bp * gains[j]; }
                return sum * formGain_ * 1.6f;
            }
        }
        return x;
    }

private:
    float fs_ = 48000.f;
    FilterType type_ = FilterType::Off;
    float lastFc_ = -1.f, lastRes_ = -1.f;
    float G_ = 0.5f, beta_ = 0.5f, k_ = 0.f, comp_ = 1.f, lc_[4] = {}, lInv_ = 1.f;
    float ladder_[4] = {};
    Svf::Coef ca_{}, cb_{}, cf_[3]{};
    Svf svfA_, svfB_, form_[3];
    float formGain_ = 1.f;
    std::vector<float> comb_;
    int combPos_ = 0;
    float combDelay_ = 100.f, combFb_ = 0.5f, combOut_ = 1.f, combLp_ = 0.f;
};

// RBJ shelving biquad (transposed direct form II), for the master EQ.
struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] = {}, z2[2] = {};
    void shelf(bool high, float hz, float db, float fs) {
        const float A = std::pow(10.f, db / 40.f), w = kTwoPi * hz / fs, cw = std::cos(w), sw = std::sin(w);
        const float alpha = sw / 2.f * std::sqrt(2.f), sa = 2.f * std::sqrt(A) * alpha;
        float B0, B1, B2, A0, A1, A2;
        if (!high) {
            B0 = A * ((A + 1) - (A - 1) * cw + sa); B1 = 2 * A * ((A - 1) - (A + 1) * cw); B2 = A * ((A + 1) - (A - 1) * cw - sa);
            A0 = (A + 1) + (A - 1) * cw + sa; A1 = -2 * ((A - 1) + (A + 1) * cw); A2 = (A + 1) + (A - 1) * cw - sa;
        } else {
            B0 = A * ((A + 1) + (A - 1) * cw + sa); B1 = -2 * A * ((A - 1) + (A + 1) * cw); B2 = A * ((A + 1) + (A - 1) * cw - sa);
            A0 = (A + 1) - (A - 1) * cw + sa; A1 = 2 * ((A - 1) - (A + 1) * cw); A2 = (A + 1) - (A - 1) * cw - sa;
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.f; }
    inline float tick(float x, int ch) {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }
};

}  // namespace ultra
