// DSP and sequencer tests for the ULTRA engine (no JUCE). Each check prints PASS / FAIL; the exit code is the
// failure count. `ultra_tests --levels` prints each model's peak at the default dials (for kModelGain).
#include "ultra/machine.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

using namespace ultra;

static int failures = 0;
static bool verbose = false;
#define CHECK(cond, ...) do { if (cond) std::printf("PASS  "); else { std::printf("FAIL  "); ++failures; } std::printf(__VA_ARGS__); std::printf("\n"); } while (0)

namespace {
constexpr double kFs = 48000.0;

// A kit with every effect off, so the output is the voices alone.
KitParams dryKit() {
    KitParams k;
    k.master.reverbReturn = 0.f; k.master.echoReturn = 0.f;
    for (auto& t : k.tracks) { t.sendA = t.sendB = 0.f; t.note = -1; }
    return k;
}

struct Out { std::vector<float> L, R; };

// Renders `total` samples through the machine with internal play on or off.
Out run(DrumMachine& m, int total, int block, bool play, std::vector<MidiEvent> events = {}, Transport tr = {}) {
    Out o; o.L.assign((size_t)total, 0.f); o.R.assign((size_t)total, 0.f);
    m.internalPlay = play;
    std::vector<MidiEvent> local;
    for (int off = 0; off < total; off += block) {
        const int n = std::min(block, total - off);
        local.clear();
        for (const auto& e : events) if (e.offset >= off && e.offset < off + n) { auto c = e; c.offset -= off; local.push_back(c); }
        Outputs out; out.L = o.L.data() + off; out.R = o.R.data() + off;
        m.process(out, n, local.data(), (int)local.size(), tr);
        if (tr.playing) tr.ppq += n * tr.bpm / 60.0 / kFs;
    }
    return o;
}

float peakOf(const std::vector<float>& v, size_t a = 0, size_t b = SIZE_MAX) {
    float p = 0; for (size_t i = a; i < std::min(b, v.size()); ++i) p = std::max(p, std::abs(v[i])); return p;
}
bool finite(const std::vector<float>& v) { for (float x : v) if (!std::isfinite(x)) return false; return true; }

// One voice rendered directly, at 48 kHz.
std::vector<float> renderVoice(const VoiceParams& vp, float seconds, float vel = 1.f, bool* ended = nullptr) {
    DrumVoice v; v.prepare(kFs, 99);
    v.trigger(vp, 1.f, vel);
    v.setFilter(vp.filter, vp.cutoff, vp.resonance);
    std::vector<float> out((size_t)(seconds * kFs), 0.f);
    for (size_t off = 0; off < out.size(); off += 64) v.render(out.data() + off, (int)std::min<size_t>(64, out.size() - off));
    if (ended) *ended = !v.active();
    return out;
}

// Hit times (absolute samples) of a sequencer run.
struct SeqHit { int64_t t; int track; float vel; int pitch; float lock; };
std::vector<SeqHit> seqRun(const PatternBank& bank, KitParams kit, int64_t total, int block, float swing = 0.5f,
                           std::vector<std::pair<int64_t, int>> patternChanges = {}, Transport tr = {}, bool internal = true) {
    Sequencer s; s.prepare(kFs);
    std::vector<SeqHit> out;
    Hit hits[1024];
    for (int64_t off = 0; off < total; off += block) {
        const int n = (int)std::min<int64_t>(block, total - off);
        for (auto& pc : patternChanges) if (pc.first >= off && pc.first < off + n) kit.pattern = pc.second;
        const int c = s.process(bank, kit, swing, 0.f, 1.f, tr, internal, n, hits, 1024);
        for (int i = 0; i < c; ++i) out.push_back({off + hits[i].offset, hits[i].track, hits[i].velocity, hits[i].pitch, hits[i].lock});
        if (tr.playing) tr.ppq += n * tr.bpm / 60.0 / kFs;
    }
    return out;
}
std::vector<int64_t> timesOf(const std::vector<SeqHit>& h, int track) { std::vector<int64_t> t; for (auto& x : h) if (x.track == track) t.push_back(x.t); return t; }
}  // namespace

