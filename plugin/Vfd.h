#pragma once
// A vacuum fluorescent display, drawn the way the real thing looks: a dark tinted glass over a
// 5x7 dot-matrix whose unlit dots stay faintly visible, a blue-green ZnO:Zn phosphor that blooms
// around lit dots, the fine anode mesh in front of the phosphor, a reflection on the glass and a
// slight flicker. Used by the preset selector (name field and its arrows).
#include <juce_gui_basics/juce_gui_basics.h>

namespace vfd {

extern const juce::Colour kPhosphor;     // lit dot colour

// The display is a fixed dot matrix (7 rows, as many columns as fit at the dot pitch). Content is
// laid out as a drive mask on that matrix; each dot has its own phosphor state that rises in a few
// milliseconds when driven and cools over a few hundred when not, shifting colour as it fades.
// Text wider than the matrix scrolls column by column; the trail it leaves is the phosphor cooling.
struct Content { juce::String left, centre; juce::String glyph; juce::String right; bool alignLeft = false; };   // right: a dim glyph at the far end (the combo chevron)

class Display : public juce::Component, private juce::Timer {
public:
    Display();
    void setContent(const Content& c);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void renderNow(double dt) { step(dt); }          // tests / snapshots
    void setAnimating(bool on) { paused_ = !on; if (on && !isTimerRunning()) startTimerHz(30); else if (!on) stopTimer(); }
    void setMinPitch(float p) { minPitch_ = p; resized(); }   // readouts allow finer dots so values never scroll
private:
    void timerCallback() override;
    void rebuildStatic();                           // glass, ghost matrix, mesh, sprites, for the current size
    void rebuildMask();                             // drive levels for the current content and scroll
    void step(double dt);                           // phosphor physics
    Content content_;
    int cols_ = 0; float pitch_ = 3.f, dot_ = 2.f, x0_ = 0.f, y0_ = 0.f, minPitch_ = 1.9f;
    std::vector<float> mask_, state_;               // cols x 7, row-major
    std::vector<juce::Image> sprites_;              // dot at brightness levels
    juce::Image under_, over_;                      // static layers
    int textCols_ = 0, availCols_ = 0, scroll_ = 0; double hold_ = 0.0, scrollAcc_ = 0.0;
    float flicker_ = 1.f, target_ = 1.f;
    bool dirty_ = true, paused_ = false;
    juce::Random rnd_;
};

// A slider readout on a VFD: shows the value; while editing, the normal text editor appears.
class ValueLabel : public juce::Label {
public:
    ValueLabel();
    void paint(juce::Graphics&) override {}
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void textWasChanged() override { sync(); }
    void enablementChanged() override { sync(); }
    void editorShown(juce::TextEditor*) override { vfd_.setVisible(false); }
    void editorAboutToBeHidden(juce::TextEditor*) override { vfd_.setVisible(true); }
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override {}
private:
    void sync();
    Display vfd_;
};

// Pauses or resumes every Display below a component.
void setAnimatingAll(juce::Component& root, bool on);
// Steps every Display below a component (headless snapshots).
void stepAll(juce::Component& root, double dt);

// A combo box whose face is a VFD: the chosen entry left-aligned, a dim down-arrow at the end.
class ComboBox : public juce::ComboBox {
public:
    ComboBox();
    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void valueChanged(juce::Value&) override;
    void enablementChanged() override { sync(); }
private:
    void sync();
    Display vfd_;
};

}  // namespace vfd
