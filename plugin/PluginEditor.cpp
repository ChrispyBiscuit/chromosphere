#include "PluginEditor.h"
#include "fdn/decay_design.h"
#include "er/er_default_table.h"
#include <cmath>

namespace {
const juce::Colour kBg(0xff14171c), kPanel(0xff1c2027), kPanelEdge(0xff2a303a), kText(0xffe6e9ee), kDim(0xff8a93a1),
    kAccent(0xff4fc3a1), kAccent2(0xff6aa7ff), kWarn(0xffe0a458), kGrid(0xff2a303a);
}

// ------------------------------------------------------------------------------------------------
class FancyReverbEditor::Look : public juce::LookAndFeel_V4 {
public:
    Look() {
        setColour(juce::ResizableWindow::backgroundColourId, kBg);
        setColour(juce::Label::textColourId, kText);
        setColour(juce::Slider::textBoxTextColourId, kText);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::ComboBox::backgroundColourId, kPanel);
        setColour(juce::ComboBox::textColourId, kText);
        setColour(juce::ComboBox::outlineColourId, kPanelEdge);
        setColour(juce::PopupMenu::backgroundColourId, kPanel);
        setColour(juce::PopupMenu::textColourId, kText);
        setColour(juce::TextButton::buttonColourId, kPanel);
        setColour(juce::TextButton::textColourOffId, kText);
        setColour(juce::ToggleButton::textColourId, kText);
        setColour(juce::ToggleButton::tickColourId, kAccent);
        setColour(juce::ListBox::backgroundColourId, kPanel);
        setColour(juce::TextEditor::backgroundColourId, kBg);
        setColour(juce::TextEditor::textColourId, kText);
        setColour(juce::TextEditor::outlineColourId, kPanelEdge);
    }
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s) override {
        const auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(4.f);
        const float r = std::min(b.getWidth(), b.getHeight()) * 0.5f, cx = b.getCentreX(), cy = b.getCentreY();
        const float th = std::max(2.f, r * 0.16f), ra = r - th * 0.5f;
        const bool on = s.isEnabled();
        juce::Path track; track.addCentredArc(cx, cy, ra, ra, 0.f, start, end, true);
        g.setColour(kGrid); g.strokePath(track, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const float a = start + pos * (end - start);
        juce::Path val; val.addCentredArc(cx, cy, ra, ra, 0.f, start, a, true);
        g.setColour(on ? (s.hasKeyboardFocus(false) ? kAccent2 : kAccent) : kDim.withAlpha(0.4f));
        g.strokePath(val, juce::PathStrokeType(th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path ptr; const float pl = ra - th * 1.2f;
        ptr.addLineSegment(juce::Line<float>(0.f, -pl * 0.35f, 0.f, -pl), th * 0.6f);
        g.setColour(on ? kText : kDim);
        g.fillPath(ptr, juce::AffineTransform::rotation(a).translated(cx, cy));
    }
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool hl, bool) override {
        const auto r = b.getLocalBounds().toFloat().reduced(1.f);
        const float box = std::min(r.getHeight() * 0.7f, 18.f);
        const auto sq = juce::Rectangle<float>(r.getX() + 2.f, r.getCentreY() - box * 0.5f, box, box);
        g.setColour(b.getToggleState() ? kAccent : kGrid); g.fillRoundedRectangle(sq, 3.f);
        if (hl || b.hasKeyboardFocus(false)) { g.setColour(kAccent2); g.drawRoundedRectangle(sq.expanded(1.5f), 4.f, 1.5f); }
        g.setColour(b.isEnabled() ? kText : kDim);
        g.setFont(juce::FontOptions(std::max(11.f, r.getHeight() * 0.5f)));
        g.drawText(b.getButtonText(), r.withTrimmedLeft(box + 8.f), juce::Justification::centredLeft);
    }
    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down) override {
        const auto r = b.getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(down ? kAccent.withAlpha(0.5f) : hl ? kPanelEdge.brighter(0.2f) : kPanel);
        g.fillRoundedRectangle(r, 5.f);
        g.setColour(b.hasKeyboardFocus(false) ? kAccent2 : kPanelEdge);
        g.drawRoundedRectangle(r, 5.f, 1.f);
    }
};

