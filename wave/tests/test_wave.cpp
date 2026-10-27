// DSP tests for the WAVE engine (no JUCE): the gates of docs/PRD.md Section 8. Each check prints PASS / FAIL; the
// exit code is the failure count.
#include "synth_voice.h"
#include "wave/engine.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <vector>

using namespace wave;
using wavetest::Note;

static int failures = 0;
#define CHECK(cond, ...) do { if (cond) std::printf("PASS  "); else { std::printf("FAIL  "); ++failures; } std::printf(__VA_ARGS__); std::printf("\n"); } while (0)

namespace {
struct Ev { int64_t t; Event::Type type; int note; float value; };

std::vector<Ev> run(Engine& e, const std::vector<float>& in, int block = 256) {
    std::vector<Ev> out;
    std::vector<Event> buf(4096);
    for (size_t off = 0; off < in.size(); off += (size_t)block) {
        const int n = (int)std::min((size_t)block, in.size() - off);
        const int k = e.process(in.data() + off, n, buf.data(), (int)buf.size());
        for (int i = 0; i < k; ++i) out.push_back({(int64_t)off + buf[(size_t)i].offset, buf[(size_t)i].type, buf[(size_t)i].note, buf[(size_t)i].value});
    }
    return out;
}
std::vector<Ev> runWith(const Params& p, const std::vector<float>& in, double fs = 48000.0, int block = 256, Engine* keep = nullptr) {
    Engine local;
    Engine& e = keep ? *keep : local;
    e.setParams(p);
    e.prepare(fs);
    return run(e, in, block);
}
std::vector<Ev> only(const std::vector<Ev>& v, Event::Type t) { std::vector<Ev> r; for (auto& e : v) if (e.type == t) r.push_back(e); return r; }
std::string notes(const std::vector<Ev>& v) { std::string s; for (auto& e : v) if (e.type == Event::NoteOn) s += std::to_string(e.note) + " "; return s; }
bool balanced(const std::vector<Ev>& v) {
    int on[128] = {};
    for (auto& e : v) { if (e.type == Event::NoteOn) ++on[e.note]; else if (e.type == Event::NoteOff) --on[e.note]; }
    for (int x : on) if (x != 0) return false;
    return true;
}

// Mean tracker pitch over [a, b) seconds of a signal.
float trackedPitch(const std::vector<float>& sig, double fs, int range, double a, double b, float* worstDev = nullptr) {
    Tracker t; t.prepare(fs); t.setRange(range);
    double sum = 0.0; int n = 0; float lo = 1e9f, hi = -1e9f;
    for (size_t i = 0; i < sig.size(); ++i)
        if (t.push(sig[i]) && i >= (size_t)(a * fs) && i < (size_t)(b * fs) && t.frame().pitch > 0.f) {
            sum += t.frame().pitch; ++n;
            lo = std::min(lo, t.frame().pitch); hi = std::max(hi, t.frame().pitch);
        }
    if (worstDev) *worstDev = n ? std::max(hi - (float)(sum / n), (float)(sum / n) - lo) : 99.f;
    return n ? (float)(sum / n) : -1.f;
}

void testPitch() {
    const double fs = 48000.0;
    float worstErr = 0.f;
    bool all = true;
    for (int m = 40; m <= 84; m += 4) {                    // 82 Hz .. 1047 Hz
        Note n; n.start = 0.05; n.dur = 0.6; n.midi = (float)m; n.vowel = m % 5;
        const auto sig = wavetest::render({n}, 0.7, fs, (uint32_t)m);
        const float p = trackedPitch(sig, fs, 0, 0.25, 0.55);
        const float err = std::abs(p - (float)m) * 100.f;
        worstErr = std::max(worstErr, err);
        all &= p > 0.f && err < 10.f;
    }
    CHECK(all, "pitch: steady voices from E2 to C6 within 10 cents (worst %.1f cents)", worstErr);

    Note w; w.start = 0.05; w.dur = 0.6; w.midi = 50.f; w.fundamental = 0.08f; w.vowel = 1;
    const float pw = trackedPitch(wavetest::render({w}, 0.7, fs), fs, 0, 0.25, 0.55);
    CHECK(std::abs(pw - 50.f) < 0.15f, "pitch: a weak fundamental (8%%) is not an octave error (tracked %.2f for 50)", pw);

    // High soprano: almost all the energy in the 2nd harmonic, the fundamental 18 dB under it (found on a real
    // recording, Essentia's long_voice.flac).
    bool sop = true; float worst = 0.f;
    for (int m : {72, 74, 76, 78, 80, 82}) {
        Note s; s.start = 0.05; s.dur = 0.6; s.midi = (float)m; s.fundamental = 0.1f; s.vowel = 1; s.vibCents = 30.f;
        const float ps = trackedPitch(wavetest::render({s}, 0.7, fs, (uint32_t)m), fs, 0, 0.25, 0.55);
        worst = std::max(worst, std::abs(ps - (float)m));
        sop &= std::abs(ps - (float)m) < 0.3f;
    }
    CHECK(sop, "pitch: soprano C5 to A#5 with the 2nd harmonic dominant, no octave errors (worst %.2f semitones)", worst);

    // The 3rd harmonic dominant (seen at C#5 on the same recording): vowel "i" puts F2 near it.
    bool third = true; worst = 0.f;
    for (int m : {70, 73, 76}) {
        Note s; s.start = 0.05; s.dur = 0.6; s.midi = (float)m; s.fundamental = 0.12f; s.vowel = 2; s.vibCents = 30.f;
        const float ps = trackedPitch(wavetest::render({s}, 0.7, fs, (uint32_t)m), fs, 0, 0.25, 0.55);
        worst = std::max(worst, std::abs(ps - (float)m));
        third &= std::abs(ps - (float)m) < 0.3f;
    }
    CHECK(third, "pitch: soprano with the 3rd harmonic dominant, no twelfth errors (worst %.2f semitones)", worst);
}

void testLatency() {
    struct Case { int range; int midi; double fs; double limitMs; const char* what; };
    const Case cases[] = {
        {1, 57, 48000, 30, "Mid range, A3, 48 kHz"}, {1, 45, 48000, 30, "Mid range, A2, 48 kHz"},
        {0, 40, 48000, 40, "Low range, E2, 48 kHz"}, {2, 64, 48000, 30, "High range, E4, 48 kHz"},
        {1, 57, 44100, 30, "Mid range, A3, 44.1 kHz"}, {1, 57, 96000, 30, "Mid range, A3, 96 kHz"},
        {1, 57, 192000, 30, "Mid range, A3, 192 kHz"},
    };
    for (const auto& c : cases) {
        Note n; n.start = 0.3; n.dur = 0.5; n.midi = (float)c.midi;
        Params p; p.range = c.range;
        const auto ev = only(runWith(p, wavetest::render({n}, 1.0, c.fs), c.fs), Event::NoteOn);
        const double ms = ev.empty() ? 1e9 : (ev[0].t / c.fs - n.start) * 1000.0;
        CHECK(ev.size() == 1 && ev[0].note == c.midi && ms <= c.limitMs, "latency: %s, note %d after %.1f ms (limit %.0f)",
              c.what, ev.empty() ? -1 : ev[0].note, ms, c.limitMs);
    }
}

void testSegmentation() {
    const double fs = 48000.0;
    Params p;
    {   // Vibrato on a held note.
        Note n; n.start = 0.1; n.dur = 2.0; n.midi = 57.f; n.vibCents = 50.f;
        const auto ev = runWith(p, wavetest::render({n}, 2.4, fs));
        CHECK(only(ev, Event::NoteOn).size() == 1 && balanced(ev), "vibrato +/-50 cents at 5.5 Hz: one note (%s)", notes(ev).c_str());
    }
    {   // A sung scale with gaps.
        std::vector<Note> ns;
        const int sc[] = {60, 62, 64, 65, 67, 69, 71, 72};
        for (int i = 0; i < 8; ++i) { Note n; n.start = 0.1 + i * 0.36; n.dur = 0.3; n.midi = (float)sc[i]; n.vowel = i % 5; ns.push_back(n); }
        const auto ev = runWith(p, wavetest::render(ns, 3.2, fs));
        CHECK(notes(ev) == "60 62 64 65 67 69 71 72 " && balanced(ev), "sung C major scale with gaps: %s", notes(ev).c_str());
    }
    {   // A glide (legato).
        Note n; n.start = 0.1; n.dur = 1.0; n.midi = 60.f; n.midiEnd = 67.f; n.glide = 0.08; n.glideAt = 0.5;
        const auto sig = wavetest::render({n}, 1.3, fs);
        const auto ev = runWith(p, sig);
        const auto ons = only(ev, Event::NoteOn), offs = only(ev, Event::NoteOff);
        const bool legato = ons.size() == 2 && offs.size() >= 1 && offs[0].note == 60 && offs[0].t >= ons[1].t;
        CHECK(notes(ev) == "60 67 " && legato && balanced(ev), "glide C4 to G4: two notes, the second starts before the first ends (%s)", notes(ev).c_str());
    }
    {   // Re-articulation on one pitch ("da da da"): level dips without silence.
        Note n; n.start = 0.1; n.dur = 1.3; n.midi = 55.f;
        auto sig = wavetest::render({n}, 1.6, fs);
        for (double t : {0.5, 0.9}) {
            for (size_t i = (size_t)((t - 0.03) * fs); i < (size_t)((t + 0.03) * fs); ++i) {
                const double x = std::abs((double)i / fs - t) / 0.03;  // 0 at the centre of the dip
                sig[i] *= (float)(0.08 + 0.92 * x * x);
            }
        }
        const auto ev = runWith(p, sig);
        CHECK(only(ev, Event::NoteOn).size() == 3 && balanced(ev), "re-articulation: three notes on one pitch (%s)", notes(ev).c_str());
    }
    {   // Fricative onsets ("sa"): a noise burst before each vowel plays nothing by itself.
        std::vector<Note> ns;
        for (int i = 0; i < 4; ++i) { Note n; n.start = 0.2 + i * 0.5; n.dur = 0.3; n.midi = 52.f + i * 5; ns.push_back(n); }
        auto sig = wavetest::render(ns, 2.3, fs);
        uint32_t r = 3; float hp = 0.f, prev = 0.f;
        for (int i = 0; i < 4; ++i)
            for (size_t k = (size_t)((0.12 + i * 0.5) * fs); k < (size_t)((0.2 + i * 0.5) * fs); ++k) {
                r = r * 1664525u + 1013904223u;
                const float w = (r >> 8) / 16777216.f * 2.f - 1.f;
                hp = 0.7f * (hp + w - prev); prev = w;             // hissy (high-passed) noise, like "s"
                sig[k] += hp * 0.15f;
            }
        const auto ev = runWith(p, sig);
        CHECK(notes(ev) == "52 57 62 67 " && balanced(ev), "fricatives before the vowels: only the sung notes (%s)", notes(ev).c_str());
    }
    {   // A long phrase: random pitches, lengths, gaps, legato steps, vowels, vibrato and levels.
        uint32_t r = 11;
        auto rnd = [&] { r = r * 1664525u + 1013904223u; return (r >> 8) / 16777216.f; };
        std::vector<Note> ns; std::vector<int> want;
        double t = 0.1; int prevMidi = 57;
        for (int i = 0; i < 40; ++i) {
            Note n; n.start = t; n.dur = 0.18 + rnd() * 0.4;
            int m = prevMidi + (int)std::lround((rnd() - 0.5f) * 10.f);
            m = std::clamp(m, 48, 72);
            if (m == prevMidi) m += 2;
            n.midi = (float)m; n.vowel = (int)(rnd() * 5) % 5; n.vibCents = rnd() * 35.f; n.db = -24.f + rnd() * 16.f;
            ns.push_back(n); want.push_back(m); prevMidi = m;
            t += n.dur + 0.06 + rnd() * 0.2;
        }
        const auto ev = only(runWith(p, wavetest::render(ns, t + 0.3, fs)), Event::NoteOn);
        // Match played notes to sung notes in order (a simple alignment: each sung note must appear, extras count).
        size_t j = 0; int hit = 0;
        for (int w : want) { while (j < ev.size() && ev[j].note != w) ++j; if (j < ev.size()) { ++hit; ++j; } }
        const int extra = (int)ev.size() - hit;
        CHECK(hit == 40 && extra == 0, "40-note random phrase: %d of 40 notes right, %d extra", hit, extra);
    }
    {   // Noise and silence.
        std::vector<float> noise((size_t)(2.0 * fs));
        uint32_t r = 7; double b0 = 0, b1 = 0, b2 = 0;
        for (auto& x : noise) { r = r * 1664525u + 1013904223u; x = ((r >> 8) / 16777216.f * 2.f - 1.f) * 0.17f; }
        auto pink = noise;
        for (auto& x : pink) { b0 = 0.99765 * b0 + x * 0.099; b1 = 0.963 * b1 + x * 0.2965; b2 = 0.57 * b2 + x * 1.0527; x = (float)(b0 + b1 + b2 + x * 0.1848) * 0.35f; }
        const auto e1 = runWith(p, noise), e2 = runWith(p, pink), e3 = runWith(p, std::vector<float>((size_t)fs, 0.f));
        CHECK(only(e1, Event::NoteOn).empty() && only(e2, Event::NoteOn).empty() && only(e3, Event::NoteOn).empty(),
              "white and pink noise at -20 dBFS and silence: no notes (%zu, %zu, %zu)", only(e1, Event::NoteOn).size(),
              only(e2, Event::NoteOn).size(), only(e3, Event::NoteOn).size());
    }
    {   // Quiet voice below the gate.
        Note n; n.start = 0.1; n.dur = 0.5; n.midi = 57.f; n.db = -60.f;
        CHECK(only(runWith(p, wavetest::render({n}, 0.8, fs)), Event::NoteOn).empty(), "a voice under the gate plays nothing");
    }
}

void testQuantizeAndFollow() {
    CHECK(quantize(61.4f, 0, Scale::Major) == 62 && quantize(60.4f, 0, Scale::Major) == 60 && quantize(66.f, 0, Scale::Major) == 65,
          "quantize to C major: 61.4 -> D, 60.4 -> C, F# -> F (lower on a tie)");
    CHECK(quantize(60.6f, 0, Scale::Chromatic, 60) == 60 && quantize(60.75f, 0, Scale::Chromatic, 60) == 61,
          "hysteresis: the held note holds to 0.7 semitones, then moves");
    CHECK(quantize(61.4f, 0, Scale::Major, 60) == 60 && quantize(61.5f, 0, Scale::Major, 60) == 62 && quantize(64.8f, 0, Scale::Major, 64) == 65,
          "hysteresis scales with the gap: C holds to 61.44 against D; E to F moves at 64.72");
    CHECK(quantize(70.f, 9, Scale::MinorPentatonic) == 69 && quantize(71.f, 9, Scale::MinorPentatonic) == 72,
          "A minor pentatonic: Bb -> A, B -> C");

    const double fs = 48000.0;
    Note n; n.start = 0.1; n.dur = 1.5; n.midi = 61.f; n.vibCents = 40.f;
    Params p; p.key = 0; p.scale = Scale::Major;
    auto ev = runWith(p, wavetest::render({n}, 1.8, fs));
    const auto ons = only(ev, Event::NoteOn);
    CHECK(ons.size() == 1 && (ons[0].note == 60 || ons[0].note == 62) && only(ev, Event::Tuning).empty(),
          "C# with vibrato in C major: one note, C or D, locked (Follow 0): %s", notes(ev).c_str());
    n.midi = 64.3f;                                      // E, 30 cents sharp
    p.follow = 1.f;
    ev = runWith(p, wavetest::render({n}, 1.8, fs));
    float lo = 9, hi = -9;
    for (auto& e : only(ev, Event::Tuning)) if (e.t > 0.4 * fs && e.t < 1.4 * fs) { lo = std::min(lo, e.value); hi = std::max(hi, e.value); }
    CHECK(notes(ev) == "64 " && lo > -0.25f && lo < 0.05f && hi > 0.55f && hi < 0.85f,
          "Follow 100%%: tuning tracks a sharp E (+0.3) with its vibrato (%.2f .. %.2f)", lo, hi);
}

void testChords() {
    const char* expectC[7] = {"", "m", "m", "", "", "m", "dim"};
    bool ok = true;
    ChordSpec s; s.key = 0; s.scale = Scale::Major; s.voicing = Voicing::Close;
    const int deg[7] = {60, 62, 64, 65, 67, 69, 71};
    for (int i = 0; i < 7; ++i) { const auto c = buildChord(deg[i], s, nullptr); ok &= std::strcmp(chordSuffix(c.mask), expectC[i]) == 0 && c.root == deg[i]; }
    CHECK(ok, "C major triads: C Dm Em F G Am Bdim");
    s.type = ChordType::Seventh;
    const auto g7 = buildChord(67, s, nullptr), cm7 = buildChord(60, s, nullptr);
    CHECK(std::strcmp(chordSuffix(g7.mask), "7") == 0 && std::strcmp(chordSuffix(cm7.mask), "maj7") == 0, "sevenths in C major: G7, Cmaj7");

    bool inKey = true;
    for (int sc = (int)Scale::Major; sc <= (int)Scale::Locrian; ++sc)
        for (int t = 0; t < (int)ChordType::Power; ++t)
            for (int k = 0; k < 12; k += 5)
                for (int d = 0; d < 7; ++d) {
                    ChordSpec q; q.key = k; q.scale = (Scale)sc; q.type = (ChordType)t; q.voicing = Voicing::Close;
                    const auto tones = scaleTones(q.scale);
                    const int sung = 60 + k + tones.pc[d];
                    const auto c = buildChord(sung, q, nullptr);
                    for (int i = 0; i < c.n; ++i) inKey &= inScale(c.notes[i], k, q.scale);
                }
    CHECK(inKey, "every chord type on every degree of the seven-note scales stays in the key");

    bool top = true;
    s.type = ChordType::Triad; s.role = Role::Top;
    for (int m = 55; m <= 79; ++m) {
        if (!inScale(m, 0, Scale::Major)) continue;
        const auto c = buildChord(m, s, nullptr);
        top &= c.n >= 3 && c.notes[c.n - 1] == m && c.notes[0] > m - 12;
    }
    const auto cE = buildChord(64, s, nullptr), cB = buildChord(71, s, nullptr), cA = buildChord(69, s, nullptr);
    CHECK(top && cE.root % 12 == 0 && cB.root % 12 == 7 && cA.root % 12 == 5,
          "Top: the sung note stays highest; E is harmonised by C, B by G, A by F");

    // Voice leading over I-vi-IV-V (roots sung).
    auto movement = [](Voicing v) {
        ChordSpec q; q.key = 0; q.scale = Scale::Major; q.voicing = v;
        Chord prev; int total = 0;
        for (int r : {60, 69, 65, 67, 60}) {
            const auto c = buildChord(r, q, prev.n ? &prev : nullptr);
            if (prev.n) for (int i = 0; i < std::min(c.n, prev.n); ++i) total += std::abs(c.notes[i] - prev.notes[i]);
            prev = c;
        }
        return total;
    };
    CHECK(movement(Voicing::Smooth) < movement(Voicing::Close) / 2, "Smooth voicing moves less than Close over I-vi-IV-V-I (%d vs %d semitones)",
          movement(Voicing::Smooth), movement(Voicing::Close));

    s.role = Role::Root; s.voicing = Voicing::Close; s.bass = 1;
    const auto b = buildChord(62, s, nullptr);
    s.bass = 2;
    const auto b2 = buildChord(62, s, nullptr);
    CHECK(b.n == 4 && b.notes[0] == 50 && b.bassN == 1 && b2.notes[0] == 38, "bass: D minor gets D3 (one octave down) or D2");

    s.bass = 0; s.voicing = Voicing::Open;
    const auto o = buildChord(60, s, nullptr);
    CHECK(o.n == 3 && o.notes[0] == 52 && o.notes[1] == 60 && o.notes[2] == 67, "Open (drop-2): C major as E3 C4 G4");

    ChordSpec pent; pent.key = 9; pent.scale = Scale::MinorPentatonic; pent.voicing = Voicing::Close;
    const auto pa = buildChord(69, pent, nullptr), pc = buildChord(72, pent, nullptr);
    CHECK(std::strcmp(chordSuffix(pa.mask), "m") == 0 && std::strcmp(chordSuffix(pc.mask), "") == 0,
          "A minor pentatonic: A gives Am, C gives C (the quality that fits the scale)");

    // Strum and chords through the engine.
    Note n; n.start = 0.1; n.dur = 0.6; n.midi = 62.f;
    Params p; p.mode = Mode::Chords; p.scale = Scale::Major; p.strumMs = 30.f; p.voicing = Voicing::Close;
    const auto ev = runWith(p, wavetest::render({n}, 1.0, 48000.0));
    const auto ons = only(ev, Event::NoteOn);
    CHECK(ons.size() == 3 && ons[0].note == 62 && ons[1].note == 65 && ons[2].note == 69 &&
          ons[1].t - ons[0].t == 1440 && ons[2].t - ons[1].t == 1440 && balanced(ev),
          "engine: D sung in C major plays D F A, strummed 30 ms apart");

    p.octave = -1; p.strumMs = 0.f; p.bass = 1;
    const auto e2 = only(runWith(p, wavetest::render({n}, 1.0, 48000.0)), Event::NoteOn);
    CHECK(e2.size() == 4 && e2[0].note == 38 && e2[1].note == 50 && e2[3].note == 57, "engine: octave -1 with a bass");
}

void testHoldAndRelease() {
    std::vector<Note> ns;
    for (int i = 0; i < 3; ++i) { Note n; n.start = 0.1 + i * 0.5; n.dur = 0.35; n.midi = 57.f + i * 2; ns.push_back(n); }
    const auto sig = wavetest::render(ns, 1.8, 48000.0);
    Params p; p.hold = true;
    Engine e;
    const auto ev = runWith(p, sig, 48000.0, 256, &e);
    int maxOn = 0, on = 0;
    for (auto& x : ev) { on += x.type == Event::NoteOn ? 1 : x.type == Event::NoteOff ? -1 : 0; maxOn = std::max(maxOn, on); }
    CHECK(e.soundingCount() == 1 && maxOn == 1 && only(ev, Event::NoteOn).size() == 3, "Hold: each note sounds until the next sung note, the last one stays");
    std::vector<Event> buf(32);
    const int k = e.releaseAll(buf.data(), 32);
    CHECK(k == 1 && buf[0].type == Event::NoteOff && e.soundingCount() == 0, "releaseAll ends the held note");
}

void testBlockSizes() {
    std::vector<Note> ns;
    for (int i = 0; i < 6; ++i) { Note n; n.start = 0.1 + i * 0.3; n.dur = 0.26; n.midi = 52.f + i * 3; n.vibCents = 30.f; ns.push_back(n); }
    const auto sig = wavetest::render(ns, 2.1, 48000.0);
    Params p; p.mode = Mode::Chords; p.strumMs = 12.f; p.follow = 0.5f; p.scale = Scale::Dorian; p.key = 2;
    const auto ref = runWith(p, sig, 48000.0, 512);
    bool same = true;
    for (int b : {1, 7, 64, 4096}) {
        const auto ev = runWith(p, sig, 48000.0, b);
        same &= ev.size() == ref.size();
        for (size_t i = 0; same && i < ev.size(); ++i)
            same &= ev[i].t == ref[i].t && ev[i].type == ref[i].type && ev[i].note == ref[i].note && ev[i].value == ref[i].value;
    }
    CHECK(same && !ref.empty() && balanced(ref), "blocks of 1, 7, 64, 512 and 4096 samples give identical events (%zu events)", ref.size());
}

void testExpression() {
    Note n; n.start = 0.1; n.dur = 0.8; n.midi = 57.f; n.db = -10.f;
    Params p;
    const auto ev = runWith(p, wavetest::render({n}, 1.3, 48000.0));
    const auto at = only(ev, Event::Aftertouch);
    float mx = 0.f; bool range = true;
    for (auto& e : at) { mx = std::max(mx, e.value); range &= e.value >= 0.f && e.value <= 1.f; }
    CHECK(at.size() > 5 && range && mx > 0.5f && at.back().value == 0.f, "loudness to aftertouch: rises with the voice, back to 0 after (peak %.2f)", mx);
    p.levelTo = Expr::Off; p.brightTo = Expr::ModWheel;
    const auto ev2 = runWith(p, wavetest::render({n}, 1.3, 48000.0));
    CHECK(only(ev2, Event::Aftertouch).empty() && !only(ev2, Event::ModWheel).empty(), "brightness to mod wheel, aftertouch off");

    Note quiet = n; quiet.db = -36.f;
    const auto vl = only(runWith(Params{}, wavetest::render({n}, 1.3, 48000.0)), Event::NoteOn);
    const auto vq = only(runWith(Params{}, wavetest::render({quiet}, 1.3, 48000.0)), Event::NoteOn);
    CHECK(vl.size() == 1 && vq.size() == 1 && vl[0].value > vq[0].value + 0.15f, "velocity follows loudness (%.2f loud, %.2f quiet)",
          vl.empty() ? 0.f : vl[0].value, vq.empty() ? 0.f : vq[0].value);
}

void testGarbageInput() {
    // A block of NaN, infinity and huge values (a host glitch) must not stop WAVE from hearing the next note.
    Engine e; e.setParams(Params{}); e.prepare(48000.0);
    std::vector<float> junk(512);
    for (size_t i = 0; i < junk.size(); ++i) junk[i] = i % 3 == 0 ? std::nanf("") : i % 3 == 1 ? INFINITY : 1e30f;
    std::vector<Event> buf(256);
    e.process(junk.data(), (int)junk.size(), buf.data(), 256);
    Note n; n.start = 0.1; n.dur = 0.5; n.midi = 57.f;
    const auto ev = run(e, wavetest::render({n}, 0.8, 48000.0));
    CHECK(notes(ev) == "57 ", "NaN, infinity and huge input are ignored; the next sung note plays (%s)", notes(ev).c_str());
}

void testKeyFinder() {
    // A melody in A minor and one in D major, with the tonic and fifth longest.
    auto sing = [](std::vector<std::pair<int, double>> mel) {
        std::vector<Note> ns; double t = 0.1;
        for (auto [m, d] : mel) { Note n; n.start = t; n.dur = d; n.midi = (float)m; ns.push_back(n); t += d + 0.08; }
        Engine e; e.setParams(Params{}); e.prepare(48000.0); e.listen(true);
        run(e, wavetest::render(ns, t + 0.2, 48000.0));
        e.listen(false);
        return e.guessKey();
    };
    const auto am = sing({{57, .6}, {59, .2}, {60, .4}, {62, .2}, {64, .6}, {65, .2}, {64, .3}, {62, .2}, {60, .4}, {59, .2}, {57, .8}, {56, .2}, {57, .5}});
    const auto dM = sing({{62, .6}, {64, .2}, {66, .4}, {67, .2}, {69, .6}, {71, .2}, {73, .3}, {74, .7}, {69, .4}, {66, .3}, {62, .8}});
    CHECK(am.key == 9 && am.minor, "key listening: an A minor melody finds A minor (%s %s)", kKeyNames[am.key], am.minor ? "minor" : "major");
    CHECK(dM.key == 2 && !dM.minor, "key listening: a D major melody finds D major (%s %s)", kKeyNames[dM.key], dM.minor ? "minor" : "major");
}

void testCpu() {
    std::vector<Note> ns;
    for (int i = 0; i < 20; ++i) { Note n; n.start = i * 0.5; n.dur = 0.45; n.midi = 45.f + (i * 7) % 24; n.vibCents = 30.f; ns.push_back(n); }
    const auto sig = wavetest::render(ns, 10.0, 48000.0);
    Params p; p.range = 0; p.mode = Mode::Chords; p.follow = 1.f; p.chord = ChordType::Ninth; p.bass = 1;
    Engine e; e.setParams(p); e.prepare(48000.0);
    const auto t0 = std::chrono::steady_clock::now();
    run(e, sig, 128);
    const double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    // The PRD target is 1% (0.5% measured on a 2.3 GHz cloud Xeon); the gate is 2% so shared CI runners do not flake.
    CHECK(sec / 10.0 < 0.02, "CPU: whole engine, Low range, chords with Follow: %.2f%% of one core at 48 kHz", sec / 10.0 * 100.0);
}
}  // namespace

int main() {
    testPitch();
    testLatency();
    testSegmentation();
    testQuantizeAndFollow();
    testChords();
    testHoldAndRelease();
    testBlockSizes();
    testExpression();
    testGarbageInput();
    testKeyFinder();
    testCpu();
    std::printf("%d failure(s)\n", failures);
    return failures;
}
