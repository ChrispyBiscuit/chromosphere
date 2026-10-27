#pragma once
// One drum voice: a model (Section 5.4 of the PRD) captured at trigger time, then the track's drive and filter.
// Mono; the machine pans it and feeds the sends. No allocation after prepare().
//
// Engine 2 (voice.cpp): every model is built for electronic music (deep house, tech house, techno, trance, drum and
// bass): circuit-style analog kicks with post-envelope saturation, layered hard kicks, two-mode snares with wire
// noise, multi-hand claps, steep metallic hats, ride and crash, tension-bent toms and congas, FM, cowbell and bell,
// resonator rims, granular shakers and risers, zaps and mallet-struck modal percussion. Each hit varies very
// slightly (pitch, decay, level), the way an analog machine does.
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
    float level() const { return e_[0] * gain_; }

    // Per-hit mix offsets from a lock lane (set by the machine at trigger time) and smoothed mix gains.
    float lockPan = 0.f, lockSendA = 0.f, lockSendB = 0.f, cutoffMul = 1.f;
    float mixL = 0.f, mixR = 0.f, mixA = 0.f, mixB = 0.f;
    bool mixInit = false;

    // Model base frequency and decay range, for the UI and tests.
    static float baseHz(Model m);
    static void decayRange(Model m, float& lo, float& hi);

private:
    static constexpr int kEnv = 6, kOsc = 8, kRes = 8, kFlt = 4, kK = 12;
    float renderSample();
    bool finished() const;
    void startModel();
    void retune(int filter, float hz, float k);    // set one of the model's SVFs

    double fs_ = 48000.0;
    float fsf_ = 48000.f, invFs_ = 1.f / 48000.f, nyq_ = 21600.f;
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

    // Model state. e_[0] is always the longest (master) envelope; every envelope multiplies by its coefficient.
    float e_[kEnv] = {}, ec_[kEnv] = {};
    float ph_[kOsc] = {}, inc_[kOsc] = {};
    Svf s_[kFlt];
    Svf::Coef c_[kFlt]{};
    float k_[kK] = {};                             // per-model constants (meaning noted in startModel)
    float lp_[4] = {};                             // one-pole states
    // Two-pole resonators (modal models, rim): y = b x + a1 y1 - a2 y2.
    float r1_[kRes] = {}, r2_[kRes] = {}, ra1_[kRes] = {}, ra2_[kRes] = {}, rb_[kRes] = {}, ramp_[kRes] = {};
    int modes_ = 0;
    unsigned levelEnvs_ = 0x3f;                    // envelopes that carry level (finished() waits for these)
    int count_ = 0, n1_ = 0, n2_ = 0, next_ = 0;   // per-model counters
    float last_ = 0.f;                             // previous sample of a feedback path (FM)
    // Curved master envelope (kicks, toms): the decay rate grows over the hit, so the body holds and then falls.
    bool curved_ = false;
    double cEnv_ = 0.0, cCoef_ = 1.0, cCoef2_ = 1.0;
};

}  // namespace ultra
