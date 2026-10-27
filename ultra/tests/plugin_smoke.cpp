// Exercises the ULTRA processor: parameters, patterns through the value tree and the triple buffer, state save and
// restore, undo of a pattern edit, Ignite, pads, MIDI, the mono and multi-out bus layouts, parameter fuzzing while
// the sequencer runs, and the user preset round trip.
#include "Ignite.h"
#include "PluginProcessor.h"
#include <cstdio>
#include <random>

static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf(__VA_ARGS__); std::printf("\n"); } else { std::printf("PASS  "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static void setParam(UltraProcessor& p, const juce::String& id, float plain) {
    if (auto* prm = p.apvts.getParameter(id)) prm->setValueNotifyingHost(prm->convertTo0to1(plain));
}
static float val(UltraProcessor& p, const juce::String& id) { auto* q = p.apvts.getParameter(id); return q->convertFrom0to1(q->getValue()); }

struct Stats { bool finite = true; float peak = 0; };

static Stats run(UltraProcessor& proc, juce::AudioBuffer<float>& buf, double seconds, std::function<void(int, juce::MidiBuffer&)> perBlock = {},
                 int checkChannels = -1) {
    const int block = buf.getNumSamples();
    Stats s;
    const int blocks = (int)(seconds * 48000.0 / block);
    const int chans = checkChannels < 0 ? buf.getNumChannels() : checkChannels;
    for (int b = 0; b < blocks; ++b) {
        juce::MidiBuffer midi;
        if (perBlock) perBlock(b, midi);
        proc.processBlock(buf, midi);
        for (int c = 0; c < chans; ++c)
            for (int i = 0; i < block; ++i) {
                const float v = buf.getSample(c, i);
                if (!std::isfinite(v)) s.finite = false;
                s.peak = std::max(s.peak, std::abs(v));
            }
    }
    return s;
}

int main() {
    setenv("ULTRA_NO_UPDATE_CHECK", "1", 1);
    juce::ScopedJuceInitialiser_GUI init;
    UltraProcessor proc;
    proc.setPlayConfigDetails(0, 2, 48000.0, 256);
    proc.prepareToPlay(48000.0, 256);
    juce::AudioBuffer<float> stereo(2, 256);
    const int nParams = proc.getParameters().size();
    CHECK(nParams > 200, "%d automatable parameters", nParams);
    CHECK(proc.getLatencySamples() == 0 && proc.acceptsMidi() && !proc.producesMidi(), "instrument: MIDI in, no latency");
    CHECK(proc.getBusCount(false) == 1 + ultra::kNumAux && proc.getBusCount(true) == 0, "main output plus %d aux outputs", ultra::kNumAux);
    CHECK(proc.patterns.trackName(3) == "Closed Hat", "Init track names");

    // The Init beat plays from the internal clock.
    proc.setPlaying(true);
    auto s = run(proc, stereo, 2.0);
    CHECK(s.finite && s.peak > 0.05f && s.peak < 0.95f, "Init beat plays (peak %.3f)", s.peak);
    CHECK(proc.machine.hitCount[0].load() >= 3 && proc.machine.hitCount[3].load() >= 6, "kick and hats hit (%d, %d)", proc.machine.hitCount[0].load(), proc.machine.hitCount[3].load());
    proc.setPlaying(false);

    // A pattern edit reaches the engine through the tree and the triple buffer; undo takes it back.
    proc.undoManager.beginNewTransaction("test");
    proc.patterns.setLane(0, 7, 2, PatternStore::Vel, 99);
    proc.patterns.setLane(0, 7, 2, PatternStore::Ratchet, 3);
    const auto* bank = proc.patterns.acquireForAudio();
    CHECK(bank->patterns[0].tracks[7].steps[2].vel == 99 && bank->patterns[0].tracks[7].steps[2].ratchet == 3, "pattern edit published to the audio side");
    proc.undoManager.beginNewTransaction("after");
    proc.undoManager.undo();
    CHECK(proc.patterns.lane(0, 7, 2, PatternStore::Vel) == 0 && proc.patterns.lane(0, 7, 2, PatternStore::Ratchet) == 1, "undo reverts the pattern edit");
    proc.undoManager.redo();
    CHECK(proc.patterns.lane(0, 7, 2, PatternStore::Vel) == 99, "redo restores it");

    // State round trip: parameters, patterns, names.
    setParam(proc, "t2Decay", 0.37f); setParam(proc, "t5Model", 9.f); setParam(proc, "swing", 0.6f); setParam(proc, "pattern", 2.f);
    proc.patterns.setLane(2, 4, 31, PatternStore::Cond, 7);
    proc.patterns.setLength(2, 4, 32);
    proc.patterns.setTrackName(6, "Conga");
    juce::MemoryBlock state; proc.getStateInformation(state);
    UltraProcessor other;
    other.setStateInformation(state.getData(), (int)state.getSize());
    CHECK(std::abs(val(other, "t2Decay") - 0.37f) < 1e-3f && (int)val(other, "t5Model") == 9 && std::abs(val(other, "swing") - 0.6f) < 1e-3f &&
          (int)val(other, "pattern") == 2, "parameters restored");
    CHECK(other.patterns.lane(2, 4, 31, PatternStore::Cond) == 7 && other.patterns.track(2, 4).length == 32 && other.patterns.lane(0, 7, 2, PatternStore::Vel) == 99,
          "patterns restored");
    CHECK(other.patterns.trackName(6) == "Conga", "track names restored");
    const auto* ob = other.patterns.acquireForAudio();
    CHECK(ob->patterns[2].tracks[4].steps[31].cond == 7, "restored patterns reach the audio side");
    CHECK(state.getSize() < 64 * 1024, "state is compact (%d bytes)", (int)state.getSize());

    // Pads and MIDI notes.
    {
        UltraProcessor p2; p2.setPlayConfigDetails(0, 2, 48000.0, 256); p2.prepareToPlay(48000.0, 256);
        juce::AudioBuffer<float> b(2, 256);
        p2.playPad(1, 1.f);
        auto st = run(p2, b, 0.1);
        CHECK(st.peak > 0.05f && p2.machine.hitCount[1].load() == 1, "a pad plays its track (peak %.3f)", st.peak);
        run(p2, b, 0.2, [](int blk, juce::MidiBuffer& m) { if (blk == 0) m.addEvent(juce::MidiMessage::noteOn(10, 42, (juce::uint8)100), 17); });
        CHECK(p2.machine.hitCount[3].load() == 1, "MIDI note 42 plays the closed hat");
    }

    // Mono main output and the multi-out layout with Aux 3 enabled.
    {
        UltraProcessor mono;
        auto layout = mono.getBusesLayout();
        layout.outputBuses.getReference(0) = juce::AudioChannelSet::mono();
        CHECK(mono.setBusesLayout(layout), "mono main output accepted");
        mono.prepareToPlay(48000.0, 256);
        juce::AudioBuffer<float> b(1, 256);
        mono.setPlaying(true);
        auto ms = run(mono, b, 1.0);
        CHECK(ms.finite && ms.peak > 0.02f, "mono output works (peak %.3f)", ms.peak);
    }
    {
        UltraProcessor multi;
        auto layout = multi.getBusesLayout();
        layout.outputBuses.getReference(3) = juce::AudioChannelSet::stereo();
        CHECK(multi.setBusesLayout(layout), "Aux 3 can be enabled");
        CHECK(!multi.checkBusesLayoutSupported([] { juce::AudioProcessor::BusesLayout l; l.outputBuses.add(juce::AudioChannelSet::create5point1()); return l; }()),
              "5.1 main output refused");
        multi.prepareToPlay(48000.0, 256);
        setParam(multi, "t1Out", 3.f);
        juce::AudioBuffer<float> b(multi.getTotalNumOutputChannels(), 256);
        float mainPk = 0.f, auxPk = 0.f;
        for (int blk = 0; blk < 40; ++blk) {
            juce::MidiBuffer m;
            if (blk == 0) m.addEvent(juce::MidiMessage::noteOn(1, 36, (juce::uint8)127), 0);
            multi.processBlock(b, m);
            for (int i = 0; i < 256; ++i) { mainPk = std::max(mainPk, std::abs(b.getSample(0, i))); auxPk = std::max(auxPk, std::abs(b.getSample(2, i))); }
        }
        CHECK(auxPk > 0.05f && mainPk < 0.02f, "kick routed to Aux 3 plays there, not on main (aux %.3f, main %.3f)", auxPk, mainPk);
    }

    // Ignite: every genre, several times; ids valid, sound finite and bounded.
    juce::Random rng(42);
    bool idsOk = true, soundOk = true;
    float worst = 0;
    for (int k = 0; k < 32; ++k) {
        auto kit = ignite::generate(rng, k % ignite::kNumGenres);
        for (const auto& kv : kit.params) {
            auto* prm = proc.apvts.getParameter(kv.name.toString());
            if (!prm) { idsOk = false; std::printf("  ignite: unknown id %s\n", kv.name.toString().toRawUTF8()); continue; }
            if (auto* c = dynamic_cast<juce::AudioParameterChoice*>(prm))
                if (kv.value.isString() && c->choices.indexOf(kv.value.toString()) < 0) { idsOk = false; std::printf("  ignite: bad choice %s=%s\n", kv.name.toString().toRawUTF8(), kv.value.toString().toRawUTF8()); }
        }
        proc.presets.applyGenerated(kit.params, kit.patterns, kit.names, kit.name, kit.category);
        proc.machine.reset();
        proc.setPlaying(true);
        auto st = run(proc, stereo, 2.0);
        proc.setPlaying(false);
        if (!st.finite || st.peak > 0.95f || st.peak < 0.02f) soundOk = false;
        worst = std::max(worst, st.peak);
    }
    CHECK(idsOk, "Ignite writes only real parameters and choices");
    CHECK(soundOk, "32 Ignite kits play: finite, audible and under the ceiling (worst peak %.3f)", worst);

    // Fuzz: random parameter changes, pattern edits and pattern switches while the sequencer and MIDI run.
    std::mt19937 r(7);
    auto params = proc.getParameters();
    proc.presets.apply(proc.presets.indexOf("Init"));
    proc.setPlaying(true);
    auto fz = run(proc, stereo, 12.0, [&](int b, juce::MidiBuffer& m) {
        if (b % 3 == 0) {
            auto* prm = dynamic_cast<juce::RangedAudioParameter*>(params[(int)(r() % (unsigned)params.size())]);
            const juce::String id = prm->getParameterID();
            if (id != "outDb" && !id.endsWith("Level")) prm->setValueNotifyingHost((r() % 1000) / 999.f);
        }
        if (b % 7 == 0) proc.patterns.setLane((int)(r() % 8), (int)(r() % 8), (int)(r() % 32), (PatternStore::Lane)(r() % 7), (int)(r() % 200) - 100);
        if (b % 17 == 0) m.addEvent(juce::MidiMessage::noteOn(1, 24 + (int)(r() % 40), (juce::uint8)(20 + r() % 107)), (int)(r() % 256));
        if (b % 211 == 0) m.addEvent(juce::MidiMessage::allNotesOff(1), 0);
    });
    proc.setPlaying(false);
    CHECK(fz.finite && fz.peak < 0.95f, "12 s of parameter and pattern fuzzing with live MIDI (peak %.3f)", fz.peak);

    // User preset round trip, patterns included.
    const juce::String userName = "__ultra_smoke_test__";
    setParam(proc, "t3Color", 0.81f);
    proc.patterns.setLane(1, 2, 5, PatternStore::Vel, 77);
    CHECK(proc.presets.saveUser(userName, "User", {"test"}), "save user preset");
    setParam(proc, "t3Color", 0.1f);
    proc.patterns.setLane(1, 2, 5, PatternStore::Vel, 0);
    CHECK(proc.presets.apply(proc.presets.indexOf(userName)) && std::abs(val(proc, "t3Color") - 0.81f) < 1e-3f && proc.patterns.lane(1, 2, 5, PatternStore::Vel) == 77,
          "apply user preset (parameters and patterns)");
    // Keep patterns: the kit changes, the steps stay.
    proc.presets.keepPatterns = true;
    proc.patterns.setLane(1, 2, 5, PatternStore::Vel, 33);
    proc.presets.apply(proc.presets.indexOf(userName));
    CHECK(proc.patterns.lane(1, 2, 5, PatternStore::Vel) == 33, "keep patterns leaves the steps alone");
    proc.presets.keepPatterns = false;
    CHECK(proc.presets.deleteUser(proc.presets.indexOf(userName)) && proc.presets.indexOf(userName) < 0, "delete user preset");

    std::printf("%d failure(s)\n", failures);
    return failures;
}
