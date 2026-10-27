#include "PluginEditor.h"
#include "Ignite.h"
#include "Vfd.h"

using namespace look;

// ---- Preset bar: previous / next, a name field opening the browser, save, delete, A/B, undo / redo ----------
class UltraEditor::PresetBar : public juce::Component {
public:
    explicit PresetBar(UltraProcessor& p) : proc(p) {
        for (juce::Component* b : std::initializer_list<juce::Component*>{&prev, &next, &save, &del, &ab, &copy, &undo, &redo, &keep}) addAndMakeVisible(b);
        keep.setButtonText("Keep");
        keep.setTitle("Keep patterns: load only the kit (sounds, mixer, effects) and keep the current patterns");
        keep.setTooltip("Keep patterns: presets load the kit only");
        keep.setClickingTogglesState(true);
        keep.onClick = [this] { proc.presets.keepPatterns = keep.getToggleState(); keep.getProperties().set("accent", keep.getToggleState()); keep.repaint(); };
        addAndMakeVisible(name);
        save.setButtonText("Save"); del.setButtonText("Delete");
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
    // The preset name on a VFD (the one cold instrument, as on EXO); the button only adds hover and focus rings.
    struct NameField : public juce::TextButton {
        NameField() { setTitle("Preset browser"); addAndMakeVisible(vfd); }
        void resized() override { vfd.setBounds(getLocalBounds()); }
        void paintOverChildren(juce::Graphics& g) override {
            const auto r = getLocalBounds().toFloat().reduced(0.5f);
            if (hasKeyboardFocus(false)) { g.setColour(kAccent.withAlpha(0.8f)); g.drawRoundedRectangle(r, 5.f, 1.f); }
            else if (isMouseOver()) { g.setColour(juce::Colours::white.withAlpha(0.10f)); g.drawRoundedRectangle(r, 5.f, 1.f); }
        }
        void paintButton(juce::Graphics&, bool, bool) override {}
        void setShown(const juce::String& t, const juce::String& cat) { text = t; category = cat; vfd.setContent({cat, t, {}, {}, false}); }
        juce::String text, category;
        vfd::Display vfd;
    };
    // The arrows: the same glass with a single arrow glyph.
    struct ArrowButton : public juce::TextButton {
        explicit ArrowButton(const juce::String& glyph) { addAndMakeVisible(vfd); vfd.setContent({{}, {}, glyph, {}, false}); }
        void resized() override { vfd.setBounds(getLocalBounds()); }
        void paintButton(juce::Graphics&, bool, bool) override {}
        void paintOverChildren(juce::Graphics& g) override {
            const auto r = getLocalBounds().toFloat().reduced(0.5f);
            if (isDown()) { g.setColour(juce::Colours::black.withAlpha(0.25f)); g.fillRoundedRectangle(r, 5.f); }
            else if (isMouseOver()) { g.setColour(juce::Colours::white.withAlpha(0.10f)); g.drawRoundedRectangle(r, 5.f, 1.f); }
        }
        vfd::Display vfd;
    };
    void refresh() {
        const int ci = proc.presets.currentIndex();
        const juce::String n = proc.presets.currentName(), cat = proc.presets.currentCategory();
        const juce::String shown = n.isEmpty() ? juce::String("No preset") : n;
        if (name.text != shown || name.category != cat) name.setShown(shown, cat);
        ab.setButtonText(proc.presets.activeSlot() == 0 ? "A" : "B");
        del.setEnabled(ci >= 0 && !proc.presets.presets()[(size_t)ci].factory);
        undo.setEnabled(proc.undoManager.canUndo()); redo.setEnabled(proc.undoManager.canRedo());
    }
    void resized() override {
        auto r = getLocalBounds();
        const int h = r.getHeight();
        keep.setBounds(r.removeFromRight(54)); r.removeFromRight(12);
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
    UltraProcessor& proc;
    ArrowButton prev{"<"}, next{">"};
    juce::TextButton save, del, ab, copy, undo, redo, keep;
    NameField name;
};

namespace {
class PresetBrowser : public juce::Component, private juce::ListBoxModel {
public:
    PresetBrowser(UltraProcessor& p, std::function<void()> onPick) : proc(p), picked(std::move(onPick)) {
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
        drawCaps(g, p.tags.joinIntoString(u8("  ·  ")), juce::Rectangle<float>((float)w / 2, 0.f, (float)w / 2 - 12.f, (float)h), juce::Justification::centredRight, 8.f, kTextFaint);
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
    look::ResourceScope resources;                 // a call-out box can outlive the editor
    UltraProcessor& proc;
    std::function<void()> picked;
    juce::TextEditor search;
    juce::ListBox list;
    std::vector<Row> rows;
    int hover = -1;
};
}  // namespace

namespace {
// Update settings and status, in a call-out from the header's Updates button.
// It holds its own reference to the shared updater, so it may outlive the editor (a call-out box can).
class UpdatePanel : public juce::Component, private juce::ChangeListener {
public:
    UpdatePanel() : up(*shared) {
        optIn.setButtonText("Check for updates once a day");
        optIn.setToggleState(up.enabled(), juce::dontSendNotification);
        optIn.onClick = [this] { up.setEnabled(optIn.getToggleState()); refresh(); };
        autoInstall.setButtonText(Updater::automaticSupported() ? "Install new versions automatically when " + Updater::hostName() + " closes"
                                                                : "Automatic install is Windows only");
        autoInstall.setToggleState(up.automatic(), juce::dontSendNotification);
        autoInstall.onClick = [this] { up.setAutomatic(autoInstall.getToggleState()); refresh(); };
        check.setButtonText("Check now"); install.setButtonText("Install now"); open.setButtonText("Run installer"); skip.setButtonText("Skip version");
        check.onClick = [this] { up.checkNow(); };
        install.onClick = [this] { up.installNow(); };
        open.onClick = [this] { up.openDownloaded(); };
        skip.onClick = [this] { up.skipVersion(up.info().version); };
        for (auto* c : std::initializer_list<juce::Component*>{&optIn, &autoInstall, &check, &install, &open, &skip}) addAndMakeVisible(c);
        up.addChangeListener(this);
        refresh();
        setSize(500, 236);
    }
    ~UpdatePanel() override { up.removeChangeListener(this); }
    void changeListenerCallback(juce::ChangeBroadcaster*) override { refresh(); }
    void refresh() {
        const auto s = up.state();
        autoInstall.setEnabled(up.enabled() && Updater::automaticSupported());
        install.setEnabled((s == Updater::State::Available && up.info().url.isNotEmpty()) || s == Updater::State::Downloaded);
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
        g.drawText("Installed: version " + Updater::currentVersion() + ". The check reads one small file from the release page.", r.removeFromTop(18), juce::Justification::centredLeft);
        r.removeFromTop(104);
        juce::String msg = up.message();
        if (up.state() == Updater::State::Downloading) msg += "  " + juce::String((int)(up.progress() * 100)) + "%";
        const auto s = up.state();
        g.setColour(s == Updater::State::Failed ? kDanger : (s == Updater::State::Available || s == Updater::State::Scheduled) ? kAccent : kTextDim);
        g.drawFittedText(msg, r.removeFromTop(36), juce::Justification::topLeft, 2);
    }
    void resized() override {
        auto r = getLocalBounds().reduced(16, 12);
        r.removeFromTop(48);
        optIn.setBounds(r.removeFromTop(30));
        autoInstall.setBounds(r.removeFromTop(30));
        r.removeFromTop(44);
        auto row = r.removeFromTop(28);
        check.setBounds(row.removeFromLeft(100)); row.removeFromLeft(6);
        install.setBounds(row.removeFromLeft(104)); row.removeFromLeft(6);
        open.setBounds(row.removeFromLeft(110)); row.removeFromLeft(6);
        skip.setBounds(row);
    }
private:
    look::ResourceScope resources;                 // it can outlive the editor, so it holds the fonts too
    juce::SharedResourcePointer<Updater> shared;
    Updater& up;
    juce::ToggleButton optIn, autoInstall;
    juce::TextButton check, install, open, skip;
};
}  // namespace

void UltraEditor::PresetBar::openBrowser() {
    auto content = std::make_unique<PresetBrowser>(proc, [this] { refresh(); });
    juce::CallOutBox::launchAsynchronously(std::move(content), name.getScreenBounds(), nullptr);
}

// ---- Page tabs: a segmented switch in the console style -------------------------------------------------------
class UltraEditor::Tabs : public juce::Component {
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
                g.setColour(juce::Colour(0xff221f2a)); g.fillRoundedRectangle(cell, 4.f);
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
    juce::StringArray names{"Seq", "Voice", "Mix", "FX"};
    int current = 0;
    std::function<void(int)> onPick;
};

// ---- Editor -----------------------------------------------------------------------------------------------
static float dbTo01(float x) { const float db = 20.f * std::log10(std::max(x, 1e-5f)); return juce::jlimit(0.f, 1.f, (db + 54.f) / 48.f); }
float UltraEditor::sunLevel(UltraProcessor& p) { return dbTo01(p.machine.peak.load(std::memory_order_relaxed)); }
sun::Drive UltraEditor::sunDrive(UltraProcessor& p) { return {sunLevel(p), dbTo01(p.machine.bass.load(std::memory_order_relaxed) * 1.5f)}; }

UltraEditor::UltraEditor(UltraProcessor& p) : AudioProcessorEditor(&p), proc_(p) {
    setLookAndFeel(&look_);
    ui_.onTrackChanged = [this] {
        for (auto& pg : pages_) pg->trackChanged();
        if (pads_) pads_->repaint();
    };
    emblem_ = std::make_unique<sun::SunView>(sun::Mode::Disc, [this] { return sunDrive(proc_); }, false);
    addAndMakeVisible(*emblem_);
    wordmark_ = std::make_unique<wordmark::Wordmark>([this] { return sunLevel(proc_); });
    addAndMakeVisible(*wordmark_);
    presetBar_ = std::make_unique<PresetBar>(proc_);
    addAndMakeVisible(*presetBar_);
    ignite_.setButtonText("Ignite");
    ignite_.setTitle("Ignite: generate a new kit and patterns");
    ignite_.setTooltip("Generate a new kit and patterns from genre rules");
    ignite_.onClick = [this] {
        juce::PopupMenu m;
        m.addSectionHeader("Ignite a new kit");
        m.addItem(1, "Surprise me");
        for (int i = 0; i < ignite::kNumGenres; ++i) m.addItem(10 + i, ignite::kGenres[i]);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&ignite_), [this](int r) {
            if (r == 1) ignite(-1);
            else if (r >= 10) ignite(r - 10);
        });
    };
    addAndMakeVisible(ignite_);
    updates_.setButtonText("Updates"); updates_.setTitle("Updates: check for and install new versions");
    updates_.onClick = [this] {
        auto content = std::make_unique<UpdatePanel>();
        juce::CallOutBox::launchAsynchronously(std::move(content), updates_.getScreenBounds(), nullptr);
    };
    addAndMakeVisible(updates_);
    // The updater is shared by every instance in the process; each editor listens to it (EXO's guide, 4).
    proc_.updater->addChangeListener(this);
    changeListenerCallback(nullptr);
    display_ = std::make_unique<panels::MainDisplay>(proc_, ui_, [this] { return sunDrive(proc_); });
    addAndMakeVisible(*display_);
    macros_ = std::make_unique<panels::MacroBay>(proc_);
    addAndMakeVisible(*macros_);
    tabs_ = std::make_unique<Tabs>([this](int i) { showPage(i); });
    addAndMakeVisible(*tabs_);
    transport_ = std::make_unique<panels::Transport>(proc_);
    addAndMakeVisible(*transport_);
    pages_.push_back(std::make_unique<panels::SeqPage>(proc_, ui_));
    pages_.push_back(std::make_unique<panels::VoicePage>(proc_, ui_));
    pages_.push_back(std::make_unique<panels::MixPage>(proc_, ui_));
    pages_.push_back(std::make_unique<panels::FxPage>(proc_, ui_));
    for (auto& pg : pages_) addChildComponent(*pg);
    pads_ = std::make_unique<panels::Pads>(proc_, ui_);
    addAndMakeVisible(*pads_);
    showPage(0);
    setResizable(true, true);
    setResizeLimits(1180, 820, 1900, 1300);
    setSize(1280, 900);
    startTimerHz(20);
}

UltraEditor::~UltraEditor() {
    stopTimer(); proc_.updater->removeChangeListener(this); setLookAndFeel(nullptr);
}

void UltraEditor::forceCpuSun() { emblem_->forceCpuPath(); display_->sun.forceCpuPath(); }
void UltraEditor::renderSunNow() { emblem_->renderNow(); display_->sun.renderNow(); wordmark_->renderNow(); vfd::stepAll(*this, 1.0 / 30.0); }

void UltraEditor::showPage(int index) {
    page_ = juce::jlimit(0, (int)pages_.size() - 1, index);
    for (int i = 0; i < (int)pages_.size(); ++i) {
        pages_[(size_t)i]->setVisible(i == page_);
        vfd::setAnimatingAll(*pages_[(size_t)i], i == page_ && animating_);   // the VFDs on hidden pages keep no timers
    }
    tabs_->current = page_; tabs_->repaint();
    pages_[(size_t)page_]->tick();
}

void UltraEditor::ignite(int genre) {
    auto kit = ignite::generate(rng_, genre);
    proc_.presets.applyGenerated(kit.params, kit.patterns, kit.names, kit.name, kit.category);
    presetBar_->refresh();
}

void UltraEditor::changeListenerCallback(juce::ChangeBroadcaster*) {
    const auto s = proc_.updater->state();
    const bool badge = s == Updater::State::Available || s == Updater::State::Scheduled || s == Updater::State::Downloaded;
    updates_.setButtonText(s == Updater::State::Scheduled ? "Update ready" : badge ? "Update " + proc_.updater->info().version : "Updates");
    updates_.getProperties().set("accent", badge);
    if (badge != updateBadge_) { updateBadge_ = badge; resized(); }
    updates_.repaint();
}

bool UltraEditor::windowVisible() const {
    if (!isShowing()) return false;
    if (auto* peer = getPeer()) return !peer->isMinimised();
    return true;
}

void UltraEditor::setAnimating(bool on) {
    animating_ = on;
    emblem_->setAnimating(on); display_->sun.setAnimating(on); wordmark_->setAnimating(on);
    vfd::setAnimatingAll(*presetBar_, on);
    vfd::setAnimatingAll(*pages_[(size_t)page_], on);
}

void UltraEditor::timerCallback() {
    // Nothing animates while the window is hidden or minimised (EXO's optimization guide, 3.3).
    if (const bool vis = windowVisible(); vis != animating_) setAnimating(vis);
    if (animating_) tick();
}

void UltraEditor::tick() {
    presetBar_->refresh();
    // The scope repaints itself with each sun frame; the header only when its status line changes.
    for (auto& pg : pages_) if (pg->isVisible()) pg->tick();
    transport_->tick();
    pads_->tick();
    const juce::String cap = proc_.presets.currentDescription();
    if (cap != lastCaption_) { lastCaption_ = cap; display_->caption = cap; }
    const float bassNow = sunDrive(proc_).bass;
    bassSmooth_ += (bassNow - bassSmooth_) * (bassNow > bassSmooth_ ? 0.6f : 0.12f);
    const float glow = bassSmooth_ < 0.01f ? 0.f : bassSmooth_;
    if (std::abs(glow - look_.bassGlow.load()) > 0.02f || (glow <= 0.f && look_.bassGlow.load() > 0.f)) { look_.bassGlow.store(glow); macros_->repaint(); }
    if (const auto s = statusText(); s != lastStatus_) { lastStatus_ = s; repaint(statusArea()); }
}

juce::String UltraEditor::statusText() const {
    auto& m = proc_.machine;
    const bool running = m.running.load(std::memory_order_relaxed);
    const int pat = running ? m.currentPattern.load() : (int)std::lround(proc_.apvts.getRawParameterValue("pattern")->load());
    juce::String s = juce::String("Pattern ") + ultra::kPatternNames[juce::jlimit(0, ultra::kPatterns - 1, pat)];
    s << (running ? u8("  ·  step ") + juce::String(m.stepNow[0].load() + 1) : u8("  ·  stopped"));
    s << u8("  ·  ") << juce::String(proc_.hostBpm.load(), 1) << u8(" bpm  ·  ") << juce::String(juce::roundToInt(proc_.getSampleRate() / 1000.0)) << u8(" kHz  ·  v") << JucePlugin_VersionString;
    return s;
}

juce::Rectangle<int> UltraEditor::statusArea() const {
    auto b = brandArea_;
    b.removeFromLeft(b.getHeight() + 14);
    b.removeFromTop((int)(b.getHeight() * 0.55f));
    return b;
}

void UltraEditor::paint(juce::Graphics& g) {
    // The texture and the plates (with their blurred drop shadows) are static: they are drawn once per window
    // size into one image, not on every repaint of the dials, displays and suns in front of them.
    if (!chrome_.isValid() || chrome_.getWidth() != getWidth() || chrome_.getHeight() != getHeight()) {
        chrome_ = juce::Image(juce::Image::RGB, std::max(1, getWidth()), std::max(1, getHeight()), false);
        juce::Graphics cg(chrome_);
        cg.drawImageAt(backgroundTexture(getWidth(), getHeight()), 0, 0);
        drawPlate(cg, macroPlate_.toFloat());
        drawPlate(cg, mainPlate_.toFloat());
    }
    g.drawImageAt(chrome_, 0, 0);
    drawCaps(g, lastStatus_ = statusText(), statusArea().toFloat(), juce::Justification::centredLeft, 8.5f, kTextFaint);
}

void UltraEditor::resized() {
    if (!presetBar_ || !tabs_ || !pads_) return;
    chrome_ = juce::Image();
    const int m = 22;
    auto r = getLocalBounds().reduced(m, 0);
    auto header = r.removeFromTop(68).withTrimmedTop(18).withTrimmedBottom(16);
    brandArea_ = header.removeFromLeft(std::clamp(getWidth() / 4 - 30, 262, 330));   // narrower on small windows: room for the preset name
    emblem_->setBounds(brandArea_.withWidth(brandArea_.getHeight() + 8).expanded(4));
    emblem_->background = [this, at = emblem_->getPosition()](juce::Graphics& g) { g.drawImageAt(backgroundTexture(getWidth(), getHeight()), -at.x, -at.y); };
    {
        auto b = brandArea_; b.removeFromLeft(brandArea_.getHeight() + 14);
        wordmark_->setBounds(b.removeFromTop((int)(b.getHeight() * 0.55f)).withTrimmedLeft(-8).withTrimmedRight(-8));
    }
    header.removeFromLeft(18);
    ignite_.setBounds(header.removeFromRight(getWidth() < 1300 ? 80 : 96)); header.removeFromRight(10);
    updates_.setBounds(header.removeFromRight(updateBadge_ ? 128 : 84)); header.removeFromRight(14);
    presetBar_->setBounds(header);
    r.removeFromBottom(m - 8);
    padArea_ = r.removeFromBottom(std::clamp(r.getHeight() / 12, 56, 84));
    pads_->setBounds(padArea_);
    r.removeFromBottom(12);
    // Display row: the scope and the macro bay.
    auto top = r.removeFromTop(std::clamp((int)(r.getHeight() * 0.33f), 210, 340));
    macroPlate_ = top.removeFromRight(std::clamp(top.getWidth() * 3 / 10, 360, 480));
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
    auto tabRow = in.removeFromTop(30);
    tabs_->setBounds(tabRow.removeFromLeft(std::min(400, tabRow.getWidth() * 2 / 5)));
    tabRow.removeFromLeft(24);
    transport_->setBounds(tabRow.removeFromRight(std::min(560, tabRow.getWidth())));
    in.removeFromTop(12);
    for (auto& pg : pages_) pg->setBounds(in);
}
