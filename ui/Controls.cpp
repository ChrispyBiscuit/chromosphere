#include "Controls.h"

using namespace look;

namespace controls {

Knob::Knob(APVTS& s, const juce::String& id, const juce::String& title, Size size, bool bipolar) : paramId(id), title_(title), size_(size) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 18);
    slider.setTitle(title); slider.setDescription(title);
    slider.getProperties().set("bipolar", bipolar);
    auto* prm = s.getParameter(id);
    jassert(prm != nullptr);
    slider.setDoubleClickReturnValue(true, prm->convertFrom0to1(prm->getDefaultValue()));
    addAndMakeVisible(slider);
    att_ = std::make_unique<APVTS::SliderAttachment>(s, id, slider);
    setWantsKeyboardFocus(false);
}

void Knob::paint(juce::Graphics& g) {
    const float h = size_ == Large ? 11.5f : size_ == Small ? 9.f : 10.f;
    drawCaps(g, title_, label_.toFloat(), juce::Justification::centred, h, slider.isEnabled() ? (size_ == Large ? kText : kTextDim) : kTextFaint);
}

void Knob::resized() {
    // Title, dial, a band of air, then the seven-segment readout (EXO); the air narrows on dense pages.
    const int nameH = size_ == Large ? 17 : size_ == Small ? 13 : 15;
    const int textH = size_ == Large ? 22 : size_ == Small ? 17 : 18;
    const int gap = size_ == Large ? look::kDialReadoutGap : size_ == Small ? 5 : 7;
    slider.getProperties().set("readoutGap", gap);
    const int maxSide = size_ == Large ? 84 : size_ == Small ? 46 : 64;
    const int side = std::clamp(std::min(getWidth() - 6, getHeight() - nameH - textH - gap - 4), 24, maxSide);
    const int groupH = nameH + 2 + side + gap + textH, top = std::max(0, (getHeight() - groupH) / 2);
    label_ = {0, top, getWidth(), nameH};
    const int boxW = std::min(getWidth() - 2, 124), sw = std::max(side + 16, boxW + 2);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, boxW, textH);
    slider.setBounds((getWidth() - sw) / 2, top + nameH + 2, sw, side + gap + textH);
}

Choice::Choice(APVTS& s, const juce::String& id, const juce::String& title) : title_(title) {
    auto* prm = dynamic_cast<juce::AudioParameterChoice*>(s.getParameter(id));
    jassert(prm != nullptr);
    box.addItemList(prm->choices, 1);
    box.setTitle(title);
    addAndMakeVisible(box);
    att_ = std::make_unique<APVTS::ComboBoxAttachment>(s, id, box);
}
void Choice::paint(juce::Graphics& g) {
    if (title_.isNotEmpty()) drawCaps(g, title_, getLocalBounds().removeFromTop(14).toFloat(), juce::Justification::centredLeft, 9.f, box.isEnabled() ? kTextFaint : kTextFaint.withAlpha(0.5f));
}
void Choice::resized() {
    auto r = getLocalBounds();
    if (title_.isNotEmpty()) r.removeFromTop(15);
    box.setBounds(r.withSizeKeepingCentre(r.getWidth(), std::min(r.getHeight(), 26)));
}

Toggle::Toggle(APVTS& s, const juce::String& id, const juce::String& title) {
    button.setButtonText(title); button.setTitle(title);
    addAndMakeVisible(button);
    att_ = std::make_unique<APVTS::ButtonAttachment>(s, id, button);
}

}  // namespace controls
