#pragma once
#include "er/er_default_table.h"
#include "common/simd.h"
#include <vector>

namespace cs {

// Sparse multi-tap early-reflection stage. One mono circular buffer, N taps with
// gain and pan. Taps are sorted by delay; `taps` selects how many are used.
class EarlyReflections {
public:
    // sizeScale stretches all tap delays (room size). Allocates.
    void configure(double fs, int taps, double sizeScale, double maxPreDelaySec = 0.0);
    void reset();
    void process(const float* inL, const float* inR, float* outL, float* outR, int n);

private:
    struct Tap { int delay; float gL, gR; };
    std::vector<Tap> taps_;
    std::vector<float> buf_;
    int size_ = 1, wp_ = 0;
};

}  // namespace cs
