#pragma once
// Runs the reverb engine safely against live parameter changes. No JUCE dependency.
//
//  - Structural changes (quality, size, pre-delay, diffusion) build a new engine on a
//    worker thread. The audio thread cross-fades to it (equal power, ~150 ms).
//  - Decay changes are designed on the worker thread and interpolated inside the
//    running engine, so the ringing tail is kept.
//  - The audio thread never allocates, locks, or waits. Objects move between threads
//    through single-producer single-consumer pointer queues; the worker frees them.
#include "reverb.h"
#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace fr {

struct EngineSettings {
    Quality quality = Quality::Normal;
    double size = 1.0, preDelayMs = 0.0;
    bool diffusion = true;
    double t60[3] = {3.0, 2.0, 1.0};
};

template <typename T, size_t N>
class SpscQueue {
public:
    bool push(T* p) {
        size_t t = tail_.load(std::memory_order_relaxed), n = (t + 1) % N;
        if (n == head_.load(std::memory_order_acquire)) return false;
        slot_[t] = p; tail_.store(n, std::memory_order_release);
        return true;
    }
    T* pop() {
        size_t h = head_.load(std::memory_order_relaxed);
        if (h == tail_.load(std::memory_order_acquire)) return nullptr;
        T* p = slot_[h]; head_.store((h + 1) % N, std::memory_order_release);
        return p;
    }
private:
    T* slot_[N] = {};
    std::atomic<size_t> head_{0}, tail_{0};
};

class EngineHost {
public:
    using SettingsFn = std::function<EngineSettings()>;   // called on the worker thread
    explicit EngineHost(SettingsFn fn) : settings_(std::move(fn)) {}
    ~EngineHost() { release(); }

    void prepare(double fs, int maxBlock);   // not the audio thread; builds the first engine
    void release();

    // Audio thread. Any n >= 0. outL/outR may alias inL/inR.
    void process(const float* inL, const float* inR, float* outL, float* outR, int n,
                 float erLevel, float tailLevel, double modDepth, double modRateHz);

    // For tests: number of engine builds completed so far.
    int buildsCompleted() const { return builds_.load(); }

private:
    struct Msg { std::unique_ptr<Reverb> engine; std::unique_ptr<DecaySet> decay; uint32_t gen = 0; };
    static constexpr int kChunk = 512;
    static constexpr int kFadeMs = 150;

    void workerLoop();
    std::unique_ptr<Reverb> build(const EngineSettings& s) const;
    void retire(Reverb* r) { if (r && !toWorkerEngines_.push(r)) delete r; /* queue full: should not happen */ }
    void pickUpMessages();

    SettingsFn settings_;
    double fs_ = 48000;
    int maxBlock_ = kChunk;
    std::thread worker_;
    std::atomic<bool> run_{false};
    std::atomic<int> builds_{0};

    SpscQueue<Msg, 16> toAudio_;
    SpscQueue<Msg, 32> toWorkerMsgs_;
    SpscQueue<Reverb, 16> toWorkerEngines_;

    // Audio-thread state.
    std::unique_ptr<Reverb> active_, fading_;
    uint32_t activeGen_ = 0, fadingGen_ = 0;
    int fadePos_ = 0, fadeLen_ = 1;
    float lastEr_ = -1, lastTail_ = -1;
    double lastModDepth_ = -1, lastModRate_ = -1;
    std::vector<float> aL_, aR_, bL_, bR_;
};

}  // namespace fr
