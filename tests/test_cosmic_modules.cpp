#include "cosmic/modules.h"
#include <cmath>
#include <cstdio>
#include <vector>

using namespace cs;
static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

// Dominant frequency by zero-crossing count over [a, b).
static double zcFreq(const std::vector<float>& x, int a, int b, double fs) {
    int z = 0; for (int i = a + 1; i < b; ++i) if ((x[i] >= 0) != (x[i - 1] >= 0)) ++z;
    return 0.5 * z * fs / (b - a);
}
static double rms(const std::vector<float>& x, int a, int b) { double s = 0; for (int i = a; i < b; ++i) s += (double)x[i] * x[i]; return std::sqrt(s / (b - a)); }

int main() {
    const double fs = 48000;
    // 1. Pitch shifter: +12 doubles the frequency, -12 halves it, level stays within 2 dB, output finite.
    for (double st : {12.0, -12.0, 7.0}) {
        GrainPitchShifter ps; ps.configure(fs); ps.setSemitones(st); ps.setGrainMs(60);
        std::vector<float> in(48000), out(48000);
        for (int i = 0; i < 48000; ++i) in[i] = (float)std::sin(2 * M_PI * 440.0 * i / fs);
        ps.process(in.data(), out.data(), 48000);
        const double f = zcFreq(out, 12000, 44000, fs), want = 440.0 * std::pow(2.0, st / 12.0);
        CHECK(std::fabs(f / want - 1) < 0.03, "shift %+g st: %g Hz, want %g", st, f, want);
        const double lvl = 20 * std::log10(rms(out, 12000, 44000) / rms(in, 12000, 44000));
        CHECK(std::fabs(lvl) < 2.0, "shift %+g st level %g dB", st, lvl);
    }
    // 2. Swept filter: low-pass at 500 Hz removes a 5 kHz tone (> 30 dB) and keeps 100 Hz (< 1 dB), stable with LFO.
    {
        SweptFilter f; f.configure(fs, 1); f.set(0, 500.0, 0.7, 0.0, 0.0);
        std::vector<float> in(48000), out(48000);
        for (int i = 0; i < 48000; ++i) in[i] = (float)std::sin(2 * M_PI * 5000.0 * i / fs);
        f.process(in.data(), out.data(), 48000);
        CHECK(20 * std::log10(rms(out, 24000, 48000) / rms(in, 24000, 48000)) < -30, "LP leak");
        f.reset(); for (int i = 0; i < 48000; ++i) in[i] = (float)std::sin(2 * M_PI * 100.0 * i / fs);
        f.process(in.data(), out.data(), 48000);
        CHECK(std::fabs(20 * std::log10(rms(out, 24000, 48000) / rms(in, 24000, 48000))) < 1.0, "LP pass band");
        f.set(1, 1000.0, 2.0, 0.5, 2.0, 0.0, 0.5); f.reset();
        for (int i = 0; i < 48000; ++i) in[i] = ((i * 7919) % 1000) / 500.f - 1.f;
        f.process(in.data(), out.data(), 48000);
        bool fin = true; for (float v : out) fin = fin && std::isfinite(v) && std::fabs(v) < 10.f;
        CHECK(fin, "swept band-pass unstable");
    }
    // 3. Saturator: unity at small signals, compresses large ones, no DC.
    {
        Saturator s; s.configure(fs); s.setDrive(0.7);
        std::vector<float> in(48000), out(48000);
        for (int i = 0; i < 48000; ++i) in[i] = 0.01f * (float)std::sin(2 * M_PI * 200.0 * i / fs);
        s.process(in.data(), out.data(), 48000);
        CHECK(std::fabs(20 * std::log10(rms(out, 24000, 48000) / rms(in, 24000, 48000))) < 0.5, "small-signal gain");
        for (int i = 0; i < 48000; ++i) in[i] = 1.0f * (float)std::sin(2 * M_PI * 200.0 * i / fs);
        s.reset(); s.process(in.data(), out.data(), 48000);
        CHECK(20 * std::log10(rms(out, 24000, 48000) / rms(in, 24000, 48000)) < -6, "large-signal compression");
        double mean = 0; for (int i = 24000; i < 48000; ++i) mean += out[i]; mean /= 24000;
        CHECK(std::fabs(mean) < 1e-3, "DC %g", mean);
    }
    // 4. Reverse buffer: a rising ramp within a chunk comes out falling; constant level; finite.
    {
        ReverseBuffer rb; rb.configure(fs); rb.setChunkMs(500);
        const int L = 24000;
        std::vector<float> in(L * 6), out(L * 6);
        for (int i = 0; i < (int)in.size(); ++i) in[i] = (float)std::sin(2 * M_PI * 300.0 * i / fs) * (0.2f + 0.8f * ((i % L) / (float)L));   // swells within each chunk
        rb.process(in.data(), out.data(), (int)in.size());
        // In the input, each chunk gets louder toward its end; reversed, it should get quieter.
        const int a = L * 3;
        const double first = rms(out, a + L / 10, a + L / 10 + 2000), last = rms(out, a + L - L / 10 - 2000, a + L - L / 10);
        CHECK(first > last * 1.5, "not reversed: start %g end %g", first, last);
        bool fin = true; for (float v : out) fin = fin && std::isfinite(v);
        CHECK(fin, "non-finite");
    }
    // 5. Granulator: produces output from input, silent for silence, finite, bounded.
    {
        Granulator gr; gr.configure(fs, 3); gr.set(40, 80, 400, 0.3, 0.3, 1.0);
        std::vector<float> in(96000), oL(96000), oR(96000);
        for (int i = 0; i < 96000; ++i) in[i] = (float)std::sin(2 * M_PI * 220.0 * i / fs) * 0.5f;
        gr.process(in.data(), in.data(), oL.data(), oR.data(), 96000);
        CHECK(rms(oL, 48000, 96000) > 0.02, "granulator silent: %g", rms(oL, 48000, 96000));
        float peak = 0; bool fin = true; for (int i = 0; i < 96000; ++i) { fin = fin && std::isfinite(oL[i]); peak = std::max(peak, std::fabs(oL[i])); }
        CHECK(fin && peak < 4.f, "granulator peak %g", peak);
        std::fill(in.begin(), in.end(), 0.f); gr.reset();
        gr.process(in.data(), in.data(), oL.data(), oR.data(), 96000);
        CHECK(rms(oL, 0, 96000) == 0.0, "granulator not silent for silence");
    }
    // 6. Pulse gate: 2 Hz full-depth gate gives a 2 Hz amplitude pattern; ducking lowers gain with a loud side input.
    {
        PulseGate pg; pg.configure(fs); pg.set(2.0, 0.8, 1.0, 0.0, 200.0);
        std::vector<float> l(48000, 1.f), r(48000, 1.f);
        pg.process(l.data(), r.data(), nullptr, nullptr, 48000);
        double mn = 1, mx = 0; for (float v : l) { mn = std::min(mn, (double)v); mx = std::max(mx, (double)v); }
        CHECK(mn < 0.05 && mx > 0.95, "gate range %g..%g", mn, mx);
        int z = 0; for (int i = 1; i < 48000; ++i) if ((l[i] >= 0.5f) != (l[i - 1] >= 0.5f)) ++z;
        CHECK(z >= 3 && z <= 4, "gate cycles: %d crossings, want 3 or 4", z);
        pg.set(2.0, 0.0, 0.0, 1.0, 100.0); pg.reset();
        std::fill(l.begin(), l.end(), 1.f); std::vector<float> side(48000, 0.8f);
        pg.process(l.data(), l.data(), side.data(), side.data(), 48000);
        CHECK(l[24000] < 0.2f, "duck %g", l[24000]);
    }
    std::printf(failures ? "%d failure(s)\n" : "cosmic module tests passed\n", failures);
    return failures ? 1 : 0;
}
