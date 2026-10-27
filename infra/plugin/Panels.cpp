#include "Panels.h"
#include "PluginProcessor.h"
#include "infra/tables.h"
#include <complex>

using namespace look;

namespace panels {

// The scope is an amber CRT, the same instrument as on EXO: P3 phosphor body, a hotter near-white core,
// dim amber for grid and labels. It reads clearly over the red limb.
static const juce::Colour kAmber(0xffffb000), kAmberHot(0xffffe3a6), kAmberText(0xb8ffb000), kAmberGrid(0x29ffb000);

// ---- Page -------------------------------------------------------------------------------------------------
Page::Page(InfraProcessor& p) : proc_(p) {}

Knob& Page::knob(const juce::String& id, const juce::String& title, Knob::Size sz, bool bipolar) {
    auto k = std::make_unique<Knob>(proc_.apvts, id, title, sz, bipolar);
    auto& ref = *k; addAndMakeVisible(ref); byId_[id] = &ref; owned_.push_back(std::move(k));
    return ref;
}
Choice& Page::choice(const juce::String& id, const juce::String& title) {
    auto k = std::make_unique<Choice>(proc_.apvts, id, title);
    auto& ref = *k; addAndMakeVisible(ref); byId_[id] = &ref; owned_.push_back(std::move(k));
    return ref;
}
Toggle& Page::toggle(const juce::String& id, const juce::String& title) {
    auto k = std::make_unique<Toggle>(proc_.apvts, id, title);
    auto& ref = *k; addAndMakeVisible(ref); byId_[id] = &ref; owned_.push_back(std::move(k));
    return ref;
}
void Page::paint(juce::Graphics& g) {
    for (const auto& s : sections_) drawSection(g, s.r.toFloat(), s.code, s.title);
    // Scored dividers between dial cells: a dark line with a lighter edge.
    for (const auto& d : dividers_) {
        g.setColour(juce::Colours::black.withAlpha(0.35f)); g.drawLine(d, 1.f);
        g.setColour(juce::Colours::white.withAlpha(0.045f)); g.drawLine(juce::Line<float>(d.getStart().translated(1.f, 0.f), d.getEnd().translated(1.f, 0.f)), 1.f);
    }
}
void Page::row(juce::Rectangle<int> area, std::initializer_list<juce::Component*> items, int gap) {
    const int n = (int)items.size();
    if (n == 0) return;
    const int w = (area.getWidth() - gap * (n - 1)) / n;
    juce::Component* prev = nullptr;
    for (auto* c : items) {
        auto cell = area.removeFromLeft(w);
        if (c) c->setBounds(cell);
        if (prev && c && dynamic_cast<Knob*>(prev) && dynamic_cast<Knob*>(c)) {
            const float x = (float)cell.getX() - gap * 0.5f;
            dividers_.push_back({x, (float)cell.getY() + 10.f, x, (float)cell.getBottom() - 6.f});
        }
        prev = c;
        area.removeFromLeft(gap);
    }
}

static void setOn(juce::Component* c, bool on) { if (c && c->isEnabled() != on) { c->setEnabled(on); c->setAlpha(on ? 1.f : 0.45f); } }

// ---- SYNTH ------------------------------------------------------------------------------------------------
SynthPage::SynthPage(InfraProcessor& p) : Page(p) {
    for (const juce::String x : {"a", "b"}) {
        toggle(x + "On", "On");
        choice(x + "Table", "Table"); choice(x + "Warp", "Warp");
        knob(x + "Morph", "Morph"); knob(x + "WarpAmt", "Warp"); knob(x + "Level", "Level");
        knob(x + "Unison", "Unison"); knob(x + "Detune", "Detune"); knob(x + "Width", "Width");
        knob(x + "Oct", "Octave", Knob::Normal, true); knob(x + "Semi", "Semi", Knob::Normal, true); knob(x + "Fine", "Fine", Knob::Normal, true);
        knob(x + "Pan", "Pan", Knob::Normal, true); knob(x + "Phase", "Phase"); knob(x + "PhaseRand", "Free");
    }
    choice("subShape", "Sub"); knob("subOct", "Sub octave", Knob::Small); knob("subLevel", "Sub level", Knob::Small);
    choice("noiseType", "Noise"); knob("noiseLevel", "Noise level", Knob::Small);
    choice("fType", "Type");
    knob("fCutoff", "Cutoff", Knob::Large); knob("fRes", "Resonance"); knob("fDrive", "Drive");
    knob("fEnv", "Env amt", Knob::Normal, true); knob("fKey", "Keytrack"); knob("fVel", "Vel amt");
    for (const juce::String x : {"amp", "fenv"}) { knob(x + "A", "Attack"); knob(x + "D", "Decay"); knob(x + "S", "Sustain"); knob(x + "R", "Release"); }
    knob("ampCurve", "Curve", Knob::Normal, true); knob("velSens", "Velocity");
}

void SynthPage::tick() {
    for (const juce::String x : {"a", "b"}) {
        const bool on = proc_.apvts.getRawParameterValue(x + "On")->load() > 0.5f;
        for (const juce::String id : {"Table", "Warp", "Morph", "WarpAmt", "Level", "Unison", "Detune", "Width", "Oct", "Semi", "Fine", "Pan", "Phase", "PhaseRand"})
            setOn(c(x + id), on);
        setOn(c(x + "WarpAmt"), on && proc_.apvts.getRawParameterValue(x + "Warp")->load() > 0.5f);
    }
}

void SynthPage::resized() {
    beginLayout();
    auto r = getLocalBounds();
    const int gap = 22;
    auto c1 = r.removeFromLeft((int)(r.getWidth() * 0.48f)); r.removeFromLeft(gap);
    auto c2 = r.removeFromLeft((int)(r.getWidth() * 0.44f)); r.removeFromLeft(gap);
    auto c3 = r;
    // Oscillators: a column of selectors, then two rows of six dials.
    const int oscH = c1.getHeight() / 2;
    for (int o = 0; o < 2; ++o) {
        const juce::String x = o == 0 ? "a" : "b";
        auto b = c1.removeFromTop(oscH);
        if (o == 0) b.removeFromBottom(8);
        section(o == 0 ? "01" : "02", o == 0 ? "Oscillator A" : "Oscillator B", b.removeFromTop(14));
        b.removeFromTop(4);
        auto sel = b.removeFromLeft(118); b.removeFromLeft(10);
        c(x + "On")->setBounds(sel.removeFromTop(24)); sel.removeFromTop(4);
        c(x + "Table")->setBounds(sel.removeFromTop(42)); sel.removeFromTop(2);
        c(x + "Warp")->setBounds(sel.removeFromTop(42));
        auto r1 = b.removeFromTop(b.getHeight() / 2);
        row(r1, {c(x + "Morph"), c(x + "WarpAmt"), c(x + "Level"), c(x + "Unison"), c(x + "Detune"), c(x + "Width")});
        row(b, {c(x + "Oct"), c(x + "Semi"), c(x + "Fine"), c(x + "Pan"), c(x + "Phase"), c(x + "PhaseRand")});
    }
    // Sub, noise, filter.
    {
        auto top = c2.removeFromTop(std::max(150, c2.getHeight() * 2 / 5));
        section("03", "Sub and noise", top.removeFromTop(14)); top.removeFromTop(4);
        auto sel = top.removeFromTop(42);
        c("subShape")->setBounds(sel.removeFromLeft(sel.getWidth() / 2 - 5)); sel.removeFromLeft(10);
        c("noiseType")->setBounds(sel);
        top.removeFromTop(2);
        row(top, {c("subOct"), c("subLevel"), c("noiseLevel")});
        c2.removeFromTop(10);
        section("04", "Filter", c2.removeFromTop(14)); c2.removeFromTop(4);
        auto head = c2.removeFromTop(c2.getHeight() / 2);
        auto ft = head.removeFromLeft(head.getWidth() * 34 / 100);
        c("fType")->setBounds(ft.withSizeKeepingCentre(ft.getWidth(), 42)); head.removeFromLeft(6);
        c("fCutoff")->setBounds(head.removeFromLeft(head.getWidth() / 2));
        c("fRes")->setBounds(head);
        row(c2, {c("fDrive"), c("fEnv"), c("fKey"), c("fVel")});
    }
    // Envelopes.
    {
        auto amp = c3.removeFromTop(c3.getHeight() * 2 / 3);
        section("05", "Amp envelope", amp.removeFromTop(14)); amp.removeFromTop(4);
        row(amp.removeFromTop(amp.getHeight() / 2), {c("ampA"), c("ampD"), c("ampS"), c("ampR")});
        row(amp, {c("ampCurve"), c("velSens"), nullptr, nullptr});
        c3.removeFromTop(6);
        section("06", "Filter envelope", c3.removeFromTop(14)); c3.removeFromTop(4);
        row(c3, {c("fenvA"), c("fenvD"), c("fenvS"), c("fenvR")});
    }
}

// ---- MOD --------------------------------------------------------------------------------------------------
namespace {
class MatrixRow : public juce::Component {
public:
    MatrixRow(APVTS& s, int slot) : index_(slot) {
        const juce::String x = "m" + juce::String(slot);
        for (auto* cb : {&src_, &dst_}) addAndMakeVisible(cb);
        src_.addItemList(dynamic_cast<juce::AudioParameterChoice*>(s.getParameter(x + "Src"))->choices, 1);
        dst_.addItemList(dynamic_cast<juce::AudioParameterChoice*>(s.getParameter(x + "Dst"))->choices, 1);
        src_.setTitle("Mod " + juce::String(slot) + " source"); dst_.setTitle("Mod " + juce::String(slot) + " destination");
        amt_.setSliderStyle(juce::Slider::LinearHorizontal);
        amt_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 58, 20);
        amt_.getProperties().set("bipolar", true);
        amt_.setDoubleClickReturnValue(true, 0.0);
        amt_.setTitle("Mod " + juce::String(slot) + " amount");
        addAndMakeVisible(amt_);
        a1_ = std::make_unique<APVTS::ComboBoxAttachment>(s, x + "Src", src_);
        a2_ = std::make_unique<APVTS::ComboBoxAttachment>(s, x + "Dst", dst_);
        a3_ = std::make_unique<APVTS::SliderAttachment>(s, x + "Amt", amt_);
    }
    bool isLive() const { return src_.getSelectedId() > 1 && dst_.getSelectedId() > 1 && std::abs(amt_.getValue()) > 1e-4; }
    void refresh() { if (const bool l = isLive(); l != shownLive_) { shownLive_ = l; repaint(); } }   // only when the slot turns on or off
    void paint(juce::Graphics& g) override {
        const bool live = shownLive_ = isLive();
        auto r = getLocalBounds().toFloat();
        g.setFont(font(11.f, 600)); g.setColour(live ? kAccent : kTextFaint);
        g.drawText(juce::String(index_), r.removeFromLeft(22.f), juce::Justification::centredLeft);
        // Signal-flow arrow between source and destination.
        const float ax = (float)src_.getRight() + 5.f, ay = r.getCentreY();
        juce::Path arrow; arrow.startNewSubPath(ax, ay); arrow.lineTo(ax + 9.f, ay);
        arrow.addTriangle(ax + 9.f, ay - 3.f, ax + 13.f, ay, ax + 9.f, ay + 3.f);
        g.setColour(live ? kAccent.withAlpha(0.8f) : kTextFaint); g.strokePath(arrow, juce::PathStrokeType(1.2f)); g.fillPath(arrow);
    }
    void resized() override {
        auto r = getLocalBounds().reduced(0, 2);
        r.removeFromLeft(22);
        src_.setBounds(r.removeFromLeft((int)(r.getWidth() * 0.3f))); r.removeFromLeft(20);
        dst_.setBounds(r.removeFromLeft((int)(r.getWidth() * 0.4f))); r.removeFromLeft(10);
        amt_.setBounds(r);
    }
    vfd::ComboBox src_, dst_;
    juce::Slider amt_;
private:
    int index_;
    bool shownLive_ = false;
    std::unique_ptr<APVTS::ComboBoxAttachment> a1_, a2_;
    std::unique_ptr<APVTS::SliderAttachment> a3_;
};
}  // namespace

