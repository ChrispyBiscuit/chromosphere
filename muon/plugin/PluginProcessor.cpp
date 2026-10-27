#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

using namespace muon;
using juce::ParameterID;

const char* const MuonProcessor::kMacroNames[kNumMacros] = {"Velocity", "Mass", "Charge", "Decay"};
const char* const MuonProcessor::kMacroMeanings[kNumMacros] = {"The arrival", "Weight", "Energy", "Aftermath"};

namespace {
constexpr int kV = 1;   // parameter version hint

template <size_t N> juce::StringArray names(const char* const (&list)[N]) { juce::StringArray s; for (auto* x : list) s.add(x); return s; }

juce::NormalisableRange<float> logRange(float lo, float hi) {
    return {lo, hi,
            [](float s, float e, float t) { return s * std::pow(e / s, t); },
            [](float s, float e, float v) { return std::log(v / s) / std::log(e / s); },
            [](float s, float e, float v) { return juce::jlimit(s, e, v); }};
}
juce::String hz(float v, int) { return v >= 1000.f ? juce::String(v / 1000.f, v >= 10000.f ? 1 : 2) + " kHz" : juce::String(v, v < 100.f ? 1 : 0) + " Hz"; }
juce::String secs(float v, int) { return v < 1.f ? juce::String(v * 1000.f, v < 0.1f ? 1 : 0) + " ms" : juce::String(v, 2) + " s"; }
juce::String pct(float v, int) { return juce::String(juce::roundToInt(v * 100.f)) + "%"; }
juce::String bip(float v, int) { const int pc = juce::roundToInt(v * 100.f); return (pc > 0 ? "+" : "") + juce::String(pc) + "%"; }
juce::String db(float v, int) { return (v > 0.f ? "+" : "") + juce::String(v, 1) + " dB"; }
juce::String noteName(int n) { static const char* k[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"}; return juce::String(k[n % 12]) + juce::String(n / 12 - 1); }
}  // namespace

juce::AudioProcessorValueTreeState::ParameterLayout MuonProcessor::createLayout() {
    using F = juce::AudioParameterFloat; using C = juce::AudioParameterChoice; using B = juce::AudioParameterBool;
    using A = juce::AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto fl = [&](const juce::String& id, const juce::String& name, juce::NormalisableRange<float> r, float def, std::function<juce::String(float, int)> txt = {}) {
        auto attr = A();
        if (txt) attr = attr.withStringFromValueFunction(txt);
        p.push_back(std::make_unique<F>(ParameterID{id, kV}, name, r, def, attr));
    };
    auto unit = [&](const juce::String& id, const juce::String& name, float def) { fl(id, name, {0.f, 1.f, 0.001f}, def, pct); };
    auto bi = [&](const juce::String& id, const juce::String& name) { fl(id, name, {-1.f, 1.f, 0.001f}, 0.f, bip); };
    auto ch = [&](const juce::String& id, const juce::String& name, juce::StringArray c, int def) { p.push_back(std::make_unique<C>(ParameterID{id, kV}, name, c, def)); };
    auto bo = [&](const juce::String& id, const juce::String& name, bool def) { p.push_back(std::make_unique<B>(ParameterID{id, kV}, name, def)); };

    // Split.
    ch("bands", "Bands", {"1", "2", "3"}, 2);
    fl("xLow", "Crossover Low", logRange(40.f, 1000.f), 120.f, hz);
    fl("xHigh", "Crossover High", logRange(500.f, 16000.f), 2500.f, hz);
    ch("xType", "Crossover", names(kCrossoverNames), 0);
    ch("stereo", "Stereo Mode", names(kStereoModeNames), 0);
    unit("sens", "Sensitivity", 0.5f);
    ch("look", "Lookahead", names(kLookaheadNames), 0);
    ch("os", "Oversampling", names(kOversampleNames), 2);
    bi("punch", "Punch"); bi("tail", "Tail");
    // Bands.
    for (int b = 1; b <= kMaxBands; ++b) {
        const juce::String x = "b" + juce::String(b), N = "Band " + juce::String(b) + " ";
        bi(x + "Atk", N + "Attack"); bi(x + "Sus", N + "Sustain");
        unit(x + "Spd", N + "Speed", 0.5f); unit(x + "Clip", N + "Clip", 0.f);
        ch(x + "Src", N + "Harmonics Source", names(kHarmSourceNames), 0);
        for (int h = 1; h <= kOrders; ++h) unit(x + "H" + juce::String(h), N + "H" + juce::String(h), h == 1 ? 1.f : 0.f);
        for (int h = 2; h <= kOrders; ++h) bo(x + "Inv" + juce::String(h), N + "H" + juce::String(h) + " Invert", false);
        unit(x + "Harm", N + "Harmonics", 0.f);
        ch(x + "Curve", N + "Drive Curve", names(kDriveCurveNames), 0);
        unit(x + "Drv", N + "Drive", 0.f);
        bi(x + "Bias", N + "Bias");
        unit(x + "Tone", N + "Tone", 0.5f);
        unit(x + "DMix", N + "Drive Mix", 1.f);
        bo(x + "Mute", N + "Mute", false);
    }
    // Sub.
    unit("sub", "Sub", 0.f); unit("sub2", "Sub -2 Oct", 0.f); unit("subPur", "Sub Purity", 0.8f);
    fl("subRange", "Sub Range", logRange(40.f, 400.f), 150.f, hz);
    unit("phantom", "Phantom", 0.f); unit("phCut", "Phantom Cut", 0.f);
    fl("mono", "Mono Below", {0.f, 300.f, 1.f}, 0.f, [](float v, int) { return v < 1.f ? juce::String("Off") : juce::String((int)v) + " Hz"; });
    // Resonance.
    ch("resMode", "Resonance Mode", names(kResModeNames), 0);
    ch("resIn", "Resonance Input", names(kResInputNames), 0);
    ch("pitch", "Pitch Source", names(kPitchSourceNames), 0);
    p.push_back(std::make_unique<juce::AudioParameterInt>(ParameterID{"root", kV}, "Root", 24, 96, 45,
        juce::AudioParameterIntAttributes().withStringFromValueFunction([](int v, int) { return noteName(v); })));
    ch("chord", "Chord", names(kChordNames), 1);
    ch("scale", "Scale", names(kScaleNames), 1);
    ch("material", "Material", names(kMaterialNames), 1);
    unit("size", "Size", 0.5f); unit("spread", "Spread", 0.f); unit("resStereo", "Resonance Stereo", 0.5f);
    { juce::NormalisableRange<float> r(0.01f, 4.f, 0.001f); r.setSkewForCentre(0.5f); fl("resDecay", "Resonance Decay", r, 0.6f, secs); }
    unit("resDamp", "Resonance Damping", 0.4f); unit("resTone", "Resonance Tone", 0.5f);
    unit("strike", "Strike", 0.6f); unit("resMix", "Resonance Mix", 0.f);
    // Afterglow.
    ch("glowSrc", "Afterglow Source", names(kGlowSourceNames), 0);
    unit("glowSend", "Afterglow Send", 0.f);
    { juce::NormalisableRange<float> r(0.5f, 3.f, 0.001f); r.setSkewForCentre(1.2f); fl("glowSize", "Afterglow Size", r, 1.f, [](float v, int) { return juce::String(v, 2) + "x"; }); }
    { juce::NormalisableRange<float> r(0.2f, 20.f, 0.01f); r.setSkewForCentre(2.5f); fl("glowDecay", "Afterglow Decay", r, 2.f, secs); }
    unit("glowDamp", "Afterglow Damping", 0.4f);
    { juce::NormalisableRange<float> r(0.f, 250.f, 0.1f); r.setSkewForCentre(40.f); fl("glowPre", "Afterglow Pre-delay", r, 10.f, [](float v, int) { return juce::String(v, 0) + " ms"; }); }
    unit("glowMix", "Afterglow Return", 0.5f);
    // Output.
    bo("autoGain", "Auto Gain", true);
    unit("mix", "Mix", 1.f);
    fl("outDb", "Output", {-24.f, 24.f, 0.1f}, 0.f, db);
    bo("clipGuard", "Clip Guard", false);
    ch("listen", "Listen", names(kListenNames), 0);
    // Macros.
    for (int m = 0; m < kNumMacros; ++m) unit("macro" + juce::String(m + 1), kMacroNames[m], 0.f);
    return {p.begin(), p.end()};
}

// ---- Slot indices --------------------------------------------------------------------------------------------
struct MuonProcessor::Ids {
    int bands, xLow, xHigh, xType, stereo, sens, look, os, punch, tail;
    struct Band { int atk, sus, spd, clip, src, h[kOrders], inv[kOrders], harm, curve, drv, bias, tone, dmix, mute; } band[kMaxBands];
    int sub, sub2, subPur, subRange, phantom, phCut, mono;
    int resMode, resIn, pitch, root, chord, scale, material, size, spread, resStereo, resDecay, resDamp, resTone, strike, resMix;
    int glowSrc, glowSend, glowSize, glowDecay, glowDamp, glowPre, glowMix;
    int autoGain, mix, outDb, clipGuard, listen;
};

int MuonProcessor::idx(const juce::String& id) const {
    for (size_t i = 0; i < slots_.size(); ++i) if (slots_[i].param->getParameterID() == id) return (int)i;
    jassertfalse;
    return 0;
}

MuonProcessor::MuonProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, &undoManager, "MUON", createLayout()), presets(*this) {
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(prm)) slots_.push_back({apvts.getRawParameterValue(r->getParameterID()), r});
    values_.assign(slots_.size(), 0.f);
    ids_ = std::make_unique<Ids>();
    Ids& d = *ids_;
    d.bands = idx("bands"); d.xLow = idx("xLow"); d.xHigh = idx("xHigh"); d.xType = idx("xType"); d.stereo = idx("stereo");
    d.sens = idx("sens"); d.look = idx("look"); d.os = idx("os"); d.punch = idx("punch"); d.tail = idx("tail");
    for (int b = 0; b < kMaxBands; ++b) {
        const juce::String x = "b" + juce::String(b + 1);
        auto& q = d.band[b];
        q.atk = idx(x + "Atk"); q.sus = idx(x + "Sus"); q.spd = idx(x + "Spd"); q.clip = idx(x + "Clip"); q.src = idx(x + "Src");
        for (int h = 0; h < kOrders; ++h) { q.h[h] = idx(x + "H" + juce::String(h + 1)); q.inv[h] = h == 0 ? -1 : idx(x + "Inv" + juce::String(h + 1)); }
        q.harm = idx(x + "Harm"); q.curve = idx(x + "Curve"); q.drv = idx(x + "Drv"); q.bias = idx(x + "Bias"); q.tone = idx(x + "Tone");
        q.dmix = idx(x + "DMix"); q.mute = idx(x + "Mute");
    }
    d.sub = idx("sub"); d.sub2 = idx("sub2"); d.subPur = idx("subPur"); d.subRange = idx("subRange"); d.phantom = idx("phantom"); d.phCut = idx("phCut"); d.mono = idx("mono");
    d.resMode = idx("resMode"); d.resIn = idx("resIn"); d.pitch = idx("pitch"); d.root = idx("root"); d.chord = idx("chord"); d.scale = idx("scale");
    d.material = idx("material"); d.size = idx("size"); d.spread = idx("spread"); d.resStereo = idx("resStereo"); d.resDecay = idx("resDecay");
    d.resDamp = idx("resDamp"); d.resTone = idx("resTone"); d.strike = idx("strike"); d.resMix = idx("resMix");
    d.glowSrc = idx("glowSrc"); d.glowSend = idx("glowSend"); d.glowSize = idx("glowSize"); d.glowDecay = idx("glowDecay");
    d.glowDamp = idx("glowDamp"); d.glowPre = idx("glowPre"); d.glowMix = idx("glowMix");
    d.autoGain = idx("autoGain"); d.mix = idx("mix"); d.outDb = idx("outDb"); d.clipGuard = idx("clipGuard"); d.listen = idx("listen");
    for (int m = 0; m < kNumMacros; ++m) macroSlot_[m] = idx("macro" + juce::String(m + 1));
    notes_.reserve(512);
    engine.setParams(currentParams());
}

