#include "vintage/modules.h"
#include <algorithm>
#include <cmath>

namespace cs {
namespace {
constexpr double kPi = 3.14159265358979323846;
}

// ---- Bandwidth ----
void Bandwidth::set(double cutoffHz) {
    const double fc = std::clamp(cutoffHz, 200.0, fs_ * 0.45);
    const double w = std::tan(kPi * fc / fs_), w2 = w * w, k = std::sqrt(2.0);
    const double n = 1.0 / (1.0 + k * w + w2);
    b0_ = (float)(w2 * n); b1_ = (float)(2.0 * w2 * n); b2_ = b0_;
    a1_ = (float)(2.0 * (w2 - 1.0) * n); a2_ = (float)((1.0 - k * w + w2) * n);
}
void Bandwidth::process(const float* in, float* out, int n) {
    for (int i = 0; i < n; ++i) {   // transposed direct form II
        const float x = in[i];
        const float y = b0_ * x + z1_;
        z1_ = b1_ * x - a1_ * y + z2_;
        z2_ = b2_ * x - a2_ * y;
        out[i] = y;
    }
}

// ---- Quantizer ----
void Quantizer::set(double bits, double rateHz) {
    q_ = (float)std::pow(2.0, std::clamp(bits, 6.0, 24.0) - 1.0) - 1.f;
    const double rate = std::clamp(rateHz, 4000.0, fs_);
    step_ = rate / fs_;
    const double fc = std::min(rate * 0.45, fs_ * 0.45);
    lpK_ = (float)(1.0 - std::exp(-2.0 * kPi * fc / fs_));
    dither_ = bits < 15.5 ? 1.f / q_ : 0.f;      // TPDF of one LSB peak each way: the converter hisses on silence, as they did
}
void Quantizer::process(const float* in, float* out, int n) {
    for (int i = 0; i < n; ++i) {
        acc_ += step_;
        if (acc_ >= 1.0) {
            acc_ -= 1.0;
            // TPDF dither while there is signal; none on digital silence (the hiss would never stop).
            const float d = (dither_ > 0.f && std::fabs(in[i]) > 1e-5f) ? (float)(rng_.uniform() + rng_.uniform() - 1.0) * dither_ : 0.f;
            hold_ = std::round((in[i] + d) * q_) / q_;
        }
        lp_ += lpK_ * (hold_ - lp_);
        out[i] = lp_;
    }
}

// ---- SpringTank ----
void SpringTank::configure(double fs, uint64_t seed, double size) {
    fs_ = fs;
    Rng rng(seed);
    const double baseMs[3] = {31.0, 41.0, 53.0};
    for (int k = 0; k < 3; ++k) {
        auto& s = s_[k];
        s.len = std::max(8, (int)std::lround(baseMs[k] * std::clamp(size, 0.4, 2.5) * fs / 1000.0 * (0.95 + 0.1 * rng.uniform())));
        s.buf.assign((size_t)s.len, 0.f); s.pos = 0;
        s.chain.configure(120);
    }
    set(2, 0.5, 4000.0, 1.5);
    reset();
}
void SpringTank::reset() { for (auto& s : s_) { std::fill(s.buf.begin(), s.buf.end(), 0.f); s.pos = 0; s.chain.reset(); s.lp = 0.f; } }
void SpringTank::set(int springs, double boing, double tensionHz, double t60) {
    count_ = std::clamp(springs, 1, 3);
    const double b = std::clamp(boing, 0.0, 1.0);
    for (int k = 0; k < 3; ++k) {
        auto& s = s_[k];
        s.chain.set((int)std::lround(24.0 + 96.0 * b), (float)(-0.45 - 0.25 * b));
        const double loopSec = s.len / fs_;
        s.g = (float)std::pow(10.0, -3.0 * loopSec / std::max(0.05, t60));
        const double fc = std::clamp(tensionHz * (1.0 + 0.15 * k), 800.0, fs_ * 0.45);
        s.lpK = (float)(1.0 - std::exp(-2.0 * kPi * fc / fs_));
    }
}
void SpringTank::process(const float* in, float* out, int n) {
    const float norm = 1.f / std::sqrt((float)count_);
    for (int i = 0; i < n; ++i) {
        const float x = in[i];
        float sum = 0.f;
        for (int k = 0; k < count_; ++k) {
            auto& s = s_[k];
            const float rd = s.buf[(size_t)s.pos];
            s.lp += s.lpK * (rd - s.lp);
            const float y = s.chain.tick(s.lp);
            s.buf[(size_t)s.pos] = x + s.g * y;
            if (++s.pos >= s.len) s.pos = 0;
            sum += y;
        }
        out[i] = sum * norm;
    }
}

// ---- SpringBank ----
void SpringBank::configure(double fs, uint64_t seed, double size) {
    fs_ = fs;
    Rng rng(seed);
    const double baseMs[3] = {31.0, 41.0, 53.0};
    for (int ch = 0; ch < 2; ++ch)
        for (int k = 0; k < 3; ++k) {
            const int j = ch * 4 + k;
            len_[j] = std::max(8, (int)std::lround(baseMs[k] * std::clamp(size, 0.4, 2.5) * fs / 1000.0 * (0.95 + 0.1 * rng.uniform()) * (ch ? 1.03 : 1.0)));
            buf_[j].assign((size_t)len_[j], 0.f);
        }
    for (int j : {3, 7}) { len_[j] = 1; buf_[j].assign(1, 0.f); }
    x1_.assign((size_t)kMaxStages * kLanes, 0.f); y1_.assign((size_t)kMaxStages * kLanes, 0.f);
    set(2, 0.5, 4000.0, 1.5);
    reset();
}
void SpringBank::reset() {
    for (int j = 0; j < kLanes; ++j) { std::fill(buf_[j].begin(), buf_[j].end(), 0.f); pos_[j] = 0; lp_[j] = 0.f; }
    std::fill(x1_.begin(), x1_.end(), 0.f); std::fill(y1_.begin(), y1_.end(), 0.f);
}
void SpringBank::set(int springs, double boing, double tensionHz, double t60) {
    count_ = std::clamp(springs, 1, 3);
    const double b = std::clamp(boing, 0.0, 1.0);
    stages_ = (int)std::lround(24.0 + 96.0 * b);
    a_ = (float)(-0.45 - 0.25 * b);
    for (int ch = 0; ch < 2; ++ch)
        for (int k = 0; k < 3; ++k) {
            const int j = ch * 4 + k;
            const double loopSec = len_[j] / fs_;
            g_[j] = (float)std::pow(10.0, -3.0 * loopSec / std::max(0.05, t60));
            const double fc = std::clamp(tensionHz * (1.0 + 0.15 * k), 800.0, fs_ * 0.45);
            lpK_[j] = (float)(1.0 - std::exp(-2.0 * kPi * fc / fs_));
            on_[j] = k < count_ ? 1.f : 0.f;
        }
}
void SpringBank::process(const float* inL, const float* inR, float* outL, float* outR, int n) {
    const float norm = 1.f / std::sqrt((float)count_), a = a_;
    float* __restrict X1 = x1_.data(); float* __restrict Y1 = y1_.data();
    alignas(32) float x[kLanes];
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < kLanes; ++j) { const float rd = buf_[j][(size_t)pos_[j]]; lp_[j] += lpK_[j] * (rd - lp_[j]); x[j] = lp_[j]; }
        for (int k = 0; k < stages_; ++k) {
            float* __restrict p = X1 + k * kLanes; float* __restrict q = Y1 + k * kLanes;
            #pragma omp simd
            for (int j = 0; j < kLanes; ++j) { const float y = -a * x[j] + p[j] + a * q[j]; p[j] = x[j]; q[j] = y; x[j] = y; }
        }
        float l = 0.f, r = 0.f;
        for (int j = 0; j < kLanes; ++j) {
            const float v = x[j] * on_[j];
            buf_[j][(size_t)pos_[j]] = (j < 4 ? inL[i] : inR[i]) + g_[j] * v;
            if (++pos_[j] >= len_[j]) pos_[j] = 0;
            if (j < 4) l += v; else r += v;
        }
        outL[i] = l * norm; outR[i] = r * norm;
    }
}

