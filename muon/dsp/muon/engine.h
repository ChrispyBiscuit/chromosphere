#pragma once
// Chromosphere | MUON engine (PRD Section 4). JUCE-free; no allocation, locks or file access after prepare().
//
//   in -> crossover -> per band: differential-envelope detector -> lookahead -> attack/sustain gain
//         -> transient / body split (mask) -> transient clip -> harmonics (oversampled, on the chosen stream)
//      -> band sum (+ phantom cut, mono below) + sub / phantom + resonance -> auto-gain -> + Afterglow return
//      -> mix with the delay-matched dry signal -> output, clip guard, listen
#include "crossover.h"
#include "harmonics.h"
#include "params.h"
#include "resonance.h"
#include "sub.h"
#include <atomic>
#include <memory>

namespace fr { class EngineHost; }

namespace muon {

// One summary of the last 128 samples for the editor's track view (lock-free FIFO, audio -> UI).
struct TrackFrame {
    float inMin = 0, inMax = 0;          // mono input
    float outMin = 0, outMax = 0;        // mono output
    float mask = 0;                      // largest transient mask over the bands, 0..1
    float gainDb = 0;                    // gain applied to the loudest band, dB
    bool onset = false;                  // a transient started in this frame
};

class TrackFifo {
public:
    static constexpr int kSize = 1024;
    bool push(const TrackFrame& f) {
        const int w = w_.load(std::memory_order_relaxed), nx = (w + 1) % kSize;
        if (nx == r_.load(std::memory_order_acquire)) return false;
        buf_[w] = f; w_.store(nx, std::memory_order_release); return true;
    }
    bool pop(TrackFrame& f) {
        const int r = r_.load(std::memory_order_relaxed);
        if (r == w_.load(std::memory_order_acquire)) return false;
        f = buf_[r]; r_.store((r + 1) % kSize, std::memory_order_release); return true;
    }
private:
    TrackFrame buf_[kSize];
    std::atomic<int> w_{0}, r_{0};
};

struct Meters {
    std::atomic<float> inPeak{0}, outPeak{0}, autoGainDb{0}, resonance{0}, bass{0}, trackedHz{0}, limiterGain{1};
    std::atomic<float> bandMask[kMaxBands], bandGainDb[kMaxBands];
    // The resonators' current frequencies (the editor lights them on a keyboard strip).
    std::atomic<int> resCount{0};
    std::atomic<float> resHz[kMaxResonators];
    // The newest output samples (mono), for the editor's analyser.
    static constexpr int kScopeSize = 4096;
    std::atomic<float> scope[kScopeSize];
    std::atomic<int> scopePos{0};
    Meters() {
        for (int i = 0; i < kMaxBands; ++i) { bandMask[i] = 0.f; bandGainDb[i] = 0.f; }
        for (auto& h : resHz) h = 0.f;
        for (auto& s : scope) s = 0.f;
    }
};

class Engine {
public:
    Engine();
    ~Engine();
    // Not the audio thread. Uses the parameters last given to setParams() for the first state.
    void prepare(double fs, int maxBlock);
    void release();
    void reset();
    void setParams(const Params& p);            // audio thread, once per block
    const Params& params() const { return p_; }
    // In place, stereo. Notes: resonator pitch (MIDI pitch source). Any n >= 0.
    void process(float* L, float* R, int n, const NoteEvent* notes = nullptr, int nNotes = 0);
    // Latency in samples for the current parameters (lookahead, linear-phase crossover and oversampling).
    int latency() const { return latencyFor(p_); }
    int latencyFor(const Params& p) const;
    double sampleRate() const { return fs_; }
    // The largest order the order bank uses in band b (PRD 4.4.4), for the current parameters.
    int maxOrder(int band) const;
    // Sleep on silence (optimization guide 5.1): after 0.25 s (plus the latency) of input below -120 dBFS with the
    // output below it too, the engine stops running and outputs silence; any input wakes it on the same block.
    bool sleeping() const { return sleeping_; }

    Meters meters;
    TrackFifo track;

private:
    static constexpr int kChunk = 64;
    struct Smooth {
        float v = 0, t = 0;
        inline float next(float a) { v = t + a * (v - t); if (std::fabs(v - t) < 1e-6f) v = t; return v; }
        void snap() { v = t; }
    };
    struct Detector {
        float e1 = 0, e2 = 0, e3 = 0, g = 1.f;
        int hold = 0;
        float aSlow = 0, aRel = 0, aLong = 0, aGain = 0;
    };
    struct BandState {
        Detector det[2];
        Delay look[2], osDelay[2];
        Adaa clip[2];
        Smooth attack, sustain, amount, drive, driveMix;
        float mask[2] = {0, 0};
        float lastSpeed = -1.f, lastScale = -1.f;
    };
    void processChunk(float* L, float* R, int n);
    void setTimes(BandState& b, float speed, float scale);
    float bandScale(int band, int bands) const;
    float bandEdge(int band, int bands) const;
    int osIndex() const;

    double fs_ = 48000.0;
    int maxBlock_ = 0;
    bool prepared_ = false;
    Params p_;
    long long t_ = 0;
    Crossover xover_;
    BandState band_[kMaxBands];
    // Harmonic units (stereo) for every oversampling factor and phase type: [linear][factor index][band].
    std::unique_ptr<HarmonicBand> harm_[2][4][kMaxBands];
    int harmLin_ = -1, harmOs_ = -1;            // the set in use (reset when it changes)
    int engage_ = 0;                             // cross-fade into the oversampled path (samples)
    SubGenerator sub_;
    ResonatorBank res_;
    PitchTracker tracker_;
    std::unique_ptr<fr::EngineHost> glow_;
    std::atomic<float> glowSize_{1.f}, glowDecay_{2.f}, glowDamp_{0.4f}, glowPre_{10.f};
    int glowHold_ = 0;
    Delay dryDelay_[2], auxDelay_[2];
    Svf cutHp_[2], monoHp_[2];
    float cutHz_ = -1.f, monoHz_ = -1.f;
    // Auto-gain (K-weighting: high shelf + high-pass, then mean square over about 400 ms).
    struct Biquad { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        inline float tick(float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; } };
    Biquad kIn_[2][2], kOut_[2][2];
    float msIn_ = 0, msOut_ = 0, agDb_ = 0.f, agA_ = 0.f, agGainA_ = 0.f;
    Smooth mix_, outGain_, resMix_, glowSend_, glowMix_, subL_, sub2L_, phantom_;
    float limGain_ = 1.f, limRel_ = 0.999f;
    float bassEnv_ = 0.f;
    bool sleeping_ = false;
    long long silent_ = 0;
    float lastWetPeak_ = 1.f;
    int sleepFrames_ = 0;
    // Scratch (kChunk each).
    std::vector<float> bandBuf_[kMaxBands][2], bandOut_[kMaxBands][2], src_[2], oth_[2], direct_[2], tSum_[2], bSum_[2], wet_[2],
        dry_[2], mono_, subOut_, resL_, resR_, glowL_, glowR_, lowMono_, amt_, drv_, dmx_, aux_[2], gainArr_[kMaxBands][2], maskArr_[kMaxBands][2], env2Arr_[kMaxBands][2];
    // Track view accumulation.
    TrackFrame frame_;
    int frameCount_ = 0;
    float prevMask_ = 0.f;
};

}  // namespace muon
