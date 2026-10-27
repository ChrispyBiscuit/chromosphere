#include "fx.h"
#include "EngineHost.h"

namespace infra {

// ---- Drive ----------------------------------------------------------------------------------------
void Drive::process(float* L, float* R, int n, const FxState& s) {
    if (s.drive < 0.001f || s.driveMix < 0.001f) return;
    // Sub clean: a Linkwitz-Riley crossover (the bands sum flat, in phase) splits off the lows, and only the rest is
    // driven, so a bass keeps a round sub under gritty mids.
    const bool split = s.driveLowCut >= 20.f;
    const Svf::Coef xc = Svf::make(std::tan(kPi * std::clamp(s.driveLowCut, 20.f, 0.45f * fs_) / fs_), 1.41421356f);
    float lows[2] = {0.f, 0.f};
    const float a = s.drive;
    const float g = 1.f + a * a * 24.f;
    const float makeup = 0.35f + 0.65f / std::sqrt(g);
    const float fc = 800.f * std::exp2(s.driveTone * 4.6f);
    const float lpc = 1.f - std::exp(-kTwoPi * std::min(fc, 0.45f * fs_) / fs_);
    const float mix = s.driveMix;
    const float bits = 16.f - a * 13.f, levels = std::exp2(bits - 1.f);
    const float hold = 1.f + a * a * 23.f;
    float* ch[2] = {L, R};
    for (int i = 0; i < n; ++i) {
        if (s.driveMode == DriveMode::CosmicRay) {
            holdCount_ += 1.f;
            const bool take = holdCount_ >= hold;
            if (take) holdCount_ -= hold;
            for (int c = 0; c < 2; ++c) {
                const float x = ch[c][i];
                if (take) hold_[c] = std::round(fastTanh(x * (1.f + a * 2.f)) * levels) / levels;
                lp_[c] += (hold_[c] - lp_[c]) * lpc;
                ch[c][i] = x + (lp_[c] * 0.9f - x) * mix;
            }
            continue;
        }
        for (int c = 0; c < 2; ++c) {
            float x = ch[c][i];
            if (split) {
                float lp, bp, hp, l2, h2;
                xlo_[c][0].tick(x, xc, lp, bp, hp); xlo_[c][1].tick(lp, xc, l2, bp, hp);
                xhi_[c][0].tick(x, xc, lp, bp, hp); xhi_[c][1].tick(hp, xc, lp, bp, h2);
                lows[c] = l2; x = h2;
            }
            float y;
            switch (s.driveMode) {
                case DriveMode::Tube: { const float u = g * x; y = fastTanh(u + 0.22f * a * u * u); break; }
                case DriveMode::Fold: { const float u = g * x * 0.5f; y = std::sin(u * kPi * 0.5f) * 0.9f; break; }
                default: y = fastTanh(g * x); break;
            }
            // DC blocker (the tube curve is asymmetric), then the tone low-pass.
            const float dc = y - dcx_[c] + 0.9995f * dcy_[c];
            dcx_[c] = y; dcy_[c] = dc;
            lp_[c] += (dc - lp_[c]) * lpc;
            ch[c][i] = x + (lp_[c] * makeup - x) * mix + lows[c];
        }
    }
}

// ---- Squash -------------------------------------------------------------------------------------------
void Squash::prepare(double fs) {
    fs_ = (float)fs;
    aLo_ = 1.f - std::exp(-kTwoPi * 120.f / fs_);
    aHi_ = 1.f - std::exp(-kTwoPi * 2500.f / fs_);
    att_ = 1.f - std::exp(-1.f / (0.003f * fs_));
    rel_ = 1.f - std::exp(-1.f / (0.08f * fs_));
    lift_ = 1.f - std::exp(-1.f / (0.04f * fs_));
    reset();
}
void Squash::reset() {
    for (auto& c : lo_) c[0] = c[1] = 0.f;
    for (auto& c : hi_) c[0] = c[1] = 0.f;
    env_[0] = env_[1] = env_[2] = 0.f; gain_[0] = gain_[1] = gain_[2] = 1.f; mixSmooth_ = 0.f;
}
void Squash::process(float* L, float* R, int n, float amount) {
    if (amount < 0.001f && mixSmooth_ < 0.001f) { mixSmooth_ = 0.f; return; }
    const float mixStep = 1.f - std::exp(-1.f / (0.02f * fs_));
    // Thresholds (linear) and ratios: downward above -24 dBFS, upward below -36 dBFS (lift capped at +20 dB).
    constexpr float kDown = 0.063f, kUp = 0.0158f, kFloor = 0.001f, kMaxLift = 10.f;
    constexpr float kBandGain[3] = {1.0f, 1.25f, 1.4f};   // makeup: the mids and highs carry the presence
    for (int i = 0; i < n; ++i) {
        mixSmooth_ += (amount - mixSmooth_) * mixStep;
        const float depth = mixSmooth_;
        float band[3][2];
        float* ch[2] = {L, R};
        float pk[3] = {0.f, 0.f, 0.f};
        for (int c = 0; c < 2; ++c) {
            const float x = ch[c][i];
            lo_[c][0] += (x - lo_[c][0]) * aLo_; lo_[c][1] += (lo_[c][0] - lo_[c][1]) * aLo_;
            const float low = lo_[c][1], rest = x - low;
            hi_[c][0] += (rest - hi_[c][0]) * aHi_; hi_[c][1] += (hi_[c][0] - hi_[c][1]) * aHi_;
            const float mid = hi_[c][1], high = rest - mid;
            band[0][c] = low; band[1][c] = mid; band[2][c] = high;
            for (int b = 0; b < 3; ++b) pk[b] = std::max(pk[b], std::abs(band[b][c]));
        }
        float out[2] = {0.f, 0.f};
        for (int b = 0; b < 3; ++b) {
            env_[b] += (pk[b] - env_[b]) * (pk[b] > env_[b] ? att_ : rel_);
            const float e = env_[b];
            float g = 1.f;
            if (e > kDown) g = std::pow(kDown / e, 0.75f * depth);                       // up to 4:1 downward
            else if (e > kFloor && e < kUp) g = std::min(kMaxLift, std::pow(kUp / e, 0.6f * depth));   // upward
            // A note after silence starts with the envelope near the floor, where the lift is largest: let the lift
            // grow in over ~40 ms (cuts still act at once), or every onset would spike.
            gain_[b] = g < gain_[b] ? g : gain_[b] + (g - gain_[b]) * lift_;
            const float gb = gain_[b] * (1.f + (kBandGain[b] * 1.8f - 1.f) * depth);    // makeup
            out[0] += band[b][0] * gb; out[1] += band[b][1] * gb;
        }
        for (int c = 0; c < 2; ++c) ch[c][i] = out[c];
    }
}

// ---- Aurora chorus ----------------------------------------------------------------------------------
void Chorus::prepare(double fs) {
    fs_ = (float)fs;
    for (auto& b : buf_) b.assign((size_t)(0.06 * fs) + 4, 0.f);
    reset();
}
void Chorus::reset() { for (auto& b : buf_) std::fill(b.begin(), b.end(), 0.f); pos_ = 0; phase_ = 0.0; mixSmooth_ = 0.f; }

void Chorus::skip(int n, const FxState& s) {
    if (s.chorusMix < 0.001f && mixSmooth_ < 0.001f) { mixSmooth_ = 0.f; return; }
    const float mixStep = 1.f - std::exp(-1.f / (0.02f * fs_));
    mixSmooth_ = s.chorusMix + (mixSmooth_ - s.chorusMix) * std::pow(1.f - mixStep, (float)n);
    phase_ += (double)std::clamp(s.chorusRate, 0.01f, 8.f) / fs_ * (double)n;
    phase_ -= std::floor(phase_);
    const int size = (int)buf_[0].size();
    for (int i = 0; i < n; ++i) { buf_[0][(size_t)pos_] = 0.f; buf_[1][(size_t)pos_] = 0.f; if (++pos_ >= size) pos_ = 0; }
}

void Chorus::process(float* L, float* R, int n, const FxState& s) {
    if (s.chorusMix < 0.001f && mixSmooth_ < 0.001f) { mixSmooth_ = 0.f; return; }
    const int size = (int)buf_[0].size();
    const float rate = std::clamp(s.chorusRate, 0.01f, 8.f);
    const float base = 0.011f * fs_, depth = (0.0005f + 0.0055f * std::clamp(s.chorusDepth, 0.f, 1.f)) * fs_;
    const float mixStep = 1.f - std::exp(-1.f / (0.02f * fs_));
    float* ch[2] = {L, R};
    for (int i = 0; i < n; ++i) {
        mixSmooth_ += (s.chorusMix - mixSmooth_) * mixStep;
        phase_ += (double)rate / fs_;
        if (phase_ >= 1.0) phase_ -= 1.0;
        const float inL = L[i], inR = R[i];
        buf_[0][(size_t)pos_] = inL; buf_[1][(size_t)pos_] = inR;
        for (int c = 0; c < 2; ++c) {
            float wet = 0.f;
            for (int k = 0; k < 3; ++k) {
                const float ph = (float)phase_ + (float)k / 3.f + (c ? 0.25f : 0.f);
                const float d = base + depth * (0.5f + 0.5f * std::sin(kTwoPi * ph));
                float rp = (float)pos_ - d;
                if (rp < 0.f) rp += (float)size;
                // A read position a hair below zero wraps to exactly `size` in float: keep the index inside the buffer
                // (it used to read one past the end, which now and then brought in a huge value from the heap).
                int i0 = (int)rp;
                if (i0 >= size) i0 -= size; else if (i0 < 0) i0 += size;
                const float fr = std::clamp(rp - (float)(int)rp, 0.f, 1.f);
                const int i1 = i0 + 1 >= size ? 0 : i0 + 1;
                // Each channel reads mostly its own line and a little of the other: a wide, even ensemble.
                const auto& own = buf_[(size_t)c];
                const auto& other = buf_[(size_t)(1 - c)];
                const float v = own[(size_t)i0] + (own[(size_t)i1] - own[(size_t)i0]) * fr;
                const float o = other[(size_t)i0] + (other[(size_t)i1] - other[(size_t)i0]) * fr;
                wet += (k == 1 ? o : v);
            }
            wet *= (1.f / 3.f);
            const float dry = c ? inR : inL;
            ch[c][i] = dry * (1.f - 0.45f * mixSmooth_) + wet * mixSmooth_ * 1.1f;
        }
        if (++pos_ >= size) pos_ = 0;
    }
}

// ---- Echo delay -------------------------------------------------------------------------------------
void Echo::prepare(double fs) {
    fs_ = (float)fs;
    for (auto& b : buf_) b.assign((size_t)(2.6 * fs) + 4, 0.f);
    reset();
}
void Echo::reset() { for (auto& b : buf_) std::fill(b.begin(), b.end(), 0.f); pos_ = 0; delay_ = -1.0; lp_[0] = lp_[1] = hp_[0] = hp_[1] = 0.f; mixSmooth_ = 0.f; }

void Echo::process(float* L, float* R, int n, const FxState& s) {
    if (s.delayMix < 0.001f && mixSmooth_ < 0.001f) {
        mixSmooth_ = 0.f;
        // Keep writing silence so a later enable does not replay stale echoes.
        for (int i = 0; i < n; ++i) { buf_[0][(size_t)pos_] = 0.f; buf_[1][(size_t)pos_] = 0.f; if (++pos_ >= (int)buf_[0].size()) pos_ = 0; }
        return;
    }
    const int size = (int)buf_[0].size();
    const double target = std::clamp(s.delaySamples, 16.0, (double)size - 8.0);
    if (delay_ < 0.0) delay_ = target;
    const double slew = 1.0 - std::exp(-1.0 / (0.06 * fs_));
    const float fb = std::clamp(s.delayFeedback, 0.f, 0.95f);
    const float fc = 20000.f * std::exp2(-std::clamp(s.delayDamp, 0.f, 1.f) * 4.5f);
    const float lpc = 1.f - std::exp(-kTwoPi * std::min(fc, 0.45f * fs_) / fs_);
    const float hpc = 1.f - std::exp(-kTwoPi * 90.f / fs_);
    const float mixStep = 1.f - std::exp(-1.f / (0.02f * fs_));
    for (int i = 0; i < n; ++i) {
        delay_ += (target - delay_) * slew;
        mixSmooth_ += (s.delayMix - mixSmooth_) * mixStep;
        double rp = (double)pos_ - delay_;
        if (rp < 0.0) rp += size;
        int i0 = (int)rp;                                             // the same wrap guard as the chorus
        if (i0 >= size) i0 -= size; else if (i0 < 0) i0 += size;
        const float fr = (float)std::clamp(rp - (double)(int)rp, 0.0, 1.0);
        const int i1 = i0 + 1 >= size ? 0 : i0 + 1;
        float d[2];
        for (int c = 0; c < 2; ++c) {
            const auto& b = buf_[(size_t)c];
            float v = b[(size_t)i0] + (b[(size_t)i1] - b[(size_t)i0]) * fr;
            lp_[c] += (v - lp_[c]) * lpc;
            hp_[c] += (lp_[c] - hp_[c]) * hpc;
            d[c] = lp_[c] - hp_[c];
        }
        const float inL = L[i], inR = R[i];
        if (s.pingPong) {
            buf_[0][(size_t)pos_] = fastTanh((inL + inR) * 0.5f + fb * d[1]);
            buf_[1][(size_t)pos_] = fastTanh(fb * d[0]);
        } else {
            buf_[0][(size_t)pos_] = fastTanh(inL + fb * d[0]);
            buf_[1][(size_t)pos_] = fastTanh(inR + fb * d[1]);
        }
        if (++pos_ >= size) pos_ = 0;
        L[i] = inL + d[0] * mixSmooth_;
        R[i] = inR + d[1] * mixSmooth_;
    }
}

// ---- Reverb -----------------------------------------------------------------------------------------
ReverbStage::ReverbStage() {
    host_ = std::make_unique<fr::EngineHost>([this] {
        fr::EngineSettings s;
        s.mode = fr::EngineMode::Algorithmic;
        s.quality = fr::Quality::Normal;
        s.size = std::clamp((double)size_.load(), 0.5, 3.0);
        s.preDelayMs = std::clamp((double)pre_.load(), 0.0, 250.0);
        s.diffusion = true;
        const double mid = std::clamp((double)decay_.load(), 0.2, 30.0), damp = std::clamp((double)damp_.load(), 0.0, 1.0);
        s.t60[0] = mid * 1.15; s.t60[1] = mid; s.t60[2] = std::max(0.15, mid * (1.0 - 0.85 * damp));
        return s;
    });
}
ReverbStage::~ReverbStage() { release(); }

void ReverbStage::prepare(double fs, int maxBlock) {
    release();
    host_->prepare(fs, std::min(512, std::max(1, maxBlock)));
    for (auto* v : {&inL_, &inR_, &wetL_, &wetR_}) v->assign(512, 0.f);
    mixSmooth_ = 0.f;
    prepared_ = true;
}
void ReverbStage::release() { if (prepared_) { host_->release(); prepared_ = false; } }

void ReverbStage::setShape(float size, float decay, float damp, float preDelayMs) {
    size_.store(size, std::memory_order_relaxed); decay_.store(decay, std::memory_order_relaxed);
    damp_.store(damp, std::memory_order_relaxed); pre_.store(preDelayMs, std::memory_order_relaxed);
}

void ReverbStage::process(float* L, float* R, int n, float mix) {
    if (!prepared_) return;
    if (mix < 1e-4f && mixSmooth_ < 1e-4f) { mixSmooth_ = 0.f; return; }
    for (int off = 0; off < n; off += 512) {
        const int c = std::min(512, n - off);
        std::copy(L + off, L + off + c, inL_.begin());
        std::copy(R + off, R + off + c, inR_.begin());
        host_->process(inL_.data(), inR_.data(), wetL_.data(), wetR_.data(), c, 0.55f, 1.f, 1.0, 0.6);
        const float step = (mix - mixSmooth_) / (float)c;
        for (int i = 0; i < c; ++i) {
            mixSmooth_ += step;
            L[off + i] += wetL_[(size_t)i] * mixSmooth_;
            R[off + i] += wetR_[(size_t)i] * mixSmooth_;
        }
    }
}

}  // namespace infra
