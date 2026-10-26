#pragma once
#include "PluginProcessor.h"
#include "RetroLook.h"

// Resizable, vector-drawn editor in a retro hardware style: brushed-metal panel, analog dials,
// VFD readouts and graphs. Preset bar, three live displays (decay curve, level history, early
// reflections / IR), primary and secondary controls, an advanced panel, and meters.
class FancyReverbEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit FancyReverbEditor(FancyReverbProcessor&);
    ~FancyReverbEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

    // Used by tests and the snapshot tool.
    void setAdvancedVisible(bool v);

    class Knob;
    class DecayView;
    class EnvelopeView;
    class ErView;
    class MeterPanel;
    class PresetBar;

private:
    void timerCallback() override;
    void updateEnabledStates();
    void chooseIr();

    FancyReverbProcessor& proc_;
    retro::RetroLookAndFeel look_;
    std::vector<std::unique_ptr<Knob>> knobs_;
    std::unique_ptr<DecayView> decay_;
    std::unique_ptr<EnvelopeView> env_;
    std::unique_ptr<ErView> er_;
    std::unique_ptr<MeterPanel> meters_;
    std::unique_ptr<PresetBar> presetBar_;

    juce::ComboBox mode_, quality_, factoryIr_;
    juce::ToggleButton freeze_{"Freeze"}, diffusion_{"Diffusion"}, reverse_{"Reverse"}, advanced_{"Advanced"};
    juce::TextButton loadIr_{"Load IR"};
    juce::Label irName_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt_, qualityAtt_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAtt_, diffusionAtt_, reverseAtt_;
    std::unique_ptr<juce::FileChooser> chooser_;
    juce::Rectangle<int> brandArea_, decayTitle_, envTitle_, erTitle_, modeLabel_, qualityLabel_;
    bool advancedVisible_ = false;
    int lastMode_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FancyReverbEditor)
};
