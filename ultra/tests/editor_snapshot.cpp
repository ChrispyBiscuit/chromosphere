// Renders the ULTRA editor to PNG files so the layout can be checked without a screen.
//   ultra_editor_snapshot out_dir
#include "PluginEditor.h"
#include <cstdio>
#include <cstdlib>
#include <thread>

static void save(const juce::Image& img, const juce::File& f) {
    juce::PNGImageFormat png;
    f.deleteFile();
    juce::FileOutputStream out(f);
    if (out.openedOk()) png.writeImageToStream(img, out);
    std::printf("wrote %s (%dx%d)\n", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
}

int main(int argc, char** argv) {
#if !defined(_WIN32)
    setenv("ULTRA_NO_UPDATE_CHECK", "1", 1);      // the editor would otherwise run its daily update check
#endif
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File dir(argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]) : juce::File::getCurrentWorkingDirectory());
    dir.createDirectory();
    UltraProcessor proc;
    const double fs = 48000; const int block = 512;
    proc.setPlayConfigDetails(0, 2, fs, block);
    proc.prepareToPlay(fs, block);

    // Run the sequencer so the radar, the scope, the meters, the pads and the sun have something to show.
    auto play = [&](double seconds) {
        juce::AudioBuffer<float> buf(2, block);
        const int blocks = (int)(seconds * fs / block);
        proc.setPlaying(true);
        for (int b = 0; b < blocks; ++b) {
            juce::MidiBuffer midi;
            proc.processBlock(buf, midi);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    };

    struct Shot { const char* preset; int page; int track; const char* file; int w, h; };
    const Shot shots[] = {
        {"Init", 0, 0, "seq.png", 1280, 900},
        {"Init", 1, 1, "voice.png", 1280, 900},
        {"Init", 2, 2, "mix.png", 1280, 900},
        {"Init", 3, 0, "fx.png", 1280, 900},
        {"Init", 0, 3, "small.png", 1180, 820},
        {"Init", 1, 4, "large.png", 1700, 1150},
    };
    for (const auto& s : shots) {
        juce::String preset = s.preset;
        if (argc > 2) preset = argv[2];
        proc.presets.apply(std::max(0, proc.presets.indexOf(preset)));
        std::unique_ptr<UltraEditor> ed(new UltraEditor(proc));
        ed->forceCpuSun();
        ed->showPage(s.page);
        ed->selectTrack(s.track);
        ed->setSize(s.w, s.h);
        play(2.5);
        ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);   // first paint: the VFD combos pick up their text
        for (int i = 0; i < 60; ++i) { ed->renderSunNow(); std::this_thread::sleep_for(std::chrono::milliseconds(16)); }
        ed->resized();
        for (int i = 0; i < 4; ++i) ed->tickNow();
        save(ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f), dir.getChildFile(s.file));
        proc.setPlaying(false);
    }
    return 0;
}
