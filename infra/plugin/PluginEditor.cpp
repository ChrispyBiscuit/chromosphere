#include "PluginEditor.h"
#include "Ignite.h"

using namespace look;

// ---- Preset bar: previous / next, a name field opening the browser, save, delete, A/B, undo / redo ----------
class InfraEditor::PresetBar : public juce::Component {
public:
    explicit PresetBar(InfraProcessor& p) : proc(p) {
        for (auto* b : {&prev, &next, &save, &del, &ab, &copy, &undo, &redo}) addAndMakeVisible(b);
        addAndMakeVisible(name);
        prev.setButtonText("<"); next.setButtonText(">"); save.setButtonText("Save"); del.setButtonText("Delete");
        ab.setButtonText("A"); copy.setButtonText("A > B"); undo.setButtonText("Undo"); redo.setButtonText("Redo");
        prev.setTitle("Previous preset"); next.setTitle("Next preset"); save.setTitle("Save user preset"); del.setTitle("Delete user preset");
        ab.setTitle("Toggle A/B comparison"); copy.setTitle("Copy this setting to the other slot"); undo.setTitle("Undo"); redo.setTitle("Redo");
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
            g.setColour(down ? juce::Colour(0xff0a0809) : kDisplayBg); g.fillRoundedRectangle(r, 6.f);
            g.setColour(hasKeyboardFocus(false) ? kAccent.withAlpha(0.8f) : juce::Colours::white.withAlpha(over ? 0.14f : 0.08f)); g.drawRoundedRectangle(r, 6.f, 1.f);
            auto inner = r.reduced(12.f, 0.f);
            const float catW = std::min(130.f, juce::GlyphArrangement::getStringWidth(caps(9.5f), category.toUpperCase()) + 4.f);
            drawCaps(g, category, inner.removeFromLeft(catW), juce::Justification::centredLeft, 9.5f, kTextFaint);
            inner.removeFromRight(catW);
            g.setFont(font(14.f, 500)); g.setColour(kText); g.drawText(text, inner, juce::Justification::centred, true);
        }
        juce::String text, category;
    };
    void refresh() {
        const int ci = proc.presets.currentIndex();
        const juce::String n = proc.presets.currentName(), cat = proc.presets.currentCategory();
        if (name.text != n || name.category != cat) { name.text = n.isEmpty() ? "No preset" : n; name.category = cat; name.repaint(); }
        ab.setButtonText(proc.presets.activeSlot() == 0 ? "A" : "B");
        del.setEnabled(ci >= 0 && !proc.presets.presets()[(size_t)ci].factory);
        undo.setEnabled(proc.undoManager.canUndo()); redo.setEnabled(proc.undoManager.canRedo());
    }
    void resized() override {
        auto r = getLocalBounds();
        const int h = r.getHeight();
        redo.setBounds(r.removeFromRight(54)); r.removeFromRight(6);
        undo.setBounds(r.removeFromRight(54)); r.removeFromRight(12);
        copy.setBounds(r.removeFromRight(56)); r.removeFromRight(6);
        ab.setBounds(r.removeFromRight(h)); r.removeFromRight(12);
        del.setBounds(r.removeFromRight(62)); r.removeFromRight(6);
        save.setBounds(r.removeFromRight(54)); r.removeFromRight(12);
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
    InfraProcessor& proc;
    juce::TextButton prev, next, save, del, ab, copy, undo, redo;
    NameField name;
};

