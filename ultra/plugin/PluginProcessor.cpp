#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

using namespace ultra;
using juce::ParameterID;

namespace {
constexpr int kV = 1;   // parameter version hint

juce::StringArray names(const char* const* list, int n) { juce::StringArray s; for (int i = 0; i < n; ++i) s.add(list[i]); return s; }
template <size_t N> juce::StringArray names(const char* const (&list)[N]) { return names(list, (int)N); }

// Logarithmic range: equal knob travel per octave.
juce::NormalisableRange<float> logRange(float lo, float hi) {
    return {lo, hi,
            [](float s, float e, float t) { return s * std::pow(e / s, t); },
            [](float s, float e, float v) { return std::log(v / s) / std::log(e / s); },
            [](float s, float e, float v) { return juce::jlimit(s, e, v); }};
}
juce::String hz(float v, int) { return v >= 1000.f ? juce::String(v / 1000.f, v >= 10000.f ? 1 : 2) + " kHz" : juce::String(v, v < 10.f ? 2 : v < 100.f ? 1 : 0) + " Hz"; }
juce::String pct(float v, int) { return juce::String(juce::roundToInt(v * 100.f)) + "%"; }
juce::String bip(float v, int) { const float pc = v * 100.f; return (v > 0 ? "+" : "") + juce::String(juce::roundToInt(pc)) + "%"; }
juce::String db(float v, int) { return v <= -59.9f ? juce::String("-inf dB") : juce::String(v, 1) + " dB"; }
juce::String ms(float v, int) { return juce::String(v, v < 10.f ? 1 : 0) + " ms"; }
juce::String st(float v, int) { return (v > 0 ? "+" : "") + juce::String(v, std::abs(v - std::round(v)) < 0.05f ? 0 : 1) + " st"; }

// The Init kit (PRD Section 9): one of each role, the GM drum notes.
struct TrackDefault { Model model; float tune, decay, tone, snap, color, sweep, level; int note, choke; };
const TrackDefault kInit[kTracks] = {
    {Model::CoreKick, 0.f, 0.45f, 0.5f, 0.5f, 0.25f, 0.45f, 0.f, 36, 0},
    {Model::FlareSnare, 0.f, 0.4f, 0.5f, 0.5f, 0.55f, 0.4f, -1.f, 38, 0},
    {Model::BurstClap, 0.f, 0.45f, 0.5f, 0.5f, 0.6f, 0.4f, -2.f, 39, 0},
    {Model::IonHat, 0.f, 0.2f, 0.55f, 0.5f, 0.3f, 0.2f, -5.f, 42, 1},
    {Model::IonHat, 0.f, 0.62f, 0.5f, 0.4f, 0.35f, 0.3f, -6.f, 46, 1},
    {Model::OrbitTom, -5.f, 0.45f, 0.4f, 0.4f, 0.3f, 0.45f, -3.f, 45, 0},
    {Model::OrbitTom, 4.f, 0.4f, 0.4f, 0.4f, 0.3f, 0.45f, -3.f, 50, 0},
    {Model::QuasarMetal, 0.f, 0.4f, 0.5f, 0.5f, 0.2f, 0.2f, -7.f, 56, 0},
};
}  // namespace