// ---- Models ---------------------------------------------------------------------------------------------
static void testModels(bool printLevels) {
    for (int m = 0; m < kNumModels; ++m) {
        VoiceParams vp; vp.model = (Model)m;
        bool ended = false;
        const auto out = renderVoice(vp, 8.f, 1.f, &ended);
        const float pk = peakOf(out);
        if (printLevels) std::printf("level %-14s %6.2f dBFS\n", kModelNames[m], 20.f * std::log10(std::max(pk, 1e-9f)));
        CHECK(finite(out) && pk > 0.25f && pk < 0.9f && ended, "%s at the default dials: finite, peak %.2f, ends within 8 s", kModelNames[m], pk);
    }
    // Every corner of Color / Snap / Sweep / Tone at both ends of Tune and Decay.
    for (int m = 0; m < kNumModels; ++m) {
        float lo, hi; DrumVoice::decayRange((Model)m, lo, hi);
        bool ok = true, ends = true; float worst = 0.f, quietest = 9.f;
        for (int c = 0; c < 32; ++c) {
            VoiceParams vp; vp.model = (Model)m;
            vp.color = (c & 1) ? 1.f : 0.f; vp.snap = (c & 2) ? 1.f : 0.f; vp.sweep = (c & 4) ? 1.f : 0.f;
            vp.tune = (c & 8) ? 24.f : -24.f; vp.decay = (c & 16) ? 1.f : 0.f; vp.tone = (c & 3) == 1 ? 1.f : 0.3f;
            bool ended = false;
            const auto out = renderVoice(vp, hi * 2.f + 0.5f, 1.f, &ended);
            const float pk = peakOf(out);
            if (!finite(out)) ok = false;
            worst = std::max(worst, pk); quietest = std::min(quietest, pk);
            if (!ended) ends = false;
            if (verbose && (!ended || pk > 1.5f || pk < 0.005f))
                std::printf("      corner c=%d (color %.0f snap %.0f sweep %.0f tune %+.0f decay %.0f tone %.1f): peak %.3f%s\n", c, vp.color, vp.snap,
                            vp.sweep, vp.tune, vp.decay, vp.tone, pk, ended ? "" : " still ringing");
        }
        CHECK(ok && worst < 1.5f && quietest > 0.005f && ends, "%s corners: finite, peak %.2f..%.2f, every hit ends", kModelNames[m], quietest, worst);
    }
    // Velocity: lower velocity is quieter.
    VoiceParams vp; vp.model = Model::FlareSnare;
    const float loud = peakOf(renderVoice(vp, 0.5f, 1.f)), soft = peakOf(renderVoice(vp, 0.5f, 0.3f));
    CHECK(soft < loud * 0.7f, "velocity scales level (%.2f vs %.2f)", soft, loud);
}

// PolyBLEP keeps the hat squares clean: a 3130 Hz square (the top of the metal bank at Tune +24) at 48 kHz,
// aliasing products vs the fundamental. 3130 Hz does not divide 48 kHz, so the aliases fall between harmonics.
static void testAliasing() {
    const int N = 4800; const float f = 3130.f, dt = f / (float)kFs;
    std::vector<float> x((size_t)N);
    float p = 0.f;
    for (int i = 0; i < N; ++i) { x[(size_t)i] = blepSquare(p, dt); p = wrap(p + dt); }
    // Bins are 10 Hz apart; harmonics of 3130 Hz are on multiples of 313 bins. Hann window.
    double fund = 0, worst = 0;
    for (int k = 1; k < N / 2; ++k) {
        double re = 0, im = 0;
        for (int i = 0; i < N; ++i) {
            const double w = 0.5 - 0.5 * std::cos(2 * M_PI * i / N);
            re += x[(size_t)i] * w * std::cos(2 * M_PI * k * i / N); im -= x[(size_t)i] * w * std::sin(2 * M_PI * k * i / N);
        }
        const double mag = std::sqrt(re * re + im * im);
        const int nearHarm = (k + 156) / 313 * 313;
        if (k == 313) fund = mag;
        else if (std::abs(k - nearHarm) > 3) worst = std::max(worst, mag);
    }
    const double db = 20 * std::log10(worst / fund);
    CHECK(db < -28.0, "band-limited square at 3130 Hz: worst alias %.1f dB below the fundamental", -db);
}

