#pragma once
// The ULTRA engine: sequencer, MIDI pads, eight tracks of drum voices, the mixer (choke groups, sends, aux
// outputs) and the master bus. process() takes a block of outputs and the block's MIDI events; it does not
// allocate.
#include "fx.h"
#include "sequencer.h"
#include "voice.h"
#include <array>
#include <atomic>

namespace ultra {

struct MidiEvent {
    enum Type : uint8_t { NoteOn, NoteOff, AllNotesOff };
    int offset = 0;
    Type type = NoteOn;
    int number = 0;
    float value = 0.f;           // velocity 0..1
};

struct Outputs {
    float* L = nullptr;
    float* R = nullptr;
    std::array<float*, kNumAux> auxL{}, auxR{};   // nullptr: that aux is not enabled
};

class DrumMachine {
public:
    static constexpr int kSlots = 3;               // voices per track (at most two sound fully)
    static constexpr int kSub = 32;                // control sub-block: live parameters move this often

    DrumMachine();
    ~DrumMachine();
    void prepare(double fs, int maxBlock);
    void release();
    void reset();                                  // voices off, effects cleared, sequencer stopped
    void setParams(const KitParams& p) { p_ = p; }
    const KitParams& params() const { return p_; }
    void setPatterns(const PatternBank* bank) { bank_ = bank; }
    void process(const Outputs& out, int n, const MidiEvent* events, int numEvents, const Transport& t);
    // Plays a track now (the UI's pads go through MIDI; tests call this directly).
    void trigger(int track, float velocity, int pitch = 0, float lock = 0.f);

    std::atomic<bool> internalPlay{false};

    // ---- Read by the UI (any thread) ----
    static constexpr int kScopeSize = 2048;
    std::array<std::atomic<float>, kScopeSize> scope{};
    std::atomic<int> scopePos{0};
    std::atomic<float> peak{0.f}, bass{0.f}, limiterGain{1.f}, glueReduction{0.f};
    std::atomic<int> currentPattern{0}, queuedPattern{-1};
    std::atomic<bool> running{false};
    std::atomic<double> position{0.0}, bpm{120.0};
    std::array<std::atomic<int>, kTracks> stepNow{}, hitCount{};
    std::array<std::atomic<float>, kTracks> hitVelocity{}, trackLevel{};
    std::atomic<int> activeVoices{0};
    bool sleeping() const { return sleeping_; }   // audio thread / tests: the engine skipped the last block

private:
    void handleMidi(const MidiEvent& e);
    void renderTracks(const Outputs& out, int pos, int len, bool anySolo);
    float effectiveSwing() const;

    KitParams p_{};
    const PatternBank* bank_ = nullptr;
    PatternBank empty_{};
    double fs_ = 48000.0;
    int maxBlock_ = 512;
    Sequencer seq_;
    std::array<std::array<DrumVoice, kSlots>, kTracks> voices_{};
    uint64_t ageCounter_ = 0;
    std::array<Hit, 512> hits_{};
    std::array<float, kSub> mono_{};
    std::array<float, kTracks> trackPeak_{};

    // Buses (sized in prepare).
    std::vector<float> aL_, aR_, bL_, bR_, eL_, eR_, rL_, rR_;
    Drive drive_;
    Glue glue_;
    Echo echo_;
    ReverbStage reverb_;
    Biquad lowShelf_, highShelf_;
    float lastLow_ = 1e9f, lastHigh_ = 1e9f;
    Limiter limiter_;
    static constexpr float kSleepLevel = 1e-6f;   // -120 dBFS
    bool sleeping_ = false;
    int64_t silentSamples_ = 0;
    float outGainSmooth_ = 1.f, widthSmooth_ = 1.f, retASmooth_ = 0.f, retBSmooth_ = 0.f, bassLp_ = 0.f, dcX_[2] = {}, dcY_[2] = {};
};

}  // namespace ultra
