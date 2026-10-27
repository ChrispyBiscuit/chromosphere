// DSP tests for the MUON engine (no JUCE): the objective gates of PRD Section 8, plus the building blocks.
// Each check prints PASS / FAIL; the exit code is the failure count.
#include "muon/engine.h"
#include "muon/crossover.h"
#include "pffft.h"
#include <cstdio>
#include <functional>
#include <random>
#include <vector>

using namespace muon;

static int failures = 0;
#define CHECK(cond, ...) do { if (cond) std::printf("PASS  "); else { std::printf("FAIL  "); ++failures; } std::printf(__VA_ARGS__); std::printf("\n"); } while (0)

namespace {
constexpr double kFs = 48000.0;
using Buf = std::vector<float>;

double db(double x) { return 20.0 * std::log10(std::max(x, 1e-30)); }

// Magnitude spectrum with a Blackman-Harris window, scaled so a full-scale sine's peak bin reads 0 dB.
std::vector<double> spectrum(const float* x, int N) {
    PFFFT_Setup* s = pffft_new_setup(N, PFFFT_REAL);
    float* in = (float*)pffft_aligned_malloc(sizeof(float) * (size_t)N);
    float* out = (float*)pffft_aligned_malloc(sizeof(float) * (size_t)N);
    double wsum = 0.0;
    for (int i = 0; i < N; ++i) {
        const double t = (double)i / (N - 1);
        const double w = 0.35875 - 0.48829 * std::cos(2 * kPi * t) + 0.14128 * std::cos(4 * kPi * t) - 0.01168 * std::cos(6 * kPi * t);
        in[i] = (float)(x[i] * w); wsum += w;
    }
    pffft_transform_ordered(s, in, out, nullptr, PFFFT_FORWARD);
    std::vector<double> m((size_t)N / 2);
    m[0] = std::fabs(out[0]) / wsum;
    for (int k = 1; k < N / 2; ++k) m[(size_t)k] = 2.0 * std::hypot(out[2 * k], out[2 * k + 1]) / wsum;
    pffft_aligned_free(in); pffft_aligned_free(out); pffft_destroy_setup(s);
    return m;
}
// Peak level (dB) within +-w bins of frequency f.
double levelAt(const std::vector<double>& m, double fs, int N, double f, int w = 4) {
    const int k = (int)std::lround(f * N / fs);
    double p = 0.0;
    for (int j = std::max(1, k - w); j <= std::min((int)m.size() - 1, k + w); ++j) p = std::max(p, m[(size_t)j]);
    return db(p);
}

// The nearest frequency that falls on an FFT bin centre (so peak levels read exactly).
double onBin(double f, int N = 32768, double fs = kFs) { return std::round(f * N / fs) * fs / N; }
Buf sine(double f, double amp, int n, double fs = kFs) { Buf b((size_t)n); for (int i = 0; i < n; ++i) b[(size_t)i] = (float)(amp * std::sin(2 * kPi * f * i / fs)); return b; }

// Neutral parameters: everything at its transparent setting, auto-gain off.
Params neutral(int bands = 1) {
    Params p;
    p.bands = bands; p.autoGain = false; p.oversample = 2;
    return p;
}

// Renders a stereo buffer (same signal on both channels unless R is given) through a fresh engine.
struct Out { Buf L, R; int latency = 0; };
Out render(const Params& p, const Buf& in, int block = 256, double fs = kFs, const Buf* inR = nullptr, Engine* keep = nullptr) {
    Engine local; Engine& e = keep ? *keep : local;
    e.setParams(p);
    e.prepare(fs, block);
    Out o; o.L = in; o.R = inR ? *inR : in; o.latency = e.latency();
    for (size_t off = 0; off < in.size(); off += (size_t)block) {
        const int n = (int)std::min((size_t)block, in.size() - off);
        e.setParams(p);
        e.process(o.L.data() + off, o.R.data() + off, n);
    }
    return o;
}
double rms(const Buf& b, size_t a, size_t e) { double s = 0; e = std::min(e, b.size()); for (size_t i = a; i < e; ++i) s += (double)b[i] * b[i]; return std::sqrt(s / std::max<size_t>(1, e - a)); }
double peak(const Buf& b, size_t a = 0, size_t e = SIZE_MAX) { double p = 0; e = std::min(e, b.size()); for (size_t i = a; i < e; ++i) p = std::max(p, (double)std::fabs(b[i])); return p; }
bool finite(const Buf& b) { for (float v : b) if (!std::isfinite(v)) return false; return true; }

// A drum-like test loop: decaying noise-and-tone hits on a sustained low tone bed.
Buf drumLoop(double amp, int n) {
    Buf b((size_t)n, 0.f);
    std::mt19937 rng(7); std::normal_distribution<float> g(0.f, 1.f);
    const int hit = (int)(0.25 * kFs);
    for (int i = 0; i < n; ++i) {
        const int t = i % hit; const double ts = t / kFs;
        const double k = std::exp(-ts / 0.004) * g(rng) * 0.5 + std::exp(-ts / 0.12) * std::sin(2 * kPi * (60 + 80 * std::exp(-ts / 0.03)) * ts);
        b[(size_t)i] = (float)(amp * 0.7 * (k + 0.15 * std::sin(2 * kPi * 220 * i / kFs)));
    }
    return b;
}
double crest(const Buf& b, size_t a, size_t e) { return db(peak(b, a, e) / std::max(1e-12, rms(b, a, e))); }

// Frequency by interpolated positive zero crossings over [a, e).
double zeroCrossHz(const Buf& b, size_t a, size_t e, double fs = kFs) {
    double first = -1, last = -1; int count = 0;
    for (size_t i = a + 1; i < e; ++i) {
        if (b[i - 1] < 0.f && b[i] >= 0.f) {
            const double t = (double)(i - 1) + b[i - 1] / (double)(b[i - 1] - b[i]);
            if (first < 0) first = t; else ++count;
            last = t;
        }
    }
    return count > 0 ? fs * count / (last - first) : 0.0;
}
}  // namespace

