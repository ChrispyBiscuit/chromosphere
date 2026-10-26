#pragma once
// Retro hardware look: brushed-metal panel, analog dials, and VFD (vacuum fluorescent display)
// style readouts with 14-segment lettering. Everything is drawn procedurally, so it is sharp at
// any size and needs no image assets.
#include <juce_gui_basics/juce_gui_basics.h>

namespace retro {

// Colours.
extern const juce::Colour kPanel, kPanelLight, kPanelDark, kSilk, kSilkDim, kEngrave,
    kVfd, kVfdDim, kVfdGlass, kVfdGlassTop, kVfdBezel, kAmber, kRed, kIvory, kChrome, kChromeDark;

// ---- Panel -------------------------------------------------------------------------------------
// Brushed dark metal with a vignette, cached per size.
const juce::Image& panelTexture(int w, int h);
void drawScrew(juce::Graphics& g, float cx, float cy, float r, float slotAngle);
// Silk-screen style label: uppercase, letter-spaced, with a dark offset so it reads as printed/engraved.
void drawSilk(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just,
              float fontHeight, juce::Colour colour = kSilk, bool bold = true);

// ---- VFD ---------------------------------------------------------------------------------------
// Dark glass with bezel, dot grid, and a top reflection. Returns the inner (glass) area.
// bezel: frame thickness; pad: inset of the returned drawing area inside the glass.
juce::Rectangle<float> drawVfdGlass(juce::Graphics& g, juce::Rectangle<float> r, float corner = 5.f, float bezel = 3.f, float pad = 4.f);
// Strokes a path with a phosphor glow (three passes).
void glowPath(juce::Graphics& g, const juce::Path& p, juce::Colour c, float width, float glowScale = 1.f);
void glowDot(juce::Graphics& g, float x, float y, float r, juce::Colour c);
// 14-segment text. Unknown characters show as blanks; '.' attaches as a decimal point to the previous
// cell. `ghost` draws the unlit segments faintly, as a real VFD does. Returns the width used.
float drawSegText(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just,
                  juce::Colour c = kVfd, bool ghost = true, float maxCellHeight = 1e9f);
// Width of one 14-segment cell for a given cell height.
float segCellWidth(float cellHeight);
// Segmented level bar (vertical when the rect is taller than wide). level01 in [0,1], peak01 optional.
void drawVfdBar(juce::Graphics& g, juce::Rectangle<float> r, float level01, float peak01 = -1.f, int segments = 24);

// ---- Look and feel -----------------------------------------------------------------------------
class RetroLookAndFeel : public juce::LookAndFeel_V4 {
public:
    RetroLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    juce::Label* createSliderTextBox(juce::Slider&) override;
    void drawLabel(juce::Graphics&, juce::Label&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawComboBox(juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void drawPopupMenuBackground(juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>&, bool, bool, bool, bool, bool,
                           const juce::String&, const juce::String&, const juce::Drawable*, const juce::Colour*) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont(juce::TextButton&, int) override;
    void drawCallOutBoxBackground(juce::CallOutBox&, juce::Graphics&, const juce::Path&, juce::Image&) override;
};

// Metal push-button face with an optional LED; shared by the toggle and text buttons.
void drawMetalButton(juce::Graphics& g, juce::Rectangle<float> r, bool down, bool over, bool focused);
void drawLed(juce::Graphics& g, float cx, float cy, float r, juce::Colour c, bool on);

}  // namespace retro
