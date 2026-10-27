#pragma once
// The vintage suite: reverbs in the manner of hardware from 1970 to 1990. Each unit has its own
// topology, converter character and three macro controls. See docs/VINTAGE.md.
#include "vintage/modules.h"
#include "cosmic/modules.h"
#include "common/delay_line.h"
#include "reverb.h"
#include <array>

namespace cs {

enum class Unit { SteelPlate, SpringTank, Digital76, Concert78, RandomHall86, Nonlinear81, SpaceStation, Hall85 };
constexpr int kUnitCount = 8;
const char* unitName(Unit u);
std::array<const char*, 3> unitMacroNames(Unit u);
// Reverb parameters as this unit wants them (size scaling, ER level, modulation) from the user's.
ReverbParams unitReverbParams(Unit u, const ReverbParams& p);

class VintageEngine {
public:
    void configure(Unit u, const ReverbParams& p, double fs, int maxBlock);
    void reset();
    void process(const float* inL, const float* inR, float* outL, float* outR, int n);
    void setMacros(double a, double b, double c);
    void setLevels(float er, float tail, int rampSamples);
    void setModulation(double depth, double rate);     // honoured only by units whose character is not fixed
    void setDecay(const DecaySet& d, int ramp);
    void setFreeze(bool on, int ramp);
    Unit unit() const { return unit_; }
    bool usesFdn() const { return unit_ != Unit::SpringTank && unit_ != Unit::Nonlinear81 && unit_ != Unit::SpaceStation; }

private:
    void applyMacros();
    Unit unit_ = Unit::SteelPlate;
    double fs_ = 48000, a_ = 0.5, b_ = 0.5, c_ = 0.5, t60mid_ = 2.0, size_ = 1.0;
    int maxBlock_ = 512;
    ReverbParams p_;
    Reverb tail_;
    DelayLine pre_;
    Bandwidth bw_[2], damper_[2];
    Quantizer q_[2];
    DispersionChain disp_[2];
    Saturator sat_[2];
    SpringBank springs_;
    Burst burst_;
    bool useBw_ = false, useDamper_ = false, useQ_ = false, useDisp_ = false, useSat_ = false;
    float lowShelfGain_ = 1.f, lsState_[2] = {0.f, 0.f}, lsK_ = 0.01f;   // Concert '78 bass control
    float gain_ = 1.f, gainTarget_ = 1.f, gainStep_ = 0.f; int gainLeft_ = 0;   // tail level for non-FDN units
    float erLevel_ = 0.5f, tailLevel_ = 1.f;
    std::vector<float> aL_, aR_, bL_, bR_;
};

}  // namespace cs