// ---- Sequencer ------------------------------------------------------------------------------------------
static void testSequencer() {
    KitParams kit = dryKit(); kit.tempo = 120.f;
    PatternBank bank;
    auto& t0 = bank.patterns[0].tracks[0];
    for (int s = 0; s < 16; ++s) t0.steps[(size_t)s].vel = 100;
    // Exact 1/16 grid at every block size.
    bool exact = true;
    for (int block : {1, 7, 64, 333, 512, 4096}) {
        const auto t = timesOf(seqRun(bank, kit, 200000, block), 0);
        if (t.size() != 34) exact = false;
        for (size_t i = 0; i < t.size(); ++i) if (t[i] != (int64_t)i * 6000) exact = false;
    }
    CHECK(exact, "1/16 steps land every 6000 samples at 120 BPM for block sizes 1..4096");
    // Swing 60%: odd steps late by 0.1 x 0.5 beat = 1200 samples.
    {
        const auto t = timesOf(seqRun(bank, kit, 25000, 256, 0.6f), 0);
        CHECK(t.size() >= 4 && t[0] == 0 && t[1] == 7200 && t[2] == 12000 && t[3] == 19200, "swing 60%% delays the off-steps by 1200 samples");
    }
    // Nudge, ratchet, pitch, lock.
    {
        PatternBank b2;
        auto& p = b2.patterns[0].tracks[0];
        p.steps[0] = {100, 100, 4, 0, 0, 0, 0};
        p.steps[1] = {100, 100, 1, 7, 50, 0, 0};
        p.steps[3] = {100, 100, 1, -3, -50, 0, -40};
        const auto h = seqRun(b2, kit, 20000, 128);
        std::vector<int64_t> t; for (auto& x : h) t.push_back(x.t);
        CHECK(t.size() == 6 && t[0] == 0 && t[1] == 1500 && t[2] == 3000 && t[3] == 4500 && t[4] == 9000 && t[5] == 15000,
              "ratchet x4 at 1500-sample spacing, nudge +50%% and -50%% land half a step off");
        CHECK(h.size() == 6 && h[1].vel < h[0].vel && h[4].pitch == 7 && h[5].pitch == -3 && std::abs(h[5].lock + 0.4f) < 1e-4f,
              "ratchet repeats get softer; pitch and lock lanes reach the hit");
    }
    // Rates: 1/16T steps are 4000 samples, 1/8 are 12000.
    {
        PatternBank b3;
        b3.patterns[0].tracks[0].rate = 1; b3.patterns[0].tracks[1].rate = 5;
        for (int s = 0; s < 4; ++s) { b3.patterns[0].tracks[0].steps[(size_t)s].vel = 90; b3.patterns[0].tracks[1].steps[(size_t)s].vel = 90; }
        const auto h = seqRun(b3, kit, 13000, 100);
        const auto a = timesOf(h, 0), b = timesOf(h, 1);
        CHECK(a.size() == 4 && a[1] == 4000 && a[3] == 12000 && b.size() == 2 && b[1] == 12000, "triplet and eighth rates");
    }
    // Polymeter: a 3-step and a 4-step loop.
    {
        PatternBank b4;
        b4.patterns[0].tracks[0].length = 3; b4.patterns[0].tracks[0].steps[0].vel = 100;
        b4.patterns[0].tracks[1].length = 4; b4.patterns[0].tracks[1].steps[0].vel = 100;
        const auto h = seqRun(b4, kit, 72001, 512);
        const auto a = timesOf(h, 0), b = timesOf(h, 1);
        CHECK(a.size() == 5 && a[1] == 18000 && a[4] == 72000 && b.size() == 4 && b[1] == 24000 && b[3] == 72000,
              "3 against 4: loops of 18000 and 24000 samples meet at 72000");
    }
    // Conditions over 4 loops of a 16-step pattern (96000 samples each).
    {
        auto loopsOf = [&](Cond c, bool fill) {
            PatternBank b5; auto& s = b5.patterns[0].tracks[0].steps[0]; s.vel = 100; s.cond = (uint8_t)c;
            KitParams k = kit; k.fill = fill;
            std::vector<int> loops;
            for (auto t : timesOf(seqRun(b5, k, 96000 * 4, 512), 0)) loops.push_back((int)(t / 96000));
            return loops;
        };
        CHECK(loopsOf(Cond::C12, false) == std::vector<int>({0, 2}) && loopsOf(Cond::C22, false) == std::vector<int>({1, 3}) &&
              loopsOf(Cond::C34, false) == std::vector<int>({2}) && loopsOf(Cond::First, false) == std::vector<int>({0}) &&
              loopsOf(Cond::NotFirst, false) == std::vector<int>({1, 2, 3}),
              "conditions 1:2, 2:2, 3:4, 1st and Not 1st pick the right loops");
        CHECK(loopsOf(Cond::Fill, false).empty() && loopsOf(Cond::Fill, true).size() == 4 && loopsOf(Cond::NotFill, true).empty(),
              "Fill and Not fill follow the fill switch");
    }
    // Chance: deterministic and near its percentage.
    {
        PatternBank b6;
        for (int s = 0; s < 16; ++s) { b6.patterns[0].tracks[0].steps[(size_t)s].vel = 100; b6.patterns[0].tracks[0].steps[(size_t)s].chance = 50; }
        const int64_t total = 6000LL * 10000;
        const auto a = seqRun(b6, kit, total, 4096), b = seqRun(b6, kit, total, 997);
        bool same = a.size() == b.size();
        for (size_t i = 0; same && i < a.size(); ++i) same = a[i].t == b[i].t;
        const double frac = (double)a.size() / 10000.0;
        CHECK(same && std::abs(frac - 0.5) < 0.03, "chance 50%%: %.3f of 10000 steps, identical across block sizes", frac);
    }
    // Pattern switch on the bar line.
    {
        PatternBank b7;
        b7.patterns[0].tracks[0].steps[0].vel = 100; b7.patterns[0].tracks[0].steps[8].vel = 100;
        b7.patterns[1].tracks[1].steps[0].vel = 100;
        const auto h = seqRun(b7, kit, 100000, 512, 0.5f, {{30000, 1}});
        const auto a = timesOf(h, 0), b = timesOf(h, 1);
        CHECK(a == std::vector<int64_t>({0, 48000}) && b == std::vector<int64_t>({96000}), "a pattern change mid-bar waits for the bar line");
    }
    // Host transport: a loop jump back replays the first step.
    {
        Sequencer s; s.prepare(kFs);
        Hit hits[64]; int fired = 0;
        Transport tr; tr.playing = true; tr.hasTempo = true; tr.bpm = 120.0;
        for (int b = 0; b < 100; ++b) {
            tr.ppq = (b < 50 ? b : b - 50) * 512 * 2.0 / kFs;      // jump back to 0 at block 50
            const int c = s.process(bank, kit, 0.5f, 0.f, 1.f, tr, false, 512, hits, 64);
            for (int i = 0; i < c; ++i) if (hits[i].track == 0 && (b == 0 || b == 50) && hits[i].offset == 0) ++fired;
        }
        CHECK(fired == 2, "host loop jump replays the step at the loop start");
    }
}

