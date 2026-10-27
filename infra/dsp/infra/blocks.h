#pragma once
// Small DSP building blocks used per voice: random numbers, saturation, envelopes, LFOs, filters, noise.
#include "params.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace infra {

constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

struct Rng {
    uint32_t s = 0x9e3779b9u;
    void seed(uint32_t v) { s = v ? v : 0x9e3779b9u; }
    uint32_t nextU() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (nextU() >> 8) * (1.f / 16777216.f); }     // 0..1
    float bi() { return uni() * 2.f - 1.f; }                          // -1..1
};

// Rational tanh approximation, accurate to about 1e-3 and bounded to +-1.
inline float fastTanh(float x) {
    x = x < -3.f ? -3.f : x > 3.f ? 3.f : x;        // the rational form reaches exactly +-1 at +-3
    const float x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}

inline float noteToHz(float note) { return 440.f * std::exp2((note - 69.f) * (1.f / 12.f)); }

// ---- ADSR with exponential decay and release; the attack is linear in x and shaped by a power curve. ----
class Adsr {
public:
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    void setup(const EnvParams& p, double fs, int step, float curve = 0.f) {
        const float key[7] = {p.a, p.d, p.s, p.r, curve, (float)fs, (float)step};
        if (std::memcmp(key, lastKey_, sizeof(key)) == 0) return;      // unchanged: skip the exp / pow
        std::memcpy(lastKey_, key, sizeof(key));
        const double a = std::max(0.0005, (double)p.a), d = std::max(0.001, (double)p.d), r = std::max(0.001, (double)p.r);
        attackInc_ = (float)(step / (a * fs));
        decayCoef_ = (float)std::exp(-4.6 * step / (d * fs));
        releaseCoef_ = (float)std::exp(-4.6 * step / (r * fs));
        sustain_ = std::clamp(p.s, 0.f, 1.f);
        exponent_ = std::pow(4.f, std::clamp(curve, -1.f, 1.f));
        linear_ = std::abs(curve) < 1e-4f;
    }
    void gateOn() {
        // Restart from the current level (legato, retrigger and voice steal stay click free).
        x_ = value_ > 0.f ? std::pow(std::min(value_, 1.f), 1.f / exponent_) : 0.f;
        stage_ = Attack;
    }
    void gateOff() { if (stage_ != Idle) stage_ = Release; }
    void kill() { stage_ = Idle; value_ = 0.f; x_ = 0.f; }
    float next() {
        switch (stage_) {
            case Attack:
                x_ += attackInc_;
                if (x_ >= 1.f) { x_ = 1.f; stage_ = Decay; }
                value_ = linear_ ? x_ : std::pow(x_, exponent_);
                break;
            case Decay:
                value_ = sustain_ + (value_ - sustain_) * decayCoef_;
                if (value_ - sustain_ < 1e-4f) { value_ = sustain_; stage_ = Sustain; }
                break;
            case Sustain: value_ = sustain_; break;
            case Release:
                value_ *= releaseCoef_;
                if (value_ < 3e-4f) { value_ = 0.f; stage_ = Idle; }
                break;
            case Idle: break;
        }
        return value_;
    }
    float value() const { return value_; }
    Stage stage() const { return stage_; }
    bool active() const { return stage_ != Idle; }
private:
    float lastKey_[7] = {-1.f, -1.f, -1.f, -1.f, -9.f, 0.f, 0.f};
    bool linear_ = true;
    Stage stage_ = Idle;
    float value_ = 0.f, x_ = 0.f, attackInc_ = 0.01f, decayCoef_ = 0.99f, releaseCoef_ = 0.99f, sustain_ = 1.f, exponent_ = 1.f;
};

// ---- LFO: advanced at control rate. Output -1..1. ----
class Lfo {
public:
    void reset(float phase, Rng& rng) { phase_ = phase; held_ = rng.bi(); prev_ = held_; target_ = rng.bi(); }
    float advance(LfoShape shape, float cyclesPerStep, Rng& rng) {
        phase_ += cyclesPerStep;
        if (phase_ >= 1.f) {
            phase_ -= std::floor(phase_);
            held_ = rng.bi();
            prev_ = target_; target_ = rng.bi();
        }
        return value(shape);
    }
    float value(LfoShape shape) const {
        const float p = phase_;
        switch (shape) {
            case LfoShape::Sine: return std::sin(kTwoPi * p);
            case LfoShape::Triangle: return p < 0.25f ? 4.f * p : p < 0.75f ? 2.f - 4.f * p : 4.f * p - 4.f;
            case LfoShape::SawUp: return 2.f * p - 1.f;
            case LfoShape::SawDown: return 1.f - 2.f * p;
            case LfoShape::Square: return p < 0.5f ? 1.f : -1.f;
            case LfoShape::SampleHold: return held_;
            case LfoShape::Drift: { const float s = p * p * (3.f - 2.f * p); return prev_ + (target_ - prev_) * s; }
        }
        return 0.f;
    }
    float phase() const { return phase_; }
private:
    float phase_ = 0.f, held_ = 0.f, prev_ = 0.f, target_ = 0.f;
};