juce::AudioProcessorValueTreeState::ParameterLayout UltraProcessor::createLayout() {
    using F = juce::AudioParameterFloat; using I = juce::AudioParameterInt; using C = juce::AudioParameterChoice; using B = juce::AudioParameterBool;
    using A = juce::AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto fl = [&](const juce::String& id, const juce::String& name, juce::NormalisableRange<float> r, float def, std::function<juce::String(float, int)> txt = {}) {
        auto attr = A();
        if (txt) attr = attr.withStringFromValueFunction(txt);
        p.push_back(std::make_unique<F>(ParameterID{id, kV}, name, r, def, attr));
    };
    auto unit = [&](const juce::String& id, const juce::String& name, float def) { fl(id, name, {0.f, 1.f, 0.001f}, def, pct); };
    auto in = [&](const juce::String& id, const juce::String& name, int lo, int hi, int def) { p.push_back(std::make_unique<I>(ParameterID{id, kV}, name, lo, hi, def)); };
    auto ch = [&](const juce::String& id, const juce::String& name, juce::StringArray c, int def) { p.push_back(std::make_unique<C>(ParameterID{id, kV}, name, c, def)); };
    auto bo = [&](const juce::String& id, const juce::String& name, bool def) { p.push_back(std::make_unique<B>(ParameterID{id, kV}, name, def)); };

    for (int t = 0; t < kTracks; ++t) {
        const auto& d = kInit[t];
        const juce::String N = "T" + juce::String(t + 1) + " ";
        ch(tid(t, "Model"), N + "Model", names(kModelNames), (int)d.model);
        fl(tid(t, "Tune"), N + "Tune", {-24.f, 24.f, 0.01f}, d.tune, st);
        unit(tid(t, "Decay"), N + "Decay", d.decay);
        unit(tid(t, "Tone"), N + "Tone", d.tone);
        unit(tid(t, "Snap"), N + "Snap", d.snap);
        unit(tid(t, "Color"), N + "Color", d.color);
        unit(tid(t, "Sweep"), N + "Sweep", d.sweep);
        unit(tid(t, "Drive"), N + "Drive", 0.f);
        ch(tid(t, "Filter"), N + "Filter", names(kFilterNames), 0);
        fl(tid(t, "Cutoff"), N + "Cutoff", logRange(20.f, 20000.f), 20000.f, hz);
        unit(tid(t, "Res"), N + "Resonance", 0.1f);
        unit(tid(t, "Vel"), N + "Velocity", 0.7f);
        fl(tid(t, "Level"), N + "Level", {-60.f, 6.f, 0.1f}, d.level, db);
        fl(tid(t, "Pan"), N + "Pan", {-1.f, 1.f, 0.001f}, 0.f, bip);
        unit(tid(t, "SendA"), N + "Echo Send", 0.f);
        unit(tid(t, "SendB"), N + "Reverb Send", t == 2 ? 0.15f : 0.05f);
        bo(tid(t, "Mute"), N + "Mute", false);
        bo(tid(t, "Solo"), N + "Solo", false);
        ch(tid(t, "Choke"), N + "Choke Group", names(kChokeNames), d.choke);
        in(tid(t, "Note"), N + "MIDI Note", 0, 127, d.note);
        ch(tid(t, "Out"), N + "Output", names(kOutputNames), 0);
        ch(tid(t, "Lock"), N + "Lock Target", names(kLockNames), 1);
    }
    ch("pattern", "Pattern", names(kPatternNames), 0);
    bo("fill", "Fill", false);
    fl("swing", "Swing", {0.5f, 0.75f, 0.001f}, 0.5f, pct);
    fl("tempo", "Tempo", {40.f, 240.f, 0.1f}, 120.f, [](float v, int) { return juce::String(v, 1) + " bpm"; });
    for (int m = 0; m < kNumMacros; ++m) unit("macro" + juce::String(m + 1), kMacroNames[m], 0.f);

    ch("drvMode", "Drive Mode", names(kDriveModeNames), 0);
    unit("drvAmt", "Drive", 0.f); unit("drvTone", "Drive Tone", 0.7f); unit("drvMix", "Drive Mix", 1.f);
    fl("glThr", "Glue Threshold", {-40.f, 0.f, 0.1f}, -12.f, db);
    { juce::NormalisableRange<float> r(1.f, 10.f, 0.01f); r.setSkewForCentre(3.f); fl("glRatio", "Glue Ratio", r, 2.f, [](float v, int) { return juce::String(v, 1) + ":1"; }); }
    { juce::NormalisableRange<float> r(0.1f, 30.f, 0.01f); r.setSkewForCentre(5.f); fl("glAtk", "Glue Attack", r, 10.f, ms); }
    { juce::NormalisableRange<float> r(50.f, 1200.f, 1.f); r.setSkewForCentre(250.f); fl("glRel", "Glue Release", r, 200.f, ms); }
    fl("glMakeup", "Glue Makeup", {0.f, 18.f, 0.1f}, 0.f, db);
    unit("glMix", "Glue Mix", 0.f);
    bo("dlySync", "Echo Sync", true);
    ch("dlyDiv", "Echo Division", names(kDivisionNames), 9);
    { juce::NormalisableRange<float> r(1.f, 2000.f, 0.1f); r.setSkewForCentre(300.f); fl("dlyMs", "Echo Time", r, 300.f, ms); }
    fl("dlyFb", "Echo Feedback", {0.f, 0.95f, 0.001f}, 0.35f, pct);
    unit("dlyDamp", "Echo Damping", 0.4f);
    unit("dlyRet", "Echo Return", 0.8f);
    unit("dlyToRv", "Echo to Reverb", 0.f);
    bo("dlyPing", "Echo Ping-Pong", true);
    { juce::NormalisableRange<float> r(0.5f, 3.f, 0.001f); r.setSkewForCentre(1.2f); fl("rvSize", "Reverb Size", r, 1.f); }
    { juce::NormalisableRange<float> r(0.2f, 30.f, 0.01f); r.setSkewForCentre(3.f); fl("rvDecay", "Reverb Decay", r, 1.6f, [](float v, int) { return juce::String(v, 2) + " s"; }); }
    unit("rvDamp", "Reverb Damping", 0.4f);
    { juce::NormalisableRange<float> r(0.f, 250.f, 0.1f); r.setSkewForCentre(40.f); fl("rvPre", "Reverb Pre-delay", r, 8.f, ms); }
    unit("rvRet", "Reverb Return", 0.8f);
    fl("eqLow", "Low Shelf", {-12.f, 12.f, 0.1f}, 0.f, db);
    fl("eqHigh", "High Shelf", {-12.f, 12.f, 0.1f}, 0.f, db);
    fl("width", "Stereo Width", {0.f, 2.f, 0.001f}, 1.f, pct);
    fl("outDb", "Output", {-30.f, 12.f, 0.1f}, 0.f, db);
    return {p.begin(), p.end()};
}

