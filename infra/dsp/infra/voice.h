#pragma once
// One synth voice: two wavetable oscillators with unison and warps, sub, noise, stereo filter, three envelopes,
// two LFOs and the per-voice part of the modulation matrix. Modulation runs at control rate (kControl samples);
// oscillators, filter and amp run per sample.
#include "blocks.h"
#include "tables.h"

namespace infra {

constexpr int kControl = 16;

// Everything a voice needs from the engine at control rate.
struct VoiceContext {
    const SynthParams* p = nullptr;
    double fs = 48000.0;
    double bpm = 120.0;
    float modWheel = 0.f, aftertouch = 0.f, bend = 0.f;   // bend -1..1
    float globalLfoPhase[2] = {0.f, 0.f};
};

class Voice {
public:
    void prepare(double fs, uint32_t seed);
    // Start (or restart) a note. glideFrom < 0: no glide. retrigger: restart the envelopes (from their current level).
    void start(const VoiceContext& c, int note, float velocity, uint32_t id, float stepValue, float glideFrom, bool retrigger);
    void legatoTo(int note, uint32_t id, float velocity, float glideFrom);   // mono / legato: new target pitch, no retrigger
    void release();
    void kill() { amp_.kill(); active_ = false; }
    void control(const VoiceContext& c);                 // modulation for the next kControl samples
    void render(float* L, float* R, int n);              // adds into L, R

    bool active() const { return active_; }
    bool gated() const { return gate_; }
    int note() const { return note_; }
    uint32_t id() const { return id_; }
    uint64_t age() const { return age_; }
    float level() const { return amp_.value(); }
    float morphShown(int o) const { return osc_[o].morph; }
    void setAge(uint64_t a) { age_ = a; }

private:
    struct Osc {
        float phase[kMaxUnison] = {};
        float inc[kMaxUnison] = {};
        float gl[kMaxUnison] = {}, gr[kMaxUnison] = {};
        float detuneRand[kMaxUnison] = {};
        float ratio[kMaxUnison] = {};
        float cacheDet = -1.f, cachePan = -9.f, cacheWidth = -9.f; int cacheN = -1;
        int unison = 1;
        int table = 0;
        float morph = 0.f, morphPrev = 0.f;               // 0..1, ramped per sample over the control block
        int L0 = 0; float wl = 0.f;
        Warp warp = Warp::Off; float warpAmt = 0.f;
        float level = 0.f, levelPrev = 0.f;
        bool on = true;
    };
    void renderOsc(Osc& o, int n, int ramp0, bool allowCross, const float* mod, float* outL, float* outR, float* outMono);

    double fs_ = 48000.0;
    Rng rng_;
    bool active_ = false, gate_ = false;
    int note_ = 60;
    uint32_t id_ = 0;
    uint64_t age_ = 0;
    float velocity_ = 1.f, velCurve_ = 1.f, stepValue_ = 1.f, random_ = 0.f;
    float pitch_ = 60.f, target_ = 60.f;                 // current (gliding) and target pitch in semitones
    float glideCoef_ = 1.f;
    float timeSinceOn_ = 0.f;

    Osc osc_[2];
    float subPhase_ = 0.f, subInc_ = 0.f, subLevel_ = 0.f, subLevelPrev_ = 0.f; SubShape subShape_ = SubShape::Sine;
    Noise noise_; float noiseLevel_ = 0.f, noiseLevelPrev_ = 0.f; NoiseColour noiseColour_ = NoiseColour::White;
    VoiceFilter filter_;
    Adsr amp_, fenv_, menv_;
    Lfo lfo_[2], drift_;
    float driftRate_ = 0.4f, driftStatic_ = 0.f;
    float lfoRateMod_[2] = {0.f, 0.f};
    // Output gain ramp (amp mod, velocity, pan) across the control block.
    float gainL_ = 0.f, gainR_ = 0.f, gainLPrev_ = 0.f, gainRPrev_ = 0.f;
    bool firstControl_ = true;
    int rampPos_ = 0;                                    // samples since the last control step
    float dcX_[2] = {}, dcY_[2] = {}, dcR_ = 0.999f;
};

}  // namespace infra
