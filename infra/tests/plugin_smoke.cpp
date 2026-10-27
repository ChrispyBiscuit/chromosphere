// Exercises the INFRA processor: parameters, state save and restore, Ignite, mono output, parameter fuzzing
// while notes and the arp play, the user preset round trip, and WAVE (a sung voice on the input bus).
#include "Ignite.h"
#include "PluginProcessor.h"
#include "synth_voice.h"
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
    CHECK(proc.getLatencySamples() == 0 && proc.acceptsMidi() && proc.producesMidi(), "instrument: MIDI in, MIDI out (WAVE), no latency");

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


    // ---- WAVE: a sung voice on the input bus ----------------------------------------------------------------
    {
        InfraProcessor wp;
        CHECK(wp.getBusCount(true) == 1 && wp.getBus(true, 0)->getName() == "WAVE In", "input bus \"WAVE In\"");
        auto layout = wp.getBusesLayout();
        bool layouts = true;
        for (auto in : {juce::AudioChannelSet::disabled(), juce::AudioChannelSet::mono(), juce::AudioChannelSet::stereo()}) {
            layout.inputBuses.getReference(0) = in; layouts &= wp.checkBusesLayoutSupported(layout);
        }
        layout.inputBuses.getReference(0) = juce::AudioChannelSet::create5point1();
        CHECK(layouts && !wp.checkBusesLayoutSupported(layout), "WAVE In: disabled, mono or stereo");

        wp.setPlayConfigDetails(1, 2, 48000.0, 256);
        wp.prepareToPlay(48000.0, 256);
        wp.presets.apply(wp.presets.indexOf("Init"));
        setParam(wp, "aMorph", 0.f); setParam(wp, "fCutoff", 20000.f); setParam(wp, "rvMix", 0.f); setParam(wp, "drift", 0.f);
        wavetest::Note sung; sung.start = 0.1; sung.dur = 1.6; sung.midi = 57.f;
        const auto voice = wavetest::render({sung}, 2.4, 48000.0);
        // Runs the voice through; returns the output's frequency over [1.0, 1.5) s, the MIDI out notes and the peak.
        struct WaveRun { double hz = 0; std::vector<int> ons, offs; float peak = 0; bool finite = true; };
        auto sing = [&](const std::vector<float>& in) {
            WaveRun w; const int block = 256;
            juce::AudioBuffer<float> buf(2, block);
            int crossings = 0, counted = 0; float prev = 0.f;
            for (size_t off = 0; off + block <= in.size(); off += block) {
                buf.clear();
                buf.copyFrom(0, 0, in.data() + off, block);
                juce::MidiBuffer m;
                wp.processBlock(buf, m);
                for (const auto meta : m) {
                    const auto msg = meta.getMessage();
                    if (msg.isNoteOn()) w.ons.push_back(msg.getNoteNumber()); else if (msg.isNoteOff()) w.offs.push_back(msg.getNoteNumber());
                }
                for (int i = 0; i < block; ++i) {
                    const float v = buf.getSample(0, i);
                    w.finite &= std::isfinite(v); w.peak = std::max(w.peak, std::abs(v));
                    const double t = (double)(off + (size_t)i) / 48000.0;
                    if (t >= 1.0 && t < 1.5) { if (prev <= 0.f && v > 0.f) ++crossings; ++counted; }
                    prev = v;
                }
            }
            w.hz = crossings * 48000.0 / std::max(1, counted);
            return w;
        };
        auto off = sing(voice);
        CHECK(off.peak < 1e-4f, "WAVE off: the input is ignored (peak %.2g)", off.peak);
        setParam(wp, "wvOn", 1.f);
        auto on = sing(voice);
        CHECK(on.finite && on.peak > 0.01f && std::abs(on.hz - 220.0) < 4.0 && on.ons.empty(),
              "WAVE on: a sung A3 plays A3 (%.1f Hz, peak %.3f); no MIDI out unless asked", on.hz, on.peak);
        setParam(wp, "wvMidiOut", 1.f); setParam(wp, "wvOct", 1.f);
        auto out = sing(voice);
        CHECK(out.ons.size() == 1 && out.ons[0] == 69 && out.offs.size() == 1 && std::abs(out.hz - 440.0) < 8.0,
              "WAVE MIDI out: one note, A4 with Octave +1 (%zu on, %.1f Hz)", out.ons.size(), out.hz);
        setParam(wp, "wvOct", 0.f); setParam(wp, "wvMode", 1.f); setParam(wp, "wvScale", 1.f); setParam(wp, "wvKey", 0.f);
        auto ch = sing(voice);
        CHECK(ch.ons.size() == 3 && ch.ons[0] == 57 && ch.ons[1] == 60 && ch.ons[2] == 64 && ch.offs.size() == 3,
              "WAVE chords in C major: A sung plays A minor (%zu notes)", ch.ons.size());

        // Presets, Ignite and A/B leave the WAVE settings alone; user presets do not store them.
        setParam(wp, "wvKey", 7.f); setParam(wp, "wvStrum", 25.f);
        auto wv = [&](const char* id) { auto* q = wp.apvts.getParameter(id); return q->convertFrom0to1(q->getValue()); };
        wp.presets.apply(wp.presets.indexOf("Orbit Arp"));
        juce::Random rr(3); auto patch = ignite::generate(rr, 0);
        wp.presets.applyGenerated(patch.params, patch.name, patch.category);
        wp.presets.toggleAB(); setParam(wp, "wvKey", 2.f); wp.presets.toggleAB();
        CHECK(wv("wvOn") > 0.5f && (int)wv("wvMode") == 1 && (int)wv("wvKey") == 2 && std::abs(wv("wvStrum") - 25.f) < 0.1f,
              "presets, Ignite and A/B keep the WAVE settings (key %d)", (int)wv("wvKey"));
        const juce::String waveUser = "__infra_wave_smoke__";
        wp.presets.saveUser(waveUser, "User", {});
        const auto file = wp.presets.userFolder().getChildFile(waveUser + ".cipreset");
        CHECK(file.existsAsFile() && !file.loadFileAsString().contains("\"wv"), "user presets do not store WAVE settings");
        wp.presets.deleteUser(wp.presets.indexOf(waveUser));

        // Switching WAVE off mid-note releases its notes (MIDI out too); the synth falls silent.
        wp.presets.apply(wp.presets.indexOf("Init"));
        setParam(wp, "wvMode", 0.f); setParam(wp, "rvMix", 0.f);
        std::vector<int> ons, offs;
        juce::AudioBuffer<float> buf(2, 256);
        for (int b = 0; b < 400; ++b) {
            buf.clear();
            if ((size_t)(b + 1) * 256 <= voice.size()) buf.copyFrom(0, 0, voice.data() + (size_t)b * 256, 256);
            if (b == 150) setParam(wp, "wvOn", 0.f);
            juce::MidiBuffer m;
            wp.processBlock(buf, m);
            for (const auto meta : m) { const auto msg = meta.getMessage(); if (msg.isNoteOn()) ons.push_back(msg.getNoteNumber()); else if (msg.isNoteOff()) offs.push_back(msg.getNoteNumber()); }
        }
        CHECK(ons.size() == 1 && offs.size() == 1 && wp.synth.activeVoices.load() == 0 && buf.getMagnitude(0, 0, 256) < 1e-4f,
              "WAVE switched off mid-note: its note is released (%zu on, %zu off)", ons.size(), offs.size());

        // Fuzz with a voice on the input: every WAVE setting and the rest, at random.
        setParam(wp, "wvOn", 1.f);
        std::mt19937 rz(9);
        auto all = wp.getParameters();
        std::vector<wavetest::Note> phrase;
        for (int i = 0; i < 30; ++i) { wavetest::Note nn; nn.start = i * 0.3; nn.dur = 0.25; nn.midi = 45.f + (float)(rz() % 30); nn.vibCents = 20.f; phrase.push_back(nn); }
        const auto longVoice = wavetest::render(phrase, 9.0, 48000.0);
        float peak = 0.f; bool finite = true;
        for (size_t b = 0; (b + 1) * 256 <= longVoice.size(); ++b) {
            if (b % 5 == 0) {
                auto* prm = dynamic_cast<juce::RangedAudioParameter*>(all[(int)(rz() % (unsigned)all.size())]);
                const juce::String id = prm->getParameterID();
                if (id != "outDb" && id != "wvOn" && id != "wvMonitor") prm->setValueNotifyingHost((rz() % 1000) / 999.f);
                if (b % 40 == 0) setParam(wp, (rz() % 2) ? "wvMode" : "wvHold", (float)(rz() % 2));
            }
            buf.clear();
            buf.copyFrom(0, 0, longVoice.data() + b * 256, 256);
            juce::MidiBuffer m;
            wp.processBlock(buf, m);
            for (int i = 0; i < 256; ++i) { const float v = buf.getSample(0, i); finite &= std::isfinite(v); peak = std::max(peak, std::abs(v)); }
        }
        CHECK(finite && peak < 0.95f, "9 s of WAVE fuzzing with a sung phrase (peak %.3f)", peak);
    }

    std::printf("%d failure(s)\n", failures);
    return failures;
}
