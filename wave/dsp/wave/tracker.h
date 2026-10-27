#pragma once
// WAVE pitch tracker: one voice in, one analysis frame out every hop (32 decimated samples, about 2.7 ms).
// High-pass, anti-alias low-pass, decimation to about 11-12 kHz, then YIN (difference function, cumulative mean
// normalisation, absolute threshold, parabolic interpolation) on a window sized from the Range setting, plus RMS
// levels and a brightness measure. prepare() allocates; push() does not.
#include <vector>

namespace wave {

// Lowest pitch per range: Low (from E2), Mid (from A2), High (from E3), Very high (from A3). The window, and so the
// latency, scales with the lowest period.
inline constexpr const char* kRangeNames[] = {"Low (E2)", "Mid (A2)", "High (E3)", "Very high (A3)"};
inline constexpr float kRangeMinHz[] = {75.f, 100.f, 150.f, 200.f};
inline constexpr float kMaxHz = 1600.f;
inline constexpr int kNumRanges = 4;

struct Frame {
    float pitch = -1.f;          // fractional MIDI note, -1 when there is no estimate
    float conf = 0.f;            // 1 - YIN's normalised difference at the chosen lag (1 = perfectly periodic)
    float levelDb = -120.f;      // RMS of the last 5 ms, or of the longest period of the range if longer
    float bright = 0.f;          // 0..1, energy above 1.5 kHz relative to the whole
};

class Tracker {
public:
    void prepare(double fs);
    void reset();
    void setRange(int r);
    int range() const { return range_; }
    // One input sample; true when a hop completed and frame() holds a new analysis.
    bool push(float x);
    const Frame& frame() const { return frame_; }
    double analysisRate() const { return fa_; }
    double hopSeconds() const { return kHop / fa_; }
    static constexpr int kHop = 32;

private:
    struct Biquad {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        float run(float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
        void lowpass(double fs, double f, double q);
        void highpass(double fs, double f, double q);
        void clear() { z1 = z2 = 0; }
    };
    void analyse();

    double fs_ = 48000.0, fa_ = 12000.0;
    int decim_ = 4, phase_ = 0, hopCount_ = 0, range_ = 1;
    int maxLag_ = 160, minLag_ = 7;
    Biquad hp_, aa_[2];
    float brightLp_ = 0.f, brightA_ = 0.f, eAll_ = 0.f, eHigh_ = 0.f, eA_ = 0.f;
    // Decimated history, written twice (at i and i + kRing) so the last N samples are always contiguous.
    static constexpr int kRing = 1024;
    std::vector<float> ring_;
    int pos_ = 0, filled_ = 0;
    std::vector<float> d_, raw_;              // YIN's normalised and raw difference functions
    Frame frame_;
};

}  // namespace wave
