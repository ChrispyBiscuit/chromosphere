// Exercises the INFRA processor: parameters, state save and restore, Ignite, mono output, parameter fuzzing
// while notes and the arp play, and the user preset round trip.
#include "Ignite.h"
#include "PluginProcessor.h"
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow", "-Wfloat-equal", "-Wnon-virtual-dtor")
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstnoteexpression.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
#include <cstdio>
#include <random>

static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf(__VA_ARGS__); std::printf("\n"); } else { std::printf("PASS  "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static void setParam(InfraProcessor& p, const juce::String& id, float plain) {
    if (auto* prm = p.apvts.getParameter(id)) prm->setValueNotifyingHost(prm->convertTo0to1(plain));
}

struct Stats { bool finite = true; float peak = 0; };

// A host's VST3 event list, for feeding NoteExpression the events FL Studio sends.
struct TestEventList : Steinberg::Vst::IEventList {
    std::vector<Steinberg::Vst::Event> ev;
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID, void** obj) override { *obj = nullptr; return Steinberg::kNoInterface; }
    Steinberg::uint32 PLUGIN_API addRef() override { return 1; }
    Steinberg::uint32 PLUGIN_API release() override { return 1; }
    Steinberg::int32 PLUGIN_API getEventCount() override { return (Steinberg::int32)ev.size(); }
    Steinberg::tresult PLUGIN_API getEvent(Steinberg::int32 i, Steinberg::Vst::Event& e) override { e = ev[(size_t)i]; return Steinberg::kResultOk; }
    Steinberg::tresult PLUGIN_API addEvent(Steinberg::Vst::Event& e) override { ev.push_back(e); return Steinberg::kResultOk; }
};

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

    // Pitch bend reaches the voices: a held A3 (220 Hz) bent fully up with a 12-semitone range plays A4 (440 Hz).
    {
        proc.presets.apply(proc.presets.indexOf("Init"));
        setParam(proc, "aMorph", 0.f); setParam(proc, "fCutoff", 20000.f); setParam(proc, "rvMix", 0.f); setParam(proc, "drift", 0.f);
        setParam(proc, "bendRange", 12.f);
        auto freqWithBend = [&](int wheel) {
            const int block = 256; juce::AudioBuffer<float> buf(1, block);
            int crossings = 0, counted = 0; float prev = 0.f;
            for (int b = 0; b < 200; ++b) {
                juce::MidiBuffer m;
                if (b == 0) { m.addEvent(juce::MidiMessage::pitchWheel(1, wheel), 0); m.addEvent(juce::MidiMessage::noteOn(1, 57, (juce::uint8)100), 1); }
                if (b == 199) m.addEvent(juce::MidiMessage::allNotesOff(1), 0);
                proc.processBlock(buf, m);
                if (b < 40) { prev = buf.getSample(0, block - 1); continue; }   // let the note settle
                for (int i = 0; i < block; ++i) { const float v = buf.getSample(0, i); if (prev <= 0.f && v > 0.f) ++crossings; prev = v; }
                counted += block;
            }
            juce::MidiBuffer m; m.addEvent(juce::MidiMessage::pitchWheel(1, 8192), 0);
            juce::AudioBuffer<float> tail(1, 256);
            for (int b = 0; b < 100; ++b) { proc.processBlock(tail, m); m.clear(); }
            return crossings * 48000.0 / counted;
        };
        const double f0 = freqWithBend(8192), fUp = freqWithBend(16383), fDown = freqWithBend(0);
        CHECK(std::abs(f0 - 220.0) < 4.0, "no bend: %.1f Hz", f0);
        CHECK(std::abs(fUp - 440.0) < 8.0, "bend up 12 st: %.1f Hz", fUp);
        CHECK(std::abs(fDown - 110.0) < 3.0, "bend down 12 st: %.1f Hz", fDown);

        // VST3 note expression, as FL Studio sends slides: a note-on with a note ID, then a Tuning expression on that
        // ID. A slide of +12 semitones from A3 must play A4.
        auto* ext = proc.getVST3ClientExtensions();
        CHECK(ext != nullptr, "VST3 client extensions present");
        void* ctl = nullptr;
        const Steinberg::TUID nec = INLINE_UID(0xB7F8F859, 0x41234872, 0x91169581, 0x4F3721A3);
        CHECK(ext->queryIEditController(nec, &ctl) == Steinberg::kResultOk && ctl != nullptr, "edit controller offers INoteExpressionController");
        if (ctl != nullptr) {
            auto* nc = static_cast<Steinberg::Vst::INoteExpressionController*>(ctl);
            Steinberg::Vst::NoteExpressionTypeInfo info{};
            CHECK(nc->getNoteExpressionCount(0, 0) == 1 && nc->getNoteExpressionInfo(0, 0, 0, info) == Steinberg::kResultTrue
                  && info.typeId == Steinberg::Vst::kTuningTypeID, "advertises the Tuning expression");
        }
        auto freqWithSlide = [&](int note, double tuneNorm, int otherNote) {
            const int block = 256; juce::AudioBuffer<float> buf(1, block);
            int crossings = 0, counted = 0; float prev = 0.f;
            for (int b = 0; b < 200; ++b) {
                juce::MidiBuffer m; TestEventList list;
                if (b == 0) {
                    m.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8)100), 1);
                    if (otherNote >= 0) m.addEvent(juce::MidiMessage::noteOn(1, otherNote, (juce::uint8)100), 1);
                    Steinberg::Vst::Event on{}; on.type = Steinberg::Vst::Event::kNoteOnEvent; on.sampleOffset = 1;
                    on.noteOn.pitch = (Steinberg::int16)note; on.noteOn.noteId = 7; on.noteOn.velocity = 0.8f; list.ev.push_back(on);
                    if (otherNote >= 0) { on.noteOn.pitch = (Steinberg::int16)otherNote; on.noteOn.noteId = 8; list.ev.push_back(on); }
                }
                if (b == 10) {
                    Steinberg::Vst::Event x{}; x.type = Steinberg::Vst::Event::kNoteExpressionValueEvent; x.sampleOffset = 0;
                    x.noteExpressionValue.typeId = Steinberg::Vst::kTuningTypeID; x.noteExpressionValue.noteId = 7; x.noteExpressionValue.value = tuneNorm;
                    list.ev.push_back(x);
                }
                if (b == 199) m.addEvent(juce::MidiMessage::allNotesOff(1), 0);
                ext->processInputEvents(list);
                proc.processBlock(buf, m);
                if (b < 40) { prev = buf.getSample(0, block - 1); continue; }
                for (int i = 0; i < block; ++i) { const float v = buf.getSample(0, i); if (prev <= 0.f && v > 0.f) ++crossings; prev = v; }
                counted += block;
            }
            juce::AudioBuffer<float> tail(1, 256); juce::MidiBuffer none;
            for (int b = 0; b < 100; ++b) proc.processBlock(tail, none);
            return crossings * 48000.0 / counted;
        };
        setParam(proc, "bendRange", 2.f);
        const double fSlide = freqWithSlide(57, 0.5 + 12.0 / 240.0, -1), fFlat = freqWithSlide(57, 0.5 - 12.0 / 240.0, -1);
        CHECK(std::abs(fSlide - 440.0) < 8.0, "note expression +12 st: %.1f Hz", fSlide);
        CHECK(std::abs(fFlat - 110.0) < 3.0, "note expression -12 st: %.1f Hz", fFlat);
        const double fNext = freqWithBend(8192);
        CHECK(std::abs(fNext - 220.0) < 4.0, "the next note starts in tune: %.1f Hz", fNext);
    }

    // Glide: in every voice mode, a second, overlapping note starts at the first note's pitch and slides up to its
    // own (A3 -> A4 with a 0.3 s glide: well below 440 Hz in the first 40 ms, at 440 Hz after a second).
    for (const char* mode : {"Poly", "Mono", "Legato"}) {
        proc.presets.apply(proc.presets.indexOf("Init"));
        setParam(proc, "aMorph", 0.f); setParam(proc, "fCutoff", 20000.f); setParam(proc, "rvMix", 0.f); setParam(proc, "drift", 0.f);
        setParam(proc, "glide", 0.3f);
        auto* vm = dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter("voiceMode"));
        vm->setValueNotifyingHost(vm->convertTo0to1((float)vm->choices.indexOf(mode)));
        const int block = 256; juce::AudioBuffer<float> buf(1, block);
        auto measure = [&](int fromBlock, int toBlock, int totalBlocks) {
            int crossings = 0, counted = 0; float prev = 0.f;
            for (int b = 0; b < totalBlocks; ++b) {
                juce::MidiBuffer m;
                if (b == 0) m.addEvent(juce::MidiMessage::noteOn(1, 57, (juce::uint8)100), 0);
                if (b == 40) m.addEvent(juce::MidiMessage::noteOn(1, 69, (juce::uint8)100), 0);    // overlaps the first
                if (b == 41) m.addEvent(juce::MidiMessage::noteOff(1, 57), 0);
                proc.processBlock(buf, m);
                if (b < fromBlock || b >= toBlock) { prev = buf.getSample(0, block - 1); continue; }
                for (int i = 0; i < block; ++i) { const float v = buf.getSample(0, i); if (prev <= 0.f && v > 0.f) ++crossings; prev = v; }
                counted += block;
            }
            juce::MidiBuffer off; off.addEvent(juce::MidiMessage::allNotesOff(1), 0);
            juce::AudioBuffer<float> tail(1, 256);
            for (int b = 0; b < 100; ++b) { proc.processBlock(tail, off); off.clear(); }
            return crossings * 48000.0 / std::max(1, counted);
        };
        const double early = measure(40, 48, 48), late = measure(240, 280, 280);
        CHECK(early < 380.0 && early > 200.0, "%s glide: %.0f Hz in the first 40 ms of the slide", mode, early);
        CHECK(std::abs(late - 440.0) < 8.0, "%s glide arrives: %.1f Hz", mode, late);
    }

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
