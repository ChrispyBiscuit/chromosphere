// DSP tests for the INFRA engine (no JUCE). Each check prints PASS / FAIL; the exit code is the failure count.
#include "infra/synth.h"
#include "pffft.h"
#include <cstdio>
#include <vector>

using namespace infra;

static int failures = 0;
#define CHECK(cond, ...) do { if (cond) std::printf("PASS  "); else { std::printf("FAIL  "); ++failures; } std::printf(__VA_ARGS__); std::printf("\n"); } while (0)

namespace {
SynthParams quietFx() {
    SynthParams p;
    p.fx.reverbMix = 0.f; p.fx.delayMix = 0.f; p.fx.chorusMix = 0.f; p.fx.drive = 0.f;
    p.drift = 0.f;
    return p;
}

struct Render { std::vector<float> L, R; };

// Renders `total` samples; events are absolute sample times.
Render render(Synth& s, int total, int block, std::vector<MidiEvent> events, Transport tr = {}) {
    Render out; out.L.assign((size_t)total, 0.f); out.R.assign((size_t)total, 0.f);
    std::vector<MidiEvent> local;
    for (int off = 0; off < total; off += block) {
        const int n = std::min(block, total - off);
        local.clear();
        for (const auto& e : events) if (e.offset >= off && e.offset < off + n) { auto c = e; c.offset -= off; local.push_back(c); }
        s.process(out.L.data() + off, out.R.data() + off, n, local.data(), (int)local.size(), tr);
        if (tr.playing) tr.ppq += n * tr.bpm / 60.0 / 48000.0;
    }
    return out;
}
MidiEvent on(int t, int note, float vel = 0.8f) { MidiEvent e; e.offset = t; e.type = MidiEvent::NoteOn; e.number = note; e.value = vel; return e; }
MidiEvent off(int t, int note) { MidiEvent e; e.offset = t; e.type = MidiEvent::NoteOff; e.number = note; return e; }

float peakOf(const std::vector<float>& v, size_t a, size_t b) { float p = 0; for (size_t i = a; i < std::min(b, v.size()); ++i) p = std::max(p, std::abs(v[i])); return p; }
bool finite(const Render& r) { for (size_t i = 0; i < r.L.size(); ++i) if (!std::isfinite(r.L[i]) || !std::isfinite(r.R[i])) return false; return true; }
}  // namespace

static void testTables() {
    const auto& b = WavetableBank::get();
    // Classic frame 0 is a sine: peak at a quarter cycle, near zero at the start.
    const float* f = b.frame(0, 0, 0);
    CHECK(std::abs(std::abs(f[WavetableBank::kSize / 4]) - 1.f) < 0.01f && std::abs(f[0]) < 0.01f, "classic frame 0 is a unit sine (%.3f at 1/4)", f[WavetableBank::kSize / 4]);
    // Every frame of every table is normalised and finite.
    bool ok = true;
    for (int t = 0; t < kNumTables; ++t) for (int fr = 0; fr < WavetableBank::kFrames; ++fr) {
        float pk = 0;
        const float* d = b.frame(t, fr, 0);
        for (int i = 0; i <= WavetableBank::kSize; ++i) { if (!std::isfinite(d[i])) ok = false; pk = std::max(pk, std::abs(d[i])); }
        if (std::abs(pk - 1.f) > 0.02f) ok = false;
        if (d[WavetableBank::kSize] != d[0]) ok = false;
    }
    CHECK(ok, "all %d tables x %d frames normalised, finite, guard sample set", kNumTables, WavetableBank::kFrames);
    // The top level of a saw frame is a single sine (harmonic 1 only).
    const float* top = b.frame(0, 8, WavetableBank::kLevels - 1);
    float pk = 0; for (int i = 0; i < WavetableBank::kSize; ++i) pk = std::max(pk, std::abs(top[i]));
    CHECK(pk > 0.1f, "top mip level keeps the fundamental (peak %.3f)", pk);
}