// ---- Topology-preserving state variable filter (Simper / Zavalishin). ----
struct Svf {
    float ic1 = 0.f, ic2 = 0.f;
    void reset() { ic1 = ic2 = 0.f; }
    struct Coef { float a1, a2, a3, k; };
    static Coef make(float g, float k) { const float a1 = 1.f / (1.f + g * (g + k)); return {a1, g * a1, g * g * a1, k}; }
    // Returns lp, bp, hp through references.
    inline void tick(float x, const Coef& c, float& lp, float& bp, float& hp) {
        const float v3 = x - ic2;
        const float v1 = c.a1 * ic1 + c.a2 * v3;
        const float v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
        ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2;
        lp = v2; bp = v1; hp = x - c.k * v1 - v2;
    }
};

// ---- Per-voice stereo filter with all eight types. Coefficients are set at control rate. ----
class VoiceFilter {
public:
    void prepare(double fs) {
        fs_ = (float)fs;
        const int maxDelay = (int)std::ceil(fs / 20.0) + 4;
        for (auto& d : comb_) d.assign((size_t)maxDelay, 0.f);
        reset();
    }
    void reset() {
        for (auto& s : svfA_) s.reset();
        for (auto& s : svfB_) s.reset();
        for (auto& ch : form_) for (auto& s : ch) s.reset();
        for (auto& ch : ladder_) for (auto& s : ch) s = 0.f;
        for (auto& d : comb_) std::fill(d.begin(), d.end(), 0.f);
        combPos_ = 0; combLp_[0] = combLp_[1] = 0.f;
    }
    void set(FilterType type, float cutoffHz, float res, float drive) {
        type_ = type;
        res = std::clamp(res, 0.f, 1.f);
        const float fc = std::clamp(cutoffHz, 16.f, 0.47f * fs_);
        const float g = std::tan(kPi * fc / fs_);
        driveGain_ = 1.f + 7.f * drive * drive;
        driveNorm_ = 0.5f / fastTanh(driveGain_ * 0.5f);   // an input at 0.5 comes out at 0.5
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
                combDelay_ = std::min((float)comb_[0].size() - 3.f, fs_ / f);
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
        }
    }
    inline void process(float& l, float& r) {
        if (driveGain_ > 1.001f) { l = fastTanh(l * driveGain_) * driveNorm_; r = fastTanh(r * driveGain_) * driveNorm_; }
        if (type_ == FilterType::LP24) { l = ladder(ladder_[0], l); r = ladder(ladder_[1], r); return; }   // the common case, inlined
        l = processCh(0, l); r = processCh(1, r);
    }

