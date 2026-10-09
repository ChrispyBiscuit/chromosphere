#include "reverb.h"
#include "fdn/decay_design.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static std::vector<float> render(const fr::ReverbParams& p, double fs, int len, int block, bool impulse, unsigned seed = 1, float amp = 1.f) {
    fr::Reverb rv; rv.configure(p, fs, block);
    std::vector<float> in(len, 0.f), out(2 * len);
    if (impulse) in[0] = amp;
    else { unsigned s = seed; for (float& x : in) { s = s * 1664525u + 1013904223u; x = ((s >> 8) * (1.0f / 8388608.f) - 1.f) * amp; } }
    std::vector<float> oL(len), oR(len);
    for (int off = 0; off < len; off += block) {
        int n = std::min(block, len - off);
        rv.process(&in[off], &in[off], &oL[off], &oR[off], n);
    }
    for (int i = 0; i < len; ++i) { out[2 * i] = oL[i]; out[2 * i + 1] = oR[i]; }
    return out;
}

int main() {
    const double fs = 48000;
    // 1. Decay filter design hits the target T60 curve (within 0.3 dB per pass).
    {
        double t60[3] = {3.0, 2.0, 1.0}, delay = 1500;
        fr::DecayFilter d = fr::designDecayFilter(delay, t60, fs);
        for (double f : {125., 250., 500., 1000., 2000., 4000., 8000.}) {
            double want = -60.0 * delay / (fs * fr::t60Target(f, t60));
            CHECK(std::fabs(fr::decayFilterDb(d, f, fs) - want) < 0.3, "f=%g got %g want %g", f, fr::decayFilterDb(d, f, fs), want);
        }
        for (double f : {20., 100., 1000., 10000., 20000.}) CHECK(fr::decayFilterDb(d, f, fs) < 0.0, "filter gain >= 1 at %g Hz", f);
    }
    // 2. Silence in -> exact silence out (Q8).
    {
        fr::ReverbParams p; p.erLevel = 0.5f;
        auto y = render(p, fs, 48000, 128, false, 1, 0.f);
        float m = 0; for (float v : y) m = std::max(m, std::fabs(v));
        CHECK(m == 0.f, "silent input gave %g", m);
    }
    // 3. Block-size independence: same output for any host block size.
    {
        fr::ReverbParams p; p.erLevel = 0.5f;
        auto a = render(p, fs, 20000, 64, false, 7, 0.3f);
        auto b = render(p, fs, 20000, 509, false, 7, 0.3f);
        auto c = render(p, fs, 20000, 1, false, 7, 0.3f);
        bool same = a == b && a == c;
        CHECK(same, "output depends on block size");
    }
    // 4. Determinism (Q11).
    {
        fr::ReverbParams p; p.erLevel = 0.5f;
        CHECK(render(p, fs, 20000, 128, false, 3, 0.3f) == render(p, fs, 20000, 128, false, 3, 0.3f), "not deterministic");
    }
    // 5. Stability: no NaN/Inf and bounded energy across extreme settings; decay after input stops.
    {
        const double sizes[] = {0.3, 1.0, 3.0};
        const double t60s[][3] = {{0.2, 0.2, 0.1}, {3, 2, 1}, {20, 20, 20}};
        const fr::Quality qs[] = {fr::Quality::Eco, fr::Quality::Normal, fr::Quality::High};
        for (double size : sizes) for (auto& t : t60s) for (auto q : qs) {
            fr::ReverbParams p; p.size = size; p.quality = q; p.erLevel = 0.5f;
            for (int k = 0; k < 3; ++k) p.t60[k] = t[k];
            p.modDepth = 4.0;
            auto y = render(p, fs, 24000, 256, false, 5, 0.9f);
            bool ok = true; float m = 0;
            for (float v : y) { if (!std::isfinite(v)) ok = false; m = std::max(m, std::fabs(v)); }
            CHECK(ok && m < 20.f, "size %g t60 %g q %d peak %g", size, t[0], (int)q, m);
        }
    }
    // 6. Decay: tail energy at 1.5 s is far below the first 100 ms (T60 3 s => about 30 dB down).
    {
        fr::ReverbParams p;
        auto y = render(p, fs, 96000, 128, true);
        auto e = [&](int a, int b) { double s = 0; for (int i = a; i < b; ++i) s += (double)y[2 * i] * y[2 * i]; return s; };
        double early = e(0, 4800), late = e(72000, 76800);
        CHECK(10 * std::log10(late / early) < -20, "decay too slow: %g dB", 10 * std::log10(late / early));
    }
    std::printf(failures ? "%d failure(s)\n" : "all tests passed\n", failures);
    return failures ? 1 : 0;
}
