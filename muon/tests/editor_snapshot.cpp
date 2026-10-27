// Renders the MUON editor to PNG files so the layout can be checked without a screen.
//   muon_editor_snapshot out_dir
#include "PluginEditor.h"
#include "material.h"
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
    setenv("MUON_NO_UPDATE_CHECK", "1", 1);       // the editor would otherwise run its daily update check
#endif
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File dir(argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]) : juce::File::getCurrentWorkingDirectory());
    dir.createDirectory();
    MuonProcessor proc;
    const double fs = 48000; const int block = 256;
    proc.setPlayConfigDetails(2, 2, fs, block);
    proc.prepareToPlay(fs, block);

    struct Shot { const char* preset; const char* material; int page; int band; const char* file; int w, h; };
    const Shot shots[] = {
        {"Punch Bus", "drums", 0, 0, "shape.png", 1180, 760},
        {"Bass Weight", "bass", 1, 0, "harmonics.png", 1180, 760},
        {"Tuned Impact", "kick", 2, 0, "resonance.png", 1180, 760},
        {"Master Warmth", "mix", 3, 0, "output.png", 1180, 760},
        {"Crack Front", "snare", 1, 2, "small.png", 1000, 680},
        {"Cosmic Ray", "drums", 2, 0, "large.png", 1700, 1100},
    };
    for (const auto& s : shots) {
        proc.presets.apply(proc.presets.indexOf(s.preset));
        const auto mat = material::make(s.material);
        std::unique_ptr<MuonEditor> ed(new MuonEditor(proc));
        ed->forceCpuSun();
        ed->showPage(s.page);
        ed->setSize(s.w, s.h);
        if (auto* hp = dynamic_cast<panels::HarmonicsPage*>(ed->getChildComponent(0)); hp) (void)hp;
        // Play the material in real time-ish blocks so the scope, meters and track view fill.
        juce::AudioBuffer<float> buf(2, block);
        size_t pos = 0;
        for (int b = 0; b < (int)(3.04 * fs / block); ++b) {          // ends just after a beat, so the analyser has signal
            for (int i = 0; i < block; ++i, ++pos) { buf.setSample(0, i, mat.L[pos % mat.L.size()]); buf.setSample(1, i, mat.R[pos % mat.R.size()]); }
            juce::MidiBuffer midi;
            proc.processBlock(buf, midi);
            if (b % 8 == 0) { ed->renderSunNow(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
        }
        ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);   // first paint: the VFD combos pick up their text
        for (int i = 0; i < 30; ++i) ed->renderSunNow();
        ed->resized();
        save(ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f), dir.getChildFile(s.file));
    }
    return 0;
}