// ---- Parameter cache: pointers looked up once, so the audio thread never builds an id string ------------
struct UltraProcessor::Raw {
    using P = std::atomic<float>*;
    struct Track { P model, tune, decay, tone, snap, color, sweep, drive, filter, cutoff, res, vel, level, pan, sendA, sendB, mute, solo, choke, note, out, lock; } t[kTracks];
    P pattern, fill, swing, tempo, macro[kNumMacros];
    P drvMode, drvAmt, drvTone, drvMix, glThr, glRatio, glAtk, glRel, glMakeup, glMix;
    P dlySync, dlyDiv, dlyMs, dlyFb, dlyDamp, dlyRet, dlyToRv, dlyPing, rvSize, rvDecay, rvDamp, rvPre, rvRet, eqLow, eqHigh, width, outDb;

    explicit Raw(juce::AudioProcessorValueTreeState& s) {
        auto g = [&](const juce::String& id) { auto* v = s.getRawParameterValue(id); jassert(v != nullptr); return v; };
        for (int i = 0; i < kTracks; ++i) {
            auto x = [&](const char* n) { return g(tid(i, n)); };
            t[i] = {x("Model"), x("Tune"), x("Decay"), x("Tone"), x("Snap"), x("Color"), x("Sweep"), x("Drive"), x("Filter"), x("Cutoff"), x("Res"),
                    x("Vel"), x("Level"), x("Pan"), x("SendA"), x("SendB"), x("Mute"), x("Solo"), x("Choke"), x("Note"), x("Out"), x("Lock")};
        }
        pattern = g("pattern"); fill = g("fill"); swing = g("swing"); tempo = g("tempo");
        for (int m = 0; m < kNumMacros; ++m) macro[m] = g("macro" + juce::String(m + 1));
        drvMode = g("drvMode"); drvAmt = g("drvAmt"); drvTone = g("drvTone"); drvMix = g("drvMix");
        glThr = g("glThr"); glRatio = g("glRatio"); glAtk = g("glAtk"); glRel = g("glRel"); glMakeup = g("glMakeup"); glMix = g("glMix");
        dlySync = g("dlySync"); dlyDiv = g("dlyDiv"); dlyMs = g("dlyMs"); dlyFb = g("dlyFb"); dlyDamp = g("dlyDamp"); dlyRet = g("dlyRet");
        dlyToRv = g("dlyToRv"); dlyPing = g("dlyPing");
        rvSize = g("rvSize"); rvDecay = g("rvDecay"); rvDamp = g("rvDamp"); rvPre = g("rvPre"); rvRet = g("rvRet");
        eqLow = g("eqLow"); eqHigh = g("eqHigh"); width = g("width"); outDb = g("outDb");
    }
};

namespace {
inline float F(std::atomic<float>* p) { return p->load(std::memory_order_relaxed); }
inline int I(std::atomic<float>* p) { return (int)std::lround(F(p)); }
inline bool B(std::atomic<float>* p) { return F(p) > 0.5f; }
}  // namespace

