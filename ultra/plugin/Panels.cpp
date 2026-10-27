#include "Panels.h"
#include "PluginProcessor.h"
#include "ultra/voice.h"

using namespace look;
using namespace ultra;

namespace panels {

// The scope is an amber CRT, the same instrument as on EXO and INFRA: P3 phosphor body, a hotter near-white core,
// dim amber for grid and labels. It reads clearly over the violet limb.
static const juce::Colour kAmber(0xffffb000), kAmberHot(0xffffe3a6), kAmberText(0xb8ffb000), kAmberGrid(0x29ffb000);

static float raw(UltraProcessor& p, const juce::String& id) { return p.apvts.getRawParameterValue(id)->load(); }
static int rawI(UltraProcessor& p, const juce::String& id) { return (int)std::lround(raw(p, id)); }
static void setPlain(UltraProcessor& p, const juce::String& id, float plain) {
    if (auto* prm = p.apvts.getParameter(id)) {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost(prm->convertTo0to1(plain));
        prm->endChangeGesture();
    }
}
static int editPattern(UltraProcessor& p) { return juce::jlimit(0, kPatterns - 1, rawI(p, "pattern")); }
static juce::String trackTitle(UltraProcessor& p, int t) { return juce::String(t + 1).paddedLeft('0', 2) + "  " + p.patterns.trackName(t); }

// ---- Controls (INFRA) ------------------------------------------------------------------------------------
Knob::Knob(APVTS& s, const juce::String& id, const juce::String& title, Size size, bool bipolar) : paramId(id), title_(title), size_(size) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 18);
    slider.setTitle(title); slider.setDescription(title);
    slider.getProperties().set("bipolar", bipolar);
    auto* prm = s.getParameter(id);
    jassert(prm != nullptr);
    slider.setDoubleClickReturnValue(true, prm->convertFrom0to1(prm->getDefaultValue()));
    addAndMakeVisible(slider);
    att_ = std::make_unique<APVTS::SliderAttachment>(s, id, slider);
    setWantsKeyboardFocus(false);
}

void Knob::paint(juce::Graphics& g) {
    const float h = size_ == Large ? 11.5f : size_ == Small ? 9.f : 10.f;
    drawCaps(g, title_, label_.toFloat(), juce::Justification::centred, h, slider.isEnabled() ? (size_ == Large ? kText : kTextDim) : kTextFaint);
}

void Knob::resized() {
    // Title, dial, a band of air, then the seven-segment readout (EXO); the air narrows on dense pages.
    const int nameH = size_ == Large ? 17 : size_ == Small ? 13 : 15;
    const int textH = size_ == Large ? 22 : size_ == Small ? 17 : 18;
    const int gap = size_ == Large ? look::kDialReadoutGap : size_ == Small ? 5 : 7;
    slider.getProperties().set("readoutGap", gap);
    const int maxSide = size_ == Large ? 84 : size_ == Small ? 46 : 64;
    const int side = std::clamp(std::min(getWidth() - 6, getHeight() - nameH - textH - gap - 4), 24, maxSide);
    const int groupH = nameH + 2 + side + gap + textH, top = std::max(0, (getHeight() - groupH) / 2);
    label_ = {0, top, getWidth(), nameH};
    const int boxW = std::min(getWidth() - 2, 124), sw = std::max(side + 16, boxW + 2);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, boxW, textH);
    slider.setBounds((getWidth() - sw) / 2, top + nameH + 2, sw, side + gap + textH);
}

Choice::Choice(APVTS& s, const juce::String& id, const juce::String& title) : title_(title) {
    auto* prm = dynamic_cast<juce::AudioParameterChoice*>(s.getParameter(id));
    jassert(prm != nullptr);
    box.addItemList(prm->choices, 1);
    box.setTitle(title);
    addAndMakeVisible(box);
    att_ = std::make_unique<APVTS::ComboBoxAttachment>(s, id, box);
}
void Choice::paint(juce::Graphics& g) {
    if (title_.isNotEmpty()) drawCaps(g, title_, getLocalBounds().removeFromTop(14).toFloat(), juce::Justification::centredLeft, 9.f, box.isEnabled() ? kTextFaint : kTextFaint.withAlpha(0.5f));
}
void Choice::resized() {
    auto r = getLocalBounds();
    if (title_.isNotEmpty()) r.removeFromTop(15);
    box.setBounds(r.withSizeKeepingCentre(r.getWidth(), std::min(r.getHeight(), 26)));
}

Toggle::Toggle(APVTS& s, const juce::String& id, const juce::String& title) {
    button.setButtonText(title); button.setTitle(title);
    addAndMakeVisible(button);
    att_ = std::make_unique<APVTS::ButtonAttachment>(s, id, button);
}

Switch::Switch(APVTS& s, const juce::String& id, const juce::String& text, const juce::String& title) {
    button.setButtonText(text); button.setTitle(title);
    button.setClickingTogglesState(true);
    addAndMakeVisible(button);
    att_ = std::make_unique<APVTS::ButtonAttachment>(s, id, button);
}

// ---- Page (INFRA) ----------------------------------------------------------------------------------------------
Page::Page(UltraProcessor& p, UiState& ui) : proc_(p), ui_(ui) {}

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
Switch& Page::sw(const juce::String& id, const juce::String& text, const juce::String& title) {
    auto k = std::make_unique<Switch>(proc_.apvts, id, text, title);
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
        if (prev && c && prev->isVisible() && c->isVisible() && dynamic_cast<Knob*>(prev) && dynamic_cast<Knob*>(c)) {
            const float x = (float)cell.getX() - gap * 0.5f;
            dividers_.push_back({x, (float)cell.getY() + 10.f, x, (float)cell.getBottom() - 6.f});
        }
        prev = c;
        area.removeFromLeft(gap);
    }
}

static void setOn(juce::Component* c, bool on) { if (c && c->isEnabled() != on) { c->setEnabled(on); c->setAlpha(on ? 1.f : 0.45f); } }

// ---- Step grid -------------------------------------------------------------------------------------------------
StepGrid::StepGrid(UltraProcessor& p, UiState& ui) : proc_(p), ui_(ui) {
    setTitle("Step grid"); setDescription("Eight tracks of sixteen steps: click a step to toggle it, drag to paint");
    for (auto& s : lastStep_) s = -1;
}

int StepGrid::pattern() const { return editPattern(proc_); }

juce::Rectangle<float> StepGrid::rowRect(int t) const {
    const float h = getHeight() / (float)kTracks;
    return {0.f, h * t, (float)getWidth(), h};
}
juce::Rectangle<float> StepGrid::labelRect(int t) const {
    return rowRect(t).withWidth(std::clamp(getWidth() * 0.16f, 130.f, 200.f)).reduced(0.f, 2.f);
}
juce::Rectangle<float> StepGrid::lengthRect(int t) const { auto r = rowRect(t).reduced(0.f, 3.f); return r.withX(r.getRight() - 96.f).withWidth(40.f); }
juce::Rectangle<float> StepGrid::rateRect(int t) const { auto r = rowRect(t).reduced(0.f, 3.f); return r.withX(r.getRight() - 52.f).withWidth(52.f); }
juce::Rectangle<float> StepGrid::stepRect(int t, int col) const {
    const auto row = rowRect(t);
    const float x0 = labelRect(t).getRight() + 12.f, x1 = row.getRight() - 108.f;
    const float groupGap = 8.f, cellGap = 3.f;
    const float cw = (x1 - x0 - 3.f * groupGap - 12.f * cellGap) / 16.f;
    const float x = x0 + col * (cw + cellGap) + (float)(col / 4) * (groupGap - cellGap);
    return juce::Rectangle<float>(x, row.getY(), cw, row.getHeight()).reduced(0.f, 3.f);
}

StepGrid::Hit StepGrid::hitTest(juce::Point<int> p) const {
    const auto q = p.toFloat();
    for (int t = 0; t < kTracks; ++t) {
        if (!rowRect(t).contains(q)) continue;
        const auto lab = labelRect(t);
        if (lab.contains(q)) {
            if (q.x > lab.getRight() - 24.f) return {Solo, t, -1};
            if (q.x > lab.getRight() - 46.f) return {Mute, t, -1};
            return {Label, t, -1};
        }
        if (lengthRect(t).contains(q)) return {Length, t, -1};
        if (rateRect(t).contains(q)) return {Rate, t, -1};
        for (int c = 0; c < 16; ++c) if (stepRect(t, c).expanded(1.5f, 3.f).contains(q)) return {Step, t, ui_.stepPage * 16 + c};
        return {None, t, -1};
    }
    return {};
}

void StepGrid::mouseDown(const juce::MouseEvent& e) {
    const auto h = hitTest(e.getPosition());
    dragPart_ = h.part; dragTrack_ = h.track;
    switch (h.part) {
        case Label: ui_.select(h.track); break;
        case Mute: setPlain(proc_, UltraProcessor::tid(h.track, "Mute"), raw(proc_, UltraProcessor::tid(h.track, "Mute")) > 0.5f ? 0.f : 1.f); break;
        case Solo: setPlain(proc_, UltraProcessor::tid(h.track, "Solo"), raw(proc_, UltraProcessor::tid(h.track, "Solo")) > 0.5f ? 0.f : 1.f); break;
        case Step: {
            ui_.select(h.track);
            proc_.undoManager.beginNewTransaction("Edit steps");
            const int v = proc_.patterns.lane(pattern(), h.track, h.step, PatternStore::Vel);
            dragVel_ = v > 0 ? 0 : 100;
            proc_.patterns.setLane(pattern(), h.track, h.step, PatternStore::Vel, dragVel_);
            break;
        }
        case Length:
            proc_.undoManager.beginNewTransaction("Track length");
            lenDragStart_ = proc_.patterns.track(pattern(), h.track).length; lenDragY_ = (float)e.y;
            break;
        case Rate: {
            juce::PopupMenu m;
            m.addSectionHeader("Step rate");
            const int cur = proc_.patterns.track(pattern(), h.track).rate;
            for (int i = 0; i < kNumRates; ++i) m.addItem(i + 1, kRateNames[i], true, i == cur);
            const int t = h.track, pat = pattern();
            m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(localAreaToGlobal(rateRect(t).toNearestInt())), [this, t, pat](int r) {
                if (r <= 0) return;
                proc_.undoManager.beginNewTransaction("Track rate");
                proc_.patterns.setRate(pat, t, r - 1);
            });
            break;
        }
        case None: break;
    }
}