// ---- Building blocks -----------------------------------------------------------------------------------------
static void testResampler() {
    for (int lin = 0; lin < 2; ++lin) {
        for (int f : {2, 4, 8}) {
            Resampler r; r.prepare(f, lin == 1, 4096);
            const int n = 4096;
            // Image rejection: a 15 kHz sine upsampled; everything above 24 kHz must be gone.
            Buf x = sine(15000.0, 1.0, n), up((size_t)n * f);
            r.up(x.data(), up.data(), n);
            const int N = 8192 * f / 2;
            auto m = spectrum(up.data() + (size_t)(n * f - N), N);
            double worst = -300;
            for (size_t k = 0; k < m.size(); ++k) { const double hz = k * kFs * f / N; if (hz > 24500.0) worst = std::max(worst, db(m[k])); }
            // Round trip: gain at 1, 10 and 19 kHz, and the latency (impulse).
            double dev = 0.0;
            for (double hz : {1000.0, 10000.0, 19000.0}) {
                Resampler rr; rr.prepare(f, lin == 1, 8192);
                Buf s = sine(hz, 0.5, 8192), u((size_t)8192 * f), d(8192);
                rr.up(s.data(), u.data(), 8192); rr.down(u.data(), d.data(), 8192);
                dev = std::max(dev, std::fabs(db(rms(d, 4096, 8192) / rms(s, 4096, 8192))));
            }
            Resampler ri; ri.prepare(f, lin == 1, 1024);
            Buf imp(1024, 0.f), u((size_t)1024 * f), d(1024); imp[10] = 1.f;
            ri.up(imp.data(), u.data(), 1024); ri.down(u.data(), d.data(), 1024);
            size_t pk = 0; for (size_t i = 0; i < d.size(); ++i) if (std::fabs(d[i]) > std::fabs(d[pk])) pk = i;
            const int lat = (int)pk - 10;
            CHECK(worst < -95.0 && dev < 0.05 && (lin == 0 || lat == r.roundTripLatency()),
                  "%s %dx: images %.1f dB, passband deviation %.4f dB to 19 kHz, round trip %d samples (reported %d)",
                  lin ? "FIR" : "IIR", f, worst, dev, lat, r.roundTripLatency());
        }
    }
}

static void testHilbert() {
    double worst = 0.0;
    for (double f : {25.0, 100.0, 1000.0, 10000.0, 19000.0}) {
        Hilbert h; h.prepare(kFs);
        const int n = 48000;
        double lo = 1e9, hi = 0;
        for (int i = 0; i < n; ++i) {
            float a, b; h.process((float)std::sin(2 * kPi * f * i / kFs), a, b);
            if (i > n / 2) { const double m = std::sqrt((double)a * a + (double)b * b); lo = std::min(lo, m); hi = std::max(hi, m); }
        }
        worst = std::max(worst, std::max(std::fabs(db(lo)), std::fabs(db(hi))));
    }
    CHECK(worst < 0.01, "Hilbert pair: analytic amplitude of a sine flat within %.4f dB from 25 Hz to 19 kHz", worst);
}