ModPage::ModPage(InfraProcessor& p) : Page(p) {
    for (int i = 1; i <= 2; ++i) {
        const juce::String x = "l" + juce::String(i);
        choice(x + "Shape", "Shape"); choice(x + "Div", "Division");
        toggle(x + "Sync", "Sync"); toggle(x + "Retrig", "Retrig");
        knob(x + "Rate", "Rate"); knob(x + "Fade", "Fade in");
    }
    knob("menvA", "Attack"); knob("menvD", "Decay"); knob("menvS", "Sustain"); knob("menvR", "Release");
    for (int s = 1; s <= infra::kNumModSlots; ++s) {
        auto r = std::make_unique<MatrixRow>(proc_.apvts, s);
        addAndMakeVisible(*r); byId_["row" + juce::String(s)] = r.get(); owned_.push_back(std::move(r));
    }
    choice("voiceMode", "Voice mode"); knob("polyphony", "Voices", Knob::Small); knob("glide", "Glide", Knob::Small);
    toggle("glideLegato", "Legato glide"); knob("bendRange", "Bend", Knob::Small); knob("drift", "Drift", Knob::Small); knob("tune", "Tune", Knob::Small, true);
}

void ModPage::tick() {
    for (int i = 1; i <= 2; ++i) {
        const juce::String x = "l" + juce::String(i);
        const bool sync = proc_.apvts.getRawParameterValue(x + "Sync")->load() > 0.5f;
        setOn(c(x + "Rate"), !sync); setOn(c(x + "Div"), sync);
    }
    for (int s = 1; s <= infra::kNumModSlots; ++s) if (auto* r = dynamic_cast<MatrixRow*>(c("row" + juce::String(s)))) r->refresh();
}

