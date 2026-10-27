#pragma once
#include "Look.h"
#include "MuonTheme.h"
#include "Panels.h"
#include "PluginProcessor.h"
#include "Sun.h"
#include "Wordmark.h"

// The MUON console: header (live sun, wordmark, status, presets, Updates, Ignite), the main display over the blue
// limb, the macro bay, and four pages (SHAPE, HARMONICS, RESONANCE, OUTPUT).
class MuonEditor : private look::ThemeGuard, public juce::AudioProcessorEditor, private juce::Timer, private juce::ChangeListener {
public:
    explicit MuonEditor(MuonProcessor&);
    ~MuonEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

    void showPage(int index);
    int currentPage() const { return page_; }
    // Tests and snapshots: render the suns and pull the display data on this thread.
    void forceCpuSun();
    void renderSunNow();
    void ignite(int source);

    static float sunLevel(MuonProcessor& p);
    static sun::Drive sunDrive(MuonProcessor& p);

private:
    class PresetBar;
    class Tabs;
    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;   // the updater's state

    MuonProcessor& proc_;
    look::PanelLookAndFeel look_;
    std::unique_ptr<sun::SunView> emblem_;
    std::unique_ptr<wordmark::Wordmark> wordmark_;
    std::unique_ptr<PresetBar> presetBar_;
    juce::TextButton ignite_, updates_;
    bool updateBadge_ = false;
    std::unique_ptr<panels::MainDisplay> display_;
    std::unique_ptr<panels::MacroBay> macros_;
    std::unique_ptr<Tabs> tabs_;
    std::vector<std::unique_ptr<panels::Page>> pages_;
    juce::Random rng_;
    int page_ = 0;
    float bassSmooth_ = 0.f;
    bool animating_ = true;
    juce::Rectangle<int> brandArea_, displayPlate_, macroPlate_, mainPlate_;
    juce::String lastCaption_, lastStatus_;
    juce::Image chrome_;                          // background texture and plates, drawn once per size
    juce::String statusText() const;
    juce::Rectangle<int> statusArea() const;
};
