#include "filters.h"

namespace muon {

// ---- Half-band design (elliptic prototype; the method of HIIR's PolyphaseIir2Designer) ------------------
namespace {
double ipow(double x, long n) { double r = 1.0; while (n-- > 0) r *= x; return r; }
void transitionParams(double tbw, double& k, double& q) {
    k = std::tan((1.0 - tbw * 2.0) * kPi / 4.0); k *= k;
    const double kk = std::pow(1.0 - k * k, 0.25), e = 0.5 * (1.0 - kk) / (1.0 + kk), e2 = e * e, e4 = e2 * e2;
    q = e * (1.0 + e4 * (2.0 + e4 * (15.0 + 150.0 * e4)));
}
int orderFor(double att, double q) {
    const double ap = std::pow(10.0, -att / 10.0), a = ap / (1.0 - ap);
    int o = (int)std::ceil(std::log(a * a / 16.0) / std::log(q));
    if (o % 2 == 0) ++o;
    return std::max(o, 3);
}
double accNum(double q, int order, int c) {
    double acc = 0.0, t; int i = 0, j = 1;
    do { t = ipow(q, (long)i * (i + 1)) * std::sin((i * 2 + 1) * c * kPi / order) * j; acc += t; j = -j; ++i; } while (std::fabs(t) > 1e-100);
    return acc;
}
double accDen(double q, int order, int c) {
    double acc = 0.0, t; int i = 1, j = -1;
    do { t = ipow(q, (long)i * i) * std::cos(i * 2 * c * kPi / order) * j; acc += t; j = -j; ++i; } while (std::fabs(t) > 1e-100);
    return acc;
}
}  // namespace

std::vector<double> designHalfband(double attDb, double tbw) {
    double k, q; transitionParams(tbw, k, q);
    const int order = orderFor(attDb, q), n = (order - 1) / 2;
    std::vector<double> c((size_t)n);
    for (int i = 0; i < n; ++i) {
        const double num = accNum(q, order, i + 1) * std::pow(q, 0.25), den = accDen(q, order, i + 1) + 0.5;
        const double ww = num / den, w2 = ww * ww, x = std::sqrt((1.0 - w2 * k) * (1.0 - w2 / k)) / (1.0 + w2);
        c[(size_t)i] = (1.0 - x) / (1.0 + x);
    }
    return c;
}

// ---- Hilbert ---------------------------------------------------------------------------------------------
void Hilbert::prepare(double fs) {
    // A half-band with a transition of 2 x 15 Hz, shifted by fs/4: the transition lands at DC and Nyquist.
    auto c = designHalfband(70.0, std::min(0.02, 30.0 / fs));
    n0_ = n1_ = 0;
    for (size_t i = 0; i < c.size(); ++i) {
        if (i % 2 == 0) { if (n0_ < kMax) s0_[n0_++].c = (float)c[i]; }
        else if (n1_ < kMax) s1_[n1_++].c = (float)c[i];
    }
    reset();
}
void Hilbert::reset() {
    for (auto& s : s0_) s.x1 = s.x2 = s.y1 = s.y2 = 0.f;
    for (auto& s : s1_) s.x1 = s.x2 = s.y1 = s.y2 = 0.f;
    d_ = 0.f;
}

// ---- 2x stages ------------------------------------------------------------------------------------------
void Stage2x::setupIir(const std::vector<double>& c) {
    fir_ = false; p0_.clear(); p1_.clear();
    for (size_t i = 0; i < c.size(); ++i) (i % 2 == 0 ? p0_ : p1_).push_back({(float)c[i], 0.f, 0.f});
    reset();
}

void Stage2x::setupFir(int halfLen, double tbw) {
    fir_ = true; half_ = halfLen;
    // Kaiser-windowed half-band sinc; only the odd offsets are nonzero apart from the centre (0.5).
    const double att = 100.0, beta = 0.1102 * (att - 8.7);
    auto bessel0 = [](double x) { double s = 1.0, t = 1.0; for (int k = 1; k < 50; ++k) { t *= (x / (2.0 * k)) * (x / (2.0 * k)); s += t; } return s; };
    const double i0b = bessel0(beta);
    (void)tbw;
    taps_.clear();
    double sum = 0.0;
    for (int d = 1; d <= half_; d += 2) {
        const double r = (double)d / (double)(half_ + 1);
        const double w = bessel0(beta * std::sqrt(std::max(0.0, 1.0 - r * r))) / i0b;
        const double h = std::sin(kPi * d / 2.0) / (kPi * d) * w;   // 0.5 * sinc(d/2)
        taps_.push_back((float)h);
        sum += h;
    }
    // Both polyphase branches at unity DC gain: centre 0.5, the odd taps (both sides) summing to 0.5.
    for (auto& t : taps_) t = (float)(t * 0.25 / sum);
    histLen_ = 2 * half_ + 2;
    hist_.assign((size_t)histLen_ * 2, 0.f);
    lowLen_ = half_ + 1;
    histLow_.assign((size_t)lowLen_ * 2, 0.f);
    reset();
}

void Stage2x::reset() {
    for (auto& s : p0_) s.x1 = s.y1 = 0.f;
    for (auto& s : p1_) s.x1 = s.y1 = 0.f;
    odd_ = 0.f;
    std::fill(hist_.begin(), hist_.end(), 0.f);
    std::fill(histLow_.begin(), histLow_.end(), 0.f);
    pos_ = posLow_ = 0;
}

void Stage2x::up(const float* in, float* out, int n) {
    if (!fir_) {
        for (int i = 0; i < n; ++i) {
            float y0 = in[i], y1 = in[i];
            for (auto& s : p0_) { const float o = s.c * (y0 - s.y1) + s.x1; s.x1 = y0; s.y1 = o; y0 = o; }
            for (auto& s : p1_) { const float o = s.c * (y1 - s.y1) + s.x1; s.x1 = y1; s.y1 = o; y1 = o; }
            out[2 * i] = y0; out[2 * i + 1] = y1;
        }
        return;
    }
    // FIR: history x[n], x[n-1], ... mirrored so a window is contiguous. Even output: centre tap (a pure delay of
    // half_/2 low-rate samples); odd output: the odd taps.
    const int nt = (int)taps_.size();
    for (int i = 0; i < n; ++i) {
        posLow_ = (posLow_ == 0 ? lowLen_ : posLow_) - 1;
        histLow_[(size_t)posLow_] = histLow_[(size_t)(posLow_ + lowLen_)] = in[i];
        const float* h = &histLow_[(size_t)posLow_];        // h[k] = x[n - k]
        out[2 * i] = h[half_ / 2];                          // centre: 2 * 0.5 * x[n - H/2]
        // y[2n+1] = 2 * sum_{j odd} g[j] x[n - (j-1)/2], g[j] = taps at offset |j - H|.
        // j = H - d (d odd, d = 1..H-1 step 2) gives index (H - d - 1)/2; j = H + d gives (H + d - 1)/2.
        float acc = 0.f;
        for (int t = 0; t < nt; ++t) {
            const int d = 2 * t + 1;
            acc += taps_[(size_t)t] * (h[(half_ - d - 1) / 2] + h[(half_ + d - 1) / 2]);
        }
        out[2 * i + 1] = 2.f * acc;
    }
}

void Stage2x::down(const float* in, float* out, int n) {
    if (!fir_) {
        for (int i = 0; i < n; ++i) {
            float y0 = in[2 * i], y1 = odd_;
            odd_ = in[2 * i + 1];
            for (auto& s : p0_) { const float o = s.c * (y0 - s.y1) + s.x1; s.x1 = y0; s.y1 = o; y0 = o; }
            for (auto& s : p1_) { const float o = s.c * (y1 - s.y1) + s.x1; s.x1 = y1; s.y1 = o; y1 = o; }
            out[i] = 0.5f * (y0 + y1);
        }
        return;
    }
    const int nt = (int)taps_.size();
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < 2; ++k) {
            pos_ = (pos_ == 0 ? histLen_ : pos_) - 1;
            hist_[(size_t)pos_] = hist_[(size_t)(pos_ + histLen_)] = in[2 * i + k];
        }
        // After writing v[2n] then v[2n+1], v[2n - j] sits at offset j + 1.
        const float* h = &hist_[(size_t)pos_ + 1];
        float acc = 0.5f * h[half_];
        for (int t = 0; t < nt; ++t) {
            const int d = 2 * t + 1;
            acc += taps_[(size_t)t] * (h[half_ - d] + h[half_ + d]);
        }
        out[i] = acc;
    }
}

