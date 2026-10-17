#include "conv/ir_tools.h"
#include <algorithm>
#include <cmath>

namespace fr {
namespace {
constexpr double kPi = 3.14159265358979323846;

double besselI0(double x) {
    double s = 1, t = 1;
    for (int k = 1; k < 60; ++k) { t *= (x / (2.0 * k)) * (x / (2.0 * k)); s += t; if (t < 1e-14 * s) break; }
    return s;
}

struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    float tick(float x) {
        double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float)y;
    }
};
// RBJ cookbook, Butterworth Q.
Biquad design(bool highpass, double f, double fs, double q = 0.70710678) {
    Biquad b;
    double w = 2 * kPi * f / fs, c = std::cos(w), s = std::sin(w), al = s / (2 * q), a0 = 1 + al;
    if (highpass) { b.b0 = (1 + c) / 2 / a0; b.b1 = -(1 + c) / a0; b.b2 = (1 + c) / 2 / a0; }
    else          { b.b0 = (1 - c) / 2 / a0; b.b1 = (1 - c) / a0;  b.b2 = (1 - c) / 2 / a0; }
    b.a1 = -2 * c / a0; b.a2 = (1 - al) / a0;
    return b;
}
Biquad bandpass(double f, double fs, double q) {
    Biquad b;
    double w = 2 * kPi * f / fs, c = std::cos(w), s = std::sin(w), al = s / (2 * q), a0 = 1 + al;
    b.b0 = al / a0; b.b1 = 0; b.b2 = -al / a0; b.a1 = -2 * c / a0; b.a2 = (1 - al) / a0;
    return b;
}
void filterInPlace(std::vector<float>& x, std::vector<Biquad> chain) {
    for (float& v : x) for (Biquad& b : chain) v = b.tick(v);
}
}  // namespace

std::vector<float> resample(const std::vector<float>& x, double ratio) {
    if (x.empty() || std::fabs(ratio - 1.0) < 1e-9) return x;
    const int zc = 32;
    const double beta = 9.0, cutoff = std::min(1.0, ratio) * 0.97;   // low-pass at the lower Nyquist
    const double invI0 = 1.0 / besselI0(beta);
    const size_t outLen = (size_t)std::ceil(x.size() * ratio);
    std::vector<float> y(outLen);
    const double half = zc / cutoff;                                  // kernel half-width in input samples
    for (size_t n = 0; n < outLen; ++n) {
        const double pos = n / ratio;
        const long i0 = (long)std::ceil(pos - half), i1 = (long)std::floor(pos + half);
        double acc = 0;
        for (long i = i0; i <= i1; ++i) {
            if (i < 0 || i >= (long)x.size()) continue;
            const double d = (pos - i) * cutoff;
            const double sinc = std::fabs(d) < 1e-12 ? 1.0 : std::sin(kPi * d) / (kPi * d);
            const double r = (pos - i) / half;
            const double win = std::fabs(r) >= 1 ? 0.0 : besselI0(beta * std::sqrt(1 - r * r)) * invI0;
            acc += x[(size_t)i] * sinc * win;
        }
        y[n] = (float)(acc * cutoff);
    }
    return y;
}

Ir resampleIr(const Ir& in, double newFs) {
    if (std::fabs(newFs - in.fs) < 0.5) return in;
    Ir out; out.fs = newFs;
    for (const auto& c : in.ch) out.ch.push_back(resample(c, newFs / in.fs));
    return out;
}

Ir editIr(const Ir& in, const IrEdit& e) {
    Ir r = in;
    const double msToS = in.fs / 1000.0;
    // Trim.
    size_t start = (size_t)std::max(0.0, e.startMs * msToS);
    for (auto& c : r.ch) {
        size_t s = std::min(start, c.size());
        size_t end = e.maxLengthMs > 0 ? std::min(c.size(), s + (size_t)(e.maxLengthMs * msToS)) : c.size();
        c = std::vector<float>(c.begin() + s, c.begin() + end);
    }
    if (e.reverse) for (auto& c : r.ch) std::reverse(c.begin(), c.end());
    if (std::fabs(e.stretch - 1.0) > 1e-6) for (auto& c : r.ch) c = resample(c, std::clamp(e.stretch, 0.25, 4.0));
    // Filters (second-order Butterworth, one pass; changes the phase, fine for reverb).
    if (e.lowCutHz > 0) for (auto& c : r.ch) filterInPlace(c, {design(true, e.lowCutHz, in.fs)});
    if (e.highCutHz > 0 && e.highCutHz < in.fs * 0.49) for (auto& c : r.ch) filterInPlace(c, {design(false, e.highCutHz, in.fs)});
    // Width: mid/side scale on L/R pairs (channels 0,1 and for true stereo 2,3).
    if (std::fabs(e.width - 1.0) > 1e-6 && r.ch.size() >= 2) {
        auto ms = [&](std::vector<float>& a, std::vector<float>& b) {
            for (size_t i = 0; i < std::min(a.size(), b.size()); ++i) {
                float m = 0.5f * (a[i] + b[i]), s = 0.5f * (a[i] - b[i]) * (float)e.width;
                a[i] = m + s; b[i] = m - s;
            }
        };
        if (r.ch.size() == 2) ms(r.ch[0], r.ch[1]);
        else if (r.ch.size() == 4) { ms(r.ch[0], r.ch[1]); ms(r.ch[2], r.ch[3]); }
    }
    // Decay shortening: gain 10^(-3 t (1/T' - 1/T)) with T'/T = decayScale, using the mid-band decay.
    if (e.decayScale < 0.999) {
        Ir tmp = r;
        const double t0 = analyzeIr(tmp).t60[1], k = 3.0 * (1.0 / std::max(0.05, e.decayScale) - 1.0) / std::max(0.05, t0);
        for (auto& c : r.ch) for (size_t i = 0; i < c.size(); ++i) c[i] *= (float)std::pow(10.0, -k * i / in.fs);
    }
    // Fade-out (half Hann) over the last fadeOutMs.
    if (e.fadeOutMs > 0) for (auto& c : r.ch) {
        size_t n = std::min(c.size(), (size_t)(e.fadeOutMs * msToS));
        for (size_t i = 0; i < n; ++i) c[c.size() - n + i] *= (float)(0.5 + 0.5 * std::cos(kPi * (i + 1.0) / n));
    }
    if (e.normalize) {
        // Power gain per output channel = sum of squared taps of the filters feeding it
        // (the output level for unit-variance white input). Scale the larger one to the target.
        auto sumsq = [](const std::vector<float>& h) { double s = 0; for (float v : h) s += (double)v * v; return s; };
        double pL, pR;
        if (r.ch.size() == 1) pL = pR = sumsq(r.ch[0]);
        else if (r.ch.size() == 2) { pL = sumsq(r.ch[0]); pR = sumsq(r.ch[1]); }
        else { pL = sumsq(r.ch[0]) + sumsq(r.ch[2]); pR = sumsq(r.ch[1]) + sumsq(r.ch[3]); }
        double p = std::max(pL, pR);
        if (p > 0) {
            float g = (float)std::sqrt(e.targetPowerGain / p);
            for (auto& c : r.ch) for (float& v : c) v *= g;
        }
    }
    return r;
}