void StepGrid::mouseDrag(const juce::MouseEvent& e) {
    if (dragPart_ == Step && dragTrack_ >= 0) {
        for (int c = 0; c < 16; ++c)
            if (stepRect(dragTrack_, c).getHorizontalRange().expanded(1.5f).contains((float)e.x)) {
                const int s = ui_.stepPage * 16 + c;
                if ((proc_.patterns.lane(pattern(), dragTrack_, s, PatternStore::Vel) > 0) != (dragVel_ > 0))
                    proc_.patterns.setLane(pattern(), dragTrack_, s, PatternStore::Vel, dragVel_);
            }
    } else if (dragPart_ == Length && dragTrack_ >= 0) {
        const int len = juce::jlimit(1, kMaxSteps, lenDragStart_ + (int)std::lround((lenDragY_ - (float)e.y) / 6.f));
        if (len != proc_.patterns.track(pattern(), dragTrack_).length) proc_.patterns.setLength(pattern(), dragTrack_, len);
    }
}
void StepGrid::mouseUp(const juce::MouseEvent&) { dragPart_ = None; dragTrack_ = -1; }
void StepGrid::mouseDoubleClick(const juce::MouseEvent& e) {
    const auto h = hitTest(e.getPosition());
    if (h.part == Length) { proc_.undoManager.beginNewTransaction("Track length"); proc_.patterns.setLength(pattern(), h.track, 16); }
}

void StepGrid::tick() {
    bool all = false;
    const int ver = proc_.patterns.version.load();
    if (ver != paintVer_) { paintVer_ = ver; all = true; }
    const int pat = pattern();
    if (pat != lastPattern_) { lastPattern_ = pat; all = true; }
    const bool running = proc_.machine.running.load();
    if (running != lastRunning_) { lastRunning_ = running; all = true; }
    for (int t = 0; t < kTracks; ++t) {
        bool row = false;
        const int s = running ? proc_.machine.stepNow[(size_t)t].load() : -1;
        if (s != lastStep_[t]) { lastStep_[t] = s; row = true; }
        const int h = proc_.machine.hitCount[(size_t)t].load();
        if (h != lastHits_[t]) { lastHits_[t] = h; flash_[t] = 0.4f + 0.6f * proc_.machine.hitVelocity[(size_t)t].load(); row = true; }
        else if (flash_[t] > 0.f) { flash_[t] = std::max(0.f, flash_[t] - 0.18f); row = true; }
        // Mute and solo lamps follow the host too.
        const int lamps = (raw(proc_, UltraProcessor::tid(t, "Mute")) > 0.5f ? 1 : 0) | (raw(proc_, UltraProcessor::tid(t, "Solo")) > 0.5f ? 2 : 0);
        if (lamps != lastLamps_[t]) { lastLamps_[t] = lamps; row = true; }
        if (row && !all) repaint(rowRect(t).toNearestInt());
    }
    if (all) repaint();
}

void StepGrid::paint(juce::Graphics& g) {
    const int pat = pattern();
    const bool showHead = proc_.machine.running.load() && proc_.machine.currentPattern.load() == pat;
    for (int t = 0; t < kTracks; ++t) {
        const auto& tp = proc_.patterns.track(pat, t);
        const bool sel = t == ui_.track;
        const bool muted = raw(proc_, UltraProcessor::tid(t, "Mute")) > 0.5f, solo = raw(proc_, UltraProcessor::tid(t, "Solo")) > 0.5f;
        // Label: number, hit lamp, name, mute and solo.
        auto lab = labelRect(t);
        g.setColour(sel ? juce::Colour(0xff221f2a) : kDisplayBg.withAlpha(0.6f)); g.fillRoundedRectangle(lab, 4.f);
        if (sel) { g.setColour(kAccent); g.fillRoundedRectangle(lab.withWidth(2.5f).reduced(0.f, 4.f), 1.f); }
        auto l = lab.reduced(8.f, 0.f);
        g.setFont(font(10.f, 600)); g.setColour(kAccent.withAlpha(sel ? 1.f : 0.7f));
        g.drawText(juce::String(t + 1).paddedLeft('0', 2), l.removeFromLeft(18.f), juce::Justification::centredLeft);
        const float lr = 3.f;
        auto lamp = l.removeFromLeft(12.f);
        g.setColour(flash_[t] > 0.f ? kAccent.withAlpha(0.25f + 0.75f * flash_[t]) : juce::Colour(0xff2c2933));
        g.fillEllipse(lamp.getCentreX() - lr, lamp.getCentreY() - lr, 2 * lr, 2 * lr);
        auto ms = l.removeFromRight(44.f);
        auto mRect = ms.removeFromLeft(20.f).withSizeKeepingCentre(19.f, 16.f), sRect = ms.removeFromRight(20.f).withSizeKeepingCentre(19.f, 16.f);
        g.setColour(muted ? kAccentDim.brighter(0.4f) : juce::Colour(0xff201e25)); g.fillRoundedRectangle(mRect, 3.f);
        g.setColour(solo ? kAccent : juce::Colour(0xff201e25)); g.fillRoundedRectangle(sRect, 3.f);
        g.setFont(font(9.5f, 600));
        g.setColour(muted ? kText : kTextFaint); g.drawText("M", mRect, juce::Justification::centred);
        g.setColour(solo ? kBg : kTextFaint); g.drawText("S", sRect, juce::Justification::centred);
        drawCaps(g, proc_.patterns.trackName(t), l.reduced(2.f, 0.f), juce::Justification::centredLeft, 9.5f, muted ? kTextFaint : sel ? kText : kTextDim);

        // Steps.
        const int len = tp.length;
        const int head = showHead ? lastStep_[t] : -1;
        for (int col = 0; col < 16; ++col) {
            const int s = ui_.stepPage * 16 + col;
            auto r = stepRect(t, col);
            const bool in = s < len;
            const auto& st = tp.steps[(size_t)s];
            g.setColour(kDisplayBg.withAlpha(in ? 1.f : 0.45f)); g.fillRoundedRectangle(r, 3.f);
            g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawRoundedRectangle(r, 3.f, 1.f);
            if (col % 4 == 0 && in) { g.setColour(juce::Colours::white.withAlpha(0.05f)); g.drawRoundedRectangle(r.reduced(1.f), 2.f, 1.f); }
            if (st.vel > 0) {
                const float v = st.vel / 127.f;
                auto f = r.reduced(2.5f);
                const juce::Colour c = in ? kAccent.withAlpha(0.25f + 0.75f * v) : kTextFaint.withAlpha(0.35f);
                if (in && s == head) { g.setColour(kAccent.withAlpha(0.3f)); g.fillRoundedRectangle(r.expanded(1.5f), 4.f); }
                g.setColour(muted ? c.withMultipliedAlpha(0.5f) : c); g.fillRoundedRectangle(f, 2.f);
                // Marks: ratchet dots, a chance notch, a condition tag, a lock tick.
                const juce::Colour ink = juce::Colour(0xff0d0b12).withAlpha(0.85f);
                if (st.ratchet > 1) {
                    const float d = 3.f, span = st.ratchet * d + (st.ratchet - 1) * 2.f;
                    g.setColour(ink);
                    for (int k = 0; k < st.ratchet; ++k) g.fillEllipse(f.getCentreX() - span * 0.5f + k * (d + 2.f), f.getBottom() - 6.f, d, d);
                }
                if (st.chance < 100) {
                    juce::Path notch; notch.addTriangle(f.getRight() - 7.f, f.getY(), f.getRight(), f.getY(), f.getRight(), f.getY() + 7.f);
                    g.setColour(ink); g.fillPath(notch);
                }
                if (st.cond != 0) { g.setColour(ink); g.setFont(font(8.f, 600)); g.drawText(kCondNames[st.cond], f.reduced(2.f, 1.f), juce::Justification::topLeft, false); }
                if (st.lock != 0) { g.setColour(ink); g.fillRect(f.getX() + 2.f, f.getCentreY() - 0.5f, 5.f, 1.5f); }
            }
            if (in && s == head) { g.setColour(kText.withAlpha(0.85f)); g.drawRoundedRectangle(r.expanded(1.f), 3.5f, 1.4f); }
        }
        // Length and rate.
        auto lr2 = lengthRect(t), rr = rateRect(t);
        g.setColour(kDisplayBg); g.fillRoundedRectangle(lr2, 3.f); g.fillRoundedRectangle(rr, 3.f);
        g.setFont(font(11.f, 500)); g.setColour(kTextDim);
        g.drawText(juce::String(len), lr2, juce::Justification::centred);
        g.drawText(kRateNames[juce::jlimit(0, kNumRates - 1, tp.rate)], rr, juce::Justification::centred);
    }
    // Column headers would crowd the rows; the beat groups and the length column carry the structure.
}

// ---- Lane editor -----------------------------------------------------------------------------------------------
static const char* kLaneNames[PatternStore::NumLanes] = {"Velocity", "Chance", "Ratchet", "Pitch", "Nudge", "Condition", "Lock"};

