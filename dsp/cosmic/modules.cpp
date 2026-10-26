#include "cosmic/modules.h"
#include <algorithm>
#include <cmath>

namespace cs {
namespace {
constexpr double kPi = 3.14159265358979323846;
inline float lerpRead(const std::vector<float>& b, int size, double pos) {
    const int i0 = (int)std::floor(pos);
    const float f = (float)(pos - i0);
    const int a = ((i0 % size) + size) % size, c = (a + 1) % size;
    return b[(size_t)a] + (b[(size_t)c] - b[(size_t)a]) * f;
}
}  // namespace

// ---- GrainPitchShifter ---------------------------------------------------------------------------
void GrainPitchShifter::configure(double fs, double maxGrainMs) {
    fs_ = fs;
    size_ = (int)(maxGrainMs * fs / 1000.0) + 8;
    buf_.assign((size_t)size_, 0.f);
    setGrainMs(50.0);
    reset();
}
void GrainPitchShifter::reset() { std::fill(buf_.begin(), buf_.end(), 0.f); wp_ = 0; phase_ = 0.f; }
void GrainPitchShifter::setSemitones(double st) { ratio_ = std::pow(2.0, std::clamp(st, -24.0, 24.0) / 12.0); inc_ = (float)((1.0 - ratio_) / grain_); }
void GrainPitchShifter::setGrainMs(double ms) { grain_ = (float)std::clamp(ms * fs_ / 1000.0, 64.0, (double)(size_ - 8)); inc_ = (float)((1.0 - ratio_) / grain_); }

void GrainPitchShifter::process(const float* in, float* out, int n) {
    for (int i = 0; i < n; ++i) {
        buf_[(size_t)wp_] = in[i];
        // Two read heads half a grain apart; each sweeps a delay of 0..grain and is faded by a
        // raised-cosine window, so the sum stays continuous while the heads jump.
        float acc = 0.f;
        for (int h = 0; h < 2; ++h) {
            float ph = phase_ + (h ? 0.5f : 0.f);
            ph -= std::floor(ph);
            const double delay = ph * grain_;
            const float win = 0.5f - 0.5f * std::cos(2.f * (float)kPi * ph);
            acc += win * lerpRead(buf_, size_, (double)wp_ - delay);
        }
        out[i] = acc;
        phase_ += inc_;
        phase_ -= std::floor(phase_);
        if (++wp_ >= size_) wp_ = 0;
    }
}

// ---- SweptFilter ---------------------------------------------------------------------------------
void SweptFilter::configure(double fs, uint64_t seed) { fs_ = fs; rng_ = Rng(seed); reset(); update(); }
void SweptFilter::reset() { ic1_ = ic2_ = 0.f; phase_ = 0; walk_ = 0; ctr_ = 0; }
void SweptFilter::set(int mode, double cutoffHz, double q, double lfoRateHz, double lfoDepthOct, double lfoPhase, double randomWalk) {
    mode_ = mode; cutoff_ = cutoffHz; q_ = std::max(0.3, q); rate_ = lfoRateHz; depth_ = lfoDepthOct; phase_ = lfoPhase; walkAmt_ = randomWalk;
}
void SweptFilter::update() {
    const double dt = 16.0 / fs_;
    phase_ += rate_ * dt; if (phase_ >= 1.0) phase_ -= 1.0;
    walk_ = 0.995 * walk_ + 0.005 * (rng_.uniform() * 2.0 - 1.0);
    lfo_ = std::sin(2 * kPi * phase_) * depth_ + walk_ * walkAmt_ * 6.0;
    const double fc = std::clamp(cutoff_ * std::pow(2.0, lfo_), 20.0, fs_ * 0.45);
    g_ = (float)std::tan(kPi * fc / fs_);
    k_ = (float)(1.0 / q_);
    a1_ = 1.f / (1.f + g_ * (g_ + k_)); a2_ = g_ * a1_; a3_ = g_ * a2_;
}
void SweptFilter::process(const float* in, float* out, int n) {
    for (int i = 0; i < n; ++i) {
        if (ctr_ == 0) update();
        if (++ctr_ >= 16) ctr_ = 0;
        const float v0 = in[i];
        const float v3 = v0 - ic2_;
        const float v1 = a1_ * ic1_ + a2_ * v3;
        const float v2 = ic2_ + a2_ * ic1_ + a3_ * v3;
        ic1_ = 2.f * v1 - ic1_; ic2_ = 2.f * v2 - ic2_;
        out[i] = mode_ == 0 ? v2 : mode_ == 1 ? v1 : v0 - k_ * v1 - v2;
    }
}

// ---- Saturator -----------------------------------------------------------------------------------
void Saturator::configure(double fs) { dc_ = (float)std::exp(-2 * kPi * 10.0 / fs); reset(); setDrive(0.0); }
void Saturator::setDrive(double d) {
    d = std::clamp(d, 0.0, 1.0);
    pre_ = (float)(1.0 + 23.0 * d);             // up to +27.6 dB into the curve
    post_ = 1.f / pre_;                          // unity small-signal gain
}
void Saturator::process(const float* in, float* out, int n) {
    for (int i = 0; i < n; ++i) {
        const float x = in[i] * pre_;
        // Rational tanh approximation, exact at 0, saturates at +-1.
        const float x2 = x * x;
        const float y = x * (27.f + x2) / (27.f + 9.f * x2);
        const float s = (y > 1.f ? 1.f : y < -1.f ? -1.f : y) * post_;
        const float o = s - x1_ + dc_ * y1_;      // DC blocker
        x1_ = s; y1_ = o;
        out[i] = o;
    }
}

// ---- ReverseBuffer -------------------------------------------------------------------------------
void ReverseBuffer::configure(double fs, double maxChunkMs) {
    fsForChunk_ = fs;
    size_ = (int)(maxChunkMs * fs / 1000.0) * 2 + 16;
    buf_.assign((size_t)size_, 0.f);
    chunk_ = (int)(0.5 * fs);
    reset();
}
void ReverseBuffer::reset() { std::fill(buf_.begin(), buf_.end(), 0.f); wp_ = 0; pos_ = 0; }
void ReverseBuffer::setChunkMs(double ms) { chunk_ = std::clamp((int)(ms * fsForChunk_ / 1000.0), 256, (size_ - 16) / 2); }

void ReverseBuffer::process(const float* in, float* out, int n) {
    const int L = chunk_;
    for (int i = 0; i < n; ++i) {
        buf_[(size_t)wp_] = in[i];
        // Two overlapping windows, half a chunk apart. Each reads its chunk backwards: the chunk
        // that ended when the window started.
        float acc = 0.f;
        for (int h = 0; h < 2; ++h) {
            int p = pos_ + (h ? L / 2 : 0); if (p >= L) p -= L;
            const int chunkStart = wp_ - p;                  // where this window began writing
            const int readIdx = chunkStart - p;              // mirror within the previous chunk
            const float win = 0.5f - 0.5f * std::cos(2.f * (float)kPi * (float)p / (float)L);
            acc += win * buf_[(size_t)(((readIdx % size_) + size_) % size_)];
        }
        out[i] = acc;
        if (++pos_ >= L) pos_ = 0;
        if (++wp_ >= size_) wp_ = 0;
    }
}

// ---- Granulator ----------------------------------------------------------------------------------
void Granulator::configure(double fs, uint64_t seed, int voices) {
    fs_ = fs; rng_ = Rng(seed);
    size_ = (int)fs + 64;
    buf_.assign((size_t)size_ * 2, 0.f);       // interleaved L/R
    grains_.assign((size_t)voices, Grain());
    reset();
}
void Granulator::reset() { std::fill(buf_.begin(), buf_.end(), 0.f); for (auto& g : grains_) g.on = false; wp_ = 0; spawnAcc_ = 0; }
void Granulator::set(double density, double grainMs, double spreadMs, double up, double down, double width) {
    density_ = density; grain_ = grainMs / 1000.0; spread_ = spreadMs / 1000.0; up_ = up; down_ = down; width_ = width;
}
void Granulator::spawn() {
    for (auto& g : grains_) {
        if (g.on) continue;
        g.on = true; g.age = 0;
        g.len = std::max(64, (int)(grain_ * fs_ * (0.7 + 0.6 * rng_.uniform())));
        const double back = 0.02 * fs_ + rng_.uniform() * spread_ * fs_;        // start in the recent past
        g.pos = (double)wp_ - back - g.len;                                        // play forward from there
        const double u = rng_.uniform();
        g.inc = u < up_ ? 2.0 : u < up_ + down_ ? 0.5 : 1.0;
        if (g.inc > 1.0) g.pos -= g.len;                                           // needs twice the history
        const double pan = 0.5 + (rng_.uniform() - 0.5) * width_;
        g.gl = (float)std::cos(pan * kPi / 2); g.gr = (float)std::sin(pan * kPi / 2);
        return;
    }
}
void Granulator::process(const float* inL, const float* inR, float* outL, float* outR, int n) {
    for (int i = 0; i < n; ++i) {
        buf_[(size_t)wp_ * 2] = inL[i]; buf_[(size_t)wp_ * 2 + 1] = inR[i];
        spawnAcc_ += density_ / fs_;
        while (spawnAcc_ >= 1.0) { spawn(); spawnAcc_ -= 1.0; }
        float l = 0.f, r = 0.f;
        for (auto& g : grains_) {
            if (!g.on) continue;
            const float win = 0.5f - 0.5f * std::cos(2.f * (float)kPi * (float)g.age / (float)g.len);
            const int i0 = (int)std::floor(g.pos), a = ((i0 % size_) + size_) % size_, c = (a + 1) % size_;
            const float f = (float)(g.pos - i0);
            const float sl = buf_[(size_t)a * 2] + (buf_[(size_t)c * 2] - buf_[(size_t)a * 2]) * f;
            const float sr = buf_[(size_t)a * 2 + 1] + (buf_[(size_t)c * 2 + 1] - buf_[(size_t)a * 2 + 1]) * f;
            const float m = 0.5f * (sl + sr) * win;
            l += m * g.gl; r += m * g.gr;
            g.pos += g.inc;
            if (++g.age >= g.len) g.on = false;
        }
        const float norm = 1.0f / std::sqrt(std::max(1.f, (float)grains_.size() * 0.25f));
        outL[i] = l * norm; outR[i] = r * norm;
        if (++wp_ >= size_) wp_ = 0;
    }
}

// ---- PulseGate -----------------------------------------------------------------------------------
void PulseGate::configure(double fs) { fs_ = fs; reset(); set(2.0, 0.5, 0.0, 0.0, 200.0); }
void PulseGate::set(double rateHz, double shape, double depth, double duck, double duckReleaseMs) {
    rate_ = rateHz; shape_ = std::clamp(shape, 0.0, 1.0); depth_ = std::clamp(depth, 0.0, 1.0); duck_ = std::clamp(duck, 0.0, 1.0);
    rel_ = std::exp(-1.0 / (duckReleaseMs * 0.001 * fs_));
}
void PulseGate::process(float* l, float* r, const float* sideL, const float* sideR, int n) {
    const double k = 1.0 + shape_ * 30.0;                         // sine -> near-square
    for (int i = 0; i < n; ++i) {
        const double s = std::sin(2 * kPi * phase_);
        const double shaped = std::tanh(k * s) / std::tanh(k);    // -1..1
        double gain = 1.0 - depth_ * 0.5 * (1.0 - shaped);
        if (sideL) {
            const double x = std::max(std::fabs((double)sideL[i]), std::fabs((double)sideR[i]));
            env_ = x > env_ ? x : env_ * rel_;
            gain *= 1.0 - duck_ * std::min(1.0, env_ * 3.0);
        }
        l[i] *= (float)gain; r[i] *= (float)gain;
        phase_ += rate_ / fs_; if (phase_ >= 1.0) phase_ -= 1.0;
    }
}

}  // namespace cs
