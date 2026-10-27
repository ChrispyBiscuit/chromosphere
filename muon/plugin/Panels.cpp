#include "Panels.h"
#include "PluginProcessor.h"

using namespace look;

namespace panels {

// The scope is the family's amber CRT: P3 phosphor body, a hotter near-white core, dim amber for grid and labels.
static const juce::Colour kAmber(0xffffb000), kAmberHot(0xffffe3a6), kAmberText(0xb8ffb000), kAmberGrid(0x29ffb000);

static void setOn(juce::Component* c, bool on) { if (c && c->isEnabled() != on) { c->setEnabled(on); c->setAlpha(on ? 1.f : 0.45f); } }
static juce::String hzText(float f) { return f >= 1000.f ? juce::String(f / 1000.f, f >= 10000.f ? 1 : 2) + " kHz" : juce::String((int)std::lround(f)) + " Hz"; }

// ---- Page and the macro menu --------------------------------------------------------------------------------
// Right-click on a dial: assign its parameter to a macro, with an amount, or remove the assignment.
struct Page::MacroMenu : public juce::MouseListener {
    MacroMenu(MuonProcessor& p, Knob& k, juce::String title) : proc(p), knob(k), name(std::move(title)) { knob.slider.addMouseListener(this, false); }
    ~MacroMenu() override { knob.slider.removeMouseListener(this); }
    void mouseDown(const juce::MouseEvent& e) override {
        if (!e.mods.isPopupMenu()) return;
        if (dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter(knob.paramId)) == nullptr || knob.paramId.startsWith("macro")) return;
        juce::PopupMenu menu;
        menu.addSectionHeader("Macros for " + name);
        const float amounts[] = {-1.f, -0.5f, -0.25f, 0.25f, 0.5f, 1.f};
        for (int m = 0; m < MuonProcessor::kNumMacros; ++m) {
            float cur = 0.f;
            for (auto& t : proc.macroTargets()) if (t.macro == m && t.paramId == knob.paramId) cur = t.amount;
            juce::PopupMenu sub;
            for (float a : amounts) sub.addItem(100 * (m + 1) + (int)std::lround(a * 40) + 50, (a > 0 ? "+" : "") + juce::String((int)std::lround(a * 100)) + "%", true, std::abs(cur - a) < 1e-3f);
            sub.addItem(100 * (m + 1) + 99, "Remove", std::abs(cur) > 1e-6f);
            menu.addSubMenu(juce::String(MuonProcessor::kMacroNames[m]) + (std::abs(cur) > 1e-6f ? "  (" + juce::String((int)std::lround(cur * 100)) + "%)" : ""), sub);
        }
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&knob), [this](int r) {
            if (r <= 0) return;
            const int m = r / 100 - 1, code = r % 100;
            auto t = proc.macroTargets();
            t.erase(std::remove_if(t.begin(), t.end(), [&](const MacroTarget& x) { return x.macro == m && x.paramId == knob.paramId; }), t.end());
            if (code != 99) t.push_back({m, knob.paramId, (float)(code - 50) / 40.f});
            proc.setMacroTargets(std::move(t));
        });
    }
    MuonProcessor& proc;
    Knob& knob;
    juce::String name;
};

Page::Page(MuonProcessor& p) : proc_(p) {}
Page::~Page() { menus_.clear(); }

Knob& Page::knob(const juce::String& id, const juce::String& title, Knob::Size sz, bool bipolar) {
    auto& k = own(std::make_unique<Knob>(proc_.apvts, id, title, sz, bipolar), id);
    menus_.push_back(std::make_unique<MacroMenu>(proc_, k, title));
    return k;
}
Choice& Page::choice(const juce::String& id, const juce::String& title) { return own(std::make_unique<Choice>(proc_.apvts, id, title), id); }
Toggle& Page::toggle(const juce::String& id, const juce::String& title) { return own(std::make_unique<Toggle>(proc_.apvts, id, title), id); }
float Page::value(const juce::String& id) const { return proc_.apvts.getRawParameterValue(id)->load(); }

void Page::paint(juce::Graphics& g) {
    for (const auto& s : sections_) drawSection(g, s.r.toFloat(), s.code, s.title);
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
    for (auto* comp : items) {
        auto cell = area.removeFromLeft(w);
        if (comp) comp->setBounds(cell);
        if (prev && comp && dynamic_cast<Knob*>(prev) && dynamic_cast<Knob*>(comp)) {
            const float x = (float)cell.getX() - gap * 0.5f;
            dividers_.push_back({x, (float)cell.getY() + 10.f, x, (float)cell.getBottom() - 6.f});
        }
        prev = comp;
        area.removeFromLeft(gap);
    }
}

// ---- SHAPE --------------------------------------------------------------------------------------------------
ShapePage::ShapePage(MuonProcessor& p) : Page(p) {
    choice("bands", "Bands"); choice("xType", "Crossover"); choice("stereo", "Stereo"); choice("look", "Lookahead");
    knob("xLow", "Low split", Knob::Small); knob("xHigh", "High split", Knob::Small); knob("sens", "Sensitivity", Knob::Small);
    for (int b = 1; b <= muon::kMaxBands; ++b) {
        const juce::String x = "b" + juce::String(b);
        knob(x + "Atk", "Attack", Knob::Large, true); knob(x + "Sus", "Sustain", Knob::Large, true);
        knob(x + "Spd", "Speed", Knob::Small); knob(x + "Clip", "Clip", Knob::Small);
        toggle(x + "Mute", "Mute");
    }
}

void ShapePage::tick() {
    const int bands = (int)std::lround(value("bands")) + 1;
    setOn(c("xLow"), bands >= 2); setOn(c("xHigh"), bands >= 3);
    for (int b = 1; b <= muon::kMaxBands; ++b) {
        const juce::String x = "b" + juce::String(b);
        for (const juce::String id : {"Atk", "Sus", "Spd", "Clip", "Mute"}) setOn(c(x + id), b <= bands);
    }
    // Band titles follow the crossover points.
    const float lo = value("xLow"), hi = value("xHigh");
    juce::String titles[3];
    if (bands == 1) titles[0] = "Full band";
    else if (bands == 2) { titles[0] = "Low  < " + hzText(lo); titles[1] = "High  > " + hzText(lo); }
    else { titles[0] = "Low  < " + hzText(lo); titles[1] = "Mid  " + hzText(lo) + " - " + hzText(hi); titles[2] = "High  > " + hzText(hi); }
    bool changed = false;
    for (int b = 0; b < 3; ++b) {
        const juce::String t = titles[b].isEmpty() ? juce::String("Band ") + juce::String(b + 1) + "  (off)" : titles[b];
        if (b + 1 < (int)sections_.size() && sections_[(size_t)b + 1].title != t) { sections_[(size_t)b + 1].title = t; changed = true; }
    }
    if (changed) repaint();
}