KitParams UltraProcessor::currentParams() const {
    const Raw& r = *raw_;
    KitParams k;
    for (int i = 0; i < kTracks; ++i) {
        const auto& q = r.t[i];
        auto& tp = k.tracks[(size_t)i];
        tp.v.model = (Model)juce::jlimit(0, kNumModels - 1, I(q.model));
        tp.v.tune = F(q.tune); tp.v.decay = F(q.decay); tp.v.tone = F(q.tone); tp.v.snap = F(q.snap);
        tp.v.color = F(q.color); tp.v.sweep = F(q.sweep); tp.v.drive = F(q.drive);
        tp.v.filter = (FilterType)juce::jlimit(0, kNumFilterTypes - 1, I(q.filter));
        tp.v.cutoff = F(q.cutoff); tp.v.resonance = F(q.res); tp.v.velSens = F(q.vel);
        tp.levelDb = F(q.level); tp.pan = F(q.pan); tp.sendA = F(q.sendA); tp.sendB = F(q.sendB);
        tp.mute = B(q.mute); tp.solo = B(q.solo); tp.choke = I(q.choke); tp.note = I(q.note); tp.output = I(q.out);
        tp.lock = (LockTarget)juce::jlimit(0, kNumLockTargets - 1, I(q.lock));
    }
    k.pattern = I(r.pattern); k.fill = B(r.fill); k.swing = F(r.swing); k.tempo = F(r.tempo);
    for (int m = 0; m < kNumMacros; ++m) k.macro[(size_t)m] = F(r.macro[m]);
    auto& m = k.master;
    m.driveMode = (DriveMode)I(r.drvMode); m.drive = F(r.drvAmt); m.driveTone = F(r.drvTone); m.driveMix = F(r.drvMix);
    m.glueThresh = F(r.glThr); m.glueRatio = F(r.glRatio); m.glueAttack = F(r.glAtk); m.glueRelease = F(r.glRel);
    m.glueMakeup = F(r.glMakeup); m.glueMix = F(r.glMix);
    m.echoSync = B(r.dlySync); m.echoDivision = I(r.dlyDiv); m.echoMs = F(r.dlyMs); m.echoFeedback = F(r.dlyFb);
    m.echoDamp = F(r.dlyDamp); m.echoReturn = F(r.dlyRet); m.echoToReverb = F(r.dlyToRv); m.echoPingPong = B(r.dlyPing);
    m.reverbSize = F(r.rvSize); m.reverbDecay = F(r.rvDecay); m.reverbDamp = F(r.rvDamp); m.reverbPreDelay = F(r.rvPre); m.reverbReturn = F(r.rvRet);
    m.lowShelf = F(r.eqLow); m.highShelf = F(r.eqHigh); m.width = F(r.width); m.outDb = F(r.outDb);
    return k;
}

// ---- Processor ----------------------------------------------------------------------------------------
juce::AudioProcessor::BusesProperties UltraProcessor::makeBuses() {
    auto b = BusesProperties().withOutput("Main", juce::AudioChannelSet::stereo(), true);
    for (int i = 1; i <= kNumAux; ++i) b = b.withOutput("Aux " + juce::String(i), juce::AudioChannelSet::stereo(), false);
    return b;
}

UltraProcessor::UltraProcessor()
    : AudioProcessor(makeBuses()), apvts(*this, &undoManager, "ULTRA", createLayout()), patterns(apvts, &undoManager), presets(*this) {
    raw_ = std::make_unique<Raw>(apvts);
    events_.reserve(4096);
    for (auto& p : pads_) p.store(0.f);
    machine.setParams(currentParams());
    undoManager.clearUndoHistory();
}

UltraProcessor::~UltraProcessor() { machine.release(); }

bool UltraProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto main = layouts.getMainOutputChannelSet();
    if (main != juce::AudioChannelSet::stereo() && main != juce::AudioChannelSet::mono()) return false;
    if (!layouts.inputBuses.isEmpty() && !layouts.getMainInputChannelSet().isDisabled()) return false;
    for (int i = 1; i < layouts.outputBuses.size(); ++i) {
        const auto& s = layouts.outputBuses.getReference(i);
        if (!s.isDisabled() && s != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}

void UltraProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    machine.setParams(currentParams());
    machine.setPatterns(patterns.acquireForAudio());
    machine.prepare(sampleRate, std::max(1, samplesPerBlock));
    monoScratch_.setSize(2, std::max(32, samplesPerBlock));
}