// ------------------------------------------------------------------------------------------------
class FancyReverbEditor::Knob : public juce::Component {
public:
    Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& title, const juce::String& suffix = {}, int decimals = 2)
        : paramId(id) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
        slider.setTextValueSuffix(suffix.isEmpty() ? juce::String() : " " + suffix);
        slider.setNumDecimalPlacesToDisplay(decimals);
        slider.setTitle(title);
        slider.setDescription(title);
        slider.setDoubleClickReturnValue(true, apvts.getParameter(id)->convertFrom0to1(apvts.getParameter(id)->getDefaultValue()));
        label.setText(title, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, kDim);
        label.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(slider); addAndMakeVisible(label);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, id, slider);
        setWantsKeyboardFocus(false);
    }
    void resized() override {
        // Name, dial and value read as one group, centred in the cell so big cells do not spread them apart.
        const int nameH = std::clamp(getHeight() / 6, 14, 20), textH = std::clamp(getHeight() / 6, 14, 20);
        const int side = std::max(20, std::min(getWidth() - 4, getHeight() - nameH - textH - 4));
        const int groupH = nameH + side + textH, top = std::max(0, (getHeight() - groupH) / 2);
        label.setBounds(0, top, getWidth(), nameH);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, std::max(50, getWidth() - 4), textH);
        slider.setBounds((getWidth() - side) / 2 - 10, top + nameH, side + 20, side + textH);
    }
    juce::String paramId;
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// ------------------------------------------------------------------------------------------------
namespace {
void drawPanel(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title) {
    g.setColour(kPanel); g.fillRoundedRectangle(r, 6.f);
    g.setColour(kPanelEdge); g.drawRoundedRectangle(r, 6.f, 1.f);
    g.setColour(kDim); g.setFont(juce::FontOptions(12.f));
    g.drawText(title, r.reduced(8.f, 4.f).removeFromTop(16.f), juce::Justification::topLeft);
}
}  // namespace

class FancyReverbEditor::DecayView : public juce::Component {
public:
    explicit DecayView(FancyReverbProcessor& p) : proc(p) { setTitle("Decay time against frequency"); setDescription("Reverb time in seconds for each frequency"); }
    void paint(juce::Graphics& g) override {
        const auto all = getLocalBounds().toFloat();
        drawPanel(g, all, "Decay time (T60) against frequency");
        auto r = all.reduced(36.f, 10.f); r.removeFromTop(14.f); r.removeFromBottom(14.f);
        const double fmin = 63.0, fmax = 16000.0;
        double tmax = 1.0;
        for (int i = 0; i <= 64; ++i) tmax = std::max(tmax, proc.displayedT60(fmin * std::pow(fmax / fmin, i / 64.0)));
        tmax = std::ceil(tmax * 1.1);
        auto X = [&](double f) { return r.getX() + r.getWidth() * (float)(std::log(f / fmin) / std::log(fmax / fmin)); };
        auto Y = [&](double t) { return r.getBottom() - r.getHeight() * (float)std::min(1.0, t / tmax); };
        g.setFont(juce::FontOptions(10.f));
        for (double f : {125., 250., 500., 1000., 2000., 4000., 8000.}) {
            g.setColour(kGrid); g.drawVerticalLine((int)X(f), r.getY(), r.getBottom());
            g.setColour(kDim); g.drawText(f >= 1000 ? juce::String((int)(f / 1000)) + "k" : juce::String((int)f), (int)X(f) - 16, (int)r.getBottom() + 1, 32, 12, juce::Justification::centred);
        }
        for (int i = 0; i <= 4; ++i) {
            const double t = tmax * i / 4.0;
            g.setColour(kGrid); g.drawHorizontalLine((int)Y(t), r.getX(), r.getRight());
            g.setColour(kDim); g.drawText(juce::String(t, t < 10 ? 1 : 0) + " s", (int)all.getX() + 2, (int)Y(t) - 6, 32, 12, juce::Justification::centredRight);
        }
        juce::Path path;
        for (int i = 0; i <= 160; ++i) {
            const double f = fmin * std::pow(fmax / fmin, i / 160.0);
            const auto p = juce::Point<float>(X(f), Y(proc.displayedT60(f)));
            if (i == 0) path.startNewSubPath(p); else path.lineTo(p);
        }
        g.setColour(kAccent); g.strokePath(path, juce::PathStrokeType(2.2f, juce::PathStrokeType::curved));
        for (double f : {125., 1000., 8000.}) {   // the three anchor frequencies the engine fits
            g.setColour(kAccent2); g.fillEllipse(X(f) - 3.5f, Y(proc.displayedT60(f)) - 3.5f, 7.f, 7.f);
        }
    }
    FancyReverbProcessor& proc;
};