MuonProcessor::~MuonProcessor() { engine.release(); }

// ---- Macros ---------------------------------------------------------------------------------------------------
void MuonProcessor::setMacroTargets(std::vector<MacroTarget> t) {
    targets_.clear();
    Table tab;
    for (auto& m : t) {
        if (m.macro < 0 || m.macro >= kNumMacros || tab.n >= kMaxTargets) continue;
        int s = -1;
        for (size_t i = 0; i < slots_.size(); ++i) if (slots_[i].param->getParameterID() == m.paramId) { s = (int)i; break; }
        // Macros drive continuous parameters only (and never another macro).
        if (s < 0 || dynamic_cast<juce::AudioParameterFloat*>(slots_[(size_t)s].param) == nullptr || m.paramId.startsWith("macro")) continue;
        tab.macro[tab.n] = m.macro; tab.slot[tab.n] = s; tab.amt[tab.n] = juce::jlimit(-1.f, 1.f, m.amount); ++tab.n;
        targets_.push_back(m);
    }
    seq_.fetch_add(1, std::memory_order_acq_rel);          // odd: writing
    shared_ = tab;
    seq_.fetch_add(1, std::memory_order_acq_rel);          // even: done
}

juce::var MuonProcessor::macrosToVar(const std::vector<MacroTarget>& t) {
    juce::Array<juce::var> out;
    for (auto& m : t) {
        auto* o = new juce::DynamicObject();
        o->setProperty("macro", m.macro); o->setProperty("id", m.paramId); o->setProperty("amount", m.amount);
        out.add(juce::var(o));
    }
    return out;
}

