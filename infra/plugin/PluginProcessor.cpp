#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

using namespace infra;
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
juce::String secs(float v, int) { return v < 1.f ? juce::String(v * 1000.f, v < 0.01f ? 1 : 0) + " ms" : juce::String(v, 2) + " s"; }
juce::String pct(float v, int) { return juce::String(juce::roundToInt(v * 100.f)) + "%"; }
juce::String bip(float v, int) { const float pc = v * 100.f; return (v > 0 ? "+" : "") + (std::abs(pc) < 10.f && std::abs(pc) > 0.05f ? juce::String(pc, 1) : juce::String(juce::roundToInt(pc))) + "%"; }
}  // namespace

juce::AudioProcessorValueTreeState::ParameterLayout InfraProcessor::createLayout() {
    using F = juce::AudioParameterFloat; using I = juce::AudioParameterInt; using C = juce::AudioParameterChoice; using B = juce::AudioParameterBool;
    using A = juce::AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto fl = [&](const juce::String& id, const juce::String& name, juce::NormalisableRange<float> r, float def, std::function<juce::String(float, int)> txt = {}, const juce::String& label = {}) {
        auto attr = A();
        if (txt) attr = attr.withStringFromValueFunction(txt);
        if (label.isNotEmpty()) attr = attr.withLabel(label);
        p.push_back(std::make_unique<F>(ParameterID{id, kV}, name, r, def, attr));
    };
    auto unit = [&](const juce::String& id, const juce::String& name, float def) { fl(id, name, {0.f, 1.f, 0.001f}, def, pct); };
    auto in = [&](const juce::String& id, const juce::String& name, int lo, int hi, int def) { p.push_back(std::make_unique<I>(ParameterID{id, kV}, name, lo, hi, def)); };
    auto ch = [&](const juce::String& id, const juce::String& name, juce::StringArray c, int def) { p.push_back(std::make_unique<C>(ParameterID{id, kV}, name, c, def)); };
    auto bo = [&](const juce::String& id, const juce::String& name, bool def) { p.push_back(std::make_unique<B>(ParameterID{id, kV}, name, def)); };
    auto time = [&](const juce::String& id, const juce::String& name, float lo, float hi, float def) {
        juce::NormalisableRange<float> r(lo, hi, 0.0001f); r.setSkewForCentre(hi > 10.f ? 0.8f : 0.3f);
        fl(id, name, r, def, secs);
    };

    // Oscillators.
    for (int o = 0; o < 2; ++o) {
        const juce::String x = o == 0 ? "a" : "b", N = o == 0 ? "Osc A " : "Osc B ";
        bo(x + "On", N + "On", o == 0);
        ch(x + "Table", N + "Table", names(kTableNames), 0);
        unit(x + "Morph", N + "Morph", 0.5f);
        in(x + "Oct", N + "Octave", -3, 3, 0);
        in(x + "Semi", N + "Semitone", -12, 12, 0);
        fl(x + "Fine", N + "Fine", {-100.f, 100.f, 0.1f}, 0.f, [](float v, int) { return juce::String(v, 1) + " ct"; });
        unit(x + "Level", N + "Level", o == 0 ? 0.8f : 0.6f);
        fl(x + "Pan", N + "Pan", {-1.f, 1.f, 0.001f}, 0.f, bip);
        in(x + "Unison", N + "Unison", 1, kMaxUnison, 1);
        unit(x + "Detune", N + "Detune", 0.2f);
        unit(x + "Width", N + "Width", 0.5f);
        ch(x + "Warp", N + "Warp", names(kWarpNames), 0);
        unit(x + "WarpAmt", N + "Warp Amount", 0.f);
        unit(x + "Phase", N + "Phase", 0.f);
        unit(x + "PhaseRand", N + "Phase Random", 1.f);
    }
    ch("subShape", "Sub Shape", names(kSubShapeNames), 0);
    in("subOct", "Sub Octave", 1, 2, 1);
    unit("subLevel", "Sub Level", 0.f);
    ch("noiseType", "Noise Colour", names(kNoiseNames), 0);
    unit("noiseLevel", "Noise Level", 0.f);

    // Filter.
    ch("fType", "Filter Type", names(kFilterNames), 0);
    fl("fCutoff", "Cutoff", logRange(20.f, 20000.f), 9000.f, hz);
    unit("fRes", "Resonance", 0.1f);
    unit("fDrive", "Filter Drive", 0.f);
    fl("fEnv", "Filter Env Amount", {-72.f, 72.f, 0.1f}, 0.f, [](float v, int) { return (v > 0 ? "+" : "") + juce::String(v, 1) + " st"; });
    unit("fKey", "Key Track", 0.5f);
    unit("fVel", "Velocity to Cutoff", 0.f);

    // Envelopes.
    time("ampA", "Amp Attack", 0.f, 20.f, 0.005f); time("ampD", "Amp Decay", 0.001f, 20.f, 0.4f);
    unit("ampS", "Amp Sustain", 0.8f); time("ampR", "Amp Release", 0.001f, 20.f, 0.3f);
    fl("ampCurve", "Amp Attack Curve", {-1.f, 1.f, 0.001f}, 0.f, bip);
    unit("velSens", "Velocity Sensitivity", 0.6f);
    time("fenvA", "Filter Env Attack", 0.f, 20.f, 0.005f); time("fenvD", "Filter Env Decay", 0.001f, 20.f, 0.4f);
    unit("fenvS", "Filter Env Sustain", 0.3f); time("fenvR", "Filter Env Release", 0.001f, 20.f, 0.4f);
    time("menvA", "Mod Env Attack", 0.f, 20.f, 0.2f); time("menvD", "Mod Env Decay", 0.001f, 20.f, 0.6f);
    unit("menvS", "Mod Env Sustain", 0.f); time("menvR", "Mod Env Release", 0.001f, 20.f, 0.5f);

    // LFOs.
    for (int i = 0; i < 2; ++i) {
        const juce::String x = "l" + juce::String(i + 1), N = "LFO " + juce::String(i + 1) + " ";
        ch(x + "Shape", N + "Shape", names(kLfoShapeNames), i == 0 ? 0 : 1);
        fl(x + "Rate", N + "Rate", logRange(0.01f, 40.f), i == 0 ? 2.f : 0.3f, hz);
        bo(x + "Sync", N + "Sync", false);
        ch(x + "Div", N + "Division", names(kDivisionNames), i == 0 ? 10 : 3);
        bo(x + "Retrig", N + "Retrigger", true);
        time(x + "Fade", N + "Fade In", 0.f, 10.f, 0.f);
    }
    // Modulation matrix.
    for (int s = 0; s < kNumModSlots; ++s) {
        const juce::String x = "m" + juce::String(s + 1), N = "Mod " + juce::String(s + 1) + " ";
        ch(x + "Src", N + "Source", names(kModSourceNames), 0);
        ch(x + "Dst", N + "Destination", names(kModDestNames), 0);
        fl(x + "Amt", N + "Amount", {-1.f, 1.f, 0.001f}, 0.f, bip);
    }
    // Macros.
    static const char* macroNames[4] = {"Heat", "Gravity", "Orbit", "Flux"};
    for (int m = 0; m < kNumMacros; ++m) unit("macro" + juce::String(m + 1), macroNames[m], 0.f);

    // Voices.
    ch("voiceMode", "Voice Mode", names(kVoiceModeNames), 0);
    in("polyphony", "Polyphony", 1, kMaxVoices, 12);
    { juce::NormalisableRange<float> r(0.f, 2.f, 0.0001f); r.setSkewForCentre(0.25f); fl("glide", "Glide", r, 0.f, secs); }
    bo("glideLegato", "Glide Legato Only", false);
    in("bendRange", "Pitch Bend Range", 0, 24, 2);
    unit("drift", "Analog Drift", 0.2f);
    fl("tune", "Master Tune", {-100.f, 100.f, 0.1f}, 0.f, [](float v, int) { return juce::String(v, 1) + " ct"; });

    // Arpeggiator.
    bo("arpOn", "Arp On", false);
    ch("arpMode", "Arp Mode", names(kArpModeNames), 0);
    ch("arpDiv", "Arp Rate", names(kDivisionNames), 13);
    in("arpOct", "Arp Octaves", 1, 4, 1);
    fl("arpGate", "Arp Gate", {0.05f, 2.f, 0.001f}, 0.6f, pct);
    unit("arpSwing", "Arp Swing", 0.f);
    bo("arpLatch", "Arp Latch", false);
    in("arpLength", "Arp Length", 1, kArpSteps, 16);
    unit("arpProb", "Arp Probability", 1.f);
    for (int s = 0; s < kArpSteps; ++s) {
        const juce::String x = "s" + juce::String(s + 1), N = "Step " + juce::String(s + 1) + " ";
        unit(x + "Vel", N + "Velocity", 1.f);
        in(x + "Tr", N + "Transpose", -12, 12, 0);
        in(x + "Rat", N + "Ratchet", 1, 4, 1);
    }

    // Effects.
    ch("drvMode", "Drive Mode", names(kDriveModeNames), 0);
    unit("drvAmt", "Drive", 0.f); unit("drvTone", "Drive Tone", 0.7f); unit("drvMix", "Drive Mix", 1.f);
    fl("chRate", "Aurora Rate", logRange(0.02f, 8.f), 0.4f, hz);
    unit("chDepth", "Aurora Depth", 0.5f); unit("chMix", "Aurora Mix", 0.f);
    bo("dlySync", "Echo Sync", true);
    ch("dlyDiv", "Echo Division", names(kDivisionNames), 10);
    { juce::NormalisableRange<float> r(1.f, 2000.f, 0.1f); r.setSkewForCentre(300.f); fl("dlyMs", "Echo Time", r, 375.f, [](float v, int) { return juce::String(v, 0) + " ms"; }); }
    fl("dlyFb", "Echo Feedback", {0.f, 0.95f, 0.001f}, 0.35f, pct);
    unit("dlyDamp", "Echo Damping", 0.4f); unit("dlyMix", "Echo Mix", 0.f);
    bo("dlyPing", "Echo Ping-Pong", true);
    { juce::NormalisableRange<float> r(0.5f, 3.f, 0.001f); r.setSkewForCentre(1.2f); fl("rvSize", "Reverb Size", r, 1.f); }
    { juce::NormalisableRange<float> r(0.2f, 30.f, 0.01f); r.setSkewForCentre(3.f); fl("rvDecay", "Reverb Decay", r, 2.5f, secs); }
    unit("rvDamp", "Reverb Damping", 0.4f);
    { juce::NormalisableRange<float> r(0.f, 250.f, 0.1f); r.setSkewForCentre(40.f); fl("rvPre", "Reverb Pre-delay", r, 10.f, [](float v, int) { return juce::String(v, 0) + " ms"; }); }
    unit("rvMix", "Reverb Mix", 0.15f);
    fl("width", "Stereo Width", {0.f, 2.f, 0.001f}, 1.f, pct);
    fl("outDb", "Output", {-30.f, 18.f, 0.1f}, 0.f, [](float v, int) { return juce::String(v, 1) + " dB"; });
    return {p.begin(), p.end()};
}

