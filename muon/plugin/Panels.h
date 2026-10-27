#pragma once
// Editor building blocks for MUON: the four pages (SHAPE, HARMONICS, RESONANCE, OUTPUT), the order-bank faders,
// the keyboard strip of tuned resonators, the macro bay and the main display (track view, analyser, meters).
#include "Controls.h"
#include "Sun.h"
#include "muon/engine.h"
#include <juce_dsp/juce_dsp.h>
#include <map>
#include <memory>
#include <vector>

class MuonProcessor;

namespace panels {

using APVTS = juce::AudioProcessorValueTreeState;
using controls::Choice;
using controls::Knob;
using controls::Toggle;

// A page: a set of controls and section headers. Subclasses lay them out. Right-click on any dial offers the macro
// assignments for its parameter.
class Page : public juce::Component {
public:
    explicit Page(MuonProcessor& p);
    ~Page() override;
    void paint(juce::Graphics&) override;
    virtual void tick() {}                      // 30 Hz from the editor
protected:
    Knob& knob(const juce::String& id, const juce::String& title, Knob::Size sz = Knob::Normal, bool bipolar = false);
    Choice& choice(const juce::String& id, const juce::String& title);
    Toggle& toggle(const juce::String& id, const juce::String& title);
    template <class T> T& own(std::unique_ptr<T> c, const juce::String& id) { auto& r = *c; addAndMakeVisible(r); byId_[id] = &r; owned_.push_back(std::move(c)); return r; }
    struct Section { juce::String code, title; juce::Rectangle<int> r; };
    std::vector<Section> sections_;
    void section(const juce::String& code, const juce::String& title, juce::Rectangle<int> r) { sections_.push_back({code, title, r}); }
    void row(juce::Rectangle<int> area, std::initializer_list<juce::Component*> items, int gap = 4);
    void beginLayout() { sections_.clear(); dividers_.clear(); }
    std::vector<juce::Line<float>> dividers_;
    juce::Component* c(const juce::String& id) const { auto it = byId_.find(id); return it == byId_.end() ? nullptr : it->second; }
    float value(const juce::String& id) const;
    MuonProcessor& proc_;
    std::vector<std::unique_ptr<juce::Component>> owned_;
    std::map<juce::String, juce::Component*> byId_;
private:
    struct MacroMenu;
    std::vector<std::unique_ptr<MacroMenu>> menus_;
};

class ShapePage : public Page { public: explicit ShapePage(MuonProcessor&); void resized() override; void tick() override; };

// Eight vertical faders (H1 .. H8) with a phase lamp under H2 .. H8; orders above the clean limit are greyed.
class OrderBank : public juce::Component {
public:
    OrderBank(MuonProcessor& p, int band);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void tick();
    void applyShape(int shape);                  // one of kBankShapes
    static const char* const kBankShapes[7];
private:
    juce::Rectangle<float> faderRect(int order) const;
    juce::Rectangle<float> lampRect(int order) const;
    int orderAt(juce::Point<int> p) const;
    juce::RangedAudioParameter* param(int order, bool invert = false) const;
    void set(int order, float v);
    MuonProcessor& proc_;
    int band_, gesture_ = -1, maxOrder_ = 8;
    float shown_[8] = {};
};

class HarmonicsPage : public Page {
public:
    explicit HarmonicsPage(MuonProcessor&);
    void resized() override;
    void tick() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void showBand(int b);
private:
    int band_ = 0, bands_ = 3;
    juce::Rectangle<int> bandTabs_;
    std::vector<std::unique_ptr<OrderBank>> banks_;
    juce::TextButton shapes_;
};

// The tuned resonators on a keyboard strip, C1 .. C8: a lit key for each resonator's nearest note.
class KeyStrip : public juce::Component {
public:
    explicit KeyStrip(MuonProcessor& p) : proc_(p) {}
    void paint(juce::Graphics&) override;
    void tick();
private:
    MuonProcessor& proc_;
    std::vector<int> lit_;
};

class ResonancePage : public Page {
public:
    explicit ResonancePage(MuonProcessor&);
    void resized() override;
    void tick() override;
private:
    KeyStrip keys_;
};

class OutputPage : public Page {
public:
    explicit OutputPage(MuonProcessor&);
    void resized() override;
    void tick() override;
    void paint(juce::Graphics&) override;
private:
    juce::Rectangle<int> info_;
    juce::String shown_;
};

// The four macros as large dials, with Punch and Tail beside them.
class MacroBay : public juce::Component {
public:
    explicit MacroBay(MuonProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    std::vector<std::unique_ptr<Knob>> knobs_;
    std::unique_ptr<Knob> punch_, tail_;
};

// The recessed scope over the rising blue limb: the track view (waveform, transient trail, gain), the analyser with
// the crossover handles, and meters. Drawn as an amber CRT, like the rest of the family.
class MainDisplay : public juce::Component, public look::Animated, private juce::Timer {
public:
    MainDisplay(MuonProcessor& p, std::function<sun::Drive()> drive);
    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    sun::SunView sun;
    juce::String caption;
    void pullFrames();                           // drain the engine's track FIFO (also called by the snapshot tool)
    void setAnimating(bool on) override { if (on) startTimerHz(30); else stopTimer(); }
private:
    void timerCallback() override { pullFrames(); }
    void paintScope(juce::Graphics&);
    void paintTraces(juce::Graphics&, juce::Rectangle<float> all);
    void paintTrack(juce::Graphics&, juce::Rectangle<float>);
    void paintSpectrum(juce::Graphics&, juce::Rectangle<float>);
    void paintMeters(juce::Graphics&, juce::Rectangle<float>);
    float freqToX(float f) const;
    float xToFreq(float x) const;
    MuonProcessor& proc_;
    juce::Image traceLayer_, glow_, scanlines_;
    bool tracesDirty_ = true;                     // redraw the traces only when what they show has changed
    int lastScopePos_ = -1;
    juce::String drawnState_;                     // parameters the traces show (crossovers, bands, caption)
    bool spectrumLive_ = true;
    // Track history: one frame per 128 samples, about 3 s.
    static constexpr int kFrames = 1200;
    std::vector<muon::TrackFrame> frames_;
    int head_ = 0, liveFrames_ = 0;              // frames in the ring that carry signal
    struct Streak { float age = 0.f; float y = 0.f; float slant = 0.f; int frame = 0; };
    std::vector<Streak> streaks_;
    // Analyser.
    juce::dsp::FFT fft_{12};
    std::vector<float> fftBuf_, spec_;
    juce::Rectangle<float> specArea_;
    int dragHandle_ = -1, hoverHandle_ = -1;
    float inHold_ = 0.f, outHold_ = 0.f;
};

}  // namespace panels