LaneEditor::LaneEditor(UltraProcessor& p, UiState& ui) : proc_(p), ui_(ui) {
    setTitle("Lane editor"); setDescription("Per-step velocity, chance, ratchet, pitch, nudge, condition and lock of the selected track");
}
juce::Rectangle<float> LaneEditor::laneList() const { return getLocalBounds().toFloat().removeFromLeft(110.f); }
juce::Rectangle<float> LaneEditor::area() const { auto r = getLocalBounds().toFloat(); r.removeFromLeft(122.f); r.removeFromTop(16.f); return r; }
int LaneEditor::stepAt(float x) const {
    const auto a = area();
    if (x < a.getX() || x >= a.getRight()) return -1;
    return ui_.stepPage * 16 + juce::jlimit(0, 15, (int)((x - a.getX()) / (a.getWidth() / 16.f)));
}

void LaneEditor::setFromY(int step, float y) {
    const auto lane = (PatternStore::Lane)ui_.lane;
    const auto a = area().reduced(0.f, 2.f);
    const float u = juce::jlimit(0.f, 1.f, (a.getBottom() - y) / a.getHeight());
    const int lo = PatternStore::laneMin(lane), hi = PatternStore::laneMax(lane);
    int v = (int)std::lround(lo + u * (hi - lo));
    if (lane == PatternStore::Ratchet) v = juce::jlimit(1, 4, 1 + (int)(u * 4.f));
    if (v != proc_.patterns.lane(editPattern(proc_), ui_.track, step, lane)) proc_.patterns.setLane(editPattern(proc_), ui_.track, step, lane, v);
}

void LaneEditor::mouseDown(const juce::MouseEvent& e) {
    const auto list = laneList();
    if (list.contains(e.position)) {
        ui_.lane = juce::jlimit(0, (int)PatternStore::NumLanes - 1, (int)((e.position.y - list.getY()) / (list.getHeight() / (float)PatternStore::NumLanes)));
        repaint();
        return;
    }
    const int s = stepAt(e.position.x);
    if (s < 0) return;
    const auto lane = (PatternStore::Lane)ui_.lane;
    if (lane == PatternStore::Cond) {
        juce::PopupMenu m;
        m.addSectionHeader("Step " + juce::String(s + 1) + " plays");
        const int cur = proc_.patterns.lane(editPattern(proc_), ui_.track, s, lane);
        for (int i = 0; i < kNumConds; ++i) m.addItem(i + 1, kCondNames[i], true, i == cur);
        const int t = ui_.track, pat = editPattern(proc_);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMousePosition(), [this, t, pat, s](int r) {
            if (r <= 0) return;
            proc_.undoManager.beginNewTransaction("Step condition");
            proc_.patterns.setLane(pat, t, s, PatternStore::Cond, r - 1);
        });
        return;
    }
    proc_.undoManager.beginNewTransaction("Edit lane");
    setFromY(s, e.position.y);
}
void LaneEditor::mouseDrag(const juce::MouseEvent& e) {
    if (ui_.lane == PatternStore::Cond || laneList().contains(e.mouseDownPosition)) return;
    const int s = stepAt(e.position.x);
    if (s >= 0) setFromY(s, e.position.y);
}
void LaneEditor::mouseDoubleClick(const juce::MouseEvent& e) {
    const int s = stepAt(e.position.x);
    if (s < 0 || ui_.lane == PatternStore::Cond) return;
    const auto lane = (PatternStore::Lane)ui_.lane;
    proc_.undoManager.beginNewTransaction("Reset step");
    proc_.patterns.setLane(editPattern(proc_), ui_.track, s, lane, lane == PatternStore::Vel ? 100 : PatternStore::laneDefault(lane));
}

void LaneEditor::tick() {
    const int ver = proc_.patterns.version.load();
    const int st = proc_.machine.running.load() ? proc_.machine.stepNow[(size_t)ui_.track].load() : -1;
    if (ver != paintVer_ || st != lastStep_ || ui_.track != lastTrack_) { paintVer_ = ver; lastStep_ = st; lastTrack_ = ui_.track; repaint(); }
}

void LaneEditor::paint(juce::Graphics& g) {
    const int pat = editPattern(proc_), t = ui_.track;
    const auto& tp = proc_.patterns.track(pat, t);
    const auto lane = (PatternStore::Lane)ui_.lane;
    // Lane list.
    auto list = laneList();
    const float rh = list.getHeight() / (float)PatternStore::NumLanes;
    for (int l = 0; l < PatternStore::NumLanes; ++l) {
        auto r = juce::Rectangle<float>(list.getX(), list.getY() + rh * l, list.getWidth(), rh).reduced(0.f, 1.5f);
        if (l == ui_.lane) { g.setColour(juce::Colour(0xff221f2a)); g.fillRoundedRectangle(r, 4.f); g.setColour(kAccent); g.fillRoundedRectangle(r.withWidth(2.5f).reduced(0.f, 3.f), 1.f); }
        juce::String name = kLaneNames[l];
        if (l == PatternStore::Lock) name << "  " << kLockNames[juce::jlimit(0, kNumLockTargets - 1, rawI(proc_, UltraProcessor::tid(t, "Lock")))];
        drawCaps(g, name, r.reduced(10.f, 0.f), juce::Justification::centredLeft, 9.f, l == ui_.lane ? kText : kTextDim);
    }
    // Step numbers and the lane.
    const auto a = area();
    const float cw = a.getWidth() / 16.f;
    const bool showHead = proc_.machine.running.load() && proc_.machine.currentPattern.load() == pat;
    const int lo = PatternStore::laneMin(lane), hi = PatternStore::laneMax(lane);
    const bool bipolar = lo < 0;
    for (int col = 0; col < 16; ++col) {
        const int s = ui_.stepPage * 16 + col;
        const bool in = s < tp.length, now = showHead && s == lastStep_;
        const float x = a.getX() + cw * col;
        g.setFont(font(9.5f, 600)); g.setColour(now ? kAccent : in ? kTextDim : kTextFaint.withAlpha(0.5f));
        g.drawText(juce::String(s + 1), juce::Rectangle<float>(x, 0.f, cw, 14.f), juce::Justification::centred);
        auto slot = juce::Rectangle<float>(x + 3.f, a.getY(), cw - 6.f, a.getHeight());
        g.setColour(kDisplayBg.withAlpha(in ? 1.f : 0.45f)); g.fillRoundedRectangle(slot, 3.f);
        g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawRoundedRectangle(slot, 3.f, 1.f);
        const auto& st = tp.steps[(size_t)s];
        const int v = proc_.patterns.lane(pat, t, s, lane);
        const bool hit = st.vel > 0;
        const juce::Colour col_ = !in ? kTextFaint.withAlpha(0.4f) : !hit && lane != PatternStore::Vel ? kTextFaint.withAlpha(0.6f)
                                  : now ? kAccent.brighter(0.3f) : kAccent.withAlpha(0.8f);
        auto inner = slot.reduced(2.f);
        if (lane == PatternStore::Cond) {
            g.setFont(font(10.f, 500)); g.setColour(v == 0 ? kTextFaint : col_);
            g.drawText(v == 0 ? juce::String("-") : juce::String(kCondNames[v]), inner, juce::Justification::centred);
        } else if (lane == PatternStore::Ratchet) {
            const float d = std::min(6.f, inner.getWidth() * 0.18f);
            for (int k = 0; k < v; ++k) { g.setColour(col_); g.fillEllipse(inner.getCentreX() - d * 0.5f, inner.getBottom() - (k + 1) * (inner.getHeight() / 4.5f), d, d); }
        } else if (bipolar) {
            const float mid = inner.getCentreY(), u = (float)v / (float)hi;
            g.setColour(kLine); g.drawHorizontalLine((int)mid, inner.getX(), inner.getRight());
            if (v != 0) {
                auto bar = u > 0 ? juce::Rectangle<float>(inner.getX(), mid - u * inner.getHeight() * 0.5f, inner.getWidth(), u * inner.getHeight() * 0.5f)
                                 : juce::Rectangle<float>(inner.getX(), mid, inner.getWidth(), -u * inner.getHeight() * 0.5f);
                g.setColour(col_); g.fillRoundedRectangle(bar, 2.f);
                g.setFont(font(9.f, 600)); g.setColour(kText.withAlpha(0.8f));
                g.drawText((v > 0 ? "+" : "") + juce::String(v), inner.withHeight(12.f).withY(u > 0 ? inner.getBottom() - 13.f : inner.getY() + 1.f), juce::Justification::centred);
            }
        } else {
            const float u = (float)(v - lo) / (float)(hi - lo);
            if (v > lo) { g.setColour(col_); g.fillRoundedRectangle(inner.withTop(inner.getBottom() - inner.getHeight() * u), 2.f); }
        }
        if (col % 4 == 0 && col > 0) { g.setColour(kLine); g.drawVerticalLine((int)(x - 0.5f), a.getY(), a.getBottom()); }
    }
}