namespace {
class PresetBrowser : public juce::Component, private juce::ListBoxModel {
public:
    PresetBrowser(InfraProcessor& p, std::function<void()> onPick) : proc(p), picked(std::move(onPick)) {
        search.setTextToShowWhenEmpty("Search name, category, tag or description", kTextFaint);
        search.setFont(font(13.f));
        search.onTextChange = [this] { rebuild(); };
        search.setTitle("Search presets");
        list.setModel(this); list.setRowHeight(26); list.setTitle("Presets");
        addAndMakeVisible(search); addAndMakeVisible(list);
        rebuild();
        setSize(520, 560);
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(kPlate);
        // Description of the row under the mouse (or the current preset) at the bottom.
        const int i = hover >= 0 ? hover : proc.presets.currentIndex();
        if (i >= 0 && i < (int)proc.presets.presets().size()) {
            g.setFont(font(12.f)); g.setColour(kTextDim);
            g.drawFittedText(proc.presets.presets()[(size_t)i].description, getLocalBounds().reduced(12, 8).removeFromBottom(34), juce::Justification::centredLeft, 2);
        }
    }
    void resized() override { auto r = getLocalBounds().reduced(10); search.setBounds(r.removeFromTop(28)); r.removeFromTop(8); r.removeFromBottom(40); list.setBounds(r); }
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
                if (q.isNotEmpty() && !(p.name.toLowerCase().contains(q) || p.category.toLowerCase().contains(q) ||
                                        p.tags.joinIntoString(" ").toLowerCase().contains(q) || p.description.toLowerCase().contains(q))) continue;
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
        if (r.index < 0) { drawCaps(g, r.text, juce::Rectangle<float>(10.f, 0.f, (float)w - 10.f, (float)h), juce::Justification::centredLeft, 9.5f, kAccent.withAlpha(0.8f)); return; }
        if (sel || r.index == proc.presets.currentIndex()) { g.setColour(juce::Colours::white.withAlpha(sel ? 0.08f : 0.04f)); g.fillRoundedRectangle(juce::Rectangle<float>(4.f, 1.f, (float)w - 8.f, (float)h - 2.f), 4.f); }
        const auto& p = proc.presets.presets()[(size_t)r.index];
        g.setColour(r.index == proc.presets.currentIndex() ? kAccent : kText); g.setFont(font(13.5f));
        g.drawText(r.text + (p.factory ? "" : "  (user)"), 20, 0, w / 2, h, juce::Justification::centredLeft);
        drawCaps(g, p.tags.joinIntoString("  ·  "), juce::Rectangle<float>((float)w / 2, 0.f, (float)w / 2 - 12.f, (float)h), juce::Justification::centredRight, 8.f, kTextFaint);
    }
    void listBoxItemClicked(int row, const juce::MouseEvent&) override { choose(row); }
    void returnKeyPressed(int row) override { choose(row); }
    void selectedRowsChanged(int row) override { hover = row >= 0 && row < (int)rows.size() ? rows[(size_t)row].index : -1; repaint(); }
    void choose(int row) {
        if (row < 0 || row >= (int)rows.size() || rows[(size_t)row].index < 0) return;
        proc.presets.apply(rows[(size_t)row].index);
        picked();
    }
    struct Row { int index; juce::String text; };
    InfraProcessor& proc;
    std::function<void()> picked;
    juce::TextEditor search;
    juce::ListBox list;
    std::vector<Row> rows;
    int hover = -1;
};
}  // namespace

void InfraEditor::PresetBar::openBrowser() {
    auto content = std::make_unique<PresetBrowser>(proc, [this] { refresh(); });
    juce::CallOutBox::launchAsynchronously(std::move(content), name.getScreenBounds(), nullptr);
}

// ---- Page tabs: a segmented switch in the console style -------------------------------------------------------
class InfraEditor::Tabs : public juce::Component {
public:
    explicit Tabs(std::function<void(int)> pick) : onPick(std::move(pick)) { setTitle("Pages"); }
    void paint(juce::Graphics& g) override {
        const auto r = getLocalBounds().toFloat();
        g.setColour(kDisplayBg); g.fillRoundedRectangle(r, 6.f);
        g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawRoundedRectangle(r.reduced(0.5f), 6.f, 1.f);
        const float w = r.getWidth() / (float)names.size();
        for (int i = 0; i < names.size(); ++i) {
            auto cell = juce::Rectangle<float>(r.getX() + w * i, r.getY(), w, r.getHeight()).reduced(3.f);
            if (i == current) {
                g.setColour(juce::Colour(0xff242024)); g.fillRoundedRectangle(cell, 4.f);
                g.setColour(juce::Colours::white.withAlpha(0.06f)); g.drawRoundedRectangle(cell, 4.f, 1.f);
                g.setColour(kAccent); g.fillRoundedRectangle(cell.removeFromBottom(2.f).reduced(cell.getWidth() * 0.3f, 0.f), 1.f);
            }
            drawCaps(g, names[i], juce::Rectangle<float>(r.getX() + w * i, r.getY(), w, r.getHeight()), juce::Justification::centred, 10.5f, i == current ? kText : kTextDim);
        }
    }
    void mouseDown(const juce::MouseEvent& e) override {
        const int i = juce::jlimit(0, names.size() - 1, (int)(e.x / (getWidth() / (float)names.size())));
        if (onPick) onPick(i);
    }
    juce::StringArray names{"Synth", "Mod", "Arp", "FX"};
    int current = 0;
    std::function<void(int)> onPick;
};