class FancyReverbEditor::EnvelopeView : public juce::Component {
public:
    explicit EnvelopeView(FancyReverbProcessor& p) : proc(p) { setTitle("Reverb level over time"); setDescription("Level of the reverb output in the last few seconds"); }
    void paint(juce::Graphics& g) override {
        const auto all = getLocalBounds().toFloat();
        drawPanel(g, all, "Reverb output level (last " + juce::String(FancyReverbProcessor::kEnvSize * 1024.0 / std::max(1.0, proc.getSampleRate()), 1) + " s)");
        auto r = all.reduced(36.f, 10.f); r.removeFromTop(14.f);
        auto Y = [&](float db) { return r.getBottom() - r.getHeight() * juce::jlimit(0.f, 1.f, (db + 90.f) / 90.f); };
        g.setFont(juce::FontOptions(10.f));
        for (float db : {0.f, -30.f, -60.f, -90.f}) {
            g.setColour(kGrid); g.drawHorizontalLine((int)Y(db), r.getX(), r.getRight());
            g.setColour(kDim); g.drawText(juce::String((int)db), (int)all.getX() + 2, (int)Y(db) - 6, 30, 12, juce::Justification::centredRight);
        }
        const int n = FancyReverbProcessor::kEnvSize, pos = proc.envPos.load(std::memory_order_acquire);
        juce::Path p;
        for (int i = 0; i < n; ++i) {
            const float db = proc.envRing[(size_t)((pos + i) % n)].load(std::memory_order_relaxed);
            const juce::Point<float> pt(r.getX() + r.getWidth() * i / (float)(n - 1), Y(db));
            if (i == 0) p.startNewSubPath(r.getX(), r.getBottom()), p.lineTo(pt); else p.lineTo(pt);
        }
        p.lineTo(r.getRight(), r.getBottom()); p.closeSubPath();
        g.setColour(kAccent2.withAlpha(0.35f)); g.fillPath(p);
        g.setColour(kAccent2); g.strokePath(p, juce::PathStrokeType(1.4f));
    }
    FancyReverbProcessor& proc;
};

class FancyReverbEditor::ErView : public juce::Component {
public:
    explicit ErView(FancyReverbProcessor& p) : proc(p) { setTitle("Early reflections"); setDescription("Early reflection taps, or the start of the impulse response"); }
    void paint(juce::Graphics& g) override {
        const bool algo = proc.apvts.getRawParameterValue("mode")->load() < 0.5f;
        const auto all = getLocalBounds().toFloat();
        drawPanel(g, all, algo ? "Early reflections (delay and level of each tap)" : "Impulse response, first 500 ms (level in dB)");
        auto r = all.reduced(36.f, 10.f); r.removeFromTop(14.f); r.removeFromBottom(12.f);
        g.setFont(juce::FontOptions(10.f));
        if (algo) {
            const double size = proc.apvts.getRawParameterValue("size")->load(), maxMs = 60.0 * std::max(1.0, size);
            const int taps = proc.apvts.getRawParameterValue("quality")->load() < 0.5f ? 16 : proc.apvts.getRawParameterValue("quality")->load() < 1.5f ? 32 : 64;
            const float lvl = proc.apvts.getRawParameterValue("erLevel")->load();
            for (int i = 0; i < std::min(taps, fr::kErDefaultCount); ++i) {
                const auto& t = fr::kErDefaultTable[i];
                const float x = r.getX() + r.getWidth() * (float)(t.delaySec * 1000.0 * size / maxMs);
                const float db = juce::jlimit(-60.f, 0.f, 20.f * std::log10(std::max(t.gain, 1e-4f)) + lvl);
                const float y = r.getBottom() - r.getHeight() * (db + 60.f) / 60.f;
                if (x > r.getRight()) continue;
                g.setColour((t.pan < 0 ? kAccent : kAccent2).withAlpha(0.9f));
                g.drawLine(x, r.getBottom(), x, y, 2.f); g.fillEllipse(x - 2.5f, y - 2.5f, 5.f, 5.f);
            }
            for (int ms = 0; ms <= (int)maxMs; ms += (maxMs > 100 ? 40 : 20)) {
                const float x = r.getX() + r.getWidth() * (float)(ms / maxMs);
                g.setColour(kDim); g.drawText(juce::String(ms) + " ms", (int)x - 20, (int)r.getBottom() + 1, 40, 12, juce::Justification::centred);
            }
        } else if (auto info = proc.getIrInfo()) {
            juce::Path p;
            const int n = (int)info->preview.size();
            for (int i = 0; i < n; ++i) {
                const float x = r.getX() + r.getWidth() * i / (float)std::max(1, n - 1);
                const float y = r.getBottom() - r.getHeight() * juce::jlimit(0.f, 1.f, (info->preview[(size_t)i] + 60.f) / 60.f);
                if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
            }
            g.setColour(kAccent); g.strokePath(p, juce::PathStrokeType(1.8f));
            g.setColour(kDim);
            for (int ms = 0; ms <= 500; ms += 100)
                g.drawText(juce::String(ms) + " ms", (int)(r.getX() + r.getWidth() * ms / 500.f) - 20, (int)r.getBottom() + 1, 40, 12, juce::Justification::centred);
        } else {
            g.setColour(kWarn); g.drawText("No impulse response loaded", r, juce::Justification::centred);
        }
    }
    FancyReverbProcessor& proc;
};

