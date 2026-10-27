#include "common/simd.h"
#include "fdn/fdn.h"
#include "fdn/decay_design.h"
#include "common/rng.h"
#include <algorithm>
#include <cmath>

namespace cs {
namespace {
bool isPrime(int n) {
    if (n < 2) return false;
    for (int k = 2; k * k <= n; ++k) if (n % k == 0) return false;
    return true;
}
}  // namespace

std::vector<int> Fdn::computeDelays(const FdnConfig& c, double fs) {
    // Distinct primes near a linear spread, scaled by size.
    const int n = c.lines;
    std::vector<int> delay(n), used;
    for (int i = 0; i < n; ++i) {
        double ms = n > 1 ? c.minMs + (c.maxMs - c.minMs) * i / (n - 1) : c.minMs;
        int d = (int)std::lround(ms * fs / 1000.0 * c.size);
        while (!isPrime(d) || std::find(used.begin(), used.end(), d) != used.end()) ++d;
        used.push_back(d);
        delay[i] = d;
    }
    return delay;
}

void Fdn::configure(const FdnConfig& c, double fs) {
    n_ = c.lines;
    fs_ = fs;
    delay_ = computeDelays(c, fs);
    int maxD = *std::max_element(delay_.begin(), delay_.end());
    int margin = (int)std::ceil(6.0 * std::max(c.modDepth, 8.0)) + 8;   // room for live depth changes up to 8 samples
    stride_ = maxD + margin + 3;                       // odd offset avoids power-of-two strides
    if ((stride_ & (stride_ - 1)) == 0) ++stride_;
    lineLen_ = stride_ + kGuard;                       // ring plus a mirror of its first kGuard samples
    buf_.assign((size_t)n_ * lineLen_, 0.f);

    const size_t N = n_;
    g0_.assign(N, 0.f);
    b0_.assign(kSections * N, 0.f); b1_.assign(kSections * N, 0.f); a1_.assign(kSections * N, 0.f);
    st_.assign(kSections * N, 0.f);
    {
        DecaySet d = designDecay(c.t60);
        g0_ = d.g0; b0_ = d.b0; b1_ = d.b1; a1_ = d.a1;
        target_ = d; step_ = d; normal_ = d; freeze_ = d; frozen_ = false;
        std::fill(freeze_.g0.begin(), freeze_.g0.end(), 0.99999f);
        std::fill(freeze_.b0.begin(), freeze_.b0.end(), 1.f);
        std::fill(freeze_.b1.begin(), freeze_.b1.end(), 0.f);
        std::fill(freeze_.a1.begin(), freeze_.a1.end(), 0.f);
        std::fill(step_.g0.begin(), step_.g0.end(), 0.f);
        std::fill(step_.b0.begin(), step_.b0.end(), 0.f);
        std::fill(step_.b1.begin(), step_.b1.end(), 0.f);
        std::fill(step_.a1.begin(), step_.a1.end(), 0.f);
        rampTicks_ = 0;
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
    delayF_.resize(N);
    yBlk_.assign(N * kCtrl, 0.f); vBlk_.assign(N * kCtrl, 0.f);
    for (auto* v : {&wa_, &wb_, &wc_, &wd_}) v->assign(N, 0.f);
    di_.assign(N, 0);
    for (size_t i = 0; i < N; ++i) delayF_[i] = (float)delay_[i];
    rngState_.resize(N);
    for (size_t i = 0; i < N; ++i) { Rng r(c.seed * 7919 + i + 1); rngState_[i] = r.next() | 1; }
    reset();
}

DecaySet Fdn::designDecaySet(const std::vector<int>& delay, const double t60[3], double fs) {
    const size_t N = delay.size();
    DecaySet d;
    d.g0.assign(N, 0.f);
    d.t60mid = t60[1];
    d.b0.assign(kSections * N, 0.f); d.b1.assign(kSections * N, 0.f); d.a1.assign(kSections * N, 0.f);
    for (size_t i = 0; i < N; ++i) {
        DecayFilter df = designDecayFilter(delay[i], t60, fs);
        d.g0[i] = (float)df.gain;
        for (int k = 0; k < kSections; ++k) {
            d.b0[k * N + i] = (float)df.sh[k].b0;
            d.b1[k * N + i] = (float)df.sh[k].b1;
            d.a1[k * N + i] = (float)df.sh[k].a1;
        }
    }
    return d;
}

void Fdn::setDecay(const DecaySet& d, int rampSamples) {
    if (d.g0.size() != g0_.size() || d.b0.size() != b0_.size()) return;   // built for another configuration
    std::copy(d.g0.begin(), d.g0.end(), normal_.g0.begin()); std::copy(d.b0.begin(), d.b0.end(), normal_.b0.begin());
    std::copy(d.b1.begin(), d.b1.end(), normal_.b1.begin()); std::copy(d.a1.begin(), d.a1.end(), normal_.a1.begin());
    normal_.t60mid = d.t60mid;
    if (!frozen_) rampTo(normal_, rampSamples);
}

void Fdn::setFreeze(bool on, int rampSamples) {
    if (on == frozen_) return;
    frozen_ = on;
    rampTo(on ? freeze_ : normal_, rampSamples);
}

void Fdn::rampTo(const DecaySet& d, int rampSamples) {
    rampTicks_ = std::max(1, rampSamples / kCtrl);
    const float inv = 1.0f / (float)rampTicks_;
    auto go = [&](const std::vector<float>& tgt, std::vector<float>& cur, std::vector<float>& tg, std::vector<float>& st) {
        for (size_t i = 0; i < cur.size(); ++i) { tg[i] = tgt[i]; st[i] = (tgt[i] - cur[i]) * inv; }
    };
    go(d.g0, g0_, target_.g0, step_.g0); go(d.b0, b0_, target_.b0, step_.b0);
    go(d.b1, b1_, target_.b1, step_.b1); go(d.a1, a1_, target_.a1, step_.a1);
}

void Fdn::setModulation(double depthSamples, double rateHz) {
    modDepth_ = depthSamples;
    modK_ = std::exp(-2.0 * 3.14159265358979323846 * rateHz * kCtrl / fs_);
    modNorm_ = std::sqrt((1.0 + modK_) / (1.0 - modK_));
}

void Fdn::reset() {
    std::fill(buf_.begin(), buf_.end(), 0.f);
    std::fill(st_.begin(), st_.end(), 0.f);
    std::fill(modState_.begin(), modState_.end(), 0.f);
    std::fill(modCur_.begin(), modCur_.end(), 0.f);
    std::fill(modNext_.begin(), modNext_.end(), 0.f);
    wp_ = 0;
    ctrlPos_ = kCtrl;   // forces a refresh on the first sample
    
}

void Fdn::refreshModulation() {
    if (rampTicks_ > 0) {   // coefficient ramp toward the new decay set
        auto adv = [&](std::vector<float>& cur, const std::vector<float>& st, const std::vector<float>& tg, bool last) {
            for (size_t i = 0; i < cur.size(); ++i) cur[i] = last ? tg[i] : cur[i] + st[i];
        };
        bool last = rampTicks_ == 1;
        adv(g0_, step_.g0, target_.g0, last); adv(b0_, step_.b0, target_.b0, last);
        adv(b1_, step_.b1, target_.b1, last); adv(a1_, step_.a1, target_.a1, last);
        --rampTicks_;
    }
    // Per-line smoothed noise. Uniform white input (unit variance) is enough for a slow
    // delay wobble and avoids log/cos in the control loop. xorshift64* per line.
    const float k = (float)modK_, kn = (float)(modNorm_ * modDepth_), k1 = 1.0f - k;
    for (int i = 0; i < n_; ++i) {
        uint64_t x = rngState_[i];
        x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
        rngState_[i] = x;
        float u = (float)((x * 0x2545F4914F6CDD1Dull) >> 40) * (1.0f / 8388608.0f) - 1.0f;   // [-1, 1)
        float w = u * 1.7320508f;                                                            // unit variance
        modState_[i] = k * modState_[i] + k1 * w;
        modCur_[i] = modNext_[i];
        modNext_[i] = modState_[i] * kn;
        modStep_[i] = (modNext_[i] - modCur_[i]) * (1.0f / (float)kCtrl);
        // Interpolation for the whole control period, at its midpoint. Computed here, not per chunk,
        // so the result does not depend on where host blocks split the period.
        const float D = delayF_[i] + 0.5f * (modCur_[i] + modNext_[i]);
        const int Di = (int)D;
        const float f = 1.0f - (D - (float)Di), f2 = f * f, f3 = f2 * f;
        di_[i] = Di;
        wa_[i] = -0.5f * f + f2 - 0.5f * f3; wb_[i] = 1.0f - 2.5f * f2 + 1.5f * f3;
        wc_[i] = 0.5f * f + 2.0f * f2 - 1.5f * f3; wd_[i] = -0.5f * f2 + 0.5f * f3;
    }
}

// The network is processed in control chunks of up to kCtrl samples. Every delay is far longer than
// a chunk, so within one chunk each line's reads come from samples written before the chunk began:
// a line can be read for the whole chunk at once (contiguous, vectorised over time), with its
// interpolation weights held for the chunk (the modulation moves a fraction of a sample per
// second; holding it 16 samples is inaudible). The per-sample work (decay filters, output taps,
// Householder feedback) is then two loops over a compile-time line count.
template <int N>
void Fdn::processN(const float* inL, const float* inR, float* outL, float* outR, int count) {
    const int R = stride_;
    float* __restrict B = buf_.data();
    float* __restrict Y = yBlk_.data();                 // [t][line]
    float* __restrict V = vBlk_.data();                 // [t][line]: values written back
    int s = 0;
    while (s < count) {
        if (ctrlPos_ >= kCtrl) { refreshModulation(); ctrlPos_ = 0; }
        const int L = std::min(kCtrl - ctrlPos_, count - s);
        // 1. Reads: per line, L contiguous Catmull-Rom outputs with weights for the chunk's midpoint.
        for (int i = 0; i < N; ++i) {
            const float wa = wa_[i], wb = wb_[i], wc = wc_[i], wd = wd_[i];   // fixed for the control period
            int start = wp_ - 2 - di_[i]; if (start < 0) start += R;          // runs past R into the mirror at most
            const float* __restrict src = B + (size_t)i * lineLen_ + start;
            float tmp[kCtrl];
            for (int t = 0; t < L; ++t) tmp[t] = wa * src[t] + wb * src[t + 1] + wc * src[t + 2] + wd * src[t + 3];
            for (int t = 0; t < L; ++t) Y[t * N + i] = tmp[t];
        }
        // 2. Per sample: decay filters, taps, feedback.
        const float* __restrict g0 = g0_.data();
        const float* __restrict tl = tapL_.data();
        const float* __restrict tr = tapR_.data();
        const float* __restrict il = inL_.data();
        const float* __restrict ir = inR_.data();
        float* __restrict st = st_.data();
        const float* __restrict b0 = b0_.data();
        const float* __restrict b1 = b1_.data();
        const float* __restrict a1 = a1_.data();
        for (int t = 0; t < L; ++t) {
            const float* __restrict y = Y + t * N;
            float* __restrict v = V + t * N;
            float oL = 0.f, oR = 0.f, sum = 0.f;
            #pragma omp simd reduction(+:oL, oR, sum)
            for (int i = 0; i < N; ++i) {
                const float yi = y[i];
                oL += tl[i] * yi; oR += tr[i] * yi;
                float z = g0[i] * yi;
                for (int k = 0; k < kSections; ++k) {
                    const int j = k * N + i;
                    const float o = b0[j] * z + st[j];
                    st[j] = b1[j] * z - a1[j] * o;
                    z = o;
                }
                v[i] = z;
                sum += z;
            }
            const float k2 = twoOverN_ * sum, xl = inL[s + t], xr = inR[s + t];
            #pragma omp simd
            for (int i = 0; i < N; ++i) v[i] = v[i] - k2 + il[i] * xl + ir[i] * xr;
            outL[s + t] = oL; outR[s + t] = oR;
        }
        // 3. Writes: per line, L contiguous samples; the first kGuard slots are mirrored past the end
        //    so a read that crosses the end of the ring stays contiguous.
        const int first = std::min(L, R - wp_);
        for (int i = 0; i < N; ++i) {
            float* __restrict line = B + (size_t)i * lineLen_;
            for (int t = 0; t < first; ++t) line[wp_ + t] = V[t * N + i];
            for (int t = first; t < L; ++t) line[wp_ + t - R] = V[t * N + i];
        }
        {   // mirror the slots written this chunk that fall in the first kGuard of the ring
            const int w0 = wp_, w1 = wp_ + L;                // written [w0, w1), possibly wrapping past R
            auto mirror = [&](int a, int b) {                // ring slots [a, b) with a < kGuard
                const int e = std::min(b, kGuard);
                for (int i = 0; i < N && a < e; ++i) { float* line = B + (size_t)i * lineLen_; for (int k = a; k < e; ++k) line[R + k] = line[k]; }
            };
            if (w0 < kGuard) mirror(w0, std::min(w1, R));
            if (w1 > R) mirror(0, w1 - R);
        }
        wp_ += L; if (wp_ >= R) wp_ -= R;
        ctrlPos_ += L;
        s += L;
    }
}

CS_CLONES void Fdn::process(const float* inL, const float* inR, float* outL, float* outR, int count) {
    switch (n_) {
        case 16: processN<16>(inL, inR, outL, outR, count); break;
        case 32: processN<32>(inL, inR, outL, outR, count); break;
        case 64: processN<64>(inL, inR, outL, outR, count); break;
        default: processN<32>(inL, inR, outL, outR, count); break;   // configure() only builds 16, 32, 64
    }
}

}  // namespace cs