// ---- Parameter cache: pointers looked up once, so the audio thread never builds an id string ------------
struct InfraProcessor::Raw {
    using P = std::atomic<float>*;
    struct Osc { P on, table, morph, oct, semi, fine, level, pan, unison, detune, width, warp, warpAmt, phase, phaseRand; } osc[2];
    P subShape, subOct, subLevel, noiseType, noiseLevel, fType, fCutoff, fRes, fDrive, fEnv, fKey, fVel;
    P amp[4], ampCurve, velSens, fenv[4], menv[4];
    struct Lfo { P shape, rate, sync, div, retrig, fade; } lfo[2];
    struct Slot { P src, dst, amt; } mod[kNumModSlots];
    P macro[kNumMacros];
    P voiceMode, polyphony, glide, glideLegato, bendRange, drift, tune;
    P arpOn, arpMode, arpDiv, arpOct, arpGate, arpSwing, arpLatch, arpLength, arpProb;
    struct Step { P vel, tr, rat; } step[kArpSteps];
    P drvMode, drvAmt, drvTone, drvMix, chRate, chDepth, chMix, dlySync, dlyDiv, dlyMs, dlyFb, dlyDamp, dlyMix, dlyPing;
    P rvSize, rvDecay, rvDamp, rvPre, rvMix, width, outDb;