class FancyReverbEditor::MeterBar : public juce::Component {
public:
    MeterBar(std::atomic<float>& src, const juce::String& n) : source(src), name(n) { setTitle(n + " level meter"); }
    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        auto lab = r.removeFromBottom(16.f);
        g.setColour(kDim); g.setFont(juce::FontOptions(11.f)); g.drawText(name, lab, juce::Justification::centred);
        r.reduce(r.getWidth() * 0.25f, 4.f);
        g.setColour(kPanel); g.fillRoundedRectangle(r, 3.f);
        const float db = 20.f * std::log10(std::max(source.load(std::memory_order_relaxed), 1e-5f));
        const float frac = juce::jlimit(0.f, 1.f, (db + 60.f) / 60.f);
        auto fill = r.withTop(r.getBottom() - r.getHeight() * frac);
        g.setColour(db > -1.f ? juce::Colour(0xffe05a5a) : db > -12.f ? kWarn : kAccent);
        g.fillRoundedRectangle(fill, 3.f);
        g.setColour(kPanelEdge); g.drawRoundedRectangle(r, 3.f, 1.f);
    }
    std::atomic<float>& source;
    juce::String name;
};

// Preset bar: previous/next, name button opening a searchable browser, save/delete, A/B, undo/redo.
class FancyReverbEditor::PresetBar : public juce::Component {
public:
    explicit PresetBar(FancyReverbProcessor& p) : proc(p) {
        for (auto* b : {&prev, &next, &name, &save, &del, &ab, &copy, &undo, &redo}) addAndMakeVisible(b);
        prev.setButtonText("<"); next.setButtonText(">"); save.setButtonText("Save"); del.setButtonText("Delete");
        ab.setButtonText("A"); copy.setButtonText("A>B"); undo.setButtonText("Undo"); redo.setButtonText("Redo");
        prev.setTitle("Previous preset"); next.setTitle("Next preset"); name.setTitle("Preset browser"); save.setTitle("Save user preset");
        del.setTitle("Delete user preset"); ab.setTitle("Toggle A/B comparison"); copy.setTitle("Copy this setting to the other slot");
        undo.setTitle("Undo"); redo.setTitle("Redo");
        prev.onClick = [this] { step(-1); };
        next.onClick = [this] { step(+1); };
        name.onClick = [this] { openBrowser(); };
        save.onClick = [this] { askSave(); };
        del.onClick = [this] { if (proc.presets.deleteUser(proc.presets.currentIndex())) refresh(); };
        ab.onClick = [this] { proc.presets.toggleAB(); refresh(); };
        copy.onClick = [this] { proc.presets.copyToOtherSlot(); };
        undo.onClick = [this] { proc.undoManager.undo(); };
        redo.onClick = [this] { proc.undoManager.redo(); };
        refresh();
    }
    void refresh() {
        auto n = proc.presets.currentName();
        name.setButtonText(n.isEmpty() ? juce::String("(no preset)") : n);
        ab.setButtonText(proc.presets.activeSlot() == 0 ? "A" : "B");
        const int ci = proc.presets.currentIndex();
        del.setEnabled(ci >= 0 && !proc.presets.presets()[(size_t)ci].factory);
        undo.setEnabled(proc.undoManager.canUndo()); redo.setEnabled(proc.undoManager.canRedo());
    }
    void resized() override {
        auto r = getLocalBounds().reduced(0, 4);
        const int h = r.getHeight(), small = std::max(34, h + 10);
        prev.setBounds(r.removeFromLeft(h)); r.removeFromLeft(4);
        next.setBounds(r.removeFromRight(h)); r.removeFromRight(8);
        redo.setBounds(r.removeFromRight(small + 10)); r.removeFromRight(4);
        undo.setBounds(r.removeFromRight(small + 10)); r.removeFromRight(10);
        copy.setBounds(r.removeFromRight(small + 4)); r.removeFromRight(4);
        ab.setBounds(r.removeFromRight(h + 6)); r.removeFromRight(10);
        del.setBounds(r.removeFromRight(small + 16)); r.removeFromRight(4);
        save.setBounds(r.removeFromRight(small + 6)); r.removeFromRight(10);
        name.setBounds(r);
    }
    void step(int d) {
        const auto& ps = proc.presets.presets();
        if (ps.empty()) return;
        int i = proc.presets.currentIndex();
        i = i < 0 ? 0 : (i + d + (int)ps.size()) % (int)ps.size();
        proc.presets.apply(i); refresh();
    }
    void openBrowser();
    void askSave() {
        auto* w = new juce::AlertWindow("Save preset", "Name for the new user preset:", juce::MessageBoxIconType::NoIcon);
        w->addTextEditor("name", proc.presets.currentName() + " (copy)");
        w->addTextEditor("tags", "", "Tags (comma separated)");
        w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        w->enterModalState(true, juce::ModalCallbackFunction::create([this, w](int r) {
            if (r == 1) {
                juce::StringArray tags; tags.addTokens(w->getTextEditorContents("tags"), ",", ""); tags.trim(); tags.removeEmptyStrings();
                proc.presets.saveUser(w->getTextEditorContents("name"), "User", tags);
                refresh();
            }
            delete w;
        }), false);
    }
    FancyReverbProcessor& proc;
    juce::TextButton prev, next, name, save, del, ab, copy, undo, redo;
};