void ModPage::resized() {
    beginLayout();
    auto r = getLocalBounds();
    auto c1 = r.removeFromLeft((int)(r.getWidth() * 0.4f)); r.removeFromLeft(24);
    auto c2 = r;
    const int lfoH = (int)(c1.getHeight() * 0.36f);
    for (int i = 1; i <= 2; ++i) {
        const juce::String x = "l" + juce::String(i);
        auto b = c1.removeFromTop(lfoH); b.removeFromBottom(6);
        section(i == 1 ? "07" : "08", "LFO " + juce::String(i), b.removeFromTop(14)); b.removeFromTop(4);
        auto sel = b.removeFromLeft(b.getWidth() * 45 / 100); b.removeFromLeft(8);
        auto top = sel.removeFromTop(sel.getHeight() / 2);
        c(x + "Shape")->setBounds(top.removeFromLeft(top.getWidth() / 2 - 3)); top.removeFromLeft(6);
        c(x + "Div")->setBounds(top);
        auto tg = sel.withTrimmedTop(6);
        c(x + "Sync")->setBounds(tg.removeFromTop(24)); tg.removeFromTop(2);
        c(x + "Retrig")->setBounds(tg.removeFromTop(24));
        row(b, {c(x + "Rate"), c(x + "Fade")});
    }
    section("09", "Mod envelope", c1.removeFromTop(14)); c1.removeFromTop(4);
    row(c1, {c("menvA"), c("menvD"), c("menvS"), c("menvR")});
    // Matrix and voice settings.
    auto voice = c2.removeFromBottom(std::max(90, c2.getHeight() / 4));
    section("10", "Modulation matrix", c2.removeFromTop(14)); c2.removeFromTop(6);
    c2.removeFromBottom(10);
    const int rh = c2.getHeight() / infra::kNumModSlots;
    for (int s = 1; s <= infra::kNumModSlots; ++s) c("row" + juce::String(s))->setBounds(c2.removeFromTop(rh));
    section("11", "Voice", voice.removeFromTop(14)); voice.removeFromTop(4);
    auto vm = voice.removeFromLeft(130);
    c("voiceMode")->setBounds(vm.withSizeKeepingCentre(vm.getWidth(), 42)); voice.removeFromLeft(10);
    auto lg = voice.removeFromRight(130);
    c("glideLegato")->setBounds(lg.withSizeKeepingCentre(lg.getWidth(), 24));
    row(voice, {c("polyphony"), c("glide"), c("bendRange"), c("drift"), c("tune")});
}

// ---- ARP --------------------------------------------------------------------------------------------------
StepEditor::StepEditor(InfraProcessor& p) : proc_(p) { setTitle("Arp step lanes"); setDescription("Velocity (0 is a rest), transpose and ratchet for each of the 16 steps"); }

juce::Rectangle<float> StepEditor::laneRect(Lane l) const {
    auto r = getLocalBounds().toFloat();
    r.removeFromLeft(86.f);
    r.removeFromTop(16.f);                 // step numbers
    const float h = r.getHeight();
    auto vel = r.removeFromTop(h * 0.56f); r.removeFromTop(8.f);
    auto tr = r.removeFromTop(h * 0.2f); r.removeFromTop(6.f);
    auto rat = r;
    return l == Vel ? vel : l == Tr ? tr : rat;
}
StepEditor::Lane StepEditor::laneAt(juce::Point<int> p) const {
    for (Lane l : {Vel, Tr, Rat}) if (laneRect(l).expanded(0.f, 3.f).contains(p.toFloat())) return l;
    return None;
}
int StepEditor::stepAt(juce::Point<int> p) const {
    const auto r = laneRect(Vel);
    if (p.x < r.getX() || p.x >= r.getRight()) return -1;
    return juce::jlimit(0, 15, (int)((p.x - r.getX()) / (r.getWidth() / 16.f)));
}
juce::RangedAudioParameter* StepEditor::param(int step, Lane l) const {
    const juce::String id = "s" + juce::String(step + 1) + (l == Vel ? "Vel" : l == Tr ? "Tr" : "Rat");
    return proc_.apvts.getParameter(id);
}
float StepEditor::value(int step, Lane l) const { auto* p = param(step, l); return p->convertFrom0to1(p->getValue()); }
void StepEditor::set(int step, Lane l, float plain) {
    auto* p = param(step, l);
    if (gestureStep_ != step) { if (gestureStep_ >= 0) param(gestureStep_, dragLane_)->endChangeGesture(); p->beginChangeGesture(); gestureStep_ = step; }
    p->setValueNotifyingHost(p->convertTo0to1(plain));
    repaint();
}

