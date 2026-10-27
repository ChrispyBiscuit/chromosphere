#pragma once
// Parameter-bound controls in the family style, shared by the Chromosphere editors: a dial with its caps title and
// seven-segment readout, a VFD combo box with a caps title, and a lever switch.
#include "Look.h"
#include "Vfd.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace controls {

using APVTS = juce::AudioProcessorValueTreeState;

// A dial with its caps label above and an inset readout below (the EXO knob).
class Knob : public juce::Component {
public:
    enum Size { Small, Normal, Large };
    Knob(APVTS& s, const juce::String& id, const juce::String& title, Size size = Normal, bool bipolar = false);
    void paint(juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;
    juce::String paramId;
private:
    std::unique_ptr<APVTS::SliderAttachment> att_;
    juce::String title_;
    Size size_;
    juce::Rectangle<int> label_;
};

// A combo box bound to a choice parameter, with a caps label above.
class Choice : public juce::Component {
public:
    Choice(APVTS& s, const juce::String& id, const juce::String& title);
    void paint(juce::Graphics&) override;
    void resized() override;
    vfd::ComboBox box;
private:
    std::unique_ptr<APVTS::ComboBoxAttachment> att_;
    juce::String title_;
};

// A lever switch bound to a bool parameter.
class Toggle : public juce::Component {
public:
    Toggle(APVTS& s, const juce::String& id, const juce::String& title);
    void resized() override { button.setBounds(getLocalBounds()); }
    juce::ToggleButton button;
private:
    std::unique_ptr<APVTS::ButtonAttachment> att_;
};

}  // namespace controls
