#pragma once
#include "InfraLook.h"
#include "Panels.h"
#include "PluginProcessor.h"
#include "Sun.h"
#include "Wordmark.h"
#include <juce_audio_utils/juce_audio_utils.h>

// The INFRA console: header (live sun, wordmark, status, presets, Ignite), the main display over the infrared
// limb, the macro bay, four pages (SYNTH, MOD, ARP, FX) and a keyboard.
class InfraEditor : public juce::AudioProcessorEditor, private juce::Timer, private juce::ChangeListener {
public:
    explicit InfraEditor(InfraProcessor&);
    ~InfraEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

    void showPage(int index);
    int currentPage() const { return page_; }
    // Tests and snapshots: render the suns on this thread.
    void forceCpuSun();
    void renderSunNow();
    void ignite(int archetype);

    static float sunLevel(InfraProcessor& p);
    static sun::Drive sunDrive(InfraProcessor& p);

private:
    class PresetBar;
    class Tabs;
    void timerCallback() override;
    void changeListenerCallback(juce::ChangeBroadcaster*) override;   // updater state (header button)

    InfraProcessor& proc_;
    juce::SharedResourcePointer<look::Assets> assets_;   // fonts and image caches; first, so everything below can use them
    look::InfraLookAndFeel look_;
    std::unique_ptr<sun::SunView> emblem_;
    std::unique_ptr<wordmark::Wordmark> wordmark_;
    std::unique_ptr<PresetBar> presetBar_;
    juce::TextButton ignite_, updates_;
    bool updateBadge_ = false;
    std::unique_ptr<panels::MainDisplay> display_;
    std::unique_ptr<panels::MacroBay> macros_;
    std::unique_ptr<Tabs> tabs_;
    std::vector<std::unique_ptr<panels::Page>> pages_;
    juce::MidiKeyboardComponent keyboard_;
    juce::Random rng_;
    int page_ = 0;
    float bassSmooth_ = 0.f;
    juce::Rectangle<int> brandArea_, displayPlate_, macroPlate_, mainPlate_, keyboardArea_;
    juce::String lastCaption_, lastStatus_;
    juce::Image chrome_;                          // background texture and plates, drawn once per size
    juce::String statusText() const;
    juce::Rectangle<int> statusArea() const;
};
