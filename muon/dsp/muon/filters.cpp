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

// ---- FIR half-band and latency ---------------------------------------------------------------------------
std::vector<float> designFirHalfband(int halfLen) {
    const double att = 100.0, beta = 0.1102 * (att - 8.7);
    auto bessel0 = [](double x) { double s = 1.0, t = 1.0; for (int k = 1; k < 50; ++k) { t *= (x / (2.0 * k)) * (x / (2.0 * k)); s += t; } return s; };
    const double i0b = bessel0(beta);
    std::vector<float> taps;
    double sum = 0.0;
    for (int d = 1; d <= halfLen; d += 2) {
        const double r = (double)d / (double)(halfLen + 1);
        const double w = bessel0(beta * std::sqrt(std::max(0.0, 1.0 - r * r))) / i0b;
        const double h = std::sin(kPi * d / 2.0) / (kPi * d) * w;   // 0.5 * sinc(d/2)
        taps.push_back((float)h);
        sum += h;
    }
    for (auto& t : taps) t = (float)(t * 0.25 / sum);
    return taps;
}

int resamplerLatency(int factor, bool linear) {
    if (!linear) return 0;
    const int stages = factor >= 8 ? 3 : factor >= 4 ? 2 : factor >= 2 ? 1 : 0;
    int lat = 0;
    for (int s = 0; s < stages; ++s) lat += kStageSpecs[s].firHalf >> s;   // H_k / 2^k per way, two ways
    return lat;
}

}  // namespace muon