// ---- Resampler ------------------------------------------------------------------------------------------
namespace {
struct StageSpec { double tbw; int firHalf; };
// Stage k runs between rates 2^k fs and 2^(k+1) fs. FIR half-lengths are chosen so the round-trip latency is
// an integer number of base samples (H1 even, H2 a multiple of 2, H3 a multiple of 4).
constexpr StageSpec kStages[3] = {{0.04, 80}, {0.15, 22}, {0.30, 20}};
int stagesFor(int factor) { return factor >= 8 ? 3 : factor >= 4 ? 2 : factor >= 2 ? 1 : 0; }
}  // namespace

int resamplerLatency(int factor, bool linear) {
    if (!linear) return 0;
    int lat = 0;
    for (int s = 0; s < stagesFor(factor); ++s) lat += kStages[s].firHalf >> s;   // H_k / 2^k per way, two ways
    return lat;
}

void Resampler::prepare(int factor, bool linear, int maxBlock) {
    nStages_ = stagesFor(factor);
    factor_ = 1 << nStages_;
    for (int s = 0; s < nStages_; ++s) {
        if (linear) { upSt_[s].setupFir(kStages[s].firHalf, kStages[s].tbw); downSt_[s].setupFir(kStages[s].firHalf, kStages[s].tbw); }
        else { const auto c = designHalfband(100.0, kStages[s].tbw); upSt_[s].setupIir(c); downSt_[s].setupIir(c); }
    }
    latency_ = resamplerLatency(factor_, linear);
    tmpA_.assign((size_t)maxBlock * 8, 0.f);
    tmpB_.assign((size_t)maxBlock * 8, 0.f);
}

void Resampler::reset() { for (int s = 0; s < 3; ++s) { upSt_[s].reset(); downSt_[s].reset(); } }

void Resampler::up(const float* in, float* out, int n) {
    if (nStages_ == 0) { std::memcpy(out, in, sizeof(float) * (size_t)n); return; }
    const float* src = in; int len = n;
    for (int s = 0; s < nStages_; ++s) {
        float* dst = s == nStages_ - 1 ? out : (s % 2 == 0 ? tmpA_.data() : tmpB_.data());
        upSt_[s].up(src, dst, len);
        src = dst; len *= 2;
    }
}

void Resampler::down(const float* in, float* out, int n) {
    if (nStages_ == 0) { std::memcpy(out, in, sizeof(float) * (size_t)n); return; }
    const float* src = in; int len = n << nStages_;
    for (int s = nStages_ - 1; s >= 0; --s) {
        len /= 2;
        float* dst = s == 0 ? out : (s % 2 == 0 ? tmpA_.data() : tmpB_.data());
        downSt_[s].down(src, dst, len);
        src = dst;
    }
}

}  // namespace muon
