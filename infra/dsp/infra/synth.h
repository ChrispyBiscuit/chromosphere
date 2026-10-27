#pragma once
// The INFRA engine: key layer (sustain pedal), arpeggiator, voice allocation, voices, effects rack.
// process() takes a block of output and the block's MIDI events (sample offsets); it does not allocate.
#include "arp.h"
#include "fx.h"
#include "voice.h"
#include <array>
#include <atomic>

namespace infra {

struct MidiEvent {
    enum Type : uint8_t { NoteOn, NoteOff, Controller, PitchBend, ChannelPressure, AllNotesOff };
    int offset = 0;              // sample offset in the block
    Type type = NoteOn;
    int number = 0;              // note or controller number
    float value = 0.f;           // velocity 0..1, controller 0..1, pitch bend -1..1, pressure 0..1
};

struct Transport {
    double bpm = 120.0;
    bool playing = false;
    double ppq = 0.0;            // host position at the start of the block, in quarter notes (when playing)
};

class Synth {
public:
    Synth();
    ~Synth();
    void prepare(double fs, int maxBlock);
    void release();
    void reset();                                // all voices off, arp cleared, effects cleared
    void setParams(const SynthParams& p) { p_ = p; }
    const SynthParams& params() const { return p_; }
    void process(float* L, float* R, int n, const MidiEvent* events, int numEvents, const Transport& t);

    // ---- Read by the UI (any thread) ----
    static constexpr int kScopeSize = 2048;
    std::array<std::atomic<float>, kScopeSize> scope{};       // mono output, ring
    std::atomic<int> scopePos{0};
    std::atomic<int> activeVoices{0}, arpLane{-1};
    std::atomic<float> peak{0.f}, bass{0.f}, morphShown[2]{{0.f}, {0.f}};
    std::atomic<float> limiterGain{1.f};

private:
    void handleMidi(const MidiEvent& e, int64_t now);
    void keyOn(int note, float vel, int64_t now);
    void keyOff(int note, int64_t now);
    void voiceOn(int note, float vel, uint32_t id, float stepValue);
    void voiceOff(uint32_t id);
    void allVoicesOff(bool hard);
    void controlAll();
    void renderVoices(float* L, float* R, int n);
    void computeGlobal(int n, FxState& fx);
    static uint32_t directId(int note) { return 0x80000000u | (uint32_t)note; }

    SynthParams p_{};
    double fs_ = 48000.0;
    int maxBlock_ = 512;
    VoiceContext ctx_{};
    std::array<Voice, kMaxVoices> voices_{};
    uint64_t ageCounter_ = 0;
    int64_t clock_ = 0;
    int controlPhase_ = 0;

    // Key layer.
    std::array<bool, 128> keyDown_{}, sustained_{};
    std::array<float, 128> keyVel_{};
    bool sustainPedal_ = false;
    float modWheel_ = 0.f, aftertouch_ = 0.f, bend_ = 0.f;
    float lastPitch_ = -1.f;
    // Mono / legato note stack.
    struct StackEntry { int note; uint32_t id; float vel; float step; };
    std::array<StackEntry, 64> stack_{};
    int stackN_ = 0;

    Arpeggiator arp_;
    bool arpWasOn_ = false;
    std::array<ArpEvent, 64> arpOut_{};

    Lfo globalLfo_[2];
    Rng rng_;

    Drive drive_;
    Chorus chorus_;
    Echo echo_;
    ReverbStage reverb_;
    Limiter limiter_;
    float outGainSmooth_ = 1.f, widthSmooth_ = 1.f, bassLp_ = 0.f;
};

}  // namespace infra