static void testLinearKernel() {
    const auto h = LinearCrossover::designLowpass(kFs, 1000.0, 1025);
    double sum = 0, sym = 0;
    for (size_t i = 0; i < h.size(); ++i) { sum += h[i]; sym = std::max(sym, (double)std::fabs(h[i] - h[h.size() - 1 - i])); }
    // Response at fc should be -6 dB (LR4: |H| = 1/2 at the crossover).
    double re = 0, im = 0; const double w = 2 * kPi * 1000.0 / kFs;
    for (size_t i = 0; i < h.size(); ++i) { re += h[i] * std::cos(w * i); im -= h[i] * std::sin(w * i); }
    const double atFc = db(std::hypot(re, im));
    CHECK(std::fabs(sum - 1.0) < 1e-5 && sym < 1e-7 && std::fabs(atFc + 6.02) < 0.2,
          "linear-phase kernel: DC %.6f, symmetric (%.1e), %.2f dB at the crossover", sum, sym, atFc);
}

// ---- Gates: transparency and latency --------------------------------------------------------------------------
static void testTransparency() {
    std::mt19937 rng(1); std::uniform_real_distribution<float> u(-0.5f, 0.5f);
    Buf noise(48000); for (auto& v : noise) v = u(rng);
    {
        auto o = render(neutral(1), noise, 100);
        bool exact = true; for (size_t i = 0; i < noise.size(); ++i) if (o.L[i] != noise[i] || o.R[i] != noise[i]) { exact = false; break; }
        CHECK(exact, "neutral, one band: output bit-identical to the input");
    }
    {
        // Three bands, minimum phase: impulse response magnitude flat.
        Buf imp(16384, 0.f); imp[100] = 1.f;
        auto o = render(neutral(3), imp, 256);
        auto m = spectrum(o.L.data(), 16384);
        // spectrum() normalises for sines; an impulse's flat magnitude reads the same in every bin. Compare bins.
        double ref = 0; int cnt = 0, lo = (int)(20.0 * 16384 / kFs), hi = (int)(20000.0 * 16384 / kFs);
        for (int k = lo; k <= hi; ++k) { ref += m[(size_t)k]; ++cnt; }
        ref /= cnt;
        double dev = 0; for (int k = lo; k <= hi; ++k) dev = std::max(dev, std::fabs(db(m[(size_t)k] / ref)));
        // Windowing an impulse response that is not centred makes the estimate itself ripple; use a rectangular
        // DFT on the raw response instead.
        dev = 0;
        for (double f = 20; f <= 20000; f *= 1.05) {
            double re = 0, im = 0; const double w = 2 * kPi * f / kFs;
            for (size_t i = 0; i < o.L.size(); ++i) { re += o.L[i] * std::cos(w * i); im -= o.L[i] * std::sin(w * i); }
            dev = std::max(dev, std::fabs(db(std::hypot(re, im))));
        }
        CHECK(dev < 0.01, "neutral, three bands, minimum phase: magnitude flat within %.4f dB, 20 Hz - 20 kHz", dev);
    }
    {
        Params p = neutral(3); p.crossover = CrossoverType::LinearPhase;
        auto o = render(p, noise, 256);
        const int lat = o.latency;
        double err = 0, ref = 0;
        for (size_t i = (size_t)lat + 4800; i < noise.size(); ++i) { const double d = o.L[i] - noise[i - (size_t)lat]; err += d * d; ref += (double)noise[i - (size_t)lat] * noise[i - (size_t)lat]; }
        CHECK(10 * std::log10(err / ref) < -100.0, "neutral, three bands, linear phase: null against the input delayed by %d samples at %.1f dB", lat, 10 * std::log10(err / ref));
    }
    // Reported latency equals measured latency (impulse) in every latency-bearing mode.
    struct Mode { const char* name; Params p; };
    std::vector<Mode> modes;
    for (int la = 0; la < 4; ++la) { Params p = neutral(3); p.lookahead = la; modes.push_back({"lookahead", p}); }
    { Params p = neutral(3); p.crossover = CrossoverType::LinearPhase; p.lookahead = 2; modes.push_back({"linear + lookahead", p}); }
    { Params p = neutral(2); p.crossover = CrossoverType::LinearPhase; p.band[0].drive = 0.01f; p.oversample = 3; modes.push_back({"linear + 8x drive", p}); }
    bool ok = true; std::string bad;
    for (auto& m : modes) {
        Buf imp(8192, 0.f); imp[200] = 1.f;
        auto o = render(m.p, imp, 128);
        size_t pk = 0; for (size_t i = 0; i < o.L.size(); ++i) if (std::fabs(o.L[i]) > std::fabs(o.L[pk])) pk = i;
        if ((int)pk - 200 != o.latency) { ok = false; bad += std::string(m.name) + " (" + std::to_string((int)pk - 200) + " vs " + std::to_string(o.latency) + ") "; }
    }
    CHECK(ok, "reported latency equals the measured impulse delay in %zu modes %s", modes.size(), bad.c_str());
}

