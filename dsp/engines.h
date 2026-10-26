#pragma once
// The three reverb engines behind one interface, so the host layer can cross-fade between them.
//   Algorithmic: ER taps + diffusion + FDN (Reverb).
//   Convolution: the impulse response through the zero-latency partitioned convolver.
//   Hybrid:      the first erMs of the IR through the convolver, then an FDN tail that is
//                matched to the IR's decay times and level and starts erMs - 30 ms in.
#include "reverb.h"
#include "conv/ir_tools.h"
#include "conv/partitioned_convolver.h"
#include "cosmic/cosmic_engine.h"
#include <memory>

namespace cs {

// Hybrid mode needs an IR whose mid-band T60 is at least this long (seconds); shorter IRs are convolved fully.
constexpr double kHybridMinT60 = 0.6;

enum class EngineMode { Algorithmic, Convolution, Hybrid, Cosmic };

struct EngineSpec {
    EngineMode mode = EngineMode::Algorithmic;
    ReverbParams reverb;                  // size, quality, pre-delay, diffusion, t60, seed (Algorithmic and Hybrid tail)
    std::shared_ptr<const Ir> ir;         // already edited and at the host sample rate (Convolution, Hybrid)
    double erMs = 0;                      // Hybrid: how much of the IR is convolved. 0 = auto (T60/5, 20..80 ms)
    Recipe recipe = Recipe::Shimmer;      // Cosmic: which design
    double macro[3] = {0.5, 0.5, 0.5};    // Cosmic: its three controls, 0..1
};

class Engine {
public:
    virtual ~Engine() = default;
    virtual void process(const float* inL, const float* inR, float* outL, float* outR, int n) = 0;
    // er: algorithmic ER / IR level. tail: algorithmic or hybrid FDN tail level. Linear gains.
    virtual void setLevels(float er, float tail, int rampSamples) = 0;
    virtual void setModulation(double depth, double rateHz) = 0;
    virtual void setDecay(const DecaySet& d, int rampSamples) = 0;
    // Hold the reverb tail and mute the input (Algorithmic and Hybrid; no effect in Convolution mode).
    virtual void setFreeze(bool on, int rampSamples) = 0;
    // Cosmic macro controls; no effect on other engines.
    virtual void setMacros(double, double, double) {}
    virtual bool hasFdn() const = 0;
};

// Needs ir for Convolution and Hybrid; with no IR it builds the algorithmic engine instead.
std::unique_ptr<Engine> makeEngine(const EngineSpec& spec, double fs, int maxBlock);
// Decay filters for the engine's FDN (worker thread). Empty set when the engine has no FDN.
DecaySet designEngineDecay(const EngineSpec& spec, double fs);
// The tail times a Hybrid engine would use for the IR (analysis of the IR, scaled).
void hybridTailT60(const Ir& ir, double scale, double out[3]);

}  // namespace cs
