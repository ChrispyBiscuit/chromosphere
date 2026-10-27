// Renders presets with a phrase that suits their role, for listening and for measurement:
//   infra_audition --role bass|lead --out dir [--category Bass] [--only name[,name...]] [--presets file.json]
// --presets renders the presets in a JSON file (the presets.json format; see dump_presets.py) instead of the factory
// set, so a preset file being designed can be heard and measured without rebuilding the plug-in.
// Writes <dir>/<preset>.f32 (interleaved stereo float32, 48 kHz) for every preset selected.
//   bass: a 120 bpm line in octaves 1-2 (E1 to D2): staccato eighths, held notes, an octave jump, overlapping
//         (legato) pairs, velocity accents.
//   lead: a melody in octaves 4-5 with long notes, a legato run, a leap, and the mod wheel brought in halfway.
//   chord: a four-chord progression (Am9 Fmaj7 C Gsus, 4-5 note voicings around middle C): two bars held, then the
//          same chords as offbeat stabs.
//   melody: a melody in octaves 4-5 for plucks, keys, bells and mallets: quick notes, repeated notes, a held note.
#include "PluginProcessor.h"
#include <cstdio>

namespace {
struct Ev { double beat; bool on; int note; int vel; };

std::vector<Ev> phrase(const juce::String& role) {
    std::vector<Ev> e;
    auto note = [&](double start, double len, int n, int vel = 100) { e.push_back({start, true, n, vel}); e.push_back({start + len, false, n, 0}); };
    if (role == "bass") {
        // Bar 1: driving eighths on E1 with accents.
        for (int i = 0; i < 8; ++i) note(i * 0.5, 0.35, 28, i % 4 == 0 ? 120 : 80);
        // Bar 2: G1 held, A1, then an octave jump to E2 and back.
        note(4.0, 1.4, 31); note(5.5, 0.4, 33); note(6.0, 0.45, 40, 120); note(6.5, 0.45, 28);
        // Bar 3: overlapping (legato) pairs: a slide up and down.
        note(8.0, 1.1, 28); note(9.0, 1.0, 35); note(10.0, 0.6, 36); note(10.5, 1.0, 31);
        // Bar 4: a long low note to hear the sustain and the sub.
        note(12.0, 3.0, 26, 110);
    } else if (role == "chord") {
        const int ch[4][5] = {{57, 60, 64, 67, 71}, {53, 57, 60, 64, 0}, {48, 55, 60, 64, 67}, {55, 60, 62, 67, 0}};
        for (int c = 0; c < 4; ++c) for (int n : ch[c]) if (n) note(c * 2.0, 1.9, n, 90);              // held
        for (int c = 0; c < 4; ++c) for (int k = 0; k < 3; ++k) for (int n : ch[c]) if (n) note(8.0 + c * 1.5 + k * 0.5 + 0.25, 0.2, n, k == 0 ? 110 : 85);   // stabs
    } else if (role == "melody") {
        const int mel[] = {72, 76, 79, 84, 83, 79, 76, 74, 72, 72, 74, 76, 79, 76};
        const double len[] = {0.5, 0.5, 0.5, 1.0, 0.5, 0.5, 0.5, 0.5, 0.25, 0.25, 0.5, 0.5, 1.0, 2.0};
        double t = 0.0;
        for (int i = 0; i < 14; ++i) { note(t, len[i] * 0.85, mel[i], i % 4 == 0 ? 110 : 90); t += len[i]; }
    } else {
        note(0.0, 1.5, 69); note(1.5, 0.5, 72); note(2.0, 1.0, 76); note(3.0, 1.0, 74);
        // Legato run (each note overlaps the next).
        const int run[] = {72, 74, 76, 79, 81};
        for (int i = 0; i < 5; ++i) note(4.0 + i * 0.25, 0.3, run[i]);
        note(5.25, 2.75, 84, 110);                         // a leap up and a long note (vibrato comes in here)
        note(8.0, 0.5, 79); note(8.5, 0.5, 76); note(9.0, 3.0, 77);
    }
    std::sort(e.begin(), e.end(), [](const Ev& a, const Ev& b) { return a.beat < b.beat || (a.beat == b.beat && !a.on && b.on); });
    return e;
}
}  // namespace

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    juce::String role = "bass", category, only; juce::File out = juce::File::getCurrentWorkingDirectory(), file;
    for (int i = 1; i + 1 < argc; i += 2) {
        const juce::String a = argv[i];
        if (a == "--role") role = argv[i + 1];
        else if (a == "--out") out = juce::File::getCurrentWorkingDirectory().getChildFile(argv[i + 1]);
        else if (a == "--category") category = argv[i + 1];
        else if (a == "--only") only = argv[i + 1];
        else if (a == "--presets") file = juce::File::getCurrentWorkingDirectory().getChildFile(argv[i + 1]);
    }
    out.createDirectory();
    const double fs = 48000.0, bpm = 120.0; const int block = 256;
    InfraProcessor proc;
    proc.setPlayConfigDetails(0, 2, fs, block);
    proc.prepareToPlay(fs, block);
    const auto evs = phrase(role);
    const double beats = role == "bass" ? 16.0 : role == "chord" ? 14.0 : role == "melody" ? 10.0 : 13.0;
    const int total = (int)((beats * 60.0 / bpm + 1.5) * fs);
    std::vector<Preset> presets = proc.presets.presets();
    if (file != juce::File()) {
        presets.clear();
        const auto root = juce::JSON::parse(file.loadFileAsString());
        if (auto* list = root["presets"].getArray())
            for (const auto& item : *list) {
                Preset p; p.name = item["name"].toString(); p.category = item["category"].toString();
                if (auto* o = item["params"].getDynamicObject()) p.params = o->getProperties();
                presets.push_back(std::move(p));
            }
    }
    int count = 0;
    for (int i = 0; i < (int)presets.size(); ++i) {
        const auto& p = presets[(size_t)i];
        if (category.isNotEmpty() && p.category != category) continue;
        if (only.isNotEmpty() && !juce::StringArray::fromTokens(only, ",", "").contains(p.name)) continue;
        proc.presets.applyGenerated(p.params, p.name, p.category);
        proc.prepareToPlay(fs, block);                   // a clean start: no tail from the previous preset
        std::vector<float> data((size_t)total * 2);
        juce::AudioBuffer<float> buf(2, block);
        size_t next = 0;
        for (int off = 0; off < total; off += block) {
            juce::MidiBuffer midi;
            const double t0 = off / fs * bpm / 60.0, t1 = (off + block) / fs * bpm / 60.0;
            if (role == "lead") {                        // the mod wheel rises from beat 5.5 to 7
                const double w = juce::jlimit(0.0, 1.0, (t0 - 5.5) / 1.5) * 0.7;
                midi.addEvent(juce::MidiMessage::controllerEvent(1, 1, (int)(w * 127)), 0);
            }
            while (next < evs.size() && evs[next].beat < t1) {
                const int at = juce::jlimit(0, block - 1, (int)((evs[next].beat * 60.0 / bpm) * fs) - off);
                midi.addEvent(evs[next].on ? juce::MidiMessage::noteOn(1, evs[next].note, (juce::uint8)evs[next].vel)
                                           : juce::MidiMessage::noteOff(1, evs[next].note), at);
                ++next;
            }
            proc.processBlock(buf, midi);
            for (int s = 0; s < block && off + s < total; ++s) {
                data[(size_t)(off + s) * 2] = buf.getSample(0, s);
                data[(size_t)(off + s) * 2 + 1] = buf.getSample(1, s);
            }
        }
        out.getChildFile(juce::File::createLegalFileName(p.name) + ".f32").replaceWithData(data.data(), data.size() * sizeof(float));
        ++count;
    }
    std::printf("%d presets rendered (%s phrase) to %s\n", count, role.toRawUTF8(), out.getFullPathName().toRawUTF8());
    return 0;
}
