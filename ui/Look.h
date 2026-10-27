#pragma once
#include <atomic>
// The Chromosphere "Studio" look from EXO, shared by the family: near-black matte panel, soft lighting, one accent
// colour, Barlow small capitals, large quiet dials with lit arcs. Each product installs its Theme (palette, sun
// colours, wordmark, embedded resources) before it builds an editor; INFRA's infrared is the default.
// Everything is drawn procedurally; dial bodies are cached per size.
#include <juce_gui_basics/juce_gui_basics.h>

namespace look {

// One colour as linear-ish RGB floats, for the sun's shading.
struct Rgb3 { float r, g, b; };

// The sun's colours: the disc runs deep -> mid -> hot with brightness; the rest are tints of what is added on top.
struct SunPalette {
    Rgb3 deep{0.34f, 0.03f, 0.03f}, mid{1.0f, 0.15f, 0.07f}, hot{1.0f, 0.64f, 0.50f}, corona{1.0f, 0.2f, 0.1f};
    Rgb3 flare{1.f, 0.62f, 0.52f}, halo{1.f, 0.4f, 0.3f}, particle{1.f, 0.3f, 0.16f}, radiation{1.f, 0.7f, 0.62f}, cme{0.25f, 0.09f, 0.07f};
    juce::Colour clear{0xff0d0a0b};
};

// What makes a product's panel its own. Everything else (greys, the amber CRT, the cyan VFD, the red LED readouts)
// is shared by the family.
struct Theme {
    juce::String wordmark = "CHROMOSPHERE | INFRA";   // the header wordmark
    juce::String product = "Chromosphere INFRA";      // thread names
    juce::Colour bg{0xff121012}, plate{0xff1b1a1c}, plateEdge{0xff28262a}, line{0xff2c292c}, text{0xffefe7e4},
        textDim{0xff9f9693}, textFaint{0xff615956}, accent{0xffe8402e}, accentDim{0xff6e2620}, displayBg{0xff0d0a0b},
        grid{0xff221b1b}, curve{0xffff5f45}, curveFill{0x40e8402e}, meter{0xffcdbcb8}, meterHot{0xffe8402e}, danger{0xffff2d6a};
    SunPalette sun;
    // Embedded resources (fonts BarlowRegular_ttf / BarlowMedium_ttf / BarlowSemiBold_ttf, corona_bg_jpg,
    // solar_limb_png): the product's BinaryData getNamedResource.
    const char* (*resource)(const char* name, int& size) = nullptr;
};

// Installs a theme: updates the palette below and empties the image caches. Message thread, before any editor of
// the product draws (each product is its own binary, so one theme per process).
void setTheme(const Theme& t);
const Theme& theme();
// Installs a theme from a constructor's base-class list, so it is in place before any member (the look and feel,
// the suns) is built: class Editor : private look::ThemeGuard, public juce::AudioProcessorEditor { ... }.
struct ThemeGuard { explicit ThemeGuard(const Theme& t) { setTheme(t); } };

// Palette.
constexpr int kDialReadoutGap = 14;   // air between a dial and its readout (EXO); dense pages pass a smaller one
// The gap a slider asks for: its "readoutGap" property, else kDialReadoutGap.
inline int dialReadoutGap(const juce::Slider& s) { return (int)s.getProperties().getWithDefault("readoutGap", kDialReadoutGap); }
extern juce::Colour kBg, kPlate, kPlateEdge, kLine, kText, kTextDim, kTextFaint, kAccent, kAccentDim,
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
void releaseImageCaches();                     // empties the shared image caches (when the last editor closes)
// Gaussian blur of an ARGB image in place: two separable passes (horizontal then vertical) of 2 * radius + 1 taps,
// the same weights as juce::ImageConvolutionKernel::createGaussianBlur(sigma) but a fraction of its cost.
void gaussianBlur(juce::Image& img, int radius, float sigma);
// Embedded image by BinaryData name, or an invalid image when absent. Cached.
juce::Image asset(const char* name);

// Controls drawn by the look and feel.
class PanelLookAndFeel : public juce::LookAndFeel_V4 {
public:
    PanelLookAndFeel();
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