// ---- Gates: the shaper -----------------------------------------------------------------------------------------
static void testShaper() {
    // Level independence: crest factor change for Attack +50% at -6 and -36 dBFS.
    const int n = (int)(2.0 * kFs);
    double change[2];
    for (int k = 0; k < 2; ++k) {
        const double amp = k == 0 ? 0.5 : 0.0158;
        Buf x = drumLoop(amp, n);
        Params p = neutral(1); p.band[0].attack = 0.5f;
        auto o = render(p, x);
        change[k] = crest(o.L, n / 4, n) - crest(x, n / 4, n);
    }
    CHECK(std::fabs(change[0] - change[1]) < 0.5, "level independence: crest factor change %.2f dB at -6 dBFS, %.2f dB at -36 dBFS", change[0], change[1]);

    // Decaying tone burst: 1 kHz, starting at 0.1 s, decaying with a 200 ms time constant.
    Buf burst((size_t)kFs, 0.f);
    const int on = 4800;
    for (int i = on; i < (int)kFs; ++i) burst[(size_t)i] = (float)(0.5 * std::exp(-(i - on) / (0.2 * kFs)) * std::sin(2 * kPi * 1000.0 * (i - on) / kFs));
    auto seg = [&](const Buf& b, double t0, double t1) { return rms(b, (size_t)(on + t0 * kFs), (size_t)(on + t1 * kFs)); };
    {
        Params p = neutral(1); p.band[0].attack = 1.f;
        auto o = render(p, burst);
        const double first = db(seg(o.L, 0, 0.005) / seg(burst, 0, 0.005)), later = db(seg(o.L, 0.1, 0.12) / seg(burst, 0.1, 0.12));
        CHECK(first >= 9.0 && std::fabs(later) < 0.5, "Attack +100%%: first 5 ms %+.1f dB, at 100 ms %+.2f dB", first, later);
    }
    {
        Params p = neutral(1); p.band[0].sustain = 1.f;
        auto o = render(p, burst);
        const double at200 = db(seg(o.L, 0.19, 0.21) / seg(burst, 0.19, 0.21));
        CHECK(at200 >= 9.0, "Sustain +100%%: level at 200 ms %+.1f dB", at200);
    }
    {
        Params p = neutral(1); p.band[0].attack = -1.f;
        auto o = render(p, burst);
        const double first = db(seg(o.L, 0, 0.005) / seg(burst, 0, 0.005));
        CHECK(first <= -9.0, "Attack -100%%: first 5 ms %+.1f dB", first);
    }
    // No clicks: at extreme settings, the largest sample step is no larger than the input's largest step times
    // the largest gain applied (+15 dB attack, +24 dB sustain) with 12% headroom.
    {
        Buf x = drumLoop(0.3, n);
        double inStep = 0; for (size_t i = 1; i < x.size(); ++i) inStep = std::max(inStep, (double)std::fabs(x[i] - x[i - 1]));
        bool ok = true; double worst = 0;
        for (float a : {-1.f, 1.f}) for (float s : {-1.f, 1.f}) for (int bands : {1, 3}) {
            Params p = neutral(bands); for (auto& b : p.band) { b.attack = a; b.sustain = s; b.speed = 0.f; }
            auto o = render(p, x);
            double st = 0; for (size_t i = 1; i < o.L.size(); ++i) st = std::max(st, (double)std::fabs(o.L[i] - o.L[i - 1]));
            const double maxGain = std::pow(10.0, ((a > 0 ? 15.0 : 0.0) + (s > 0 ? 24.0 : 0.0)) / 20.0);
            worst = std::max(worst, st / (inStep * maxGain));
            if (st > inStep * maxGain * 1.12) ok = false;
            if (!finite(o.L)) ok = false;
        }
        CHECK(ok, "click test: largest output step %.2f x (input step x largest gain)", worst);
    }
}

