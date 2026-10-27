// Per-instance cost of the processor (OPTIMIZATION_GUIDE.md Section 2.1): processBlock with 256-sample blocks at
// 48 kHz, 10 s of audio, for one preset per category playing, and for a silent instance (stopped, nothing playing).
// Prints each as a percentage of one core.
//   ultra_plugin_bench [seconds]
#include "PluginProcessor.h"
#include <chrono>
#include <cstdio>
#include <map>

static double run(UltraProcessor& proc, double seconds) {
    const int block = 256;
    juce::AudioBuffer<float> buf(2, block);
    juce::MidiBuffer midi;
    const int blocks = (int)(seconds * 48000.0 / block);
    for (int i = 0; i < 50; ++i) proc.processBlock(buf, midi);              // warm up
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < blocks; ++i) proc.processBlock(buf, midi);
    const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    return 100.0 * s / seconds;
}

int main(int argc, char** argv) {
#if defined(_WIN32)
    _putenv_s("ULTRA_NO_UPDATE_CHECK", "1");
#else
    setenv("ULTRA_NO_UPDATE_CHECK", "1", 1);
#endif
    juce::ScopedJuceInitialiser_GUI init;
    const double seconds = argc > 1 ? std::atof(argv[1]) : 10.0;
    UltraProcessor proc;
    proc.setPlayConfigDetails(0, 2, 48000.0, 256);
    proc.prepareToPlay(48000.0, 256);
    std::map<juce::String, int> firstOfCategory;
    const auto& ps = proc.presets.presets();
    for (int i = 0; i < (int)ps.size(); ++i) if (ps[(size_t)i].factory && !firstOfCategory.count(ps[(size_t)i].category)) firstOfCategory[ps[(size_t)i].category] = i;
    double sum = 0.0; int n = 0;
    for (const auto& kv : firstOfCategory) {
        proc.presets.apply(kv.second);
        proc.machine.reset();
        proc.setPlaying(true);
        const double pc = run(proc, seconds);
        std::printf("playing  %-20s %-24s %6.2f%%\n", kv.first.toRawUTF8(), ps[(size_t)kv.second].name.toRawUTF8(), pc);
        sum += pc; ++n;
    }
    std::printf("playing  mean of %d presets                        %6.2f%%\n", n, sum / std::max(1, n));
    proc.setPlaying(false);
    proc.presets.apply(proc.presets.indexOf("Init"));
    run(proc, 4.0);                                                         // let tails die out
    std::printf("silent   stopped, nothing playing                  %6.3f%%\n", run(proc, seconds));
    return 0;
}