void ShapePage::resized() {
    beginLayout();
    auto r = getLocalBounds();
    auto left = r.removeFromLeft(std::clamp(r.getWidth() / 4, 230, 300)); r.removeFromLeft(24);
    section("01", "Split", left.removeFromTop(14)); left.removeFromTop(6);
    auto sel = left.removeFromTop(std::min(100, left.getHeight() / 3));
    auto s1 = sel.removeFromTop(sel.getHeight() / 2), s2 = sel;
    c("bands")->setBounds(s1.removeFromLeft(s1.getWidth() / 2 - 4)); s1.removeFromLeft(8); c("xType")->setBounds(s1);
    c("stereo")->setBounds(s2.removeFromLeft(s2.getWidth() / 2 - 4)); s2.removeFromLeft(8); c("look")->setBounds(s2);
    left.removeFromTop(8);
    row(left, {c("xLow"), c("xHigh"), c("sens")});
    const int gap = 22, w = (r.getWidth() - 2 * gap) / 3;
    for (int b = 1; b <= muon::kMaxBands; ++b) {
        const juce::String x = "b" + juce::String(b);
        auto col = r.removeFromLeft(w); r.removeFromLeft(gap);
        auto head = col.removeFromTop(14);
        c(x + "Mute")->setBounds(head.removeFromRight(70).withSizeKeepingCentre(70, 22));
        section("0" + juce::String(b + 1), "Band " + juce::String(b), head);
        col.removeFromTop(6);
        row(col.removeFromTop(col.getHeight() * 58 / 100), {c(x + "Atk"), c(x + "Sus")});
        row(col, {c(x + "Spd"), c(x + "Clip")});
    }
    tick();
}

// ---- Order bank ------------------------------------------------------------------------------------------------
const char* const OrderBank::kBankShapes[7] = {"Tube", "Tape", "Transformer", "Square", "Saw", "Octaver", "Bell"};

OrderBank::OrderBank(MuonProcessor& p, int band) : proc_(p), band_(band) {
    setTitle("Order bank, band " + juce::String(band + 1));
    setDescription("Faders H1 to H8: the level of each harmonic order; the lamps under H2 to H8 invert that order's phase");
}

juce::RangedAudioParameter* OrderBank::param(int order, bool invert) const {
    const juce::String x = "b" + juce::String(band_ + 1);
    return proc_.apvts.getParameter(invert ? x + "Inv" + juce::String(order + 1) : x + "H" + juce::String(order + 1));
}
juce::Rectangle<float> OrderBank::faderRect(int o) const {
    auto r = getLocalBounds().toFloat();
    r.removeFromTop(16.f); r.removeFromBottom(30.f);
    const float cw = r.getWidth() / 8.f;
    return {r.getX() + cw * o + cw * 0.28f, r.getY(), cw * 0.44f, r.getHeight()};
}
juce::Rectangle<float> OrderBank::lampRect(int o) const {
    const auto f = faderRect(o);
    return {f.getCentreX() - 6.f, f.getBottom() + 12.f, 12.f, 12.f};
}
int OrderBank::orderAt(juce::Point<int> p) const {
    const float cw = getWidth() / 8.f;
    return juce::jlimit(0, 7, (int)(p.x / cw));
}
void OrderBank::set(int o, float v) {
    auto* prm = param(o);
    if (gesture_ != o) { if (gesture_ >= 0) param(gesture_)->endChangeGesture(); prm->beginChangeGesture(); gesture_ = o; }
    prm->setValueNotifyingHost(juce::jlimit(0.f, 1.f, v));
    repaint();
}
void OrderBank::mouseDown(const juce::MouseEvent& e) {
    const int o = orderAt(e.getPosition());
    gesture_ = -1;
    if (o > 0 && lampRect(o).expanded(6.f).contains(e.position)) {
        auto* inv = param(o, true);
        proc_.undoManager.beginNewTransaction("Invert H" + juce::String(o + 1));
        inv->beginChangeGesture(); inv->setValueNotifyingHost(inv->getValue() > 0.5f ? 0.f : 1.f); inv->endChangeGesture();
        repaint();
        return;
    }
    proc_.undoManager.beginNewTransaction("Order bank");
    mouseDrag(e);
}
void OrderBank::mouseDrag(const juce::MouseEvent& e) {
    const int o = orderAt(e.getPosition());
    const auto f = faderRect(o);
    if (e.y > f.getBottom() + 8 && gesture_ < 0) return;
    set(o, (f.getBottom() - (float)e.y) / f.getHeight());
}
void OrderBank::mouseUp(const juce::MouseEvent&) { if (gesture_ >= 0) param(gesture_)->endChangeGesture(); gesture_ = -1; }
void OrderBank::mouseDoubleClick(const juce::MouseEvent& e) {
    const int o = orderAt(e.getPosition());
    set(o, param(o)->getDefaultValue());
    mouseUp(e);
}
void OrderBank::applyShape(int shape) {
    static const float w[7][8] = {
        {1, 0.5f, 0.12f, 0.2f, 0.05f, 0.1f, 0, 0.05f},          // Tube: falling even orders
        {1, 0, 0.5f, 0, 0.15f, 0, 0.05f, 0},                    // Tape: mostly 3rd, gentle 5th
        {1, 0.3f, 0.4f, 0, 0, 0, 0, 0},                         // Transformer: 2nd and 3rd (use on the low band)
        {1, 0, 0.333f, 0, 0.2f, 0, 0.143f, 0},                  // Square: odd orders 1/n
        {1, 0.5f, 0.333f, 0.25f, 0.2f, 0.167f, 0.143f, 0.125f}, // Saw: all orders 1/n
        {1, 1, 0, 0, 0, 0, 0, 0},                               // Octaver: H2 only
        {0.4f, 0, 0.7f, 0, 0.5f, 0, 0.35f, 0},                  // Bell: H3, H5, H7 over a reduced fundamental
    };
    proc_.undoManager.beginNewTransaction(juce::String("Order bank: ") + kBankShapes[shape]);
    for (int o = 0; o < 8; ++o) {
        auto* prm = param(o);
        prm->beginChangeGesture(); prm->setValueNotifyingHost(prm->convertTo0to1(w[shape][o])); prm->endChangeGesture();
        if (o > 0) { auto* inv = param(o, true); inv->beginChangeGesture(); inv->setValueNotifyingHost(0.f); inv->endChangeGesture(); }
    }
    repaint();
}
void OrderBank::tick() {
    bool dirty = false;
    for (int o = 0; o < 8; ++o) { const float v = param(o)->getValue() + (o > 0 && param(o, true)->getValue() > 0.5f ? 10.f : 0.f); if (std::abs(v - shown_[o]) > 1e-7f) { shown_[o] = v; dirty = true; } }
    const int mo = proc_.engine.maxOrder(band_);
    if (mo != maxOrder_) { maxOrder_ = mo; dirty = true; }
    if (dirty) repaint();
}
void OrderBank::paint(juce::Graphics& g) {
    const float amount = proc_.apvts.getRawParameterValue("b" + juce::String(band_ + 1) + "Harm")->load();
    for (int o = 0; o < 8; ++o) {
        const auto f = faderRect(o);
        const bool clean = o < maxOrder_;
        drawCaps(g, "H" + juce::String(o + 1), juce::Rectangle<float>(f.getCentreX() - 20.f, 0.f, 40.f, 13.f), juce::Justification::centred, 9.f, clean ? kTextDim : kTextFaint);
        g.setColour(kDisplayBg); g.fillRoundedRectangle(f, 4.f);
        g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawRoundedRectangle(f, 4.f, 1.f);
        for (int t = 1; t < 4; ++t) { g.setColour(kLine); g.drawHorizontalLine((int)(f.getY() + f.getHeight() * t / 4.f), f.getX() + 2.f, f.getRight() - 2.f); }
        const float v = param(o)->getValue();
        if (v > 0.001f) {
            auto bar = f.reduced(3.f).withTop(f.getBottom() - 3.f - (f.getHeight() - 6.f) * v);
            const juce::Colour col = !clean ? kTextFaint.withAlpha(0.5f) : o == 0 ? kTextDim : kAccent.withAlpha(0.45f + 0.55f * amount);
            if (clean && o > 0) { g.setColour(kAccent.withAlpha(0.18f)); g.fillRoundedRectangle(bar.expanded(3.f), 4.f); }
            g.setColour(col); g.fillRoundedRectangle(bar, 2.5f);
        }
        if (o > 0) {
            const auto l = lampRect(o);
            const bool inv = param(o, true)->getValue() > 0.5f;
            g.setColour(inv ? kAccent.brighter(0.3f) : juce::Colour(0xff2e3238)); g.fillEllipse(l.reduced(2.f));
            if (inv) { g.setColour(kAccent.withAlpha(0.3f)); g.fillEllipse(l.expanded(2.f)); }
            drawCaps(g, juce::String::fromUTF8("\xc3\x98"), l.translated(0.f, -1.f), juce::Justification::centred, 8.f, inv ? kDisplayBg : kTextFaint);
        } else {
            drawCaps(g, "Dry", juce::Rectangle<float>(f.getCentreX() - 20.f, f.getBottom() + 10.f, 40.f, 14.f), juce::Justification::centred, 8.f, kTextFaint);
        }
    }
}

