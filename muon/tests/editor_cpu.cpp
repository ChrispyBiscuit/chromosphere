// Editor cost (optimization guide 2.2): opens the MUON editor in a real window and reads this process's CPU time
// from /proc/self/stat over 15 s, idle and then with audio running on a separate thread at real time. Linux only
// (run under xvfb-run). Prints percent of one core; the guide's target for an idle editor is under 10%.
#include "PluginEditor.h"
#include "material.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <thread>

static double cpuSeconds(bool messageThreadOnly = false) {
    std::ifstream f(messageThreadOnly ? "/proc/self/task/" + std::to_string(getpid()) + "/stat" : std::string("/proc/self/stat"));
    std::string s; std::getline(f, s);
    const auto close = s.rfind(')');
    std::istringstream rest(s.substr(close + 2));
    std::string tok; long utime = 0, stime = 0;
    for (int i = 3; i <= 15; ++i) { rest >> tok; if (i == 14) utime = std::stol(tok); if (i == 15) stime = std::stol(tok); }
    return (double)(utime + stime) / (double)sysconf(_SC_CLK_TCK);
}

int main(int argc, char** argv) {
    setenv("MUON_NO_UPDATE_CHECK", "1", 1);
    const double seconds = argc > 1 ? std::atof(argv[1]) : 15.0;
    juce::ScopedJuceInitialiser_GUI init;
    MuonProcessor proc;
    proc.setPlayConfigDetails(2, 2, 48000.0, 256);
    proc.prepareToPlay(48000.0, 256);
    proc.presets.apply(proc.presets.indexOf("Punch Bus"));
    std::unique_ptr<MuonEditor> ed(new MuonEditor(proc));
    ed->addToDesktop(0);                          // no title bar: xvfb has no window manager
    ed->setVisible(true);
    // Phases run inside JUCE's blocking dispatch loop (as in a host), sequenced by a timer: settle, idle, playing,
    // minimised. runDispatchLoopUntil would poll with 1 ms sleeps and inflate the numbers.
    std::atomic<bool> run{true}, playing{false};
    double audioBusy = 0.0; long audioBlocks = 0;
    std::thread audio([&] {
        const auto mat = material::make("drums");
        juce::AudioBuffer<float> buf(2, 256); size_t pos = 0;
        auto next = std::chrono::steady_clock::now();
        while (run) {
            next += std::chrono::microseconds((long long)(256.0 / 48000.0 * 1e6));
            if (playing) {
                for (int i = 0; i < 256; ++i, ++pos) { buf.setSample(0, i, mat.L[pos % mat.L.size()]); buf.setSample(1, i, mat.R[pos % mat.R.size()]); }
                juce::MidiBuffer m;
                const auto t0 = std::chrono::steady_clock::now();
                proc.processBlock(buf, m);
                audioBusy += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); ++audioBlocks;
            }
            std::this_thread::sleep_until(next);
        }
    });
    struct Phases : juce::Timer {
        std::function<void(int)> step; int n = 0;
        void timerCallback() override { step(n++); }
    } phases;
    double c0 = 0, m0 = 0, t0 = 0, idle = 0, play = 0, mini = 0, idleMsg = 0, playMsg = 0;
    auto begin = [&] { c0 = cpuSeconds(); m0 = cpuSeconds(true); t0 = juce::Time::getMillisecondCounterHiRes(); };
    auto endMsg = [&] { return 100.0 * (cpuSeconds(true) - m0) / ((juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0); };
    auto end = [&] { return 100.0 * (cpuSeconds() - c0) / ((juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0); };
    phases.step = [&](int k) {
        if (k == 0) { begin(); return; }                                    // after settling
        if (k == 1) { idleMsg = endMsg(); idle = end(); playing = true; juce::Thread::sleep(1500); begin(); return; }
        if (k == 2) { playMsg = endMsg(); play = end(); playing = false; if (auto* peer = ed->getPeer()) peer->setMinimised(true); juce::Thread::sleep(500); begin(); return; }
        mini = end(); phases.stopTimer(); juce::MessageManager::getInstance()->stopDispatchLoop();
    };
    juce::Timer::callAfterDelay(1500, [&] { phases.step(phases.n++); phases.startTimer((int)(seconds * 1000)); });
    juce::MessageManager::getInstance()->runDispatchLoop();
    run = false; audio.join();
    const double audioCpu = audioBlocks ? 100.0 * audioBusy / (audioBlocks * 256.0 / 48000.0) * (play > 0 ? 1.0 : 0.0) : 0.0;
    std::printf("%-34s %6.1f%% of one core (message thread %.1f%%)\n", "editor open, idle", idle, idleMsg);
    std::printf("%-34s %6.1f%% of one core (message thread %.1f%%)\n", "editor open, audio playing (total)", play, playMsg);
    std::printf("%-34s %6.1f%% of one core\n", "  of which the audio thread", audioCpu);
    std::printf("%-34s %6.1f%% of one core\n", "  editor (playing minus audio)", play - audioCpu);
    const bool minimised = ed->getPeer() != nullptr && ed->getPeer()->isMinimised();
    std::printf("%-34s %6.1f%% of one core%s\n", "window minimised", mini, minimised ? "" : "  (not minimised: no window manager)");
    ed->removeFromDesktop();
    ed.reset();
    return idle < 10.0 ? 0 : 1;
}
