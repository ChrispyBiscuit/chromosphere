// Acoustic engine: physics checks of the room model and behaviour checks of the runtime engine.
#include "space/room_model.h"
#include "space/space_engine.h"
#include "conv/ir_tools.h"
#include <cmath>
#include <cstdio>
#include <vector>

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { std::printf("FAIL %s:%d  ", __FILE__, __LINE__); std::printf(__VA_ARGS__); std::printf("\n"); ++failures; } } while (0)

using namespace cs;

static RoomSpec room(double L, double W, double H, const char* walls, const char* floor, const char* ceil) {
    RoomSpec r; r.length = L; r.width = W; r.height = H;
    r.walls = materialIndex(walls); r.floor = materialIndex(floor); r.ceiling = materialIndex(ceil);
    return r;
}

// Impulse response of the engine (impulse on both inputs), wet only.
static void impulse(const SpaceParams& p, double fs, size_t len, std::vector<float>& l, std::vector<float>& r, int block = 256) {
    SpaceEngine e; e.configure(p, fs, 512);
    std::vector<float> in(len, 0.f); in[0] = 1.f;
    l.assign(len, 0.f); r.assign(len, 0.f);
    for (size_t off = 0; off < len; off += (size_t)block) {
        const int n = (int)std::min((size_t)block, len - off);
        e.process(in.data() + off, in.data() + off, l.data() + off, r.data() + off, n);
    }
}

// T60 from Schroeder backward integration of one octave band, fitted between -5 and -25 dB (T20).
static double measureT60(const std::vector<float>& x, double fc, double fs) {
    Ir ir; ir.fs = fs; ir.ch.push_back(x);
    std::vector<float> y = x;
    // Octave band: use bandPower's filter indirectly by squaring a band-passed copy over short windows.
    const size_t win = (size_t)(0.005 * fs);
    std::vector<double> e;
    for (size_t s = 0; s + win <= y.size(); s += win) e.push_back(bandPower(y, fc, fs, s, s + win) * win);
    std::vector<double> sch(e.size());
    double acc = 0;
    for (size_t i = e.size(); i-- > 0;) { acc += e[i]; sch[i] = acc; }
    const double top = sch[0];
    double t5 = -1, t25 = -1;
    for (size_t i = 0; i < sch.size(); ++i) {
        const double db = 10 * std::log10(sch[i] / top + 1e-30);
        if (t5 < 0 && db <= -5) t5 = i * 0.005;
        if (t25 < 0 && db <= -25) { t25 = i * 0.005; break; }
    }
    return t5 >= 0 && t25 > t5 ? 3.0 * (t25 - t5) : -1;
}