namespace {
class PresetBrowser : public juce::Component, private juce::ListBoxModel {
public:
    PresetBrowser(FancyReverbProcessor& p, std::function<void()> onPick) : proc(p), picked(std::move(onPick)) {
        search.setTextToShowWhenEmpty("Search name, category or tag", kDim);
        search.onTextChange = [this] { rebuild(); };
        search.setTitle("Search presets");
        list.setModel(this); list.setRowHeight(24); list.setTitle("Presets");
        addAndMakeVisible(search); addAndMakeVisible(list);
        rebuild();
        setSize(420, 460);
    }
    void resized() override { auto r = getLocalBounds().reduced(6); search.setBounds(r.removeFromTop(26)); r.removeFromTop(4); list.setBounds(r); }
    void visibilityChanged() override { if (isVisible()) search.grabKeyboardFocus(); }
private:
    void rebuild() {
        rows.clear();
        const auto q = search.getText().trim().toLowerCase();
        const auto& ps = proc.presets.presets();
        juce::StringArray cats;
        for (auto& p : ps) cats.addIfNotAlreadyThere(p.category);
        for (auto& c : cats) {
            bool header = false;
            for (int i = 0; i < (int)ps.size(); ++i) {
                const auto& p = ps[(size_t)i];
                if (p.category != c) continue;
                if (q.isNotEmpty() && !(p.name.toLowerCase().contains(q) || p.category.toLowerCase().contains(q) || p.tags.joinIntoString(" ").toLowerCase().contains(q))) continue;
                if (!header) { rows.push_back({-1, c}); header = true; }
                rows.push_back({i, p.name});
            }
        }
        list.updateContent(); list.repaint();
    }
    int getNumRows() override { return (int)rows.size(); }
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool sel) override {
        if (row < 0 || row >= (int)rows.size()) return;
        const auto& r = rows[(size_t)row];
        if (r.index < 0) { g.setColour(kDim); g.setFont(juce::FontOptions(12.f, juce::Font::bold)); g.drawText(r.text.toUpperCase(), 8, 0, w - 8, h, juce::Justification::centredLeft); return; }
        if (sel || r.index == proc.presets.currentIndex()) { g.setColour(kAccent.withAlpha(sel ? 0.4f : 0.2f)); g.fillRect(0, 0, w, h); }
        g.setColour(kText); g.setFont(juce::FontOptions(14.f));
        g.drawText(r.text + (proc.presets.presets()[(size_t)r.index].factory ? "" : "  (user)"), 18, 0, w - 18, h, juce::Justification::centredLeft);
    }
    void listBoxItemClicked(int row, const juce::MouseEvent&) override { choose(row); }
    void returnKeyPressed(int row) override { choose(row); }
    void choose(int row) {
        if (row < 0 || row >= (int)rows.size() || rows[(size_t)row].index < 0) return;
        proc.presets.apply(rows[(size_t)row].index);
        picked();
    }
    struct Row { int index; juce::String text; };
    FancyReverbProcessor& proc;
    std::function<void()> picked;
    juce::TextEditor search;
    juce::ListBox list;
    std::vector<Row> rows;
};
}  // namespace

