#include "common/simd.h"
#include "er/early_reflections.h"
#include <algorithm>
#include <cmath>

namespace cs {

void EarlyReflections::configure(double fs, int taps, double sizeScale, double) {
    taps = std::min(taps, kErDefaultCount);
    taps_.clear();
    int maxD = 1;
    // Equal-power pan: pan in [-1, 1] -> left/right gains.
    for (int i = 0; i < taps; ++i) {
        const ErTap& t = kErDefaultTable[i];
        float a = (t.pan * 0.5f + 0.5f) * 1.5707963f;
        Tap tp;
        tp.delay = std::max(1, (int)std::lround(t.delaySec * sizeScale * fs));
        tp.gL = t.gain * std::cos(a);
        tp.gR = t.gain * std::sin(a);
        taps_.push_back(tp);
        maxD = std::max(maxD, tp.delay);
    }
    // Normalize so the summed tap energy is 1.
    double e = 0;
    for (const Tap& t : taps_) e += (double)t.gL * t.gL + (double)t.gR * t.gR;
    float norm = e > 0 ? (float)(1.0 / std::sqrt(e)) : 1.f;
    for (Tap& t : taps_) { t.gL *= norm; t.gR *= norm; }
    size_ = maxD + 2;
    buf_.assign(size_, 0.f);
    wp_ = 0;
}

void EarlyReflections::reset() { std::fill(buf_.begin(), buf_.end(), 0.f); wp_ = 0; }

CS_CLONES void EarlyReflections::process(const float* inL, const float* inR, float* outL, float* outR, int n) {
    for (int s = 0; s < n; ++s) {
        buf_[wp_] = 0.5f * (inL[s] + inR[s]);
        float l = 0.f, r = 0.f;
        for (const Tap& t : taps_) {
            int idx = wp_ - t.delay;
            if (idx < 0) idx += size_;
            float v = buf_[idx];
            l += t.gL * v; r += t.gR * v;
        }
        outL[s] = l; outR[s] = r;
        if (++wp_ >= size_) wp_ = 0;
    }
}

}  // namespace cs
