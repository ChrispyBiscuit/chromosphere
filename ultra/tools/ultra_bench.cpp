// CPU benchmark (PRD G8): all 8 tracks on 1/32 steps with ratchets, long decays, every effect and macro on.
// Prints the share of one core used at 48 kHz.
#include "ultra/machine.h"
#include <chrono>
#include <cstdio>
#include <vector>

using namespace ultra;

int main() {
    const double fs = 48000.0; const int block = 256;
    PatternBank bank;
    for (int t = 0; t < kTracks; ++t) {
        auto& tp = bank.patterns[0].tracks[(size_t)t];
        tp.rate = 0; tp.length = 32;
        for (int s = 0; s < 32; ++s) tp.steps[(size_t)s] = {120, 100, (uint8_t)(1 + s % 2), (int8_t)(s % 5), 0, 0, 20};
    }
    KitParams k;
    const Model models[kTracks] = {Model::GravityKick, Model::FlareSnare, Model::BurstClap, Model::IonHat,
                                   Model::CoronaCymbal, Model::NebulaModal, Model::QuasarMetal, Model::SolarWind};
    for (int t = 0; t < kTracks; ++t) {
        auto& tp = k.tracks[(size_t)t];
        tp.v.model = models[t]; tp.v.decay = 0.9f; tp.v.drive = 0.3f;
        tp.v.filter = (FilterType)(1 + t % 8); tp.v.cutoff = 4000.f;
        tp.sendA = 0.3f; tp.sendB = 0.5f;
    }
    k.master.drive = 0.4f; k.master.glueMix = 1.f; k.master.lowShelf = 3.f; k.master.highShelf = 2.f;
    k.macro = {0.5f, 0.5f, 0.5f, 0.5f};
    DrumMachine m; m.setParams(k); m.setPatterns(&bank); m.prepare(fs, block);
    m.internalPlay = true;
    std::vector<float> L(block), R(block);
    Outputs o; o.L = L.data(); o.R = R.data();
    for (int i = 0; i < 400; ++i) m.process(o, block, nullptr, 0, {});      // warm up, let the reverb build
    const int blocks = (int)(20.0 * fs / block);
    int maxVoices = 0;
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < blocks; ++i) { m.process(o, block, nullptr, 0, {}); maxVoices = std::max(maxVoices, m.activeVoices.load()); }
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("ultra_bench: 20 s of audio in %.3f s = %.1f%% of one core (up to %d voices)\n", secs, 100.0 * secs / 20.0, maxVoices);
    return 0;
}
