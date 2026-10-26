#include "PluginEditor.h"
#include "fdn/decay_design.h"
#include "er/er_default_table.h"
#include <cmath>

using namespace retro;

// ------------------------------------------------------------------------------------------------
class FancyReverbEditor::Knob : public juce::Component {
public:
    Knob(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& title, const juce::String& suffix = {}, int decimals = 2)
        : paramId(id), title_(title) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
        slider.setTextValueSuffix(suffix.isEmpty() ? juce::String() : " " + suffix);
        slider.setNumDecimalPlacesToDisplay(decimals);
        slider.setTitle(title); slider.setDescription(title);
        slider.setDoubleClickReturnValue(true, apvts.getParameter(id)->convertFrom0to1(apvts.getParameter(id)->getDefaultValue()));
        addAndMakeVisible(slider);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, id, slider);
        setWantsKeyboardFocus(false);
    }
    void paint(juce::Graphics& g) override {
        drawSilk(g, title_.toUpperCase(), labelArea_.toFloat(), juce::Justification::centred, std::clamp(std::min(getHeight() / 9.f, getWidth() / 9.f), 8.f, 12.f),
                 slider.isEnabled() ? kSilk : kSilkDim);
    }
    void resized() override {
        const int nameH = std::clamp(getHeight() / 7, 14, 18), textH = std::clamp(getHeight() / 6, 20, 24);
        const int side = std::max(24, std::min(getWidth() - 4, getHeight() - nameH - textH - 6));
        const int groupH = nameH + side + textH, top = std::max(0, (getHeight() - groupH) / 2);
        labelArea_ = {0, top, getWidth(), nameH};
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, std::clamp(getWidth() - 6, 56, 92), textH);
        slider.setBounds((getWidth() - side) / 2 - 12, top + nameH, side + 24, side + textH + 2);
    }
    juce::String paramId;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
private:
    juce::String title_;
    juce::Rectangle<int> labelArea_;
};

// ------------------------------------------------------------------------------------------------
namespace {
void gridLabel(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, juce::Justification j) {
    g.setFont(juce::FontOptions(9.5f)); g.setColour(kVfd.withAlpha(0.55f)); g.drawText(s, r, j);
}
void gridLine(juce::Graphics& g, float x0, float y0, float x1, float y1) {
    g.setColour(kVfd.withAlpha(0.14f));
    const float dash[] = {2.f, 3.f};
    g.drawDashedLine({x0, y0, x1, y1}, dash, 2, 1.f);
}
}  // namespace

class FancyReverbEditor::DecayView : public juce::Component {
public:
    explicit DecayView(FancyReverbProcessor& p) : proc(p) { setTitle("Decay time against frequency"); setDescription("Reverb time in seconds for each frequency"); }
    void paint(juce::Graphics& g) override {
        auto r = drawVfdGlass(g, getLocalBounds().toFloat());
        r.removeFromLeft(26.f); r.removeFromBottom(12.f); r.removeFromTop(2.f);
        const double fmin = 63.0, fmax = 16000.0;
        double tmax = 1.0;
        for (int i = 0; i <= 64; ++i) tmax = std::max(tmax, proc.displayedT60(fmin * std::pow(fmax / fmin, i / 64.0)));
        tmax = std::ceil(tmax * 1.1);
        auto X = [&](double f) { return r.getX() + r.getWidth() * (float)(std::log(f / fmin) / std::log(fmax / fmin)); };
        auto Y = [&](double t) { return r.getBottom() - r.getHeight() * (float)std::min(1.0, t / tmax); };
        for (double f : {125., 250., 500., 1000., 2000., 4000., 8000.}) {
            gridLine(g, X(f), r.getY(), X(f), r.getBottom());
            gridLabel(g, f >= 1000 ? juce::String((int)(f / 1000)) + "k" : juce::String((int)f), {X(f) - 16.f, r.getBottom() + 1.f, 32.f, 11.f}, juce::Justification::centred);
        }
        for (int i = 0; i <= 4; ++i) {
            const double t = tmax * i / 4.0;
            gridLine(g, r.getX(), Y(t), r.getRight(), Y(t));
            gridLabel(g, juce::String(t, t < 10 ? 1 : 0) + "s", {r.getX() - 27.f, Y(t) - 6.f, 25.f, 12.f}, juce::Justification::centredRight);
        }
        juce::Path path;
        for (int i = 0; i <= 160; ++i) {
            const double f = fmin * std::pow(fmax / fmin, i / 160.0);
            const juce::Point<float> p(X(f), Y(proc.displayedT60(f)));
            if (i == 0) path.startNewSubPath(p); else path.lineTo(p);
        }
        glowPath(g, path, kVfd, 2.f);
        for (double f : {125., 1000., 8000.}) glowDot(g, X(f), Y(proc.displayedT60(f)), 2.6f, kVfd);
        drawSegText(g, "T60", {r.getRight() - 46.f, r.getY() + 2.f, 44.f, 14.f}, juce::Justification::right, kVfd.withAlpha(0.8f), false);
    }
    FancyReverbProcessor& proc;
};