// ---- HARMONICS ----------------------------------------------------------------------------------------------
HarmonicsPage::HarmonicsPage(MuonProcessor& p) : Page(p) {
    for (int b = 1; b <= muon::kMaxBands; ++b) {
        const juce::String x = "b" + juce::String(b);
        choice(x + "Src", "Source"); knob(x + "Harm", "Harmonics", Knob::Large);
        choice(x + "Curve", "Curve"); knob(x + "Drv", "Drive"); knob(x + "Bias", "Bias", Knob::Normal, true); knob(x + "Tone", "Tone", Knob::Normal, true);
        knob(x + "DMix", "Mix");
        banks_.push_back(std::make_unique<OrderBank>(p, b - 1));
        addChildComponent(*banks_.back());
    }
    knob("sub", "Octave down", Knob::Small); knob("sub2", "Two down", Knob::Small); knob("subPur", "Purity", Knob::Small); knob("subRange", "Range", Knob::Small);
    knob("phantom", "Phantom", Knob::Small); knob("phCut", "Cut", Knob::Small); knob("mono", "Mono below", Knob::Small);
    shapes_.setButtonText("Shapes"); shapes_.setTitle("Order bank shapes");
    shapes_.onClick = [this] {
        juce::PopupMenu m;
        m.addSectionHeader("Order bank shape, band " + juce::String(band_ + 1));
        for (int i = 0; i < 7; ++i) m.addItem(i + 1, OrderBank::kBankShapes[i]);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&shapes_), [this](int r) { if (r > 0) banks_[(size_t)band_]->applyShape(r - 1); });
    };
    addAndMakeVisible(shapes_);
    showBand(0);
}

void HarmonicsPage::showBand(int b) {
    band_ = juce::jlimit(0, muon::kMaxBands - 1, b);
    for (int i = 0; i < muon::kMaxBands; ++i) {
        const juce::String x = "b" + juce::String(i + 1);
        for (const juce::String id : {"Src", "Harm", "Curve", "Drv", "Bias", "Tone", "DMix"}) if (auto* comp = c(x + id)) comp->setVisible(i == band_);
        banks_[(size_t)i]->setVisible(i == band_);
    }
    repaint();
}

void HarmonicsPage::mouseDown(const juce::MouseEvent& e) {
    if (!bandTabs_.contains(e.getPosition())) return;
    const int b = (int)((e.x - bandTabs_.getX()) / (bandTabs_.getWidth() / 3.f));
    if (b < bands_) showBand(b);
}

void HarmonicsPage::tick() {
    const int bands = (int)std::lround(value("bands")) + 1;
    if (bands != bands_) { bands_ = bands; if (band_ >= bands) showBand(bands - 1); repaint(); }
    banks_[(size_t)band_]->tick();
    const juce::String x = "b" + juce::String(band_ + 1);
    const bool drive = value(x + "Drv") > 0.f;
    setOn(c(x + "Bias"), drive); setOn(c(x + "Tone"), drive); setOn(c(x + "DMix"), drive);
    const bool sub = value("sub") > 0.f || value("sub2") > 0.f;
    setOn(c("subPur"), sub);
    setOn(c("phCut"), value("phantom") > 0.f);
}

void HarmonicsPage::paint(juce::Graphics& g) {
    Page::paint(g);
    // Band selector: a segmented switch.
    const auto r = bandTabs_.toFloat();
    g.setColour(kDisplayBg); g.fillRoundedRectangle(r, 5.f);
    g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawRoundedRectangle(r.reduced(0.5f), 5.f, 1.f);
    const float w = r.getWidth() / 3.f;
    for (int i = 0; i < 3; ++i) {
        auto cell = juce::Rectangle<float>(r.getX() + w * i, r.getY(), w, r.getHeight()).reduced(3.f);
        if (i == band_) {
            g.setColour(juce::Colour(0xff20242a)); g.fillRoundedRectangle(cell, 4.f);
            g.setColour(kAccent); g.fillRoundedRectangle(cell.removeFromBottom(2.f).reduced(cell.getWidth() * 0.3f, 0.f), 1.f);
        }
        drawCaps(g, "Band " + juce::String(i + 1), juce::Rectangle<float>(r.getX() + w * i, r.getY(), w, r.getHeight()), juce::Justification::centred, 9.5f,
                 i >= bands_ ? kTextFaint.withAlpha(0.5f) : i == band_ ? kText : kTextDim);
    }
}