void FancyReverbEditor::PresetBar::openBrowser() {
    auto content = std::make_unique<PresetBrowser>(proc, [this] { refresh(); });
    juce::CallOutBox::launchAsynchronously(std::move(content), name.getScreenBounds(), nullptr);
}

// ------------------------------------------------------------------------------------------------
FancyReverbEditor::FancyReverbEditor(FancyReverbProcessor& p) : AudioProcessorEditor(&p), proc_(p) {
    look_ = std::make_unique<Look>();
    setLookAndFeel(look_.get());

    auto& a = proc_.apvts;
    auto addKnob = [&](const char* id, const char* title, const char* suffix = "", int dec = 2) {
        knobs_.push_back(std::make_unique<Knob>(a, id, title, suffix, dec)); addAndMakeVisible(*knobs_.back());
    };
    // Order matters for resized(): primary (4), secondary (5), advanced (the rest).
    addKnob("size", "Size", "", 2); addKnob("decayMid", "Decay (1 kHz)", "s", 2); addKnob("preDelay", "Pre-delay", "ms", 1); addKnob("mix", "Mix", "%", 0);
    addKnob("decayLow", "Decay low", "s", 2); addKnob("decayHigh", "Decay high", "s", 2); addKnob("erLevel", "ER / IR level", "dB", 1);
    addKnob("tailLevel", "Tail level", "dB", 1); addKnob("outGain", "Output", "dB", 1);
    addKnob("modDepth", "Mod depth", "", 2); addKnob("modRate", "Mod rate", "Hz", 2);
    addKnob("erMs", "Hybrid ER length", "", 0); addKnob("tailScale", "Hybrid tail decay", "x", 2);
    addKnob("irStart", "IR start", "ms", 0); addKnob("irLength", "IR length", "", 0); addKnob("irStretch", "IR stretch", "x", 2);
    addKnob("irLowCut", "IR low cut", "", 0); addKnob("irHighCut", "IR high cut", "", 0); addKnob("irWidth", "IR width", "", 2);
    addKnob("irDecay", "IR decay", "x", 2);

    decay_ = std::make_unique<DecayView>(proc_); env_ = std::make_unique<EnvelopeView>(proc_); er_ = std::make_unique<ErView>(proc_);
    inMeter_ = std::make_unique<MeterBar>(proc_.inPeak, "In"); outMeter_ = std::make_unique<MeterBar>(proc_.outPeak, "Out");
    tailMeter_ = std::make_unique<MeterBar>(proc_.tailPeak, "Tail");
    presetBar_ = std::make_unique<PresetBar>(proc_);
    for (auto* c : std::initializer_list<juce::Component*>{decay_.get(), env_.get(), er_.get(), inMeter_.get(), outMeter_.get(), tailMeter_.get(), presetBar_.get()})
        addAndMakeVisible(c);

    mode_.addItemList({"Algorithmic", "Convolution", "Hybrid"}, 1); quality_.addItemList({"Eco", "Normal", "High"}, 1);
    mode_.setTitle("Mode"); quality_.setTitle("Quality"); factoryIr_.setTitle("Factory impulse response");
    modeAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(a, "mode", mode_);
    qualityAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(a, "quality", quality_);
    freezeAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(a, "freeze", freeze_);
    diffusionAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(a, "diffusion", diffusion_);
    reverseAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(a, "irReverse", reverse_);
    factoryIr_.addItem("Factory IR...", 1);
    {
        int id = 2;
        for (auto& n : FancyReverbProcessor::factoryImpulseResponses()) factoryIr_.addItem(n.replaceCharacter('_', ' '), id++);
    }
    factoryIr_.setSelectedId(1, juce::dontSendNotification);
    factoryIr_.onChange = [this] {
        const int idx = factoryIr_.getSelectedId() - 2;
        const auto names = FancyReverbProcessor::factoryImpulseResponses();
        if (idx >= 0 && idx < names.size()) { proc_.loadFactoryImpulseResponse(names[idx]); }
        factoryIr_.setSelectedId(1, juce::dontSendNotification);
    };
    loadIr_.setTitle("Load an impulse response file");
    loadIr_.onClick = [this] { chooseIr(); };
    irName_.setColour(juce::Label::textColourId, kDim); irName_.setJustificationType(juce::Justification::centredLeft);
    freeze_.setTitle("Freeze: hold the reverb tail and mute the input");
    advanced_.onClick = [this] { setAdvancedVisible(advanced_.getToggleState()); };
    for (auto* c : std::initializer_list<juce::Component*>{&mode_, &quality_, &factoryIr_, &freeze_, &diffusion_, &reverse_, &advanced_, &loadIr_, &irName_})
        addAndMakeVisible(c);

    setResizable(true, true);
    setResizeLimits(840, 540, 1800, 1300);
    setSize(980, 640);
    updateEnabledStates();
    startTimerHz(30);
}

