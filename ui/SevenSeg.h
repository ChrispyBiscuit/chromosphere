#pragma once
// A classic red LED seven-segment readout, drawn as the real part looks: slanted hexagonal
// segments behind a dark red filter window, the unlit segments faintly there, lit segments
// brighter in the middle of their light pipe than at the tips, a soft bloom through the epoxy,
// a glossy reflection on the face. Digits, minus and decimal point on the display; the unit is
// printed beside the window as silkscreen, since a seven-segment cannot spell it.
#include <juce_gui_basics/juce_gui_basics.h>

namespace seg {

extern const juce::Colour kLed;   // lit segment colour

// Draw a readout for `text` ("-12.0 dB", "8.00 s", "35.0 %") into r.
void draw(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, bool enabled);

// Slider text box: the readout; while editing, the normal text editor appears.
class ValueLabel : public juce::Label {
public:
    ValueLabel();
    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override {}
    void textWasChanged() override { cache_ = juce::Image(); repaint(); }
    void enablementChanged() override { cache_ = juce::Image(); repaint(); }
    void resized() override { juce::Label::resized(); cache_ = juce::Image(); }
private:
    juce::Image cache_;
};

}  // namespace seg
