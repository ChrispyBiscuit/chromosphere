// Per-instance cost of the INFRA processor (optimisation guide, section 2): processBlock with 256-sample blocks at
// 48 kHz for 10 s of audio, as a percentage of real time on one core.
//   infra_plugin_bench [seconds]
// Rows: a silent instance (preset loaded, no notes), and for a spread of presets a held four-note chord.
#include "PluginProcessor.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

static double run(InfraProcessor& proc, double seconds, bool play) {
    const int block = 256; const double fs = 48000.0;
    juce::AudioBuffer<float> buf(2, block);
    const int blocks = (int)(seconds * fs / block);
    // Warm up (and let a note's attack settle), then time.
    auto step = [&](int b) {
        juce::MidiBuffer m;
        if (play && b == 0) for (int n : {48, 55, 60, 64}) m.addEvent(juce::MidiMessage::noteOn(1, n, (juce::uint8)100), 0);
        proc.processBlock(buf, m);
    };
    for (int b = 0; b < 40; ++b) step(b);
    const auto t0 = std::chrono::steady_clock::now();
    for (int b = 40; b < 40 + blocks; ++b) step(b);
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    juce::MidiBuffer off; off.addEvent(juce::MidiMessage::allNotesOff(1), 0); off.addEvent(juce::MidiMessage::allSoundOff(1), 0);
    proc.processBlock(buf, off);
    return 100.0 * secs / seconds;
}

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    const double seconds = argc > 1 ? std::atof(argv[1]) : 10.0;
    // Load cost (guide section 4: no heavy work at load): the first instance builds the shared wavetables, later ones
    // should be cheap.
    {
        std::vector<std::unique_ptr<InfraProcessor>> session;   // instances alive together, as in a DAW session
        for (int k = 0; k < 4; ++k) {
            const auto t0 = std::chrono::steady_clock::now();
            session.push_back(std::make_unique<InfraProcessor>());
            const auto t1 = std::chrono::steady_clock::now();
            session.back()->setPlayConfigDetails(0, 2, 48000.0, 256); session.back()->prepareToPlay(48000.0, 256);
            const auto t2 = std::chrono::steady_clock::now();
            std::printf("instance %d: construct %.1f ms, prepare %.1f ms\n", k + 1, std::chrono::duration<double, std::milli>(t1 - t0).count(),
                        std::chrono::duration<double, std::milli>(t2 - t1).count());
        }
    }
    InfraProcessor proc;
    proc.setPlayConfigDetails(0, 2, 48000.0, 256);
    proc.prepareToPlay(48000.0, 256);
    const char* presets[] = {"Infrared Dawn", "Redshift Reese", "Photon Lead", "Coronal Loop", "Quasar Bells", "Galactic Section",
                             "Dyson Sphere", "Accretion Disk", "Berlin Ascent", "Supersaw Strings of Orion"};
    double sumPlay = 0; int nPlay = 0;
    for (auto* name : presets) {
        const int i = proc.presets.indexOf(name);
        if (i < 0) continue;
        proc.presets.apply(i);
        proc.prepareToPlay(48000.0, 256);          // clears the previous preset's effect tails
        const double silent = run(proc, seconds, false);
        const double playing = run(proc, seconds, true);
        sumPlay += playing; ++nPlay;
        std::printf("%-28s silent %6.3f%%   playing %6.2f%%\n", name, silent, playing);
    }
    std::printf("mean playing %.2f%%\n", sumPlay / std::max(1, nPlay));
    return 0;
}