// ---- SEQ page --------------------------------------------------------------------------------------------------
SeqPage::SeqPage(UltraProcessor& p, UiState& ui) : Page(p, ui), grid_(p, ui), lanes_(p, ui), rng_(0x5eed) {
    addAndMakeVisible(grid_); addAndMakeVisible(lanes_);
    page1_.setButtonText("1-16"); page2_.setButtonText("17-32");
    copy_.setButtonText("Copy"); paste_.setButtonText("Paste"); clear_.setButtonText("Clear");
    left_.setButtonText("<"); right_.setButtonText(">"); mutate_.setButtonText("Mutate"); patternMenu_.setButtonText("Pattern");
    page1_.setTitle("Show steps 1 to 16"); page2_.setTitle("Show steps 17 to 32");
    copy_.setTitle("Copy the selected track's steps"); paste_.setTitle("Paste steps into the selected track"); clear_.setTitle("Clear the selected track");
    left_.setTitle("Shift the selected track one step earlier"); right_.setTitle("Shift the selected track one step later");
    mutate_.setTitle("Vary the selected track"); patternMenu_.setTitle("Pattern tools: copy or clear the pattern");
    for (auto* b : {&page1_, &page2_, &copy_, &paste_, &clear_, &left_, &right_, &mutate_, &patternMenu_}) addAndMakeVisible(b);
    page1_.onClick = [this] { ui_.stepPage = 0; grid_.repaint(); lanes_.repaint(); };
    page2_.onClick = [this] { ui_.stepPage = 1; grid_.repaint(); lanes_.repaint(); };
    copy_.onClick = [this] { clip_ = std::make_unique<TrackPattern>(proc_.patterns.track(editPattern(proc_), ui_.track)); };
    paste_.onClick = [this] { if (clip_) { proc_.undoManager.beginNewTransaction("Paste track"); proc_.patterns.setTrack(editPattern(proc_), ui_.track, *clip_); } };
    clear_.onClick = [this] { proc_.undoManager.beginNewTransaction("Clear track"); proc_.patterns.clearTrack(editPattern(proc_), ui_.track); };
    left_.onClick = [this] { proc_.undoManager.beginNewTransaction("Shift track"); proc_.patterns.rotate(editPattern(proc_), ui_.track, -1); };
    right_.onClick = [this] { proc_.undoManager.beginNewTransaction("Shift track"); proc_.patterns.rotate(editPattern(proc_), ui_.track, 1); };
    mutate_.onClick = [this] { mutate(); };
    patternMenu_.onClick = [this] {
        const int pat = editPattern(proc_);
        juce::PopupMenu copyTo;
        for (int i = 0; i < kPatterns; ++i) copyTo.addItem(100 + i, juce::String("Pattern ") + kPatternNames[i], i != pat);
        juce::PopupMenu m;
        m.addSectionHeader(juce::String("Pattern ") + kPatternNames[pat]);
        m.addSubMenu("Copy to", copyTo);
        m.addItem(2, "Clear pattern");
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&patternMenu_), [this, pat](int r) {
            if (r == 2) { proc_.undoManager.beginNewTransaction("Clear pattern"); proc_.patterns.clearPattern(pat); }
            else if (r >= 100) { proc_.undoManager.beginNewTransaction("Copy pattern"); proc_.patterns.copyPattern(pat, r - 100); }
        });
    };
    knob("swing", "Swing", Knob::Small);
    knob("tempo", "Tempo", Knob::Small);
}

void SeqPage::mutate() {
    // Moves, adds and removes a few hits; keeps the downbeat of kick-like tracks; ghost notes are soft.
    const int pat = editPattern(proc_), t = ui_.track;
    TrackPattern tp = proc_.patterns.track(pat, t);
    const int len = tp.length;
    const bool weighty = isWeighty((Model)rawI(proc_, UltraProcessor::tid(t, "Model")));
    std::vector<int> on;
    for (int s = 0; s < len; ++s) if (tp.steps[(size_t)s].vel > 0) on.push_back(s);
    const int moves = 1 + rng_.nextInt(2);
    for (int m = 0; m < moves && !on.empty(); ++m) {
        const int i = rng_.nextInt((int)on.size());
        const int s = on[(size_t)i];
        if (weighty && s == 0) continue;
        const int to = (s + (rng_.nextBool() ? 1 : -1) + len) % len;
        if (tp.steps[(size_t)to].vel == 0) { std::swap(tp.steps[(size_t)s], tp.steps[(size_t)to]); on[(size_t)i] = to; }
    }
    if (rng_.nextFloat() < 0.7f) {
        const int s = rng_.nextInt(len);
        if (tp.steps[(size_t)s].vel == 0) { tp.steps[(size_t)s].vel = (uint8_t)(35 + rng_.nextInt(30)); tp.steps[(size_t)s].chance = (uint8_t)(rng_.nextBool() ? 100 : 60); }
    }
    if (on.size() > 3 && rng_.nextFloat() < 0.4f) {
        const int s = on[(size_t)rng_.nextInt((int)on.size())];
        if (!(weighty && s == 0)) tp.steps[(size_t)s].vel = 0;
    }
    if (rng_.nextFloat() < 0.3f && !on.empty()) tp.steps[(size_t)on[(size_t)rng_.nextInt((int)on.size())]].ratchet = (uint8_t)(2 + rng_.nextInt(2));
    proc_.undoManager.beginNewTransaction("Mutate");
    proc_.patterns.setTrack(pat, t, tp);
}

void SeqPage::tick() {
    grid_.tick(); lanes_.tick();
    page1_.setToggleState(ui_.stepPage == 0, juce::dontSendNotification);
    page2_.setToggleState(ui_.stepPage == 1, juce::dontSendNotification);
    page1_.getProperties().set("accent", ui_.stepPage == 0); page2_.getProperties().set("accent", ui_.stepPage == 1);
    paste_.setEnabled(clip_ != nullptr);
    const juce::String title = u8("Sequencer  ·  pattern ") + kPatternNames[editPattern(proc_)];
    if (sections_.size() >= 2 && (sections_[0].title != title || sections_[1].title != u8("Lanes  ·  ") + trackTitle(proc_, ui_.track))) { resized(); repaint(); }
}

void SeqPage::resized() {
    beginLayout();
    auto r = getLocalBounds();
    section("01", u8("Sequencer  ·  pattern ") + kPatternNames[editPattern(proc_)], r.removeFromTop(14)); r.removeFromTop(6);
    auto tools = r.removeFromTop(26);
    auto put = [&](juce::Button& b, int w, int gapAfter) { b.setBounds(tools.removeFromLeft(w)); tools.removeFromLeft(gapAfter); };
    put(page1_, 56, 4); put(page2_, 56, 18);
    put(copy_, 58, 4); put(paste_, 58, 4); put(clear_, 58, 18);
    put(left_, 30, 4); put(right_, 30, 18);
    put(mutate_, 70, 18);
    patternMenu_.setBounds(tools.removeFromRight(90));
    r.removeFromTop(8);
    grid_.setBounds(r.removeFromTop((int)(r.getHeight() * 0.56f)));
    r.removeFromTop(10);
    section("02", u8("Lanes  ·  ") + trackTitle(proc_, ui_.track), r.removeFromTop(14)); r.removeFromTop(6);
    auto side = r.removeFromRight(std::clamp(r.getWidth() / 7, 130, 180)); r.removeFromRight(14);
    row(side, {c("swing"), c("tempo")});
    lanes_.setBounds(r);
}

// ---- VOICE page --------------------------------------------------------------------------------------------------
static const char* kModelText[kNumModels] = {
    "Analog house kick: a two-stage pitch fall, a beater click, and Punch saturating the body for weight.",
    "Hard kick for trance, techno and DnB: a clean sub, a driven layer and a top tok you can turn up.",
    "Two head modes with a pitch drop, snare-wire noise driven by the head, a crack and a tail.",
    "Several hands: noise bursts at loose spacing, then a room tail. Bursts sets how many.",
    "The 808's six squares and noise through a steep high-pass; Sweep starts bright and closes.",
    "Ride to crash: a stick ping and bell partials, then a wash of metal and noise.",
    "A membrane whose pitch bends as it relaxes, with a stick strike; Conga adds the slap.",
    "Two-operator FM percussion: Ratio steps through musical ratios, Index sets the brightness.",
    "808 cowbell crossfading to an FM bell with a detuned twin.",
    "Rimshot to clave: a click into wooden resonators. Body sets the low modes.",
    "Shakers, noise hits and risers: white, pink or granular noise through a sweeping band-pass.",
    "Laser zap: a dive from up to six octaves above the pitch, through a filter that follows it.",
    "Modal resonator: eight modes struck by a mallet; Material goes membrane, bar, glass."};

VoicePage::VoicePage(UltraProcessor& p, UiState& ui) : Page(p, ui) {
    for (int t = 0; t < kTracks; ++t) {
        auto id = [t](const char* n) { return UltraProcessor::tid(t, n); };
        choice(id("Model"), "Model");
        const char* dials[6] = {"Tune", "Decay", "Tone", "Snap", "Color", "Sweep"};
        for (int d = 0; d < 6; ++d) knob(id(dials[d]), dials[d], Knob::Normal, d == 0);
        knob(id("Drive"), "Drive"); choice(id("Filter"), "Filter"); knob(id("Cutoff"), "Cutoff"); knob(id("Res"), "Resonance"); knob(id("Vel"), "Velocity");
        knob(id("Level"), "Level", Knob::Small); knob(id("Pan"), "Pan", Knob::Small, true); knob(id("SendA"), "Echo", Knob::Small); knob(id("SendB"), "Reverb", Knob::Small);
        choice(id("Choke"), "Choke group"); choice(id("Out"), "Output"); choice(id("Lock"), "Lock lane"); knob(id("Note"), "MIDI note", Knob::Small);
        shownModel_[t] = -1;
    }
    trackChanged();
}

void VoicePage::trackChanged() {
    // Every control here belongs to one track ("t3Decay"); only the selected track's set is shown.
    for (auto& kv : byId_) kv.second->setVisible(kv.first.startsWith("t" + juce::String(ui_.track + 1)));
    resized(); repaint();
}

void VoicePage::tick() {
    const int t = ui_.track;
    const int m = rawI(proc_, UltraProcessor::tid(t, "Model"));
    if (m != shownModel_[t]) {
        shownModel_[t] = m;
        const char* dials[6] = {"Tune", "Decay", "Tone", "Snap", "Color", "Sweep"};
        for (int d = 0; d < 6; ++d)
            if (auto* k = dynamic_cast<Knob*>(c(UltraProcessor::tid(t, dials[d])))) k->setTitleText(kModelDialNames[juce::jlimit(0, kNumModels - 1, m)][d]);
        repaint();
    }
    setOn(c(UltraProcessor::tid(t, "Cutoff")), rawI(proc_, UltraProcessor::tid(t, "Filter")) != 0);
    setOn(c(UltraProcessor::tid(t, "Res")), rawI(proc_, UltraProcessor::tid(t, "Filter")) != 0);
    const juce::String title = u8("Voice  ·  ") + trackTitle(proc_, t);
    if (!sections_.empty() && sections_[0].title != title) { resized(); repaint(); }
}

