#pragma once
#include <algorithm>
#include <vector>

namespace cs {
// Fixed stereo delay (whole samples). configure() allocates; process() does not.
class DelayLine {
public:
    void configure(int samples) { d_ = std::max(0, samples); buf_[0].assign((size_t)d_ + 1, 0.f); buf_[1].assign((size_t)d_ + 1, 0.f); pos_ = 0; }
    void reset() { std::fill(buf_[0].begin(), buf_[0].end(), 0.f); std::fill(buf_[1].begin(), buf_[1].end(), 0.f); pos_ = 0; }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n) {
        if (d_ == 0) { for (int i = 0; i < n; ++i) { outL[i] = inL[i]; outR[i] = inR[i]; } return; }
        const int size = d_ + 1;
        for (int i = 0; i < n; ++i) {
            const float l = inL[i], r = inR[i];
            buf_[0][(size_t)pos_] = l; buf_[1][(size_t)pos_] = r;
            int rd = pos_ - d_; if (rd < 0) rd += size;
            outL[i] = buf_[0][(size_t)rd]; outR[i] = buf_[1][(size_t)rd];
            if (++pos_ >= size) pos_ = 0;
        }
    }
private:
    int d_ = 0, pos_ = 0;
    std::vector<float> buf_[2];
};
}  // namespace cs
