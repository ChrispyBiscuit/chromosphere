#include "crossover.h"
#include "conv/partitioned_convolver.h"
#include "pffft.h"
#include <chrono>

namespace muon {

// ---- Linear phase --------------------------------------------------------------------------------------
struct LinearCrossover::Kernel {
    float f1 = 0, f2 = 0;
    fr::PartitionedConvolver c1, c2;
};

LinearCrossover::LinearCrossover() = default;
LinearCrossover::~LinearCrossover() { release(); }

std::vector<float> LinearCrossover::designLowpass(double fs, double fc, int taps) {
    // Sample the zero-phase LR4 magnitude finely, inverse FFT, window to `taps` (odd), normalise DC to 1.
    int M = 1; while (M < 8 * taps) M <<= 1;
    M = std::max(M, 16384);
    PFFFT_Setup* setup = pffft_new_setup(M, PFFFT_REAL);
    float* spec = (float*)pffft_aligned_malloc(sizeof(float) * (size_t)M);
    float* time = (float*)pffft_aligned_malloc(sizeof(float) * (size_t)M);
    float* work = (float*)pffft_aligned_malloc(sizeof(float) * (size_t)M);
    auto H = [&](int j) { const double r = (j * fs / M) / fc, r2 = r * r; return 1.0 / (1.0 + r2 * r2); };
    spec[0] = (float)H(0); spec[1] = (float)H(M / 2);
    for (int j = 1; j < M / 2; ++j) { spec[2 * j] = (float)H(j); spec[2 * j + 1] = 0.f; }
    pffft_transform_ordered(setup, spec, time, work, PFFFT_BACKWARD);
    const int half = taps / 2;
    std::vector<float> h((size_t)taps);
    double sum = 0.0;
    for (int k = -half; k <= half; ++k) {
        const double x = (double)(k + half) / (double)(taps - 1);   // Blackman-Harris over the kernel
        const double w = 0.35875 - 0.48829 * std::cos(2 * kPi * x) + 0.14128 * std::cos(4 * kPi * x) - 0.01168 * std::cos(6 * kPi * x);
        const double v = time[(k + M) % M] / (double)M * w;
        h[(size_t)(k + half)] = (float)v; sum += v;
    }
    for (auto& v : h) v = (float)(v / sum);
    pffft_aligned_free(spec); pffft_aligned_free(time); pffft_aligned_free(work);
    pffft_destroy_setup(setup);
    return h;
}

void LinearCrossover::prepare(double fs, int maxBlock, float f1, float f2) {
    release();
    fs_ = fs; maxBlock_ = maxBlock;
    latency_ = (int)std::lround(512.0 * fs / 48000.0);
    taps_ = 2 * latency_ + 1;
    fadeLen_ = (int)(0.02 * fs);
    auto* k = new Kernel;
    k->f1 = f1; k->f2 = f2;
    k->c1.configure({designLowpass(fs, f1, taps_)});
    k->c2.configure({designLowpass(fs, f2, taps_)});
    cur_ = k;
    wantF1_ = f1; wantF2_ = f2;
    for (auto& d : dl_) d.prepare(latency_ + 1);
    for (auto& t : tmp_) t.assign((size_t)maxBlock, 0.f);
    for (auto& t : scratch_) t.assign((size_t)maxBlock, 0.f);
    run_ = true;
    thread_ = std::thread([this] { worker(); });
}

void LinearCrossover::release() {
    if (run_.exchange(false) && thread_.joinable()) thread_.join();
    delete cur_; cur_ = nullptr;
    delete old_; old_ = nullptr;
    delete next_; next_ = nullptr;
    delete ready_.exchange(nullptr);
    delete trash_.exchange(nullptr);
}

void LinearCrossover::worker() {
    float builtF1 = cur_ ? cur_->f1 : 0.f, builtF2 = cur_ ? cur_->f2 : 0.f;
    while (run_.load()) {
        delete trash_.exchange(nullptr);
        const float f1 = wantF1_.load(), f2 = wantF2_.load();
        if ((f1 != builtF1 || f2 != builtF2) && ready_.load() == nullptr) {
            auto* k = new Kernel;
            k->f1 = f1; k->f2 = f2;
            k->c1.configure({designLowpass(fs_, f1, taps_)});
            k->c2.configure({designLowpass(fs_, f2, taps_)});
            builtF1 = f1; builtF2 = f2;
            ready_.store(k);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
}

void LinearCrossover::process(const float* L, const float* R, int n, float f1, float f2,
                              float* low1L, float* low1R, float* low2L, float* low2R, float* delL, float* delR) {
    if (!cur_) return;
    if (std::fabs(f1 - cur_->f1) > 0.005f * cur_->f1 || std::fabs(f2 - cur_->f2) > 0.005f * cur_->f2) {
        wantF1_.store(f1, std::memory_order_relaxed); wantF2_.store(f2, std::memory_order_relaxed);
    }
    if (!old_ && !next_) {
        if (Kernel* k = ready_.exchange(nullptr)) { next_ = k; warm_ = 0; }
    }
    if (next_) {
        // New kernels start with empty history: run them for a full kernel length before they fade in.
        next_->c1.process(L, R, scratch_[0].data(), scratch_[1].data(), n);
        next_->c2.process(L, R, scratch_[0].data(), scratch_[1].data(), n);
        warm_ += n;
        if (warm_ >= taps_) { old_ = cur_; cur_ = next_; next_ = nullptr; fade_ = 0; }
    }
    cur_->c1.process(L, R, low1L, low1R, n);
    cur_->c2.process(L, R, low2L, low2R, n);
    if (old_) {
        old_->c1.process(L, R, tmp_[0].data(), tmp_[1].data(), n);
        old_->c2.process(L, R, tmp_[2].data(), tmp_[3].data(), n);
        for (int i = 0; i < n; ++i) {
            const float g = std::min(1.f, (float)(fade_ + i) / (float)fadeLen_);
            low1L[i] = tmp_[0][(size_t)i] + g * (low1L[i] - tmp_[0][(size_t)i]);
            low1R[i] = tmp_[1][(size_t)i] + g * (low1R[i] - tmp_[1][(size_t)i]);
            low2L[i] = tmp_[2][(size_t)i] + g * (low2L[i] - tmp_[2][(size_t)i]);
            low2R[i] = tmp_[3][(size_t)i] + g * (low2R[i] - tmp_[3][(size_t)i]);
        }
        fade_ += n;
        if (fade_ >= fadeLen_ && trash_.load() == nullptr) { trash_.store(old_); old_ = nullptr; }
    }
    for (int i = 0; i < n; ++i) { delL[i] = dl_[0].tick(L[i], latency_); delR[i] = dl_[1].tick(R[i], latency_); }
}

// ---- Crossover ---------------------------------------------------------------------------------------------
void Crossover::prepare(double fs, int maxBlock, const Params& p) {
    fs_ = fs;
    f1_ = f2_ = -1.f;
    updateCoefs(p.xoverLow, p.xoverHigh);
    reset();
    linear_.prepare(fs, maxBlock, p.xoverLow, p.xoverHigh);
    for (int c = 0; c < 2; ++c) { l1_[c].assign((size_t)maxBlock, 0.f); l2_[c].assign((size_t)maxBlock, 0.f); d_[c].assign((size_t)maxBlock, 0.f); }
}

void Crossover::reset() {
    for (auto& c : ch_) {
        for (auto* s : {&c.lp1[0], &c.lp1[1], &c.hp1[0], &c.hp1[1], &c.lp2[0], &c.lp2[1], &c.hp2[0], &c.hp2[1], &c.ap2}) s->reset();
    }
}

void Crossover::updateCoefs(float f1, float f2) {
    if (f1 == f1_ && f2 == f2_) return;
    f1_ = f1; f2_ = f2;
    const double k = 1.41421356237;   // Butterworth
    for (auto& c : ch_) {
        for (int i = 0; i < 2; ++i) {
            c.lp1[i].set(fs_, f1, k); c.hp1[i].set(fs_, f1, k);
            c.lp2[i].set(fs_, f2, k); c.hp2[i].set(fs_, f2, k);
        }
        c.ap2.set(fs_, f2, k);
    }
}

void Crossover::process(const float* L, const float* R, int n, const Params& p, float* const out[kMaxBands][2]) {
    const int bands = std::clamp(p.bands, 1, kMaxBands);
    const float f1 = std::clamp(p.xoverLow, 20.f, (float)(0.4 * fs_));
    const float f2 = std::clamp(std::max(p.xoverHigh, f1 * 1.5f), 30.f, (float)(0.45 * fs_));
    const float* in[2] = {L, R};
    if (p.crossover == CrossoverType::LinearPhase) {
        linear_.process(L, R, n, f1, f2, l1_[0].data(), l1_[1].data(), l2_[0].data(), l2_[1].data(), d_[0].data(), d_[1].data());
        for (int c = 0; c < 2; ++c) {
            const float* a = l1_[c].data(); const float* b = l2_[c].data(); const float* d = d_[c].data();
            if (bands == 1) std::copy(d, d + n, out[0][c]);
            else if (bands == 2) for (int i = 0; i < n; ++i) { out[0][c][i] = a[i]; out[1][c][i] = d[i] - a[i]; }
            else for (int i = 0; i < n; ++i) { out[0][c][i] = a[i]; out[1][c][i] = b[i] - a[i]; out[2][c][i] = d[i] - b[i]; }
        }
        return;
    }
    updateCoefs(f1, f2);
    for (int c = 0; c < 2; ++c) {
        auto& s = ch_[c];
        const float* x = in[c];
        if (bands == 1) { std::copy(x, x + n, out[0][c]); continue; }
        for (int i = 0; i < n; ++i) {
            const float lo = s.lp1[1].lp(s.lp1[0].lp(x[i]));
            const float hi = s.hp1[1].hp(s.hp1[0].hp(x[i]));
            if (bands == 2) { out[0][c][i] = lo; out[1][c][i] = hi; continue; }
            out[0][c][i] = s.ap2.ap(lo);
            out[1][c][i] = s.lp2[1].lp(s.lp2[0].lp(hi));
            out[2][c][i] = s.hp2[1].hp(s.hp2[0].hp(hi));
        }
    }
}

}  // namespace muon
