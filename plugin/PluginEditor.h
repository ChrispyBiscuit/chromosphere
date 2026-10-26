#pragma once
#include "PluginProcessor.h"
#include "StudioLook.h"
#include "Sun.h"
namespace wordmark { class Wordmark; }

// Resizable, vector-drawn editor. Header with presets, one wide display (decay curve, time view,
// level history, meters), a control plate with the dials, and an optional advanced plate.
class ChromosphereEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit ChromosphereEditor(ChromosphereProcessor&, bool allowOpenGL = true);
    ~ChromosphereEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

    // Used by tests and the snapshot tool.
    void setAdvancedVisible(bool v);
    void forceCpuSun();
    void renderSunNow();
    void crackGlassForTest();
    static float sunLevel(ChromosphereProcessor&);   // 0..1 energy from the reverb level
    static sun::Drive sunDrive(ChromosphereProcessor&);   // level and bass

    class Knob;
    class MainDisplay;
    class PresetBar;

private:
    void timerCallback() override;
    void updateEnabledStates();
    void chooseIr();

    ChromosphereProcessor& proc_;
    studio::StudioLookAndFeel look_;
    std::vector<std::unique_ptr<Knob>> knobs_;
    std::unique_ptr<MainDisplay> display_;
    std::unique_ptr<sun::SunView> emblem_;
    std::unique_ptr<wordmark::Wordmark> wordmark_;
    std::unique_ptr<PresetBar> presetBar_;

    juce::ComboBox mode_, quality_, factoryIr_;
    juce::ToggleButton freeze_{"Freeze"}, diffusion_{"Diffusion"}, reverse_{"Reverse IR"}, advanced_{"Advanced"};
    juce::TextButton loadIr_{"Load IR"}, updates_;
    bool updateBadge_ = false;
    juce::Label irName_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt_, qualityAtt_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAtt_, diffusionAtt_, reverseAtt_;
    std::unique_ptr<juce::FileChooser> chooser_;
    juce::Rectangle<int> brandArea_, plate_, advancedPlate_, modeLabel_, qualityLabel_, sectionMain_, sectionCosmic_, sectionAdv_;
    bool cosmicNow_ = false;
    bool advancedVisible_ = false, allowOpenGL_ = true;
    float bassSmooth_ = 0.f;
    int lastMode_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChromosphereEditor)
};
