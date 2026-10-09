#include "reverb.h"
#include "common/denormal.h"
#include <algorithm>
#include <cmath>

namespace fr {

void Reverb::configure(const ReverbParams& p, double fs, int maxBlock) {
    p_ = p;
    maxBlock_ = std::max(1, maxBlock);
    FdnConfig fc;
    fc.lines = p.quality == Quality::Eco ? 16 : p.quality == Quality::Normal ? 32 : 64;
    fc.size = p.size;
    for (int i = 0; i < 3; ++i) fc.t60[i] = p.t60[i];
    fc.modDepth = p.modDepth; fc.modRateHz = p.modRateHz; fc.seed = p.seed;
    fdn_.configure(fc, fs);
    int taps = p.quality == Quality::Eco ? 16 : p.quality == Quality::Normal ? 32 : 64;
    er_.configure(fs, taps, p.size);

    static const double apMs[8] = {1.9, 2.9, 4.3, 6.1, 8.3, 11.3, 14.9, 19.7};
    apL_.clear(); apR_.clear();
    if (p.diffusion) {
        for (int i = 0; i < 8; ++i) {
            // Right chain is slightly detuned so the two channels decorrelate.
            for (int ch = 0; ch < 2; ++ch) {
                Allpass a;
                a.d = std::max(1, (int)(apMs[i] * (ch ? 1.07 : 1.0) * std::pow(p.size, -0.5) * fs / 1000.0));
                a.x.assign(a.d, 0.f); a.y.assign(a.d, 0.f);
                (ch ? apR_ : apL_).push_back(std::move(a));
            }
        }
    }
    preDelay_ = (int)std::lround(p.preDelayMs * fs / 1000.0);
    preSize_ = preDelay_ + 1;
    preL_.assign(preSize_, 0.f); preR_.assign(preSize_, 0.f);
    for (auto* v : {&dL_, &dR_, &tL_, &tR_, &eL_, &eR_}) v->assign(maxBlock_, 0.f);
    prePos_ = 0;
}

void Reverb::reset() {
    fdn_.reset(); er_.reset();
    for (auto& a : apL_) { std::fill(a.x.begin(), a.x.end(), 0.f); std::fill(a.y.begin(), a.y.end(), 0.f); a.pos = 0; }
    for (auto& a : apR_) { std::fill(a.x.begin(), a.x.end(), 0.f); std::fill(a.y.begin(), a.y.end(), 0.f); a.pos = 0; }
    std::fill(preL_.begin(), preL_.end(), 0.f); std::fill(preR_.begin(), preR_.end(), 0.f);
    prePos_ = 0;
}

void Reverb::processChunk(const float* inL, const float* inR, float* outL, float* outR, int n) {
    // Pre-delay.
    for (int s = 0; s < n; ++s) {
        preL_[prePos_] = inL[s]; preR_[prePos_] = inR[s];
        int r = prePos_ - preDelay_; if (r < 0) r += preSize_;
        dL_[s] = preL_[r]; dR_[s] = preR_[r];
        if (++prePos_ >= preSize_) prePos_ = 0;
    }
    er_.process(dL_.data(), dR_.data(), eL_.data(), eR_.data(), n);
    // Input diffusion (in place on a copy so ER sees the undiffused signal).
    std::copy(dL_.begin(), dL_.begin() + n, tL_.begin());
    std::copy(dR_.begin(), dR_.begin() + n, tR_.begin());
    for (auto& a : apL_) for (int s = 0; s < n; ++s) tL_[s] = a.tick(tL_[s]);
    for (auto& a : apR_) for (int s = 0; s < n; ++s) tR_[s] = a.tick(tR_[s]);
    fdn_.process(tL_.data(), tR_.data(), tL_.data(), tR_.data(), n);
    for (int s = 0; s < n; ++s) {
        outL[s] = p_.dry * inL[s] + p_.wet * (p_.tailLevel * tL_[s] + p_.erLevel * eL_[s]);
        outR[s] = p_.dry * inR[s] + p_.wet * (p_.tailLevel * tR_[s] + p_.erLevel * eR_[s]);
    }
}

void Reverb::process(const float* inL, const float* inR, float* outL, float* outR, int n) {
    ScopedNoDenormals guard;
    for (int off = 0; off < n; off += maxBlock_) {
        int c = std::min(maxBlock_, n - off);
        processChunk(inL + off, inR + off, outL + off, outR + off, c);
    }
}

}  // namespace fr
