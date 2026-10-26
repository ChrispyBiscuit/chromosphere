#include "PluginEditor.h"
#include "fdn/decay_design.h"
#include "er/er_default_table.h"
#include <cmath>

using namespace studio;

// ------------------------------------------------------------------------------------------------
class ChromosphereEditor::Knob : public juce::Component {
public:
    Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& title, const juce::String& suffix = {}, int decimals = 2)
        : paramId(id), title_(title) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        slider.setTextValueSuffix(suffix.isEmpty() ? juce::String() : " " + suffix);
        slider.setNumDecimalPlacesToDisplay(decimals);
        slider.setTitle(title); slider.setDescription(title);
        slider.setDoubleClickReturnValue(true, apvts.getParameter(id)->convertFrom0to1(apvts.getParameter(id)->getDefaultValue()));
        addAndMakeVisible(slider);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, id, slider);
        setWantsKeyboardFocus(false);
    }
    void paint(juce::Graphics& g) override {
        drawCaps(g, title_, labelArea_.toFloat(), juce::Justification::centred, std::clamp(getHeight() / 11.f, 9.5f, 11.f), slider.isEnabled() ? kTextDim : kTextFaint);
    }
    void resized() override {
        const int nameH = 15, textH = 19;
        const int side = std::max(28, std::min(getWidth() - 8, getHeight() - nameH - textH - 6));
        const int groupH = nameH + side + textH + 4, top = std::max(0, (getHeight() - groupH) / 2);
        labelArea_ = {0, top, getWidth(), nameH};
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, std::clamp(getWidth() - 4, 60, 110), textH);
        slider.setBounds((getWidth() - side) / 2 - 10, top + nameH + 2, side + 20, side + textH + 2);
    }
    void setTitleText(const juce::String& t) { if (title_ != t) { title_ = t; slider.setTitle(t); slider.setDescription(t); repaint(); } }
    juce::String paramId;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
private:
    juce::String title_;
    juce::Rectangle<int> labelArea_;
};

// ------------------------------------------------------------------------------------------------
namespace {
void axisLabel(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, juce::Justification j) {
    g.setFont(font(9.5f)); g.setColour(kTextFaint); g.drawText(s, r, j, false);
}
}  // namespace