class FancyReverbEditor::EnvelopeView : public juce::Component {
public:
    explicit EnvelopeView(FancyReverbProcessor& p) : proc(p) { setTitle("Reverb level over time"); setDescription("Level of the reverb output in the last few seconds"); }
    void paint(juce::Graphics& g) override {
        auto r = drawVfdGlass(g, getLocalBounds().toFloat());
        r.removeFromLeft(26.f); r.removeFromTop(2.f); r.removeFromBottom(2.f);
        for (float db : {0.f, -30.f, -60.f, -90.f}) {
            const float y = r.getBottom() - r.getHeight() * (db + 90.f) / 90.f;
            gridLine(g, r.getX(), y, r.getRight(), y);
            gridLabel(g, juce::String((int)db), {r.getX() - 27.f, y - 6.f, 25.f, 12.f}, juce::Justification::centredRight);
        }
        // Segmented bar-graph history: one column per group of ring entries, 16 segments tall.
        const int n = FancyReverbProcessor::kEnvSize, pos = proc.envPos.load(std::memory_order_acquire);
        const int cols = std::clamp((int)(r.getWidth() / 7.f), 16, 96), per = std::max(1, n / cols), segs = 16;
        const float cw = r.getWidth() / cols, segH = r.getHeight() / segs;
        for (int c = 0; c < cols; ++c) {
            float mx = -120.f;
            for (int k = 0; k < per; ++k) mx = std::max(mx, proc.envRing[(size_t)((pos + c * per + k) % n)].load(std::memory_order_relaxed));
            const int lit = (int)std::round(juce::jlimit(0.f, 1.f, (mx + 90.f) / 90.f) * segs);
            for (int s = 0; s < segs; ++s) {
                juce::Rectangle<float> seg(r.getX() + c * cw + 1.f, r.getBottom() - (s + 1) * segH + 1.f, cw - 2.f, segH - 2.f);
                const juce::Colour col = s >= segs - 1 ? kRed : s >= segs - 3 ? kAmber : kVfd;
                if (s < lit) { g.setColour(col.withAlpha(0.2f)); g.fillRect(seg.expanded(1.5f)); g.setColour(col); }
                else g.setColour(col.withAlpha(0.07f));
                g.fillRect(seg);
            }
        }
        drawSegText(g, "LEVEL", {r.getRight() - 60.f, r.getY() + 2.f, 58.f, 14.f}, juce::Justification::right, kVfd.withAlpha(0.8f), false);
    }
    FancyReverbProcessor& proc;
};

