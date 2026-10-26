#include "conv/partitioned_convolver.h"
#include "common/simd.h"
extern "C" {
#include "pffft.h"
}
#include <algorithm>
#include <cmath>
#include <cstring>

namespace cs {

void PartitionedConvolver::Buf::alloc(size_t count) {
    release();
    p = (float*)pffft_aligned_malloc(count * sizeof(float));
    n = count;
    std::memset(p, 0, count * sizeof(float));
}
void PartitionedConvolver::Buf::release() { if (p) pffft_aligned_free(p); p = nullptr; n = 0; }

PartitionedConvolver::PartitionedConvolver() = default;
PartitionedConvolver::~PartitionedConvolver() {
    for (Level& L : lev_) if (L.setup) pffft_destroy_setup(L.setup);
}

size_t PartitionedConvolver::memoryBytes() const {
    size_t b = 0;
    for (const Level& L : lev_) {
        for (const Buf& h : L.H) b += h.n * 4;
        for (int c = 0; c < 2; ++c) for (const Buf& x : L.X[c]) b += x.n * 4;
    }
    return b + (inRing_[0].size() + outRing_[0].size()) * 8;
}

void PartitionedConvolver::configure(const std::vector<std::vector<float>>& ir) {
    for (Level& L : lev_) {
        if (L.setup) pffft_destroy_setup(L.setup);
        L = Level();
    }
    const int chans = (int)ir.size();
    filters_.clear();
    if (chans == 4) filters_ = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    else filters_ = {{0, 0}, {1, 1}};
    auto tap = [&](int f, int64_t k) -> float {
        const std::vector<float>& h = chans == 1 ? ir[0] : ir[f];
        return k >= 0 && k < (int64_t)h.size() ? h[(size_t)k] : 0.f;
    };
    irLen_ = 0;
    for (const auto& h : ir) irLen_ = std::max(irLen_, (int)h.size());
    const int nf = (int)filters_.size();

    for (int f = 0; f < nf; ++f) {
        head_[f].assign(kHead, 0.f);
        for (int k = 0; k < kHead; ++k) head_[f][kHead - 1 - k] = tap(f, k);
    }
    for (int l = 0; l < kLevels; ++l) {
        Level& L = lev_[l];
        L.P = kStep << (2 * l); L.N = 2 * L.P;
        const int64_t start = L.P;                              // first tap of the level
        const int64_t end = l == kLevels - 1 ? (int64_t)irLen_ : std::min<int64_t>(irLen_, 4 * (int64_t)L.P);
        L.m = end > start ? (int)((end - start + L.P - 1) / L.P) : 0;
        if (L.m == 0) continue;
        L.setup = pffft_new_setup(L.N, PFFFT_REAL);
        L.tmp.alloc(L.N); L.work.alloc(L.N);
        for (int c = 0; c < 2; ++c) { L.acc[c].alloc(L.N); L.X[c].resize(L.m); for (Buf& x : L.X[c]) x.alloc(L.N); }
        L.H.resize((size_t)nf * L.m);
        for (int f = 0; f < nf; ++f)
            for (int q = 1; q <= L.m; ++q) {
                Buf& h = L.H[(size_t)f * L.m + (q - 1)];
                h.alloc(L.N);
                Buf in; in.alloc(L.N);
                for (int k = 0; k < L.P; ++k) in.p[k] = tap(f, (int64_t)q * L.P + k);
                pffft_transform(L.setup, in.p, h.p, L.work.p, PFFFT_FORWARD);
            }
        L.jobTotal = (L.m - 1) * nf;
        L.jobsPerStep = L.jobTotal == 0 ? 0 : (L.jobTotal + (L.P / kStep) - 1) / (L.P / kStep);
    }
    for (int c = 0; c < 2; ++c) {
        inRing_[c].assign(kRing, 0.f);
        outRing_[c].assign(kRing, 0.f);
        lin_[c].assign(kHead - 1 + kStep, 0.f);
    }
    t_ = 0;
}

void PartitionedConvolver::clear() {
    for (Level& L : lev_) {
        for (int c = 0; c < 2; ++c) {
            for (Buf& x : L.X[c]) if (x.p) std::memset(x.p, 0, x.n * 4);
            if (L.acc[c].p) std::memset(L.acc[c].p, 0, L.acc[c].n * 4);
        }
        L.xHead = 0; L.jobDone = L.jobTotal;   // nothing pending
    }
    for (int c = 0; c < 2; ++c) {
        std::fill(inRing_[c].begin(), inRing_[c].end(), 0.f);
        std::fill(outRing_[c].begin(), outRing_[c].end(), 0.f);
        std::fill(lin_[c].begin(), lin_[c].end(), 0.f);
    }
}

void PartitionedConvolver::reset() { clear(); t_ = 0; }

// Products of older input spectra with the partitions q >= 2, for the next output block.
void PartitionedConvolver::runJobs(Level& L, int count) {
    const int nf = (int)filters_.size();
    for (int k = 0; k < count && L.jobDone < L.jobTotal; ++k, ++L.jobDone) {
        const int f = L.jobDone / (L.m - 1), q = 2 + L.jobDone % (L.m - 1);
        const Filter& fl = filters_[f];
        const int slot = (L.xHead + (q - 2)) % L.m;                   // age q-2 from the newest spectrum
        pffft_zconvolve_accumulate(L.setup, L.X[fl.in][slot].p, L.H[(size_t)f * L.m + (q - 1)].p,
                                   L.acc[fl.out].p, 1.0f / (float)L.N);
    }
    (void)nf;
}

// Runs when the input block ending at time t is complete (t is a multiple of P).
void PartitionedConvolver::boundary(Level& L, int64_t t) {
    const int nf = (int)filters_.size();
    // 1. Finish the pending products. They index the spectra by age, so this must happen
    //    before the newest spectrum is pushed.
    runJobs(L, L.jobTotal);
    // 2. Spectrum of the last 2P input samples, pushed as the newest in each input ring.
    L.xHead = (L.xHead + L.m - 1) % L.m;
    for (int c = 0; c < 2; ++c) {
        for (int i = 0; i < L.N; ++i) L.tmp.p[i] = inRing_[c][(size_t)((t - L.N + i) & (kRing - 1))];
        pffft_transform(L.setup, L.tmp.p, L.X[c][L.xHead].p, L.work.p, PFFFT_FORWARD);
    }
    // 3. Add partition q = 1 (newest spectrum), then go back to the time domain.
    for (int f = 0; f < nf; ++f) {
        const Filter& fl = filters_[f];
        pffft_zconvolve_accumulate(L.setup, L.X[fl.in][L.xHead].p, L.H[(size_t)f * L.m].p,
                                   L.acc[fl.out].p, 1.0f / (float)L.N);
    }
    for (int c = 0; c < 2; ++c) {
        pffft_transform(L.setup, L.acc[c].p, L.tmp.p, L.work.p, PFFFT_BACKWARD);
        for (int i = 0; i < L.P; ++i) outRing_[c][(size_t)((t + i) & (kRing - 1))] += L.tmp.p[L.P + i];
        std::memset(L.acc[c].p, 0, L.acc[c].n * 4);
    }
    L.jobDone = 0;
}

CS_CLONES void PartitionedConvolver::process(const float* inL, const float* inR, float* outL, float* outR, int n) {
    const float* in[2] = {inL, inR};
    float* out[2] = {outL, outR};
    if (filters_.empty()) { for (int i = 0; i < n; ++i) { outL[i] = inL[i]; outR[i] = inR[i]; } return; }
    const int nf = (int)filters_.size();
    int done = 0;
    while (done < n) {
        const int phase = (int)(t_ % kStep);
        const int c = std::min(n - done, kStep - phase);
        if (phase == 0) {
            if (t_ > 0)
                for (Level& L : lev_) if (L.m > 0 && t_ % L.P == 0) boundary(L, t_);
            for (Level& L : lev_) if (L.m > 0) runJobs(L, L.jobsPerStep);
        }
        // Copy the chunk into the input rings and the linear head buffers, keeping a copy of
        // the inputs first (outL/outR may alias inL/inR).
        for (int ch = 0; ch < 2; ++ch) {
            float* lin = lin_[ch].data();
            for (int i = 0; i < c; ++i) {
                const float v = in[ch][done + i];
                lin[kHead - 1 + phase + i] = v;
                inRing_[ch][(size_t)((t_ + i) & (kRing - 1))] = v;
            }
        }
        float y[2][kStep];
        for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < c; ++i) y[ch][i] = 0.f;
        for (int f = 0; f < nf; ++f) {
            const float* h = head_[f].data();
            const float* lin = lin_[filters_[f].in].data();
            float* yo = y[filters_[f].out];
            for (int i = 0; i < c; ++i) {
                const float* x = lin + phase + i;
                float s = 0.f;
                #pragma omp simd reduction(+:s)
                for (int k = 0; k < kHead; ++k) s += h[k] * x[k];
                yo[i] += s;
            }
        }
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < c; ++i) {
                float& r = outRing_[ch][(size_t)((t_ + i) & (kRing - 1))];
                out[ch][done + i] = y[ch][i] + r;
                r = 0.f;
            }
        t_ += c; done += c;
        if (t_ % kStep == 0)    // slide the head history: keep the last kHead-1 samples
            for (int ch = 0; ch < 2; ++ch)
                std::memmove(lin_[ch].data(), lin_[ch].data() + kStep, (kHead - 1) * sizeof(float));
    }
}

}  // namespace cs
