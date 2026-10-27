#include "tracker.h"
#include <algorithm>
#include <cmath>

namespace wave {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr float kYinThreshold = 0.2f;      // lower (the paper's 0.1) sends vibrato-blurred dips to the octave-biased fallback
}

void Tracker::Biquad::lowpass(double fs, double f, double q) {
    const double w = 2.0 * kPi * f / fs, c = std::cos(w), al = std::sin(w) / (2.0 * q), a0 = 1.0 + al;
    b0 = (float)((1.0 - c) * 0.5 / a0); b1 = (float)((1.0 - c) / a0); b2 = b0;
    a1 = (float)(-2.0 * c / a0); a2 = (float)((1.0 - al) / a0);
}
void Tracker::Biquad::highpass(double fs, double f, double q) {
    const double w = 2.0 * kPi * f / fs, c = std::cos(w), al = std::sin(w) / (2.0 * q), a0 = 1.0 + al;
    b0 = (float)((1.0 + c) * 0.5 / a0); b1 = (float)(-(1.0 + c) / a0); b2 = b0;
    a1 = (float)(-2.0 * c / a0); a2 = (float)((1.0 - al) / a0);
}

void Tracker::prepare(double fs) {
    fs_ = fs;
    decim_ = std::max(1, (int)std::lround(fs / 11025.0));
    fa_ = fs / decim_;
    hp_.highpass(fs, 60.0, 0.7071);
    // 4th-order Butterworth low-pass at 0.3 x the decimated rate (voice fundamentals reach 1.6 kHz at most).
    aa_[0].lowpass(fs, 0.3 * fa_, 0.5412);
    aa_[1].lowpass(fs, 0.3 * fa_, 1.3066);
    brightA_ = (float)(1.0 - std::exp(-2.0 * kPi * 1500.0 / fs));
    eA_ = (float)(1.0 - std::exp(-1.0 / (0.03 * fs)));
    ring_.assign(2 * kRing, 0.f);
    d_.assign(kRing / 2 + 2, 0.f);
    raw_.assign(kRing / 2 + 2, 0.f);
    setRange(range_);
    reset();
}

void Tracker::reset() {
    hp_.clear(); aa_[0].clear(); aa_[1].clear();
    brightLp_ = eAll_ = eHigh_ = 0.f;
    std::fill(ring_.begin(), ring_.end(), 0.f);
    pos_ = filled_ = phase_ = hopCount_ = 0;
    frame_ = Frame{};
}

void Tracker::setRange(int r) {
    range_ = std::clamp(r, 0, kNumRanges - 1);
    maxLag_ = std::min(kRing / 2 - 1, (int)std::ceil(fa_ / kRangeMinHz[range_]));
    minLag_ = std::max(2, (int)std::floor(fa_ / kMaxHz));
}

bool Tracker::push(float x) {
    const float y = hp_.run(x);
    brightLp_ += brightA_ * (y - brightLp_);
    const float hi = y - brightLp_;
    eAll_ += eA_ * (y * y - eAll_);
    eHigh_ += eA_ * (hi * hi - eHigh_);
    const float z = aa_[1].run(aa_[0].run(y));
    if (++phase_ < decim_) return false;
    phase_ = 0;
    ring_[(size_t)pos_] = z; ring_[(size_t)(pos_ + kRing)] = z;
    pos_ = (pos_ + 1) & (kRing - 1);
    filled_ = std::min(filled_ + 1, kRing);
    if (++hopCount_ < kHop) return false;
    hopCount_ = 0;
    analyse();
    return true;
}

