#pragma once
#include "PluginProcessor.h"
#include "StudioLook.h"

// Resizable, vector-drawn editor. Header with presets, one wide display (decay curve, time view,
// level history, meters), a control plate with the dials, and an optional advanced plate.
class ChromosphereEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit ChromosphereEditor(ChromosphereProcessor&);
    ~ChromosphereEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

    // Used by tests and the snapshot tool.
    void setAdvancedVisible(bool v);

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
    std::unique_ptr<PresetBar> presetBar_;

    juce::ComboBox mode_, quality_, factoryIr_;
    juce::ToggleButton freeze_{"Freeze"}, diffusion_{"Diffusion"}, reverse_{"Reverse IR"}, advanced_{"Advanced"};
    juce::TextButton loadIr_{"Load IR"}, updates_;
    bool updateBadge_ = false;
    juce::Label irName_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt_, qualityAtt_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAtt_, diffusionAtt_, reverseAtt_;
    std::unique_ptr<juce::FileChooser> chooser_;
    juce::Rectangle<int> brandArea_, plate_, advancedPlate_, modeLabel_, qualityLabel_;
    bool advancedVisible_ = false;
    int lastMode_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChromosphereEditor)
};
