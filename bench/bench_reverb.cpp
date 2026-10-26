// CPU benchmark: stereo noise through the reverb in host-sized blocks.
// Reports average load as % of one core at real time and the worst block time as
// % of the block duration (PRD Section 8.1).  Build with -DFR_NATIVE=ON for local numbers.
#include "engines.h"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

struct Case { const char* name; cs::Quality q; double fs; int block; double budgetPct; };

int main() {
    const Case cases[] = {
        {"Normal 48k/128", cs::Quality::Normal, 48000, 128, 2.0},
        {"High   48k/128", cs::Quality::High,   48000, 128, 5.0},
        {"Eco    48k/64 ", cs::Quality::Eco,    48000, 64,  1.0},
        {"Normal 96k/128", cs::Quality::Normal, 96000, 128, 2.0 * 2.2},
    };
    // Convolution and hybrid: a synthetic exponentially decaying noise IR, stereo.
    auto makeIr = [](double fs, double sec, double t60) {
        auto ir = std::make_shared<cs::Ir>(); ir->fs = fs; ir->ch.assign(2, std::vector<float>((size_t)(sec * fs)));
        unsigned s = 99;
        for (auto& c : ir->ch) for (size_t i = 0; i < c.size(); ++i) { s = s * 1664525u + 1013904223u; c[i] = ((s >> 8) * (1.0f / 8388608.f) - 1.f) * (float)std::pow(10.0, -3.0 * i / fs / t60) * 0.05f; }
        return ir;
    };
    struct ECase { const char* name; cs::EngineMode mode; double sec, t60, budgetPct; cs::Recipe recipe = cs::Recipe::Shimmer; };
    const ECase ecases[] = {
        {"Cosmic Shimmer ", cs::EngineMode::Cosmic, 0, 0, 5.0, cs::Recipe::Shimmer},
        {"Cosmic Nebula  ", cs::EngineMode::Cosmic, 0, 0, 5.0, cs::Recipe::Nebula},
        {"Cosmic Wormhole", cs::EngineMode::Cosmic, 0, 0, 5.0, cs::Recipe::Wormhole},
        {"Conv 10 s IR   ", cs::EngineMode::Convolution, 10.0, 4.0, 6.0},
        {"Conv 30 s IR   ", cs::EngineMode::Convolution, 30.0, 8.0, 18.0},
        {"Hybrid 3 s IR  ", cs::EngineMode::Hybrid,       3.0, 2.0, 4.0},
    };
    std::printf("%-16s %9s %9s %12s %12s\n", "case", "avg %core", "budget", "p99.9 block%", "worst block%");
    for (const Case& c : cases) {
        cs::ReverbParams p; p.quality = c.q; p.erLevel = 0.5f;
        cs::Reverb rv; rv.configure(p, c.fs, c.block);
        const int seconds = 2, repeats = 5, blocks = (int)(c.fs * seconds) / c.block;
        std::vector<float> in(c.block), out(c.block), in2(c.block), out2(c.block);
        unsigned s = 12345;
        auto fill = [&](std::vector<float>& v) { for (float& x : v) { s = s * 1664525u + 1013904223u; x = ((s >> 8) * (1.0f / 8388608.f) - 1.f) * 0.3f; } };
        for (int b = 0; b < 200; ++b) { fill(in); fill(in2); rv.process(in.data(), in2.data(), out.data(), out2.data(), c.block); }  // warm up
        double total = 1e30, worst = 0;                    // best of `repeats` runs: least scheduler noise
        std::vector<double> times; times.reserve(blocks * repeats);
        for (int rep = 0; rep < repeats; ++rep) {
            double t = 0;
            for (int b = 0; b < blocks; ++b) {
                fill(in); fill(in2);
                auto t0 = std::chrono::steady_clock::now();
                rv.process(in.data(), in2.data(), out.data(), out2.data(), c.block);
                double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                t += dt; times.push_back(dt);
            }
            total = std::min(total, t);
        }
        worst = *std::max_element(times.begin(), times.end());
        std::sort(times.begin(), times.end());
        double blockSec = c.block / c.fs, p999 = times[(size_t)(times.size() * 0.999)];
        std::printf("%-16s %8.2f%% %8.1f%% %11.1f%% %11.1f%%\n", c.name, 100.0 * total / seconds, c.budgetPct, 100.0 * p999 / blockSec, 100.0 * worst / blockSec);
    }
    for (const ECase& c : ecases) {
        cs::EngineSpec spec; spec.mode = c.mode; spec.recipe = c.recipe;
        if (c.sec > 0) spec.ir = makeIr(48000, c.sec, c.t60);
        if (c.mode == cs::EngineMode::Hybrid) cs::hybridTailT60(*spec.ir, 1.0, spec.reverb.t60);
        auto eng = cs::makeEngine(spec, 48000, 128);
        eng->setLevels(1.f, 1.f, 0);
        const int block = 128, seconds = 2, repeats = 5, blocks = 48000 * seconds / block;
        std::vector<float> in(block), in2(block), out(block), out2(block);
        unsigned s = 12345;
        auto fill = [&](std::vector<float>& v) { for (float& x : v) { s = s * 1664525u + 1013904223u; x = ((s >> 8) * (1.0f / 8388608.f) - 1.f) * 0.3f; } };
        for (int b2 = 0; b2 < 500; ++b2) { fill(in); fill(in2); eng->process(in.data(), in2.data(), out.data(), out2.data(), block); }
        double best = 1e30, worst = 0; std::vector<double> times;
        for (int rep = 0; rep < repeats; ++rep) {
            double t = 0;
            for (int b2 = 0; b2 < blocks; ++b2) {
                fill(in); fill(in2);
                auto t0 = std::chrono::steady_clock::now();
                eng->process(in.data(), in2.data(), out.data(), out2.data(), block);
                double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                t += dt; times.push_back(dt);
            }
            best = std::min(best, t);
        }
        std::sort(times.begin(), times.end());
        double blockSec = block / 48000.0;
        std::printf("%-16s %8.2f%% %8.1f%% %11.1f%% %11.1f%%\n", c.name, 100.0 * best / seconds, c.budgetPct,
                    100.0 * times[(size_t)(times.size() * 0.999)] / blockSec, 100.0 * times.back() / blockSec);
    }
}