static void testAliasing() {
    // A saw-rich frame at ~5 kHz: every non-harmonic component at least 60 dB below the fundamental.
    Synth s; SynthParams p = quietFx();
    p.osc[0].table = 6; p.osc[0].morph = 0.f;          // Plasma frame 0: a plain saw
    p.osc[0].unison = 1; p.osc[0].phaseRand = 0.f; p.osc[1].on = false;
    p.filterType = FilterType::LP12; p.cutoff = 20000.f; p.keyTrack = 0.f; p.resonance = 0.f;
    p.ampEnv = {0.001f, 0.1f, 1.f, 0.1f}; p.velSens = 0.f; p.osc[0].fine = 40.f;   // note 111 + 40 cents ~= 5.1 kHz
    s.setParams(p); s.prepare(48000.0, 512);
    const int N = 16384;
    auto r = render(s, N + 4096, 512, {on(0, 111)});
    std::vector<float> x(N);
    for (int i = 0; i < N; ++i) { const float w = 0.5f - 0.5f * std::cos(6.2831853f * i / N); x[(size_t)i] = r.L[(size_t)(4096 + i)] * w; }
    PFFFT_Setup* st = pffft_new_setup(N, PFFFT_REAL);
    std::vector<float> out(N), work(N);
    pffft_transform_ordered(st, x.data(), out.data(), work.data(), PFFFT_FORWARD);
    pffft_destroy_setup(st);
    std::vector<double> mag(N / 2);
    for (int k = 1; k < N / 2; ++k) mag[(size_t)k] = std::hypot(out[(size_t)2 * k], out[(size_t)2 * k + 1]);
    const double f0 = 440.0 * std::pow(2.0, (111.4 - 69.0) / 12.0);
    const double binHz = 48000.0 / N;
    double fund = 0, worst = 0;
    for (int k = 2; k < N / 2; ++k) {
        const double f = k * binHz;
        const double h = f / f0;
        const bool harmonic = std::abs(h - std::round(h)) * f0 < 6 * binHz;
        if (std::abs(f - f0) < 6 * binHz) fund = std::max(fund, mag[(size_t)k]);
        else if (!harmonic && f > 60.0) worst = std::max(worst, mag[(size_t)k]);
    }
    const double db = 20.0 * std::log10(worst / fund);
    CHECK(db < -60.0, "aliasing of a saw at %.0f Hz: worst component %.1f dB below the fundamental", f0, -db);
}

static void testNoStuckNotes() {
    Synth s; SynthParams p = quietFx();
    p.polyphony = 6; p.ampEnv.r = 0.05f;
    s.setParams(p); s.prepare(48000.0, 256);
    std::vector<MidiEvent> ev;
    for (int i = 0; i < 40; ++i) ev.push_back(on(i * 300, 40 + (i * 7) % 40));
    for (int i = 0; i < 40; ++i) ev.push_back(off(14000 + i * 37, 40 + (i * 7) % 40));
    std::sort(ev.begin(), ev.end(), [](auto& a, auto& b) { return a.offset < b.offset; });
    auto r = render(s, 48000, 256, ev);
    CHECK(finite(r), "40 overlapping notes with 6-voice stealing: output finite");
    CHECK(peakOf(r.L, 40000, 48000) < 1e-4f && s.activeVoices.load() == 0, "all voices silent after release (tail peak %.2g, active %d)",
          peakOf(r.L, 40000, 48000), s.activeVoices.load());
}

static void testSustainPedal() {
    Synth s; SynthParams p = quietFx(); p.ampEnv.r = 0.02f;
    s.setParams(p); s.prepare(48000.0, 256);
    MidiEvent ped; ped.type = MidiEvent::Controller; ped.number = 64; ped.value = 1.f; ped.offset = 0;
    MidiEvent up = ped; up.value = 0.f; up.offset = 30000;
    auto r = render(s, 48000, 256, {ped, on(10, 60), off(2000, 60), up});
    CHECK(peakOf(r.L, 20000, 29000) > 0.01f, "note held by the sustain pedal after key release");
    CHECK(peakOf(r.L, 40000, 48000) < 1e-4f, "note released with the pedal");
}

