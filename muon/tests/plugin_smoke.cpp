// Exercises the MUON processor: parameters, latency reporting, state save and restore (macros included), macros
// moving their targets, Ignite, mono buses, MIDI-driven resonators, parameter fuzzing, and the user preset round trip.
#include "Ignite.h"
#include "PluginProcessor.h"
#include "material.h"
#include <cstdio>
#include <random>

static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf(__VA_ARGS__); std::printf("\n"); } else { std::printf("PASS  "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static void setParam(MuonProcessor& p, const juce::String& id, float plain) {
    if (auto* prm = p.apvts.getParameter(id)) prm->setValueNotifyingHost(prm->convertTo0to1(plain));
}
static float val(MuonProcessor& p, const char* id) { auto* q = p.apvts.getParameter(id); return q->convertFrom0to1(q->getValue()); }

struct Stats { bool finite = true; float peak = 0; double rms = 0; };

static Stats run(MuonProcessor& proc, int inCh, int outCh, const material::Stereo& src, double seconds,
                 std::function<void(int, juce::MidiBuffer&)> perBlock = {}) {
    const int block = 256;
    juce::AudioBuffer<float> buf(std::max(inCh, outCh), block);
    Stats s; double e = 0; size_t n = 0;
    const int blocks = (int)(seconds * 48000.0 / block);
    for (int b = 0; b < blocks; ++b) {
        for (int i = 0; i < block; ++i) {
            const size_t k = (size_t)((b * block + i) % (int)src.L.size());
            buf.setSample(0, i, src.L[k]);
            if (buf.getNumChannels() > 1) buf.setSample(1, i, inCh > 1 ? src.R[k] : 0.f);
        }
        juce::MidiBuffer midi;
        if (perBlock) perBlock(b, midi);
        proc.processBlock(buf, midi);
        for (int c = 0; c < outCh; ++c)
            for (int i = 0; i < block; ++i) {
                const float v = buf.getSample(c, i);
                if (!std::isfinite(v)) s.finite = false;
                s.peak = std::max(s.peak, std::abs(v));
                e += v * v; ++n;
            }
    }
    s.rms = std::sqrt(e / std::max<size_t>(n, 1));
    return s;
}

int main() {
    juce::ScopedJuceInitialiser_GUI init;
    const auto drums = material::make("drums"), bass = material::make("bass");
    MuonProcessor proc;
    proc.setPlayConfigDetails(2, 2, 48000.0, 256);
    proc.prepareToPlay(48000.0, 256);
    const int nParams = proc.getParameters().size();
    CHECK(nParams > 100, "%d automatable parameters", nParams);
    CHECK(proc.getLatencySamples() == 0 && proc.acceptsMidi() && !proc.producesMidi(), "effect: zero latency by default, MIDI in for the resonators");
    CHECK(proc.presets.presets().size() >= 120, "%d factory presets", (int)proc.presets.presets().size());

    // Neutral by default: the output equals the input (one band, no processing), after the auto-gain settles.
    {
        MuonProcessor n;
        n.setPlayConfigDetails(2, 2, 48000.0, 256);
        setParam(n, "bands", 0.f); setParam(n, "autoGain", 0.f);
        n.prepareToPlay(48000.0, 256);
        juce::AudioBuffer<float> buf(2, 256);
        bool same = true;
        for (int b = 0; b < 40 && same; ++b) {
            for (int i = 0; i < 256; ++i) { buf.setSample(0, i, drums.L[(size_t)(b * 256 + i)]); buf.setSample(1, i, drums.R[(size_t)(b * 256 + i)]); }
            juce::MidiBuffer m; n.processBlock(buf, m);
            for (int i = 0; i < 256; ++i) if (buf.getSample(0, i) != drums.L[(size_t)(b * 256 + i)] || buf.getSample(1, i) != drums.R[(size_t)(b * 256 + i)]) same = false;
        }
        CHECK(same, "default parameters with one band and auto-gain off: output bit-identical to the input");
    }

    // Latency is reported for lookahead and linear phase, and follows parameter changes.
    setParam(proc, "look", 3.f);   // 5 ms
    run(proc, 2, 2, drums, 0.05);
    CHECK(proc.getLatencySamples() == 240, "lookahead 5 ms reports 240 samples (%d)", proc.getLatencySamples());
    setParam(proc, "xType", 1.f);
    run(proc, 2, 2, drums, 0.05);
    CHECK(proc.getLatencySamples() == proc.engine.latency() && proc.getLatencySamples() > 600, "linear phase reports its latency (%d)", proc.getLatencySamples());
    setParam(proc, "look", 0.f); setParam(proc, "xType", 0.f);
    run(proc, 2, 2, drums, 0.05);
    CHECK(proc.getLatencySamples() == 0, "back to zero latency");

    // Macros: an assignment moves its target; state keeps the assignments.
    proc.setMacroTargets({{0, "b2Atk", 0.5f}, {2, "b2Drv", 0.8f}});
    setParam(proc, "macro1", 1.f); setParam(proc, "macro3", 0.5f);
    auto mp = proc.currentParams();
    CHECK(std::abs(mp.band[1].attack - 0.f - 1.f) < 0.02f && std::abs(mp.band[1].drive - 0.4f) < 0.02f,
          "macros move their targets (attack %.2f, drive %.2f)", mp.band[1].attack, mp.band[1].drive);
    setParam(proc, "b1Spd", 0.33f); setParam(proc, "xLow", 321.f); setParam(proc, "resMode", 2.f); setParam(proc, "root", 50.f);
    juce::MemoryBlock state; proc.getStateInformation(state);
    MuonProcessor other;
    other.setStateInformation(state.getData(), (int)state.getSize());
    CHECK(std::abs(val(other, "b1Spd") - 0.33f) < 1e-3f && std::abs(val(other, "xLow") - 321.f) < 1.f && (int)val(other, "resMode") == 2 &&
          (int)val(other, "root") == 50 && other.macroTargets().size() == 2, "state save and restore, macro assignments included");
    proc.setMacroTargets({}); setParam(proc, "macro1", 0.f); setParam(proc, "macro3", 0.f);

    // Every factory preset's macros all do something audible-range: each macro at full stays finite and bounded.
    {
        bool ok = true; float worst = 0;
        for (int i = 0; i < (int)proc.presets.presets().size(); i += 7) {
            proc.presets.apply(i);
            for (int m = 1; m <= 4; ++m) setParam(proc, "macro" + juce::String(m), 1.f);
            auto st = run(proc, 2, 2, drums, 0.4);
            if (!st.finite || st.peak > 8.f) ok = false;
            worst = std::max(worst, st.peak);
            for (int m = 1; m <= 4; ++m) setParam(proc, "macro" + juce::String(m), 0.f);
        }
        CHECK(ok, "every 7th preset with all macros at full: finite (worst peak %.2f)", worst);
    }

    // Ignite: every source, many times; ids valid, sound finite and under the clip guard.
    juce::Random rng(42);
    bool idsOk = true, soundOk = true;
    float worst = 0;
    for (int k = 0; k < 36; ++k) {
        auto s = ignite::generate(rng, k % ignite::kNumSources);
        for (const auto& kv : s.params) {
            auto* prm = proc.apvts.getParameter(kv.name.toString());
            if (!prm) { idsOk = false; std::printf("  ignite: unknown id %s\n", kv.name.toString().toRawUTF8()); continue; }
            if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(prm))
                if (kv.value.isString() && c->choices.indexOf(kv.value.toString()) < 0) { idsOk = false; std::printf("  ignite: bad choice %s\n", kv.name.toString().toRawUTF8()); }
        }
        for (auto& m : MuonProcessor::macrosFromVar(s.macros)) if (!proc.apvts.getParameter(m.paramId)) { idsOk = false; std::printf("  ignite: bad macro id %s\n", m.paramId.toRawUTF8()); }
        proc.presets.applyGenerated(s.params, s.macros, s.name, s.category);
        auto st = run(proc, 2, 2, k % 2 ? drums : bass, 0.6);
        if (!st.finite || st.peak > 0.95f) soundOk = false;
        worst = std::max(worst, st.peak);
    }
    CHECK(idsOk, "Ignite writes only real parameters, choices and macro targets");
    CHECK(soundOk, "36 Ignite settings: finite and under the clip guard (worst peak %.3f)", worst);

    // Mono in, stereo out; and mono in / mono out.
    for (int outs : {2, 1}) {
        MuonProcessor mono;
        mono.setPlayConfigDetails(1, outs, 48000.0, 256);
        mono.prepareToPlay(48000.0, 256);
        mono.presets.apply(mono.presets.indexOf("Punch Bus"));
        auto ms = run(mono, 1, outs, drums, 0.5);
        CHECK(ms.finite && ms.peak > 0.05f, "mono in, %d out: works (peak %.3f)", outs, ms.peak);
    }

    // MIDI-driven resonators: notes retune the bank.
    proc.presets.apply(proc.presets.indexOf("MIDI Resonator"));
    run(proc, 2, 2, drums, 0.3, [](int b, juce::MidiBuffer& m) { if (b == 0) for (int n : {48, 55, 64}) m.addEvent(juce::MidiMessage::noteOn(1, n, (juce::uint8)100), 0); });
    {
        const int cnt = proc.engine.meters.resCount.load();
        bool match = cnt == 3;
        for (int i = 0; i < cnt; ++i) {
            const float hz = proc.engine.meters.resHz[i].load();
            const float note = 69.f + 12.f * std::log2(hz / 440.f);
            if (std::abs(note - std::round(note)) > 0.05f) match = false;
        }
        CHECK(match, "MIDI notes tune the resonators (%d resonators)", cnt);
    }

    // Fuzz: random parameter changes on drums.
    std::mt19937 r(7);
    auto params = proc.getParameters();
    proc.presets.apply(proc.presets.indexOf("Bus Glue"));
    auto fz = run(proc, 2, 2, drums, 12.0, [&](int b, juce::MidiBuffer& m) {
        if (b % 3 == 0) {
            auto* prm = dynamic_cast<juce::RangedAudioParameter*>(params[(int)(r() % (unsigned)params.size())]);
            const juce::String id = prm->getParameterID();
            if (id != "outDb" && id != "listen" && id != "mix") prm->setValueNotifyingHost((r() % 1000) / 999.f);
        }
        if (b % 29 == 0) m.addEvent(juce::MidiMessage::noteOn(1, 36 + (int)(r() % 48), (juce::uint8)100), 0);
        if (b % 31 == 0) m.addEvent(juce::MidiMessage::noteOff(1, 36 + (int)(r() % 48)), 0);
    });
    CHECK(fz.finite && fz.peak < 16.f, "12 s of parameter fuzzing (peak %.3f)", fz.peak);

    // User preset round trip, macros included.
    const juce::String userName = "__muon_smoke_test__";
    setParam(proc, "b2Atk", 0.42f);
    proc.setMacroTargets({{1, "sub", 0.6f}});
    CHECK(proc.presets.saveUser(userName, "User", {"test"}), "save user preset");
    setParam(proc, "b2Atk", -0.1f); proc.setMacroTargets({});
    CHECK(proc.presets.apply(proc.presets.indexOf(userName)) && std::abs(val(proc, "b2Atk") - 0.42f) < 1e-3f && proc.macroTargets().size() == 1,
          "apply user preset (macros restored)");
    CHECK(proc.presets.deleteUser(proc.presets.indexOf(userName)) && proc.presets.indexOf(userName) < 0, "delete user preset");

    std::printf("%d failure(s)\n", failures);
    return failures;
}