class FancyReverbEditor::ErView : public juce::Component {
public:
    explicit ErView(FancyReverbProcessor& p) : proc(p) { setTitle("Early reflections"); setDescription("Early reflection taps, or the start of the impulse response"); }
    void paint(juce::Graphics& g) override {
        const bool algo = proc.apvts.getRawParameterValue("mode")->load() < 0.5f;
        auto r = drawVfdGlass(g, getLocalBounds().toFloat());
        r.removeFromLeft(6.f); r.removeFromTop(4.f); r.removeFromBottom(12.f);
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
                const juce::Colour c = t.pan < 0 ? kVfd : kAmber;
                juce::Path stem; stem.startNewSubPath(x, r.getBottom()); stem.lineTo(x, y);
                g.setColour(c.withAlpha(0.18f)); g.strokePath(stem, juce::PathStrokeType(3.f));
                g.setColour(c.withAlpha(0.8f)); g.strokePath(stem, juce::PathStrokeType(1.2f));
                glowDot(g, x, y, 1.8f, c);
            }
            for (int ms = 0; ms <= (int)maxMs; ms += (maxMs > 100 ? 40 : 20))
                gridLabel(g, juce::String(ms) + "ms", {r.getX() + r.getWidth() * (float)(ms / maxMs) - 20.f, r.getBottom() + 1.f, 40.f, 11.f}, juce::Justification::centred);
            drawSegText(g, "ER L/R", {r.getRight() - 70.f, r.getY(), 68.f, 14.f}, juce::Justification::right, kVfd.withAlpha(0.8f), false);
        } else if (auto info = proc.getIrInfo()) {
            juce::Path p;
            const int n = (int)info->preview.size();
            for (int i = 0; i < n; ++i) {
                const float x = r.getX() + r.getWidth() * i / (float)std::max(1, n - 1);
                const float y = r.getBottom() - r.getHeight() * juce::jlimit(0.f, 1.f, (info->preview[(size_t)i] + 60.f) / 60.f);
                if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
            }
            glowPath(g, p, kVfd, 1.6f);
            for (int ms = 0; ms <= 500; ms += 100)
                gridLabel(g, juce::String(ms) + "ms", {r.getX() + r.getWidth() * ms / 500.f - 20.f, r.getBottom() + 1.f, 40.f, 11.f}, juce::Justification::centred);
            drawSegText(g, "IR " + juce::String(info->seconds, 1) + "S", {r.getRight() - 90.f, r.getY(), 88.f, 14.f}, juce::Justification::right, kVfd.withAlpha(0.8f), false);
        } else {
            drawSegText(g, "NO IR LOADED", r.reduced(10.f), juce::Justification::centred, kAmber, true, 22.f);
        }
    }
    FancyReverbProcessor& proc;
};

class FancyReverbEditor::MeterPanel : public juce::Component {
public:
    explicit MeterPanel(FancyReverbProcessor& p) : proc(p) { setTitle("Level meters"); }
    void paint(juce::Graphics& g) override {
        auto r = drawVfdGlass(g, getLocalBounds().toFloat());
        auto labels = r.removeFromBottom(14.f);
        const float cw = r.getWidth() / 3.f;
        const std::atomic<float>* src[3] = {&proc.inPeak, &proc.outPeak, &proc.tailPeak};
        const char* names[3] = {"IN", "OUT", "TAIL"};
        for (int i = 0; i < 3; ++i) {
            auto col = juce::Rectangle<float>(r.getX() + cw * i, r.getY(), cw, r.getHeight()).reduced(cw * 0.3f, 4.f);
            const float db = 20.f * std::log10(std::max(src[i]->load(std::memory_order_relaxed), 1e-5f));
            drawVfdBar(g, col, (db + 60.f) / 60.f, -1.f, 26);
            gridLabel(g, names[i], {r.getX() + cw * i, labels.getY(), cw, labels.getHeight()}, juce::Justification::centred);
        }
    }
    FancyReverbProcessor& proc;
};

// Preset bar: VFD name strip with a searchable browser, metal buttons for step/save/delete/A-B/undo.
class FancyReverbEditor::PresetBar : public juce::Component {
public:
    explicit PresetBar(FancyReverbProcessor& p) : proc(p) {
        for (auto* b : {&prev, &next, &save, &del, &ab, &copy, &undo, &redo}) addAndMakeVisible(b);
        addAndMakeVisible(name);
        prev.setButtonText("<"); next.setButtonText(">"); save.setButtonText("Save"); del.setButtonText("Del");
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
    // The name strip: a VFD window drawn here, with an invisible button on top for the click.
    struct NameStrip : public juce::TextButton {
        NameStrip() { setTitle("Preset browser"); }
        void paintButton(juce::Graphics& g, bool over, bool down) override {
            auto glass = drawVfdGlass(g, getLocalBounds().toFloat(), 4.f);
            if (over || down) { g.setColour(kVfd.withAlpha(0.06f)); g.fillRect(glass); }
            drawSegText(g, text, glass.reduced(6.f, 3.f), juce::Justification::centred, kVfd, true, 22.f);
            if (hasKeyboardFocus(false)) { g.setColour(kVfd); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.f), 4.f, 1.5f); }
        }
        juce::String text;
    };
    void refresh() {
        auto n = proc.presets.currentName();
        const juce::String shown = n.isEmpty() ? juce::String("-- NO PRESET --") : n.toUpperCase();
        if (name.text != shown) { name.text = shown; name.repaint(); }
        ab.setButtonText(proc.presets.activeSlot() == 0 ? "A" : "B");
        const int ci = proc.presets.currentIndex();
        del.setEnabled(ci >= 0 && !proc.presets.presets()[(size_t)ci].factory);
        undo.setEnabled(proc.undoManager.canUndo()); redo.setEnabled(proc.undoManager.canRedo());
    }
    void resized() override {
        auto r = getLocalBounds();
        const int h = r.getHeight(), small = std::max(40, h + 12);
        prev.setBounds(r.removeFromLeft(h)); r.removeFromLeft(4);
        next.setBounds(r.removeFromRight(h)); r.removeFromRight(10);
        redo.setBounds(r.removeFromRight(small + 6)); r.removeFromRight(4);
        undo.setBounds(r.removeFromRight(small + 6)); r.removeFromRight(10);
        copy.setBounds(r.removeFromRight(small)); r.removeFromRight(4);
        ab.setBounds(r.removeFromRight(h + 4)); r.removeFromRight(10);
        del.setBounds(r.removeFromRight(small)); r.removeFromRight(4);
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
    juce::TextButton prev, next, save, del, ab, copy, undo, redo;
    NameStrip name;
};