void HarmonicsPage::resized() {
    beginLayout();
    auto r = getLocalBounds();
    const int gap = 22;
    auto left = r.removeFromLeft(std::clamp(r.getWidth() * 17 / 100, 160, 230)); r.removeFromLeft(gap);
    auto bank = r.removeFromLeft(r.getWidth() * 36 / 100); r.removeFromLeft(gap);
    auto drive = r.removeFromLeft(r.getWidth() * 47 / 100); r.removeFromLeft(gap);
    auto sub = r;
    section("03", "Harmonics", left.removeFromTop(14)); left.removeFromTop(6);
    bandTabs_ = left.removeFromTop(28); left.removeFromTop(8);
    auto srcRow = left.removeFromTop(42);
    for (int b = 1; b <= muon::kMaxBands; ++b) {
        const juce::String x = "b" + juce::String(b);
        c(x + "Src")->setBounds(srcRow.withTrimmedRight(80));
        c(x + "Harm")->setBounds(left.withTrimmedTop(6));
    }
    shapes_.setBounds(srcRow.removeFromRight(72).withSizeKeepingCentre(72, 26).translated(0, 7));
    section("04", "Order bank", bank.removeFromTop(14)); bank.removeFromTop(6);
    for (auto& b : banks_) b->setBounds(bank);
    section("05", "Drive", drive.removeFromTop(14)); drive.removeFromTop(6);
    auto curveRow = drive.removeFromTop(42);
    drive.removeFromTop(4);
    for (int b = 1; b <= muon::kMaxBands; ++b) {
        const juce::String x = "b" + juce::String(b);
        c(x + "Curve")->setBounds(curveRow);
        auto d = drive;
        auto top = d.removeFromTop(d.getHeight() / 2);
        const auto dividersBefore = dividers_.size();
        row(top, {c(x + "Drv"), c(x + "Bias")});
        row(d, {c(x + "Tone"), c(x + "DMix")});
        if (b != 1) dividers_.resize(dividersBefore);   // one set of dividers for the shared cells
    }
    section("06", "Sub", sub.removeFromTop(14)); sub.removeFromTop(6);
    auto s1 = sub.removeFromTop(sub.getHeight() / 2);
    row(s1, {c("sub"), c("sub2"), c("subPur"), c("subRange")});
    row(sub, {c("phantom"), c("phCut"), c("mono")});
}

// ---- RESONANCE ----------------------------------------------------------------------------------------------
void KeyStrip::tick() {
    std::vector<int> lit;
    const int n = proc_.engine.meters.resCount.load(std::memory_order_relaxed);
    for (int i = 0; i < n; ++i) {
        const float hz = proc_.engine.meters.resHz[i].load(std::memory_order_relaxed);
        if (hz > 0.f) lit.push_back((int)std::lround(69.0 + 12.0 * std::log2(hz / 440.0)));
    }
    if (proc_.apvts.getRawParameterValue("resMix")->load() <= 0.f && proc_.apvts.getRawParameterValue("glowSend")->load() <= 0.f) lit.clear();
    if (lit != lit_) { lit_ = lit; repaint(); }
}

void KeyStrip::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    const int lo = 24, hi = 108;                // C1 .. C8
    int whites = 0; for (int n = lo; n < hi; ++n) { const int k = n % 12; if (k != 1 && k != 3 && k != 6 && k != 8 && k != 10) ++whites; }
    const float ww = r.getWidth() / whites;
    auto isLit = [&](int n) { return std::find(lit_.begin(), lit_.end(), n) != lit_.end(); };
    float x = r.getX();
    std::vector<std::pair<float, int>> blacks;
    for (int n = lo; n < hi; ++n) {
        const int k = n % 12;
        if (k == 1 || k == 3 || k == 6 || k == 8 || k == 10) { blacks.push_back({x, n}); continue; }
        auto key = juce::Rectangle<float>(x + 0.5f, r.getY(), ww - 1.f, r.getHeight());
        g.setColour(isLit(n) ? kAccent : juce::Colour(0xff272a2f)); g.fillRoundedRectangle(key, 1.5f);
        if (isLit(n)) { g.setColour(kAccent.withAlpha(0.3f)); g.fillRoundedRectangle(key.expanded(1.5f), 2.f); }
        if (k == 0) drawCaps(g, "C" + juce::String(n / 12 - 1), key.removeFromBottom(12.f), juce::Justification::centred, 7.5f, isLit(n) ? kDisplayBg : kTextFaint);
        x += ww;
    }
    for (auto& [bx, n] : blacks) {
        auto key = juce::Rectangle<float>(bx - ww * 0.32f, r.getY(), ww * 0.64f, r.getHeight() * 0.6f);
        g.setColour(isLit(n) ? kAccent.brighter(0.2f) : juce::Colour(0xff0e1013)); g.fillRoundedRectangle(key, 1.5f);
    }
}

ResonancePage::ResonancePage(MuonProcessor& p) : Page(p), keys_(p) {
    choice("resMode", "Mode"); choice("resIn", "Struck by"); choice("pitch", "Pitch");
    choice("chord", "Chord"); choice("scale", "Scale"); choice("material", "Material");
    knob("root", "Root"); knob("size", "Size"); knob("spread", "Spread"); knob("resStereo", "Stereo");
    knob("resDecay", "Decay"); knob("resDamp", "Damping"); knob("resTone", "Tone", Knob::Normal, true); knob("strike", "Strike");
    knob("resMix", "Resonance", Knob::Large);
    choice("glowSrc", "Source");
    knob("glowSend", "Send", Knob::Large); knob("glowSize", "Size"); knob("glowDecay", "Decay"); knob("glowDamp", "Damping");
    knob("glowPre", "Pre-delay"); knob("glowMix", "Return");
    addAndMakeVisible(keys_);
}

void ResonancePage::tick() {
    keys_.tick();
    const int mode = (int)std::lround(value("resMode")), pitch = (int)std::lround(value("pitch"));
    setOn(c("chord"), mode == 0 && (pitch == 0 || pitch == 3));
    setOn(c("scale"), mode == 0 && pitch == 1);
    setOn(c("material"), mode == 1);
    setOn(c("root"), pitch == 0 || pitch == 1 || (pitch == 3));
    setOn(c("size"), mode != 0);
    setOn(c("spread"), mode != 1);
    const bool glow = value("glowSend") > 0.f;
    for (const juce::String id : {"glowSize", "glowDecay", "glowDamp", "glowPre", "glowMix"}) setOn(c(id), glow);
}

