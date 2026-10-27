// Exercises the INFRA processor: parameters, state save and restore, Ignite, mono output, parameter fuzzing
// while notes and the arp play, and the user preset round trip.
#include "Ignite.h"
#include "PluginProcessor.h"
#include <cstdio>
#include <random>

static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf(__VA_ARGS__); std::printf("\n"); } else { std::printf("PASS  "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static void setParam(InfraProcessor& p, const juce::String& id, float plain) {
    if (auto* prm = p.apvts.getParameter(id)) prm->setValueNotifyingHost(prm->convertTo0to1(plain));
}

struct Stats { bool finite = true; float peak = 0; };

static Stats run(InfraProcessor& proc, int channels, double seconds, std::function<void(int, juce::MidiBuffer&)> perBlock = {}) {
    const int block = 256;
    juce::AudioBuffer<float> buf(channels, block);
    Stats s;
    const int blocks = (int)(seconds * 48000.0 / block);
    for (int b = 0; b < blocks; ++b) {
        juce::MidiBuffer midi;
        if (perBlock) perBlock(b, midi);
        proc.processBlock(buf, midi);
        for (int c = 0; c < channels; ++c)
            for (int i = 0; i < block; ++i) {
                const float v = buf.getSample(c, i);
                if (!std::isfinite(v)) s.finite = false;
                s.peak = std::max(s.peak, std::abs(v));
            }
    }
    return s;
}

int main() {
    juce::ScopedJuceInitialiser_GUI init;
    InfraProcessor proc;
    proc.setPlayConfigDetails(0, 2, 48000.0, 256);
    proc.prepareToPlay(48000.0, 256);
    const int nParams = proc.getParameters().size();
    CHECK(nParams > 170, "%d automatable parameters", nParams);
    CHECK(proc.getLatencySamples() == 0 && proc.acceptsMidi() && !proc.producesMidi(), "instrument: MIDI in, no latency");

    // Every choice parameter has a name for each engine value (the editor and presets rely on that).
    CHECK(dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter("m1Dst"))->choices.size() == infra::kNumModDests, "mod destinations listed");

    // A held chord with the arp on must make sound.
    proc.presets.apply(proc.presets.indexOf("Orbit Arp"));
    auto chord = [](int b, juce::MidiBuffer& m) {
        if (b == 1) for (int n : {60, 64, 67}) m.addEvent(juce::MidiMessage::noteOn(1, n, (juce::uint8)100), 10);
        if (b == 300) for (int n : {60, 64, 67}) m.addEvent(juce::MidiMessage::noteOff(1, n), 0);
    };
    auto s = run(proc, 2, 2.0, chord);
    CHECK(s.finite && s.peak > 0.01f && s.peak < 0.95f, "arp preset plays (peak %.3f)", s.peak);

    // State round trip.
    setParam(proc, "aMorph", 0.37f); setParam(proc, "fCutoff", 1234.f); setParam(proc, "arpMode", 9.f); setParam(proc, "s5Tr", 7.f);
    juce::MemoryBlock state; proc.getStateInformation(state);
    InfraProcessor other;
    other.setStateInformation(state.getData(), (int)state.getSize());
    auto val = [](InfraProcessor& p, const char* id) { auto* q = p.apvts.getParameter(id); return q->convertFrom0to1(q->getValue()); };
    CHECK(std::abs(val(other, "aMorph") - 0.37f) < 1e-3f && std::abs(val(other, "fCutoff") - 1234.f) < 1.f && (int)val(other, "arpMode") == 9 &&
          (int)val(other, "s5Tr") == 7, "state save and restore");
    CHECK(other.presets.currentName() == proc.presets.currentName(), "preset name restored (%s)", other.presets.currentName().toRawUTF8());

    // Ignite: every archetype, many times; ids valid, sound finite and bounded.
    juce::Random rng(42);
    bool idsOk = true, soundOk = true;
    float worst = 0;
    for (int k = 0; k < 64; ++k) {
        auto patch = ignite::generate(rng, k % ignite::kNumArchetypes);
        for (const auto& kv : patch.params) {
            auto* prm = proc.apvts.getParameter(kv.name.toString());
            if (!prm) { idsOk = false; std::printf("  ignite: unknown id %s\n", kv.name.toString().toRawUTF8()); continue; }
            if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(prm))
                if (kv.value.isString() && c->choices.indexOf(kv.value.toString()) < 0) { idsOk = false; std::printf("  ignite: bad choice %s=%s\n", kv.name.toString().toRawUTF8(), kv.value.toString().toRawUTF8()); }
        }
        proc.presets.applyGenerated(patch.params, patch.name, patch.category);
        proc.synth.reset();
        auto st = run(proc, 2, 0.8, [](int b, juce::MidiBuffer& m) {
            if (b == 0) for (int n : {48, 55, 62}) m.addEvent(juce::MidiMessage::noteOn(1, n, (juce::uint8)110), 0);
            if (b == 100) for (int n : {48, 55, 62}) m.addEvent(juce::MidiMessage::noteOff(1, n), 0);
        });
        if (!st.finite || st.peak > 0.95f) soundOk = false;
        worst = std::max(worst, st.peak);
    }
    CHECK(idsOk, "Ignite writes only real parameters and choices");
    CHECK(soundOk, "64 Ignite patches: finite and under the ceiling (worst peak %.3f)", worst);

    // Mono output bus.
    InfraProcessor mono;
    mono.setPlayConfigDetails(0, 1, 48000.0, 256);
    mono.prepareToPlay(48000.0, 256);
    auto ms = run(mono, 1, 0.5, [](int b, juce::MidiBuffer& m) { if (b == 0) m.addEvent(juce::MidiMessage::noteOn(1, 57, (juce::uint8)100), 3); });
    CHECK(ms.finite && ms.peak > 0.01f, "mono output works (peak %.3f)", ms.peak);

    // Fuzz: random parameter changes while notes, bends, wheel and pressure move.
    std::mt19937 r(7);
    auto params = proc.getParameters();
    proc.presets.apply(proc.presets.indexOf("Init"));
    auto fz = run(proc, 2, 12.0, [&](int b, juce::MidiBuffer& m) {
        if (b % 3 == 0) {
            auto* prm = dynamic_cast<juce::RangedAudioParameter*>(params[(int)(r() % (unsigned)params.size())]);
            const juce::String id = prm->getParameterID();
            if (id != "outDb") prm->setValueNotifyingHost((r() % 1000) / 999.f);
        }
        if (b % 17 == 0) m.addEvent(juce::MidiMessage::noteOn(1, 36 + (int)(r() % 48), (juce::uint8)(20 + r() % 107)), (int)(r() % 256));
        if (b % 23 == 0) m.addEvent(juce::MidiMessage::noteOff(1, 36 + (int)(r() % 48)), (int)(r() % 256));
        if (b % 11 == 0) m.addEvent(juce::MidiMessage::pitchWheel(1, (int)(r() % 16384)), 0);
        if (b % 13 == 0) m.addEvent(juce::MidiMessage::controllerEvent(1, 1, (int)(r() % 128)), 0);
        if (b % 19 == 0) m.addEvent(juce::MidiMessage::channelPressureChange(1, (int)(r() % 128)), 0);
        if (b % 97 == 0) m.addEvent(juce::MidiMessage::controllerEvent(1, 64, (int)(r() % 2) * 127), 0);
    });
    CHECK(fz.finite && fz.peak < 0.95f, "12 s of parameter fuzzing with live MIDI (peak %.3f)", fz.peak);

    // User preset round trip.
    const juce::String userName = "__infra_smoke_test__";
    setParam(proc, "aMorph", 0.81f);
    CHECK(proc.presets.saveUser(userName, "User", {"test"}), "save user preset");
    setParam(proc, "aMorph", 0.1f);
    CHECK(proc.presets.apply(proc.presets.indexOf(userName)) && std::abs(val(proc, "aMorph") - 0.81f) < 1e-3f, "apply user preset");
    CHECK(proc.presets.deleteUser(proc.presets.indexOf(userName)) && proc.presets.indexOf(userName) < 0, "delete user preset");

    std::printf("%d failure(s)\n", failures);
    return failures;
}