void StepEditor::mouseDown(const juce::MouseEvent& e) {
    dragLane_ = laneAt(e.getPosition()); dragStep_ = stepAt(e.getPosition()); gestureStep_ = -1;
    if (dragLane_ == None || dragStep_ < 0) return;
    proc_.undoManager.beginNewTransaction("Edit arp steps");
    if (dragLane_ == Vel) mouseDrag(e);
    else if (dragLane_ == Rat) set(dragStep_, Rat, (float)((int)value(dragStep_, Rat) % 4 + 1));
    else { dragStartY_ = (float)e.y; dragStartVal_ = value(dragStep_, Tr); }
}
void StepEditor::mouseDrag(const juce::MouseEvent& e) {
    if (dragLane_ == Vel) {
        const auto r = laneRect(Vel);
        const int st = stepAt(e.getPosition());
        if (st < 0) return;
        set(st, Vel, juce::jlimit(0.f, 1.f, (r.getBottom() - (float)e.y) / r.getHeight()));
    } else if (dragLane_ == Tr && dragStep_ >= 0) {
        set(dragStep_, Tr, juce::jlimit(-12.f, 12.f, std::round(dragStartVal_ + (dragStartY_ - (float)e.y) / 6.f)));
    }
}
void StepEditor::mouseUp(const juce::MouseEvent&) {
    if (gestureStep_ >= 0) param(gestureStep_, dragLane_)->endChangeGesture();
    gestureStep_ = -1; dragLane_ = None;
}
void StepEditor::mouseDoubleClick(const juce::MouseEvent& e) {
    const Lane l = laneAt(e.getPosition()); const int st = stepAt(e.getPosition());
    if (l == None || st < 0) return;
    dragLane_ = l;
    set(st, l, l == Vel ? 1.f : l == Tr ? 0.f : 1.f);
    mouseUp(e);
}
void StepEditor::tick() {
    const int lane = proc_.synth.arpLane.load(std::memory_order_relaxed);
    if (lane != lastLane_) { lastLane_ = lane; repaint(); }
}

void StepEditor::paint(juce::Graphics& g) {
    const int len = (int)std::lround(proc_.apvts.getRawParameterValue("arpLength")->load());
    const int live = proc_.synth.arpLane.load(std::memory_order_relaxed);
    const auto vel = laneRect(Vel), tr = laneRect(Tr), rat = laneRect(Rat);
    const float cw = vel.getWidth() / 16.f;
    // Lane names.
    auto names = getLocalBounds().toFloat().removeFromLeft(80.f);
    drawCaps(g, "Velocity", names.withY(vel.getY()).withHeight(14.f), juce::Justification::centredLeft, 9.f, kTextDim);
    drawCaps(g, "0 = rest", names.withY(vel.getY() + 14.f).withHeight(14.f), juce::Justification::centredLeft, 8.f, kTextFaint);
    drawCaps(g, "Transpose", names.withY(tr.getCentreY() - 7.f).withHeight(14.f), juce::Justification::centredLeft, 9.f, kTextDim);
    drawCaps(g, "Ratchet", names.withY(rat.getCentreY() - 7.f).withHeight(14.f), juce::Justification::centredLeft, 9.f, kTextDim);
    for (int s = 0; s < 16; ++s) {
        const bool inLen = s < len, now = s == live;
        const float x = vel.getX() + cw * s;
        // Step number and lamp.
        const auto num = juce::Rectangle<float>(x, 0.f, cw, 14.f);
        g.setFont(font(10.f, 600)); g.setColour(now ? kAccent : inLen ? kTextDim : kTextFaint.withAlpha(0.5f));
        g.drawText(juce::String(s + 1), num, juce::Justification::centred);
        // Velocity bar in a recessed slot.
        auto slot = juce::Rectangle<float>(x + 3.f, vel.getY(), cw - 6.f, vel.getHeight());
        g.setColour(kDisplayBg); g.fillRoundedRectangle(slot, 3.f);
        g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawRoundedRectangle(slot, 3.f, 1.f);
        const float v = value(s, Vel);
        if (v > 0.001f) {
            auto bar = slot.reduced(2.f).withTop(slot.getBottom() - 2.f - (slot.getHeight() - 4.f) * v);
            const juce::Colour c = !inLen ? kTextFaint.withAlpha(0.4f) : now ? kAccent.brighter(0.3f) : kAccent.withAlpha(0.75f);
            if (now && inLen) { g.setColour(kAccent.withAlpha(0.25f)); g.fillRoundedRectangle(bar.expanded(3.f), 4.f); }
            g.setColour(c); g.fillRoundedRectangle(bar, 2.f);
        } else if (inLen) {
            g.setColour(kTextFaint); g.drawLine(slot.getCentreX() - 4.f, slot.getBottom() - 8.f, slot.getCentreX() + 4.f, slot.getBottom() - 8.f, 1.2f);
        }
        // Transpose cell.
        auto tc = juce::Rectangle<float>(x + 3.f, tr.getY(), cw - 6.f, tr.getHeight());
        g.setColour(kDisplayBg); g.fillRoundedRectangle(tc, 3.f);
        const int t = (int)std::lround(value(s, Tr));
        g.setFont(font(11.f, 500)); g.setColour(t == 0 ? kTextFaint : inLen ? kCurve : kTextFaint);
        g.drawText(t == 0 ? juce::String("0") : (t > 0 ? "+" : "") + juce::String(t), tc, juce::Justification::centred);
        // Ratchet cell: one dot per repeat.
        auto rc = juce::Rectangle<float>(x + 3.f, rat.getY(), cw - 6.f, rat.getHeight());
        g.setColour(kDisplayBg); g.fillRoundedRectangle(rc, 3.f);
        const int n = (int)std::lround(value(s, Rat));
        const float d = 4.f, span = n * d + (n - 1) * 3.f;
        for (int k = 0; k < n; ++k) {
            g.setColour(inLen ? (n > 1 ? kAccent : kTextDim) : kTextFaint);
            g.fillEllipse(rc.getCentreX() - span * 0.5f + k * (d + 3.f), rc.getCentreY() - d * 0.5f, d, d);
        }
        // Beat dividers.
        if (s % 4 == 0 && s > 0) { g.setColour(kLine); g.drawVerticalLine((int)x, vel.getY(), rat.getBottom()); }
    }
}

