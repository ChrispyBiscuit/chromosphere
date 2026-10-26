#pragma once
// Zero-latency partitioned FFT convolution (single thread, audio-thread safe).
//
// Layout of the impulse response (taps from 0):
//   [0, 64)            direct form, one multiply-add per tap per sample
//   [P_l, 4 P_l)       level l, uniform partitions of P_l = 64 * 4^l taps, FFT size 2 P_l, l = 0..3
//                      (the last level carries every remaining tap)
// A partition of size P that starts at tap q*P (q >= 1) only needs input that is already
// complete when its output block starts, so no latency is added. The products with older input
// blocks are spread over the preceding block period, so no block does all the work at once.
#include <cstdint>
#include <vector>

struct PFFFT_Setup;

namespace cs {

class PartitionedConvolver {
public:
    PartitionedConvolver();
    ~PartitionedConvolver();
    PartitionedConvolver(const PartitionedConvolver&) = delete;
    PartitionedConvolver& operator=(const PartitionedConvolver&) = delete;

    // ir: 1 channel (same filter on L and R), 2 channels (L->L, R->R) or 4 channels
    // (L->L, L->R, R->L, R->R). Allocates and transforms; call off the audio thread.
    void configure(const std::vector<std::vector<float>>& ir);
    void reset();   // clear all signal history (keeps the filter)
    // Any n >= 0. outL/outR may alias inL/inR. No allocation.
    void process(const float* inL, const float* inR, float* outL, float* outR, int n);

    int irLength() const { return irLen_; }
    size_t memoryBytes() const;

private:
    static constexpr int kHead = 64;      // direct-form taps
    static constexpr int kStep = 64;      // internal step; also the smallest partition size
    static constexpr int kLevels = 4;
    static constexpr int kMaxFilters = 4;

    struct Buf {                          // aligned float array
        float* p = nullptr; size_t n = 0;
        void alloc(size_t count);
        void release();
        ~Buf() { release(); }
        Buf() = default;
        Buf(const Buf&) = delete; Buf& operator=(const Buf&) = delete;
        Buf(Buf&& o) noexcept : p(o.p), n(o.n) { o.p = nullptr; o.n = 0; }
        Buf& operator=(Buf&& o) noexcept { release(); p = o.p; n = o.n; o.p = nullptr; o.n = 0; return *this; }
    };
    struct Level {
        int P = 0, N = 0, m = 0;          // partition size, FFT size, partition count (0 = unused)
        PFFFT_Setup* setup = nullptr;
        std::vector<Buf> H;               // [filter * m + (q-1)] spectra
        std::vector<Buf> X[2];            // per input channel: ring of m spectra (newest at xHead)
        int xHead = 0;
        Buf acc[2];                       // per output channel
        Buf tmp, work;
        int jobTotal = 0, jobDone = 0, jobsPerStep = 0;
    };
    struct Filter { int in, out; };

    void boundary(Level& L, int64_t t);
    void runJobs(Level& L, int count);
    void clear();

    std::vector<Filter> filters_;
    Level lev_[kLevels];
    std::vector<float> head_[kMaxFilters];       // reversed head taps
    int irLen_ = 0, nIn_ = 2;
    int64_t t_ = 0;

    // Signal history: input rings (power of two), output accumulation rings, linear head buffers.
    static constexpr int kRing = 16384;
    std::vector<float> inRing_[2];
    std::vector<float> outRing_[2];
    std::vector<float> lin_[2];
};

}  // namespace cs