// ---- Gates: harmonics -----------------------------------------------------------------------------------------
static void testOrderBank() {
    const int n = 65536, N = 32768;
    bool ok = true; std::string detail;
    for (int order = 2; order <= kOrders; ++order) {
        Params p = neutral(1); p.oversample = 2;
        p.band[0].harmonics = 1.f; p.band[0].orders[(size_t)order - 1] = 1.f;
        const double f0 = onBin(200.0, N);
        Buf x = sine(f0, 0.25, n);
        auto o = render(p, x);
        auto m = spectrum(o.L.data() + (n - N), N);
        const double target = levelAt(m, kFs, N, f0 * order);
        double other = -300;
        for (int h = 2; h <= 20; ++h) if (h != order) other = std::max(other, levelAt(m, kFs, N, f0 * h));
        const double fund = levelAt(m, kFs, N, f0);
        char buf[96]; std::snprintf(buf, sizeof buf, " H%d %.1f/%.1f", order, target, other - target);
        detail += buf;
        if (std::fabs(target - db(0.25)) > 1.0 || other - target > -40.0 || std::fabs(fund - db(0.25)) > 0.5) ok = false;
    }
    CHECK(ok, "order bank, 200 Hz at -12 dBFS (target dB / worst other relative):%s", detail.c_str());
    // H1 at zero removes the source (the dry fundamental) at full amount.
    {
        Params p = neutral(1); p.band[0].harmonics = 1.f; p.band[0].orders[0] = 0.f; p.band[0].orders[2] = 1.f;
        const double f0 = onBin(200.0, N);
        auto o = render(p, sine(f0, 0.25, n));
        auto m = spectrum(o.L.data() + (n - N), N);
        CHECK(levelAt(m, kFs, N, f0) < -40.0 && std::fabs(levelAt(m, kFs, N, 3 * f0) - db(0.25)) < 1.0,
              "H1 at 0: fundamental at %.1f dBFS, H3 at %.1f dBFS", levelAt(m, kFs, N, f0), levelAt(m, kFs, N, 3 * f0));
    }
    // Order limits per band: the bank stops at the highest clean order.
    {
        Engine e; Params p = neutral(1); p.oversample = 0; e.setParams(p); e.prepare(kFs, 256);
        const int o1 = e.maxOrder(0);
        p.oversample = 2; e.setParams(p);
        const int o4 = e.maxOrder(0);
        CHECK(o1 == 1 && o4 == 8, "highest clean order for a full band: %d at 1x, %d at 4x", o1, o4);
    }
}

// Worst non-harmonic component below 20 kHz (the audible band; the half-band transition folds 24-26 kHz into
// 22-24 kHz), relative to the fundamental, for a sine at f through the given parameters.
static double aliasLevel(const Params& p, double fIn, double amp) {
    const int n = 65536, N = 32768;
    const double f = onBin(fIn, N);
    auto o = render(p, sine(f, amp, n));
    auto m = spectrum(o.L.data() + (n - N), N);
    const double fund = levelAt(m, kFs, N, f, 3);
    double worst = -300;
    for (size_t k = 5; k < m.size(); ++k) {
        const double hz = k * kFs / N;
        if (hz > 20000.0) break;
        const double nearH = std::fabs(hz / f - std::round(hz / f)) * f;
        if (nearH < 6 * kFs / N) continue;      // a harmonic (main lobe)
        worst = std::max(worst, db(m[k]));
    }
    return worst - fund;
}

static void testAliasing() {
    Params p = neutral(1); p.band[0].drive = 1.f; p.band[0].curve = DriveCurve::Soft;
    p.oversample = 2;
    const double os4 = aliasLevel(p, 5000.0, 1.0);
    p.oversample = 3;
    const double os8 = aliasLevel(p, 5000.0, 1.0);
    p.oversample = 0;
    const double os1 = aliasLevel(p, 5000.0, 1.0);
    CHECK(os4 < -60.0 && os8 < -75.0 && os1 < -25.0,
          "drive Soft at full on a 5 kHz sine: aliases %.1f dB (4x), %.1f dB (8x), %.1f dB (1x, ADAA only)", os4, os8, os1);
    Params q = neutral(1); q.oversample = 2; q.band[0].harmonics = 1.f;
    for (int k = 1; k < kOrders; ++k) q.band[0].orders[(size_t)k] = 1.f;
    const double bank = aliasLevel(q, 5000.0, 0.5);
    CHECK(bank < -90.0, "order bank H2-H8 at full on a 5 kHz sine, 4x: aliases %.1f dB", bank);
    // Every curve stays finite and bounded at full drive.
    bool ok = true;
    for (int c = 0; c < 6; ++c) for (float bias : {-1.f, 0.f, 1.f}) {
        Params r = neutral(3); for (auto& b : r.band) { b.drive = 1.f; b.curve = (DriveCurve)c; b.bias = bias; b.tone = 1.f; }
        auto o = render(r, drumLoop(0.9, 24000));
        if (!finite(o.L) || peak(o.L) > 4.0) ok = false;
    }
    CHECK(ok, "all six curves, three biases, full drive: finite and bounded");
}