void VoicePage::paint(juce::Graphics& g) {
    Page::paint(g);
    if (auto* m = c(UltraProcessor::tid(ui_.track, "Model"))) {
        const int model = juce::jlimit(0, kNumModels - 1, rawI(proc_, UltraProcessor::tid(ui_.track, "Model")));
        auto r = m->getBounds().toFloat();
        g.setFont(font(12.f)); g.setColour(kTextDim);
        g.drawFittedText(kModelText[model], juce::Rectangle<float>(r.getRight() + 16.f, r.getY() + 12.f, (float)getWidth() * 0.46f - r.getWidth() - 16.f, r.getHeight()).toNearestInt(),
                         juce::Justification::centredLeft, 2);
    }
}

void VoicePage::resized() {
    beginLayout();
    const int t = ui_.track;
    auto id = [t](const char* n) { return UltraProcessor::tid(t, n); };
    auto r = getLocalBounds();
    const int gap = 22;
    auto c1 = r.removeFromLeft((int)(r.getWidth() * 0.46f)); r.removeFromLeft(gap);
    auto c2 = r.removeFromLeft((int)(r.getWidth() * 0.5f)); r.removeFromLeft(gap);
    auto c3 = r;
    section("03", u8("Voice  ·  ") + trackTitle(proc_, t), c1.removeFromTop(14)); c1.removeFromTop(6);
    c(id("Model"))->setBounds(c1.removeFromTop(42).removeFromLeft(std::min(220, c1.getWidth() / 2)));
    c1.removeFromTop(6);
    row(c1.removeFromTop(c1.getHeight() / 2), {c(id("Tune")), c(id("Decay")), c(id("Tone"))});
    row(c1, {c(id("Snap")), c(id("Color")), c(id("Sweep"))});
    section("04", "Shape", c2.removeFromTop(14)); c2.removeFromTop(6);
    auto ft = c2.removeFromTop(42);
    c(id("Filter"))->setBounds(ft.removeFromLeft(std::min(160, ft.getWidth() / 2)));
    c2.removeFromTop(6);
    row(c2.removeFromTop(c2.getHeight() / 2), {c(id("Cutoff")), c(id("Res"))});
    row(c2, {c(id("Drive")), c(id("Vel"))});
    section("05", "Mix and routing", c3.removeFromTop(14)); c3.removeFromTop(6);
    row(c3.removeFromTop(c3.getHeight() * 45 / 100), {c(id("Level")), c(id("Pan")), c(id("SendA")), c(id("SendB"))});
    c3.removeFromTop(6);
    auto sel = c3.removeFromLeft(c3.getWidth() * 62 / 100); c3.removeFromLeft(10);
    const int ch = std::min(44, sel.getHeight() / 3);
    c(id("Choke"))->setBounds(sel.removeFromTop(ch)); sel.removeFromTop(4);
    c(id("Out"))->setBounds(sel.removeFromTop(ch)); sel.removeFromTop(4);
    c(id("Lock"))->setBounds(sel.removeFromTop(ch));
    c(id("Note"))->setBounds(c3);
}

// ---- MIX page ----------------------------------------------------------------------------------------------------
MixPage::MixPage(UltraProcessor& p, UiState& ui) : Page(p, ui) {
    for (int t = 0; t < kTracks; ++t) {
        auto id = [t](const char* n) { return UltraProcessor::tid(t, n); };
        knob(id("Level"), "Level"); knob(id("Pan"), "Pan", Knob::Small, true);
        knob(id("SendA"), "Echo", Knob::Small); knob(id("SendB"), "Reverb", Knob::Small);
        sw(id("Mute"), "M", "Track " + juce::String(t + 1) + " mute"); sw(id("Solo"), "S", "Track " + juce::String(t + 1) + " solo");
        choice(id("Choke"), "Choke"); choice(id("Out"), "Out");
    }
    knob("dlyRet", "Echo return"); knob("rvRet", "Reverb return"); knob("outDb", "Output", Knob::Normal, true);
}

void MixPage::tick() {
    bool dirty = false;
    for (int t = 0; t < kTracks; ++t) {
        const float v = proc_.machine.trackLevel[(size_t)t].load();
        const float db = 20.f * std::log10(std::max(v, 1e-5f)), f = juce::jlimit(0.f, 1.f, (db + 60.f) / 60.f);
        if (std::abs(f - shown_[(size_t)t]) > 0.01f) { shown_[(size_t)t] = f; dirty = true; }
    }
    if (dirty) repaint();
}

void MixPage::mouseDown(const juce::MouseEvent& e) {
    for (int t = 0; t < kTracks; ++t) if (strips_[(size_t)t].withHeight(24).contains(e.getPosition())) { ui_.select(t); repaint(); }
}

void MixPage::paint(juce::Graphics& g) {
    Page::paint(g);
    for (int t = 0; t < kTracks; ++t) {
        auto s = strips_[(size_t)t].toFloat();
        g.setColour(t == ui_.track ? juce::Colour(0xff1f1c26) : juce::Colours::black.withAlpha(0.12f));
        g.fillRoundedRectangle(s, 5.f);
        auto head = s.removeFromTop(22.f).reduced(8.f, 0.f);
        g.setFont(font(10.f, 600)); g.setColour(kAccent.withAlpha(t == ui_.track ? 1.f : 0.7f));
        g.drawText(juce::String(t + 1).paddedLeft('0', 2), head.removeFromLeft(18.f), juce::Justification::centredLeft);
        drawCaps(g, proc_.patterns.trackName(t), head, juce::Justification::centredLeft, 9.f, t == ui_.track ? kText : kTextDim);
    }
}

void MixPage::paintOverChildren(juce::Graphics& g) {
    // A thin level meter on the right edge of each strip.
    for (int t = 0; t < kTracks; ++t) {
        auto s = strips_[(size_t)t].toFloat();
        auto m = juce::Rectangle<float>(s.getRight() - 6.f, s.getY() + 28.f, 3.f, s.getHeight() * 0.42f);
        g.setColour(kDisplayBg); g.fillRoundedRectangle(m, 1.5f);
        const float f = shown_[(size_t)t];
        if (f > 0.f) { g.setColour(f > 0.95f ? kDanger : kAccent); g.fillRoundedRectangle(m.withTop(m.getBottom() - m.getHeight() * f), 1.5f); }
    }
}

void MixPage::resized() {
    beginLayout();
    auto r = getLocalBounds();
    section("06", "Mixer", r.removeFromTop(14)); r.removeFromTop(6);
    auto ret = r.removeFromRight(std::clamp(r.getWidth() / 9, 110, 150)); r.removeFromRight(14);
    const int gap = 6, w = (r.getWidth() - gap * (kTracks - 1)) / kTracks;
    for (int t = 0; t < kTracks; ++t) {
        auto id = [t](const char* n) { return UltraProcessor::tid(t, n); };
        auto s = r.removeFromLeft(w); r.removeFromLeft(gap);
        strips_[(size_t)t] = s;
        auto in = s.reduced(6, 0).withTrimmedRight(6);
        in.removeFromTop(24);
        const int H = in.getHeight();
        c(id("Level"))->setBounds(in.removeFromTop(H * 30 / 100));
        auto pr = in.removeFromTop(H * 22 / 100);
        c(id("Pan"))->setBounds(pr.removeFromLeft(pr.getWidth() / 2)); c(id("SendA"))->setBounds(pr);
        auto rr = in.removeFromTop(H * 22 / 100);
        c(id("SendB"))->setBounds(rr.removeFromLeft(rr.getWidth() / 2));
        auto tg = rr.reduced(4, std::max(0, (rr.getHeight() - 52) / 2));
        c(id("Mute"))->setBounds(tg.removeFromTop(24)); tg.removeFromTop(4); c(id("Solo"))->setBounds(tg.removeFromTop(24));
        auto chs = in;
        c(id("Choke"))->setBounds(chs.removeFromTop(chs.getHeight() / 2).reduced(0, 1)); c(id("Out"))->setBounds(chs.reduced(0, 1));
    }
    section("07", "Returns", ret.removeFromTop(14)); ret.removeFromTop(4);
    const int rh = ret.getHeight() / 3;
    c("dlyRet")->setBounds(ret.removeFromTop(rh)); c("rvRet")->setBounds(ret.removeFromTop(rh)); c("outDb")->setBounds(ret);
}

