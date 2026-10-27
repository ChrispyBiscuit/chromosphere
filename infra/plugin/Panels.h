#pragma once
// Editor building blocks for INFRA: controls bound to parameters, the four pages (SYNTH, MOD, ARP, FX),
// the macro bay, the step-lane editor and the main display.
#include "InfraLook.h"
#include "Sun.h"
#include "Vfd.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <memory>
#include <vector>

class InfraProcessor;

namespace panels {

using APVTS = juce::AudioProcessorValueTreeState;

// A dial with its caps label above and an inset readout below (the EXO knob).
class Knob : public juce::Component {
public:
    enum Size { Small, Normal, Large };
    Knob(APVTS& s, const juce::String& id, const juce::String& title, Size size = Normal, bool bipolar = false);
    void paint(juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;
    juce::String paramId;
private:
    std::unique_ptr<APVTS::SliderAttachment> att_;
    juce::String title_;
    Size size_;
    juce::Rectangle<int> label_;
};

// A combo box bound to a choice parameter, with a caps label above.
class Choice : public juce::Component {
public:
    Choice(APVTS& s, const juce::String& id, const juce::String& title);
    void paint(juce::Graphics&) override;
    void resized() override;
    vfd::ComboBox box;
private:
    std::unique_ptr<APVTS::ComboBoxAttachment> att_;
    juce::String title_;
};

// A pill switch bound to a bool parameter.
class Toggle : public juce::Component {
public:
    Toggle(APVTS& s, const juce::String& id, const juce::String& title);
    void resized() override { button.setBounds(getLocalBounds()); }
    juce::ToggleButton button;
private:
    std::unique_ptr<APVTS::ButtonAttachment> att_;
};

// A page: a set of controls and section headers. Subclasses lay them out.
class Page : public juce::Component {
public:
    explicit Page(InfraProcessor& p);
    void paint(juce::Graphics&) override;
    virtual void tick() {}                      // 30 Hz from the editor
protected:
    Knob& knob(const juce::String& id, const juce::String& title, Knob::Size sz = Knob::Normal, bool bipolar = false);
    Choice& choice(const juce::String& id, const juce::String& title);
    Toggle& toggle(const juce::String& id, const juce::String& title);
    struct Section { juce::String code, title; juce::Rectangle<int> r; };
    std::vector<Section> sections_;
    void section(const juce::String& code, const juce::String& title, juce::Rectangle<int> r) { sections_.push_back({code, title, r}); }
    // Lays items out in equal columns; a scored divider goes between neighbouring dials (as on EXO).
    void row(juce::Rectangle<int> area, std::initializer_list<juce::Component*> items, int gap = 4);
    void beginLayout() { sections_.clear(); dividers_.clear(); }
    std::vector<juce::Line<float>> dividers_;
    juce::Component* c(const juce::String& id) const { auto it = byId_.find(id); return it == byId_.end() ? nullptr : it->second; }
    InfraProcessor& proc_;
    std::vector<std::unique_ptr<juce::Component>> owned_;
    std::map<juce::String, juce::Component*> byId_;
};

class SynthPage : public Page { public: explicit SynthPage(InfraProcessor&); void resized() override; void tick() override; };
class ModPage : public Page { public: explicit ModPage(InfraProcessor&); void resized() override; void tick() override; };
class FxPage : public Page { public: explicit FxPage(InfraProcessor&); void resized() override; void tick() override; };

class StepEditor : public juce::Component {
public:
    explicit StepEditor(InfraProcessor& p);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void tick();
private:
    enum Lane { Vel, Tr, Rat, None };
    Lane laneAt(juce::Point<int> p) const;
    int stepAt(juce::Point<int> p) const;
    juce::Rectangle<float> laneRect(Lane l) const;
    juce::RangedAudioParameter* param(int step, Lane l) const;
    float value(int step, Lane l) const;
    void set(int step, Lane l, float plain);
    InfraProcessor& proc_;
    int lastLane_ = -2, dragStep_ = -1, gestureStep_ = -1; Lane dragLane_ = None;
    float dragStartY_ = 0, dragStartVal_ = 0;
};

class ArpPage : public Page { public: explicit ArpPage(InfraProcessor&); void resized() override; void tick() override; private: StepEditor steps_; };

// Four large macro dials with their meanings.
class MacroBay : public juce::Component {
public:
    explicit MacroBay(InfraProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    std::vector<std::unique_ptr<Knob>> knobs_;
};

// The recessed scope over the rising infrared limb: wavetables, filter response, envelope, output scope, meters.
class MainDisplay : public juce::Component {
public:
    MainDisplay(InfraProcessor& p, std::function<sun::Drive()> drive);
    void resized() override;
    void paint(juce::Graphics&) override;
    sun::SunView sun;
    juce::String caption;                       // preset description, shown in the scope
private:
    void paintScope(juce::Graphics&);
    void paintTraces(juce::Graphics&, juce::Rectangle<float> all);
    juce::Image traceLayer_;
    void paintTable(juce::Graphics&, juce::Rectangle<float>, int osc);
    void paintFilter(juce::Graphics&, juce::Rectangle<float>);
    void paintEnvelope(juce::Graphics&, juce::Rectangle<float>);
    void paintWave(juce::Graphics&, juce::Rectangle<float>);
    InfraProcessor& proc_;
};

}  // namespace panels
