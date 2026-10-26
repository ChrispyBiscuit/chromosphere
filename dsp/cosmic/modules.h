#pragma once
// Building blocks for the cosmic reverb suite. All are audio-thread safe after configure():
// no allocation in process(), deterministic for a given seed, any block size.
#include "common/rng.h"
#include <cstdint>
#include <vector>

namespace cs {

// Two-grain crossfading delay-line pitch shifter (the classic "shimmer" shifter). Smooth for
// sustained material; grain length 40-120 ms. Latency: one grain.
class GrainPitchShifter {
public:
    void configure(double fs, double maxGrainMs = 150.0);
    void reset();
    void setSemitones(double st);                 // -24 .. +24
    void setGrainMs(double ms);
    void process(const float* in, float* out, int n);   // mono; outputs may alias inputs
private:
    double fs_ = 48000, ratio_ = 1.0;
    float grain_ = 2400.f, phase_ = 0.f, inc_ = 0.f;
    std::vector<float> buf_;
    int size_ = 1, wp_ = 0;
};

// State-variable filter (Cytomic / Simper), per sample, with a slow LFO on the cutoff.
// mode: 0 low-pass, 1 band-pass, 2 high-pass. Cutoff and LFO are updated every 16 samples.
class SweptFilter {
public:
    void configure(double fs, uint64_t seed);
    void reset();
    void set(int mode, double cutoffHz, double q, double lfoRateHz, double lfoDepthOct, double lfoPhase = 0.0, double randomWalk = 0.0);
    void process(const float* in, float* out, int n);
private:
    void update();
    double fs_ = 48000, cutoff_ = 1000, q_ = 0.7, rate_ = 0.1, depth_ = 0, phase_ = 0, lfo_ = 0, walk_ = 0, walkAmt_ = 0;
    int mode_ = 0, ctr_ = 0;
    float g_ = 0, k_ = 1, a1_ = 0, a2_ = 0, a3_ = 0, ic1_ = 0, ic2_ = 0;
    Rng rng_;
};

// Soft saturation with a DC blocker: drive 0..1 maps to a gentle tanh-like curve. Unity gain at
// small signals so it can sit inside a feedback loop without changing the decay time much.
class Saturator {
public:
    void configure(double fs);
    void reset() { x1_ = y1_ = 0.f; }
    void setDrive(double d);
    void process(const float* in, float* out, int n);
private:
    float pre_ = 1.f, post_ = 1.f, x1_ = 0.f, y1_ = 0.f, dc_ = 0.995f;
};

// Reverse buffer: plays the recent input backwards in overlapping windowed chunks, so the output
// is continuous reversed audio (the "reverse reverb" pre-stage). Latency: one chunk.
class ReverseBuffer {
public:
    void configure(double fs, double maxChunkMs = 2000.0);
    void reset();
    void setChunkMs(double ms);
    void process(const float* in, float* out, int n);
private:
    std::vector<float> buf_;
    int size_ = 1, wp_ = 0, chunk_ = 4800, pos_ = 0;
    double fsForChunk_ = 48000;
};

// Granular cloud: grains taken from the last second of input at random positions, random pitch
// from a small set, random pan. Stereo out. Density = grains per second.
class Granulator {
public:
    void configure(double fs, uint64_t seed, int voices = 24);
    void reset();
    void set(double density, double grainMs, double spreadMs, double octaveUpProb, double octaveDownProb, double width);
    void process(const float* inL, const float* inR, float* outL, float* outR, int n);
private:
    struct Grain { bool on = false; double pos = 0, inc = 1; int len = 0, age = 0; float gl = 0, gr = 0; };
    void spawn();
    double fs_ = 48000, density_ = 20, grain_ = 0.08, spread_ = 0.5, up_ = 0.3, down_ = 0.3, width_ = 1.0, spawnAcc_ = 0;
    std::vector<float> buf_;
    std::vector<Grain> grains_;
    int size_ = 1, wp_ = 0;
    Rng rng_;
};

// Pulse gate: a slow LFO shaped gain (sine to near-square) for rhythmic "pulsar" tails, plus an
// optional envelope-follower duck driven by a side input.
class PulseGate {
public:
    void configure(double fs);
    void reset() { phase_ = 0; env_ = 0; }
    void set(double rateHz, double shape, double depth, double duckAmount, double duckReleaseMs);
    // gates `inL/inR` in place; `side` is the dry input used for ducking (may be null).
    void process(float* l, float* r, const float* sideL, const float* sideR, int n);
private:
    double fs_ = 48000, rate_ = 2, shape_ = 0.5, depth_ = 1, phase_ = 0, duck_ = 0, env_ = 0, rel_ = 0.999;
};

}  // namespace cs