static void testBlockSizeIndependence() {
    SynthParams p = quietFx();
    p.osc[0].unison = 4; p.osc[1].table = 5; p.lfo[0].rateHz = 3.f;
    p.mod[0] = {ModSource::Lfo1, ModDest::Cutoff, 0.3f};
    p.mod[1] = {ModSource::ModEnv, ModDest::MorphA, 0.7f};
    p.lfo[1].retrig = false; p.lfo[1].rateHz = 0.7f;                       // a free-running LFO on the effects too
    p.mod[2] = {ModSource::Lfo2, ModDest::MorphB, 0.4f};
    p.mod[3] = {ModSource::Lfo2, ModDest::FxChorus, 0.3f};
    p.fx.chorusMix = 0.2f;
    std::vector<MidiEvent> ev = {on(100, 48), on(777, 55), on(5000, 62), off(20000, 48), off(20001, 55), off(23000, 62)};
    std::vector<Render> outs;
    for (int bs : {1, 7, 64, 256, 480, 4096}) {
        Synth s; s.setParams(p); s.prepare(48000.0, 4096);
        outs.push_back(render(s, 32768, bs, ev));
    }
    double worst = 0;
    for (size_t k = 1; k < outs.size(); ++k)
        for (size_t i = 0; i < outs[0].L.size(); ++i) worst = std::max(worst, (double)std::abs(outs[k].L[i] - outs[0].L[i]));
    // Identical with GCC; with MSVC, FMA contraction in vectorised loops and their leftover samples can differ in the
    // last bits (3e-6, about -110 dB, was measured). Anything audible is far above this.
    CHECK(worst < 1e-5, "same output for block sizes 1, 7, 64, 256, 480, 4096 (max difference %.2g)", worst);
}

static void testSampleRates() {
    bool ok = true;
    for (double fs : {44100.0, 48000.0, 96000.0, 192000.0}) {
        Synth s; SynthParams p; p.fx.reverbMix = 0.3f; p.fx.delayMix = 0.3f; p.fx.chorusMix = 0.5f; p.fx.drive = 0.4f;
        p.filterType = FilterType::Comb; p.resonance = 0.9f; p.noiseLevel = 0.3f; p.noiseColour = NoiseColour::SolarWind;
        s.setParams(p); s.prepare(fs, 512);
        const int n = (int)(fs * 0.5);
        Render r; r.L.assign((size_t)n, 0.f); r.R.assign((size_t)n, 0.f);
        MidiEvent e = on(0, 36);
        for (int off = 0; off < n; off += 512) {
            const int c = std::min(512, n - off);
            s.process(r.L.data() + off, r.R.data() + off, c, off == 0 ? &e : nullptr, off == 0 ? 1 : 0, {});
        }
        if (!finite(r) || peakOf(r.L, 0, r.L.size()) < 0.01f || peakOf(r.L, 0, r.L.size()) > 0.95f) ok = false;
    }
    CHECK(ok, "44.1, 48, 96 and 192 kHz: finite, audible, under the limiter ceiling");
}

static void testFilterTypes() {
    bool ok = true;
    for (int t = 0; t < kNumFilterTypes; ++t) {
        Synth s; SynthParams p = quietFx(); p.filterType = (FilterType)t; p.resonance = 1.f; p.cutoff = 1200.f; p.filterDrive = 0.6f;
        p.osc[0].table = 6; p.noiseLevel = 0.5f;
        s.setParams(p); s.prepare(48000.0, 512);
        auto r = render(s, 24000, 512, {on(0, 45, 1.f), on(0, 57, 1.f), on(0, 64, 1.f)});
        const float pk = peakOf(r.L, 0, r.L.size());
        if (!finite(r) || pk < 0.005f || pk > 0.95f) { ok = false; std::printf("      filter %s: peak %.3f\n", kFilterNames[t], pk); }
    }
    CHECK(ok, "all 8 filter types at full resonance and drive: finite, audible, bounded");
}