namespace {
class PresetBrowser : public juce::Component, private juce::ListBoxModel {
public:
    PresetBrowser(FancyReverbProcessor& p, std::function<void()> onPick) : proc(p), picked(std::move(onPick)) {
        search.setTextToShowWhenEmpty("Search name, category or tag", kVfdDim.brighter(0.5f));
        search.onTextChange = [this] { rebuild(); };
        search.setTitle("Search presets");
        list.setModel(this); list.setRowHeight(24); list.setTitle("Presets");
        addAndMakeVisible(search); addAndMakeVisible(list);
        rebuild();
        setSize(440, 470);
    }
    void paint(juce::Graphics& g) override { g.fillAll(kPanel); }
    void resized() override { auto r = getLocalBounds().reduced(8); search.setBounds(r.removeFromTop(26)); r.removeFromTop(6); list.setBounds(r); }
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
        if (r.index < 0) { g.setColour(kAmber.withAlpha(0.85f)); g.setFont(juce::FontOptions(11.f, juce::Font::bold)); g.drawText(r.text.toUpperCase(), 8, 0, w - 8, h, juce::Justification::centredLeft); return; }
        if (sel || r.index == proc.presets.currentIndex()) { g.setColour(kVfd.withAlpha(sel ? 0.25f : 0.12f)); g.fillRect(0, 0, w, h); }
        g.setColour(kVfd); g.setFont(juce::FontOptions(14.f));
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
    setLookAndFeel(&look_);
    auto& a = proc_.apvts;
    auto addKnob = [&](const char* id, const char* title, const char* suffix = "", int dec = 2) {
        knobs_.push_back(std::make_unique<Knob>(a, id, title, suffix, dec)); addAndMakeVisible(*knobs_.back());
    };
    // Order matters for resized(): primary (4), secondary (5), advanced (the rest).
    addKnob("size", "Size", "", 2); addKnob("decayMid", "Decay", "s", 2); addKnob("preDelay", "Pre-delay", "ms", 1); addKnob("mix", "Mix", "%", 0);
    addKnob("decayLow", "Decay lo", "s", 2); addKnob("decayHigh", "Decay hi", "s", 2); addKnob("erLevel", "ER / IR", "dB", 1);
    addKnob("tailLevel", "Tail", "dB", 1); addKnob("outGain", "Output", "dB", 1);
    addKnob("modDepth", "Mod depth", "", 2); addKnob("modRate", "Mod rate", "Hz", 2);
    addKnob("erMs", "Hybrid ER", "", 0); addKnob("tailScale", "Hybrid tail", "x", 2);
    addKnob("irStart", "IR start", "ms", 0); addKnob("irLength", "IR length", "", 0); addKnob("irStretch", "IR stretch", "x", 2);
    addKnob("irLowCut", "IR low cut", "", 0); addKnob("irHighCut", "IR high cut", "", 0); addKnob("irWidth", "IR width", "", 2);
    addKnob("irDecay", "IR decay", "x", 2);

