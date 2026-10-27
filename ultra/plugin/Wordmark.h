#pragma once
// The CHROMOSPHERE | ULTRA wordmark, rendered through a small software lens: lateral chromatic aberration
// (red and blue fringes that grow towards the ends), spherical aberration (a slight barrel and a
// softening away from the centre), an occasional subtle glitch (sliced bands shifting for a few
// frames, with the fringing briefly widening), and a soft-breathing underscore cursor after the word.
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace wordmark {

class Wordmark : public juce::Component, private juce::Timer {
public:
    // level: optional 0..1 energy; a hot signal makes the glitches a little more frequent.
    explicit Wordmark(std::function<float()> level = {});
    ~Wordmark() override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    void renderNow();                               // advance and redraw once (used by the snapshot tool)
    void setAnimating(bool on) { if (on) { if (!isTimerRunning()) startTimerHz(10); } else stopTimer(); }
    void setTime(double seconds) { fixedTime_ = seconds; }   // deterministic frames for tests

private:
    void timerCallback() override;
    void buildSource();
    void render(double t);
    std::function<float()> level_;
    juce::Image source_, out_;
    double start_ = 0, fixedTime_ = -1;
    double glitchStart_ = -1, glitchEnd_ = -1, nextGlitch_ = 2.0;
    int bandY_[3] = {0, 0, 0}, bandH_[3] = {0, 0, 0}; float bandDx_[3] = {0, 0, 0};
    juce::Random rnd_{0x5eed};
    float textL_ = 0.f, textR_ = 1.f;               // word extent in render pixels: the lens centre
    float cursorX_ = 0.f;                           // cursor's left edge in component pixels
    double now_ = 0.0;
    bool lensStale_ = true, full_ = true;
    static constexpr float kScale = 2.f;            // render at 2x and draw down for crisp fringes
    static constexpr int kPad = 8;                  // room for displacement, in component pixels
};

}  // namespace wordmark