// ---- Gates: sub ---------------------------------------------------------------------------------------------
static void testSub() {
    // A bass line: 60, 80, 100, 120 Hz notes of 0.5 s, then silence.
    const int note = (int)(0.5 * kFs), n = note * 4 + (int)(0.2 * kFs);
    const double fr[4] = {60, 80, 100, 120};
    Buf x((size_t)n, 0.f);
    double ph = 0;
    for (int k = 0; k < 4; ++k) for (int i = 0; i < note; ++i) { ph += 2 * kPi * fr[k] / kFs; x[(size_t)(k * note + i)] = (float)(0.4 * std::sin(ph)); }
    Params p = neutral(1); p.subLevel = 1.f; p.subPurity = 1.f; p.listen = Listen::Sub;
    auto o = render(p, x);
    bool ok = true; double worstCents = 0;
    for (int k = 0; k < 4; ++k) {
        // From two input periods after the note starts, plus the filter settling (~4 periods).
        const size_t a = (size_t)(k * note + 6.0 * kFs / fr[k]), b = (size_t)((k + 1) * note - 200);
        const double hz = zeroCrossHz(o.L, a, b);
        const double cents = 1200.0 * std::log2(hz / (fr[k] / 2));
        worstCents = std::max(worstCents, std::fabs(cents));
        if (std::fabs(cents) > 1.0) ok = false;
    }
    const double tail = peak(o.L, (size_t)(4 * note + 0.05 * kFs), (size_t)n);
    const double level = db(rms(o.L, (size_t)(note / 2), (size_t)note) / rms(x, (size_t)(note / 2), (size_t)note));
    CHECK(ok && tail < 1e-6, "sub octave down: worst %.3f cents off f/2 on four notes, level %+.1f dB re input, %.1f dB 50 ms after the input stops",
          worstCents, level, db(tail));
    p.subLevel = 0.f; p.sub2Level = 1.f;
    auto o2 = render(p, x);
    const size_t a = (size_t)(note / 2), b = (size_t)note;
    const double hz2 = zeroCrossHz(o2.L, a, b);
    CHECK(std::fabs(1200.0 * std::log2(hz2 / 15.0)) < 2.0, "two octaves down on 60 Hz: %.3f Hz (15 Hz expected)", hz2);
    // Phantom: harmonics 2-4 of a 50 Hz tone appear.
    Params q = neutral(1); q.phantom = 1.f; q.listen = Listen::Sub;
    auto o3 = render(q, sine(50.0, 0.4, 65536));
    auto m = spectrum(o3.L.data() + 32768, 32768);
    CHECK(levelAt(m, kFs, 32768, 100) > -20 && levelAt(m, kFs, 32768, 150) > -25 && levelAt(m, kFs, 32768, 50) < -40,
          "phantom on 50 Hz: H2 %.1f, H3 %.1f, H4 %.1f, fundamental %.1f dBFS",
          levelAt(m, kFs, 32768, 100), levelAt(m, kFs, 32768, 150), levelAt(m, kFs, 32768, 200), levelAt(m, kFs, 32768, 50));
}

// ---- Gates: resonance ------------------------------------------------------------------------------------------
static void testResonance() {
    ResonatorBank bank; bank.prepare(kFs);
    Params p; p.resMode = ResMode::Tuned; p.chord = Chord::Unison; p.spread = 0.f; p.root = 45; p.resDecay = 1.f;
    p.resDamping = 0.f; p.strike = 0.7f; p.resStereo = 0.f;
    const int n = (int)(2.0 * kFs);
    Buf x((size_t)n, 0.f), L((size_t)n), R((size_t)n);
    x[10] = 1.f;
    for (int off = 0; off < n; off += 256) bank.process(x.data() + off, L.data() + off, R.data() + off, std::min(256, n - off), p, 0.f);
    const double hz = zeroCrossHz(L, (size_t)(0.1 * kFs), (size_t)(0.5 * kFs));
    // T60 from the decay slope between 0.1 s and 0.6 s.
    const double a0 = rms(L, (size_t)(0.1 * kFs), (size_t)(0.15 * kFs)), a1 = rms(L, (size_t)(0.55 * kFs), (size_t)(0.6 * kFs));
    const double t60 = 60.0 / (db(a0 / a1) / 0.45);
    CHECK(std::fabs(1200 * std::log2(hz / 110.0)) < 1.0 && std::fabs(t60 - 1.0) < 0.1,
          "tuned resonator at A2: %.3f Hz, T60 %.3f s (decay 1 s)", hz, t60);
    // Every mode and material: finite, decays to silence.
    bool ok = true;
    for (int mode = 0; mode < 3; ++mode) for (int mat = 0; mat < 5; ++mat) for (int ch = 0; ch < 7; ++ch) {
        if (mode != 1 && mat > 0) continue;
        if (mode != 0 && ch > 0) continue;
        ResonatorBank b; b.prepare(kFs);
        Params q; q.resMode = (ResMode)mode; q.material = (Material)mat; q.chord = (Chord)ch; q.resDecay = 2.f; q.strike = 1.f; q.size = 0.f;
        std::mt19937 rng(3); std::uniform_real_distribution<float> u(-1.f, 1.f);
        Buf h((size_t)(6 * kFs), 0.f); for (int i = 0; i < (int)kFs; ++i) h[(size_t)i] = u(rng);   // 1 s of full-scale noise
        Buf l(h.size()), r(h.size());
        for (size_t off = 0; off < h.size(); off += 512) b.process(h.data() + off, l.data() + off, r.data() + off, (int)std::min<size_t>(512, h.size() - off), q, 0.f);
        if (!finite(l) || !finite(r) || peak(l, h.size() - 4800) > 1e-4) ok = false;
    }
    CHECK(ok, "all modes, materials and chords under full-scale noise: finite, silent 5 s later");
    // Engine: a struck transient rings.
    {
        Params q = neutral(1); q.resMix = 1.f; q.listen = Listen::Resonance; q.root = 57;
        Buf hits((size_t)kFs, 0.f);
        for (int i = 0; i < 400; ++i) hits[(size_t)(4800 + i)] = (float)(0.8 * std::exp(-i / 60.0) * ((i % 7) < 3 ? 1 : -1));
        auto o = render(q, hits);
        CHECK(rms(o.L, 9600, 24000) > 1e-3, "engine: transients strike the resonance (level %.1f dBFS after the hit)", db(rms(o.L, 9600, 24000)));
    }
    // Pitch tracker on a 110 Hz saw.
    {
        PitchTracker t; t.prepare(kFs);
        Buf s((size_t)kFs); for (size_t i = 0; i < s.size(); ++i) s[i] = (float)(0.3 * (2.0 * std::fmod(110.0 * i / kFs, 1.0) - 1.0));
        t.push(s.data(), (int)s.size());
        CHECK(t.voiced() && std::fabs(t.hz() / 110.0 - 1.0) < 0.01, "pitch tracker: %.2f Hz on a 110 Hz saw", t.hz());
    }
}