void ResonancePage::resized() {
    beginLayout();
    auto r = getLocalBounds();
    const int gap = 22;
    auto res = r.removeFromLeft(r.getWidth() * 64 / 100); r.removeFromLeft(gap);
    auto glow = r;
    section("07", "Resonance", res.removeFromTop(14)); res.removeFromTop(6);
    keys_.setBounds(res.removeFromBottom(std::clamp(res.getHeight() / 7, 30, 46))); res.removeFromBottom(8);
    auto sel = res.removeFromTop(42);
    const int sw = (sel.getWidth() - 5 * 8) / 6;
    for (const juce::String id : {"resMode", "resIn", "pitch", "chord", "scale", "material"}) { c(id)->setBounds(sel.removeFromLeft(sw)); sel.removeFromLeft(8); }
    res.removeFromTop(6);
    auto mix = res.removeFromRight(res.getWidth() / 5);
    c("resMix")->setBounds(mix);
    row(res.removeFromTop(res.getHeight() / 2), {c("root"), c("size"), c("spread"), c("resStereo")});
    row(res, {c("resDecay"), c("resDamp"), c("resTone"), c("strike")});
    section("08", "Afterglow", glow.removeFromTop(14)); glow.removeFromTop(6);
    c("glowSrc")->setBounds(glow.removeFromTop(42).removeFromLeft(std::min(180, glow.getWidth())));
    glow.removeFromTop(6);
    auto top = glow.removeFromTop(glow.getHeight() / 2);
    row(top, {c("glowSend"), c("glowMix")});
    row(glow, {c("glowSize"), c("glowDecay"), c("glowDamp"), c("glowPre")});
}

// ---- OUTPUT -------------------------------------------------------------------------------------------------
OutputPage::OutputPage(MuonProcessor& p) : Page(p) {
    knob("mix", "Mix", Knob::Large); knob("outDb", "Output", Knob::Large, true);
    toggle("autoGain", "Auto gain"); toggle("clipGuard", "Clip guard");
    choice("listen", "Listen"); choice("os", "Oversampling");
}

void OutputPage::tick() {
    const auto& e = proc_.engine;
    const int lat = proc_.latencyShown.load();
    const double fs = e.sampleRate();
    juce::String s = "Latency " + juce::String(lat) + " samples (" + juce::String(1000.0 * lat / std::max(1.0, fs), 2) + " ms)";
    const int bands = (int)std::lround(value("bands")) + 1;
    s << "\nClean harmonic orders: ";
    for (int b = 0; b < bands; ++b) s << (b ? ", " : "") << "band " << (b + 1) << " up to H" << e.maxOrder(b);
    s << "\nAuto gain " << juce::String(e.meters.autoGainDb.load(), 1) << " dB";
    if (s != shown_) { shown_ = s; repaint(info_); }
}

void OutputPage::paint(juce::Graphics& g) {
    Page::paint(g);
    g.setFont(font(12.5f)); g.setColour(kTextDim);
    g.drawFittedText(shown_, info_, juce::Justification::topLeft, 4);
}

void OutputPage::resized() {
    beginLayout();
    auto r = getLocalBounds();
    auto out = r.removeFromLeft(r.getWidth() * 45 / 100); r.removeFromLeft(24);
    section("09", "Output", out.removeFromTop(14)); out.removeFromTop(6);
    auto tg = out.removeFromRight(160);
    c("autoGain")->setBounds(tg.removeFromTop(tg.getHeight() / 2).withSizeKeepingCentre(150, 26));
    c("clipGuard")->setBounds(tg.withSizeKeepingCentre(150, 26));
    row(out, {c("mix"), c("outDb")});
    section("10", "Monitor and engine", r.removeFromTop(14)); r.removeFromTop(6);
    auto sel = r.removeFromTop(42);
    c("listen")->setBounds(sel.removeFromLeft(sel.getWidth() / 2 - 6)); sel.removeFromLeft(12);
    c("os")->setBounds(sel);
    r.removeFromTop(14);
    info_ = r.removeFromTop(70);
}

// ---- Macros ---------------------------------------------------------------------------------------------------
MacroBay::MacroBay(MuonProcessor& p) {
    for (int m = 0; m < MuonProcessor::kNumMacros; ++m) {
        knobs_.push_back(std::make_unique<Knob>(p.apvts, "macro" + juce::String(m + 1), MuonProcessor::kMacroNames[m], Knob::Large));
        addAndMakeVisible(*knobs_.back());
    }
    punch_ = std::make_unique<Knob>(p.apvts, "punch", "Punch", Knob::Normal, true);
    tail_ = std::make_unique<Knob>(p.apvts, "tail", "Tail", Knob::Normal, true);
    addAndMakeVisible(*punch_); addAndMakeVisible(*tail_);
}
void MacroBay::paint(juce::Graphics& g) {
    auto r = getLocalBounds();
    drawSection(g, r.removeFromTop(14).toFloat(), "M", "Macros");
    for (int m = 0; m < MuonProcessor::kNumMacros; ++m) {
        const auto kb = knobs_[(size_t)m]->getBounds();
        drawCaps(g, MuonProcessor::kMacroMeanings[m], juce::Rectangle<float>((float)kb.getX(), (float)kb.getBottom(), (float)kb.getWidth(), 12.f),
                 juce::Justification::centred, 7.5f, kTextFaint);
    }
    const float x = (float)punch_->getX() - 8.f;
    g.setColour(juce::Colours::black.withAlpha(0.35f)); g.drawLine(x, (float)punch_->getY() + 10.f, x, (float)tail_->getBottom() - 6.f, 1.f);
    g.setColour(juce::Colours::white.withAlpha(0.045f)); g.drawLine(x + 1.f, (float)punch_->getY() + 10.f, x + 1.f, (float)tail_->getBottom() - 6.f, 1.f);
}
void MacroBay::resized() {
    auto r = getLocalBounds();
    r.removeFromTop(18);
    r.removeFromBottom(14);
    auto side = r.removeFromRight(std::max(70, r.getWidth() / 6)); r.removeFromRight(16);
    punch_->setBounds(side.removeFromTop(side.getHeight() / 2)); tail_->setBounds(side);
    const int w = r.getWidth() / 4;
    for (int m = 0; m < 4; ++m) knobs_[(size_t)m]->setBounds(r.removeFromLeft(w));
}

