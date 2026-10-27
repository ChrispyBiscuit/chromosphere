// CPU benchmark at 48 kHz. The PRD gate (G6) is the first line: one stereo instance, three bands, every stage on,
// 4x oversampling, under 3% of one core. The other lines show where the time goes.
#include "muon/engine.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace muon;

static double run(const Params& p, double seconds = 10.0) {
    Engine e; e.setParams(p);
    const double fs = 48000.0; const int block = 256;
    e.prepare(fs, block);
    std::vector<float> L(block), R(block);
    const int blocks = (int)(fs * seconds / block);
    double ph = 0.0;
    auto fill = [&] {
        for (int i = 0; i < block; ++i) {
            ph += 2.0 * 3.14159265 * 110.0 / fs;
            const float hit = (std::fmod(ph, 40.0) < 0.6) ? 0.6f : 0.f;
            L[(size_t)i] = 0.3f * (float)std::sin(ph) + hit * (float)std::sin(ph * 37.0);
            R[(size_t)i] = L[(size_t)i] * 0.9f;
        }
    };
    for (int b = 0; b < 20; ++b) { fill(); e.process(L.data(), R.data(), block); }
    double busy = 0.0;
    for (int b = 0; b < blocks; ++b) {
        fill();
        const auto t0 = std::chrono::steady_clock::now();
        e.process(L.data(), R.data(), block);
        busy += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    }
    return 100.0 * busy / seconds;
}

int main() {
    Params full;
    full.bands = 3; full.oversample = 2;
    for (auto& b : full.band) { b.attack = 0.5f; b.sustain = 0.3f; b.clip = 0.3f; b.harmonics = 0.5f; b.orders[1] = 0.6f; b.orders[2] = 0.4f; b.drive = 0.4f; }
    full.subLevel = 0.5f; full.phantom = 0.3f; full.resMix = 0.4f; full.resMode = ResMode::Body; full.autoGain = true;
    const double all = run(full);
    std::printf("PRD gate: 3 bands, every stage on, 4x: %.2f%% of one core (target < 3%%)\n", all);
    Params withGlow = full; withGlow.glowSend = 0.5f;
    std::printf("  ... plus Afterglow:                         %.2f%%\n", run(withGlow));
    Params shapeOnly; shapeOnly.bands = 3; for (auto& b : shapeOnly.band) { b.attack = 0.5f; b.sustain = 0.3f; }
    std::printf("  shaping only, 3 bands:                      %.2f%%\n", run(shapeOnly));
    Params harm = full; harm.subLevel = harm.phantom = harm.resMix = 0.f;
    std::printf("  shaping + harmonics, 4x:                    %.2f%%\n", run(harm));
    Params all2 = full; all2.oversample = 1;
    std::printf("  every stage on, 2x:                         %.2f%%\n", run(all2));
    Params harm8 = harm; harm8.oversample = 3;
    std::printf("  shaping + harmonics, 8x:                    %.2f%%\n", run(harm8));
    Params lin = full; lin.crossover = CrossoverType::LinearPhase;
    std::printf("  everything, linear phase:                   %.2f%%\n", run(lin));
    Params neutral; neutral.bands = 1; neutral.autoGain = false;
    std::printf("  neutral, one band:                          %.2f%%\n", run(neutral));
    return all < 3.0 ? 0 : 1;
}