// ---- Gates: auto-gain, engine robustness --------------------------------------------------------------------
static void testAutoGain() {
    std::mt19937 rng(5); std::normal_distribution<float> g(0.f, 1.f);
    Buf pink((size_t)(4 * kFs));
    float b0 = 0, b1 = 0, b2 = 0;
    for (auto& v : pink) { const float w = g(rng); b0 = 0.99765f * b0 + w * 0.0990460f; b1 = 0.96300f * b1 + w * 0.2965164f; b2 = 0.57000f * b2 + w * 1.0526913f; v = 0.05f * (b0 + b1 + b2 + w * 0.1848f); }
    Params p = neutral(3); p.autoGain = true;
    for (auto& b : p.band) { b.drive = 0.8f; b.harmonics = 0.6f; b.orders[1] = 0.8f; b.orders[2] = 0.5f; b.attack = 0.6f; }
    p.subLevel = 0.5f;
    auto o = render(p, pink);
    // K-weighted loudness (BS.1770 pre-filter) of the last 2 s of input and output.
    auto loud = [](const Buf& x) {
        struct Bq { double b0, b1, b2, a1, a2, z1 = 0, z2 = 0; double tick(double v) { const double y = b0 * v + z1; z1 = b1 * v - a1 * y + z2; z2 = b2 * v - a2 * y; return y; } };
        const double G = 3.999843853973347, Q1 = 0.7071752369554196, f1 = 1681.974450955533, Q2 = 0.5003270373238773, f2 = 38.13547087602444;
        const double A = std::pow(10.0, G / 40), w = 2 * kPi * f1 / kFs, cw = std::cos(w), al = std::sin(w) / (2 * Q1), sa = std::sqrt(A);
        const double a0 = (A + 1) - (A - 1) * cw + 2 * sa * al;
        Bq s{A * ((A + 1) + (A - 1) * cw + 2 * sa * al) / a0, -2 * A * ((A - 1) + (A + 1) * cw) / a0, A * ((A + 1) + (A - 1) * cw - 2 * sa * al) / a0,
             2 * ((A - 1) - (A + 1) * cw) / a0, ((A + 1) - (A - 1) * cw - 2 * sa * al) / a0};
        const double w2 = 2 * kPi * f2 / kFs, c2 = std::cos(w2), al2 = std::sin(w2) / (2 * Q2), b0 = 1 + al2;
        Bq h{(1 + c2) / 2 / b0, -(1 + c2) / b0, (1 + c2) / 2 / b0, -2 * c2 / b0, (1 - al2) / b0};
        double e = 0; size_t cnt = 0;
        for (size_t i = 0; i < x.size(); ++i) { const double v = h.tick(s.tick(x[i])); if (i >= x.size() / 2) { e += v * v; ++cnt; } }
        return 10 * std::log10(e / cnt);
    };
    const double d = loud(o.L) - loud(pink);
    CHECK(std::fabs(d) < 1.0, "auto-gain: output %+.2f dB re input on pink noise with heavy processing", d);
}