// ---- Main display ---------------------------------------------------------------------------------------------
MainDisplay::MainDisplay(MuonProcessor& p, std::function<sun::Drive()> drive) : sun(sun::Mode::Limb, std::move(drive), false), proc_(p) {
    setTitle("Display"); setDescription("Track view of the input and output with the transients, the output spectrum with the crossover points, and meters");
    sun.overlay = [this](juce::Graphics& g) { paintScope(g); };
    sun.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(sun);
    frames_.assign(kFrames, {});
    fftBuf_.assign((size_t)2 * 4096, 0.f);
    spec_.assign(2048, -120.f);
    startTimerHz(30);
}
void MainDisplay::resized() { sun.setBounds(getLocalBounds()); }
void MainDisplay::paint(juce::Graphics& g) { g.setColour(kDisplayBg); g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.f); }

void MainDisplay::pullFrames() {
    muon::TrackFrame f;
    int got = 0;
    auto live = [](const muon::TrackFrame& t) { return t.inMax - t.inMin > 1e-6f || t.outMax - t.outMin > 1e-6f; };
    while (proc_.engine.track.pop(f)) {
        liveFrames_ += (live(f) ? 1 : 0) - (live(frames_[(size_t)head_]) ? 1 : 0);
        frames_[(size_t)head_] = f;
        if (f.onset) {
            Streak s; s.frame = head_; s.age = 0.f;
            s.y = juce::Random::getSystemRandom().nextFloat();
            s.slant = juce::Random::getSystemRandom().nextFloat() * 2.f - 1.f;
            streaks_.push_back(s);
        }
        head_ = (head_ + 1) % kFrames;
        ++got;
    }
    if (got > 0 && liveFrames_ > 0) tracesDirty_ = true;     // signal on screen scrolls; an all-silent ring stays still
    if (!streaks_.empty()) tracesDirty_ = true;
    if (inHold_ > 1e-5f || outHold_ > 1e-5f) tracesDirty_ = true;
    const float dt = got * 128.f / (float)std::max(1.0, proc_.engine.sampleRate());
    for (auto& s : streaks_) s.age += dt;
    streaks_.erase(std::remove_if(streaks_.begin(), streaks_.end(), [](const Streak& s) { return s.age > 0.6f; }), streaks_.end());
}

void MainDisplay::paintScope(juce::Graphics& g) {
    auto all = getLocalBounds().toFloat();
    drawDisplay(g, all, 8.f, false);
    const int W = std::max(1, getWidth()), H = std::max(1, getHeight());
    // The traces (and their halation) are redrawn only when what they show changed: new signal, a moving streak,
    // falling meters, the analyser catching up, or a parameter they draw. A quiet scope just recomposites the
    // cached layers over the sun's newest frame (optimization guide 3.2).
    const juce::String state = juce::String(proc_.apvts.getRawParameterValue("bands")->load()) + "|" + juce::String(proc_.apvts.getRawParameterValue("xLow")->load())
        + "|" + juce::String(proc_.apvts.getRawParameterValue("xHigh")->load()) + "|" + caption + "|" + juce::String(hoverHandle_) + juce::String(dragHandle_);
    if (state != drawnState_) { drawnState_ = state; tracesDirty_ = true; }
    const int pos = proc_.engine.meters.scopePos.load(std::memory_order_relaxed);
    if (pos != lastScopePos_) { lastScopePos_ = pos; tracesDirty_ = true; }
    if (spectrumLive_) tracesDirty_ = true;
    if (!traceLayer_.isValid() || traceLayer_.getWidth() != W || traceLayer_.getHeight() != H) {
        traceLayer_ = juce::Image(juce::Image::ARGB, W, H, true, juce::SoftwareImageType());
        scanlines_ = juce::Image(juce::Image::ARGB, W, H, true, juce::SoftwareImageType());
        juce::Graphics sg(scanlines_);
        juce::Path clip; clip.addRoundedRectangle(all, 8.f); sg.reduceClipRegion(clip);
        sg.setColour(juce::Colours::black.withAlpha(0.13f));
        for (float y = all.getY() + 1.f; y < all.getBottom(); y += 3.f) sg.fillRect(all.getX(), y, all.getWidth(), 1.f);
        tracesDirty_ = true;
    }
    if (tracesDirty_) {
        tracesDirty_ = false;
        traceLayer_.clear(traceLayer_.getBounds(), juce::Colours::transparentBlack);
        { juce::Graphics lg(traceLayer_); paintTraces(lg, all); }
        glow_ = traceLayer_.rescaled(std::max(1, W / 2), std::max(1, H / 2), juce::Graphics::mediumResamplingQuality);
        gaussianBlur(glow_, 2, 1.8f);
    }
    g.setOpacity(0.75f); g.drawImageTransformed(glow_, juce::AffineTransform::scale((float)W / glow_.getWidth(), (float)H / glow_.getHeight()), false);
    g.setOpacity(1.f); g.drawImageAt(traceLayer_, 0, 0);
    g.drawImageAt(scanlines_, 0, 0);
}

void MainDisplay::paintTraces(juce::Graphics& g, juce::Rectangle<float> all) {
    auto r = all.reduced(16.f, 12.f);
    if (caption.isNotEmpty()) {
        auto cap = r.removeFromBottom(14.f);
        g.setFont(font(11.f)); g.setColour(kAmberText);
        g.drawText(caption, cap, juce::Justification::centredLeft, true);
        r.removeFromBottom(4.f);
    }
    auto meters = r.removeFromRight(70.f); r.removeFromRight(16.f);
    auto track = r.removeFromLeft(r.getWidth() * 0.56f); r.removeFromLeft(18.f);
    g.setColour(kAmberGrid);
    g.drawVerticalLine((int)(track.getRight() + 9.f), r.getY(), r.getBottom());
    g.drawVerticalLine((int)(r.getRight() + 8.f), r.getY(), r.getBottom());
    paintTrack(g, track);
    paintSpectrum(g, r);
    paintMeters(g, meters);
}

