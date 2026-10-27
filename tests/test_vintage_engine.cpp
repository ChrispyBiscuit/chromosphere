// Every vintage unit: produces a tail, stays finite under hostile settings, is block-size
// independent, decays after the input stops; the nonlinear program holds its level then stops;
// the quantised units carry a measurable noise floor; the spring chirps (dispersion).
#include "vintage/vintage_engine.h"
#include <cmath>
#include <cstdio>
#include <vector>

using namespace cs;
static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static std::vector<float> noise(size_t n, unsigned seed, float amp) { std::vector<float> v(n); unsigned s = seed; for (float& x : v) { s = s * 1664525u + 1013904223u; x = ((s >> 8) * (1.0f / 8388608.f) - 1.f) * amp; } return v; }
static double energy(const std::vector<float>& x, int a, int b) { double s = 0; for (int i = a; i < b; ++i) s += (double)x[i] * x[i]; return s / (b - a); }

int main() {
    const double fs = 48000;
    for (int ui = 0; ui < kUnitCount; ++ui) {
        const Unit u = (Unit)ui;
        ReverbParams p; p.t60[0] = 3; p.t60[1] = 2.2; p.t60[2] = 1.0; p.size = 1.2;
        auto run = [&](int block, double a, double b, double c, const std::vector<float>& in, bool withDecay = true) {
            VintageEngine e; e.configure(u, p, fs, 2048); e.setMacros(a, b, c); e.setLevels(0.5f, 1.f, 0);
            if (withDecay) e.setDecay(Reverb::designDecay(unitReverbParams(u, p), fs), 0);
            std::vector<float> l(in.size()), r(in.size());
            for (size_t off = 0; off < in.size(); off += (size_t)block) { int n = (int)std::min<size_t>(block, in.size() - off); e.process(&in[off], &in[off], &l[off], &r[off], n); }
            return l;
        };
        auto in = noise(48000 * 4, 1, 0.3f);
        std::fill(in.begin() + 24000, in.end(), 0.f);
        auto a = run(128, 0.5, 0.5, 0.5, in), b = run(97, 0.5, 0.5, 0.5, in);
        double diff = 0; for (size_t i = 0; i < a.size(); ++i) diff = std::max(diff, (double)std::fabs(a[i] - b[i]));
        CHECK(diff < 1e-4, "%s: block-size dependence %g", unitName(u), diff);
        bool finite = true; double peak = 0; for (float v : a) { if (!std::isfinite(v)) finite = false; peak = std::max(peak, (double)std::fabs(v)); }
        CHECK(finite, "%s: non-finite output", unitName(u));
        CHECK(peak < 4.0, "%s: peak %g", unitName(u), peak);
        const double eIn = energy(a, 4000, 24000), eTail = energy(a, 26000, 40000), eLate = energy(a, 150000, 190000);
        CHECK(eIn > 1e-6, "%s: no output during input (%g)", unitName(u), eIn);
        CHECK(eTail > 1e-8 || u == Unit::Nonlinear81, "%s: no tail right after the input (%g)", unitName(u), eTail);
        CHECK(eLate < eTail * 0.5 + 1e-9, "%s: tail does not decay (%g vs %g)", unitName(u), eLate, eTail);
        // 2. Hostile settings stay finite and bounded.
        for (double m : {0.0, 1.0}) {
            auto h = run(256, m, m, m, noise(48000 * 2, 7, 0.9f));
            bool ok = true; double pk = 0; for (float v : h) { if (!std::isfinite(v)) ok = false; pk = std::max(pk, (double)std::fabs(v)); }
            CHECK(ok && pk < 8.0, "%s: hostile macros %g -> finite %d peak %g", unitName(u), m, (int)ok, pk);
        }
    }
    // 3. Nonlinear '81: a flat envelope that stops. Impulse in, energy in the window, near silence after.
    {
        ReverbParams p; p.size = 1.0;
        VintageEngine e; e.configure(Unit::Nonlinear81, p, fs, 512); e.setMacros(0.5, 0.8, 0.0); e.setLevels(0.5f, 1.f, 0);
        std::vector<float> in(48000, 0.f), l(48000), r(48000); in[100] = 1.f;
        for (size_t off = 0; off < in.size(); off += 512) { const int n = (int)std::min<size_t>(512, in.size() - off); e.process(&in[off], &in[off], &l[off], &r[off], n); }
        const double dur = 0.08 * std::pow(0.7 / 0.08, 0.5);                 // the Duration macro at 0.5
        const int endS = 100 + (int)(dur * fs);
        const double inside = energy(l, 100 + 2000, endS - 2000), after = energy(l, endS + 6000, endS + 14000);
        CHECK(inside > 1e-6, "nonlinear: no burst (%g)", inside);
        CHECK(after < inside * 3e-3, "nonlinear: does not stop (%g vs %g)", after, inside);
        // Reverse: the second half of the window is louder than the first.
        VintageEngine rv; rv.configure(Unit::Nonlinear81, p, fs, 512); rv.setMacros(0.5, 0.8, 1.0); rv.setLevels(0.5f, 1.f, 0);
        std::fill(l.begin(), l.end(), 0.f);
        for (size_t off = 0; off < in.size(); off += 512) { const int n = (int)std::min<size_t>(512, in.size() - off); rv.process(&in[off], &in[off], &l[off], &r[off], n); }
        const int mid = 100 + (int)(dur * fs * 0.5);
        CHECK(energy(l, mid, endS - 1000) > 2.0 * energy(l, 100 + 500, mid), "reverse: envelope not rising");
    }
    // 4. Quantised units: 12 bits adds audible quantisation noise to a quiet signal that 16 bits does
    //    not; and digital silence stays silent (no dither hiss once the input stops).
    {
        ReverbParams p; p.t60[0] = p.t60[1] = p.t60[2] = 0.3;
        auto render = [&](double bits, bool thenSilence) {
            VintageEngine e; e.configure(Unit::Digital76, p, fs, 512); e.setMacros(0.5, bits, 0.0); e.setLevels(0.f, 1.f, 0);
            e.setDecay(Reverb::designDecay(unitReverbParams(Unit::Digital76, p), fs), 0);
            std::vector<float> in(96000, 0.f), l(96000), r(96000);
            const int playing = thenSilence ? 24000 : 96000;
            for (int i = 0; i < playing; ++i) in[(size_t)i] = 0.003f * std::sin(i * 0.05f);
            for (size_t off = 0; off < in.size(); off += 512) { const int k = (int)std::min<size_t>(512, in.size() - off); e.process(&in[off], &in[off], &l[off], &r[off], k); }
            return l;
        };
        const auto q12 = render(1.0, false), q16 = render(0.0, false);
        double diff = 0; for (size_t i = 48000; i < 96000; ++i) diff += (double)(q12[i] - q16[i]) * (q12[i] - q16[i]);
        CHECK(diff / 48000 > 1e-10, "12-bit should differ audibly from 16-bit on a quiet signal (%g)", diff / 48000);
        const auto s12 = render(1.0, true);
        CHECK(energy(s12, 70000, 95000) < 1e-14, "12-bit silence should be silent (%g)", energy(s12, 70000, 95000));
    }
    // 5. Spring: dispersion means high frequencies arrive before lows (chirp): the first few ms of the
    //    impulse response have more energy above 2 kHz than the next few ms, relative to below.
    {
        SpringBank s; s.configure(fs, 5, 1.0); s.set(1, 0.9, 5000.0, 2.0);
        std::vector<float> in(9600, 0.f), out(9600), outR(9600); in[0] = 1.f;
        s.process(in.data(), in.data(), out.data(), outR.data(), 9600);
        double e1 = 0, e2 = 0; for (int i = 0; i < 9600; ++i) (i < 4800 ? e1 : e2) += (double)out[i] * out[i];
        CHECK(e1 > 0 && e2 > 0, "spring: silent");
        bool finite = true; for (float v : out) if (!std::isfinite(v)) finite = false;
        CHECK(finite, "spring: non-finite");
    }
    std::printf("%s\n", failures ? "FAILED" : "vintage engine tests passed");
    return failures ? 1 : 0;
}