// ---- Editor -----------------------------------------------------------------------------------------------
static float dbTo01(float x) { const float db = 20.f * std::log10(std::max(x, 1e-5f)); return juce::jlimit(0.f, 1.f, (db + 54.f) / 48.f); }
float InfraEditor::sunLevel(InfraProcessor& p) { return dbTo01(p.synth.peak.load(std::memory_order_relaxed)); }
sun::Drive InfraEditor::sunDrive(InfraProcessor& p) { return {sunLevel(p), dbTo01(p.synth.bass.load(std::memory_order_relaxed) * 1.5f)}; }

InfraEditor::InfraEditor(InfraProcessor& p)
    : AudioProcessorEditor(&p), proc_(p), keyboard_(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard) {
    setLookAndFeel(&look_);
    emblem_ = std::make_unique<sun::SunView>(sun::Mode::Disc, [this] { return sunDrive(proc_); }, false);
    addAndMakeVisible(*emblem_);
    wordmark_ = std::make_unique<wordmark::Wordmark>([this] { return sunLevel(proc_); });
    addAndMakeVisible(*wordmark_);
    presetBar_ = std::make_unique<PresetBar>(proc_);
    addAndMakeVisible(*presetBar_);
    ignite_.setButtonText("Ignite");
    ignite_.setTitle("Ignite: generate a new patch");
    ignite_.setTooltip("Generate a new patch from musical rules");
    ignite_.onClick = [this] {
        juce::PopupMenu m;
        m.addSectionHeader("Ignite a new patch");
        m.addItem(1, "Surprise me");
        for (int i = 0; i < ignite::kNumArchetypes; ++i) m.addItem(10 + i, ignite::kArchetypes[i]);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&ignite_), [this](int r) {
            if (r == 1) ignite(-1);
            else if (r >= 10) ignite(r - 10);
        });
    };
    addAndMakeVisible(ignite_);
    display_ = std::make_unique<panels::MainDisplay>(proc_, [this] { return sunDrive(proc_); });
    addAndMakeVisible(*display_);
    macros_ = std::make_unique<panels::MacroBay>(proc_);
    addAndMakeVisible(*macros_);
    tabs_ = std::make_unique<Tabs>([this](int i) { showPage(i); });
    addAndMakeVisible(*tabs_);
    pages_.push_back(std::make_unique<panels::SynthPage>(proc_));
    pages_.push_back(std::make_unique<panels::ModPage>(proc_));
    pages_.push_back(std::make_unique<panels::ArpPage>(proc_));
    pages_.push_back(std::make_unique<panels::FxPage>(proc_));
    for (auto& pg : pages_) addChildComponent(*pg);
    keyboard_.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xff2a2629));
    keyboard_.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff0f0c0d));
    keyboard_.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff151213));
    keyboard_.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, kAccent.withAlpha(0.25f));
    keyboard_.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, kAccent.withAlpha(0.8f));
    keyboard_.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha(0.5f));
    keyboard_.setColour(juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, kPlate);
    keyboard_.setColour(juce::MidiKeyboardComponent::upDownButtonArrowColourId, kTextDim);
    keyboard_.setColour(juce::MidiKeyboardComponent::textLabelColourId, kTextFaint);
    keyboard_.setLowestVisibleKey(36);
    keyboard_.setWantsKeyboardFocus(false);
    addAndMakeVisible(keyboard_);
    showPage(0);
    setResizable(true, true);
    setResizeLimits(1180, 820, 1900, 1300);
    setSize(1240, 860);
    startTimerHz(30);
}

InfraEditor::~InfraEditor() { stopTimer(); setLookAndFeel(nullptr); }

void InfraEditor::forceCpuSun() { emblem_->forceCpuPath(); display_->sun.forceCpuPath(); }
void InfraEditor::renderSunNow() { emblem_->renderNow(); display_->sun.renderNow(); wordmark_->renderNow(); }

void InfraEditor::showPage(int index) {
    page_ = juce::jlimit(0, (int)pages_.size() - 1, index);
    for (int i = 0; i < (int)pages_.size(); ++i) pages_[(size_t)i]->setVisible(i == page_);
    tabs_->current = page_; tabs_->repaint();
    pages_[(size_t)page_]->tick();
}

void InfraEditor::ignite(int archetype) {
    auto patch = ignite::generate(rng_, archetype);
    proc_.presets.applyGenerated(patch.params, patch.name, patch.category);
    presetBar_->refresh();
}

