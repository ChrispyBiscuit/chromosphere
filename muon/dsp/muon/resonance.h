#pragma once
// RESONANCE (PRD 4.5): resonators struck by the transients, and the pitch sources that tune them.
//  - Tuned: up to 8 two-pole resonators on a chord (or the scale in Key mode, held notes in MIDI mode).
//  - Body: 16 modes with the frequency ratios of a material, scaled by Size.
//  - Comb: a feedback comb per channel with a damping low-pass in the loop.
// Each resonator is y = 2 r cos(w) y1 - r^2 y2 + sin(w) k x: a unit impulse rings at amplitude k, decaying to
// -60 dB in the mode's T60. Excitation is mono; resonators are panned alternately by Stereo.
#include "filters.h"
#include "params.h"
#include <vector>

namespace muon {

// Monophonic pitch tracker (YIN) on a mono signal decimated to about 6 kHz. Range 30 .. 1000 Hz.
class PitchTracker {
public:
    void prepare(double fs);
    void reset();
    void push(const float* x, int n);           // runs the analysis every hop
    float hz() const { return hz_; }            // last voiced pitch (0 until the first)
    bool voiced() const { return voiced_; }
private:
    void analyse();
    double fs_ = 48000.0, fsd_ = 6000.0;
    int dec_ = 8, phase_ = 0, hop_ = 128, sinceHop_ = 0, win_ = 200, tauMax_ = 200, tauMin_ = 6;
    Svf aa1_, aa2_;
    std::vector<float> ring_, d_;
    int pos_ = 0;
    float hz_ = 0.f; bool voiced_ = false;
};

class ResonatorBank {
public:
    void prepare(double fs);
    void reset();
    // Notes from the MIDI pitch source (applied at the block start).
    void noteOn(int note);
    void noteOff(int note);
    // excitation: mono; out added? No: written to outL/outR. trackedHz: from the pitch tracker (0 = none).
    void process(const float* excitation, float* outL, float* outR, int n, const Params& p, float trackedHz);
    int activeCount() const { return count_; }
    float frequency(int i) const { return freq_[i]; }   // current (smoothed) frequency of resonator i
    // Target frequencies and amplitudes for a parameter set (no smoothing); exposed for tests.
    int targets(const Params& p, float trackedHz, float* hz, float* amp) const;

private:
    void updateCoefs(const Params& p);
    double fs_ = 48000.0;
    struct Res { float c1 = 0, c2 = 0, g = 0, y1 = 0, y2 = 0, panL = 0.7f, panR = 0.7f; };
    Res res_[kMaxResonators];
    float freq_[kMaxResonators] = {}, target_[kMaxResonators] = {}, amp_[kMaxResonators] = {};
    int count_ = 0;
    long long t_ = 0;
    OnePole toneLp_;
    // Comb, one per channel.
    Delay comb_[2];
    float combLp_[2] = {0.f, 0.f};
    DcBlock combDc_[2];
    float combHz_ = 110.f;
    // MIDI notes held, most recent last.
    int held_[8] = {}; int nHeld_ = 0;
    int lastNotes_[8] = {45}; int nLast_ = 1;
    bool first_ = true;
};

}  // namespace muon
