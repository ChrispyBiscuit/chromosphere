#pragma once
// Band splitting for MUON (PRD 4.2.1).
//  - Minimum phase: Linkwitz-Riley 4th order (two Butterworth sections per side), zero latency. With three bands
//    the low band also passes an all-pass matching the upper split, so the bands sum to an all-pass.
//  - Linear phase: FIR low-passes with the magnitude of the LR4 low-pass (1 / (1 + (f/fc)^4)); the band above is
//    the delayed input minus the low-pass, so the bands sum to a pure delay. The FIRs run through the repository's
//    zero-latency partitioned convolver; when a crossover frequency moves, new kernels are designed on a worker
//    thread and cross-faded in.
#include "filters.h"
#include "params.h"
#include <atomic>
#include <memory>
#include <thread>

namespace fr { class PartitionedConvolver; }

namespace muon {

class LinearCrossover {
public:
    LinearCrossover();
    ~LinearCrossover();
    // Off the audio thread: designs the first kernels for f1, f2 and starts the worker.
    void prepare(double fs, int maxBlock, float f1, float f2);
    void release();
    int latency() const { return latency_; }
    // Audio thread. low1 = low-pass at f1, low2 = low-pass at f2, delayed = input delayed by latency().
    // Requests new kernels when f1 or f2 moved by more than 0.5%.
    void process(const float* L, const float* R, int n, float f1, float f2,
                 float* low1L, float* low1R, float* low2L, float* low2R, float* delL, float* delR);
    static std::vector<float> designLowpass(double fs, double fc, int taps);   // for tests

private:
    struct Kernel;
    void worker();
    double fs_ = 48000.0;
    int taps_ = 1025, latency_ = 512, maxBlock_ = 64;
    Kernel* cur_ = nullptr;          // audio thread
    Kernel* old_ = nullptr;          // audio thread, fading out
    Kernel* next_ = nullptr;         // audio thread, filling its history before the fade
    int fade_ = 0, fadeLen_ = 1024, warm_ = 0;
    std::atomic<Kernel*> ready_{nullptr}, trash_{nullptr};
    std::atomic<float> wantF1_{120.f}, wantF2_{2500.f};
    std::atomic<bool> run_{false};
    std::thread thread_;
    Delay dl_[2];
    std::vector<float> tmp_[4], scratch_[2];
};

class Crossover {
public:
    void prepare(double fs, int maxBlock, const Params& p);
    void release() { linear_.release(); }
    void reset();
    int latency(CrossoverType t) const { return t == CrossoverType::LinearPhase ? linear_.latency() : 0; }
    // Splits n samples (n <= maxBlock) into bands (out[band][channel]). Unused bands are left untouched.
    void process(const float* L, const float* R, int n, const Params& p, float* const out[kMaxBands][2]);
private:
    void updateCoefs(float f1, float f2);
    double fs_ = 48000.0;
    struct Ch { Svf lp1[2], hp1[2], lp2[2], hp2[2], ap2; } ch_[2];
    float f1_ = -1.f, f2_ = -1.f;
    LinearCrossover linear_;
    std::vector<float> l1_[2], l2_[2], d_[2];
};

}  // namespace muon
