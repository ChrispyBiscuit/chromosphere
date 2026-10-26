#pragma once
// The cosmic reverbs: the stock tail (early reflections, diffusion, FDN) with a feedback path
// around it that can pitch-shift, saturate and filter, plus optional pre and post stages.
// See docs/COSMIC.md for the ten designs and their macro controls.
#include "cosmic/modules.h"
#include "common/delay_line.h"
#include "reverb.h"
#include <array>

namespace cs {

enum class Recipe { Shimmer, Redshift, Corona, Wormhole, EventHorizon, Nebula, Pulsar, SolarWind, Aurora, Supernova };
constexpr int kRecipeCount = 10;
const char* recipeName(Recipe r);
// Names of the three macro controls for a recipe.
std::array<const char*, 3> recipeMacroNames(Recipe r);

class CosmicEngine {
public:
    // Allocates. `p` sets the tail (size, decay, quality, modulation, seed); its mix/levels are ignored.
    void configure(Recipe r, const ReverbParams& p, double fs, int maxBlock);
    void reset();
    // Audio thread, any block size, in-place safe.
    void process(const float* inL, const float* inR, float* outL, float* outR, int n);
    // Live controls (audio-thread safe, no allocation).
    void setMacros(double a, double b, double c);
    void setLevels(float er, float tail, int rampSamples) { tail_.setMix(0.f, 1.f, er, tail, rampSamples); }
    void setModulation(double depth, double rate) { tail_.setModulation(depth, rate); }
    void setDecay(const DecaySet& d, int ramp) { tail_.setDecay(d, ramp); t60mid_ = d.t60mid; applyMacros(); }
    void setFreeze(bool on, int ramp) { tail_.setFreeze(on, ramp); }
    Recipe recipe() const { return recipe_; }

private:
    static constexpr int kChunk = 32;            // feedback loop delay: fixed, so output is block-size independent
    void processChunk(const float* inL, const float* inR, float* outL, float* outR, int n);
    void applyMacros();
    void watchdog(float* l, float* r, int n);

    Recipe recipe_ = Recipe::Shimmer;
    double fs_ = 48000, a_ = 0.5, b_ = 0.5, c_ = 0.5, t60mid_ = 2.0;
    int maxBlock_ = 512;
    Reverb tail_;
    DelayLine pre_;
    // Feedback path (per channel).
    GrainPitchShifter shiftA_[2], shiftB_[2];
    Saturator sat_[2];
    SweptFilter loopFilter_[2];
    float fb_ = 0.f, shiftMixA_ = 0.f, shiftMixB_ = 0.f, dryLoop_ = 1.f, subMix_ = 0.f;
    bool useSat_ = false, useLoopFilter_ = false;
    // Pre and post stages.
    Granulator gran_;
    ReverseBuffer rev_[2];
    float preMix_ = 0.f;                          // granular / reverse amount vs direct input
    GrainPitchShifter postShift_[2];              // Wormhole chorus (detuned copies)
    SweptFilter postFilter_[2];
    PulseGate gate_;
    float postMix_ = 0.f, width_ = 1.f;
    // Buffers.
    std::vector<float> fbL_, fbR_, xL_, xR_, tL_, tR_, t2L_, t2R_, pL_, pR_;
    // Persistent 32-sample frame: input is collected, processed as a whole chunk, and played out
    // one chunk later, so the feedback delay is exactly one chunk whatever the host block size.
    float inAccL_[kChunk] = {}, inAccR_[kChunk] = {}, outAccL_[kChunk] = {}, outAccR_[kChunk] = {};
    int accPos_ = 0;
    int badCount_ = 0;
};

}  // namespace cs