    explicit Raw(juce::AudioProcessorValueTreeState& s) {
        auto g = [&](const juce::String& id) { auto* v = s.getRawParameterValue(id); jassert(v != nullptr); return v; };
        for (int o = 0; o < 2; ++o) {
            const juce::String x = o == 0 ? "a" : "b";
            auto& q = osc[o];
            q = {g(x + "On"), g(x + "Table"), g(x + "Morph"), g(x + "Oct"), g(x + "Semi"), g(x + "Fine"), g(x + "Level"), g(x + "Pan"),
                 g(x + "Unison"), g(x + "Detune"), g(x + "Width"), g(x + "Warp"), g(x + "WarpAmt"), g(x + "Phase"), g(x + "PhaseRand")};
        }
        subShape = g("subShape"); subOct = g("subOct"); subLevel = g("subLevel"); noiseType = g("noiseType"); noiseLevel = g("noiseLevel");
        fType = g("fType"); fCutoff = g("fCutoff"); fRes = g("fRes"); fDrive = g("fDrive"); fEnv = g("fEnv"); fKey = g("fKey"); fVel = g("fVel");
        const char* st[4] = {"A", "D", "S", "R"};
        for (int i = 0; i < 4; ++i) { amp[i] = g(juce::String("amp") + st[i]); fenv[i] = g(juce::String("fenv") + st[i]); menv[i] = g(juce::String("menv") + st[i]); }
        ampCurve = g("ampCurve"); velSens = g("velSens");
        for (int i = 0; i < 2; ++i) {
            const juce::String x = "l" + juce::String(i + 1);
            lfo[i] = {g(x + "Shape"), g(x + "Rate"), g(x + "Sync"), g(x + "Div"), g(x + "Retrig"), g(x + "Fade")};
        }
        for (int i = 0; i < kNumModSlots; ++i) { const juce::String x = "m" + juce::String(i + 1); mod[i] = {g(x + "Src"), g(x + "Dst"), g(x + "Amt")}; }
        for (int i = 0; i < kNumMacros; ++i) macro[i] = g("macro" + juce::String(i + 1));
        voiceMode = g("voiceMode"); polyphony = g("polyphony"); glide = g("glide"); glideLegato = g("glideLegato");
        bendRange = g("bendRange"); drift = g("drift"); tune = g("tune");
        arpOn = g("arpOn"); arpMode = g("arpMode"); arpDiv = g("arpDiv"); arpOct = g("arpOct"); arpGate = g("arpGate");
        arpSwing = g("arpSwing"); arpLatch = g("arpLatch"); arpLength = g("arpLength"); arpProb = g("arpProb");
        for (int i = 0; i < kArpSteps; ++i) { const juce::String x = "s" + juce::String(i + 1); step[i] = {g(x + "Vel"), g(x + "Tr"), g(x + "Rat")}; }
        drvMode = g("drvMode"); drvAmt = g("drvAmt"); drvTone = g("drvTone"); drvMix = g("drvMix");
        chRate = g("chRate"); chDepth = g("chDepth"); chMix = g("chMix");
        dlySync = g("dlySync"); dlyDiv = g("dlyDiv"); dlyMs = g("dlyMs"); dlyFb = g("dlyFb"); dlyDamp = g("dlyDamp"); dlyMix = g("dlyMix"); dlyPing = g("dlyPing");
        rvSize = g("rvSize"); rvDecay = g("rvDecay"); rvDamp = g("rvDamp"); rvPre = g("rvPre"); rvMix = g("rvMix");
        width = g("width"); outDb = g("outDb");
    }
};