ArpPage::ArpPage(InfraProcessor& p) : Page(p), steps_(p) {
    toggle("arpOn", "Arp on"); toggle("arpLatch", "Latch");
    choice("arpMode", "Mode"); choice("arpDiv", "Rate");
    knob("arpOct", "Octaves", Knob::Small); knob("arpGate", "Gate", Knob::Small); knob("arpSwing", "Swing", Knob::Small);
    knob("arpLength", "Length", Knob::Small); knob("arpProb", "Chance", Knob::Small);
    addAndMakeVisible(steps_);
}
void ArpPage::tick() {
    steps_.tick();
    const bool on = proc_.apvts.getRawParameterValue("arpOn")->load() > 0.5f;
    for (const juce::String id : {"arpLatch", "arpMode", "arpDiv", "arpOct", "arpGate", "arpSwing", "arpLength", "arpProb"}) setOn(c(id), on);
    steps_.setAlpha(on ? 1.f : 0.55f);
}
void ArpPage::resized() {
    beginLayout();
    auto r = getLocalBounds();
    section("12", "Orbit arpeggiator", r.removeFromTop(14)); r.removeFromTop(4);
    auto top = r.removeFromTop(std::clamp(r.getHeight() / 4, 80, 110));
    auto tg = top.removeFromLeft(130);
    c("arpOn")->setBounds(tg.removeFromTop(tg.getHeight() / 2).withSizeKeepingCentre(tg.getWidth(), 24));
    c("arpLatch")->setBounds(tg.withSizeKeepingCentre(tg.getWidth(), 24));
    top.removeFromLeft(10);
    c("arpMode")->setBounds(top.removeFromLeft(150).withSizeKeepingCentre(150, 42)); top.removeFromLeft(12);
    c("arpDiv")->setBounds(top.removeFromLeft(110).withSizeKeepingCentre(110, 42)); top.removeFromLeft(16);
    row(top, {c("arpOct"), c("arpGate"), c("arpSwing"), c("arpLength"), c("arpProb")});
    r.removeFromTop(10);
    section("13", "Step lanes", r.removeFromTop(14)); r.removeFromTop(6);
    steps_.setBounds(r);
}

// ---- FX ---------------------------------------------------------------------------------------------------
FxPage::FxPage(InfraProcessor& p) : Page(p) {
    choice("drvMode", "Mode"); knob("drvAmt", "Drive"); knob("drvTone", "Tone"); knob("drvMix", "Mix");
    knob("chRate", "Rate"); knob("chDepth", "Depth"); knob("chMix", "Mix");
    toggle("dlySync", "Sync"); toggle("dlyPing", "Ping-pong"); choice("dlyDiv", "Time");
    knob("dlyMs", "Time"); knob("dlyFb", "Feedback"); knob("dlyDamp", "Damping"); knob("dlyMix", "Mix");
    knob("rvSize", "Size"); knob("rvDecay", "Decay"); knob("rvDamp", "Damping"); knob("rvPre", "Pre-delay"); knob("rvMix", "Mix");
    knob("width", "Width"); knob("outDb", "Output", Knob::Normal, true);
}
void FxPage::tick() {
    const bool sync = proc_.apvts.getRawParameterValue("dlySync")->load() > 0.5f;
    setOn(c("dlyMs"), !sync); setOn(c("dlyDiv"), sync);
}
void FxPage::resized() {
    beginLayout();
    auto r = getLocalBounds();
    const int gap = 20, W = r.getWidth() - gap * 4;
    auto col = [&](float share) { auto c = r.removeFromLeft((int)(W * share)); r.removeFromLeft(gap); return c; };
    auto drv = col(0.2f), cho = col(0.15f), dly = col(0.25f), rev = col(0.28f), out = r;
    auto head = [&](juce::Rectangle<int>& b, const char* code, const char* t) { section(code, t, b.removeFromTop(14)); b.removeFromTop(4); };
    head(drv, "14", "Drive");
    c("drvMode")->setBounds(drv.removeFromTop(44).reduced(0, 1)); drv.removeFromTop(4);
    c("drvAmt")->setBounds(drv.removeFromTop(drv.getHeight() / 2));
    row(drv, {c("drvTone"), c("drvMix")});
    head(cho, "15", "Aurora");
    const int ch = cho.getHeight() / 3;
    c("chRate")->setBounds(cho.removeFromTop(ch)); c("chDepth")->setBounds(cho.removeFromTop(ch)); c("chMix")->setBounds(cho);
    head(dly, "16", "Echo");
    {
        auto t = dly.removeFromTop(44);
        auto tg = t.removeFromLeft(t.getWidth() / 2);
        c("dlySync")->setBounds(tg.removeFromTop(22)); c("dlyPing")->setBounds(tg.removeFromTop(22));
        t.removeFromLeft(6); c("dlyDiv")->setBounds(t);
        dly.removeFromTop(4);
        row(dly.removeFromTop(dly.getHeight() / 2), {c("dlyMs"), c("dlyFb")});
        row(dly, {c("dlyDamp"), c("dlyMix")});
    }
    head(rev, "17", "Chromosphere reverb");
    row(rev.removeFromTop(rev.getHeight() / 2), {c("rvSize"), c("rvDecay"), c("rvDamp")});
    row(rev, {c("rvPre"), c("rvMix")});
    head(out, "18", "Output");
    c("width")->setBounds(out.removeFromTop(out.getHeight() / 2)); c("outDb")->setBounds(out);
}