void Tracker::analyse() {
    const int W = maxLag_, N = W + maxLag_;
    const float* p = ring_.data() + ((pos_ - N + kRing) & (kRing - 1));   // the last N samples, contiguous

    // Levels: RMS over at least the longest period of the range (5 ms would ripple within a low note's period).
    const int L = std::max(W, (int)std::lround(0.005 * fa_));
    double ms = 0.0;
    for (int i = N - L; i < N; ++i) ms += (double)p[i] * p[i];
    frame_.levelDb = (float)(10.0 * std::log10(ms / L + 1e-12));
    const float ratio = eHigh_ / (eAll_ + 1e-12f);
    frame_.bright = std::clamp((10.f * std::log10(ratio + 1e-9f) + 40.f) / 35.f, 0.f, 1.f);

    frame_.pitch = -1.f; frame_.conf = 0.f;
    if (filled_ < N || frame_.levelDb < -90.f) return;

    // YIN: difference function, cumulative mean normalised.
    float* d = d_.data();
    d[0] = 1.f;
    double running = 0.0;
    for (int tau = 1; tau <= maxLag_; ++tau) {
        float s = 0.f;
        const float* a = p;
        const float* b = p + tau;
        for (int j = 0; j < W; ++j) { const float diff = a[j] - b[j]; s += diff * diff; }
        running += s;
        raw_[(size_t)tau] = s;
        d[tau] = running > 0.0 ? (float)(s * tau / running) : 1.f;
    }
    int tau = -1;
    for (int t = minLag_; t < maxLag_; ++t)
        if (d[t] < kYinThreshold) {
            while (t + 1 < maxLag_ && d[t + 1] < d[t]) ++t;
            tau = t;
            break;
        }
    if (tau < 0) {                                           // no dip under the threshold: the best one, for confidence
        tau = minLag_;
        for (int t = minLag_ + 1; t < maxLag_; ++t) if (d[t] < d[tau]) tau = t;
    }
    // Harmonic check: when an upper harmonic dominates (high sopranos: the 2nd or 3rd harmonic 15-20 dB over the
    // fundamental), the dip at 1/2 or 1/3 of the period is shallow but passes the threshold. The smallest multiple
    // of the lag (2x, 3x, 4x) with a clearly deeper dip is the true period. Dips are measured at their exact
    // fractional lag (cubic interpolation of the signal), relative to the window's energy: at integer lags a high
    // note's narrow dips look shallower than they are, and the normalised function favours longer lags.
    {
        double e = 0.0;
        for (int j = 0; j < N; ++j) e += (double)p[j] * p[j];
        const double scale = e > 0.0 ? N / (2.0 * W * e) : 0.0;
        auto depthAt = [&](double lag) {
            const int i0 = (int)lag;
            const float f = (float)(lag - i0);
            const float c0 = -0.5f * f * (1 - f) * (1 - f), c1 = 1 - 2.5f * f * f + 1.5f * f * f * f;    // Catmull-Rom
            const float c2 = f * (0.5f + 2 * f - 1.5f * f * f), c3 = -0.5f * f * f * (1 - f);
            double s = 0.0;
            for (int j = 0; j < W; ++j) {
                const float* q = p + j + i0;
                const float y = c0 * q[-1] + c1 * q[0] + c2 * q[1] + c3 * q[2];
                const float diff = p[j] - y;
                s += (double)diff * diff;
            }
            return s * scale;
        };
        auto bestDepth = [&](int t) {
            double best = 1e9;
            for (int k = -6; k <= 6; ++k) best = std::min(best, depthAt(t + 0.1 * k));
            return best;
        };
        const double here = d[tau] > 0.02f ? bestDepth(tau) : 0.0;
        for (int k = 2; here > 0.008 && k <= 4 && k * tau + k + 3 < maxLag_; ++k) {
            int tk = k * tau - k;
            for (int t = tk + 1; t <= k * tau + k; ++t) if (raw_[(size_t)t] < raw_[(size_t)tk]) tk = t;
            const double there = bestDepth(tk);
            if (there < 0.25 * here) {
                tau = tk;
                while (tau + 1 < maxLag_ && d[tau + 1] < d[tau]) ++tau;
                while (tau - 1 > minLag_ && d[tau - 1] < d[tau]) --tau;
                break;
            }
        }
    }
    const float a = d[tau - 1], b = d[tau], c = d[tau + 1], den = a - 2.f * b + c;
    const float shift = den > 0.f ? std::clamp(0.5f * (a - c) / den, -0.5f, 0.5f) : 0.f;
    const double f0 = fa_ / ((double)tau + shift);
    frame_.conf = std::clamp(1.f - b, 0.f, 1.f);
    frame_.pitch = (float)(69.0 + 12.0 * std::log2(f0 / 440.0));
}

}  // namespace wave
