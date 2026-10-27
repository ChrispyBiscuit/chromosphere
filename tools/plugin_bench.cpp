// Whole-plug-in CPU benchmark: the real processor (no editor), every Mode, 48 kHz, 256-sample blocks,
// stereo noise. Prints % of one core per instance at real time (best of 3 runs of 4 s) and the
// worst block as % of its deadline.  cs_plugin_bench [mode-index]
#include "PluginProcessor.h"
#include "engines.h"
#include <chrono>
#include <cstdio>

static void setParam(ChromosphereProcessor& p, const char* id, float v) {
    auto* prm = p.apvts.getParameter(id);
    prm->setValueNotifyingHost(prm->convertTo0to1(v));
}

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000; const int block = 256;
    const int only = argc > 1 ? atoi(argv[1]) : -1;
    std::printf("%-16s %10s %12s %10s\n", "mode", "% core", "worst blk %", "quiet %");
    double sum = 0; int n = 0;
    for (int mode = 0; mode < cs::kModeCount; ++mode) {
        if (only >= 0 && mode != only) continue;
        ChromosphereProcessor proc;
        proc.setPlayConfigDetails(2, 2, fs, block);
        setParam(proc, "mode", (float)mode);
        proc.prepareToPlay(fs, block);
        if (mode == 1 || mode == 2) proc.loadFactoryImpulseResponse("studio_hall");
        juce::AudioBuffer<float> buf(2, block); juce::MidiBuffer midi;
        unsigned s = 1;
        auto fill = [&] { for (int c = 0; c < 2; ++c) { auto* d = buf.getWritePointer(c); for (int i = 0; i < block; ++i) { s = s * 1664525u + 1013904223u; d[i] = ((s >> 8) * (1.0f / 8388608.f) - 1.f) * 0.2f; } } };
        // Let the worker build the engine and the crossfade finish.
        for (int i = 0; i < 400; ++i) { fill(); proc.processBlock(buf, midi); if (i % 20 == 0) juce::Thread::sleep(5); }
        juce::Thread::sleep(200);
        for (int i = 0; i < 200; ++i) { fill(); proc.processBlock(buf, midi); }
        const int blocks = (int)(fs * 4) / block;
        double best = 1e30, worst = 0;
        for (int rep = 0; rep < 3; ++rep) {
            double t = 0;
            for (int b = 0; b < blocks; ++b) {
                fill();
                const auto t0 = std::chrono::steady_clock::now();
                proc.processBlock(buf, midi);
                const double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                t += dt; worst = std::max(worst, dt);
            }
            best = std::min(best, t);
        }
        const double pct = 100.0 * best / 4.0;
        sum += pct; ++n;
        // Quiet: the track stops; after the tail has died the instance sleeps.
        buf.clear();
        for (int i = 0; i < (int)(fs * 12) / block; ++i) { buf.clear(); proc.processBlock(buf, midi); }
        double quiet = 0;
        { const auto t0 = std::chrono::steady_clock::now(); for (int b = 0; b < blocks; ++b) { buf.clear(); proc.processBlock(buf, midi); } quiet = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); }
        std::printf("%-16s %9.2f%% %11.1f%% %9.3f%%\n", cs::modeName(mode), pct, 100.0 * worst / (block / fs), 100.0 * quiet / 4.0);
        proc.releaseResources();
    }
    if (n > 1) std::printf("%-16s %9.2f%%\n", "mean", sum / n);
    return 0;
}
