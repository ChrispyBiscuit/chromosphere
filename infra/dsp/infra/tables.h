#pragma once
// The ten spectral tables. Each has kFrames single-cycle frames; each frame is stored at kLevels band-limited
// levels (level L holds harmonics 1 .. 1024 >> L). Built once per process, shared read-only.
#include "params.h"
#include <vector>

namespace infra {

class WavetableBank {
public:
    static constexpr int kSize = 2048;        // samples per cycle
    static constexpr int kFrames = 16;
    static constexpr int kLevels = 11;
    static constexpr int kStride = kSize + 1; // one guard sample so interpolation never wraps

    static const WavetableBank& get();        // thread-safe, builds on first use

    // Frame data for table t, frame f, level L. kStride floats, the last equal to the first.
    const float* frame(int t, int f, int L) const { return data_.data() + ((size_t)((t * kFrames + f) * kLevels + L)) * kStride; }

    // Level choice for a phase increment (cycles per sample): the richer level L0 and the blend toward L0 + 1.
    // L0 keeps every harmonic below Nyquist; its weight falls to zero as its top harmonic reaches Nyquist.
    static void levelFor(float inc, int& L0, float& frac);

private:
    WavetableBank();
    std::vector<float> data_;
};

// One sample from a (frame, level) pair at a phase in [0, 1).
inline float readTable(const float* tab, float phase) {
    const float x = phase * (float)WavetableBank::kSize;
    const int i = (int)x;
    const float fr = x - (float)i;
    return tab[i] + (tab[i + 1] - tab[i]) * fr;
}

}  // namespace infra
