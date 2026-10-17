// EngineHost under live parameter changes. Runs at real-time pace (a few seconds).
#include "EngineHost.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <random>
#include <thread>

using namespace fr;
static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

struct Shared {
    std::mutex m; EngineSettings s;
    EngineSettings get() { std::lock_guard<std::mutex> g(m); return s; }
    void set(const EngineSettings& v) { std::lock_guard<std::mutex> g(m); s = v; }
};

int main() {
    const double fs = 48000; const int block = 128;
    std::mt19937 rng(42);
    auto noise = [&](std::vector<float>& v, float a) { for (float& x : v) x = ((rng() >> 8) * (1.0f / 8388608.f) - 1.f) * a; };
    auto sleepBlock = [&](int n) { std::this_thread::sleep_for(std::chrono::microseconds((long)(1e6 * n / fs * 0.5))); };   // 2x real time

    // 1. Size change: cross-fades, builds exactly once, no click.
    {
        Shared sh; EngineHost h([&] { return sh.get(); });
        h.prepare(fs, block);
        std::vector<float> in(block), l(block), r(block);
        double maxStep = 0, maxStepAfter = 0; float prev = 0; int blocks = 0, changeAt = (int)(fs * 2 / block), total = (int)(fs * 5 / block);
        bool finite = true;
        for (int b = 0; b < total; ++b) {
            if (b == changeAt) { EngineSettings s = sh.get(); s.size = 2.0; sh.set(s); }
            noise(in, 0.3f);
            h.process(in.data(), in.data(), l.data(), r.data(), block, 0.5f, 1.f, 1.0, 0.7);
            for (int i = 0; i < block; ++i) {
                finite = finite && std::isfinite(l[i]) && std::isfinite(r[i]);
                double d = std::fabs(l[i] - prev); prev = l[i];
                (b < changeAt ? maxStep : maxStepAfter) = std::max(b < changeAt ? maxStep : maxStepAfter, d);
            }
            ++blocks; sleepBlock(block);
        }
        CHECK(finite, "non-finite output");
        CHECK(h.buildsCompleted() == 1, "expected 1 build, got %d", h.buildsCompleted());
        CHECK(maxStepAfter < 1.6 * maxStep, "click on size change: %g vs %g", maxStepAfter, maxStep);
    }
    // 2. Decay change keeps the tail and applies the new time.
    {
        Shared sh; EngineHost h([&] { return sh.get(); });
        h.prepare(fs, block);
        std::vector<float> in(block, 0.f), l(block), r(block);
        auto energy = [&](int blocks) { double e = 0; for (int b = 0; b < blocks; ++b) { std::fill(in.begin(), in.end(), 0.f); h.process(in.data(), in.data(), l.data(), r.data(), block, 0.f, 1.f, 1.0, 0.7); for (float v : l) e += (double)v * v; sleepBlock(block); } return e; };
        in[0] = 1.f; h.process(in.data(), in.data(), l.data(), r.data(), block, 0.f, 1.f, 1.0, 0.7);
        double e1 = energy((int)(fs * 0.5 / block));
        CHECK(e1 > 0, "tail vanished");
        EngineSettings s = sh.get(); s.t60[0] = s.t60[1] = s.t60[2] = 0.3; sh.set(s);
        energy((int)(fs * 0.4 / block));                       // worker designs, audio ramps
        double e2 = energy((int)(fs * 0.5 / block));
        CHECK(e2 < 0.01 * e1, "decay change not applied: %g vs %g", e2, e1);
        CHECK(h.buildsCompleted() == 0, "decay change rebuilt the engine");
    }
    // 3. Fuzz: random settings and block sizes for ~8 s.
    {
        Shared sh; EngineHost h([&] { return sh.get(); });
        h.prepare(fs, 2048);
        std::vector<float> in(2048), l(2048), r(2048);
        bool ok = true; float peak = 0;
        auto t0 = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - t0 < std::chrono::seconds(8)) {
            if (rng() % 40 == 0) {
                EngineSettings s;
                s.quality = (Quality)(rng() % 3); s.size = 0.5 + (rng() % 2500) / 1000.0; s.preDelayMs = (rng() % 3) * 40.0;
                s.diffusion = rng() % 2;
                for (double& t : s.t60) t = 0.1 + (rng() % 20000) / 1000.0;
                sh.set(s);
            }
            int n = 1 + rng() % 2048;
            noise(in, 0.5f);
            h.process(in.data(), in.data(), l.data(), r.data(), n, 0.5f, 1.f, (rng() % 400) / 100.0, 0.1 + (rng() % 290) / 100.0);
            for (int i = 0; i < n; ++i) { ok = ok && std::isfinite(l[i]) && std::isfinite(r[i]); peak = std::max(peak, std::fabs(l[i])); }
            sleepBlock(n);
        }
        CHECK(ok, "non-finite output in fuzz");
        CHECK(peak < 50.f, "peak %g", peak);
        std::printf("fuzz: %d builds, peak %.2f\n", h.buildsCompleted(), peak);
    }
    // 4. Modes: Algorithmic -> Hybrid -> Convolution with a synthetic IR; live decay in hybrid; no clicks at the switches.
    {
        auto ir = std::make_shared<Ir>(); ir->fs = fs; ir->ch.assign(2, std::vector<float>((size_t)(fs * 2.5)));
        for (size_t i = 0; i < ir->ch[0].size(); ++i) for (auto& c : ir->ch) c[i] = ((rng() >> 8) * (1.0f / 8388608.f) - 1.f) * (float)std::pow(10.0, -3.0 * i / fs / 1.8) * 0.05f;
        Shared sh; EngineHost h([&] { return sh.get(); });
        h.prepare(fs, block);
        std::vector<float> in(block), l(block), r(block);
        double maxStep = 0, maxAfter = 0; float prev = 0; bool finite = true;
        auto runFor = [&](double sec, double& stepOut) {
            for (int b = 0; b < (int)(fs * sec / block); ++b) {
                noise(in, 0.3f);
                h.process(in.data(), in.data(), l.data(), r.data(), block, 0.5f, 1.f, 1.0, 0.7);
                for (int i = 0; i < block; ++i) { finite = finite && std::isfinite(l[i]); stepOut = std::max(stepOut, (double)std::fabs(l[i] - prev)); prev = l[i]; }
                sleepBlock(block);
            }
        };
        runFor(1.0, maxStep);
        EngineSettings s = sh.get(); s.mode = EngineMode::Hybrid; s.rawIr = ir; sh.set(s);
        runFor(1.2, maxAfter);
        CHECK(h.buildsCompleted() == 1, "hybrid build count %d", h.buildsCompleted());
        s.tailScale = 0.4; sh.set(s);                       // shorter hybrid tail: decay update, no rebuild
        runFor(0.8, maxAfter);
        CHECK(h.buildsCompleted() == 1, "tail scale rebuilt the engine (%d builds)", h.buildsCompleted());
        s.mode = EngineMode::Convolution; sh.set(s);
        runFor(1.2, maxAfter);
        CHECK(h.buildsCompleted() == 2, "convolution build count %d", h.buildsCompleted());
        s.edit.lowCutHz = 200; s.edit.reverse = true; sh.set(s);       // IR edits rebuild
        runFor(1.2, maxAfter);
        CHECK(h.buildsCompleted() == 3, "IR edit build count %d", h.buildsCompleted());
        s.rawIr.reset(); sh.set(s);                                   // IR removed: falls back to the algorithmic engine
        runFor(1.2, maxAfter);
        CHECK(finite, "non-finite output when switching modes");
        CHECK(maxAfter < 3.0 * maxStep, "click at mode switches: %g vs %g", maxAfter, maxStep);
    }
    // 5. prepare() twice (sample-rate change) is safe.
    {
        Shared sh; EngineHost h([&] { return sh.get(); });
        h.prepare(44100, 64); h.prepare(96000, 1024); h.release(); h.prepare(48000, 256);
        std::vector<float> in(256, 0.1f), l(256), r(256);
        h.process(in.data(), in.data(), l.data(), r.data(), 256, 0.5f, 1.f, 1.0, 0.7);
        CHECK(std::isfinite(l[0]), "after re-prepare");
    }
    std::printf(failures ? "%d failure(s)\n" : "host tests passed\n", failures);
    return failures ? 1 : 0;
}
