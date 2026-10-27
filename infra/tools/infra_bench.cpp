// CPU benchmark at 48 kHz. The PRD gate (G5) is the first line: 16 voices, both oscillators with 4 unison
// voices, LP 24, all effects on, under 15% of one core. The other lines show where the time goes.
#include "infra/synth.h"
#include <chrono>
#include <cstdio>
#include <functional>
#include <vector>

using namespace infra;

static double run(const SynthParams& p, int notes) {
    Synth s; s.setParams(p);
    const double fs = 48000.0; const int block = 256;
    s.prepare(fs, block);
    std::vector<float> L(block), R(block);
    std::vector<MidiEvent> ev;
    for (int i = 0; i < notes; ++i) { MidiEvent e; e.type = MidiEvent::NoteOn; e.number = 36 + i * 3; e.value = 0.8f; e.offset = i; ev.push_back(e); }
    s.process(L.data(), R.data(), block, ev.data(), (int)ev.size(), {});
    const int blocks = (int)(fs * 10 / block);
    const auto t0 = std::chrono::steady_clock::now();
    for (int b = 0; b < blocks; ++b) s.process(L.data(), R.data(), block, nullptr, 0, {});
    return 100.0 * std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() / 10.0;
}

int main() {
    SynthParams p;
    p.polyphony = 16;
    for (auto& o : p.osc) { o.unison = 4; o.detune = 0.3f; o.level = 0.6f; }
    p.osc[1].table = 1; p.osc[1].morph = 0.4f;
    p.osc[0].warp = Warp::Bend; p.osc[0].warpAmt = 0.4f;
    p.subLevel = 0.3f; p.noiseLevel = 0.05f;
    p.mod[0] = {ModSource::Lfo1, ModDest::Cutoff, 0.2f};
    p.mod[1] = {ModSource::ModEnv, ModDest::MorphA, 0.5f};
    p.fx.drive = 0.3f; p.fx.chorusMix = 0.5f; p.fx.delayMix = 0.3f; p.fx.reverbMix = 0.3f;
    p.ampEnv.s = 1.f;
    const double full = run(p, 16);
    std::printf("PRD gate: 16 voices x 2 osc x 4 unison, all effects: %.2f%% of one core (target < 15%%)\n", full);
    SynthParams noFx = p; noFx.fx.drive = 0.f; noFx.fx.chorusMix = 0.f; noFx.fx.delayMix = 0.f; noFx.fx.reverbMix = 0.f;
    std::printf("  voices only:                                   %.2f%%\n", run(noFx, 16));
    std::printf("  effects only (no notes):                       %.2f%%\n", run(p, 0));
    SynthParams uni1 = noFx; for (auto& o : uni1.osc) o.unison = 1;
    std::printf("  voices, unison 1:                              %.2f%%\n", run(uni1, 16));
    SynthParams warp0 = noFx; warp0.osc[0].warp = Warp::Off;
    std::printf("  voices, no warp:                               %.2f%%\n", run(warp0, 16));
    return full < 15.0 ? 0 : 1;
}
