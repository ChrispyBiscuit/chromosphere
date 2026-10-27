// Renders the INFRA editor to PNG files so the layout can be checked without a screen.
//   infra_editor_snapshot out_dir
#include "PluginEditor.h"
#include "synth_voice.h"
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
    proc.setPlayConfigDetails(1, 2, fs, block);
    proc.prepareToPlay(fs, block);

    // A sung phrase for the WAVE page: a few notes with vibrato, a glide and a re-sung note.
    std::vector<wavetest::Note> phrase;
    {
        const float mel[] = {57, 60, 62, 64, 62, 60, 59, 57};
        double t = 0.05;
        for (int i = 0; i < 8; ++i) {
            wavetest::Note n; n.start = t; n.dur = i == 3 ? 0.55 : 0.32; n.midi = mel[i]; n.vibCents = i == 3 ? 35.f : 12.f; n.vowel = i % 5;
            if (i == 6) { n.midiEnd = 55.f; n.glideAt = 0.18; }
            phrase.push_back(n); t += n.dur + 0.07;
        }
    }
    const auto voice = wavetest::render(phrase, 4.0, fs);
    auto setParam = [&](const char* id, float plain) { if (auto* prm = proc.apvts.getParameter(id)) prm->setValueNotifyingHost(prm->convertTo0to1(plain)); };

    // Play a held chord (or, with WAVE on, sing) so the scope, meters, sun, step lights and voice trace have something to show.
    auto play = [&](double seconds) {
        juce::AudioBuffer<float> buf(2, block);
        const int blocks = (int)(seconds * fs / block);
        const bool sing = proc.apvts.getRawParameterValue("wvOn")->load() > 0.5f;
        for (int b = 0; b < blocks; ++b) {
            juce::MidiBuffer midi;
            buf.clear();
            if (sing) { if ((size_t)(b + 1) * block <= voice.size()) buf.copyFrom(0, 0, voice.data() + (size_t)b * block, block); }
            else if (b == 0) for (int n : {45, 52, 57, 60, 64}) midi.addEvent(juce::MidiMessage::noteOn(1, n, (juce::uint8)100), 0);
            proc.processBlock(buf, midi);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    };
    auto release = [&] {
        juce::AudioBuffer<float> buf(2, block); juce::MidiBuffer midi;
        buf.clear();
        midi.addEvent(juce::MidiMessage::allNotesOff(1), 0);
        proc.processBlock(buf, midi);
    };

    struct Shot { const char* preset; int page; const char* file; int w, h; int wave = 0; };   // wave: 0 off, 1 melody, 2 chords
    const Shot shots[] = {
        {"Infrared Dawn", 0, "synth.png", 1280, 900},
        {"Coronal Loop", 2, "arp.png", 1280, 900},
        {"Photon Lead", 1, "mod.png", 1280, 900},
        {"Dyson Sphere", 3, "fx.png", 1280, 900},
        {"Quasar Bells", 0, "small.png", 1180, 820},
        {"Redshift Reese", 0, "large.png", 1700, 1150},
        {"Photon Lead", 4, "wave.png", 1280, 900, 1},
        {"Infrared Dawn", 4, "wave_chords.png", 1280, 900, 2},
        {"Infrared Dawn", 4, "wave_small.png", 1180, 820, 2},
        {"Infrared Dawn", 4, "wave_off.png", 1280, 900, 0},
    };
    for (const auto& s : shots) {
        proc.presets.apply(proc.presets.indexOf(s.preset));
        setParam("wvOn", s.wave > 0 ? 1.f : 0.f);
        setParam("wvMode", s.wave == 2 ? 1.f : 0.f);
        setParam("wvScale", s.wave > 0 ? 1.f : 0.f);          // C major
        setParam("wvFollow", s.wave == 1 ? 0.6f : 0.f);
        setParam("wvVoicing", 2.f);
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
