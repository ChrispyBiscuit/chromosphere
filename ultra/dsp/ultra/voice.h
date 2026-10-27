#pragma once
// One drum voice: a model (Section 5.4 of the PRD) captured at trigger time, then the track's drive and filter.
// Mono; the machine pans it and feeds the sends. No allocation after prepare().
#include "blocks.h"
#include "params.h"

namespace ultra {

class DrumVoice {
public:
    void prepare(double fs, uint32_t seed);
    // v holds the dials with every offset (step pitch and lock, macros, velocity tone) already applied.
    // decayMul scales the model's decay time (GRAVITY). velocityGain is the level from velocity.
    void trigger(const VoiceParams& v, float decayMul, float velocityGain);
    void choke(float ms);                          // fade out (choke group, retrigger)
    void kill() { active_ = false; }
    bool active() const { return active_; }
    bool fading() const { return fadeStep_ > 0.f; }
    uint64_t age() const { return age_; }
    void setAge(uint64_t a) { age_ = a; }
    // Live filter (cutoff and resonance follow the dials while the hit rings). Call before render.
    void setFilter(FilterType type, float cutoffHz, float res);
    // Adds n samples of mono output to out.
    void render(float* out, int n);
    float level() const { return env_ * gain_; }

    // Per-hit mix offsets from a lock lane (set by the machine at trigger time) and smoothed mix gains.
    float lockPan = 0.f, lockSendA = 0.f, lockSendB = 0.f, cutoffMul = 1.f;
    float mixL = 0.f, mixR = 0.f, mixA = 0.f, mixB = 0.f;
    bool mixInit = false;

    // Model base frequency and decay range, for the UI and tests.
    static float baseHz(Model m);
    static void decayRange(Model m, float& lo, float& hi);

private:
    float renderSample();
    bool finished() const;
    void startModel();

    double fs_ = 48000.0;
    float fsf_ = 48000.f, invFs_ = 1.f / 48000.f;
    Rng rng_;
    VoiceFilter filter_;
    bool active_ = false;
    uint64_t age_ = 0;
    VoiceParams p_{};
    float decayMul_ = 1.f, gain_ = 1.f;
    float f0_ = 100.f, T_ = 0.5f;
    int64_t t_ = 0;                                // samples since the trigger

    // Drive.
    float driveG_ = 1.f, driveNorm_ = 1.f;
    // Fade (choke).
    float fade_ = 1.f, fadeStep_ = 0.f;
    // Declick ramp at the start.
    float atk_ = 0.f, atkStep_ = 1.f;

    // Generic model state.
    float env_ = 0.f, envC_ = 0.999f;              // master envelope (longest component)
    float e2_ = 0.f, e2C_ = 0.99f;                 // second component envelope
    float tr_ = 0.f, trC_ = 0.9f;                  // transient envelope
    float pe_ = 0.f, peC_ = 0.99f;                 // pitch / sweep envelope
    float ie_ = 0.f, ieC_ = 0.99f;                 // index / extra envelope
    float ph_[8] = {}, inc_[8] = {};               // oscillator phases and base increments
    float amp_[8] = {}, pEnv_[8] = {}, pC_[8] = {};  // partial amplitudes and envelopes
    float sweepSt_ = 0.f;                          // pitch sweep depth in semitones
    float lp1_ = 0.f, lpC_ = 0.5f;                 // one-pole low-pass
    float k1_ = 1.f, k2_ = 1.f, a_ = 0.f, b_ = 0.f, c_ = 0.f;   // per-model constants
    int count_ = 0, n1_ = 0, n2_ = 0;              // per-model counters
    Svf s1_, s2_, s3_;
    Svf::Coef c1_{}, c2_{}, c3_{};
    float fcBase_ = 1000.f, fcNow_ = -1.f;
    float pinkB0_ = 0.f, pinkB1_ = 0.f, pinkB2_ = 0.f, crackle_ = 0.f;
    // Modal resonators.
    float r1_[6] = {}, r2_[6] = {}, ra1_[6] = {}, ra2_[6] = {}, rb_[6] = {};
    int modes_ = 0;
};

}  // namespace ultra
