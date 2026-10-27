#pragma once
#include "UltraLook.h"
#include "Panels.h"
#include "PluginProcessor.h"
#include "Sun.h"
#include "Wordmark.h"

// The ULTRA console: header (live sun, wordmark, status, presets, Ignite), the main display over the ultraviolet
// limb, the macro bay, the page tabs with the transport, four pages (SEQ, VOICE, MIX, FX) and the pads.
class UltraEditor : public juce::AudioProcessorEditor, private juce::Timer, private juce::ChangeListener {
public:
    explicit UltraEditor(UltraProcessor&);
    ~UltraEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

    void showPage(int index);
    int currentPage() const { return page_; }
    void selectTrack(int t) { ui_.select(t); }
    // Tests and snapshots: render the suns on this thread.
    void forceCpuSun();
    void renderSunNow();
    void ignite(int genre);
    void tickNow() { tick(); }                      // snapshots: one UI frame, on screen or not

    static float sunLevel(UltraProcessor& p);
    static sun::Drive sunDrive(UltraProcessor& p);

private:
    class PresetBar;
    class Tabs;
    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;   // the shared updater changed
    bool windowVisible() const;
    void tick();
    void setAnimating(bool on);
    bool animating_ = true;

    UltraProcessor& proc_;
    look::ResourceScope resources_;              // shared typefaces and image caches: before anything that draws
    look::UltraLookAndFeel look_;
    panels::UiState ui_;
    std::unique_ptr<sun::SunView> emblem_;
    std::unique_ptr<wordmark::Wordmark> wordmark_;
    std::unique_ptr<PresetBar> presetBar_;
    juce::TextButton ignite_, updates_;
    bool updateBadge_ = false;
    std::unique_ptr<panels::MainDisplay> display_;
    std::unique_ptr<panels::MacroBay> macros_;
    std::unique_ptr<Tabs> tabs_;
    std::unique_ptr<panels::Transport> transport_;
    std::vector<std::unique_ptr<panels::Page>> pages_;
    std::unique_ptr<panels::Pads> pads_;
    juce::Random rng_;
    int page_ = 0;
    float bassSmooth_ = 0.f;
    juce::Rectangle<int> brandArea_, displayPlate_, macroPlate_, mainPlate_, padArea_;
    juce::String lastCaption_, lastStatus_;
    juce::Image chrome_;                          // background texture and plates, drawn once per size
    juce::String statusText() const;
    juce::Rectangle<int> statusArea() const;
};