void InfraEditor::timerCallback() {
    presetBar_->refresh();
    display_->repaint();
    for (auto& pg : pages_) if (pg->isVisible()) pg->tick();
    const juce::String cap = proc_.presets.currentDescription();
    if (cap != lastCaption_) { lastCaption_ = cap; display_->caption = cap; }
    const float bassNow = sunDrive(proc_).bass;
    bassSmooth_ += (bassNow - bassSmooth_) * (bassNow > bassSmooth_ ? 0.6f : 0.12f);
    const float glow = bassSmooth_ < 0.01f ? 0.f : bassSmooth_;
    if (std::abs(glow - look_.bassGlow.load()) > 0.004f) { look_.bassGlow.store(glow); macros_->repaint(); }
    repaint(brandArea_);
}

void InfraEditor::paint(juce::Graphics& g) {
    g.drawImageAt(backgroundTexture(getWidth(), getHeight()), 0, 0);
    drawPlate(g, macroPlate_.toFloat());
    drawPlate(g, mainPlate_.toFloat());
    // Status line under the wordmark.
    auto b = brandArea_.toFloat();
    b.removeFromLeft(b.getHeight() + 14.f);
    b.removeFromTop(b.getHeight() * 0.55f);
    const int voices = proc_.synth.activeVoices.load(std::memory_order_relaxed);
    const int poly = (int)std::lround(proc_.apvts.getRawParameterValue("polyphony")->load());
    const juce::String status = "Voices " + juce::String(voices) + " / " + juce::String(poly) + "  ·  " +
                                juce::String(proc_.hostBpm.load(), 1) + " bpm  ·  " + juce::String(proc_.getSampleRate() / 1000.0, 1) + " kHz  ·  v" +
                                JucePlugin_VersionString;
    drawCaps(g, status, b, juce::Justification::centredLeft, 8.5f, kTextFaint);
}

void InfraEditor::resized() {
    if (!presetBar_ || !tabs_) return;
    const int m = 22;
    auto r = getLocalBounds().reduced(m, 0);
    auto header = r.removeFromTop(68).withTrimmedTop(18).withTrimmedBottom(16);
    brandArea_ = header.removeFromLeft(330);
    emblem_->setBounds(brandArea_.withWidth(brandArea_.getHeight() + 8).expanded(4));
    emblem_->background = [this, at = emblem_->getPosition()](juce::Graphics& g) { g.drawImageAt(backgroundTexture(getWidth(), getHeight()), -at.x, -at.y); };
    {
        auto b = brandArea_; b.removeFromLeft(brandArea_.getHeight() + 14);
        wordmark_->setBounds(b.removeFromTop((int)(b.getHeight() * 0.55f)).withTrimmedLeft(-8).withTrimmedRight(-8));
    }
    header.removeFromLeft(18);
    ignite_.setBounds(header.removeFromRight(96)); header.removeFromRight(14);
    presetBar_->setBounds(header);
    r.removeFromBottom(m - 8);
    keyboardArea_ = r.removeFromBottom(std::clamp(r.getHeight() / 13, 52, 76));
    keyboard_.setBounds(keyboardArea_);
    keyboard_.setKeyWidth(std::max(10.f, keyboardArea_.getWidth() / 52.f));
    r.removeFromBottom(12);
    // Display row: the scope and the macro bay.
    auto top = r.removeFromTop(std::clamp((int)(r.getHeight() * 0.38f), 236, 380));
    macroPlate_ = top.removeFromRight(std::clamp(top.getWidth() / 4, 280, 360));
    macros_->setBounds(macroPlate_.reduced(20, 16));
    top.removeFromRight(14);
    displayPlate_ = top;
    display_->setBounds(top);
    display_->sun.background = [this, at = display_->getPosition(), sz = display_->getLocalBounds()](juce::Graphics& g) {
        g.drawImageAt(backgroundTexture(getWidth(), getHeight()), -at.x, -at.y);
        g.setColour(kDisplayBg); g.fillRoundedRectangle(sz.toFloat(), 8.f);
    };
    r.removeFromTop(14);
    mainPlate_ = r;
    auto in = mainPlate_.reduced(24, 16);
    tabs_->setBounds(in.removeFromTop(30).removeFromLeft(std::min(420, in.getWidth() / 2)));
    in.removeFromTop(12);
    for (auto& pg : pages_) pg->setBounds(in);
}
