#include "fx.h"
#include "EngineHost.h"

namespace ultra {

// ---- Drive (INFRA) ----------------------------------------------------------------------------------
void Drive::process(float* L, float* R, int n, DriveMode mode, float amount, float tone, float mix) {
    if (amount < 0.001f || mix < 0.001f) return;
    const float a = amount;
    const float g = 1.f + a * a * 24.f;
    const float makeup = 0.35f + 0.65f / std::sqrt(g);
    const float fc = 800.f * std::exp2(tone * 4.6f);
    const float lpc = 1.f - std::exp(-kTwoPi * std::min(fc, 0.45f * fs_) / fs_);
    const float bits = 16.f - a * 13.f, levels = std::exp2(bits - 1.f);
    const float hold = 1.f + a * a * 23.f;
    float* ch[2] = {L, R};
    for (int i = 0; i < n; ++i) {
        if (mode == DriveMode::CosmicRay) {
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
            const float x = ch[c][i];
            float y;
            switch (mode) {
                case DriveMode::Tube: { const float u = g * x; y = fastTanh(u + 0.22f * a * u * u); break; }
                case DriveMode::Fold: { const float u = g * x * 0.5f; y = std::sin(u * kPi * 0.5f) * 0.9f; break; }
                default: y = fastTanh(g * x); break;
            }
            // DC blocker (the tube curve is asymmetric), then the tone low-pass.
            const float dc = y - dcx_[c] + 0.9995f * dcy_[c];
            dcx_[c] = y; dcy_[c] = dc;
            lp_[c] += (dc - lp_[c]) * lpc;
            ch[c][i] = x + (lp_[c] * makeup - x) * mix;
        }
    }
}

// ---- Glue -------------------------------------------------------------------------------------------
void Glue::process(float* L, float* R, int n, float threshDb, float ratio, float attackMs, float releaseMs, float makeupDb, float mix) {
    if (mix < 0.001f) { grDb_ = 0.f; envDb_ = -120.f; return; }
    const float att = std::exp(-1.f / (std::max(0.05f, attackMs) * 0.001f * fs_));
    const float rel = std::exp(-1.f / (std::max(5.f, releaseMs) * 0.001f * fs_));
    const float slope = 1.f - 1.f / std::max(1.f, ratio), knee = 6.f;
    const float makeup = dbToGain(makeupDb);
    for (int i = 0; i < n; ++i) {
        const float pk = std::max(std::abs(L[i]), std::abs(R[i]));
        const float db = 20.f * std::log10(std::max(pk, 1e-6f));
        envDb_ = db > envDb_ ? db + (envDb_ - db) * att : db + (envDb_ - db) * rel;
        const float over = envDb_ - threshDb;
        float gr = 0.f;
        if (over > knee * 0.5f) gr = over * slope;
        else if (over > -knee * 0.5f) { const float u = over + knee * 0.5f; gr = slope * u * u / (2.f * knee); }
        grDb_ = gr;
        const float g = dbToGain(-gr) * makeup;
        L[i] = L[i] * (1.f - mix) + L[i] * g * mix;
        R[i] = R[i] * (1.f - mix) + R[i] * g * mix;
    }
}

// ---- Echo (INFRA, wet only) -----------------------------------------------------------------------------
void Echo::prepare(double fs) {
    fs_ = (float)fs;
    for (auto& b : buf_) b.assign((size_t)(2.6 * fs) + 4, 0.f);
    reset();
}
void Echo::reset() { for (auto& b : buf_) std::fill(b.begin(), b.end(), 0.f); pos_ = 0; delay_ = -1.0; lp_[0] = lp_[1] = hp_[0] = hp_[1] = 0.f; }

void Echo::process(const float* inL, const float* inR, float* outL, float* outR, int n,
                   double delaySamples, float feedback, float damp, bool pingPong) {
    const int size = (int)buf_[0].size();
    const double target = std::clamp(delaySamples, 16.0, (double)size - 8.0);
    if (delay_ < 0.0) delay_ = target;
    const double slew = 1.0 - std::exp(-1.0 / (0.06 * fs_));
    const float fb = std::clamp(feedback, 0.f, 0.95f);
    const float fc = 20000.f * std::exp2(-std::clamp(damp, 0.f, 1.f) * 4.5f);
    const float lpc = 1.f - std::exp(-kTwoPi * std::min(fc, 0.45f * fs_) / fs_);
    const float hpc = 1.f - std::exp(-kTwoPi * 90.f / fs_);
    for (int i = 0; i < n; ++i) {
        delay_ += (target - delay_) * slew;
        double rp = (double)pos_ - delay_;
        if (rp < 0.0) rp += size;
        const int i0 = (int)rp; const float fr = (float)(rp - i0);
        const int i1 = i0 + 1 >= size ? 0 : i0 + 1;
        float d[2];
        for (int c = 0; c < 2; ++c) {
            const auto& b = buf_[(size_t)c];
            const float v = b[(size_t)i0] + (b[(size_t)i1] - b[(size_t)i0]) * fr;
            lp_[c] += (v - lp_[c]) * lpc;
            hp_[c] += (lp_[c] - hp_[c]) * hpc;
            d[c] = lp_[c] - hp_[c];
        }
        const float l = inL[i], r = inR[i];
        if (pingPong) {
            buf_[0][(size_t)pos_] = fastTanh((l + r) * 0.5f + fb * d[1]);
            buf_[1][(size_t)pos_] = fastTanh(fb * d[0]);
        } else {
            buf_[0][(size_t)pos_] = fastTanh(l + fb * d[0]);
            buf_[1][(size_t)pos_] = fastTanh(r + fb * d[1]);
        }
        if (++pos_ >= size) pos_ = 0;
        outL[i] = d[0]; outR[i] = d[1];
    }
}

// ---- Reverb (INFRA, wet only) ---------------------------------------------------------------------------
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
    prepared_ = true;
}
void ReverbStage::release() { if (prepared_) { host_->release(); prepared_ = false; } }

void ReverbStage::setShape(float size, float decay, float damp, float preDelayMs) {
    size_.store(size, std::memory_order_relaxed); decay_.store(decay, std::memory_order_relaxed);
    damp_.store(damp, std::memory_order_relaxed); pre_.store(preDelayMs, std::memory_order_relaxed);
}

void ReverbStage::process(const float* inL, const float* inR, float* outL, float* outR, int n) {
    if (!prepared_) { std::fill(outL, outL + n, 0.f); std::fill(outR, outR + n, 0.f); return; }
    host_->process(inL, inR, outL, outR, n, 0.55f, 1.f, 1.0, 0.6);
}

}  // namespace ultra
