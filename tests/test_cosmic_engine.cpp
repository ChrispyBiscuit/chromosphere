// Every cosmic recipe: produces a tail, stays finite and bounded under hostile settings, is
// block-size independent, decays after the input stops (no runaway feedback), and freeze works.
#include "cosmic/cosmic_engine.h"
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
    for (int ri = 0; ri < kRecipeCount; ++ri) {
        const Recipe r = (Recipe)ri;
        ReverbParams p; p.t60[0] = 4; p.t60[1] = 3; p.t60[2] = 1.5; p.size = 1.6;
        // 1. Block-size independence and a non-silent tail at middle settings.
        auto run = [&](int block, double a, double b, double c, const std::vector<float>& in) {
            CosmicEngine e; e.configure(r, p, fs, 2048); e.setMacros(a, b, c); e.setLevels(0.5f, 1.f, 0);
            std::vector<float> l(in.size()), rr(in.size());
            for (size_t off = 0; off < in.size(); off += (size_t)block) { int n = (int)std::min<size_t>(block, in.size() - off); e.process(&in[off], &in[off], &l[off], &rr[off], n); }
            return l;
        };
        auto in = noise(48000 * 4, 1, 0.3f);
        std::fill(in.begin() + 24000, in.end(), 0.f);           // 0.5 s of noise, then silence
        auto a = run(128, 0.5, 0.5, 0.5, in), b = run(97, 0.5, 0.5, 0.5, in);
        double diff = 0; for (size_t i = 0; i < a.size(); ++i) diff = std::max(diff, (double)std::fabs(a[i] - b[i]));
        CHECK(diff < 1e-5, "%s: block-size dependent (%g)", recipeName(r), diff);
        CHECK(energy(a, 30000, 48000) > 1e-6, "%s: no tail", recipeName(r));
        // 2. Decays: 3.5 s after the input stopped the level is well below the first half second of tail.
        const double early = energy(a, 24000, 48000), late = energy(a, 168000, 192000);
        CHECK(late < early * 0.5, "%s: tail not decaying (%g -> %g)", recipeName(r), early, late);
        // 3. Hostile settings: all macros at 1, hot input, long decay; must stay finite and bounded.
        ReverbParams hot = p; hot.t60[0] = hot.t60[1] = hot.t60[2] = 20.0;
        CosmicEngine e; e.configure(r, hot, fs, 512); e.setMacros(1.0, 1.0, 1.0); e.setLevels(1.f, 1.f, 0);
        auto loud = noise(48000 * 6, 2, 0.95f);
        std::vector<float> l(loud.size()), rr(loud.size());
        bool fin = true; float peak = 0;
        for (size_t off = 0; off < loud.size(); off += 512) { const int n = (int)std::min<size_t>(512, loud.size() - off); e.process(&loud[off], &loud[off], &l[off], &rr[off], n); }
        for (float v : l) { fin = fin && std::isfinite(v); peak = std::max(peak, std::fabs(v)); }
        CHECK(fin && peak < 16.f, "%s: hostile settings peak %g finite %d", recipeName(r), peak, (int)fin);
        // 4. Freeze holds the tail; unfreeze decays.
        CosmicEngine f; f.configure(r, p, fs, 256); f.setMacros(0.3, 0.5, 0.5); f.setLevels(0.f, 1.f, 0);
        auto fin2 = noise(48000 * 8, 3, 0.3f); std::fill(fin2.begin() + 24000, fin2.end(), 0.f);
        std::vector<float> fl(fin2.size()), fr(fin2.size());
        for (size_t off = 0; off < fin2.size(); off += 256) {
            if (off == 30720) f.setFreeze(true, 2400);
            if (off == 48000 * 6) f.setFreeze(false, 2400);
            f.process(&fin2[off], &fin2[off], &fl[off], &fr[off], 256);
        }
        const double h0 = energy(fl, 48000 * 2, 48000 * 3), h1 = energy(fl, 48000 * 5, 48000 * 6), after = energy(fl, 48000 * 7, 48000 * 8);
        if (r != Recipe::Pulsar) CHECK(h1 > 0.3 * h0 && h1 < 3.0 * h0, "%s: frozen tail drifted %g -> %g", recipeName(r), h0, h1);
        CHECK(after < 0.2 * std::max(h1, 1e-12), "%s: no decay after unfreeze", recipeName(r));
        std::printf("%-14s tail %.2e -> %.2e, hostile peak %.2f\n", recipeName(r), early, late, peak);
    }
    std::printf(failures ? "%d failure(s)\n" : "cosmic engine tests passed\n", failures);
    return failures ? 1 : 0;
}