int main() {
    const double fs = 48000;

    // 1. Air absorption (ISO 9613-1) at 20 C, 50 % RH: 4.66 dB/km at 1 kHz, about 105 dB/km at 8 kHz.
    {
        const double a1 = airAbsorptionDbPerM(1000, 20, 50), a8 = airAbsorptionDbPerM(8000, 20, 50);
        CHECK(std::fabs(a1 - 0.00466) < 0.0003, "air 1 kHz %g dB/m", a1);
        CHECK(std::fabs(a8 - 0.105) < 0.01, "air 8 kHz %g dB/m", a8);
        CHECK(airAbsorptionDbPerM(8000, 20, 15) > a8, "dry air must absorb more at 8 kHz");
    }
    // 2. Eyring T60: a concert-hall shoebox lands in the concert-hall range; materials change the decay.
    {
        const RoomAcoustics hall = analyseRoom(room(49, 19.8, 17.8, "Plaster", "Audience", "Plaster"));
        const double tMid = 0.5 * (hall.t60[3] + hall.t60[4]);
        CHECK(tMid > 1.4 && tMid < 3.2, "hall T60 mid %g s", tMid);
        CHECK(hall.t60[7] < hall.t60[4], "air must shorten the hall's 8 kHz decay (%g vs %g)", hall.t60[7], hall.t60[4]);
        const RoomAcoustics bare = analyseRoom(room(5, 4, 3, "Painted concrete", "Painted concrete", "Painted concrete"));
        const RoomAcoustics soft = analyseRoom(room(5, 4, 3, "Painted concrete", "Thick carpet", "Acoustic tile"));
        CHECK(bare.t60[5] > 4 * soft.t60[5], "carpet and tile must shorten 2 kHz decay: %g vs %g", bare.t60[5], soft.t60[5]);
        CHECK(bare.schroederHz > 200, "small bare room Schroeder frequency %g", bare.schroederHz);
        CHECK(hall.schroederHz < 30, "hall Schroeder frequency %g", hall.schroederHz);
        CHECK(std::fabs(analyseRoom(room(10, 10, 10, "Plaster", "Plaster", "Plaster")).meanFreePath - 4.0 * 1000 / 600) < 1e-9, "mean free path");
    }
    // 3. Geometry: the floor reflection arrives when the image source says; 1/r direct gain.
    {
        RoomSpec r = room(20, 12, 8, "Plaster", "Wood floor", "Plaster");
        r.mic = MicArray::Mono; r.spread = 0; r.distance = 6;
        const RoomRender rr = renderRoom(r, fs);
        const double* s = rr.source[0]; const double* m = rr.mic[0];
        const double dDirect = std::sqrt((s[0] - m[0]) * (s[0] - m[0]) + (s[1] - m[1]) * (s[1] - m[1]) + (s[2] - m[2]) * (s[2] - m[2]));
        const double dFloor = std::sqrt((s[0] - m[0]) * (s[0] - m[0]) + (s[1] - m[1]) * (s[1] - m[1]) + (s[2] + m[2]) * (s[2] + m[2]));
        const double want = 1000.0 * (dFloor - dDirect) / rr.acoustics.c;
        CHECK(std::fabs(rr.firstReflectionMs - want) < 0.05, "first reflection %g ms, floor image says %g ms", rr.firstReflectionMs, want);
        // The early part starts there: its first sample above a fifth of its peak.
        const auto& e = rr.early[0];
        float peak = 0; for (float v : e) peak = std::max(peak, std::fabs(v));
        size_t first = 0; while (first < e.size() && std::fabs(e[first]) < 0.2f * peak) ++first;
        CHECK(std::fabs(1000.0 * first / fs - want) < 0.3, "early part starts at %g ms, floor reflection %g ms", 1000.0 * first / fs, want);
        CHECK(std::fabs(rr.direct[0].gain / rr.norm * dDirect - 1.0) < 1e-3, "direct gain is not 1/r: %g", rr.direct[0].gain / rr.norm * dDirect);
        RoomSpec far = r; far.distance = 12;
        const RoomRender rf = renderRoom(far, fs);
        const double ratio = (rr.direct[0].gain / rr.norm) / (rf.direct[0].gain / rf.norm);
        const double* mf = rf.mic[0];
        const double dFar = std::sqrt((s[0] - mf[0]) * (s[0] - mf[0]) + (s[2] - mf[2]) * (s[2] - mf[2]) + (s[1] - mf[1]) * (s[1] - mf[1]));
        CHECK(std::fabs(ratio - dFar / dDirect) < 1e-3, "inverse distance law: ratio %g, distances %g", ratio, dFar / dDirect);
    }
    // 4. Engine: measured decay matches the model; output is stereo; behaviour.
    {
        SpaceParams p; p.room = room(24, 16, 10, "Plaster", "Wood floor", "Plaster");
        const RoomAcoustics a = analyseRoom(p.room);
        std::vector<float> l, r;
        const size_t len = (size_t)(fs * std::min(6.0, 1.2 * a.t60[2] + 0.5));
        impulse(p, fs, len, l, r);
        for (double fc : {500.0, 1000.0, 4000.0}) {
            const double want = spaceT60At(a, fc), got = measureT60(l, fc, fs);
            CHECK(got > 0 && std::fabs(got / want - 1.0) < 0.2, "T60 at %g Hz: measured %g s, model %g s", fc, got, want);
        }
        bool finite = true; double el = 0, er = 0, elr = 0;
        for (size_t i = 0; i < len; ++i) {
            if (!std::isfinite(l[i]) || !std::isfinite(r[i])) finite = false;
            if (i > (size_t)(0.15 * fs)) { el += (double)l[i] * l[i]; er += (double)r[i] * r[i]; elr += (double)l[i] * r[i]; }
        }
        CHECK(finite, "non-finite output");
        const double corr = elr / std::sqrt(el * er + 1e-30);
        CHECK(std::fabs(corr) < 0.6, "ORTF late field too correlated: %g", corr);
        CHECK(std::fabs(10 * std::log10(el / er)) < 1.5, "left/right late energy differ: %g dB", 10 * std::log10(el / er));
        // Transition from the early part to the tail: no hole and no bump (+-6 dB around the hand-over).
        const RoomRender rr = renderRoom(p.room, fs);
        auto winDb = [&](double ms0, double ms1) {
            double s = 0; size_t a0 = (size_t)(ms0 * fs / 1000), a1 = (size_t)(ms1 * fs / 1000);
            for (size_t i = a0; i < a1; ++i) s += (double)l[i] * l[i] + (double)r[i] * r[i];
            return 10 * std::log10(s / (a1 - a0) + 1e-30);
        };
        const double before = winDb(0.5 * rr.earlyMs, 0.7 * rr.earlyMs), after = winDb(1.1 * rr.earlyMs, 1.3 * rr.earlyMs);
        const double tMs = 1000.0 * a.t60[4];
        const double expectDrop = 60.0 * (0.6 * rr.earlyMs) / tMs;   // decay between the two window centres
        CHECK(std::fabs((before - after) - expectDrop) < 6.0, "hand-over level step %g dB (expected decay %g dB)", before - after, expectDrop);

        // Block-size independence.
        std::vector<float> l2, r2, l3, r3;
        impulse(p, fs, 30000, l2, r2, 64);
        impulse(p, fs, 30000, l3, r3, 509);
        bool same = true; for (size_t i = 0; i < 30000; ++i) if (l2[i] != l3[i] || r2[i] != r3[i]) { same = false; break; }
        CHECK(same, "output depends on block size");
        // Silence in, silence out.
        SpaceEngine e; e.configure(p, fs, 512);
        std::vector<float> z(4096, 0.f), ol(4096), orr(4096);
        e.process(z.data(), z.data(), ol.data(), orr.data(), 4096);
        float m = 0; for (size_t i = 0; i < 4096; ++i) m = std::max(m, std::max(std::fabs(ol[i]), std::fabs(orr[i])));
        CHECK(m == 0.f, "silent input gave %g", m);
    }
    // 5. Small rooms get modes; the mono array is fully correlated; the tail ends.
    {
        SpaceParams p; p.room = room(4.5, 3.6, 2.6, "Painted concrete", "Wood floor", "Gypsum board"); p.room.mic = MicArray::Mono;
        const RoomRender rr = renderRoom(p.room, fs);
        CHECK(!rr.modes.empty() && rr.modeCrossoverHz > 60, "small room has %d modes, crossover %g Hz", (int)rr.modes.size(), rr.modeCrossoverHz);
        CHECK(std::fabs(rr.modes[0].freq - 0.5 * rr.acoustics.c / 4.5) < 0.5, "lowest mode %g Hz", rr.modes[0].freq);
        std::vector<float> l, r;
        impulse(p, fs, (size_t)(fs * 3), l, r);
        double d = 0, e2 = 0; for (size_t i = 0; i < l.size(); ++i) { d += std::fabs(l[i] - r[i]); e2 += std::fabs(l[i]); }
        CHECK(d < 1e-3 * e2, "mono array should give identical channels");
        float tailMax = 0; for (size_t i = l.size() - 4800; i < l.size(); ++i) tailMax = std::max(tailMax, std::fabs(l[i]));
        CHECK(tailMax < 1e-3f, "small room still ringing after 3 s: %g", tailMax);
        const RoomRender hall = renderRoom(room(49, 19.8, 17.8, "Plaster", "Audience", "Plaster"), fs);
        CHECK(hall.modes.empty(), "a hall should not get a modal bank (%d modes)", (int)hall.modes.size());
    }
    if (failures == 0) std::printf("space engine: all checks passed\n");
    return failures ? 1 : 0;
}