// ---- Macros -----------------------------------------------------------------------------------------------
static const char* kMacroMeaning[4] = {"Brightness", "Weight", "Motion", "Space"};
static const char* kMacroName[4] = {"Heat", "Gravity", "Orbit", "Flux"};

MacroBay::MacroBay(InfraProcessor& p) {
    for (int m = 0; m < 4; ++m) {
        knobs_.push_back(std::make_unique<Knob>(p.apvts, "macro" + juce::String(m + 1), kMacroName[m], Knob::Large));
        addAndMakeVisible(*knobs_.back());
    }
}
void MacroBay::paint(juce::Graphics& g) {
    auto r = getLocalBounds();
    drawSection(g, r.removeFromTop(14).toFloat(), "M", "Macros");
    for (int m = 0; m < 4; ++m) {
        const auto kb = knobs_[(size_t)m]->getBounds();
        drawCaps(g, kMacroMeaning[m], juce::Rectangle<float>((float)kb.getX(), (float)kb.getBottom(), (float)kb.getWidth(), 12.f),
                 juce::Justification::centred, 7.5f, kTextFaint);
    }
}
void MacroBay::resized() {
    // One row of four: the macros stay the largest dials on the panel.
    auto r = getLocalBounds();
    r.removeFromTop(18);
    r.removeFromBottom(14);                     // meaning labels under the readouts
    const int w = r.getWidth() / 4;
    for (int m = 0; m < 4; ++m) knobs_[(size_t)m]->setBounds(r.removeFromLeft(w));
}

// ---- Main display -------------------------------------------------------------------------------------------
MainDisplay::MainDisplay(InfraProcessor& p, std::function<sun::Drive()> drive) : sun(sun::Mode::Limb, std::move(drive), false), proc_(p) {
    setTitle("Display"); setDescription("Wavetables, filter response, amp envelope, output scope and level");
    sun.overlay = [this](juce::Graphics& g) { paintScope(g); };
    addAndMakeVisible(sun);
}
void MainDisplay::resized() { sun.setBounds(getLocalBounds()); }
void MainDisplay::paint(juce::Graphics& g) { g.setColour(kDisplayBg); g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.f); }

void MainDisplay::paintScope(juce::Graphics& g) {
    // Everything the beam draws goes into a layer; a blurred half-resolution copy underneath is the
    // phosphor's halation, then faint raster scanlines cross the whole tube face, sun included.
    auto all = getLocalBounds().toFloat();
    drawDisplay(g, all, 8.f, false);
    const int W = std::max(1, getWidth()), H = std::max(1, getHeight());
    // Software images: they are drawn into, scaled and blurred on the CPU every frame. On Windows a native image
    // is a Direct2D bitmap, and each of those steps would copy it back from the GPU.
    if (!traceLayer_.isValid() || traceLayer_.getWidth() != W || traceLayer_.getHeight() != H)
        traceLayer_ = juce::Image(juce::Image::ARGB, W, H, true, juce::SoftwareImageType());
    else traceLayer_.clear(traceLayer_.getBounds(), juce::Colours::transparentBlack);
    { juce::Graphics lg(traceLayer_); paintTraces(lg, all); }
    juce::Image glow = traceLayer_.rescaled(std::max(1, W / 2), std::max(1, H / 2), juce::Graphics::mediumResamplingQuality);
    gaussianBlur(glow, 2, 1.8f);
    g.setOpacity(0.75f); g.drawImageTransformed(glow, juce::AffineTransform::scale((float)W / glow.getWidth(), (float)H / glow.getHeight()), false);
    g.setOpacity(1.f); g.drawImageAt(traceLayer_, 0, 0);
    juce::Graphics::ScopedSaveState ss(g);
    juce::Path clip; clip.addRoundedRectangle(all, 8.f); g.reduceClipRegion(clip);
    g.setColour(juce::Colours::black.withAlpha(0.13f));
    for (float y = all.getY() + 1.f; y < all.getBottom(); y += 3.f) g.fillRect(all.getX(), y, all.getWidth(), 1.f);
}

void MainDisplay::paintTraces(juce::Graphics& g, juce::Rectangle<float> all) {
    auto r = all.reduced(16.f, 12.f);
    if (caption.isNotEmpty()) {
        auto cap = r.removeFromBottom(14.f);
        g.setFont(font(11.f)); g.setColour(kAmberText);
        g.drawText(caption, cap, juce::Justification::centredLeft, true);
        r.removeFromBottom(4.f);
    }
    auto tables = r.removeFromLeft(r.getWidth() * 0.36f); r.removeFromLeft(18.f);
    auto mid = r.removeFromLeft(r.getWidth() * 0.46f); r.removeFromLeft(18.f);
    g.setColour(kAmberGrid);
    g.drawVerticalLine((int)(tables.getRight() + 9.f), r.getY(), r.getBottom());
    g.drawVerticalLine((int)(mid.getRight() + 9.f), r.getY(), r.getBottom());
    paintTable(g, tables.removeFromTop(tables.getHeight() * 0.5f).withTrimmedBottom(6.f), 0);
    paintTable(g, tables, 1);
    paintFilter(g, mid.removeFromTop(mid.getHeight() * 0.6f).withTrimmedBottom(8.f));
    paintEnvelope(g, mid);
    paintWave(g, r);
}

