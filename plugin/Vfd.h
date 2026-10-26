#pragma once
// A vacuum fluorescent display, drawn the way the real thing looks: a dark tinted glass over a
// 5x7 dot-matrix whose unlit dots stay faintly visible, a blue-green ZnO:Zn phosphor that blooms
// around lit dots, the fine anode mesh in front of the phosphor, a reflection on the glass and a
// slight flicker. Used by the preset selector (name field and its arrows).
#include <juce_gui_basics/juce_gui_basics.h>

namespace vfd {

extern const juce::Colour kPhosphor;     // lit dot colour

// Draw a complete display into r: glass, ghost matrix, text, bloom, mesh, reflection.
// `left` is drawn dim and left-aligned, `centre` bright and centred, `glyph` (if set) a single
// special glyph ("<" or ">" arrows) centred. brightness 0..1 scales the lit layer.
struct Content { juce::String left, centre; juce::String glyph; };
// scroll: pixel offset applied to a centre text that does not fit (marquee); returns the overflow in px (0 = fits).
float draw(juce::Graphics& g, juce::Rectangle<float> r, const Content& c, float brightness = 1.f, float scroll = 0.f);

// Component wrapper with a cache and the flicker timer.
class Display : public juce::Component, private juce::Timer {
public:
    Display();
    void setContent(const Content& c);
    void paint(juce::Graphics& g) override;
    void resized() override { cache_ = juce::Image(); }
private:
    void timerCallback() override;
    Content content_;
    juce::Image cache_;
    float flicker_ = 1.f, target_ = 1.f;
    float overflow_ = 0.f, scroll_ = 0.f; double hold_ = 0.0;   // marquee for names that do not fit
    juce::Random rnd_;
};

}  // namespace vfd