// ---- FX page -----------------------------------------------------------------------------------------------------
FxPage::FxPage(UltraProcessor& p, UiState& ui) : Page(p, ui) {
    choice("drvMode", "Mode"); knob("drvAmt", "Drive"); knob("drvTone", "Tone"); knob("drvMix", "Mix");
    knob("glThr", "Threshold"); knob("glRatio", "Ratio"); knob("glAtk", "Attack"); knob("glRel", "Release"); knob("glMakeup", "Makeup"); knob("glMix", "Mix");
    toggle("dlySync", "Sync"); toggle("dlyPing", "Ping-pong"); choice("dlyDiv", "Time");
    knob("dlyMs", "Time"); knob("dlyFb", "Feedback"); knob("dlyDamp", "Damping"); knob("dlyToRv", "To reverb");
    knob("rvSize", "Size"); knob("rvDecay", "Decay"); knob("rvDamp", "Damping"); knob("rvPre", "Pre-delay");
    knob("eqLow", "Low", Knob::Normal, true); knob("eqHigh", "High", Knob::Normal, true); knob("width", "Width");
}
void FxPage::tick() {
    const bool sync = raw(proc_, "dlySync") > 0.5f;
    setOn(c("dlyMs"), !sync); setOn(c("dlyDiv"), sync);
}
void FxPage::resized() {
    beginLayout();
    auto r = getLocalBounds();
    const int gap = 20, W = r.getWidth() - gap * 4;
    auto col = [&](float share) { auto cc = r.removeFromLeft((int)(W * share)); r.removeFromLeft(gap); return cc; };
    auto drv = col(0.16f), glu = col(0.25f), dly = col(0.24f), rev = col(0.18f), out = r;
    auto head = [&](juce::Rectangle<int>& b, const char* code, const juce::String& t) { section(code, t, b.removeFromTop(14)); b.removeFromTop(4); };
    head(drv, "08", "Drive");
    c("drvMode")->setBounds(drv.removeFromTop(44).reduced(0, 1)); drv.removeFromTop(4);
    c("drvAmt")->setBounds(drv.removeFromTop(drv.getHeight() / 2));
    row(drv, {c("drvTone"), c("drvMix")});
    head(glu, "09", "Glue");
    row(glu.removeFromTop(glu.getHeight() / 2), {c("glThr"), c("glRatio"), c("glMix")});
    row(glu, {c("glAtk"), c("glRel"), c("glMakeup")});
    head(dly, "10", u8("Echo  ·  send A"));
    {
        auto t = dly.removeFromTop(44);
        auto tg = t.removeFromLeft(t.getWidth() / 2);
        c("dlySync")->setBounds(tg.removeFromTop(22)); c("dlyPing")->setBounds(tg.removeFromTop(22));
        t.removeFromLeft(6); c("dlyDiv")->setBounds(t);
        dly.removeFromTop(4);
        row(dly.removeFromTop(dly.getHeight() / 2), {c("dlyMs"), c("dlyFb")});
        row(dly, {c("dlyDamp"), c("dlyToRv")});
    }
    head(rev, "11", u8("Chromosphere reverb  ·  B"));
    row(rev.removeFromTop(rev.getHeight() / 2), {c("rvSize"), c("rvDecay")});
    row(rev, {c("rvDamp"), c("rvPre")});
    head(out, "12", "Output");
    row(out.removeFromTop(out.getHeight() / 2), {c("eqLow"), c("eqHigh")});
    row(out, {c("width")});
}

// ---- Macros ------------------------------------------------------------------------------------------------------
static const char* kMacroMeaning[4] = {"Brightness", "Weight", "Groove", "Space"};

MacroBay::MacroBay(UltraProcessor& p) {
    for (int m = 0; m < 4; ++m) {
        knobs_.push_back(std::make_unique<Knob>(p.apvts, "macro" + juce::String(m + 1), kMacroNames[m], Knob::Large));
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
    auto r = getLocalBounds();
    r.removeFromTop(18);
    r.removeFromBottom(14);
    const int w = r.getWidth() / 4;
    for (int m = 0; m < 4; ++m) knobs_[(size_t)m]->setBounds(r.removeFromLeft(w));
}

// ---- Transport ---------------------------------------------------------------------------------------------------
Transport::Transport(UltraProcessor& p) : proc_(p) { setTitle("Transport"); setDescription("Play, patterns A to H, fill"); }

juce::Rectangle<float> Transport::playRect() const { return getLocalBounds().toFloat().removeFromLeft(78.f); }
juce::Rectangle<float> Transport::fillRect() const { return getLocalBounds().toFloat().removeFromRight(58.f); }
juce::Rectangle<float> Transport::patternRect(int i) const {
    auto r = getLocalBounds().toFloat();
    r.removeFromLeft(90.f); r.removeFromRight(70.f);
    const float w = r.getWidth() / kPatterns;
    return juce::Rectangle<float>(r.getX() + w * i, r.getY(), w, r.getHeight()).reduced(2.f, 0.f);
}

void Transport::mouseDown(const juce::MouseEvent& e) {
    if (playRect().contains(e.position)) { proc_.setPlaying(!proc_.isPlaying()); repaint(); return; }
    if (fillRect().contains(e.position)) { fillHeld_ = true; setPlain(proc_, "fill", 1.f); repaint(); return; }
    for (int i = 0; i < kPatterns; ++i) if (patternRect(i).contains(e.position)) { setPlain(proc_, "pattern", (float)i); repaint(); }
}
void Transport::mouseUp(const juce::MouseEvent&) {
    if (fillHeld_) { fillHeld_ = false; setPlain(proc_, "fill", 0.f); repaint(); }
}

void Transport::tick() {
    // The blink only matters while a pattern waits for the bar line; otherwise it must not repaint anything.
    const bool waiting = proc_.machine.queuedPattern.load() >= 0 || (!proc_.machine.running.load() && rawI(proc_, "pattern") != proc_.machine.currentPattern.load());
    const bool blink = waiting && (juce::Time::getMillisecondCounter() / 250) % 2 == 0;
    const int state = (proc_.machine.running.load() ? 1 : 0) | (proc_.isPlaying() ? 2 : 0) | (proc_.machine.currentPattern.load() << 2) |
                      ((proc_.machine.queuedPattern.load() + 1) << 6) | (rawI(proc_, "pattern") << 10) | ((raw(proc_, "fill") > 0.5f ? 1 : 0) << 14) |
                      ((blink ? 1 : 0) << 15) | (proc_.patterns.version.load() << 16);
    if (state != lastState_) { lastState_ = state; repaint(); }
}

void Transport::paint(juce::Graphics& g) {
    const bool running = proc_.machine.running.load(), mine = proc_.isPlaying();
    // Play: a lit triangle while the internal clock runs, a hollow one while the host drives the sequencer.
    auto pr = playRect();
    g.setColour(mine ? juce::Colour(0xff241f30) : juce::Colour(0xff1d1f23)); g.fillRoundedRectangle(pr, 5.f);
    g.setColour(juce::Colours::white.withAlpha(mine ? 0.14f : 0.08f)); g.drawRoundedRectangle(pr.reduced(0.5f), 5.f, 1.f);
    auto icon = pr.removeFromLeft(30.f).withSizeKeepingCentre(10.f, 12.f);
    juce::Path tri;
    if (mine) tri.addRectangle(icon.reduced(0.5f, 1.f));
    else tri.addTriangle(icon.getX(), icon.getY(), icon.getRight(), icon.getCentreY(), icon.getX(), icon.getBottom());
    g.setColour(running ? kAccent : kTextDim); g.fillPath(tri);
    drawCaps(g, mine ? "Stop" : "Play", pr, juce::Justification::centredLeft, 10.f, running ? kText : kTextDim);

    const int cur = proc_.machine.currentPattern.load(), queued = proc_.machine.queuedPattern.load(), sel = rawI(proc_, "pattern");
    const bool blink = (juce::Time::getMillisecondCounter() / 250) % 2 == 0;
    for (int i = 0; i < kPatterns; ++i) {
        auto r = patternRect(i);
        bool content = false;
        for (const auto& tp : proc_.patterns.bank().patterns[(size_t)i].tracks) for (int s = 0; s < tp.length && !content; ++s) content = tp.steps[(size_t)s].vel > 0;
        const bool isCur = i == cur, isQueued = i == queued || (!running && i == sel && i != cur);
        g.setColour(isCur ? kAccent.withAlpha(0.85f) : juce::Colour(0xff1d1f23)); g.fillRoundedRectangle(r, 4.f);
        if (isQueued && blink) { g.setColour(kAccent); g.drawRoundedRectangle(r.reduced(0.75f), 4.f, 1.5f); }
        else { g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawRoundedRectangle(r.reduced(0.5f), 4.f, 1.f); }
        g.setFont(font(11.f, 600)); g.setColour(isCur ? kBg : content ? kText : kTextFaint);
        g.drawText(kPatternNames[i], r, juce::Justification::centred);
        if (content && !isCur) { g.setColour(kAccent.withAlpha(0.7f)); g.fillEllipse(r.getCentreX() - 1.5f, r.getBottom() - 5.f, 3.f, 3.f); }
    }
    auto fr = fillRect();
    const bool fill = raw(proc_, "fill") > 0.5f;
    g.setColour(fill ? kAccent : juce::Colour(0xff1d1f23)); g.fillRoundedRectangle(fr, 5.f);
    g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawRoundedRectangle(fr.reduced(0.5f), 5.f, 1.f);
    drawCaps(g, "Fill", fr, juce::Justification::centred, 10.f, fill ? kBg : kTextDim);
}

// ---- Pads --------------------------------------------------------------------------------------------------------
Pads::Pads(UltraProcessor& p, UiState& ui) : proc_(p), ui_(ui) { setTitle("Pads"); setDescription("Click a pad to select its track and play it; higher is louder. Double-click to rename."); }

juce::Rectangle<float> Pads::padRect(int t) const {
    const float gap = 8.f, w = (getWidth() - gap * (kTracks - 1)) / kTracks;
    return {t * (w + gap), 0.f, w, (float)getHeight()};
}

void Pads::mouseDown(const juce::MouseEvent& e) {
    for (int t = 0; t < kTracks; ++t) {
        const auto r = padRect(t);
        if (!r.contains(e.position)) continue;
        ui_.select(t);
        proc_.playPad(t, juce::jlimit(0.3f, 1.f, 1.f - 0.7f * (e.position.y - r.getY()) / r.getHeight()));
        repaint();
    }
}
void Pads::mouseDoubleClick(const juce::MouseEvent& e) {
    for (int t = 0; t < kTracks; ++t) {
        if (!padRect(t).contains(e.position)) continue;
        auto* w = new juce::AlertWindow("Rename track", "Name for track " + juce::String(t + 1) + ":", juce::MessageBoxIconType::NoIcon);
        w->addTextEditor("name", proc_.patterns.trackName(t));
        w->addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey));
        w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        w->enterModalState(true, juce::ModalCallbackFunction::create([this, w, t](int r) {
            if (r == 1) { proc_.undoManager.beginNewTransaction("Rename track"); proc_.patterns.setTrackName(t, w->getTextEditorContents("name")); }
            delete w;
        }), false);
    }
}