static void testBlocksAndRates() {
    Params p = neutral(3);
    for (auto& b : p.band) { b.attack = 0.7f; b.sustain = -0.4f; b.harmonics = 0.5f; b.orders[1] = 0.6f; b.drive = 0.4f; b.clip = 0.5f; }
    p.band[1].source = HarmSource::Transient; p.band[2].source = HarmSource::Body;
    p.subLevel = 0.5f; p.phantom = 0.3f; p.resMix = 0.4f; p.lookahead = 1; p.autoGain = true; p.mix = 0.8f;
    Buf x = drumLoop(0.5, 48000);
    auto ref = render(p, x, 4096);
    double worst = -300;
    for (int bs : {1, 7, 64, 333, 1024}) {
        auto o = render(p, x, bs);
        double e = 0; for (size_t i = 0; i < x.size(); ++i) e = std::max(e, (double)std::fabs(o.L[i] - ref.L[i]));
        worst = std::max(worst, db(e));
    }
    CHECK(worst < -120.0, "block sizes 1 .. 4096 give the same output (largest difference %.1f dBFS)", worst);
    bool ok = true;
    for (double fs : {44100.0, 88200.0, 96000.0, 192000.0}) {
        Buf y = sine(1000, 0.5, (int)fs);
        for (size_t i = 0; i < y.size(); i += (size_t)(fs / 4)) for (int k = 0; k < 200 && i + k < y.size(); ++k) y[i + k] *= 3.f;
        auto o = render(p, y, 512, fs);
        if (!finite(o.L) || peak(o.L) > 8.0 || rms(o.L, y.size() / 2, y.size()) < 1e-3) ok = false;
    }
    CHECK(ok, "44.1, 88.2, 96 and 192 kHz: finite, bounded, not silent");
    // Hostile input through everything.
    {
        Params h = p; h.glowSend = 1.f; h.resMode = ResMode::Comb; h.resDecay = 4.f; h.clipGuard = true;
        for (auto& b : h.band) { b.attack = 1.f; b.sustain = 1.f; b.drive = 1.f; b.curve = DriveCurve::Fold; }
        Buf y((size_t)kFs, 0.f);
        std::mt19937 rng(9); std::uniform_real_distribution<float> u(-1.f, 1.f);
        for (size_t i = 0; i < y.size(); ++i) y[i] = i < 12000 ? u(rng) : i < 24000 ? 1.f : (i % 2 ? 1.f : -1.f);
        auto o = render(h, y, 256);
        CHECK(finite(o.L) && finite(o.R) && peak(o.L) <= 0.92, "hostile input (noise, DC, Nyquist) with everything on: finite, clip guard peak %.3f", peak(o.L));
    }
}

static void testListenAndGlow() {
    Params p = neutral(1); p.resMix = 0.f; p.glowSend = 1.f; p.glowSource = GlowSource::Full; p.glowMix = 1.f; p.listen = Listen::Afterglow;
    Buf x((size_t)(2 * kFs), 0.f); for (int i = 0; i < 2400; ++i) x[(size_t)(1000 + i)] = (float)(0.5 * std::sin(i * 0.1));
    auto o = render(p, x);
    CHECK(rms(o.L, (size_t)(0.5 * kFs), (size_t)kFs) > 1e-4, "Afterglow returns a reverb tail (%.1f dBFS at 0.5-1 s)", db(rms(o.L, (size_t)(0.5 * kFs), (size_t)kFs)));
    Params d = neutral(1); d.listen = Listen::Delta;
    auto o2 = render(d, drumLoop(0.5, 24000));
    CHECK(peak(o2.L) == 0.0, "Delta at neutral settings is silent");
    Params m = neutral(3); m.stereo = StereoMode::MidSide; for (auto& b : m.band) b.attack = 1.f;
    Buf l = drumLoop(0.5, 24000), r((size_t)24000); for (size_t i = 0; i < r.size(); ++i) r[i] = -l[i] * 0.5f;
    auto o3 = render(m, l, 256, kFs, &r);
    CHECK(finite(o3.L) && finite(o3.R), "mid/side mode runs");
}

int main() {
    testResampler();
    testHilbert();
    testLinearKernel();
    testTransparency();
    testShaper();
    testOrderBank();
    testAliasing();
    testSub();
    testResonance();
    testAutoGain();
    testBlocksAndRates();
    testListenAndGlow();
    std::printf("\n%d failure(s)\n", failures);
    return failures;
}