double bandPower(const std::vector<float>& x, double fc, double fs, size_t from, size_t to) {
    to = std::min(to, x.size());
    if (to <= from) return 0;
    std::vector<float> y(x.begin(), x.begin() + to);
    Biquad bp = bandpass(std::min(fc, fs * 0.45), fs, std::sqrt(2.0));
    filterInPlace(y, {bp, bp});
    double s = 0;
    for (size_t i = from; i < to; ++i) s += (double)y[i] * y[i];
    return s / (to - from);
}

double windowEnergy(const Ir& ir, double fromMs, double toMs) {
    size_t a = (size_t)(fromMs * ir.fs / 1000), b = std::min(ir.length(), (size_t)(toMs * ir.fs / 1000));
    if (b <= a) return 0;
    double s = 0;
    for (size_t i = a; i < b; ++i) { double m = 0; for (const auto& c : ir.ch) m += (double)c[i] * c[i]; s += m / ir.ch.size(); }
    return s / (b - a);
}

double IrAnalysis::energyDb(const std::vector<float>& x, size_t from, size_t to) const {
    double s = 0; to = std::min(to, x.size());
    for (size_t i = from; i < to; ++i) s += (double)x[i] * x[i];
    return 10 * std::log10(s / std::max<size_t>(1, to - from) + 1e-30);
}

IrAnalysis analyzeIr(const Ir& ir) {
    IrAnalysis a;
    const size_t n = ir.length();
    if (n < 64) return a;
    std::vector<float> avg(n, 0.f);
    for (const auto& c : ir.ch) for (size_t i = 0; i < n; ++i) avg[i] += c[i] / (float)ir.ch.size();
    // Noise floor from the last 10%: cut the IR where the smoothed energy gets within 6 dB of it.
    const size_t tailStart = n - n / 10;
    const double floorDb = a.energyDb(avg, tailStart, n);
    const size_t win = std::max<size_t>(64, (size_t)(0.01 * ir.fs));
    a.usableLength = n;
    for (size_t i = 0; i + win < tailStart; i += win)
        if (a.energyDb(avg, i, i + win) < floorDb + 6.0) { a.usableLength = i; break; }
    // Octave-band T30 fits on the backward-integrated energy of the usable part.
    const double centres[3] = {125, 1000, 8000};
    for (int b = 0; b < 3; ++b) {
        std::vector<float> x(avg.begin(), avg.begin() + std::max<size_t>(64, a.usableLength));
        Biquad bp = bandpass(std::min(centres[b], ir.fs * 0.45), ir.fs, std::sqrt(2.0));
        filterInPlace(x, {bp, bp});
        std::vector<double> edc(x.size());
        double acc = 0;
        for (size_t i = x.size(); i-- > 0;) { acc += (double)x[i] * x[i]; edc[i] = acc; }
        if (edc[0] <= 0) continue;
        auto db = [&](size_t i) { return 10 * std::log10(edc[i] / edc[0] + 1e-30); };
        size_t i0 = 0, i1 = 0;
        while (i0 < x.size() && db(i0) > -5) ++i0;
        i1 = i0;
        while (i1 < x.size() && db(i1) > -35) ++i1;
        if (i1 - i0 < 16) { a.t60[b] = 0.3; continue; }
        // Least-squares line over [i0, i1).
        double sx = 0, sy = 0, sxx = 0, sxy = 0, m = (double)(i1 - i0);
        for (size_t i = i0; i < i1; ++i) { double t = i / ir.fs, y = db(i); sx += t; sy += y; sxx += t * t; sxy += t * y; }
        double slope = (m * sxy - sx * sy) / (m * sxx - sx * sx);
        a.t60[b] = std::clamp(-60.0 / slope, 0.1, 30.0);
    }
    return a;
}

}  // namespace fr