std::vector<MacroTarget> MuonProcessor::macrosFromVar(const juce::var& v) {
    std::vector<MacroTarget> t;
    if (auto* a = v.getArray())
        for (auto& e : *a) t.push_back({(int)e["macro"], e["id"].toString(), (float)(double)e["amount"]});
    return t;
}

// ---- Parameters -> engine ------------------------------------------------------------------------------------
muon::Params MuonProcessor::currentParams() {
    // Pick up a new macro table if one was published (sequence lock: retry next block if it is being written).
    const unsigned s1 = seq_.load(std::memory_order_acquire);
    if ((s1 & 1u) == 0u) {
        Table tmp = shared_;
        std::atomic_thread_fence(std::memory_order_acquire);
        if (seq_.load(std::memory_order_relaxed) == s1) local_ = tmp;
    }
    for (size_t i = 0; i < slots_.size(); ++i) values_[i] = slots_[i].value->load(std::memory_order_relaxed);
    for (int k = 0; k < local_.n; ++k) {
        const float m = values_[(size_t)macroSlot_[local_.macro[k]]];
        if (m <= 0.f) continue;
        auto* prm = slots_[(size_t)local_.slot[k]].param;
        float& v = values_[(size_t)local_.slot[k]];
        const auto& r = prm->getNormalisableRange();
        v = r.convertFrom0to1(juce::jlimit(0.f, 1.f, r.convertTo0to1(v) + m * local_.amt[k]));
    }
    const Ids& d = *ids_;
    auto F = [&](int i) { return values_[(size_t)i]; };
    auto I = [&](int i) { return (int)std::lround(values_[(size_t)i]); };
    auto Bo = [&](int i) { return values_[(size_t)i] > 0.5f; };
    Params p;
    p.bands = I(d.bands) + 1; p.xoverLow = F(d.xLow); p.xoverHigh = F(d.xHigh);
    p.crossover = (CrossoverType)I(d.xType); p.stereo = (StereoMode)I(d.stereo); p.sensitivity = F(d.sens);
    p.lookahead = I(d.look); p.oversample = I(d.os); p.punch = F(d.punch); p.tail = F(d.tail);
    for (int b = 0; b < kMaxBands; ++b) {
        const auto& q = d.band[b];
        auto& o = p.band[(size_t)b];
        o.attack = F(q.atk); o.sustain = F(q.sus); o.speed = F(q.spd); o.clip = F(q.clip); o.source = (HarmSource)I(q.src);
        for (int h = 0; h < kOrders; ++h) { o.orders[(size_t)h] = F(q.h[h]); o.invert[(size_t)h] = h > 0 && Bo(q.inv[h]); }
        o.harmonics = F(q.harm); o.curve = (DriveCurve)I(q.curve); o.drive = F(q.drv); o.bias = F(q.bias); o.tone = F(q.tone);
        o.driveMix = F(q.dmix); o.mute = Bo(q.mute);
    }
    p.subLevel = F(d.sub); p.sub2Level = F(d.sub2); p.subPurity = F(d.subPur); p.subRange = F(d.subRange);
    p.phantom = F(d.phantom); p.phantomCut = F(d.phCut); p.monoBelow = F(d.mono) < 1.f ? 0.f : F(d.mono);
    p.resMode = (ResMode)I(d.resMode); p.resInput = (ResInput)I(d.resIn); p.pitch = (PitchSource)I(d.pitch); p.root = I(d.root);
    p.chord = (Chord)I(d.chord); p.scale = (Scale)I(d.scale); p.material = (Material)I(d.material); p.size = F(d.size);
    p.spread = F(d.spread); p.resStereo = F(d.resStereo); p.resDecay = F(d.resDecay); p.resDamping = F(d.resDamp);
    p.resTone = F(d.resTone); p.strike = F(d.strike); p.resMix = F(d.resMix);
    p.glowSource = (GlowSource)I(d.glowSrc); p.glowSend = F(d.glowSend); p.glowSize = F(d.glowSize); p.glowDecay = F(d.glowDecay);
    p.glowDamp = F(d.glowDamp); p.glowPre = F(d.glowPre); p.glowMix = F(d.glowMix);
    p.autoGain = Bo(d.autoGain); p.mix = F(d.mix); p.outDb = F(d.outDb); p.clipGuard = Bo(d.clipGuard); p.listen = (Listen)I(d.listen);
    return p;
}