void UltraProcessor::releaseResources() { machine.release(); }

void UltraProcessor::playPad(int track, float velocity) {
    if (track >= 0 && track < kTracks) pads_[(size_t)track].store(juce::jlimit(0.01f, 1.f, velocity));
}

void UltraProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0) return;

    events_.clear();
    for (const auto meta : midi) {
        if (events_.size() >= events_.capacity()) break;          // never allocate on the audio thread
        const auto m = meta.getMessage();
        MidiEvent e;
        e.offset = juce::jlimit(0, n - 1, meta.samplePosition);
        if (m.isNoteOn()) { e.type = MidiEvent::NoteOn; e.number = m.getNoteNumber(); e.value = m.getFloatVelocity(); }
        else if (m.isNoteOff()) { e.type = MidiEvent::NoteOff; e.number = m.getNoteNumber(); }
        else if (m.isAllNotesOff() || m.isAllSoundOff()) e.type = MidiEvent::AllNotesOff;
        else continue;
        events_.push_back(e);
    }

    Transport tr;
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            if (auto bpm = pos->getBpm()) { tr.bpm = *bpm; tr.hasTempo = true; }
            tr.playing = pos->getIsPlaying();
            if (auto ppq = pos->getPpqPosition()) tr.ppq = *ppq; else tr.playing = false;
        }
    }
    hostBpm.store(tr.hasTempo ? tr.bpm : (double)raw_->tempo->load(), std::memory_order_relaxed);
    hostPlaying.store(tr.playing, std::memory_order_relaxed);

    machine.setParams(currentParams());
    machine.setPatterns(patterns.acquireForAudio());
    for (int t = 0; t < kTracks; ++t)
        if (const float v = pads_[(size_t)t].exchange(0.f); v > 0.f) machine.trigger(t, v);

    Outputs out;
    auto mainBus = getBusBuffer(buffer, false, 0);
    for (int k = 0; k < kNumAux; ++k) {
        if (k + 1 >= getBusCount(false)) break;
        auto* bus = getBus(false, k + 1);
        if (bus == nullptr || !bus->isEnabled() || bus->getNumberOfChannels() < 2) continue;
        auto aux = getBusBuffer(buffer, false, k + 1);
        out.auxL[(size_t)k] = aux.getWritePointer(0); out.auxR[(size_t)k] = aux.getWritePointer(1);
    }
    if (mainBus.getNumChannels() >= 2) {
        out.L = mainBus.getWritePointer(0); out.R = mainBus.getWritePointer(1);
        machine.process(out, n, events_.data(), (int)events_.size(), tr);
    } else if (mainBus.getNumChannels() == 1) {
        // Mono main output: render stereo into the scratch buffer chunk by chunk, then sum.
        const int cap = monoScratch_.getNumSamples();
        float* M = mainBus.getWritePointer(0);
        size_t ev = 0;
        for (int off = 0; off < n; off += cap) {
            const int c = std::min(cap, n - off);
            size_t first = ev;
            while (ev < events_.size() && events_[ev].offset < off + c) { events_[ev].offset -= off; ++ev; }
            Outputs o = out;
            o.L = monoScratch_.getWritePointer(0); o.R = monoScratch_.getWritePointer(1);
            for (int k = 0; k < kNumAux; ++k) { if (o.auxL[(size_t)k]) o.auxL[(size_t)k] += off; if (o.auxR[(size_t)k]) o.auxR[(size_t)k] += off; }
            machine.process(o, c, events_.data() + first, (int)(ev - first), tr);
            for (int i = 0; i < c; ++i) M[off + i] = 0.5f * (o.L[i] + o.R[i]);
            if (tr.playing) tr.ppq += c * tr.bpm / 60.0 / getSampleRate();
        }
    }
    midi.clear();
}

juce::AudioProcessorEditor* UltraProcessor::createEditor() { return new UltraEditor(*this); }

void UltraProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    state.setProperty("presetName", presets.currentName(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void UltraProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        if (xml->hasTagName(apvts.state.getType())) {
            auto tree = juce::ValueTree::fromXml(*xml);
            const juce::String name = tree.getProperty("presetName").toString();
            apvts.replaceState(tree);
            presets.setCurrentName(name);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new UltraProcessor(); }