void MainDisplay::paintTrack(juce::Graphics& g, juce::Rectangle<float> r) {
    const float gainNow = proc_.engine.meters.autoGainDb.load(std::memory_order_relaxed);
    (void)gainNow;
    drawCaps(g, "Track  input / output, transients", r.removeFromTop(12.f), juce::Justification::centredLeft, 9.f, kAmberText);
    r.removeFromTop(4.f);
    const float mid = r.getCentreY(), half = r.getHeight() * 0.46f;
    g.setColour(kAmberGrid); g.drawHorizontalLine((int)mid, r.getX(), r.getRight());
    auto lvl = [&](float v) { const float db = 20.f * std::log10(std::max(std::abs(v), 1e-4f)); return juce::jlimit(0.f, 1.f, (db + 60.f) / 60.f); };
    // One column per pixel: the frames under each column are folded together (min/max, strongest mask), so the paths
    // have a few hundred points rather than 1200.
    const int n = kFrames, cols = std::max(1, (int)r.getWidth());
    const float dx = r.getWidth() / (float)n, cw = r.getWidth() / (float)cols;
    juce::Path in, outTop, outBot, gain;
    juce::RectangleList<float> hot;
    for (int c = 0; c < cols; ++c) {
        const int i0 = c * n / cols, i1 = std::max(i0 + 1, (c + 1) * n / cols);
        float inA = 0.f, outA = 0.f, mask = 0.f, gdb = 0.f;
        for (int i = i0; i < i1; ++i) {
            const auto& f = frames_[(size_t)((head_ + i) % n)];
            inA = std::max(inA, std::max(std::abs(f.inMin), std::abs(f.inMax)));
            outA = std::max(outA, std::max(std::abs(f.outMin), std::abs(f.outMax)));
            mask = std::max(mask, f.mask);
            if (std::abs(f.gainDb) > std::abs(gdb)) gdb = f.gainDb;
        }
        const float x = r.getX() + cw * c;
        const float a = lvl(inA) * half, o = lvl(outA) * half;
        in.addRectangle(x, mid - a, cw + 0.5f, 2.f * a);
        if (c == 0) { outTop.startNewSubPath(x, mid - o); outBot.startNewSubPath(x, mid + o); }
        else { outTop.lineTo(x, mid - o); outBot.lineTo(x, mid + o); }
        const float gy = mid - juce::jlimit(-24.f, 24.f, gdb) / 24.f * half;
        if (c == 0) gain.startNewSubPath(x, gy); else gain.lineTo(x, gy);
        if (mask >= 0.3f) { hot.add(x, mid - o - 1.f, cw + 0.6f, 2.5f); hot.add(x, mid + o - 1.f, cw + 0.6f, 2.5f); }
    }
    g.setColour(kAmber.withAlpha(0.16f)); g.fillPath(in);
    g.setColour(kAmber); g.strokePath(outTop, juce::PathStrokeType(1.4f)); g.strokePath(outBot, juce::PathStrokeType(1.4f));
    // Transient segments: the output envelope burns hotter where the mask is up.
    g.setColour(kAmberHot.withAlpha(0.8f)); g.fillRectList(hot);
    g.setColour(kAmberHot.withAlpha(0.45f));
    g.strokePath(gain, juce::PathStrokeType(0.8f), juce::AffineTransform());
    // Onset tracks: a short streak per detected transient, like a particle in a cloud chamber (Cherenkov blue).
    for (const auto& s : streaks_) {
        const int age = (head_ - s.frame + n) % n;
        const float x = r.getRight() - dx * age;
        const float y = r.getY() + r.getHeight() * (0.15f + 0.7f * s.y);
        const float a = juce::jlimit(0.f, 1.f, 1.f - s.age / 0.6f);
        const float len = 18.f + 26.f * s.age;
        juce::Line<float> l(x, y, x + len * 0.9f, y - len * 0.45f * s.slant);
        g.setColour(kAccent.withAlpha(0.35f * a)); g.drawLine(l, 4.f);
        g.setColour(kAccent.brighter(0.6f).withAlpha(0.9f * a)); g.drawLine(l, 1.3f);
    }
    drawCaps(g, "Gain", juce::Rectangle<float>(r.getRight() - 40.f, r.getY(), 40.f, 10.f), juce::Justification::centredRight, 7.5f, kAmberGrid.withAlpha(0.6f));
}

float MainDisplay::freqToX(float f) const { return specArea_.getX() + specArea_.getWidth() * std::log(juce::jlimit(20.f, 20000.f, f) / 20.f) / std::log(1000.f); }
float MainDisplay::xToFreq(float x) const { return 20.f * std::pow(1000.f, juce::jlimit(0.f, 1.f, (x - specArea_.getX()) / specArea_.getWidth())); }

void MainDisplay::paintSpectrum(juce::Graphics& g, juce::Rectangle<float> r) {
    const int bands = (int)std::lround(proc_.apvts.getRawParameterValue("bands")->load()) + 1;
    const float lo = proc_.apvts.getRawParameterValue("xLow")->load(), hi = proc_.apvts.getRawParameterValue("xHigh")->load();
    const float tracked = proc_.engine.meters.trackedHz.load(std::memory_order_relaxed);
    juce::String title = "Spectrum";
    if (tracked > 0.f && (int)std::lround(proc_.apvts.getRawParameterValue("pitch")->load()) == 3) title << "  tracked " << hzText(tracked);
    drawCaps(g, title, r.removeFromTop(12.f), juce::Justification::centredLeft, 9.f, kAmberText);
    r.removeFromTop(4.f);
    specArea_ = r;
    // FFT of the newest 4096 output samples (Hann), smoothed with a slow fall.
    auto& m = proc_.engine.meters;
    const int N = 4096, pos = m.scopePos.load(std::memory_order_acquire);
    for (int i = 0; i < N; ++i) {
        const float w = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * i / (N - 1));
        fftBuf_[(size_t)i] = m.scope[(size_t)((pos + i) % muon::Meters::kScopeSize)].load(std::memory_order_relaxed) * w;
    }
    std::fill(fftBuf_.begin() + N, fftBuf_.end(), 0.f);
    fft_.performFrequencyOnlyForwardTransform(fftBuf_.data(), true);
    const double fs = std::max(1.0, proc_.engine.sampleRate());
    spectrumLive_ = false;
    for (int k = 1; k < N / 2; ++k) {
        const float db = 20.f * std::log10(std::max(fftBuf_[(size_t)k] * 4.f / N, 1e-7f));
        spec_[(size_t)k] = db > spec_[(size_t)k] ? db : std::max(-120.f, spec_[(size_t)k] - 1.2f);
        if (spec_[(size_t)k] > -95.f) spectrumLive_ = true;     // still falling into view: keep redrawing
    }
    auto Y = [&](float db) { return r.getY() + r.getHeight() * (0.f - juce::jlimit(-90.f, 0.f, db)) / 90.f; };
    g.setColour(kAmberGrid);
    for (float f : {100.f, 1000.f, 10000.f}) g.drawVerticalLine((int)freqToX(f), r.getY(), r.getBottom());
    for (float db : {-30.f, -60.f}) g.drawHorizontalLine((int)Y(db), r.getX(), r.getRight());
    juce::Path p, fill;
    bool started = false;
    for (int i = 0; i <= 160; ++i) {
        const float f = 20.f * std::pow(1000.f, i / 160.f);
        const float kf = (float)(f * N / fs);
        const int k0 = juce::jlimit(1, N / 2 - 2, (int)kf);
        const float db = std::max(spec_[(size_t)k0], spec_[(size_t)k0 + 1]);
        const juce::Point<float> q(freqToX(f), Y(db));
        if (!started) { p.startNewSubPath(q); fill.startNewSubPath(q.x, r.getBottom()); started = true; }
        p.lineTo(q); fill.lineTo(q);
    }
    fill.lineTo(r.getRight(), r.getBottom()); fill.closeSubPath();
    g.setGradientFill(juce::ColourGradient(kAmber.withAlpha(0.18f), 0.f, r.getY(), kAmber.withAlpha(0.f), 0.f, r.getBottom(), false));
    g.fillPath(fill);
    g.setColour(kAmber); g.strokePath(p, juce::PathStrokeType(1.3f));
    // Harmonic orders of the tracked pitch.
    if (tracked > 0.f)
        for (int h = 1; h <= 8; ++h) { const float x = freqToX(tracked * h); g.setColour(kAmberHot.withAlpha(h == 1 ? 0.6f : 0.3f)); g.drawLine(x, r.getBottom() - 8.f, x, r.getBottom(), 1.f); }
    // Crossover handles.
    const float handles[2] = {lo, hi};
    for (int i = 0; i < 2; ++i) {
        if (bands < i + 2) continue;
        const float x = freqToX(handles[i]);
        const bool hot = dragHandle_ == i || hoverHandle_ == i;
        g.setColour(kAmber.withAlpha(hot ? 0.9f : 0.5f));
        const float dash[] = {3.f, 3.f};
        g.drawDashedLine(juce::Line<float>(x, r.getY() + 10.f, x, r.getBottom()), dash, 2, 1.f);
        g.setColour(kDisplayBg); g.fillEllipse(x - 5.f, r.getY(), 10.f, 10.f);
        g.setColour(hot ? kAmberHot : kAmber); g.drawEllipse(x - 4.5f, r.getY() + 0.5f, 9.f, 9.f, 1.4f);
        drawCaps(g, hzText(handles[i]), juce::Rectangle<float>(x + 7.f, r.getY(), 70.f, 10.f), juce::Justification::centredLeft, 7.5f, kAmberText);
    }
}

