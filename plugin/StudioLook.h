#pragma once
// "Studio" look: a restrained, modern hardware aesthetic in the vein of current top-tier plug-ins.
// Near-black matte panel, soft lighting, one warm accent, clean typography, large quiet dials.
// Everything is drawn procedurally; dial bodies are cached per size.
#include <juce_gui_basics/juce_gui_basics.h>

namespace studio {

// Palette.
extern const juce::Colour kBg, kPlate, kPlateEdge, kLine, kText, kTextDim, kTextFaint, kAccent, kAccentDim,
    kDisplayBg, kGrid, kCurve, kCurveFill, kMeter, kMeterHot, kDanger;

// Typography. Falls back to the default sans-serif when the embedded typeface is missing.
juce::Font font(float height, int weight = 400);        // 400 regular, 500 medium, 600 semibold
juce::Font caps(float height);                          // small label style: medium, tracked, meant for uppercase text
void drawCaps(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just,
              float height, juce::Colour colour = kTextDim);

// Surfaces.
void drawPlate(juce::Graphics& g, juce::Rectangle<float> r, float corner = 8.f);     // raised faceplate section
void drawDisplay(juce::Graphics& g, juce::Rectangle<float> r, float corner = 8.f);   // recessed dark display
void drawDivider(juce::Graphics& g, float x0, float y, float x1);
void drawBolt(juce::Graphics& g, float cx, float cy, float r);
// Section header: index code, caps title, and a hairline to the right.
void drawSection(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& code, const juce::String& title);
const juce::Image& backgroundTexture(int w, int h);

// Controls drawn by the look and feel.
class StudioLookAndFeel : public juce::LookAndFeel_V4 {
public:
    StudioLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    juce::Label* createSliderTextBox(juce::Slider&) override;
    void drawLabel(juce::Graphics&, juce::Label&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    juce::Font getTextButtonFont(juce::TextButton&, int) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void drawPopupMenuBackground(juce::Graphics&, int, int) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>&, bool, bool, bool, bool, bool,
                           const juce::String&, const juce::String&, const juce::Drawable*, const juce::Colour*) override;
    juce::Font getPopupMenuFont() override;
    void drawCallOutBoxBackground(juce::CallOutBox&, juce::Graphics&, const juce::Path&, juce::Image&) override;
    juce::Font getAlertWindowMessageFont() override;
    juce::Font getAlertWindowFont() override;
    juce::Font getAlertWindowTitleFont() override;
};

}  // namespace studio