// ---- Processor -------------------------------------------------------------------------------------------------
bool MuonProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void MuonProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    engine.setParams(currentParams());
    engine.prepare(sampleRate, std::max(1, samplesPerBlock));
    const int lat = engine.latency();
    latencyShown = lat;
    setLatencySamples(lat);
}

void MuonProcessor::releaseResources() { engine.release(); }

void MuonProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels(), outCh = getTotalNumOutputChannels();
    for (int c = inCh; c < outCh; ++c) buffer.copyFrom(c, 0, buffer, 0, 0, n);   // mono in, stereo out
    notes_.clear();
    for (const auto meta : midi) {
        if (notes_.size() >= notes_.capacity()) break;            // never allocate on the audio thread
        const auto m = meta.getMessage();
        if (m.isNoteOn()) notes_.push_back({true, m.getNoteNumber()});
        else if (m.isNoteOff()) notes_.push_back({false, m.getNoteNumber()});
    }
    midi.clear();
    const Params p = currentParams();
    engine.setParams(p);
    const int lat = engine.latencyFor(p);
    if (lat != latencyShown.load(std::memory_order_relaxed)) { latencyShown.store(lat); setLatencySamples(lat); }
    if (n <= 0) return;
    float* L = buffer.getWritePointer(0);
    if (buffer.getNumChannels() > 1) {
        engine.process(L, buffer.getWritePointer(1), n, notes_.data(), (int)notes_.size());
    } else {
        float tmp[512];
        for (int off = 0; off < n; off += 512) {
            const int c = std::min(512, n - off);
            std::copy(L + off, L + off + c, tmp);
            engine.process(L + off, tmp, c, off == 0 ? notes_.data() : nullptr, off == 0 ? (int)notes_.size() : 0);
            for (int i = 0; i < c; ++i) L[off + i] = 0.5f * (L[off + i] + tmp[i]);
        }
    }
}

juce::AudioProcessorEditor* MuonProcessor::createEditor() { return new MuonEditor(*this); }

void MuonProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    state.setProperty("presetName", presets.currentName(), nullptr);
    state.setProperty("macros", juce::JSON::toString(macrosToVar(targets_), true), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void MuonProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        if (xml->hasTagName(apvts.state.getType())) {
            auto tree = juce::ValueTree::fromXml(*xml);
            const juce::String name = tree.getProperty("presetName").toString();
            const juce::String macros = tree.getProperty("macros").toString();
            apvts.replaceState(tree);
            setMacroTargets(macrosFromVar(juce::JSON::parse(macros)));
            presets.setCurrentName(name);
        }
    }
}

UpdaterProduct updaterProductForThisPlugin() { return muonUpdaterProduct(); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MuonProcessor(); }