// ---- Machine --------------------------------------------------------------------------------------------
static void testMachine() {
    PatternBank bank;
    // MIDI note triggers are sample-accurate.
    {
        DrumMachine m; KitParams k = dryKit(); k.tracks[0].note = 36; k.tracks[0].v.model = Model::CoreKick;
        m.setParams(k); m.setPatterns(&bank); m.prepare(kFs, 512);
        MidiEvent e; e.offset = 300; e.type = MidiEvent::NoteOn; e.number = 36; e.value = 1.f;
        const auto o = run(m, 2048, 512, false, {e});
        CHECK(peakOf(o.L, 0, 300) == 0.f && peakOf(o.L, 300, 330) > 0.f, "MIDI note plays its track from the event's sample");
    }
    // Pattern-select notes (24..31) switch patterns while stopped.
    {
        DrumMachine m; KitParams k = dryKit(); m.setParams(k); m.setPatterns(&bank); m.prepare(kFs, 256);
        MidiEvent e; e.offset = 10; e.type = MidiEvent::NoteOn; e.number = 27; e.value = 1.f;
        run(m, 512, 256, false, {e});
        CHECK(m.currentPattern.load() == 3, "MIDI note D#0 selects pattern D");
    }
    // Choke group: the closed hat cuts the open hat within 5 ms.
    {
        auto openTail = [&](bool choke) {
            DrumMachine m; KitParams k = dryKit();
            k.tracks[3].v.model = Model::IonHat; k.tracks[3].v.decay = 0.1f; k.tracks[3].levelDb = -60.f;
            k.tracks[4].v.model = Model::IonHat; k.tracks[4].v.decay = 0.9f;
            if (choke) k.tracks[3].choke = k.tracks[4].choke = 1;
            k.tracks[3].note = 42; k.tracks[4].note = 46;
            m.setParams(k); m.setPatterns(&bank); m.prepare(kFs, 256);
            MidiEvent a; a.type = MidiEvent::NoteOn; a.number = 46; a.value = 1.f; a.offset = 0;
            MidiEvent b = a; b.number = 42; b.offset = 4800;
            const auto o = run(m, 9600, 256, false, {a, b});
            return peakOf(o.L, 4800 + 260, 9600);
        };
        const float with = openTail(true), without = openTail(false);
        CHECK(with < 1e-6f && without > 0.01f, "choke group silences the open hat (%.2g vs %.2g without)", with, without);
    }
    // Pan lock: -100% puts the hit on the left only.
    {
        DrumMachine m; KitParams k = dryKit(); k.tracks[1].v.model = Model::FlareSnare; k.tracks[1].lock = LockTarget::Pan;
        m.setParams(k); m.setPatterns(&bank); m.prepare(kFs, 256);
        m.trigger(1, 1.f, 0, -1.f);
        const auto o = run(m, 4096, 256, false);
        CHECK(peakOf(o.R) < 1e-6f && peakOf(o.L) > 0.1f, "a pan lock of -100%% sends the hit hard left");
    }
    // Mute and solo.
    {
        DrumMachine m; KitParams k = dryKit(); k.tracks[0].note = 36; k.tracks[1].note = 38; k.tracks[1].solo = true;
        m.setParams(k); m.setPatterns(&bank); m.prepare(kFs, 256);
        MidiEvent a; a.type = MidiEvent::NoteOn; a.number = 36; a.value = 1.f;
        const auto o = run(m, 4096, 256, false, {a});
        CHECK(peakOf(o.L) < 1e-6f, "a solo on another track silences this one");
    }
    // Block-size independence of the whole machine (effects off; the reverb's worker thread is not deterministic).
    {
        PatternBank b;
        auto& p = b.patterns[0];
        const int models[8] = {0, 2, 3, 4, 5, 7, 9, 12};
        for (int t = 0; t < 8; ++t) {
            p.tracks[(size_t)t].length = 16 - t; p.tracks[(size_t)t].rate = t % 3 == 0 ? 0 : 2;
            for (int s = 0; s < 16; s += 1 + t % 3) p.tracks[(size_t)t].steps[(size_t)s] = {(uint8_t)(70 + 7 * t), 80, (uint8_t)(1 + s % 3), (int8_t)(s % 5), (int8_t)(s % 7 - 3), 0, (int8_t)(10 * (s % 4))};
        }
        KitParams k = dryKit();
        for (int t = 0; t < 8; ++t) { k.tracks[(size_t)t].v.model = (Model)models[t]; k.tracks[(size_t)t].v.filter = (FilterType)(t % kNumFilterTypes); k.tracks[(size_t)t].v.cutoff = 3000.f; k.tracks[(size_t)t].pan = (t - 3.5f) / 4.f; }
        k.swing = 0.58f; k.macro = {0.3f, 0.4f, 0.f, 0.f};
        k.master.glueMix = 1.f; k.master.drive = 0.3f; k.master.lowShelf = 3.f;
        std::vector<Out> outs;
        for (int block : {1, 64, 500, 4096}) {
            DrumMachine m; m.setParams(k); m.setPatterns(&b); m.prepare(kFs, block);
            outs.push_back(run(m, 96000, block, true));
        }
        float diff = 0.f;
        for (size_t j = 1; j < outs.size(); ++j) for (size_t i = 0; i < outs[0].L.size(); ++i) diff = std::max(diff, std::abs(outs[j].L[i] - outs[0].L[i]));
        CHECK(diff < 1e-4f && peakOf(outs[0].L) > 0.05f, "machine output is the same at block sizes 1, 64, 500, 4096 (max diff %.2g)", diff);
    }
    // Sample rates and a full kit with every effect.
    {
        bool ok = true;
        PatternBank b;
        for (int t = 0; t < 8; ++t) for (int s = 0; s < 16; s += 2) b.patterns[0].tracks[(size_t)t].steps[(size_t)s] = {110, 100, 2, 0, 0, 0, 0};
        for (double fs : {44100.0, 48000.0, 96000.0, 192000.0}) {
            KitParams k;
            for (int t = 0; t < 8; ++t) { k.tracks[(size_t)t].v.model = (Model)(t + 4); k.tracks[(size_t)t].sendA = 0.4f; k.tracks[(size_t)t].sendB = 0.5f; }
            k.master.glueMix = 1.f; k.master.drive = 0.5f; k.macro = {1.f, 1.f, 1.f, 1.f};
            DrumMachine m; m.setParams(k); m.setPatterns(&b); m.prepare(fs, 512);
            m.internalPlay = true;
            std::vector<float> L(512), R(512);
            float pk = 0.f;
            for (int i = 0; i < (int)(fs * 3 / 512); ++i) {
                Outputs o; o.L = L.data(); o.R = R.data();
                m.process(o, 512, nullptr, 0, {});
                if (!finite(L) || !finite(R)) ok = false;
                pk = std::max(pk, std::max(peakOf(L), peakOf(R)));
            }
            if (pk > 0.9f || pk < 0.05f) ok = false;
        }
        CHECK(ok, "full kit with every effect and every macro at 44.1, 48, 96 and 192 kHz: finite, under the limiter, audible");
    }
    // Aux routing: a track on Aux 2 leaves the main mix.
    {
        DrumMachine m; KitParams k = dryKit(); k.tracks[0].note = 36; k.tracks[0].output = 2;
        m.setParams(k); m.setPatterns(&bank); m.prepare(kFs, 256);
        std::vector<float> L(256), R(256), aL(256), aR(256);
        Outputs o; o.L = L.data(); o.R = R.data(); o.auxL[1] = aL.data(); o.auxR[1] = aR.data();
        MidiEvent a; a.type = MidiEvent::NoteOn; a.number = 36; a.value = 1.f;
        m.process(o, 256, &a, 1, {});
        CHECK(peakOf(L) < 1e-7f && peakOf(aL) > 0.05f, "a track routed to Aux 2 plays there and not on the main output");
    }
}

int main(int argc, char** argv) {
    const bool levels = argc > 1 && std::strcmp(argv[1], "--levels") == 0;
    verbose = levels;
    testModels(levels);
    testAliasing();
    testSequencer();
    testMachine();
    std::printf("\n%d failure(s)\n", failures);
    return failures;
}