// ---- Burst ----
void Burst::configure(double fs, uint64_t seed, double maxSec) {
    fs_ = fs;
    size_ = std::max(16, (int)(maxSec * fs) + 2);
    buf_.assign((size_t)size_, 0.f); wp_ = 0;
    const double apMs[4] = {4.3, 7.9, 11.7, 16.1};
    Rng rng(seed);
    for (int k = 0; k < 4; ++k) { ap_[k].d = std::max(1, (int)std::lround(apMs[k] * fs / 1000.0)); ap_[k].w.assign((size_t)ap_[k].d, 0.f); ap_[k].pos = 0; ap_[k].g = 0.5f; }   // modest gains: the diffusers must not ring past the gate
    rnd_.resize(128 * 3);
    for (auto& r : rnd_) r = rng.uniform();
    tapDelay_.assign(128, 0); tapL_.assign(128, 0.f); tapR_.assign(128, 0.f);
    set(0.3, 48, 0.0, 0.0, 15000.0);
    reset();
}
void Burst::reset() { std::fill(buf_.begin(), buf_.end(), 0.f); wp_ = 0; for (auto& a : ap_) { std::fill(a.w.begin(), a.w.end(), 0.f); a.pos = 0; } lpState_ = outLpL_ = outLpR_ = 0.f; }
void Burst::set(double durationSec, int taps, double shape, double feedback, double lpHz) {
    taps_ = std::clamp(taps, 4, 128);
    const double dur = std::clamp(durationSec, 0.02, (size_ - 2) / fs_);
    const float norm = 1.f / std::sqrt((float)taps_) * 1.6f;
    for (int k = 0; k < taps_; ++k) {
        const double u = (k + 0.15 + 0.7 * rnd_[(size_t)k * 3]) / (double)taps_;          // jittered, ordered
        tapDelay_[(size_t)k] = std::clamp((int)std::lround(u * dur * fs_), 1, size_ - 2);
        double env = shape >= 0 ? (1.0 - shape) + shape * u : 1.0 + shape * u;             // flat -> rising, or falling
        if (u > 0.92) env *= (1.0 - u) / 0.08;                                              // no click at the end
        const float sign = rnd_[(size_t)k * 3 + 1] < 0.5f ? -1.f : 1.f;
        const float pan = rnd_[(size_t)k * 3 + 2];                                          // 0..1
        const float g = (float)env * norm * sign;
        tapL_[(size_t)k] = g * std::cos(pan * 1.5707963f); tapR_[(size_t)k] = g * std::sin(pan * 1.5707963f);
    }
    fb_ = (float)std::clamp(feedback, 0.0, 0.9);
    const double fc = std::clamp(lpHz, 500.0, fs_ * 0.45);
    lpK_ = (float)(1.0 - std::exp(-2.0 * kPi * fc / fs_));
}
void Burst::process(const float* inL, const float* inR, float* outL, float* outR, int n) {
    for (int i = 0; i < n; ++i) {
        float x = 0.5f * (inL[i] + inR[i]);
        for (auto& a : ap_) x = a.tick(x);
        float l = 0.f, r = 0.f;
        for (int k = 0; k < taps_; ++k) {
            int rd = wp_ - tapDelay_[(size_t)k]; if (rd < 0) rd += size_;
            const float v = buf_[(size_t)rd];
            l += tapL_[(size_t)k] * v; r += tapR_[(size_t)k] * v;
        }
        lpState_ += lpK_ * (0.5f * (l + r) - lpState_);
        buf_[(size_t)wp_] = x + fb_ * lpState_;
        if (++wp_ >= size_) wp_ = 0;
        outLpL_ += lpK_ * (l - outLpL_); outLpR_ += lpK_ * (r - outLpR_);
        outL[i] = outLpL_; outR[i] = outLpR_;
    }
}

}  // namespace cs