// One wide display: decay-vs-frequency pane, time pane (ER taps or IR), level history strip, meters.
class ChromosphereEditor::MainDisplay : public juce::Component {
public:
    explicit MainDisplay(ChromosphereProcessor& p) : proc(p) { setTitle("Display"); setDescription("Decay time against frequency, early reflections or impulse response, reverb level history, and level meters"); }
    void paint(juce::Graphics& g) override {
        auto all = getLocalBounds().toFloat();
        drawDisplay(g, all);
        auto r = all.reduced(14.f, 12.f);
        auto meters = r.removeFromRight(std::clamp(r.getWidth() * 0.09f, 64.f, 96.f));
        r.removeFromRight(18.f);
        auto left = r.removeFromLeft(r.getWidth() * 0.56f);
        r.removeFromLeft(18.f);
        g.setColour(kLine); g.drawVerticalLine((int)(left.getRight() + 9.f), r.getY(), r.getBottom());
        g.drawVerticalLine((int)(r.getRight() + 9.f), r.getY(), r.getBottom());
        paintDecay(g, left);
        auto time = r;
        auto level = time.removeFromBottom(time.getHeight() * 0.32f);
        time.removeFromBottom(14.f);
        paintTime(g, time);
        paintLevel(g, level);
        paintMeters(g, meters);
    }

private:
    void paintDecay(juce::Graphics& g, juce::Rectangle<float> r) {
        drawCaps(g, "Decay time", r.removeFromTop(12.f), juce::Justification::centredLeft, 9.5f, kTextFaint);
        r.removeFromTop(8.f);
        auto plot = r; plot.removeFromLeft(28.f); plot.removeFromBottom(14.f);
        const double fmin = 50.0, fmax = 16000.0;
        double tmax = 1.0;
        for (int i = 0; i <= 64; ++i) tmax = std::max(tmax, proc.displayedT60(fmin * std::pow(fmax / fmin, i / 64.0)));
        tmax = std::ceil(tmax * 1.15);
        auto X = [&](double f) { return plot.getX() + plot.getWidth() * (float)(std::log(f / fmin) / std::log(fmax / fmin)); };
        auto Y = [&](double t) { return plot.getBottom() - plot.getHeight() * (float)std::min(1.0, t / tmax); };
        g.setColour(kGrid);
        for (double f : {100., 1000., 10000.}) g.drawVerticalLine((int)X(f), plot.getY(), plot.getBottom());
        for (int i = 1; i < 8; ++i) { const float xx = plot.getX() + plot.getWidth() * i / 8.f; g.drawVerticalLine((int)xx, plot.getBottom() - 4.f, plot.getBottom()); }
        for (int i = 0; i <= 4; ++i) g.drawHorizontalLine((int)Y(tmax * i / 4.0), plot.getX(), plot.getRight());
        for (double f : {100., 1000., 10000.})
            axisLabel(g, f >= 1000 ? juce::String((int)(f / 1000)) + " kHz" : juce::String((int)f) + " Hz", {X(f) - 24.f, plot.getBottom() + 2.f, 48.f, 12.f}, juce::Justification::centred);
        for (int i = 0; i <= 4; i += 2) {
            const double t = tmax * i / 4.0;
            axisLabel(g, juce::String(t, t < 10 ? 1 : 0) + " s", {plot.getX() - 30.f, Y(t) - 6.f, 26.f, 12.f}, juce::Justification::centredRight);
        }
        juce::Path curve, fill;
        for (int i = 0; i <= 160; ++i) {
            const double f = fmin * std::pow(fmax / fmin, i / 160.0);
            const juce::Point<float> p(X(f), Y(proc.displayedT60(f)));
            if (i == 0) { curve.startNewSubPath(p); fill.startNewSubPath(p.x, plot.getBottom()); }
            curve.lineTo(p); fill.lineTo(p);
        }
        fill.lineTo(plot.getRight(), plot.getBottom()); fill.closeSubPath();
        g.setGradientFill(juce::ColourGradient(kCurveFill, 0.f, plot.getY(), kCurveFill.withAlpha(0.f), 0.f, plot.getBottom(), false));
        g.fillPath(fill);
        g.setGradientFill(juce::ColourGradient(kCurve, plot.getX(), 0.f, kDanger.brighter(0.1f), plot.getRight(), 0.f, false));
        g.strokePath(curve, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        for (double f : {125., 1000., 8000.}) {
            const float x = X(f), y = Y(proc.displayedT60(f));
            g.setColour(kDisplayBg); g.fillEllipse(x - 3.5f, y - 3.5f, 7.f, 7.f);
            g.setColour(kCurve); g.drawEllipse(x - 3.f, y - 3.f, 6.f, 6.f, 1.4f);
        }
    }

    void paintTime(juce::Graphics& g, juce::Rectangle<float> r) {
        const float modeV = proc.apvts.getRawParameterValue("mode")->load();
        const bool algo = modeV < 0.5f || modeV > 2.5f;
        drawCaps(g, algo ? "Early reflections" : "Impulse response", r.removeFromTop(12.f), juce::Justification::centredLeft, 9.5f, kTextFaint);
        r.removeFromTop(8.f);
        auto plot = r; plot.removeFromBottom(14.f);
        g.setColour(kGrid); g.drawHorizontalLine((int)plot.getBottom(), plot.getX(), plot.getRight());
        if (algo) {
            const double size = proc.apvts.getRawParameterValue("size")->load(), maxMs = 60.0 * std::max(1.0, size);
            const int taps = proc.apvts.getRawParameterValue("quality")->load() < 0.5f ? 16 : proc.apvts.getRawParameterValue("quality")->load() < 1.5f ? 32 : 64;
            const float lvl = proc.apvts.getRawParameterValue("erLevel")->load();
            for (int i = 0; i < std::min(taps, cs::kErDefaultCount); ++i) {
                const auto& t = cs::kErDefaultTable[i];
                const float x = plot.getX() + plot.getWidth() * (float)(t.delaySec * 1000.0 * size / maxMs);
                const float db = juce::jlimit(-60.f, 0.f, 20.f * std::log10(std::max(t.gain, 1e-4f)) + lvl);
                const float y = plot.getBottom() - plot.getHeight() * (db + 60.f) / 60.f;
                if (x > plot.getRight()) continue;
                g.setColour((t.pan < 0 ? kCurve : kMeter).withAlpha(0.75f));
                g.drawLine(x, plot.getBottom(), x, y, 1.4f);
            }
            for (int ms = 0; ms <= (int)maxMs; ms += (maxMs > 100 ? 40 : 20))
                axisLabel(g, juce::String(ms) + " ms", {plot.getX() + plot.getWidth() * (float)(ms / maxMs) - 20.f, plot.getBottom() + 2.f, 40.f, 12.f}, juce::Justification::centred);
        } else if (auto info = proc.getIrInfo()) {
            juce::Path p, fill;
            const int n = (int)info->preview.size();
            for (int i = 0; i < n; ++i) {
                const float x = plot.getX() + plot.getWidth() * i / (float)std::max(1, n - 1);
                const float y = plot.getBottom() - plot.getHeight() * juce::jlimit(0.f, 1.f, (info->preview[(size_t)i] + 60.f) / 60.f);
                if (i == 0) { p.startNewSubPath(x, y); fill.startNewSubPath(x, plot.getBottom()); }
                p.lineTo(x, y); fill.lineTo(x, y);
            }
            fill.lineTo(plot.getRight(), plot.getBottom()); fill.closeSubPath();
            g.setColour(kCurveFill.withAlpha(0.12f)); g.fillPath(fill);
            g.setColour(kCurve); g.strokePath(p, juce::PathStrokeType(1.2f));
            for (int ms = 0; ms <= 500; ms += 100)
                axisLabel(g, juce::String(ms) + " ms", {plot.getX() + plot.getWidth() * ms / 500.f - 20.f, plot.getBottom() + 2.f, 40.f, 12.f}, juce::Justification::centred);
        } else {
            g.setFont(font(12.f)); g.setColour(kTextFaint); g.drawText("No impulse response loaded", plot, juce::Justification::centred);
        }
    }

    void paintLevel(juce::Graphics& g, juce::Rectangle<float> r) {
        drawCaps(g, "Reverb level", r.removeFromTop(12.f), juce::Justification::centredLeft, 9.5f, kTextFaint);
        r.removeFromTop(6.f);
        const int n = ChromosphereProcessor::kEnvSize, pos = proc.envPos.load(std::memory_order_acquire);
        juce::Path fill;
        fill.startNewSubPath(r.getX(), r.getBottom());
        for (int i = 0; i < n; ++i) {
            const float db = proc.envRing[(size_t)((pos + i) % n)].load(std::memory_order_relaxed);
            fill.lineTo(r.getX() + r.getWidth() * i / (float)(n - 1), r.getBottom() - r.getHeight() * juce::jlimit(0.f, 1.f, (db + 90.f) / 90.f));
        }
        fill.lineTo(r.getRight(), r.getBottom()); fill.closeSubPath();
        g.setColour(kGrid); g.drawHorizontalLine((int)r.getBottom(), r.getX(), r.getRight());
        g.setGradientFill(juce::ColourGradient(kMeter.withAlpha(0.35f), 0.f, r.getY(), kMeter.withAlpha(0.05f), 0.f, r.getBottom(), false));
        g.fillPath(fill);
        g.setColour(kMeter.withAlpha(0.7f)); g.strokePath(fill, juce::PathStrokeType(1.f));
    }

    void paintMeters(juce::Graphics& g, juce::Rectangle<float> r) {
        auto labels = r.removeFromBottom(14.f);
        r.removeFromTop(20.f); r.removeFromBottom(6.f);
        const float cw = r.getWidth() / 3.f;
        const std::atomic<float>* src[3] = {&proc.inPeak, &proc.outPeak, &proc.tailPeak};
        const char* names[3] = {"In", "Out", "Tail"};
        for (int i = 0; i < 3; ++i) {
            auto col = juce::Rectangle<float>(r.getX() + cw * i, r.getY(), cw, r.getHeight()).reduced(cw * 0.34f, 0.f);
            const float db = 20.f * std::log10(std::max(src[i]->load(std::memory_order_relaxed), 1e-5f));
            const float frac = juce::jlimit(0.f, 1.f, (db + 60.f) / 60.f);
            g.setColour(kGrid); g.fillRoundedRectangle(col, 2.f);
            auto fillR = col.withTop(col.getBottom() - col.getHeight() * frac);
            g.setGradientFill(juce::ColourGradient(db > -3.f ? kDanger : kMeterHot, 0.f, col.getY(), kMeter, 0.f, col.getBottom(), false));
            g.fillRoundedRectangle(fillR, 2.f);
            drawCaps(g, names[i], {r.getX() + cw * i, labels.getY(), cw, labels.getHeight()}, juce::Justification::centred, 9.f, kTextFaint);
        }
    }

    ChromosphereProcessor& proc;
};

// ------------------------------------------------------------------------------------------------
class ChromosphereEditor::PresetBar : public juce::Component {
public:
    explicit PresetBar(ChromosphereProcessor& p) : proc(p) {
        for (auto* b : {&prev, &next, &save, &del, &ab, &copy, &undo, &redo}) addAndMakeVisible(b);
        addAndMakeVisible(name);
        prev.setButtonText("<"); next.setButtonText(">"); save.setButtonText("Save"); del.setButtonText("Delete");
        ab.setButtonText("A"); copy.setButtonText("A > B"); undo.setButtonText("Undo"); redo.setButtonText("Redo");
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
    struct NameField : public juce::TextButton {
        NameField() { setTitle("Preset browser"); }
        void paintButton(juce::Graphics& g, bool over, bool down) override {
            const auto r = getLocalBounds().toFloat().reduced(0.5f);
            g.setColour(down ? juce::Colour(0xff0a0b0c) : kDisplayBg); g.fillRoundedRectangle(r, 6.f);
            g.setColour(hasKeyboardFocus(false) ? kAccent.withAlpha(0.8f) : juce::Colours::white.withAlpha(over ? 0.14f : 0.08f)); g.drawRoundedRectangle(r, 6.f, 1.f);
            auto inner = r.reduced(12.f, 0.f);
            const float catW = std::min(110.f, caps(9.5f).getStringWidthFloat(category.toUpperCase()) + 4.f);
            drawCaps(g, category, inner.removeFromLeft(catW), juce::Justification::centredLeft, 9.5f, kTextFaint);
            inner.removeFromRight(catW);     // keep the name centred in the field
            g.setFont(font(14.f, 500)); g.setColour(kText); g.drawText(text, inner, juce::Justification::centred, true);
        }
        juce::String text, category;
    };
    void refresh() {
        const int ci = proc.presets.currentIndex();
        const juce::String n = proc.presets.currentName(), cat = ci >= 0 ? proc.presets.presets()[(size_t)ci].category : juce::String();
        if (name.text != n || name.category != cat) { name.text = n.isEmpty() ? "No preset" : n; name.category = cat; name.repaint(); }
        ab.setButtonText(proc.presets.activeSlot() == 0 ? "A" : "B");
        del.setEnabled(ci >= 0 && !proc.presets.presets()[(size_t)ci].factory);
        undo.setEnabled(proc.undoManager.canUndo()); redo.setEnabled(proc.undoManager.canRedo());
    }
    void resized() override {
        auto r = getLocalBounds();
        const int h = r.getHeight();
        redo.setBounds(r.removeFromRight(56)); r.removeFromRight(6);
        undo.setBounds(r.removeFromRight(56)); r.removeFromRight(14);
        copy.setBounds(r.removeFromRight(56)); r.removeFromRight(6);
        ab.setBounds(r.removeFromRight(h)); r.removeFromRight(14);
        del.setBounds(r.removeFromRight(64)); r.removeFromRight(6);
        save.setBounds(r.removeFromRight(56)); r.removeFromRight(14);
        prev.setBounds(r.removeFromLeft(h)); r.removeFromLeft(6);
        next.setBounds(r.removeFromRight(h)); r.removeFromRight(6);
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
    ChromosphereProcessor& proc;
    juce::TextButton prev, next, save, del, ab, copy, undo, redo;
    NameField name;
};

namespace {
class PresetBrowser : public juce::Component, private juce::ListBoxModel {
public:
    PresetBrowser(ChromosphereProcessor& p, std::function<void()> onPick) : proc(p), picked(std::move(onPick)) {
        search.setTextToShowWhenEmpty("Search name, category or tag", kTextFaint);
        search.setFont(font(13.f));
        search.onTextChange = [this] { rebuild(); };
        search.setTitle("Search presets");
        list.setModel(this); list.setRowHeight(26); list.setTitle("Presets");
        addAndMakeVisible(search); addAndMakeVisible(list);
        rebuild();
        setSize(440, 480);
    }
    void paint(juce::Graphics& g) override { g.fillAll(kPlate); }
    void resized() override { auto r = getLocalBounds().reduced(10); search.setBounds(r.removeFromTop(28)); r.removeFromTop(8); list.setBounds(r); }
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
        if (r.index < 0) { drawCaps(g, r.text, juce::Rectangle<float>(10.f, 0.f, (float)w - 10.f, (float)h), juce::Justification::centredLeft, 9.5f, kTextFaint); return; }
        if (sel || r.index == proc.presets.currentIndex()) { g.setColour(juce::Colours::white.withAlpha(sel ? 0.08f : 0.04f)); g.fillRoundedRectangle(juce::Rectangle<float>(4.f, 1.f, (float)w - 8.f, (float)h - 2.f), 4.f); }
        g.setColour(r.index == proc.presets.currentIndex() ? kAccent : kText); g.setFont(font(13.5f));
        g.drawText(r.text + (proc.presets.presets()[(size_t)r.index].factory ? "" : "  (user)"), 20, 0, w - 20, h, juce::Justification::centredLeft);
    }
    void listBoxItemClicked(int row, const juce::MouseEvent&) override { choose(row); }
    void returnKeyPressed(int row) override { choose(row); }
    void choose(int row) {
        if (row < 0 || row >= (int)rows.size() || rows[(size_t)row].index < 0) return;
        proc.presets.apply(rows[(size_t)row].index);
        picked();
    }
    struct Row { int index; juce::String text; };
    ChromosphereProcessor& proc;
    std::function<void()> picked;
    juce::TextEditor search;
    juce::ListBox list;
    std::vector<Row> rows;
};
}  // namespace

namespace {
class UpdatePanel : public juce::Component {
public:
    explicit UpdatePanel(Updater& u) : up(u) {
        optIn.setButtonText("Check for updates when the plug-in opens");
        optIn.setToggleState(up.enabled(), juce::dontSendNotification);
        optIn.onClick = [this] { up.setEnabled(optIn.getToggleState()); };
        check.setButtonText("Check now"); download.setButtonText("Download"); open.setButtonText("Run installer"); skip.setButtonText("Skip this version");
        check.onClick = [this] { up.checkNow(); };
        download.onClick = [this] { up.download(); };
        open.onClick = [this] { up.openDownloaded(); };
        skip.onClick = [this] { up.skipVersion(up.info().version); };
        for (auto* c : std::initializer_list<juce::Component*>{&optIn, &check, &download, &open, &skip}) addAndMakeVisible(c);
        up.onChange = [this] { refresh(); };
        refresh();
        setSize(420, 190);
    }
    ~UpdatePanel() override { up.onChange = nullptr; }
    void refresh() {
        const auto s = up.state();
        download.setEnabled(s == Updater::State::Available && up.info().url.isNotEmpty());
        open.setEnabled(s == Updater::State::Downloaded);
        skip.setEnabled(s == Updater::State::Available);
        check.setEnabled(s != Updater::State::Checking && s != Updater::State::Downloading);
        repaint();
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(kPlate);
        auto r = getLocalBounds().reduced(16, 12);
        g.setFont(font(15.f, 600)); g.setColour(kText); g.drawText("Updates", r.removeFromTop(22), juce::Justification::centredLeft);
        g.setFont(font(12.f)); g.setColour(kTextDim);
        g.drawText("Installed: version " + Updater::currentVersion() + ". Off by default; the check only reads one small file from the release page.", r.removeFromTop(18), juce::Justification::centredLeft);
        r.removeFromTop(34);
        juce::String msg = up.message();
        if (up.state() == Updater::State::Downloading) msg += "  " + juce::String((int)(up.progress() * 100)) + "%";
        g.setColour(up.state() == Updater::State::Failed ? kDanger : up.state() == Updater::State::Available ? kAccent : kTextDim);
        g.drawFittedText(msg, r.removeFromTop(36), juce::Justification::topLeft, 2);
    }
    void resized() override {
        auto r = getLocalBounds().reduced(16, 12);
        r.removeFromTop(44);
        optIn.setBounds(r.removeFromTop(28));
        r.removeFromTop(44);
        auto row = r.removeFromTop(28);
        check.setBounds(row.removeFromLeft(90)); row.removeFromLeft(6);
        download.setBounds(row.removeFromLeft(90)); row.removeFromLeft(6);
        open.setBounds(row.removeFromLeft(100)); row.removeFromLeft(6);
        skip.setBounds(row);
    }
    Updater& up;
    juce::ToggleButton optIn;
    juce::TextButton check, download, open, skip;
};
}  // namespace

void ChromosphereEditor::PresetBar::openBrowser() {
    auto content = std::make_unique<PresetBrowser>(proc, [this] { refresh(); });
    juce::CallOutBox::launchAsynchronously(std::move(content), name.getScreenBounds(), nullptr);
}

// ------------------------------------------------------------------------------------------------
ChromosphereEditor::ChromosphereEditor(ChromosphereProcessor& p) : AudioProcessorEditor(&p), proc_(p) {
    setLookAndFeel(&look_);
    auto& a = proc_.apvts;
    auto addKnob = [&](const char* id, const char* title, const char* suffix = "", int dec = 2) {
        knobs_.push_back(std::make_unique<Knob>(a, id, title, suffix, dec)); addAndMakeVisible(*knobs_.back());
    };
    // Order matters for resized(): primary (4), secondary (5), advanced (the rest).
    addKnob("size", "Size", "", 2); addKnob("decayMid", "Decay", "s", 2); addKnob("preDelay", "Pre-delay", "ms", 1); addKnob("mix", "Mix", "%", 0);
    addKnob("decayLow", "Low decay", "s", 2); addKnob("decayHigh", "High decay", "s", 2); addKnob("erLevel", "Early / IR", "dB", 1);
    addKnob("tailLevel", "Tail", "dB", 1); addKnob("outGain", "Output", "dB", 1);
    addKnob("modDepth", "Mod depth", "", 2); addKnob("modRate", "Mod rate", "Hz", 2);
    addKnob("erMs", "Hybrid ER", "", 0); addKnob("tailScale", "Hybrid tail", "x", 2);
    addKnob("irStart", "IR start", "ms", 0); addKnob("irLength", "IR length", "", 0); addKnob("irStretch", "IR stretch", "x", 2);
    addKnob("irLowCut", "IR low cut", "", 0); addKnob("irHighCut", "IR high cut", "", 0); addKnob("irWidth", "IR width", "", 2);
    addKnob("irDecay", "IR decay", "x", 2);
    addKnob("cosmicA", "A", "", 2); addKnob("cosmicB", "B", "", 2); addKnob("cosmicC", "C", "", 2);

    display_ = std::make_unique<MainDisplay>(proc_);
    presetBar_ = std::make_unique<PresetBar>(proc_);
    addAndMakeVisible(*display_); addAndMakeVisible(*presetBar_);
    updates_.setButtonText("Updates"); updates_.setTitle("Updates: opt-in check for new versions");
    updates_.onClick = [this] {
        auto content = std::make_unique<UpdatePanel>(proc_.updater);
        juce::CallOutBox::launchAsynchronously(std::move(content), updates_.getScreenBounds(), nullptr);
    };
    addAndMakeVisible(updates_);
    proc_.updater.checkIfDue();
    proc_.updater.onChange = [this] { updateBadge_ = proc_.updater.state() == Updater::State::Available; updates_.setButtonText(updateBadge_ ? "Update available" : "Updates"); resized(); };

    mode_.addItemList({"Algorithmic", "Convolution", "Hybrid", "Shimmer", "Redshift", "Corona", "Wormhole", "Event Horizon", "Nebula", "Pulsar", "Solar Wind", "Aurora", "Supernova"}, 1); quality_.addItemList({"Eco", "Normal", "High"}, 1);
    mode_.setTitle("Mode"); quality_.setTitle("Quality"); factoryIr_.setTitle("Factory impulse response");
    modeAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(a, "mode", mode_);
    qualityAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(a, "quality", quality_);
    freezeAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(a, "freeze", freeze_);
    diffusionAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(a, "diffusion", diffusion_);
    reverseAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(a, "irReverse", reverse_);
    factoryIr_.addItem("Factory IR", 1);
    {
        int id = 2;
        for (auto& n : ChromosphereProcessor::factoryImpulseResponses()) factoryIr_.addItem(juce::String(n.replaceCharacter('_', ' ')).substring(0, 1).toUpperCase() + n.replaceCharacter('_', ' ').substring(1), id++);
    }
    factoryIr_.setSelectedId(1, juce::dontSendNotification);
    factoryIr_.onChange = [this] {
        const int idx = factoryIr_.getSelectedId() - 2;
        const auto names = ChromosphereProcessor::factoryImpulseResponses();
        if (idx >= 0 && idx < names.size()) proc_.loadFactoryImpulseResponse(names[idx]);
        factoryIr_.setSelectedId(1, juce::dontSendNotification);
    };
    loadIr_.setTitle("Load an impulse response file");
    loadIr_.onClick = [this] { chooseIr(); };
    irName_.setColour(juce::Label::textColourId, kTextFaint); irName_.setJustificationType(juce::Justification::centredLeft);
    irName_.setFont(font(12.f));
    freeze_.setTitle("Freeze: hold the reverb tail and mute the input");
    advanced_.onClick = [this] { setAdvancedVisible(advanced_.getToggleState()); };
    for (auto* c : std::initializer_list<juce::Component*>{&mode_, &quality_, &factoryIr_, &freeze_, &diffusion_, &reverse_, &advanced_, &loadIr_, &irName_})
        addAndMakeVisible(c);

    setResizable(true, true);
    setResizeLimits(900, 600, 1900, 1300);
    setSize(1080, 700);
    updateEnabledStates();
    startTimerHz(30);
}

ChromosphereEditor::~ChromosphereEditor() { stopTimer(); proc_.updater.onChange = nullptr; setLookAndFeel(nullptr); }

void ChromosphereEditor::setAdvancedVisible(bool v) { advancedVisible_ = v; advanced_.setToggleState(v, juce::dontSendNotification); resized(); repaint(); }

void ChromosphereEditor::chooseIr() {
    chooser_ = std::make_unique<juce::FileChooser>("Choose an impulse response", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
    chooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) {
        const auto f = fc.getResult();
        if (f.existsAsFile()) {
            const auto err = proc_.loadImpulseResponse(f);
            if (err.isNotEmpty()) juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Impulse response", err);
        }
    });
}

void ChromosphereEditor::updateEnabledStates() {
    const int mode = (int)std::lround(proc_.apvts.getRawParameterValue("mode")->load());
    const bool algo = mode == 0, conv = mode == 1, hybrid = mode == 2, cosmic = mode >= 3, ir = conv || hybrid;
    auto setOn = [&](const char* id, bool on) { for (auto& k : knobs_) if (k->paramId == id) { k->slider.setEnabled(on); k->repaint(); } };
    for (auto* id : {"decayLow", "decayMid", "decayHigh"}) setOn(id, algo || cosmic);
    for (auto* id : {"modDepth", "modRate"}) setOn(id, !conv);
    setOn("tailLevel", !conv); setOn("erMs", hybrid); setOn("tailScale", hybrid);
    for (auto* id : {"irStart", "irLength", "irStretch", "irLowCut", "irHighCut", "irWidth", "irDecay"}) setOn(id, ir);
    for (auto* id : {"cosmicA", "cosmicB", "cosmicC"}) setOn(id, cosmic);
    if (cosmic) {
        const auto names = cs::recipeMacroNames((cs::Recipe)(mode - 3));
        const char* ids[3] = {"cosmicA", "cosmicB", "cosmicC"};
        for (int i = 0; i < 3; ++i) for (auto& k : knobs_) if (k->paramId == ids[i]) k->setTitleText(names[(size_t)i]);
    }
    reverse_.setEnabled(ir); loadIr_.setEnabled(ir); factoryIr_.setEnabled(ir);
    diffusion_.setEnabled(!conv); quality_.setEnabled(!conv);
    const bool wasCosmic = lastMode_ >= 3;
    lastMode_ = mode;
    if (wasCosmic != cosmic) resized();
}

void ChromosphereEditor::timerCallback() {
    const int mode = (int)std::lround(proc_.apvts.getRawParameterValue("mode")->load());
    if (mode != lastMode_) updateEnabledStates();
    const auto path = proc_.getImpulseResponsePath();
    const auto shown = path.isEmpty() ? juce::String((mode == 1 || mode == 2) ? "No impulse response loaded" : "")
                                      : path.startsWith("factory:") ? path.fromFirstOccurrenceOf("factory:", false, false).replaceCharacter('_', ' ') + "  (factory)"
                                                                    : juce::File(path).getFileName();
    if (irName_.getText() != shown) irName_.setText(shown, juce::dontSendNotification);
    presetBar_->refresh();
    display_->repaint();
}

void ChromosphereEditor::paint(juce::Graphics& g) {
    g.drawImageAt(backgroundTexture(getWidth(), getHeight()), 0, 0);
    drawPlate(g, plate_.toFloat());
    if (advancedVisible_) drawPlate(g, advancedPlate_.toFloat());
    // Brand and system status.
    auto b = brandArea_.toFloat();
    auto brandRow = b.removeFromTop(b.getHeight() * 0.55f);
    g.setFont(font(17.f, 600)); g.setColour(kText); g.drawText("CHROMO", brandRow.removeFromLeft(74.f), juce::Justification::centredLeft);
    g.setFont(font(17.f, 400)); g.setColour(kTextDim); g.drawText("SPHERE", brandRow, juce::Justification::centredLeft);
    const int modeIdx = (int)std::lround(proc_.apvts.getRawParameterValue("mode")->load());
    const juce::String engine = modeIdx >= 3 ? juce::String(cs::recipeName((cs::Recipe)(modeIdx - 3))) : modeIdx == 1 ? "Convolution" : modeIdx == 2 ? "Hybrid" : "Algorithmic";
    drawCaps(g, "ENGINE " + engine + "  ·  " + juce::String(proc_.getSampleRate() / 1000.0, 1) + " kHz  ·  v" + Updater::currentVersion(), b, juce::Justification::centredLeft, 8.5f, kTextFaint);
    // Section headers inside the bays.
    drawSection(g, sectionMain_.toFloat(), "01", "Reverb");
    drawSection(g, sectionCosmic_.toFloat(), "02", cosmicNow_ ? "Cosmic" : "Levels");
    if (advancedVisible_) drawSection(g, sectionAdv_.toFloat(), "03", "Advanced");
    drawCaps(g, "Mode", modeLabel_.toFloat(), juce::Justification::centredLeft, 9.5f, kTextFaint);
    drawCaps(g, "Quality", qualityLabel_.toFloat(), juce::Justification::centredLeft, 9.5f, kTextFaint);
}

void ChromosphereEditor::resized() {
    if (!presetBar_ || !display_) return;       // called by the base class before the children exist
    const int m = 22;
    auto r = getLocalBounds().reduced(m, 0);
    auto header = r.removeFromTop(68).withTrimmedTop(18).withTrimmedBottom(16);
    brandArea_ = header.removeFromLeft(196);
    header.removeFromLeft(18);
    updates_.setBounds(header.removeFromRight(updateBadge_ ? 130 : 84)); header.removeFromRight(14);
    presetBar_->setBounds(header);
    r.removeFromBottom(m);
    const int advH = advancedVisible_ ? std::clamp(r.getHeight() / 4, 150, 230) : 0;
    const bool cosmicMode = (int)std::lround(proc_.apvts.getRawParameterValue("mode")->load()) >= 3;
    auto displayArea = r.removeFromTop(std::clamp((int)(r.getHeight() * (advancedVisible_ ? 0.24f : cosmicMode ? 0.26f : 0.30f)), 140, 300));
    display_->setBounds(displayArea);
    r.removeFromTop(16);
    if (advancedVisible_) { advancedPlate_ = r.removeFromBottom(advH); r.removeFromBottom(14); }
    plate_ = r;
    auto in = plate_.reduced(24, 18);
    cosmicNow_ = (int)std::lround(proc_.apvts.getRawParameterValue("mode")->load()) >= 3;
    // Selector row.
    auto top = in.removeFromTop(44);
    {
        auto labelRow = top.removeFromTop(14);
        const int comboW = std::clamp(in.getWidth() / 6, 130, 180), gap = 14;
        modeLabel_ = labelRow.removeFromLeft(comboW); labelRow.removeFromLeft(gap);
        qualityLabel_ = labelRow.removeFromLeft(comboW - 40);
        mode_.setBounds(top.removeFromLeft(comboW).withHeight(28)); top.removeFromLeft(gap);
        quality_.setBounds(top.removeFromLeft(comboW - 40).withHeight(28)); top.removeFromLeft(gap * 2);
        diffusion_.setBounds(top.removeFromRight(132).withHeight(28)); top.removeFromRight(gap);
        freeze_.setBounds(top.removeFromRight(112).withHeight(28));
    }
    in.removeFromTop(4);
    sectionMain_ = in.removeFromTop(14); in.removeFromTop(2);
    auto footer = in.removeFromBottom(30);
    in.removeFromBottom(8);
    auto layoutRow = [&](juce::Rectangle<int> row, int first, int count) {
        const int w = row.getWidth() / count;
        for (int i = 0; i < count; ++i) knobs_[(size_t)(first + i)]->setBounds(row.removeFromLeft(w).reduced(2));
    };
    const int modeNow = (int)std::lround(proc_.apvts.getRawParameterValue("mode")->load());
    const bool cosmic = modeNow >= 3;
    const int rowsAvail = in.getHeight();
    const size_t cosmicFirst = knobs_.size() - 3;
    if (cosmic) {
        layoutRow(in.removeFromTop((int)(rowsAvail * 0.40f)), 0, 4);
        auto second = in.removeFromTop((int)(rowsAvail * 0.25f));
        layoutRow(second, 4, 5);
        sectionCosmic_ = in.removeFromTop(14); in.removeFromTop(2);
        // Cosmic macro dials: three, centred, larger than the second row.
        auto third = in;
        const int cw = std::min(third.getWidth() / 3, 260);
        third = third.withSizeKeepingCentre(cw * 3, third.getHeight());
        for (int i = 0; i < 3; ++i) knobs_[cosmicFirst + (size_t)i]->setBounds(third.removeFromLeft(cw).reduced(2));
    } else {
        layoutRow(in.removeFromTop((int)(rowsAvail * 0.56f)), 0, 4);
        sectionCosmic_ = in.removeFromTop(14); in.removeFromTop(2);
        layoutRow(in, 4, 5);
    }
    for (int i = 0; i < 3; ++i) knobs_[cosmicFirst + (size_t)i]->setVisible(cosmic);
    advanced_.setBounds(footer.removeFromLeft(126)); footer.removeFromLeft(14);
    loadIr_.setBounds(footer.removeFromLeft(80)); footer.removeFromLeft(8);
    factoryIr_.setBounds(footer.removeFromLeft(std::min(150, footer.getWidth() / 3)).withHeight(28)); footer.removeFromLeft(10);
    reverse_.setBounds(footer.removeFromRight(std::min(126, footer.getWidth() / 2)));
    irName_.setBounds(footer);
    const bool show = advancedVisible_;
    for (size_t i = 9; i < cosmicFirst; ++i) knobs_[i]->setVisible(show);
    if (show) {
        auto adv = advancedPlate_.reduced(24, 14);
        sectionAdv_ = adv.removeFromTop(14); adv.removeFromTop(2);
        const int perRow = 6, n = (int)cosmicFirst - 9, rowsN = (n + perRow - 1) / perRow;
        const int rh = adv.getHeight() / rowsN;
        for (int row = 0; row < rowsN; ++row) {
            auto line = adv.removeFromTop(rh);
            const int cnt = std::min(perRow, n - row * perRow), w = line.getWidth() / perRow;
            for (int i = 0; i < cnt; ++i) knobs_[(size_t)(9 + row * perRow + i)]->setBounds(line.removeFromLeft(w).reduced(2));
        }
    }
}
