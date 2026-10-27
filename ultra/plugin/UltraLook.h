#pragma once
#include <atomic>
// The Chromosphere "Studio" look from EXO (via INFRA), in ultraviolet: near-black matte panel, soft lighting, one violet accent,
// Barlow small capitals, large quiet dials with lit arcs.
// Everything is drawn procedurally; dial bodies are cached per size.
#include <juce_gui_basics/juce_gui_basics.h>

namespace look {

// A UTF-8 literal as a juce::String (juce::String(const char*) reads ASCII, which mangles the middle dot).
inline juce::String u8(const char* s) { return juce::String::fromUTF8(s); }

// Palette.
constexpr int kDialReadoutGap = 14;   // air between a dial and its readout (EXO); dense pages pass a smaller one
// The gap a slider asks for: its "readoutGap" property, else kDialReadoutGap.
inline int dialReadoutGap(const juce::Slider& s) { return (int)s.getProperties().getWithDefault("readoutGap", kDialReadoutGap); }
extern const juce::Colour kBg, kPlate, kPlateEdge, kLine, kText, kTextDim, kTextFaint, kAccent, kAccentDim,
    kDisplayBg, kGrid, kCurve, kCurveFill, kMeter, kMeterHot, kDanger;

// Typography. Falls back to the default sans-serif when the embedded typeface is missing.
juce::Font font(float height, int weight = 400);        // 400 regular, 500 medium, 600 semibold
juce::Font caps(float height);                          // small label style: medium, tracked, meant for uppercase text
void drawCaps(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just,
              float height, juce::Colour colour = kTextDim);

// Surfaces.
void drawPlate(juce::Graphics& g, juce::Rectangle<float> r, float corner = 8.f);     // raised faceplate section
void drawDisplay(juce::Graphics& g, juce::Rectangle<float> r, float corner = 8.f, bool opaque = true);   // recessed dark display; opaque=false leaves the base for a live background
void drawDivider(juce::Graphics& g, float x0, float y, float x1);
void drawBolt(juce::Graphics& g, float cx, float cy, float r);
// Section header: index code, caps title, and a hairline to the right.
void drawSection(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& code, const juce::String& title);
const juce::Image& backgroundTexture(int w, int h);
const juce::Image& plateTexture();            // fine anodised grain for the instrument bays
// Holds the shared typefaces and image caches for as long as an editor is open. Every editor owns one, as its first
// member: the caches are released when the last editor in the process closes.
struct Resources;
class ResourceScope {
public:
    ResourceScope();
    ~ResourceScope();
private:
    std::unique_ptr<juce::SharedResourcePointer<Resources>> res_;
};
// Gaussian blur of an ARGB image in place: two separable passes (horizontal then vertical) of 2 * radius + 1 taps,
// the same weights as juce::ImageConvolutionKernel::createGaussianBlur(sigma) but a fraction of its cost.
void gaussianBlur(juce::Image& img, int radius, float sigma);
// Embedded image by BinaryData name, or an invalid image when absent. Cached.
juce::Image asset(const char* name);

// Controls drawn by the look and feel.
class UltraLookAndFeel : public juce::LookAndFeel_V4 {
public:
    UltraLookAndFeel();
    // 0..1 low-frequency content of the output. The dial rings glow and pulse with it.
    std::atomic<float> bassGlow{0.f};
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
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

}  // namespace look
