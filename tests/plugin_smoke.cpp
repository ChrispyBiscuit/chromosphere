// Exercises the plugin processor directly: IR loading, all three modes, random parameter changes,
// state save/restore, and loading a state saved before the mode parameters existed.
#include "PluginProcessor.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <thread>

static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static void setParam(ChromosphereProcessor& p, const char* id, float plain) {
    if (auto* prm = p.apvts.getParameter(id)) prm->setValueNotifyingHost(prm->convertTo0to1(plain));
}

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String irPath = argc > 1 ? argv[1] : "data/irs/studio_hall.wav";
    const double fs = 48000; const int block = 256;
    std::mt19937 rng(7);

    ChromosphereProcessor proc;
    proc.setPlayConfigDetails(2, 2, fs, block);
    proc.prepareToPlay(fs, block);
    CHECK(proc.getLatencySamples() == 0, "latency %d", proc.getLatencySamples());
    juce::String err = proc.loadImpulseResponse(juce::File(irPath));
    CHECK(err.isEmpty(), "load: %s", err.toRawUTF8());
    CHECK(proc.loadImpulseResponse(juce::File("/nonexistent.wav")).isNotEmpty(), "bad path accepted");
    CHECK(proc.getImpulseResponsePath() == juce::File(irPath).getFullPathName(), "path not kept after a failed load");

    juce::AudioBuffer<float> buf(2, block);
    juce::MidiBuffer midi;
    bool finite = true, nonSilent = false; float peak = 0;
    auto run = [&](double seconds, bool fuzz) {
        const int blocks = (int)(seconds * fs / block);
        for (int b = 0; b < blocks; ++b) {
            if (fuzz && rng() % 25 == 0) {
                setParam(proc, "size", 0.5f + (rng() % 2500) / 1000.f);
                setParam(proc, "decayMid", 0.2f + (rng() % 8000) / 1000.f);
                setParam(proc, "tailScale", 0.3f + (rng() % 3000) / 1000.f);
                setParam(proc, "mix", (float)(rng() % 101));
                setParam(proc, "mode", (float)(rng() % cs::kModeCount));
                setParam(proc, "cosmicA", (rng() % 101) / 100.f); setParam(proc, "cosmicB", (rng() % 101) / 100.f); setParam(proc, "cosmicC", (rng() % 101) / 100.f);
                setParam(proc, "quality", (float)(rng() % 3));
                setParam(proc, "irReverse", (float)(rng() % 2));
                setParam(proc, "freeze", (float)(rng() % 4 == 0));
                setParam(proc, "irLowCut", (float)((rng() % 3) * 100));
            }
            for (int c = 0; c < 2; ++c) for (int i = 0; i < block; ++i) buf.setSample(c, i, ((rng() >> 8) * (1.0f / 8388608.f) - 1.f) * 0.3f);
            proc.processBlock(buf, midi);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < block; ++i) {
                float v = buf.getSample(c, i);
                finite = finite && std::isfinite(v); peak = std::max(peak, std::fabs(v)); nonSilent |= std::fabs(v) > 1e-4f;
            }
            std::this_thread::sleep_for(std::chrono::microseconds((long)(1e6 * block / fs * 0.5)));
        }
    };
    setParam(proc, "mix", 100.f);
    for (int mode = 0; mode < cs::kModeCount; ++mode) {
        setParam(proc, "mode", (float)mode);
        run(mode < 3 ? 1.5 : 0.8, false);
        CHECK(finite && nonSilent, "mode %d: finite %d non-silent %d", mode, (int)finite, (int)nonSilent);
    }
    run(8.0, true);
    CHECK(finite, "non-finite output while fuzzing");
    CHECK(peak < 30.f, "peak %g", peak);

    // Every factory preset loads and renders audio.
    {
        auto& pm = proc.presets;
        int factory = 0, bad = 0;
        setParam(proc, "mix", 100.f);
        for (int i = 0; i < (int)pm.presets().size(); ++i) {
            if (!pm.presets()[(size_t)i].factory) continue;
            ++factory;
            CHECK(pm.apply(i), "apply %d", i);
            finite = true; nonSilent = false; peak = 0.f;     // judge each preset on its own output
            run(0.45, false);
            if (!finite || !nonSilent || peak > 30.f) { ++bad; std::printf("  preset '%s': finite %d non-silent %d peak %g\n", pm.presets()[(size_t)i].name.toRawUTF8(), (int)finite, (int)nonSilent, peak); }
        }
        CHECK(factory >= 100, "only %d factory presets", factory);
        CHECK(bad == 0, "%d presets rendered badly", bad);
        std::printf("%d factory presets rendered\n", factory);

        // Undo restores the previous parameter values; A/B toggles between two snapshots; user presets round-trip.
        pm.apply(pm.indexOf("Init"));
        const float before = proc.apvts.getRawParameterValue("roomLength")->load();
        pm.apply(pm.indexOf("Cathedral"));
        CHECK(std::fabs(proc.apvts.getRawParameterValue("roomLength")->load() - before) > 10.f, "preset did not change the room");
        proc.undoManager.undo();
        CHECK(std::fabs(proc.apvts.getRawParameterValue("roomLength")->load() - before) < 1e-3f, "undo did not restore the room");
        setParam(proc, "size", 2.0f);
        pm.toggleAB(); setParam(proc, "size", 0.7f);
        pm.toggleAB();
        CHECK(std::fabs(proc.apvts.getRawParameterValue("size")->load() - 2.0f) < 1e-3f, "A/B: slot A lost size");
        pm.toggleAB();
        CHECK(std::fabs(proc.apvts.getRawParameterValue("size")->load() - 0.7f) < 1e-3f, "A/B: slot B lost size");
        setParam(proc, "size", 1.234f); setParam(proc, "decayMid", 4.5f);
        const juce::String userName = "smoke test preset";
        CHECK(pm.saveUser(userName, "User", juce::StringArray("test", "tmp")), "save user preset");
        setParam(proc, "size", 1.0f);
        CHECK(pm.apply(pm.indexOf(userName)), "apply user preset");
        CHECK(std::fabs(proc.apvts.getRawParameterValue("size")->load() - 1.234f) < 2e-3f, "user preset size %g", proc.apvts.getRawParameterValue("size")->load());
        CHECK(pm.deleteUser(pm.indexOf(userName)) && pm.indexOf(userName) < 0, "delete user preset");
    }
    // State round trip.
    setParam(proc, "mode", 2.f); setParam(proc, "size", 1.7f); setParam(proc, "irWidth", 0.4f);
    juce::MemoryBlock state; proc.getStateInformation(state);
    ChromosphereProcessor other;
    other.setPlayConfigDetails(2, 2, fs, block); other.prepareToPlay(fs, block);
    other.setStateInformation(state.getData(), (int)state.getSize());
    CHECK(other.getImpulseResponsePath() == proc.getImpulseResponsePath(), "IR path not restored: '%s'", other.getImpulseResponsePath().toRawUTF8());
    for (const char* id : {"mode", "size", "irWidth", "decayMid", "mix", "quality"})
        CHECK(std::fabs(other.apvts.getRawParameterValue(id)->load() - proc.apvts.getRawParameterValue(id)->load()) < 1e-4f, "param %s differs", id);

    // A state from before version 2 (no mode, IR, or edit parameters, no irPath) loads with defaults.
    if (auto xml = juce::parseXML(juce::String::createStringFromData(state.getData(), (int)state.getSize())); false) (void)xml;
    {
        std::unique_ptr<juce::XmlElement> xml = ChromosphereProcessor::getXmlFromBinary(state.getData(), (int)state.getSize());
        if (xml) {
            for (int i = xml->getNumChildElements() - 1; i >= 0; --i) {
                auto* ch = xml->getChildElement(i);
                const juce::String id = ch->getStringAttribute("id");
                for (const char* newId : {"mode", "erMs", "tailScale", "irStart", "irLength", "irStretch", "irReverse", "irLowCut", "irHighCut", "irWidth", "irDecay", "cosmicA", "cosmicB", "cosmicC"})
                    if (id == newId) { xml->removeChildElement(ch, true); break; }
            }
            xml->removeAttribute("irPath"); xml->removeAttribute("stateVersion");
            juce::MemoryBlock old; ChromosphereProcessor::copyXmlToBinary(*xml, old);
            ChromosphereProcessor legacy;
            legacy.setPlayConfigDetails(2, 2, fs, block); legacy.prepareToPlay(fs, block);
            legacy.setStateInformation(old.getData(), (int)old.getSize());
            CHECK(legacy.apvts.getRawParameterValue("mode")->load() == 0.f, "old state: mode is %g", legacy.apvts.getRawParameterValue("mode")->load());
            CHECK(std::fabs(legacy.apvts.getRawParameterValue("size")->load() - 1.7f) < 1e-3f, "old state: size lost");
            CHECK(legacy.getImpulseResponsePath().isEmpty(), "old state: unexpected IR");
        } else CHECK(false, "could not decode state");
    }
    proc.releaseResources();
    // Sleep: after input stops and the tail dies away the instance sleeps; any input wakes it on the same block.
    {
        ChromosphereProcessor sp;
        sp.setPlayConfigDetails(2, 2, fs, block);
        setParam(sp, "mode", 0.f);   // Algorithmic: the decay dials set the tail
        setParam(sp, "decayMid", 0.4f); setParam(sp, "decayLow", 0.4f); setParam(sp, "decayHigh", 0.3f); setParam(sp, "mix", 100.f);
        sp.prepareToPlay(fs, block);
        juce::AudioBuffer<float> b(2, block); juce::MidiBuffer m;
        auto runFor = [&](double sec, float amp) {
            for (int i = 0; i < (int)(sec * fs / block); ++i) {
                for (int c = 0; c < 2; ++c) for (int k = 0; k < block; ++k) b.setSample(c, k, amp * std::sin(0.05f * (float)(i * block + k)));
                sp.processBlock(b, m);
            }
        };
        runFor(1.0, 0.3f);
        CHECK(!sp.isSleeping(), "asleep while playing");
        runFor(6.0, 0.f);
        CHECK(sp.isSleeping(), "not asleep after 6 s of silence with a 0.4 s decay");
        runFor((double)block / fs, 0.3f);
        CHECK(!sp.isSleeping(), "did not wake on input");
        runFor(0.2, 0.3f);
        double e = 0; for (int k = 0; k < block; ++k) e += (double)b.getSample(0, k) * b.getSample(0, k);
        CHECK(e > 1e-4, "no wet output after waking (%g)", e);
        sp.releaseResources();
    }
    std::printf(failures ? "%d failure(s)\n" : "plugin smoke tests passed\n", failures);
    return failures ? 1 : 0;
}