namespace {
inline float F(std::atomic<float>* p) { return p->load(std::memory_order_relaxed); }
inline int I(std::atomic<float>* p) { return (int)std::lround(F(p)); }
inline bool B(std::atomic<float>* p) { return F(p) > 0.5f; }
}  // namespace

SynthParams InfraProcessor::currentParams() const {
    const Raw& r = *raw_;
    SynthParams p;
    for (int o = 0; o < 2; ++o) {
        const auto& q = r.osc[o];
        auto& op = p.osc[(size_t)o];
        op.on = B(q.on); op.table = I(q.table); op.morph = F(q.morph);
        op.octave = I(q.oct); op.semi = I(q.semi); op.fine = F(q.fine);
        op.level = F(q.level); op.pan = F(q.pan); op.unison = I(q.unison);
        op.detune = F(q.detune); op.width = F(q.width);
        op.warp = (Warp)I(q.warp); op.warpAmt = F(q.warpAmt);
        op.phase = F(q.phase); op.phaseRand = F(q.phaseRand);
    }
    p.subShape = (SubShape)I(r.subShape); p.subOctave = I(r.subOct); p.subLevel = F(r.subLevel);
    p.noiseColour = (NoiseColour)I(r.noiseType); p.noiseLevel = F(r.noiseLevel);
    p.filterType = (FilterType)I(r.fType); p.cutoff = F(r.fCutoff); p.resonance = F(r.fRes); p.filterDrive = F(r.fDrive);
    p.filterEnvAmt = F(r.fEnv); p.keyTrack = F(r.fKey); p.filterVel = F(r.fVel);
    p.ampEnv = {F(r.amp[0]), F(r.amp[1]), F(r.amp[2]), F(r.amp[3])};
    p.ampCurve = F(r.ampCurve); p.velSens = F(r.velSens);
    p.filterEnv = {F(r.fenv[0]), F(r.fenv[1]), F(r.fenv[2]), F(r.fenv[3])};
    p.modEnv = {F(r.menv[0]), F(r.menv[1]), F(r.menv[2]), F(r.menv[3])};
    for (int i = 0; i < 2; ++i) {
        const auto& q = r.lfo[i];
        auto& l = p.lfo[(size_t)i];
        l.shape = (LfoShape)I(q.shape); l.rateHz = F(q.rate); l.sync = B(q.sync);
        l.division = I(q.div); l.retrig = B(q.retrig); l.fade = F(q.fade);
    }
    for (int s = 0; s < kNumModSlots; ++s) p.mod[(size_t)s] = {(ModSource)I(r.mod[s].src), (ModDest)I(r.mod[s].dst), F(r.mod[s].amt)};
    for (int m = 0; m < kNumMacros; ++m) p.macro[(size_t)m] = F(r.macro[m]);
    p.voiceMode = (VoiceMode)I(r.voiceMode); p.polyphony = I(r.polyphony); p.glide = F(r.glide);
    p.glideLegatoOnly = B(r.glideLegato); p.bendRange = I(r.bendRange); p.drift = F(r.drift); p.masterTune = F(r.tune);
    auto& a = p.arp;
    a.on = B(r.arpOn); a.mode = (ArpMode)I(r.arpMode); a.division = I(r.arpDiv); a.octaves = I(r.arpOct);
    a.gate = F(r.arpGate); a.swing = F(r.arpSwing); a.latch = B(r.arpLatch); a.length = I(r.arpLength); a.probability = F(r.arpProb);
    for (int s = 0; s < kArpSteps; ++s) a.steps[(size_t)s] = {F(r.step[s].vel), I(r.step[s].tr), I(r.step[s].rat)};
    auto& f = p.fx;
    f.driveMode = (DriveMode)I(r.drvMode); f.drive = F(r.drvAmt); f.driveTone = F(r.drvTone); f.driveMix = F(r.drvMix);
    f.chorusRate = F(r.chRate); f.chorusDepth = F(r.chDepth); f.chorusMix = F(r.chMix);
    f.delaySync = B(r.dlySync); f.delayDivision = I(r.dlyDiv); f.delayMs = F(r.dlyMs);
    f.delayFeedback = F(r.dlyFb); f.delayDamp = F(r.dlyDamp); f.delayMix = F(r.dlyMix); f.delayPingPong = B(r.dlyPing);
    f.reverbSize = F(r.rvSize); f.reverbDecay = F(r.rvDecay); f.reverbDamp = F(r.rvDamp); f.reverbPreDelay = F(r.rvPre); f.reverbMix = F(r.rvMix);
    f.width = F(r.width); f.outDb = F(r.outDb);
    return p;
}