void MainDisplay::paintTable(juce::Graphics& g, juce::Rectangle<float> r, int o) {
    const juce::String x = o == 0 ? "a" : "b";
    const bool on = proc_.apvts.getRawParameterValue(x + "On")->load() > 0.5f;
    const int table = juce::jlimit(0, infra::kNumTables - 1, (int)std::lround(proc_.apvts.getRawParameterValue(x + "Table")->load()));
    const float morph = proc_.synth.morphShown[o].load(std::memory_order_relaxed);
    drawCaps(g, juce::String("Osc ") + (o == 0 ? "A" : "B") + "  " + infra::kTableNames[table], r.removeFromTop(12.f), juce::Justification::centredLeft, 9.f,
             on ? kAmberText : kAmberGrid);
    r.removeFromTop(4.f);
    const auto& bank = infra::WavetableBank::get();
    const int F = infra::WavetableBank::kFrames, pts = 72;
    // Frames recede up and to the right; the current (interpolated) frame is drawn in front, lit.
    const float dx = r.getWidth() * 0.16f / F, dy = r.getHeight() * 0.42f / F;
    const float w = r.getWidth() - dx * F, h = r.getHeight() - dy * F;
    auto framePath = [&](float pos, float ox, float oy) {
        const int f0 = std::min((int)pos, F - 2); const float wf = std::min(1.f, pos - (float)f0);
        const float* a = bank.frame(table, f0, 3); const float* b = bank.frame(table, f0 + 1, 3);
        juce::Path p;
        for (int i = 0; i <= pts; ++i) {
            const int idx = i * infra::WavetableBank::kSize / pts;
            const float v = a[idx] + (b[idx] - a[idx]) * wf;
            const juce::Point<float> q(r.getX() + ox + w * i / (float)pts, r.getBottom() - oy - h * 0.5f - v * h * 0.45f);
            if (i == 0) p.startNewSubPath(q); else p.lineTo(q);
        }
        return p;
    };
    for (int f = F - 1; f >= 0; --f) {
        const float near = std::abs(f - morph * (F - 1));
        g.setColour((on ? kAmber : kAmberGrid).withAlpha(on ? juce::jmap(std::min(near, 6.f), 0.f, 6.f, 0.42f, 0.10f) : 0.12f));
        g.strokePath(framePath((float)f, dx * f, dy * f), juce::PathStrokeType(0.8f));
    }
    if (on) {
        const float pos = morph * (F - 1);
        auto p = framePath(pos, dx * pos, dy * pos);
        g.setColour(kAmber); g.strokePath(p, juce::PathStrokeType(1.7f));
        g.setColour(kAmberHot.withAlpha(0.7f)); g.strokePath(p, juce::PathStrokeType(0.7f));
    }
}