void MainDisplay::paintMeters(juce::Graphics& g, juce::Rectangle<float> r) {
    auto& m = proc_.engine.meters;
    drawCaps(g, "Levels", r.removeFromTop(12.f), juce::Justification::centredLeft, 9.f, kAmberText);
    r.removeFromTop(4.f);
    auto labels = r.removeFromBottom(12.f);
    const float w = (r.getWidth() - 3 * 6.f) / 4.f;
    auto frac = [](float pk) { const float db = 20.f * std::log10(std::max(pk, 1e-5f)); return juce::jlimit(0.f, 1.f, (db + 60.f) / 60.f); };
    inHold_ = std::max(m.inPeak.load(std::memory_order_relaxed), inHold_ * 0.92f);
    outHold_ = std::max(m.outPeak.load(std::memory_order_relaxed), outHold_ * 0.92f);
    const float vals[4] = {frac(inHold_), frac(outHold_), 0.f, juce::jlimit(0.f, 1.f, m.resonance.load(std::memory_order_relaxed) * 2.f)};
    const char* names[4] = {"In", "Out", "AG", "Res"};
    for (int i = 0; i < 4; ++i) {
        auto bar = juce::Rectangle<float>(r.getX() + i * (w + 6.f), r.getY(), w, r.getHeight());
        g.setColour(kAmberGrid); g.fillRoundedRectangle(bar, 2.f);
        if (i == 2) {   // auto-gain, bipolar around the centre
            const float db = juce::jlimit(-12.f, 12.f, m.autoGainDb.load(std::memory_order_relaxed));
            const float c = bar.getCentreY(), y = c - db / 12.f * bar.getHeight() * 0.5f;
            g.setColour(kAmber); g.fillRoundedRectangle(bar.getX(), std::min(c, y), bar.getWidth(), std::max(1.f, std::abs(y - c)), 2.f);
        } else {
            const bool hot = i == 1 && 20.f * std::log10(std::max(outHold_, 1e-5f)) > -3.f;
            g.setGradientFill(juce::ColourGradient(hot ? kDanger : kAmberHot, 0.f, bar.getY(), kAmber, 0.f, bar.getBottom(), false));
            g.fillRoundedRectangle(bar.withTop(bar.getBottom() - bar.getHeight() * vals[i]), 2.f);
        }
        drawCaps(g, names[i], juce::Rectangle<float>(bar.getX() - 4.f, labels.getY(), bar.getWidth() + 8.f, labels.getHeight()), juce::Justification::centred, 7.5f, kAmberText);
    }
    const bool limiting = m.limiterGain.load(std::memory_order_relaxed) < 0.97f;
    g.setColour(limiting ? kDanger : juce::Colour(0xff313236)); g.fillEllipse(r.getX() + w + 6.f + w * 0.5f - 3.f, r.getY() - 10.f, 6.f, 6.f);
}

void MainDisplay::mouseMove(const juce::MouseEvent& e) {
    int h = -1;
    const int bands = (int)std::lround(proc_.apvts.getRawParameterValue("bands")->load()) + 1;
    if (specArea_.expanded(4.f).contains(e.position))
        for (int i = 0; i < 2; ++i) {
            if (bands < i + 2) continue;
            const float x = freqToX(proc_.apvts.getRawParameterValue(i == 0 ? "xLow" : "xHigh")->load());
            if (std::abs(e.position.x - x) < 6.f) h = i;
        }
    if (h != hoverHandle_) { hoverHandle_ = h; setMouseCursor(h >= 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor); }
}
void MainDisplay::mouseDown(const juce::MouseEvent& e) {
    mouseMove(e);
    dragHandle_ = hoverHandle_;
    if (dragHandle_ >= 0) {
        proc_.undoManager.beginNewTransaction("Move crossover");
        proc_.apvts.getParameter(dragHandle_ == 0 ? "xLow" : "xHigh")->beginChangeGesture();
    }
}
void MainDisplay::mouseDrag(const juce::MouseEvent& e) {
    if (dragHandle_ < 0) return;
    auto* prm = proc_.apvts.getParameter(dragHandle_ == 0 ? "xLow" : "xHigh");
    prm->setValueNotifyingHost(prm->convertTo0to1(xToFreq(e.position.x)));
}
void MainDisplay::mouseUp(const juce::MouseEvent&) {
    if (dragHandle_ >= 0) proc_.apvts.getParameter(dragHandle_ == 0 ? "xLow" : "xHigh")->endChangeGesture();
    dragHandle_ = -1;
}

}  // namespace panels
