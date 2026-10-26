// Renders the editor to PNG files so the layout can be checked without a screen.
//   fr_editor_snapshot out_dir
#include "PluginEditor.h"
#include "RetroLook.h"
#include <cstdio>
#include <thread>

static void save(const juce::Image& img, const juce::File& f) {
    juce::PNGImageFormat png;
    f.deleteFile();
    juce::FileOutputStream out(f);
    if (out.openedOk()) png.writeImageToStream(img, out);
    std::printf("wrote %s (%dx%d)\n", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
}

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File dir(argc > 1 ? argv[1] : ".");
    dir.createDirectory();
    FancyReverbProcessor proc;
    const double fs = 48000; const int block = 512;
    proc.setPlayConfigDetails(2, 2, fs, block);
    proc.prepareToPlay(fs, block);

    auto feed = [&](double seconds) {   // pink-ish bursts so the meters and envelope have something to show
        juce::AudioBuffer<float> buf(2, block); juce::MidiBuffer midi; juce::Random rnd(3);
        const int blocks = (int)(seconds * fs / block);
        for (int b = 0; b < blocks; ++b) {
            const bool burst = (b % 40) < 6;
            for (int c = 0; c < 2; ++c) for (int i = 0; i < block; ++i) buf.setSample(c, i, burst ? (rnd.nextFloat() - 0.5f) * 0.8f : 0.f);
            proc.processBlock(buf, midi);
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    };

    struct Shot { const char* preset; const char* file; bool advanced; int w, h; };
    const Shot shots[] = {
        {"Concert Hall", "algorithmic.png", false, 980, 640},
        {"Studio Hall Hybrid", "hybrid_advanced.png", true, 980, 760},
        {"Small Wood Room", "small.png", false, 840, 540},
        {"Cathedral", "large.png", false, 1500, 980},
    };
    for (const auto& s : shots) {
        proc.presets.apply(proc.presets.indexOf(s.preset));
        std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
        auto* e = dynamic_cast<FancyReverbEditor*>(ed.get());
        e->setAdvancedVisible(s.advanced);
        ed->setSize(s.w, s.h);
        feed(4.0);
        save(ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f), dir.getChildFile(s.file));
    }
    // Segment-font sheet, for checking the 14-segment rendering by eye.
    {
        juce::Image im(juce::Image::RGB, 760, 200, true);
        juce::Graphics g(im);
        g.fillAll(retro::kPanel);
        auto r = retro::drawVfdGlass(g, {8.f, 8.f, 744.f, 184.f});
        retro::drawSegText(g, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", r.removeFromTop(44.f), juce::Justification::centredLeft);
        retro::drawSegText(g, "0123456789 -+/()%*", r.removeFromTop(44.f), juce::Justification::centredLeft);
        retro::drawSegText(g, "1.800  2.90 S  -7.0 DB  10.63 S  0.5", r.removeFromTop(44.f), juce::Justification::centredLeft);
        retro::drawSegText(g, "1.800", r.removeFromTop(24.f).withWidth(72.f), juce::Justification::centred, retro::kVfd, true, 20.f);
        save(im, dir.getChildFile("segfont.png"));
    }
    return 0;
}
