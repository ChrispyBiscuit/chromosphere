#include "tables.h"
#include "pffft.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>

namespace infra {
namespace {
constexpr int N = WavetableBank::kSize, H = N / 2;     // bins 0..H (H = Nyquist)
constexpr double kPi = 3.14159265358979323846;

using Spectrum = std::vector<std::complex<double>>;    // X[k], k = 0..H; x[n] = sum Re(X[k] e^{i 2 pi k n / N}) * 2 / N

// Small deterministic generator for seeded spectra and phases.
struct Lcg {
    uint32_t s;
    explicit Lcg(uint32_t seed) : s(seed * 2654435761u + 12345u) {}
    double next() { s = s * 1664525u + 1013904223u; return (s >> 8) * (1.0 / 16777216.0); }
};

// Harmonic k with amplitude a and phase given as cos/sin weights: a * (c cos + s sin).
void addHarmonic(Spectrum& X, int k, double cosAmp, double sinAmp) {
    if (k <= 0 || k >= H) return;
    X[(size_t)k] += std::complex<double>(cosAmp, -sinAmp) * (N / 2.0);
}

// Forward FFT of one cycle given in the time domain.
Spectrum fromTime(const std::vector<float>& x, PFFFT_Setup* setup, float* work, float* buf) {
    for (int i = 0; i < N; ++i) buf[i] = x[(size_t)i];
    pffft_transform_ordered(setup, buf, buf, work, PFFFT_FORWARD);
    Spectrum X((size_t)H + 1);
    X[0] = {buf[0], 0.0}; X[(size_t)H] = {buf[1], 0.0};
    for (int k = 1; k < H; ++k) X[(size_t)k] = {buf[2 * k], buf[2 * k + 1]};
    return X;
}

// ---- Vowel formants (Hz) used by Nebula and the Formant filter ----------------------------------------
constexpr double kVowels[5][3] = {{800, 1150, 2900}, {550, 1770, 2490}, {280, 2250, 2890}, {450, 800, 2830}, {325, 700, 2530}};

// ---- The ten tables ----------------------------------------------------------------------------------
Spectrum classic(double t) {
    // sine -> triangle -> saw -> square -> narrow pulse, as spectral blends between neighbours.
    auto shape = [](int which, Spectrum& X) {
        for (int k = 1; k < H; ++k) {
            switch (which) {
                case 0: if (k == 1) addHarmonic(X, k, 0, 1); break;
                case 1: if (k & 1) addHarmonic(X, k, 0, 8.0 / (kPi * kPi) * (((k - 1) / 2) % 2 ? -1.0 : 1.0) / (double)(k * k)); break;
                case 2: addHarmonic(X, k, 0, 2.0 / (kPi * k)); break;
                default: break;
            }
        }
    };
    auto pulse = [](double d, Spectrum& X) {   // pulse of duty d, centred on phase d/2
        for (int k = 1; k < H; ++k) {
            const double a = 2.0 / (kPi * k) * std::sin(kPi * k * d) * 2.0, ph = kPi * k * d;
            addHarmonic(X, k, a * std::cos(ph), a * std::sin(ph));
        }
    };
    Spectrum a((size_t)H + 1), b((size_t)H + 1);
    const double seg = t * 4.0;
    const int i = std::min(3, (int)seg);
    const double fr = seg - i;
    if (i < 2) { shape(i, a); shape(i + 1, b); }
    else if (i == 2) { shape(2, a); pulse(0.5, b); }
    else { pulse(0.5, a); pulse(0.5 - 0.42 * fr, b); return b; }
    for (size_t k = 0; k < a.size(); ++k) a[k] = a[k] * (1.0 - fr) + b[k] * fr;
    return a;
}

Spectrum fraunhofer(double t) {
    Spectrum X((size_t)H + 1);
    const int lines = 6;
    for (int k = 1; k < H; ++k) {
        const double lk = std::log2((double)k);
        double amp = 2.0 / (kPi * k) * std::pow((double)k, -0.15);
        for (int j = 0; j < lines; ++j) {
            const double pos = 1.2 + j * 1.05 + t * (1.4 + 0.25 * j);        // octaves above the fundamental
            const double w = 0.05 + 0.015 * j;
            amp *= 1.0 - 0.96 * std::exp(-((lk - pos) * (lk - pos)) / (w * w));
        }
        // A faint emission bump between the lines keeps the frame lively.
        const double em = 2.5 + 3.0 * t;
        amp *= 1.0 + 0.6 * std::exp(-((lk - em) * (lk - em)) / 0.3);
        addHarmonic(X, k, 0, amp);
    }
    return X;
}

std::vector<float> timeBuffer() { return std::vector<float>((size_t)N, 0.f); }

std::vector<float> pulsar(double t) {
    auto x = timeBuffer();
    const double duty = 1.0 - 0.94 * std::pow(t, 0.8);            // burst length as a fraction of the period
    const double cycles = 2.0;                                      // pulsaret: two cycles of a sine under a window
    for (int n = 0; n < N; ++n) {
        const double p = (double)n / N;
        if (p >= duty) continue;
        const double u = p / duty;
        const double win = 0.5 - 0.5 * std::cos(2.0 * kPi * u);
        x[(size_t)n] = (float)(std::sin(2.0 * kPi * cycles * u) * win);
    }
    return x;
}

Spectrum nebula(double t) {
    Spectrum X((size_t)H + 1);
    const double f0 = 110.0;                                        // formants are placed for this fundamental
    const double v = t * 4.0;
    const int i = std::min(3, (int)v);
    const double fr = v - i;
    double F[3];
    for (int j = 0; j < 3; ++j) F[j] = std::exp(std::log(kVowels[i][j]) * (1 - fr) + std::log(kVowels[i + 1][j]) * fr);
    const double gains[3] = {1.0, 0.6, 0.3}, bw[3] = {90.0, 120.0, 180.0};
    Lcg rng(77);
    for (int k = 1; k < H; ++k) {
        const double f = f0 * k;
        double env = 0.02;
        for (int j = 0; j < 3; ++j) env += gains[j] * std::exp(-((f - F[j]) * (f - F[j])) / (bw[j] * bw[j] * 2.0));
        const double amp = env / std::pow((double)k, 0.6);
        const double ph = rng.next() * 2.0 * kPi;
        addHarmonic(X, k, amp * std::cos(ph), amp * std::sin(ph));
    }
    return X;
}

Spectrum ionDrive(int f) {
    // Seven drawbars at 8', 4', 2 2/3', 2', 1 3/5', 1 1/3', 1' -> harmonics 1, 2, 3, 4, 5, 6, 8.
    static const char* regs[16] = {"8000000", "8600000", "8800000", "8840000", "8880000", "8808000", "8668400", "8800008",
                                   "8880008", "6888400", "8888600", "8868866", "8888808", "8888888", "6688888", "4688888"};
    static const int harm[7] = {1, 2, 3, 4, 5, 6, 8};
    Spectrum X((size_t)H + 1);
    for (int j = 0; j < 7; ++j) {
        const double a = (regs[f][j] - '0') / 8.0;
        // Each drawbar step is about 3 dB; use that law rather than linear.
        const double amp = a > 0 ? std::pow(10.0, (a * 8.0 - 8.0) * 3.0 / 20.0) : 0.0;
        addHarmonic(X, harm[j], 0, amp);
    }
    return X;
}

std::vector<float> magnetar(int f) {
    auto x = timeBuffer();
    const double ratio = f < 8 ? 2.0 : 3.0;
    const double index = f < 8 ? 6.0 * f / 7.0 : 1.0 + 7.0 * (f - 8) / 7.0;
    for (int n = 0; n < N; ++n) {
        const double p = (double)n / N;
        x[(size_t)n] = (float)std::sin(2.0 * kPi * p + index * std::sin(2.0 * kPi * ratio * p));
    }
    return x;
}

std::vector<float> plasma(double t) {
    auto x = timeBuffer();
    const double ratio = 1.0 + 7.0 * t;
    for (int n = 0; n < N; ++n) {
        const double p = (double)n / N, sp = ratio * p;
        const double saw = 2.0 * (sp - std::floor(sp)) - 1.0;
        // A gentle fade at the cycle end softens the reset edge so the sweep is not only harsh.
        const double edge = std::min(1.0, (1.0 - p) * 40.0);
        x[(size_t)n] = (float)(saw * edge);
    }
    return x;
}

std::vector<float> eventHorizon(double t) {
    auto x = timeBuffer();
    const double gain = 1.0 + 7.0 * t, bias = 0.35 * t;
    auto fold = [](double v) { const double q = (v - 1.0) / 4.0; return 4.0 * std::abs(q - std::floor(q) - 0.5) - 1.0; };
    for (int n = 0; n < N; ++n) x[(size_t)n] = (float)fold(gain * std::sin(2.0 * kPi * n / N) + bias);
    return x;
}

Spectrum darkMatter(int f) {
    Spectrum X((size_t)H + 1);
    Lcg rng(1000u + (uint32_t)f * 7919u);
    const double tilt = 0.85 + 0.35 * rng.next();
    const int maxK = 48 + (int)(rng.next() * 160);
    for (int k = 1; k < std::min(H, maxK); ++k) {
        double r = rng.next();
        r = r * r * r;
        if (rng.next() < 0.25) r *= 0.05;                           // gaps
        const double amp = (k == 1 ? 1.0 : r * 1.5) / std::pow((double)k, tilt) * (1.0 - (double)k / maxK);
        const double ph = rng.next() * 2.0 * kPi;
        addHarmonic(X, k, amp * std::cos(ph), amp * std::sin(ph));
    }
    return X;
}

Spectrum quasar(double t) {
    Spectrum X((size_t)H + 1);
    static const int set[] = {1, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
    const double centre = 1.0 + 5.0 * t;                            // octave of the moving emphasis
    for (int k : set) {
        if (k >= H) break;
        const double lk = std::log2((double)k);
        const double base = std::exp(-(double)k / (6.0 + 50.0 * t));
        const double emph = std::exp(-((lk - centre) * (lk - centre)) / 0.45);
        const double amp = (k == 1 ? 1.0 : k == 2 ? 0.18 : base * 0.6 + emph * 0.7) ;
        addHarmonic(X, k, 0, amp);
    }
    return X;
}
}  // namespace

WavetableBank::WavetableBank() {
    data_.assign((size_t)kNumTables * kFrames * kLevels * kStride, 0.f);
    PFFFT_Setup* setup = pffft_new_setup(N, PFFFT_REAL);
    float* work = (float*)pffft_aligned_malloc(sizeof(float) * N);
    float* buf = (float*)pffft_aligned_malloc(sizeof(float) * N);
    for (int t = 0; t < kNumTables; ++t) {
        for (int f = 0; f < kFrames; ++f) {
            const double u = (double)f / (kFrames - 1);
            Spectrum X;
            switch (t) {
                case 0: X = classic(u); break;
                case 1: X = fraunhofer(u); break;
                case 2: X = fromTime(pulsar(u), setup, work, buf); break;
                case 3: X = nebula(u); break;
                case 4: X = ionDrive(f); break;
                case 5: X = fromTime(magnetar(f), setup, work, buf); break;
                case 6: X = fromTime(plasma(u), setup, work, buf); break;
                case 7: X = fromTime(eventHorizon(u), setup, work, buf); break;
                case 8: X = darkMatter(f); break;
                default: X = quasar(u); break;
            }
            float gain = 0.f;
            for (int L = 0; L < kLevels; ++L) {
                const int maxH = L == 0 ? H - 1 : (H >> L);
                buf[0] = 0.f; buf[1] = 0.f;                         // no DC, no Nyquist
                for (int k = 1; k < H; ++k) {
                    const bool keep = k <= maxH;
                    buf[2 * k] = keep ? (float)X[(size_t)k].real() : 0.f;
                    buf[2 * k + 1] = keep ? (float)X[(size_t)k].imag() : 0.f;
                }
                pffft_transform_ordered(setup, buf, buf, work, PFFFT_BACKWARD);
                if (L == 0) {                                       // one gain per frame, from the full-band level
                    float peak = 1e-9f;
                    for (int i = 0; i < N; ++i) peak = std::max(peak, std::abs(buf[i]));
                    gain = 1.f / peak;
                }
                float* dst = data_.data() + ((size_t)((t * kFrames + f) * kLevels + L)) * kStride;
                for (int i = 0; i < N; ++i) dst[i] = buf[i] * gain;
                dst[N] = dst[0];
            }
        }
    }
    pffft_aligned_free(work); pffft_aligned_free(buf);
    pffft_destroy_setup(setup);
}

const WavetableBank& WavetableBank::get() {
    static const WavetableBank bank;
    return bank;
}

void WavetableBank::levelFor(float inc, int& L0, float& frac) {
    const float l = std::log2(std::max(inc, 1e-9f) * 4096.f);
    if (l <= 0.f) { L0 = 0; frac = 0.f; return; }
    L0 = (int)l; frac = l - (float)L0;
    // L0 alone is alias free across the whole octave (its top harmonic stays below Nyquist); fading to L0 + 1
    // over the upper half only keeps the top octave bright longer and halves the time spent reading two levels.
    frac = std::max(0.f, frac * 2.f - 1.f);
    if (frac > 0.995f) { ++L0; frac = 0.f; }
    if (L0 >= kLevels - 1) { L0 = kLevels - 1; frac = 0.f; }
}

}  // namespace infra
