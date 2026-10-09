#include "fdn/fdn.h"
#include "fdn/decay_design.h"
#include "common/rng.h"
#include <algorithm>
#include <cmath>

namespace fr {
namespace {
bool isPrime(int n) {
    if (n < 2) return false;
    for (int k = 2; k * k <= n; ++k) if (n % k == 0) return false;
    return true;
}
}  // namespace

void Fdn::configure(const FdnConfig& c, double fs) {
    n_ = c.lines;
    // Delay lengths: distinct primes near a linear spread, scaled by size.
    delay_.assign(n_, 0);
    std::vector<int> used;
    for (int i = 0; i < n_; ++i) {
        double ms = n_ > 1 ? c.minMs + (c.maxMs - c.minMs) * i / (n_ - 1) : c.minMs;
        int d = (int)std::lround(ms * fs / 1000.0 * c.size);
        while (!isPrime(d) || std::find(used.begin(), used.end(), d) != used.end()) ++d;
        used.push_back(d);
        delay_[i] = d;
    }
    int maxD = *std::max_element(delay_.begin(), delay_.end());
    int margin = (int)std::ceil(6.0 * c.modDepth) + 8;
    stride_ = maxD + margin + 3;                       // odd offset avoids power-of-two strides
    if ((stride_ & (stride_ - 1)) == 0) ++stride_;
    buf_.assign((size_t)n_ * stride_, 0.f);

    const size_t N = n_;
    g0_.assign(N, 0.f);
    b0_.assign(kSections * N, 0.f); b1_.assign(kSections * N, 0.f); a1_.assign(kSections * N, 0.f);
    st_.assign(kSections * N, 0.f);
    for (int i = 0; i < n_; ++i) {
        DecayFilter df = designDecayFilter(delay_[i], c.t60, fs);
        g0_[i] = (float)df.gain;
        for (int k = 0; k < kSections; ++k) {
            b0_[k * N + i] = (float)df.sh[k].b0;
            b1_[k * N + i] = (float)df.sh[k].b1;
            a1_[k * N + i] = (float)df.sh[k].a1;
        }
    }
    // Input and output gains. Left/right outputs are orthogonal random vectors.
    Rng tr(c.seed + 1000);
    std::vector<double> tl(N), trv(N);
    for (size_t i = 0; i < N; ++i) { tl[i] = tr.gauss(); trv[i] = tr.gauss(); }
    double dot = 0, nl = 0;
    for (size_t i = 0; i < N; ++i) { dot += tl[i] * trv[i]; nl += tl[i] * tl[i]; }
    for (size_t i = 0; i < N; ++i) trv[i] -= tl[i] * dot / nl;
    double nL = 0, nR = 0;
    for (size_t i = 0; i < N; ++i) { nL += tl[i] * tl[i]; nR += trv[i] * trv[i]; }
    tapL_.resize(N); tapR_.resize(N); inL_.resize(N); inR_.resize(N);
    for (size_t i = 0; i < N; ++i) {
        tapL_[i] = (float)(tl[i] / std::sqrt(nL));
        tapR_[i] = (float)(trv[i] / std::sqrt(nR));
        double s = 1.0 / std::sqrt((double)N);
        inL_[i] = (float)(((i % 2) == 0 ? 1.0 : -1.0) * s);   // L and R inputs are orthogonal
        inR_[i] = (float)(((i / 2) % 2 == 0 ? 1.0 : -1.0) * s);
    }
    twoOverN_ = 2.0f / (float)n_;

    modDepth_ = c.modDepth;
    modK_ = std::exp(-2.0 * 3.14159265358979323846 * c.modRateHz * kCtrl / fs);
    modNorm_ = std::sqrt((1.0 + modK_) / (1.0 - modK_));   // unit-variance one-pole noise
    modCur_.assign(N, 0.f); modStep_.assign(N, 0.f); modNext_.assign(N, 0.f);
    modState_.assign(N, 0.f);
    y_.assign(N, 0.f); z_.assign(N, 0.f);
    rngState_.resize(N);
    for (size_t i = 0; i < N; ++i) rngState_[i] = c.seed * 7919 + i + 1;
    reset();
}

void Fdn::reset() {
    std::fill(buf_.begin(), buf_.end(), 0.f);
    std::fill(st_.begin(), st_.end(), 0.f);
    std::fill(modState_.begin(), modState_.end(), 0.f);
    std::fill(modCur_.begin(), modCur_.end(), 0.f);
    std::fill(modNext_.begin(), modNext_.end(), 0.f);
    wp_ = 0;
    ctrlPos_ = kCtrl;   // forces a refresh on the first sample
    for (size_t i = 0; i < rngState_.size(); ++i) rngState_[i] = (rngState_[i] & 0xFFFFFFFFull) + 1;
}

void Fdn::refreshModulation() {
    for (int i = 0; i < n_; ++i) {
        Rng r(rngState_[i]);
        double w = r.gauss();
        rngState_[i] = r.next();
        modState_[i] = (float)(modK_ * modState_[i] + (1.0 - modK_) * w);
        modCur_[i] = modNext_[i];
        modNext_[i] = (float)(modState_[i] * modNorm_ * modDepth_);
        modStep_[i] = (modNext_[i] - modCur_[i]) / (float)kCtrl;
    }
}

void Fdn::process(const float* inL, const float* inR, float* outL, float* outR, int count) {
    const size_t N = n_;
    for (int s = 0; s < count; ++s) {
        if (ctrlPos_ >= kCtrl) { refreshModulation(); ctrlPos_ = 0; }
        ++ctrlPos_;
        // 1. Read every line (cubic Lagrange, fractional delay from modulation).
        for (int i = 0; i < n_; ++i) {
            modCur_[i] += modStep_[i];
            float D = (float)delay_[i] + modCur_[i];
            int Di = (int)D;
            float fr = 1.0f - (D - (float)Di);
            const float* b = &buf_[(size_t)i * stride_];
            int base = wp_ + stride_ - Di - 2;
            int i0 = base >= stride_ ? base - stride_ : base;
            int i1 = i0 + 1 >= stride_ ? i0 + 1 - stride_ : i0 + 1;
            int i2 = i1 + 1 >= stride_ ? i1 + 1 - stride_ : i1 + 1;
            int i3 = i2 + 1 >= stride_ ? i2 + 1 - stride_ : i2 + 1;
            float a = b[i0], c0 = b[i1], c1 = b[i2], d = b[i3];
            y_[i] = c0 + fr * (0.5f * (c1 - a) + fr * ((a - 2.5f * c0 + 2.0f * c1 - 0.5f * d)
                    + fr * 0.5f * (d - a + 3.0f * (c0 - c1))));
        }
        // 2. Output taps.
        float oL = 0.f, oR = 0.f;
        for (int i = 0; i < n_; ++i) { oL += tapL_[i] * y_[i]; oR += tapR_[i] * y_[i]; }
        // 3. Decay filters (gain, then shelf cascade; section-major for vectorization).
        for (int i = 0; i < n_; ++i) z_[i] = g0_[i] * y_[i];
        for (int k = 0; k < kSections; ++k) {
            const float* B0 = &b0_[k * N]; const float* B1 = &b1_[k * N]; const float* A1 = &a1_[k * N];
            float* S = &st_[k * N];
            for (int i = 0; i < n_; ++i) {
                float o = B0[i] * z_[i] + S[i];
                S[i] = B1[i] * z_[i] - A1[i] * o;
                z_[i] = o;
            }
        }
        // 4. Householder feedback: z - (2/N) * sum(z), plus input injection.
        float sum = 0.f;
        for (int i = 0; i < n_; ++i) sum += z_[i];
        const float k2 = twoOverN_ * sum, xl = inL[s], xr = inR[s];
        for (int i = 0; i < n_; ++i)
            buf_[(size_t)i * stride_ + wp_] = z_[i] - k2 + inL_[i] * xl + inR_[i] * xr;
        if (++wp_ >= stride_) wp_ = 0;
        outL[s] = oL; outR[s] = oR;
    }
}

}  // namespace fr