FancyReverbEditor::~FancyReverbEditor() { stopTimer(); setLookAndFeel(nullptr); }

void FancyReverbEditor::setAdvancedVisible(bool v) { advancedVisible_ = v; advanced_.setToggleState(v, juce::dontSendNotification); resized(); }

void FancyReverbEditor::chooseIr() {
    chooser_ = std::make_unique<juce::FileChooser>("Choose an impulse response", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
    chooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) {
        const auto f = fc.getResult();
        if (f.existsAsFile()) {
            const auto err = proc_.loadImpulseResponse(f);
            if (err.isNotEmpty()) juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Impulse response", err);
        }
    });
}

void FancyReverbEditor::updateEnabledStates() {
    const int mode = (int)std::lround(proc_.apvts.getRawParameterValue("mode")->load());
    const bool algo = mode == 0, conv = mode == 1, hybrid = mode == 2;
    auto setOn = [&](const char* id, bool on) { for (auto& k : knobs_) if (k->paramId == id) { k->slider.setEnabled(on); k->setAlpha(on ? 1.f : 0.4f); } };
    for (auto* id : {"decayLow", "decayMid", "decayHigh"}) setOn(id, algo);
    for (auto* id : {"modDepth", "modRate"}) setOn(id, !conv);
    setOn("tailLevel", !conv); setOn("erMs", hybrid); setOn("tailScale", hybrid);
    for (auto* id : {"irStart", "irLength", "irStretch", "irLowCut", "irHighCut", "irWidth", "irDecay"}) setOn(id, !algo);
    reverse_.setEnabled(!algo); loadIr_.setEnabled(!algo); factoryIr_.setEnabled(!algo);
    diffusion_.setEnabled(!conv); quality_.setEnabled(!conv);
    lastMode_ = mode;
}

void FancyReverbEditor::timerCallback() {
    const int mode = (int)std::lround(proc_.apvts.getRawParameterValue("mode")->load());
    if (mode != lastMode_) updateEnabledStates();
    const auto path = proc_.getImpulseResponsePath();
    const auto shown = path.isEmpty() ? juce::String(mode == 0 ? "" : "No impulse response loaded")
                                      : path.startsWith("factory:") ? "IR: " + path.fromFirstOccurrenceOf("factory:", false, false).replaceCharacter('_', ' ') + " (factory)"
                                                                    : "IR: " + juce::File(path).getFileName();
    if (irName_.getText() != shown) irName_.setText(shown, juce::dontSendNotification);
    presetBar_->refresh();
    decay_->repaint(); env_->repaint(); er_->repaint(); inMeter_->repaint(); outMeter_->repaint(); tailMeter_->repaint();
}

