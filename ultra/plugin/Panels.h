#pragma once
// Editor building blocks for ULTRA: controls bound to parameters (from INFRA), the four pages (SEQ, VOICE, MIX, FX),
// the step grid and lane editor, the macro bay, the transport, the pads and the main display (orbit radar, voice,
// output).
#include "UltraLook.h"
#include "Sun.h"
#include "Vfd.h"
#include "ultra/params.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <memory>
#include <vector>

class UltraProcessor;

namespace panels {

using APVTS = juce::AudioProcessorValueTreeState;

// Editor-wide selection: the track the VOICE page, lane editor and voice display show, the step page (1-16 or
// 17-32) and the lane being edited.
struct UiState {
    int track = 0, stepPage = 0, lane = 0;
    std::function<void()> onTrackChanged;          // the editor repaints what depends on the selection
    void select(int t) { if (t != track) { track = t; if (onTrackChanged) onTrackChanged(); } }
};

// A dial with its caps label above and an inset readout below (the EXO knob, as on INFRA).
class Knob : public juce::Component {
public:
    enum Size { Small, Normal, Large };
    Knob(APVTS& s, const juce::String& id, const juce::String& title, Size size = Normal, bool bipolar = false);
    void paint(juce::Graphics&) override;
    void resized() override;
    void setTitleText(const juce::String& t) { if (t != title_) { title_ = t; slider.setTitle(t); repaint(); } }
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

// A lever switch bound to a bool parameter.
class Toggle : public juce::Component {
public:
    Toggle(APVTS& s, const juce::String& id, const juce::String& title);
    void resized() override { button.setBounds(getLocalBounds()); }
    juce::ToggleButton button;
private:
    std::unique_ptr<APVTS::ButtonAttachment> att_;
};

// A small latching button bound to a bool parameter (mute, solo): lit in the accent when on.
class Switch : public juce::Component {
public:
    Switch(APVTS& s, const juce::String& id, const juce::String& text, const juce::String& title);
    void resized() override { button.setBounds(getLocalBounds()); }
    juce::TextButton button;
private:
    std::unique_ptr<APVTS::ButtonAttachment> att_;
};

// A page: a set of controls and section headers. Subclasses lay them out.
class Page : public juce::Component {
public:
    Page(UltraProcessor& p, UiState& ui);
    void paint(juce::Graphics&) override;
    virtual void tick() {}                      // 30 Hz from the editor
    virtual void trackChanged() {}
protected:
    Knob& knob(const juce::String& id, const juce::String& title, Knob::Size sz = Knob::Normal, bool bipolar = false);
    Choice& choice(const juce::String& id, const juce::String& title);
    Toggle& toggle(const juce::String& id, const juce::String& title);
    Switch& sw(const juce::String& id, const juce::String& text, const juce::String& title);
    struct Section { juce::String code, title; juce::Rectangle<int> r; };
    std::vector<Section> sections_;
    void section(const juce::String& code, const juce::String& title, juce::Rectangle<int> r) { sections_.push_back({code, title, r}); }
    // Lays items out in equal columns; a scored divider goes between neighbouring dials (as on EXO).
    void row(juce::Rectangle<int> area, std::initializer_list<juce::Component*> items, int gap = 4);
    void beginLayout() { sections_.clear(); dividers_.clear(); }
    std::vector<juce::Line<float>> dividers_;
    juce::Component* c(const juce::String& id) const { auto it = byId_.find(id); return it == byId_.end() ? nullptr : it->second; }
    UltraProcessor& proc_;
    UiState& ui_;
    std::vector<std::unique_ptr<juce::Component>> owned_;
    std::map<juce::String, juce::Component*> byId_;
};

// ---- SEQ ---------------------------------------------------------------------------------------------------
// The 8 x 16 step grid: track labels with mute and solo lamps, steps lit by velocity, the playhead, length and rate.
class StepGrid : public juce::Component, public juce::SettableTooltipClient {
public:
    StepGrid(UltraProcessor& p, UiState& ui);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void tick();
private:
    enum Part { None, Label, Mute, Solo, Step, Length, Rate };
    struct Hit { Part part = None; int track = -1, step = -1; };
    Hit hitTest(juce::Point<int> p) const;
    juce::Rectangle<float> rowRect(int t) const;
    juce::Rectangle<float> stepRect(int t, int col) const;
    juce::Rectangle<float> labelRect(int t) const;
    juce::Rectangle<float> lengthRect(int t) const;
    juce::Rectangle<float> rateRect(int t) const;
    int pattern() const;
    UltraProcessor& proc_;
    UiState& ui_;
    int paintVer_ = -1, lastPattern_ = -1, lastStep_[ultra::kTracks] = {}, lastHits_[ultra::kTracks] = {}, lastLamps_[ultra::kTracks] = {};
    float flash_[ultra::kTracks] = {};
    bool lastRunning_ = false;
    int dragTrack_ = -1, dragVel_ = -1, lenDragStart_ = 0; float lenDragY_ = 0;
    Part dragPart_ = None;
};

// The selected track's lanes, one at a time: velocity, chance, ratchet, pitch, nudge, condition, lock.
class LaneEditor : public juce::Component {
public:
    LaneEditor(UltraProcessor& p, UiState& ui);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void tick();
private:
    juce::Rectangle<float> laneList() const;
    juce::Rectangle<float> area() const;
    int stepAt(float x) const;
    void setFromY(int step, float y);
    UltraProcessor& proc_;
    UiState& ui_;
    int paintVer_ = -1, lastStep_ = -2, lastTrack_ = -1;
};

class SeqPage : public Page {
public:
    SeqPage(UltraProcessor& p, UiState& ui);
    void resized() override;
    void tick() override;
    void trackChanged() override { lanes_.repaint(); grid_.repaint(); }
private:
    void mutate();
    StepGrid grid_;
    LaneEditor lanes_;
    juce::TextButton page1_, page2_, copy_, paste_, clear_, left_, right_, mutate_, patternMenu_;
    std::unique_ptr<ultra::TrackPattern> clip_;
    juce::Random rng_;
};

// ---- VOICE / MIX / FX ----------------------------------------------------------------------------------------
class VoicePage : public Page {
public:
    VoicePage(UltraProcessor& p, UiState& ui);
    void resized() override;
    void tick() override;
    void trackChanged() override;
    void paint(juce::Graphics&) override;
private:
    int shownModel_[ultra::kTracks];
};

class MixPage : public Page {
public:
    MixPage(UltraProcessor& p, UiState& ui);
    void resized() override;
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void tick() override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    std::array<juce::Rectangle<int>, ultra::kTracks> strips_{};
    std::array<float, ultra::kTracks> shown_{};
};

class FxPage : public Page {
public:
    FxPage(UltraProcessor& p, UiState& ui);
    void resized() override;
    void tick() override;
};

// ---- Macro bay, transport, pads --------------------------------------------------------------------------------
class MacroBay : public juce::Component {
public:
    explicit MacroBay(UltraProcessor& p);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    std::vector<std::unique_ptr<Knob>> knobs_;
};

// Play, the pattern buttons A..H (the queued one blinks) and the momentary Fill.
class Transport : public juce::Component {
public:
    explicit Transport(UltraProcessor& p);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void tick();
private:
    juce::Rectangle<float> playRect() const;
    juce::Rectangle<float> patternRect(int i) const;
    juce::Rectangle<float> fillRect() const;
    UltraProcessor& proc_;
    bool fillHeld_ = false;
    int lastState_ = -1;
};

class Pads : public juce::Component {
public:
    Pads(UltraProcessor& p, UiState& ui);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void tick();
private:
    juce::Rectangle<float> padRect(int t) const;
    UltraProcessor& proc_;
    UiState& ui_;
    int lastHits_[ultra::kTracks] = {};
    float flash_[ultra::kTracks] = {};
    int paintVer_ = -1;
};

// ---- Main display ------------------------------------------------------------------------------------------------
// The recessed amber CRT over the rising ultraviolet limb: the orbit radar, the selected voice and the output.
class MainDisplay : public juce::Component {
public:
    MainDisplay(UltraProcessor& p, UiState& ui, std::function<sun::Drive()> drive);
    void resized() override;
    void paint(juce::Graphics&) override;
    sun::SunView sun;
    juce::String caption;                       // preset description, shown in the scope
private:
    void paintScope(juce::Graphics&);
    void paintTraces(juce::Graphics&, juce::Rectangle<float> all);
    void paintRadar(juce::Graphics&, juce::Rectangle<float>);
    void paintVoice(juce::Graphics&, juce::Rectangle<float>);
    void paintOutput(juce::Graphics&, juce::Rectangle<float>);
    bool updateVoicePreview();                   // true when the preview was re-rendered
    juce::Image traceLayer_, overlay_;          // traces (software, drawn into) and the flattened overlay (only drawn)
    juce::String traceKey_;
    UltraProcessor& proc_;
    UiState& ui_;
    std::vector<float> preview_;                 // peak envelope of a rendered hit, one value per 2 ms
    float previewSeconds_ = 0.5f;
    juce::String previewKey_;
    int lastHits_[ultra::kTracks] = {};
    float flash_[ultra::kTracks] = {};
};

}  // namespace panels