static void testArpTiming() {
    // 1/16 at 120 BPM and 48 kHz = 6000 samples per step.
    Arpeggiator a; a.reset();
    ArpParams p; p.on = true; p.division = 13; p.gate = 0.5f; p.mode = ArpMode::Up; p.octaves = 2;
    for (auto& s : p.steps) s = {1.f, 0, 1};
    p.steps[2].ratchet = 3; p.steps[5].velocity = 0.f; p.steps[6].transpose = 7;
    std::vector<ArpEvent> evs;
    ArpEvent buf[64];
    const int block = 333;
    for (int64_t t = 0; t < 6000 * 16; t += block) {
        a.beginBlock(p, t, 48000.0, 120.0, false, 0.0);
        if (t == 0) { a.keyOn(60, 1.f, 10); a.keyOn(64, 1.f, 10); a.keyOn(67, 1.f, 10); }
        for (int64_t s = t; s < t + block; ++s) {
            if (a.nextTime() > s) continue;
            const int m = a.process(s, buf, 64);
            for (int i = 0; i < m; ++i) evs.push_back(buf[i]);
        }
        a.endBlock(block);
    }
    std::vector<int64_t> ons; std::vector<int> notes;
    for (auto& e : evs) if (e.on) { ons.push_back(e.time); notes.push_back(e.note); }
    bool timing = ons.size() > 10 && ons[0] == 10 && ons[1] == 6010;
    // Step 2 (third step) ratchets three times inside its 6000 samples.
    timing = timing && ons[2] == 12010 && ons[3] == 14010 && ons[4] == 16010;
    CHECK(timing, "arp steps land on the sample (first ons: %lld %lld %lld %lld %lld)", (long long)ons[0], (long long)ons[1],
          (long long)ons[2], (long long)ons[3], (long long)ons[4]);
    // Up over two octaves: 60 64 67 72 76 79; step 5 is a rest; step 6 transposes by +7.
    bool order = notes[0] == 60 && notes[1] == 64 && notes[2] == 67 && notes[5] == 72;
    const bool rest = std::find(ons.begin(), ons.end(), (int64_t)(10 + 5 * 6000)) == ons.end();
    // ons: s0 s1 s2 s2 s2 s3 s4 s6 (s5 rests and does not advance the note order)
    const bool transposed = ons.size() > 7 && ons[7] == 10 + 6 * 6000 && notes[7] == 79 + 7;
    CHECK(order && rest && transposed, "arp order, rest and transpose lanes (notes %d %d %d .. %d, step 6 -> %d)", notes[0], notes[1], notes[2], notes[5], notes[7]);
    int offs = 0; for (auto& e : evs) if (!e.on) ++offs;
    CHECK(std::abs((int)ons.size() - offs) <= 1, "every arp note-on has its note-off (%zu on, %d off)", ons.size(), offs);

    // Swing: odd steps late by swing * half a step.
    Arpeggiator b; b.reset(); p.swing = 0.5f; p.steps[2].ratchet = 1; p.steps[5].velocity = 1.f;
    std::vector<int64_t> sw;
    for (int64_t t = 0; t < 6000 * 6; t += 500) {
        b.beginBlock(p, t, 48000.0, 120.0, false, 0.0);
        if (t == 0) b.keyOn(60, 1.f, 0);
        for (int64_t s = t; s < t + 500; ++s) { if (b.nextTime() > s) continue; const int m = b.process(s, buf, 64); for (int i = 0; i < m; ++i) if (buf[i].on) sw.push_back(buf[i].time); }
        b.endBlock(500);
    }
    CHECK(sw.size() >= 4 && sw[1] == 6000 + 1500 && sw[2] == 12000 && sw[3] == 18000 + 1500, "swing 50%% delays odd steps by a quarter step");

    // Host sync: steps follow the host grid.
    Arpeggiator h; h.reset(); p.swing = 0.f;
    std::vector<int64_t> hs;
    double ppq = 10.1;                       // between grid lines
    for (int64_t t = 0; t < 48000; t += 512) {
        h.beginBlock(p, t, 48000.0, 120.0, true, ppq);
        if (t == 0) h.keyOn(60, 1.f, 0);
        for (int64_t s = t; s < t + 512; ++s) { if (h.nextTime() > s) continue; const int m = h.process(s, buf, 64); for (int i = 0; i < m; ++i) if (buf[i].on) hs.push_back(buf[i].time); }
        h.endBlock(512); ppq += 512 * 2.0 / 48000.0;
    }
    // First step at once, then on grid lines: 10.25 + k/4 beats -> (0.15 + 0.25 k) beats from the start -> 3600 + 6000 k samples.
    CHECK(hs.size() > 4 && hs[0] == 0 && hs[1] == 3600 && hs[2] == 9600, "host-synced arp: first step immediate, then on the host grid (%lld %lld %lld)",
          (long long)hs[0], (long long)hs[1], (long long)hs[2]);
}