private:
    inline float ladder(float* __restrict s, float x) const {
        const float sigma = lc_[0] * s[0] + lc_[1] * s[1] + lc_[2] * s[2] + lc_[3] * s[3];
        float u = fastTanh((x * comp_ - k_ * sigma) * lInv_);
        const float G = G_;
        float v = (u - s[0]) * G; float y = v + s[0]; s[0] = y + v; u = y;
        v = (u - s[1]) * G; y = v + s[1]; s[1] = y + v; u = y;
        v = (u - s[2]) * G; y = v + s[2]; s[2] = y + v; u = y;
        v = (u - s[3]) * G; y = v + s[3]; s[3] = y + v;
        return y;
    }
    float processCh(int ch, float x) {
        float lp, bp, hp;
        switch (type_) {
            case FilterType::LP24: return ladder(ladder_[ch], x);
            case FilterType::LP12: svfA_[ch].tick(x, ca_, lp, bp, hp); return lp;
            case FilterType::BP12: svfA_[ch].tick(x, ca_, lp, bp, hp); return bp * ca_.k * 1.4f;
            case FilterType::HP12: svfA_[ch].tick(x, ca_, lp, bp, hp); return hp;
            case FilterType::Notch: svfA_[ch].tick(x, ca_, lp, bp, hp); return x - ca_.k * bp;
            case FilterType::HP24: {
                svfA_[ch].tick(x, ca_, lp, bp, hp);
                float lp2, bp2, hp2; svfB_[ch].tick(hp, cb_, lp2, bp2, hp2);
                return hp2;
            }
            case FilterType::Comb: {
                auto& buf = comb_[ch];
                const int size = (int)buf.size();
                float rp = (float)combPos_ - combDelay_;
                if (rp < 0.f) rp += (float)size;
                const int i0 = (int)rp; const float fr = rp - (float)i0;
                const int i1 = i0 + 1 >= size ? 0 : i0 + 1;
                const float d = buf[(size_t)i0] + (buf[(size_t)i1] - buf[(size_t)i0]) * fr;
                combLp_[ch] += (d - combLp_[ch]) * 0.6f;
                const float y = x + combFb_ * combLp_[ch];
                buf[(size_t)combPos_] = fastTanh(y * 0.5f) * 2.f;
                if (ch == 1 && ++combPos_ >= size) combPos_ = 0;
                return y * combOut_;
            }
            case FilterType::Formant: {
                float sum = 0.f;
                static const float gains[3] = {1.f, 0.7f, 0.45f};
                for (int j = 0; j < 3; ++j) { form_[ch][j].tick(x, cf_[j], lp, bp, hp); sum += bp * gains[j]; }
                return sum * formGain_ * 1.6f;
            }
        }
        return x;
    }

    float fs_ = 48000.f;
    FilterType type_ = FilterType::LP24;
    float driveGain_ = 1.f, driveNorm_ = 1.f;
    float G_ = 0.5f, beta_ = 0.5f, k_ = 0.f, comp_ = 1.f, lc_[4] = {}, lInv_ = 1.f;
    float ladder_[2][4] = {};
    Svf::Coef ca_{}, cb_{}, cf_[3]{};
    Svf svfA_[2], svfB_[2], form_[2][3];
    float formGain_ = 1.f;
    std::vector<float> comb_[2];
    int combPos_ = 0;
    float combDelay_ = 100.f, combFb_ = 0.5f, combOut_ = 1.f, combLp_[2] = {};
};

// ---- Noise colours (mono). ----
class Noise {
public:
    void reset(uint32_t seed) { rng_.seed(seed); b0 = b1 = b2 = brown = 0.f; svf_.reset(); crackle = 0.f; sweep = rng_.uni(); }
    void setup(float fs) { fs_ = fs; }
    inline float tick(NoiseColour c) {
        const float w = rng_.bi();
        switch (c) {
            case NoiseColour::White: return w * 0.5f;
            case NoiseColour::Pink: {
                b0 = 0.99765f * b0 + w * 0.0990460f; b1 = 0.96300f * b1 + w * 0.2965164f; b2 = 0.57000f * b2 + w * 1.0526913f;
                return (b0 + b1 + b2 + w * 0.1848f) * 0.12f;
            }
            case NoiseColour::SolarWind: {
                brown = std::clamp(brown * 0.995f + w * 0.06f, -1.f, 1.f);
                // a slow band-pass sweep over brown + white: wind gusts
                sweep += 0.07f / fs_;
                if (sweep >= 1.f) sweep -= 1.f;
                const float f = 300.f * std::exp2(3.5f * (0.5f + 0.5f * std::sin(kTwoPi * sweep)));
                if (++coefCount_ >= 32) { coefCount_ = 0; coef_ = Svf::make(std::tan(kPi * f / fs_), 0.35f); }
                float lp, bp, hp; svf_.tick(w * 0.6f + brown, coef_, lp, bp, hp);
                return (bp * 0.9f + brown * 0.35f);
            }
            case NoiseColour::Crackle: {
                crackle *= 0.86f;
                if (rng_.uni() < 0.0025f) crackle = rng_.bi() * 1.5f;
                return crackle + w * 0.02f;
            }
        }
        return 0.f;
    }
private:
    Rng rng_;
    float fs_ = 48000.f, b0 = 0, b1 = 0, b2 = 0, brown = 0, crackle = 0, sweep = 0;
    Svf svf_; Svf::Coef coef_ = Svf::make(0.05f, 0.35f); int coefCount_ = 32;
};

}  // namespace infra