void Pads::tick() {
    // Only the pads that change repaint (a hit, its glow fading); everything after a name or model change.
    const int ver = proc_.patterns.version.load();
    if (ver != paintVer_) { paintVer_ = ver; repaint(); }
    for (int t = 0; t < kTracks; ++t) {
        const int h = proc_.machine.hitCount[(size_t)t].load();
        bool dirty = false;
        if (h != lastHits_[t]) { lastHits_[t] = h; flash_[t] = 0.35f + 0.65f * proc_.machine.hitVelocity[(size_t)t].load(); dirty = true; }
        else if (flash_[t] > 0.f) { flash_[t] = std::max(0.f, flash_[t] - 0.13f); dirty = true; }
        if (dirty) repaint(padRect(t).toNearestInt().expanded(2));
    }
}

void Pads::paint(juce::Graphics& g) {
    for (int t = 0; t < kTracks; ++t) {
        auto r = padRect(t).reduced(0.5f);
        const float f = flash_[t];
        // Rubber pad in a recess: a dark body, a soft top light, the hit glow from beneath.
        g.setColour(juce::Colours::black.withAlpha(0.55f)); g.fillRoundedRectangle(r.translated(0.f, 1.5f), 7.f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff25232b), r.getX(), r.getY(), juce::Colour(0xff141317), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle(r, 7.f);
        if (f > 0.f) {
            g.setGradientFill(juce::ColourGradient(kAccent.withAlpha(0.65f * f), r.getCentreX(), r.getCentreY(), kAccent.withAlpha(0.f), r.getX(), r.getY(), true));
            g.fillRoundedRectangle(r, 7.f);
        }
        g.setColour(t == ui_.track ? kAccent.withAlpha(0.9f) : juce::Colours::white.withAlpha(0.07f));
        g.drawRoundedRectangle(r.reduced(0.5f), 7.f, t == ui_.track ? 1.5f : 1.f);
        auto in = r.reduced(10.f, 6.f);
        g.setFont(font(10.f, 600)); g.setColour(kAccent.withAlpha(0.8f));
        g.drawText(juce::String(t + 1).paddedLeft('0', 2), in.removeFromTop(13.f), juce::Justification::centredLeft);
        const int model = juce::jlimit(0, kNumModels - 1, rawI(proc_, UltraProcessor::tid(t, "Model")));
        drawCaps(g, kModelNames[model], in.removeFromBottom(12.f), juce::Justification::centredLeft, 7.5f, kTextFaint);
        drawCaps(g, proc_.patterns.trackName(t), in, juce::Justification::centredLeft, 10.5f, kText);
    }
}

// ---- Main display ------------------------------------------------------------------------------------------------
MainDisplay::MainDisplay(UltraProcessor& p, UiState& ui, std::function<sun::Drive()> drive)
    : sun(sun::Mode::Limb, std::move(drive), false), proc_(p), ui_(ui) {
    setTitle("Display"); setDescription("Orbit radar of the pattern, the selected voice, the output and level");
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
    auto& m = proc_.machine;
    // The traces are redrawn only when they can have changed: while the sequencer runs or anything sounds, or
    // when what they show changes. A stopped, silent instance re-composites the cached layers over each new sun
    // frame (EXO's optimization guide, 3.2).
    bool live = m.running.load() || m.peak.load() > 1e-4f || m.activeVoices.load() > 0;
    for (float f : flash_) live = live || f > 0.f;
    const bool previewChanged = updateVoicePreview();
    const juce::String key = juce::String(proc_.patterns.version.load()) + "|" + juce::String(ui_.track) + "|" + juce::String(editPattern(proc_)) + "|" +
                             juce::String(caption.hashCode()) + "|" + juce::String(m.currentPattern.load());
    const bool resized = !traceLayer_.isValid() || traceLayer_.getWidth() != W || traceLayer_.getHeight() != H;
    if (resized || live || previewChanged || key != traceKey_) {
        traceKey_ = key;
        // Software images: they are drawn into, scaled and blurred on the CPU. On Windows a native image is a
        // Direct2D bitmap, and each of those steps would copy it back from the GPU (INFRA's RCA).
        if (resized) traceLayer_ = juce::Image(juce::Image::ARGB, W, H, true, juce::SoftwareImageType());
        else traceLayer_.clear(traceLayer_.getBounds(), juce::Colours::transparentBlack);
        { juce::Graphics lg(traceLayer_); paintTraces(lg, all); }
        juce::Image glow = traceLayer_.rescaled(std::max(1, W / 2), std::max(1, H / 2), juce::Graphics::mediumResamplingQuality);
        gaussianBlur(glow, 2, 1.8f);
        // Glow, traces and scanlines flattened into one overlay, so each sun frame adds a single blit.
        overlay_ = juce::Image(juce::Image::ARGB, W, H, true);
        juce::Graphics og(overlay_);
        og.setOpacity(0.75f); og.drawImageTransformed(glow, juce::AffineTransform::scale((float)W / glow.getWidth(), (float)H / glow.getHeight()), false);
        og.setOpacity(1.f); og.drawImageAt(traceLayer_, 0, 0);
        juce::Path clip; clip.addRoundedRectangle(all, 8.f); og.reduceClipRegion(clip);
        og.setColour(juce::Colours::black.withAlpha(0.13f));
        for (float y = all.getY() + 1.f; y < all.getBottom(); y += 3.f) og.fillRect(all.getX(), y, all.getWidth(), 1.f);
    }
    // The scanlines also cross the sun beneath: the overlay carries them over whatever is under the traces.
    g.drawImageAt(overlay_, 0, 0);
}

void MainDisplay::paintTraces(juce::Graphics& g, juce::Rectangle<float> all) {
    for (int t = 0; t < kTracks; ++t) {
        const int h = proc_.machine.hitCount[(size_t)t].load();
        if (h != lastHits_[t]) { lastHits_[t] = h; flash_[t] = proc_.machine.hitVelocity[(size_t)t].load(); }
        else flash_[t] = std::max(0.f, flash_[t] - 0.15f);
    }
    auto r = all.reduced(16.f, 12.f);
    if (caption.isNotEmpty()) {
        auto cap = r.removeFromBottom(14.f);
        g.setFont(font(11.f)); g.setColour(kAmberText);
        g.drawText(caption, cap, juce::Justification::centredLeft, true);
        r.removeFromBottom(4.f);
    }
    auto radar = r.removeFromLeft(std::min(r.getWidth() * 0.4f, r.getHeight() + 150.f)); r.removeFromLeft(18.f);
    auto voice = r.removeFromLeft(r.getWidth() * 0.55f); r.removeFromLeft(18.f);
    g.setColour(kAmberGrid);
    g.drawVerticalLine((int)(radar.getRight() + 9.f), r.getY(), r.getBottom());
    g.drawVerticalLine((int)(voice.getRight() + 9.f), r.getY(), r.getBottom());
    paintRadar(g, radar);
    paintVoice(g, voice);
    paintOutput(g, r);
}

void MainDisplay::paintRadar(juce::Graphics& g, juce::Rectangle<float> r) {
    auto& m = proc_.machine;
    const bool running = m.running.load();
    const int pat = running ? m.currentPattern.load() : editPattern(proc_);
    drawCaps(g, u8("Orbits  ·  pattern ") + kPatternNames[pat] + (running ? u8("  ·  ") + juce::String(m.bpm.load(), 1) + " bpm" : u8("  ·  stopped")),
             r.removeFromTop(12.f), juce::Justification::centredLeft, 9.f, kAmberText);
    r.removeFromTop(4.f);
    const float side = std::min(r.getWidth(), r.getHeight());
    const auto c = juce::Point<float>(r.getX() + side * 0.5f, r.getY() + side * 0.5f);
    const float rMax = side * 0.5f - 3.f, rMin = rMax * 0.3f;
    // Track names down the right of the rings.
    const double pos = m.position.load();
    for (int t = 0; t < kTracks; ++t) {
        const auto& tp = proc_.patterns.track(pat, t);
        const float rr = rMin + (rMax - rMin) * t / (kTracks - 1.f);
        const bool sel = t == ui_.track;
        g.setColour(sel ? kAmber.withAlpha(0.35f) : kAmberGrid);
        g.drawEllipse(c.x - rr, c.y - rr, 2 * rr, 2 * rr, sel ? 1.f : 0.7f);
        const int len = tp.length;
        for (int s = 0; s < len; ++s) {
            const float a = -juce::MathConstants<float>::halfPi + juce::MathConstants<float>::twoPi * s / len;
            const auto p = c + juce::Point<float>(std::cos(a), std::sin(a)) * rr;
            const auto& st = tp.steps[(size_t)s];
            if (st.vel > 0) {
                const float v = st.vel / 127.f, d = 1.6f + 2.6f * v;
                g.setColour(kAmber.withAlpha(0.45f + 0.55f * v)); g.fillEllipse(p.x - d * 0.5f, p.y - d * 0.5f, d, d);
            } else { g.setColour(kAmberGrid); g.fillEllipse(p.x - 0.8f, p.y - 0.8f, 1.6f, 1.6f); }
        }
        if (running) {
            // Each orbit turns at its own speed: polymeter shows as orbits drifting apart.
            const double sb = kRateBeats[juce::jlimit(0, kNumRates - 1, tp.rate)];
            const double steps = pos / sb;
            const float frac = (float)(std::fmod(steps, (double)len) / len);
            const float a = -juce::MathConstants<float>::halfPi + juce::MathConstants<float>::twoPi * frac;
            const auto p = c + juce::Point<float>(std::cos(a), std::sin(a)) * rr;
            const float d = 3.f + 5.f * flash_[t];
            g.setColour(kAmberHot.withAlpha(0.5f + 0.5f * flash_[t])); g.fillEllipse(p.x - d * 0.5f, p.y - d * 0.5f, d, d);
        }
    }
    g.setColour(kAmber.withAlpha(0.8f)); g.setFont(font(13.f, 600));
    g.drawText(kPatternNames[pat], juce::Rectangle<float>(c.x - 20.f, c.y - 10.f, 40.f, 20.f), juce::Justification::centred);
    // Legend beside the rings when there is room.
    auto leg = r.withTrimmedLeft(side + 10.f);
    if (leg.getWidth() > 60.f) {
        const float lh = std::min(14.f, leg.getHeight() / kTracks);
        for (int t = kTracks - 1, row = 0; t >= 0; --t, ++row) {
            const auto lr = juce::Rectangle<float>(leg.getX(), leg.getY() + row * lh, leg.getWidth(), lh);
            drawCaps(g, juce::String(t + 1) + " " + proc_.patterns.trackName(t), lr, juce::Justification::centredLeft, 7.5f,
                     t == ui_.track ? kAmber : kAmberText.withMultipliedAlpha(0.6f + 0.4f * flash_[t]));
        }
    }
}