    decay_ = std::make_unique<DecayView>(proc_); env_ = std::make_unique<EnvelopeView>(proc_); er_ = std::make_unique<ErView>(proc_);
    meters_ = std::make_unique<MeterPanel>(proc_);
    presetBar_ = std::make_unique<PresetBar>(proc_);
    for (auto* c : std::initializer_list<juce::Component*>{decay_.get(), env_.get(), er_.get(), meters_.get(), presetBar_.get()}) addAndMakeVisible(c);

    mode_.addItemList({"Algorithmic", "Convolution", "Hybrid"}, 1); quality_.addItemList({"Eco", "Normal", "High"}, 1);
    mode_.setTitle("Mode"); quality_.setTitle("Quality"); factoryIr_.setTitle("Factory impulse response");
    modeAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(a, "mode", mode_);
    qualityAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(a, "quality", quality_);
    freeze_.setName("freeze");
    freezeAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(a, "freeze", freeze_);
    diffusionAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(a, "diffusion", diffusion_);
    reverseAtt_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(a, "irReverse", reverse_);
    factoryIr_.addItem("IR BANK", 1);
    {
        int id = 2;
        for (auto& n : FancyReverbProcessor::factoryImpulseResponses()) factoryIr_.addItem(n.replaceCharacter('_', ' ').toUpperCase(), id++);
    }
    factoryIr_.setSelectedId(1, juce::dontSendNotification);
    factoryIr_.onChange = [this] {
        const int idx = factoryIr_.getSelectedId() - 2;
        const auto names = FancyReverbProcessor::factoryImpulseResponses();
        if (idx >= 0 && idx < names.size()) proc_.loadFactoryImpulseResponse(names[idx]);
        factoryIr_.setSelectedId(1, juce::dontSendNotification);
    };
    loadIr_.setTitle("Load an impulse response file");
    loadIr_.onClick = [this] { chooseIr(); };
    irName_.setColour(juce::Label::textColourId, kSilkDim); irName_.setJustificationType(juce::Justification::centredLeft);
    irName_.setFont(juce::FontOptions(11.f));
    freeze_.setTitle("Freeze: hold the reverb tail and mute the input");
    advanced_.onClick = [this] { setAdvancedVisible(advanced_.getToggleState()); };
    for (auto* c : std::initializer_list<juce::Component*>{&mode_, &quality_, &factoryIr_, &freeze_, &diffusion_, &reverse_, &advanced_, &loadIr_, &irName_})
        addAndMakeVisible(c);