static void testArpThroughSynth() {
    Synth s; SynthParams p = quietFx();
    p.arp.on = true; p.arp.division = 13; p.arp.gate = 0.4f;
    p.ampEnv = {0.001f, 0.1f, 0.6f, 0.02f};
    s.setParams(p); s.prepare(48000.0, 512);
    auto r = render(s, 48000, 512, {on(0, 60), on(0, 64), off(30000, 60), off(30000, 64)});
    // Gate 0.4 of a 6000-sample step: silence between steps.
    const float gap = peakOf(r.L, 6000 - 1200, 6000 - 200), noteP = peakOf(r.L, 6000 + 200, 6000 + 1500);
    CHECK(noteP > 0.02f && gap < noteP * 0.05f, "arp through the engine: notes and gaps (note %.3f, gap %.4f)", noteP, gap);
    CHECK(peakOf(r.L, 36000, 48000) < 1e-4f, "arp stops when the keys are released");
}

// Sleep on silence (optimisation guide 5.1): a silent engine stops working, a note wakes it on the same block, and
// the result does not depend on the block size even across a sleep (the free-running LFOs keep time).
static void testSleepOnSilence() {
    SynthParams p = quietFx();
    p.fx.chorusMix = 0.4f; p.lfo[0].rateHz = 1.3f; p.lfo[0].retrig = false;
    p.mod[0] = {ModSource::Lfo1, ModDest::Cutoff, 0.3f};
    p.ampEnv.r = 0.05f;
    {
        Synth s; s.setParams(p); s.prepare(48000.0, 256);
        render(s, 48000, 256, {on(0, 60), off(4800, 60)});
        CHECK(s.sleeping(), "asleep after the note and its tail have died away");
        std::vector<float> L(256), R(256);
        MidiEvent e = on(0, 64);
        s.process(L.data(), R.data(), 256, &e, 1, {});
        CHECK(!s.sleeping() && peakOf(L, 0, 256) > 1e-3f, "a note wakes it on the same block (peak %.3f)", peakOf(L, 0, 256));
    }
    // Two notes with a second of silence between them, rendered at different block sizes.
    std::vector<MidiEvent> ev = {on(100, 48), off(6000, 48), on(70000, 55), off(80000, 55)};
    std::vector<Render> outs;
    for (int bs : {1, 7, 64, 256, 4096}) {
        Synth s; s.setParams(p); s.prepare(48000.0, 4096);
        outs.push_back(render(s, 96000, bs, ev));
    }
    double worst = 0;
    for (size_t k = 1; k < outs.size(); ++k)
        for (size_t i = 0; i < outs[0].L.size(); ++i) worst = std::max(worst, (double)std::abs(outs[k].L[i] - outs[0].L[i]));
    // Across a sleep only effect-buffer residue below -120 dBFS can differ.
    CHECK(worst < 1e-5, "same output across a sleep for block sizes 1, 7, 64, 256, 4096 (max difference %.2g)", worst);
}

static void testMonoLegato() {
    Synth s; SynthParams p = quietFx(); p.voiceMode = VoiceMode::Legato; p.glide = 0.05f; p.ampEnv.r = 0.02f;
    s.setParams(p); s.prepare(48000.0, 512);
    auto r = render(s, 48000, 512, {on(0, 48), on(10000, 55), off(20000, 55), off(30000, 48)});
    CHECK(s.activeVoices.load() == 0 && peakOf(r.L, 22000, 29000) > 0.01f, "legato: returning to the held note keeps sounding, then releases");
}

int main() {
    testTables();
    testAliasing();
    testNoStuckNotes();
    testSustainPedal();
    testBlockSizeIndependence();
    testSampleRates();
    testFilterTypes();
    testArpTiming();
    testArpThroughSynth();
    testMonoLegato();
    testSleepOnSilence();
    std::printf("%d failure(s)\n", failures);
    return failures;
}