void FancyReverbEditor::paint(juce::Graphics& g) {
    g.fillAll(kBg);
    g.setColour(kPanel); g.fillRect(0, 0, getWidth(), presetBar_->getBottom() + 6);
    g.setColour(kPanelEdge); g.drawHorizontalLine(presetBar_->getBottom() + 6, 0.f, (float)getWidth());
}

void FancyReverbEditor::resized() {
    if (!presetBar_ || !tailMeter_) return;       // called by the base class before the children exist
    auto r = getLocalBounds();
    presetBar_->setBounds(r.removeFromTop(46).reduced(10, 2));
    r.removeFromTop(8);
    r.reduce(10, 0); r.removeFromBottom(10);
    auto meters = r.removeFromRight(std::clamp(r.getWidth() / 9, 90, 150));
    r.removeFromRight(8);
    auto left = r.removeFromLeft((int)(r.getWidth() * 0.46f));
    r.removeFromLeft(10);
    // Left: three displays.
    const int lh = left.getHeight();
    decay_->setBounds(left.removeFromTop((int)(lh * 0.38f))); left.removeFromTop(8);
    env_->setBounds(left.removeFromTop((int)(lh * 0.3f))); left.removeFromTop(8);
    er_->setBounds(left);
    // Right: row of selectors, primary knobs, secondary knobs, advanced.
    auto top = r.removeFromTop(30);
    {
        const int w = top.getWidth(), gap = 8;
        const int modeW = std::max(90, (int)(w * 0.28f)), qualW = std::max(78, (int)(w * 0.24f)), togW = std::max(62, (w - modeW - qualW - 3 * gap) / 2);
        mode_.setBounds(top.removeFromLeft(modeW)); top.removeFromLeft(gap);
        quality_.setBounds(top.removeFromLeft(qualW)); top.removeFromLeft(gap);
        freeze_.setBounds(top.removeFromLeft(togW)); top.removeFromLeft(gap);
        diffusion_.setBounds(top);
    }
    r.removeFromTop(8);
    const int rowsAvail = r.getHeight() - 34;
    auto layoutRow = [&](juce::Rectangle<int> row, int first, int count) {
        const int w = row.getWidth() / count;
        for (int i = 0; i < count; ++i) knobs_[(size_t)(first + i)]->setBounds(row.removeFromLeft(w).reduced(2));
    };
    const float primaryShare = advancedVisible_ ? 0.30f : 0.50f, secondaryShare = advancedVisible_ ? 0.22f : 0.36f;
    layoutRow(r.removeFromTop((int)(rowsAvail * primaryShare)), 0, 4);
    layoutRow(r.removeFromTop((int)(rowsAvail * secondaryShare)), 4, 5);
    auto bar = r.removeFromTop(30);
    advanced_.setBounds(bar.removeFromLeft(96));
    loadIr_.setBounds(bar.removeFromLeft(84).reduced(0, 2)); bar.removeFromLeft(6);
    factoryIr_.setBounds(bar.removeFromLeft(112).reduced(0, 2)); bar.removeFromLeft(6);
    reverse_.setBounds(bar.removeFromRight(104));
    irName_.setBounds(bar);
    const bool show = advancedVisible_;
    for (size_t i = 9; i < knobs_.size(); ++i) knobs_[i]->setVisible(show);
    if (show) {
        const int perRow = 5, n = (int)knobs_.size() - 9, rowsN = (n + perRow - 1) / perRow;
        const int rh = r.getHeight() / rowsN;
        for (int row = 0; row < rowsN; ++row) {
            auto line = r.removeFromTop(rh);
            const int cnt = std::min(perRow, n - row * perRow), w = line.getWidth() / perRow;
            for (int i = 0; i < cnt; ++i) knobs_[(size_t)(9 + row * perRow + i)]->setBounds(line.removeFromLeft(w).reduced(2));
        }
    }
    // Meters.
    auto mrow = meters;
    const int mw = mrow.getWidth() / 3;
    inMeter_->setBounds(mrow.removeFromLeft(mw)); outMeter_->setBounds(mrow.removeFromLeft(mw)); tailMeter_->setBounds(mrow);
}
