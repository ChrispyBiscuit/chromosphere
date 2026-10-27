#include "common/simd.h"
#include "er/early_reflections.h"
#include <algorithm>
#include <cmath>

namespace fr {

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
    maxDelay_ = maxD;
    size_ = maxD + 1024;      // headroom: blocks of up to 1024 samples are written before any tap reads them
    buf_.assign(size_, 0.f);
    wp_ = 0;
}

void EarlyReflections::reset() { std::fill(buf_.begin(), buf_.end(), 0.f); wp_ = 0; }

// Write the block in, then accumulate tap by tap over contiguous runs of the buffer (vectorised
// over time) instead of a gather per tap per sample. A tap reads samples at or before the one
// being written, so writing the whole block first is equivalent.
FR_CLONES void EarlyReflections::process(const float* inL, const float* inR, float* outL, float* outR, int n) {
    float* __restrict B = buf_.data();
    for (int off = 0; off < n;) {
        // A block may only overwrite slots no tap reads during it: at most size - longest delay.
        const int m = std::min(n - off, std::max(1, size_ - maxDelay_));
        const int wp0 = wp_;
        for (int t = 0; t < m; ++t) { int w = wp0 + t; if (w >= size_) w -= size_; B[w] = 0.5f * (inL[off + t] + inR[off + t]); }
        float* __restrict oL = outL + off; float* __restrict oR = outR + off;
        for (int t = 0; t < m; ++t) { oL[t] = 0.f; oR[t] = 0.f; }
        for (const Tap& tap : taps_) {
            int r = wp0 - tap.delay; if (r < 0) r += size_;
            const float gl = tap.gL, gr = tap.gR;
            int t = 0;
            while (t < m) {
                const int run = std::min(m - t, size_ - r);
                const float* __restrict src = B + r;
                for (int k = 0; k < run; ++k) { const float v = src[k]; oL[t + k] += gl * v; oR[t + k] += gr * v; }
                t += run; r += run; if (r >= size_) r = 0;
            }
        }
        wp_ = wp0 + m; if (wp_ >= size_) wp_ -= size_;
        off += m;
    }
}

}  // namespace fr
