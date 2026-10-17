#include "conv/ir_tools.h"
#include "reverb.h"
#include <cmath>
#include <cstdio>
#include <random>

using namespace fr;
static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

// Synthetic exponentially decaying noise with different T60 per band (two noise sources, low and high).
static Ir synth(double fs, double t60, double len, unsigned seed) {
    std::mt19937 r(seed);
    Ir ir; ir.fs = fs; ir.ch.assign(1, std::vector<float>((size_t)(len * fs)));
    for (size_t i = 0; i < ir.ch[0].size(); ++i)
        ir.ch[0][i] = (float)(((r() >> 8) / 8388608.0 - 1.0) * std::pow(10.0, -3.0 * i / fs / t60));
    return ir;
}

int main() {
    // 1. T60 estimate of exponential noise decay.
    for (double t : {0.4, 1.2, 3.0}) {
        Ir ir = synth(48000, t, t * 2.2, 5);
        IrAnalysis a = analyzeIr(ir);
        CHECK(std::fabs(a.t60[1] / t - 1) < 0.1, "T60 %g measured %g at 1 kHz", t, a.t60[1]);
        CHECK(std::fabs(a.t60[2] / t - 1) < 0.1, "T60 %g measured %g at 8 kHz", t, a.t60[2]);
    }
    // 2. Resampling keeps a sine's frequency and level.
    {
        const double f = 1000;
        std::vector<float> s(48000);
        for (size_t i = 0; i < s.size(); ++i) s[i] = (float)std::sin(2 * M_PI * f * i / 48000.0);
        for (double ratio : {44100.0 / 48000, 96000.0 / 48000, 2.0 / 3}) {
            auto y = resample(s, ratio);
            double err = 0; size_t from = 2000, to = y.size() - 2000;
            for (size_t i = from; i < to; ++i) err = std::max(err, std::fabs(y[i] - std::sin(2 * M_PI * f * i / (48000.0 * ratio))));
            CHECK(err < 2e-3, "resample ratio %g error %g", ratio, err);
        }
    }
    // 3. Edits: trim, reverse, stretch, width, decay, normalize.
    {
        Ir ir = synth(48000, 1.0, 2.0, 9); ir.ch.push_back(ir.ch[0]);          // identical L/R
        IrEdit e; e.normalize = false; e.fadeOutMs = 0;
        e.startMs = 100; e.maxLengthMs = 500;
        Ir t = editIr(ir, e);
        CHECK(t.length() == (size_t)(0.5 * 48000), "trim length %zu", t.length());
        CHECK(t.ch[0][0] == ir.ch[0][4800], "trim start");
        e = IrEdit(); e.normalize = false; e.fadeOutMs = 0; e.reverse = true;
        Ir rv = editIr(ir, e); CHECK(rv.ch[0][0] == ir.ch[0].back(), "reverse");
        e = IrEdit(); e.normalize = false; e.fadeOutMs = 0; e.stretch = 2.0;
        Ir st = editIr(ir, e); CHECK(std::abs((long)st.length() - 2 * (long)ir.length()) <= 2, "stretch length %zu", st.length());
        e = IrEdit(); e.normalize = false; e.fadeOutMs = 0; e.width = 0.0;
        Ir w = editIr(ir, e); CHECK(w.ch[0][100] == w.ch[1][100], "width 0 not mono");
        e = IrEdit(); e.fadeOutMs = 0;
        Ir n = editIr(ir, e);
        double pL = 0; for (float v : n.ch[0]) pL += (double)v * v;
        CHECK(std::fabs(pL / e.targetPowerGain - 1) < 1e-3, "normalize power %g", pL);
        e = IrEdit(); e.normalize = false; e.decayScale = 0.5;
        Ir d = editIr(ir, e); IrAnalysis a0 = analyzeIr(ir), a1 = analyzeIr(d);
        CHECK(std::fabs(a1.t60[1] / (0.5 * a0.t60[1]) - 1) < 0.15, "decay scale: %g -> %g", a0.t60[1], a1.t60[1]);
    }
    // 4. The algorithmic engine's power gain, to set IrEdit::targetPowerGain (reported, not asserted).
    {
        ReverbParams p; p.erLevel = 0.5f; Reverb rv; rv.configure(p, 48000, 512);
        std::vector<float> in(48000 * 8, 0.f), l(in.size()), r(in.size()); in[0] = 1.f;
        rv.process(in.data(), in.data(), l.data(), r.data(), (int)in.size());
        double pl = 0, pr = 0; for (size_t i = 0; i < l.size(); ++i) { pl += (double)l[i] * l[i]; pr += (double)r[i] * r[i]; }
        std::printf("algorithmic engine power gain (impulse on both inputs): L %.4f R %.4f\n", pl, pr);
    }
    std::printf(failures ? "%d failure(s)\n" : "ir tool tests passed\n", failures);
    return failures ? 1 : 0;
}