    setResizable(true, true);
    setResizeLimits(880, 580, 1900, 1300);
    setSize(1040, 680);
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
    auto setOn = [&](const char* id, bool on) { for (auto& k : knobs_) if (k->paramId == id) { k->slider.setEnabled(on); k->repaint(); } };
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
    decay_->repaint(); env_->repaint(); er_->repaint(); meters_->repaint();
}

void FancyReverbEditor::paint(juce::Graphics& g) {
    g.drawImageAt(panelTexture(getWidth(), getHeight()), 0, 0);
    // Corner screws and a dividing groove under the preset bar.
    const float sr = 6.f;
    drawScrew(g, 14.f, 14.f, sr, 0.5f); drawScrew(g, getWidth() - 14.f, 14.f, sr, 2.1f);
    drawScrew(g, 14.f, getHeight() - 14.f, sr, 1.2f); drawScrew(g, getWidth() - 14.f, getHeight() - 14.f, sr, 2.9f);
    const float gy = (float)presetBar_->getBottom() + 8.f;
    g.setColour(kPanelDark); g.drawHorizontalLine((int)gy, 28.f, getWidth() - 28.f);
    g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawHorizontalLine((int)gy + 1, 28.f, getWidth() - 28.f);
    // Brand and section labels.
    drawSilk(g, "FANCYREVERB", brandArea_.toFloat().withTrimmedBottom(brandArea_.getHeight() / 2.f), juce::Justification::centredLeft, std::min(18.f, brandArea_.getHeight() * 0.5f), kIvory);
    drawSilk(g, "STEREO REVERB PROCESSOR", brandArea_.toFloat().withTrimmedTop(brandArea_.getHeight() / 2.f), juce::Justification::centredLeft, 9.f, kSilkDim, false);
    drawSilk(g, "DECAY TIME vs FREQUENCY", decayTitle_.toFloat(), juce::Justification::centredLeft, 10.f, kSilkDim);
    drawSilk(g, "REVERB OUTPUT LEVEL  /  LAST " + juce::String(FancyReverbProcessor::kEnvSize * 1024.0 / std::max(1.0, proc_.getSampleRate()), 1) + " S", envTitle_.toFloat(), juce::Justification::centredLeft, 10.f, kSilkDim);
    drawSilk(g, mode_.getSelectedId() == 1 ? "EARLY REFLECTIONS" : "IMPULSE RESPONSE  /  FIRST 500 MS", erTitle_.toFloat(), juce::Justification::centredLeft, 10.f, kSilkDim);
    drawSilk(g, "MODE", modeLabel_.toFloat(), juce::Justification::centredLeft, 9.f, kSilkDim);
    drawSilk(g, "QUALITY", qualityLabel_.toFloat(), juce::Justification::centredLeft, 9.f, kSilkDim);
}

void FancyReverbEditor::resized() {
    if (!presetBar_ || !meters_) return;       // called by the base class before the children exist
    auto r = getLocalBounds().reduced(28, 0);
    auto topRow = r.removeFromTop(58);
    topRow = topRow.withTrimmedTop(12).withTrimmedBottom(10);
    brandArea_ = topRow.removeFromLeft(std::clamp(r.getWidth() / 6, 150, 210));
    topRow.removeFromLeft(12);
    presetBar_->setBounds(topRow);
    r.removeFromTop(14);
    r.removeFromBottom(26);
    auto meters = r.removeFromRight(std::clamp(r.getWidth() / 10, 84, 120));
    r.removeFromRight(12);
    auto left = r.removeFromLeft((int)(r.getWidth() * 0.44f));
    r.removeFromLeft(14);
    // Left: three displays with silk titles above the glass.
    const int lh = left.getHeight(), titleH = 14;
    {
        auto area = left.removeFromTop((int)(lh * 0.38f)); decayTitle_ = area.removeFromTop(titleH); decay_->setBounds(area); left.removeFromTop(8);
        area = left.removeFromTop((int)(lh * 0.27f)); envTitle_ = area.removeFromTop(titleH); env_->setBounds(area); left.removeFromTop(8);
        erTitle_ = left.removeFromTop(titleH); er_->setBounds(left);
    }
    // Right: selectors, primary dials, secondary dials, advanced.
    auto top = r.removeFromTop(44);
    {
        auto labelRow = top.removeFromTop(13);
        const int gap = 8, w = top.getWidth() - 3 * gap;
        const int modeW = (int)(w * 0.32f), qualW = (int)(w * 0.22f), togW = (int)(w * 0.23f);
        modeLabel_ = labelRow.removeFromLeft(modeW); labelRow.removeFromLeft(gap);
        qualityLabel_ = labelRow.removeFromLeft(qualW);
        mode_.setBounds(top.removeFromLeft(modeW)); top.removeFromLeft(gap);
        quality_.setBounds(top.removeFromLeft(qualW)); top.removeFromLeft(gap);
        freeze_.setBounds(top.removeFromLeft(togW)); top.removeFromLeft(gap);
        diffusion_.setBounds(top);
    }
    r.removeFromTop(8);
    const int rowsAvail = r.getHeight() - 36;
    auto layoutRow = [&](juce::Rectangle<int> row, int first, int count) {
        const int w = row.getWidth() / count;
        for (int i = 0; i < count; ++i) knobs_[(size_t)(first + i)]->setBounds(row.removeFromLeft(w).reduced(2));
    };
    const float primaryShare = advancedVisible_ ? 0.30f : 0.52f, secondaryShare = advancedVisible_ ? 0.22f : 0.38f;
    layoutRow(r.removeFromTop((int)(rowsAvail * primaryShare)), 0, 4);
    layoutRow(r.removeFromTop((int)(rowsAvail * secondaryShare)), 4, 5);
    auto bar = r.removeFromTop(30);
    advanced_.setBounds(bar.removeFromLeft(98)); bar.removeFromLeft(6);
    loadIr_.setBounds(bar.removeFromLeft(74).reduced(0, 2)); bar.removeFromLeft(6);
    factoryIr_.setBounds(bar.removeFromLeft(std::min(120, bar.getWidth() / 3)).reduced(0, 1)); bar.removeFromLeft(6);
    reverse_.setBounds(bar.removeFromRight(std::min(100, bar.getWidth() / 2)));
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
    meters_->setBounds(meters);
}