void MainDisplay::paintFilter(juce::Graphics& g, juce::Rectangle<float> r) {
    auto& a = proc_.apvts;
    const int type = (int)std::lround(a.getRawParameterValue("fType")->load());
    const float fc = a.getRawParameterValue("fCutoff")->load(), res = a.getRawParameterValue("fRes")->load();
    const juce::String hz = fc >= 1000.f ? juce::String(fc / 1000.f, 2) + " kHz" : juce::String((int)fc) + " Hz";
    drawCaps(g, juce::String("Filter  ") + infra::kFilterNames[type] + "  " + hz, r.removeFromTop(12.f), juce::Justification::centredLeft, 9.f, kAmberText);
    r.removeFromTop(4.f);
    const float fmin = 20.f, fmax = 20000.f, dbTop = 18.f, dbBot = -36.f;
    auto X = [&](float f) { return r.getX() + r.getWidth() * std::log(f / fmin) / std::log(fmax / fmin); };
    auto Y = [&](float db) { return r.getY() + r.getHeight() * (dbTop - juce::jlimit(dbBot, dbTop, db)) / (dbTop - dbBot); };
    g.setColour(kAmberGrid);
    for (float f : {100.f, 1000.f, 10000.f}) g.drawVerticalLine((int)X(f), r.getY(), r.getBottom());
    g.drawHorizontalLine((int)Y(0.f), r.getX(), r.getRight());
    using C = std::complex<float>;
    auto mag = [&](float f) -> float {
        const C s(0.f, f / std::max(fc, 1.f));
        C H(1.f, 0.f);
        switch ((infra::FilterType)type) {
            case infra::FilterType::LP24: { const float k = 3.98f * res; const C p = (1.f + s) * (1.f + s); H = (1.f + 0.35f * k) / (p * p + k); break; }
            case infra::FilterType::LP12: { const float k = 1.414f * (1 - res) + 0.03f * res; H = 1.f / (s * s + k * s + 1.f); break; }
            case infra::FilterType::BP12: { const float k = 1.2f * (1 - res) + 0.06f * res; H = 1.4f * k * s / (s * s + k * s + 1.f); break; }
            case infra::FilterType::HP12: { const float k = 1.414f * (1 - res) + 0.03f * res; H = s * s / (s * s + k * s + 1.f); break; }
            case infra::FilterType::Notch: { const float k = 1.414f * (1 - res) + 0.03f * res; H = (s * s + 1.f) / (s * s + k * s + 1.f); break; }
            case infra::FilterType::HP24: { const float k = 0.765f * (1 - res) + 0.03f * res; H = s * s / (s * s + 1.848f * s + 1.f) * (s * s / (s * s + k * s + 1.f)); break; }
            case infra::FilterType::Comb: {
                const float fb = 0.2f + 0.77f * res;
                const float th = juce::MathConstants<float>::twoPi * f / std::max(fc, 20.f);
                H = (1.f - 0.75f * fb) / (C(1.f, 0.f) - fb * std::polar(1.f, -th));
                break;
            }
            case infra::FilterType::Formant: {
                static const float vow[5][3] = {{800, 1150, 2900}, {550, 1770, 2490}, {280, 2250, 2890}, {450, 800, 2830}, {325, 700, 2530}};
                const float pos = juce::jlimit(0.f, 1.f, std::log2(std::max(fc, 80.f) / 80.f) / std::log2(100.f)) * 4.f;
                const int i = std::min(3, (int)pos); const float fr = pos - (float)i;
                const float k = 0.32f - 0.26f * res, gains[3] = {1.f, 0.7f, 0.45f};
                H = 0.f;
                for (int j = 0; j < 3; ++j) {
                    const float F0 = std::exp(std::log(vow[i][j]) * (1 - fr) + std::log(vow[i + 1][j]) * fr);
                    const C sj(0.f, f / F0);
                    H += gains[j] * k * sj / (sj * sj + k * sj + 1.f);
                }
                H *= 2.2f * 1.6f * k;
                break;
            }
        }
        return 20.f * std::log10(std::max(std::abs(H), 1e-5f));
    };
    juce::Path curve, fill;
    for (int i = 0; i <= 140; ++i) {
        const float f = fmin * std::pow(fmax / fmin, i / 140.f);
        const juce::Point<float> p(X(f), Y(mag(f)));
        if (i == 0) { curve.startNewSubPath(p); fill.startNewSubPath(p.x, r.getBottom()); }
        curve.lineTo(p); fill.lineTo(p);
    }
    fill.lineTo(r.getRight(), r.getBottom()); fill.closeSubPath();
    g.setGradientFill(juce::ColourGradient(kAmber.withAlpha(0.16f), 0.f, r.getY(), kAmber.withAlpha(0.f), 0.f, r.getBottom(), false));
    g.fillPath(fill);
    g.setColour(kAmber); g.strokePath(curve, juce::PathStrokeType(1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(kAmberHot.withAlpha(0.7f)); g.strokePath(curve, juce::PathStrokeType(0.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(kDisplayBg); g.fillEllipse(X(juce::jlimit(fmin, fmax, fc)) - 3.5f, Y(mag(fc)) - 3.5f, 7.f, 7.f);
    g.setColour(kAmber); g.drawEllipse(X(juce::jlimit(fmin, fmax, fc)) - 3.f, Y(mag(fc)) - 3.f, 6.f, 6.f, 1.4f);
}

void MainDisplay::paintEnvelope(juce::Graphics& g, juce::Rectangle<float> r) {
    auto& a = proc_.apvts;
    const float A = a.getRawParameterValue("ampA")->load(), D = a.getRawParameterValue("ampD")->load(),
                S = a.getRawParameterValue("ampS")->load(), R = a.getRawParameterValue("ampR")->load();
    drawCaps(g, "Amp envelope", r.removeFromTop(12.f), juce::Justification::centredLeft, 9.f, kAmberText);
    r.removeFromTop(4.f);
    // Time on a compressed axis so 5 ms and 5 s both read.
    auto seg = [](float t) { return std::sqrt(std::max(t, 0.f) + 0.0005f); };
    const float hold = 0.35f, total = seg(A) + seg(D) + hold + seg(R);
    auto X = [&](float u) { return r.getX() + r.getWidth() * u / total; };
    juce::Path p;
    p.startNewSubPath(X(0.f), r.getBottom());
    p.lineTo(X(seg(A)), r.getY());
    for (int i = 1; i <= 24; ++i) { const float u = i / 24.f; p.lineTo(X(seg(A) + seg(D) * u), r.getBottom() - r.getHeight() * (S + (1.f - S) * std::exp(-4.6f * u))); }
    p.lineTo(X(seg(A) + seg(D) + hold), r.getBottom() - r.getHeight() * S);
    for (int i = 1; i <= 24; ++i) { const float u = i / 24.f; p.lineTo(X(seg(A) + seg(D) + hold + seg(R) * u), r.getBottom() - r.getHeight() * S * std::exp(-4.6f * u)); }
    g.setColour(kAmberGrid); g.drawHorizontalLine((int)r.getBottom(), r.getX(), r.getRight());
    g.setColour(kAmber.withAlpha(0.85f)); g.strokePath(p, juce::PathStrokeType(1.3f));
}

void MainDisplay::paintWave(juce::Graphics& g, juce::Rectangle<float> r) {
    auto& syn = proc_.synth;
    const int voices = syn.activeVoices.load(std::memory_order_relaxed);
    drawCaps(g, "Output  " + juce::String(voices) + (voices == 1 ? " voice" : " voices"), r.removeFromTop(12.f), juce::Justification::centredLeft, 9.f, kAmberText);
    r.removeFromTop(4.f);
    auto meter = r.removeFromRight(14.f); r.removeFromRight(10.f);
    // Triggered scope: the newest 1024 samples, aligned to a rising zero crossing.
    const int N = infra::Synth::kScopeSize, show = 512;
    const int pos = syn.scopePos.load(std::memory_order_acquire);
    float buf[infra::Synth::kScopeSize];
    for (int i = 0; i < N; ++i) buf[i] = syn.scope[(size_t)((pos + i) % N)].load(std::memory_order_relaxed);
    int start = N - show - 1;
    for (int i = N - show - 1; i > N / 4; --i) if (buf[i - 1] <= 0.f && buf[i] > 0.f) { start = i; break; }
    g.setColour(kAmberGrid); g.drawHorizontalLine((int)r.getCentreY(), r.getX(), r.getRight());
    juce::Path p;
    for (int i = 0; i < show; ++i) {
        const juce::Point<float> q(r.getX() + r.getWidth() * i / (float)(show - 1), r.getCentreY() - juce::jlimit(-1.f, 1.f, buf[start + i] * 1.2f) * r.getHeight() * 0.48f);
        if (i == 0) p.startNewSubPath(q); else p.lineTo(q);
    }
    g.setColour(kAmber); g.strokePath(p, juce::PathStrokeType(1.5f));
    g.setColour(kAmberHot.withAlpha(0.7f)); g.strokePath(p, juce::PathStrokeType(0.6f));
    // Output meter and limiter lamp.
    const float pk = syn.peak.load(std::memory_order_relaxed);
    const float db = 20.f * std::log10(std::max(pk, 1e-5f)), frac = juce::jlimit(0.f, 1.f, (db + 60.f) / 60.f);
    auto bar = meter.withTrimmedTop(10.f);
    g.setColour(kAmberGrid); g.fillRoundedRectangle(bar, 2.f);
    g.setGradientFill(juce::ColourGradient(db > -3.f ? kDanger : kAmberHot, 0.f, bar.getY(), kAmber, 0.f, bar.getBottom(), false));
    g.fillRoundedRectangle(bar.withTop(bar.getBottom() - bar.getHeight() * frac), 2.f);
    const bool limiting = syn.limiterGain.load(std::memory_order_relaxed) < 0.97f;
    g.setColour(limiting ? kDanger : juce::Colour(0xff3a3134)); g.fillEllipse(meter.getCentreX() - 3.f, meter.getY(), 6.f, 6.f);
}

}  // namespace panels
