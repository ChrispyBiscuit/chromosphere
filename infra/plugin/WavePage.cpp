#include "WavePage.h"
#include "PluginProcessor.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#if JucePlugin_Build_Standalone
 #include <juce_audio_utils/juce_audio_utils.h>     // the standalone holder needs the device manager and player first
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

using namespace look;

namespace panels {

namespace {
// The CRT colours of the main display.
const juce::Colour kAmber(0xffffb000), kAmberText(0xb8ffb000), kAmberGrid(0x29ffb000);

void setOn(juce::Component* c, bool on) { if (c && c->isEnabled() != on) { c->setEnabled(on); c->setAlpha(on ? 1.f : 0.45f); } }
float raw(InfraProcessor& p, const char* id) { return p.apvts.getRawParameterValue(id)->load(std::memory_order_relaxed); }
int rawI(InfraProcessor& p, const char* id) { return (int)std::lround(raw(p, id)); }
juce::String noteName(int m) { return juce::String(wave::kKeyNames[((m % 12) + 12) % 12]) + juce::String(m / 12 - 1); }
}  // namespace

// ---- Voice trace ------------------------------------------------------------------------------------------
VoiceTrace::VoiceTrace(InfraProcessor& p) : proc_(p) { setOpaque(false); setTitle("Voice trace"); }

void VoiceTrace::tick() {
    if (!isShowing()) return;
    auto& w = proc_.wave;
    // Follow the voice: the middle row eases towards the recent notes.
    const int note = w.noteNow.load(std::memory_order_relaxed);
    const float pitch = w.pitchNow.load(std::memory_order_relaxed);
    const float target = note >= 0 ? (float)note : pitch > 0.f ? pitch : centre_;
    centre_ += (std::clamp(target, 30.f, 96.f) - centre_) * 0.08f;
    uint64_t sig = 1469598103934665603ull;
    auto mix = [&sig](uint32_t v) { sig = (sig ^ v) * 1099511628211ull; };
    for (const char* id : {"wvOn", "wvKey", "wvScale", "wvMode", "wvGate"}) mix((uint32_t)std::lround(raw(proc_, id) * 10.f));
    mix((uint32_t)std::lround(centre_ * 20.f));
    for (auto ch : status) mix((uint32_t)ch);
    const int pos = w.tracePos.load(std::memory_order_acquire);
    if (pos != lastPos_ || sig != lastSig_) { lastPos_ = pos; lastSig_ = sig; repaint(); }
}

void VoiceTrace::paint(juce::Graphics& g) {
    auto all = getLocalBounds().toFloat();
    drawDisplay(g, all, 8.f);
    auto& w = proc_.wave;
    const bool on = raw(proc_, "wvOn") > 0.5f;
    const int key = rawI(proc_, "wvKey");
    const auto scale = (wave::Scale)rawI(proc_, "wvScale");
    const bool chords = rawI(proc_, "wvMode") == (int)wave::Mode::Chords;
    const float gate = raw(proc_, "wvGate");

    auto scanlines = [&] {   // raster lines across the tube face, as on the main display
        g.setColour(juce::Colours::black.withAlpha(0.12f));
        for (float y = all.getY() + 1.f; y < all.getBottom(); y += 3.f) g.fillRect(all.getX() + 2.f, y, all.getWidth() - 4.f, 1.f);
    };
    if (!on) {   // the engine is idle: no stale trace, just the way in
        g.setFont(font(13.f)); g.setColour(kAmberText);
        g.drawFittedText("WAVE is off. Switch it on, then sing, hum or whistle into the input.", all.reduced(30.f).toNearestInt(), juce::Justification::centred, 3);
        scanlines();
        return;
    }

    auto r = all.reduced(14.f, 10.f);
    auto head = r.removeFromTop(18.f);
    auto levelStrip = r.removeFromBottom(26.f);
    r.removeFromBottom(4.f);
    auto roll = r.withTrimmedLeft(30.f);

    // Readouts: the sung note and how far off it is, and the chord.
    const int note = w.noteNow.load(std::memory_order_relaxed);
    const float pitch = w.pitchNow.load(std::memory_order_relaxed);
    g.setFont(font(13.f, 600)); g.setColour(kAmber);
    if (pitch > 0.f) {
        const int nearest = (int)std::lround(pitch);
        const int cents = (int)std::lround((pitch - (float)nearest) * 100.f);
        g.drawText(noteName(nearest) + "  " + (cents >= 0 ? "+" : "") + juce::String(cents) + " ct", head, juce::Justification::centredLeft);
    } else {
        g.setColour(kAmberText); g.drawText("-", head, juce::Justification::centredLeft);
    }
    const int root = w.chordRoot.load(std::memory_order_relaxed);
    if (chords && note >= 0 && root >= 0) {
        g.setColour(kAmber);
        g.drawText(juce::String(wave::kKeyNames[root % 12]) + wave::chordSuffix(w.chordMask.load(std::memory_order_relaxed)), head, juce::Justification::centredRight);
    } else {
        drawCaps(g, juce::String(wave::kKeyNames[key]) + " " + wave::kScaleNames[(int)scale], head, juce::Justification::centredRight, 9.f, kAmberText);
    }

    // The piano roll: two octaves round the voice, the scale's rows lit, the key's root brighter.
    const float lo = std::round(centre_) - 12.f, rows = 25.f, rowH = roll.getHeight() / rows;
    auto yOf = [&](float m) { return roll.getBottom() - (m - lo + 0.5f) * rowH; };
    for (int m = (int)lo; m < (int)(lo + rows); ++m) {
        const bool inKey = wave::inScale(m, key, scale), isRoot = wave::mod12(m - key) == 0;
        if (!inKey) continue;
        g.setColour(kAmberGrid.withMultipliedAlpha(isRoot ? 2.2f : 1.f));
        g.fillRect(roll.getX(), yOf((float)m) - 0.5f, roll.getWidth(), 1.f);
        if (isRoot || wave::mod12(m) == 0) {
            g.setFont(font(9.5f)); g.setColour(isRoot ? kAmberText : kAmberText.withMultipliedAlpha(0.6f));
            g.drawText(noteName(m), juce::Rectangle<float>(r.getX(), yOf((float)m) - 7.f, 28.f, 14.f), juce::Justification::centredLeft);
        }
    }

    // The last four seconds, oldest at the left.
    const int n = std::min(wave::Engine::kTrace, (int)(4.0 / std::max(1e-4, w.hopSeconds())));
    const int pos = w.tracePos.load(std::memory_order_acquire);
    auto at = [&](int i) { return (size_t)((pos - n + i + 2 * wave::Engine::kTrace) % wave::Engine::kTrace); };
    const float dx = roll.getWidth() / (float)n;
    {   // Played notes: bars.
        g.saveState();
        g.reduceClipRegion(roll.toNearestInt());
        int runStart = 0, runNote = -1;
        auto flush = [&](int end) {
            if (runNote < 0) return;
            const auto bar = juce::Rectangle<float>(roll.getX() + runStart * dx, yOf((float)runNote) - rowH * 0.38f, (end - runStart) * dx, rowH * 0.76f);
            g.setColour(kAccent.withAlpha(0.45f)); g.fillRoundedRectangle(bar, 2.f);
            g.setColour(kAccent.withAlpha(0.9f)); g.drawRoundedRectangle(bar, 2.f, 1.f);
        };
        for (int i = 0; i < n; ++i) {
            const int m = w.traceNote[at(i)].load(std::memory_order_relaxed);
            if (m != runNote) { flush(i); runStart = i; runNote = m; }
        }
        flush(n);
        // The sung pitch: an amber trace, broken where the voice is unvoiced.
        juce::Path path; bool pen = false;
        for (int i = 0; i < n; ++i) {
            const float p = w.tracePitch[at(i)].load(std::memory_order_relaxed);
            if (p <= 0.f) { pen = false; continue; }
            const float x = roll.getX() + (i + 0.5f) * dx, y = yOf(p);
            if (!pen) path.startNewSubPath(x, y); else path.lineTo(x, y);
            pen = true;
        }
        g.setColour(kAmber.withAlpha(0.25f)); g.strokePath(path, juce::PathStrokeType(4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(kAmber); g.strokePath(path, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.restoreState();
    }

    // Input level under the roll, with the gate.
    {
        auto lv = levelStrip.withTrimmedLeft(30.f);
        auto yLv = [&](float db) { return lv.getBottom() - std::clamp((db + 70.f) / 70.f, 0.f, 1.f) * lv.getHeight(); };
        juce::Path fill; fill.startNewSubPath(lv.getX(), lv.getBottom());
        for (int i = 0; i < n; i += 2) fill.lineTo(lv.getX() + (i + 0.5f) * dx, yLv(w.traceLevel[at(i)].load(std::memory_order_relaxed)));
        fill.lineTo(lv.getRight(), lv.getBottom()); fill.closeSubPath();
        g.setColour(kAmber.withAlpha(0.28f)); g.fillPath(fill);
        g.setColour(kAccent.withAlpha(0.7f));
        const float gy = yLv(gate);
        for (float x = lv.getX(); x < lv.getRight(); x += 8.f) g.fillRect(x, gy, 4.f, 1.f);
        drawCaps(g, "In", levelStrip.withWidth(28.f), juce::Justification::centredLeft, 8.f, kAmberText);
    }

    if (status.isNotEmpty()) {
        auto s = juce::Rectangle<float>(all.getX() + 20.f, all.getY() + 32.f, all.getWidth() - 40.f, 20.f);
        g.setColour(juce::Colours::black.withAlpha(0.55f)); g.fillRoundedRectangle(s, 4.f);
        g.setFont(font(11.5f)); g.setColour(kAmber); g.drawText(status, s, juce::Justification::centred);
    }
    scanlines();
}

// ---- Page -------------------------------------------------------------------------------------------------
WavePage::WavePage(InfraProcessor& p) : Page(p), trace_(p) {
    addAndMakeVisible(trace_);
    toggle("wvOn", "WAVE on"); choice("wvRange", "Range");
    knob("wvGain", "Gain", Knob::Small); knob("wvGate", "Gate", Knob::Small); knob("wvMonitor", "Monitor", Knob::Small);
    choice("wvKey", "Key"); choice("wvScale", "Scale");
    knob("wvFollow", "Follow", Knob::Small); knob("wvStab", "Stability", Knob::Small);
    knob("wvOct", "Octave", Knob::Small, true); knob("wvDyn", "Dynamics", Knob::Small);
    choice("wvMode", "Mode"); choice("wvChord", "Chord"); choice("wvRole", "Sung note is");
    choice("wvVoicing", "Voicing"); choice("wvBass", "Bass"); toggle("wvHold", "Hold");
    knob("wvStrum", "Strum", Knob::Small);
    choice("wvLevelTo", "Loudness to"); choice("wvBrightTo", "Brightness to"); toggle("wvMidiOut", "MIDI out");
    listen_.setButtonText("Find key");
    listen_.setTitle("Listen for the key: sing, then press again to set Key and Scale");
    listen_.setTooltip("Sing a few phrases, then press again: WAVE sets Key and Scale to the key you sang in");
    listen_.onClick = [this] { toggleListen(); };
    addAndMakeVisible(listen_);
}

void WavePage::toggleListen() {
    auto& w = proc_.wave;
    if (!w.listening()) {
        w.listen(true);
        listen_.setButtonText("Set key");
        return;
    }
    w.listen(false);
    listen_.setButtonText("Find key");
    if (w.listenedSeconds() < 1.5f) {
        trace_.status = "Not enough singing to tell the key (" + juce::String(w.listenedSeconds(), 1) + " s). Try again.";
        return;
    }
    const auto k = w.guessKey();
    auto set = [&](const char* id, float plain) {
        if (auto* prm = proc_.apvts.getParameter(id)) {
            prm->beginChangeGesture(); prm->setValueNotifyingHost(prm->convertTo0to1(plain)); prm->endChangeGesture();
        }
    };
    set("wvKey", (float)k.key);
    set("wvScale", (float)(k.minor ? wave::Scale::NaturalMinor : wave::Scale::Major));
    trace_.status = juce::String("Key set: ") + wave::kKeyNames[k.key] + (k.minor ? " minor" : " major");
}

void WavePage::tick() {
    const bool on = raw(proc_, "wvOn") > 0.5f;
    for (const char* id : {"wvRange", "wvGain", "wvGate", "wvMonitor", "wvKey", "wvScale", "wvFollow", "wvStab", "wvOct", "wvDyn",
                           "wvMode", "wvHold", "wvLevelTo", "wvBrightTo", "wvMidiOut"})
        setOn(c(id), on);
    const bool chords = on && rawI(proc_, "wvMode") == (int)wave::Mode::Chords;
    for (const char* id : {"wvChord", "wvRole", "wvVoicing", "wvBass", "wvStrum"}) setOn(c(id), chords);
    setOn(&listen_, on);
    auto& w = proc_.wave;
    if (w.listening()) trace_.status = "Listening: " + juce::String(w.listenedSeconds(), 1) + " s sung. Press Set key when done.";
    else if (on && !wasOn_) trace_.status = {};
    wasOn_ = on;
    trace_.tick();
}

void WavePage::resized() {
    beginLayout();
    auto r = getLocalBounds();
    auto head = [&](juce::Rectangle<int>& b, const char* code, const char* t) { section(code, t, b.removeFromTop(14)); b.removeFromTop(6); };
    auto left = r.removeFromLeft(std::clamp(r.getWidth() * 36 / 100, 300, 560));
    head(left, "19", "Voice trace");
    trace_.setBounds(left);
    r.removeFromLeft(22);
    auto top = r.removeFromTop((r.getHeight() - 14) / 2);
    r.removeFromTop(14);
    auto bottom = r;

    auto input = top.removeFromLeft(top.getWidth() * 2 / 5);
    top.removeFromLeft(20);
    auto notes = top;
    head(input, "20", "Input");
    {
        auto line = input.removeFromTop(44);
        c("wvOn")->setBounds(line.removeFromLeft(110).withSizeKeepingCentre(110, 24));
        line.removeFromLeft(10);
        c("wvRange")->setBounds(line);
        input.removeFromTop(4);
        row(input, {c("wvGain"), c("wvGate"), c("wvMonitor")});
    }
    head(notes, "21", "Notes");
    {
        auto line = notes.removeFromTop(44);
        c("wvKey")->setBounds(line.removeFromLeft(std::min(90, line.getWidth() / 4)));
        line.removeFromLeft(8);
        listen_.setBounds(line.removeFromRight(std::min(130, line.getWidth() / 3)).withTrimmedTop(15).withSizeKeepingCentre(std::min(130, line.getWidth() / 3), 26));
        line.removeFromRight(8);
        c("wvScale")->setBounds(line);
        notes.removeFromTop(4);
        row(notes, {c("wvFollow"), c("wvStab"), c("wvOct"), c("wvDyn")});
    }

    auto chordsArea = bottom.removeFromLeft(bottom.getWidth() * 64 / 100);
    bottom.removeFromLeft(20);
    auto expr = bottom;
    head(chordsArea, "22", "Chords");
    {
        auto strum = chordsArea.removeFromRight(std::min(96, chordsArea.getWidth() / 5));
        c("wvStrum")->setBounds(strum);
        chordsArea.removeFromRight(10);
        const int lineH = std::min(44, chordsArea.getHeight() / 2 - 4);
        auto l1 = chordsArea.removeFromTop(lineH);
        chordsArea.removeFromTop(8);
        auto l2 = chordsArea.removeFromTop(lineH);
        const int cw = (l1.getWidth() - 16) / 3;
        c("wvMode")->setBounds(l1.removeFromLeft(cw)); l1.removeFromLeft(8);
        c("wvChord")->setBounds(l1.removeFromLeft(cw)); l1.removeFromLeft(8);
        c("wvRole")->setBounds(l1);
        c("wvVoicing")->setBounds(l2.removeFromLeft(cw)); l2.removeFromLeft(8);
        c("wvBass")->setBounds(l2.removeFromLeft(cw)); l2.removeFromLeft(8);
        c("wvHold")->setBounds(l2.withTrimmedTop(15).withSizeKeepingCentre(l2.getWidth(), 24));
    }
    head(expr, "23", "Expression");
    {
        const int lineH = std::min(44, expr.getHeight() / 3 - 2);
        c("wvLevelTo")->setBounds(expr.removeFromTop(lineH)); expr.removeFromTop(4);
        c("wvBrightTo")->setBounds(expr.removeFromTop(lineH)); expr.removeFromTop(6);
        c("wvMidiOut")->setBounds(expr.removeFromTop(24));
    }
}

// ---- Standalone app: the microphone ------------------------------------------------------------------------
void waveInputCheck(InfraProcessor& p, bool& wasOn) {
    const bool on = p.apvts.getRawParameterValue("wvOn")->load() > 0.5f;
#if JucePlugin_Build_Standalone
    // JUCE's standalone app mutes its audio input by default (feedback safety). WAVE needs it: switching WAVE on
    // unmutes it once (the app's Options still has the switch; INFRA never sends the input to the output unless
    // Monitor is up).
    if (on && !wasOn && p.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
        if (auto* holder = juce::StandalonePluginHolder::getInstance())
            holder->getMuteInputValue().setValue(false);
#endif
    wasOn = on;
}

}  // namespace panels
