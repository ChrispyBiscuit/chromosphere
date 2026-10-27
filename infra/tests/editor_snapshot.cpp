// Renders the INFRA editor to PNG files so the layout can be checked without a screen.
//   infra_editor_snapshot out_dir
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
    setenv("INFRA_NO_UPDATE_CHECK", "1", 1);      // the editor would otherwise run its daily update check
#endif
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File dir(argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]) : juce::File::getCurrentWorkingDirectory());
    dir.createDirectory();
    InfraProcessor proc;
    const double fs = 48000; const int block = 512;
    proc.setPlayConfigDetails(0, 2, fs, block);
    proc.prepareToPlay(fs, block);

    // Play a held chord so the scope, meters, sun and step lights have something to show.
    auto play = [&](double seconds) {
        juce::AudioBuffer<float> buf(2, block);
        const int blocks = (int)(seconds * fs / block);
        for (int b = 0; b < blocks; ++b) {
            juce::MidiBuffer midi;
            if (b == 0) for (int n : {45, 52, 57, 60, 64}) midi.addEvent(juce::MidiMessage::noteOn(1, n, (juce::uint8)100), 0);
            proc.processBlock(buf, midi);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    };
    auto release = [&] {
        juce::AudioBuffer<float> buf(2, block); juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::allNotesOff(1), 0);
        proc.processBlock(buf, midi);
    };

    struct Shot { const char* preset; int page; const char* file; int w, h; };
    const Shot shots[] = {
        {"Infrared Dawn", 0, "synth.png", 1280, 900},
        {"Coronal Loop", 2, "arp.png", 1280, 900},
        {"Photon Lead", 1, "mod.png", 1280, 900},
        {"Dyson Sphere", 3, "fx.png", 1280, 900},
        {"Quasar Bells", 0, "small.png", 1180, 820},
        {"Redshift Reese", 0, "large.png", 1700, 1150},
    };
    for (const auto& s : shots) {
        proc.presets.apply(proc.presets.indexOf(s.preset));
        std::unique_ptr<InfraEditor> ed(new InfraEditor(proc));
        ed->forceCpuSun();
        ed->showPage(s.page);
        ed->setSize(s.w, s.h);
        play(2.0);
        ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);   // first paint: the VFD combos pick up their text
        for (int i = 0; i < 60; ++i) { ed->renderSunNow(); std::this_thread::sleep_for(std::chrono::milliseconds(16)); }
        ed->resized();
        save(ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f), dir.getChildFile(s.file));
        release();
    }
    return 0;
}