// ---- Processor ----------------------------------------------------------------------------------------
InfraProcessor::InfraProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, &undoManager, "INFRA", createLayout()), presets(*this) {
    raw_ = std::make_unique<Raw>(apvts);
    events_.reserve(4096);
    keyboardMidi_.ensureSize(4096);
    synth.setParams(currentParams());
}

InfraProcessor::~InfraProcessor() { synth.release(); }

bool InfraProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void InfraProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    synth.setParams(currentParams());
    synth.prepare(sampleRate, std::max(1, samplesPerBlock));
    keyboardState.reset();
}

void InfraProcessor::releaseResources() { synth.release(); }

void InfraProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (n <= 0) return;
    keyboardState.processNextMidiBuffer(midi, 0, n, true);

    events_.clear();
    for (const auto meta : midi) {
        if (events_.size() >= events_.capacity()) break;          // never allocate on the audio thread
        const auto m = meta.getMessage();
        MidiEvent e;
        e.offset = juce::jlimit(0, n - 1, meta.samplePosition);
        if (m.isNoteOn()) { e.type = MidiEvent::NoteOn; e.number = m.getNoteNumber(); e.value = m.getFloatVelocity(); }
        else if (m.isNoteOff()) { e.type = MidiEvent::NoteOff; e.number = m.getNoteNumber(); }
        else if (m.isController()) { e.type = MidiEvent::Controller; e.number = m.getControllerNumber(); e.value = m.getControllerValue() / 127.f; }
        else if (m.isPitchWheel()) { e.type = MidiEvent::PitchBend; e.value = juce::jlimit(-1.f, 1.f, (m.getPitchWheelValue() - 8192) / 8192.f); }
        else if (m.isChannelPressure()) { e.type = MidiEvent::ChannelPressure; e.value = m.getChannelPressureValue() / 127.f; }
        else if (m.isAftertouch()) { e.type = MidiEvent::ChannelPressure; e.value = m.getAfterTouchValue() / 127.f; }
        else if (m.isAllNotesOff() || m.isAllSoundOff()) { e.type = MidiEvent::AllNotesOff; e.number = m.isAllSoundOff() ? 120 : 123; }
        else continue;
        events_.push_back(e);
    }
    // VST3 note expression (per-note tuning, e.g. FL Studio slides): merged in by offset, after any note-on at the
    // same offset. insert() stays within the reserved capacity, so nothing is allocated.
    for (int i = 0; i < noteExpression_.numChanges(); ++i) {
        if (events_.size() >= events_.capacity()) break;
        const auto& c = noteExpression_.changes()[i];
        MidiEvent e; e.type = MidiEvent::NoteTuning; e.offset = juce::jlimit(0, n - 1, c.offset); e.number = c.note; e.value = c.semitones;
        auto at = std::upper_bound(events_.begin(), events_.end(), e.offset, [](int off, const MidiEvent& x) { return off < x.offset; });
        events_.insert(at, e);
    }
    noteExpression_.clear();

    Transport tr;
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            if (auto bpm = pos->getBpm()) tr.bpm = *bpm;
            tr.playing = pos->getIsPlaying();
            if (auto ppq = pos->getPpqPosition()) tr.ppq = *ppq; else tr.playing = false;
        }
    }
    hostBpm.store(tr.bpm, std::memory_order_relaxed);
    hostPlaying.store(tr.playing, std::memory_order_relaxed);

    synth.setParams(currentParams());
    float* L = buffer.getWritePointer(0);
    if (buffer.getNumChannels() > 1) {
        synth.process(L, buffer.getWritePointer(1), n, events_.data(), (int)events_.size(), tr);
    } else {
        // Mono output: render stereo into the scratch half of a stack buffer chunk by chunk.
        float tmp[512];
        for (int off = 0; off < n; off += 512) {
            const int c = std::min(512, n - off);
            // Events for this chunk only.
            int first = 0;
            while (first < (int)events_.size() && events_[(size_t)first].offset < off) ++first;
            int last = first;
            while (last < (int)events_.size() && events_[(size_t)last].offset < off + c) { events_[(size_t)last].offset -= off; ++last; }
            synth.process(L + off, tmp, c, events_.data() + first, last - first, tr);
            for (int i = 0; i < c; ++i) L[off + i] = 0.5f * (L[off + i] + tmp[i]);
            if (tr.playing) tr.ppq += c * tr.bpm / 60.0 / getSampleRate();
        }
    }
    midi.clear();
}

juce::AudioProcessorEditor* InfraProcessor::createEditor() { return new InfraEditor(*this); }

void InfraProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    state.setProperty("presetName", presets.currentName(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void InfraProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        if (xml->hasTagName(apvts.state.getType())) {
            auto tree = juce::ValueTree::fromXml(*xml);
            const juce::String name = tree.getProperty("presetName").toString();
            apvts.replaceState(tree);
            presets.setCurrentName(name);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new InfraProcessor(); }
