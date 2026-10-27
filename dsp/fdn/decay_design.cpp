#include "fdn/decay_design.h"
#include <algorithm>
#include <cmath>
#include <complex>

namespace cs {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr struct { bool low; double fc; } kShelfDef[kShelves] = {
    {true, 200.0}, {true, 700.0}, {false, 1500.0}, {false, 5000.0}};
constexpr int kGrid = 15;  // 125 * 2^(0..7 step 0.5)

}  // namespace

Shelf makeShelf(bool low, double gain, double fc, double fs) {
    double k = 1.0 / std::tan(kPi * fc / fs);
    double b0, b1;
    if (low) { b0 = k + gain; b1 = gain - k; }
    else     { b0 = gain * k + 1.0; b1 = 1.0 - gain * k; }
    return {b0 / (k + 1.0), b1 / (k + 1.0), (1.0 - k) / (k + 1.0)};
}

namespace {

void build(const double* p, double fs, DecayFilter& out) {
    out.gain = std::pow(10.0, p[0] / 20.0);
    for (int k = 0; k < kShelves; ++k)
        out.sh[k] = makeShelf(kShelfDef[k].low, std::pow(10.0, p[1 + k] / 20.0), kShelfDef[k].fc, fs);
}

double evalDb(const double* p, double f, double fs) {
    DecayFilter d;
    build(p, fs, d);
    return decayFilterDb(d, f, fs);
}

// Solve A x = b (n x n, n <= 8) by Gaussian elimination with partial pivoting.
bool solve(double A[8][8], double b[8], int n) {
    for (int c = 0; c < n; ++c) {
        int piv = c;
        for (int r = c + 1; r < n; ++r) if (std::fabs(A[r][c]) > std::fabs(A[piv][c])) piv = r;
        if (std::fabs(A[piv][c]) < 1e-14) return false;
        if (piv != c) { for (int k = 0; k < n; ++k) std::swap(A[c][k], A[piv][k]); std::swap(b[c], b[piv]); }
        for (int r = c + 1; r < n; ++r) {
            double m = A[r][c] / A[c][c];
            for (int k = c; k < n; ++k) A[r][k] -= m * A[c][k];
            b[r] -= m * b[c];
        }
    }
    for (int r = n - 1; r >= 0; --r) {
        for (int k = r + 1; k < n; ++k) b[r] -= A[r][k] * b[k];
        b[r] /= A[r][r];
    }
    return true;
}
}  // namespace

double t60Target(double f, const double t60[3]) {
    const double rate[3] = {60.0 / t60[0], 60.0 / t60[1], 60.0 / t60[2]};
    double x = std::log2(f), x0 = std::log2(125.0), x1 = std::log2(1000.0), x2 = std::log2(8000.0);
    double r;
    if (x <= x0) r = rate[0];
    else if (x >= x2) r = rate[2];
    else if (x <= x1) r = rate[0] + (rate[1] - rate[0]) * (x - x0) / (x1 - x0);
    else r = rate[1] + (rate[2] - rate[1]) * (x - x1) / (x2 - x1);
    return 60.0 / r;
}

double decayFilterDb(const DecayFilter& d, double f, double fs) {
    std::complex<double> z1 = std::polar(1.0, -2.0 * kPi * f / fs), h = 1.0;
    for (const Shelf& s : d.sh) h *= (s.b0 + s.b1 * z1) / (1.0 + s.a1 * z1);
    return 20.0 * std::log10(d.gain * std::abs(h));
}

DecayFilter designDecayFilter(double delay, const double t60[3], double fs) {
    return designDecayFilter(delay, [t60](double f) { return t60Target(f, t60); }, fs);
}

DecayFilter designDecayFilter(double delay, const T60Curve& t60, double fs) {
    constexpr int n = 1 + kShelves;
    double freq[kGrid], want[kGrid];
    for (int i = 0; i < kGrid; ++i) {
        freq[i] = 125.0 * std::pow(2.0, 0.5 * i);
        want[i] = -60.0 * delay / (fs * t60(freq[i]));
    }
    // Errors are weighted by the target's own size, so the fit is accurate in decay *time* at every
    // frequency: a steep curve (air absorption) must not trade a long-ringing mid band for its top end.
    double wt[kGrid];
    double wMax = 0; for (int i = 0; i < kGrid; ++i) wMax = std::max(wMax, std::fabs(want[i]));
    for (int i = 0; i < kGrid; ++i) wt[i] = 1.0 / std::max(std::fabs(want[i]), 1e-3 * wMax + 1e-9);
    double p[n] = {want[kGrid / 2], 0, 0, 0, 0};
    auto cost = [&](const double* q) {
        double c = 0;
        for (int i = 0; i < kGrid; ++i) { double e = (evalDb(q, freq[i], fs) - want[i]) * wt[i]; c += e * e; }
        return c;
    };
    double lambda = 1e-3, cur = cost(p);
    for (int it = 0; it < 60; ++it) {
        double J[kGrid][n], r[kGrid];
        for (int i = 0; i < kGrid; ++i) r[i] = (evalDb(p, freq[i], fs) - want[i]) * wt[i];
        for (int j = 0; j < n; ++j) {
            double q[n]; for (int k = 0; k < n; ++k) q[k] = p[k];
            const double h = 1e-4; q[j] += h;
            for (int i = 0; i < kGrid; ++i) J[i][j] = ((evalDb(q, freq[i], fs) - want[i]) * wt[i] - r[i]) / h;
        }
        double A[8][8] = {}, g[8] = {};
        for (int a = 0; a < n; ++a) {
            for (int b = 0; b < n; ++b) for (int i = 0; i < kGrid; ++i) A[a][b] += J[i][a] * J[i][b];
            for (int i = 0; i < kGrid; ++i) g[a] -= J[i][a] * r[i];
            A[a][a] += lambda * (A[a][a] + 1e-9);
        }
        if (!solve(A, g, n)) { lambda *= 10; continue; }
        double q[n]; for (int k = 0; k < n; ++k) q[k] = p[k] + g[k];
        double c = cost(q);
        if (c < cur) { for (int k = 0; k < n; ++k) p[k] = q[k]; cur = c; lambda *= 0.3; }
        else lambda *= 10;
        if (lambda > 1e8 || cur < 1e-12) break;
    }
    DecayFilter out;
    build(p, fs, out);
    return out;
}

}  // namespace cs