bool MainDisplay::updateVoicePreview() {
    const int t = ui_.track;
    const auto kit = proc_.currentParams();
    const auto& v = kit.tracks[(size_t)t].v;
    const juce::String key = juce::String((int)v.model) + "|" + juce::String(v.tune, 2) + "|" + juce::String(v.decay, 3) + "|" + juce::String(v.tone, 3) + "|" +
                             juce::String(v.snap, 3) + "|" + juce::String(v.color, 3) + "|" + juce::String(v.sweep, 3) + "|" + juce::String(v.drive, 3) + "|" +
                             juce::String((int)v.filter) + "|" + juce::String(v.cutoff, 0) + "|" + juce::String(v.resonance, 3);
    if (key == previewKey_) return false;
    previewKey_ = key;
    // Render one hit offline and keep its peak envelope (2 ms per value), as long as the hit lasts (up to 2 s).
    DrumVoice voice;
    voice.prepare(48000.0, 77);
    voice.trigger(v, 1.f, 1.f);
    voice.setFilter(v.filter, v.cutoff, v.resonance);
    preview_.clear();
    float buf[96];
    for (int i = 0; i < 1000 && voice.active(); ++i) {
        std::fill(buf, buf + 96, 0.f);
        voice.render(buf, 96);
        float pk = 0.f; for (float x : buf) pk = std::max(pk, std::abs(x));
        preview_.push_back(pk);
    }
    previewSeconds_ = std::max(0.05f, preview_.size() * 0.002f);
    return true;
}

void MainDisplay::paintVoice(juce::Graphics& g, juce::Rectangle<float> r) {
    updateVoicePreview();
    const int t = ui_.track;
    const int model = juce::jlimit(0, kNumModels - 1, rawI(proc_, UltraProcessor::tid(t, "Model")));
    const float tune = raw(proc_, UltraProcessor::tid(t, "Tune"));
    drawCaps(g, u8("Voice  ·  ") + juce::String(t + 1) + " " + proc_.patterns.trackName(t) + u8("  ·  ") + kModelNames[model] + u8("  ·  ") +
                (tune > 0 ? "+" : "") + juce::String(tune, 1) + " st", r.removeFromTop(12.f), juce::Justification::centredLeft, 9.f, kAmberText);
    r.removeFromTop(6.f);
    auto axis = r.removeFromBottom(12.f);
    const float mid = r.getCentreY(), half = r.getHeight() * 0.46f;
    g.setColour(kAmberGrid); g.drawHorizontalLine((int)mid, r.getX(), r.getRight());
    for (int k = 1; k < 4; ++k) g.drawVerticalLine((int)(r.getX() + r.getWidth() * k / 4.f), r.getY(), r.getBottom());
    if (!preview_.empty()) {
        float pk = 0.f; for (float x : preview_) pk = std::max(pk, x);
        const float scale = pk > 1e-6f ? 1.f / std::max(pk, 0.5f) : 1.f;
        juce::Path top, fill;
        const int n = (int)preview_.size();
        for (int i = 0; i < n; ++i) {
            const float x = r.getX() + r.getWidth() * i / (float)std::max(1, n - 1);
            const float y = juce::jlimit(0.f, 1.f, preview_[(size_t)i] * scale) * half;
            if (i == 0) { top.startNewSubPath(x, mid - y); fill.startNewSubPath(x, mid - y); } else { top.lineTo(x, mid - y); fill.lineTo(x, mid - y); }
        }
        for (int i = n - 1; i >= 0; --i) {
            const float x = r.getX() + r.getWidth() * i / (float)std::max(1, n - 1);
            fill.lineTo(x, mid + juce::jlimit(0.f, 1.f, preview_[(size_t)i] * scale) * half);
        }
        fill.closeSubPath();
        g.setColour(kAmber.withAlpha(0.14f + 0.25f * flash_[t])); g.fillPath(fill);
        g.setColour(kAmber); g.strokePath(top, juce::PathStrokeType(1.6f));
        g.setColour(kAmberHot.withAlpha(0.7f)); g.strokePath(top, juce::PathStrokeType(0.6f));
        juce::Path bottom = top; bottom.applyTransform(juce::AffineTransform::verticalFlip(mid * 2.f));
        g.setColour(kAmber.withAlpha(0.6f)); g.strokePath(bottom, juce::PathStrokeType(1.f));
    }
    const juce::String len = previewSeconds_ < 1.f ? juce::String(juce::roundToInt(previewSeconds_ * 1000.f)) + " ms" : juce::String(previewSeconds_, 2) + " s";
    drawCaps(g, "0", axis, juce::Justification::centredLeft, 8.f, kAmberText);
    drawCaps(g, len, axis, juce::Justification::centredRight, 8.f, kAmberText);
}

void MainDisplay::paintOutput(juce::Graphics& g, juce::Rectangle<float> r) {
    auto& m = proc_.machine;
    const int voices = m.activeVoices.load(std::memory_order_relaxed);
    drawCaps(g, "Output  " + juce::String(voices) + (voices == 1 ? " voice" : " voices"), r.removeFromTop(12.f), juce::Justification::centredLeft, 9.f, kAmberText);
    r.removeFromTop(4.f);
    auto meters = r.removeFromRight(34.f); r.removeFromRight(10.f);
    // Rolling scope: the newest 2048 samples as peak columns, so drum hits read as shapes.
    const int N = DrumMachine::kScopeSize;
    const int pos = m.scopePos.load(std::memory_order_acquire);
    const int cols = std::max(8, (int)r.getWidth() / 2);
    g.setColour(kAmberGrid); g.drawHorizontalLine((int)r.getCentreY(), r.getX(), r.getRight());
    juce::Path hi, lo;
    for (int cI = 0; cI < cols; ++cI) {
        float mx = -1.f, mn = 1.f;
        const int a = cI * N / cols, b = (cI + 1) * N / cols;
        for (int i = a; i < b; ++i) { const float v = m.scope[(size_t)((pos + i) % N)].load(std::memory_order_relaxed); mx = std::max(mx, v); mn = std::min(mn, v); }
        const float x = r.getX() + r.getWidth() * cI / (float)(cols - 1);
        const float y1 = r.getCentreY() - juce::jlimit(-1.f, 1.f, mx * 1.1f) * r.getHeight() * 0.48f, y2 = r.getCentreY() - juce::jlimit(-1.f, 1.f, mn * 1.1f) * r.getHeight() * 0.48f;
        if (cI == 0) { hi.startNewSubPath(x, y1); lo.startNewSubPath(x, y2); } else { hi.lineTo(x, y1); lo.lineTo(x, y2); }
    }
    g.setColour(kAmber); g.strokePath(hi, juce::PathStrokeType(1.3f)); g.strokePath(lo, juce::PathStrokeType(1.3f));
    g.setColour(kAmberHot.withAlpha(0.6f)); g.strokePath(hi, juce::PathStrokeType(0.5f));
    // Output level, glue gain reduction, limiter lamp.
    auto outM = meters.removeFromLeft(14.f), grM = meters.removeFromRight(14.f);
    const float pk = m.peak.load(std::memory_order_relaxed);
    const float db = 20.f * std::log10(std::max(pk, 1e-5f)), frac = juce::jlimit(0.f, 1.f, (db + 60.f) / 60.f);
    auto bar = outM.withTrimmedTop(10.f).withTrimmedBottom(12.f);
    g.setColour(kAmberGrid); g.fillRoundedRectangle(bar, 2.f);
    g.setGradientFill(juce::ColourGradient(db > -3.f ? kDanger : kAmberHot, 0.f, bar.getY(), kAmber, 0.f, bar.getBottom(), false));
    g.fillRoundedRectangle(bar.withTop(bar.getBottom() - bar.getHeight() * frac), 2.f);
    const bool limiting = m.limiterGain.load(std::memory_order_relaxed) < 0.97f;
    g.setColour(limiting ? kDanger : juce::Colour(0xff332f3a)); g.fillEllipse(outM.getCentreX() - 3.f, outM.getY(), 6.f, 6.f);
    auto gb = grM.withTrimmedTop(10.f).withTrimmedBottom(12.f);
    g.setColour(kAmberGrid); g.fillRoundedRectangle(gb, 2.f);
    const float gr = juce::jlimit(0.f, 1.f, m.glueReduction.load(std::memory_order_relaxed) / 12.f);
    g.setColour(kAmber.withAlpha(0.85f)); g.fillRoundedRectangle(gb.withHeight(gb.getHeight() * gr), 2.f);
    drawCaps(g, "Out", outM.withTop(outM.getBottom() - 10.f).expanded(4.f, 0.f), juce::Justification::centred, 7.f, kAmberText);
    drawCaps(g, "GR", grM.withTop(grM.getBottom() - 10.f).expanded(4.f, 0.f), juce::Justification::centred, 7.f, kAmberText);
}

}  // namespace panels
