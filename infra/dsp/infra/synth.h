#pragma once
// The INFRA engine: key layer (sustain pedal), arpeggiator, voice allocation, voices, effects rack.
// process() takes a block of output and the block's MIDI events (sample offsets); it does not allocate.
#include "arp.h"
#include "fx.h"
#include "voice.h"
#include <array>
#include <atomic>
#include <vector>

namespace infra {

struct MidiEvent {
    // NoteTuning: per-note pitch offset (VST3 note expression, e.g. FL Studio slides and per-note bends). It applies
    // to every voice playing that key until the next note-on of the key resets it.
    enum Type : uint8_t { NoteOn, NoteOff, Controller, PitchBend, ChannelPressure, AllNotesOff, NoteTuning };
    int offset = 0;              // sample offset in the block
    Type type = NoteOn;
    int number = 0;              // note or controller number
    float value = 0.f;           // velocity 0..1, controller 0..1, pitch bend -1..1, pressure 0..1, tuning in semitones
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
    bool sleeping() const { return sleeping_; }  // silent and skipping work (tests)
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
    // Global modulation, on the absolute timeline so the result does not depend on the host's block size: the
    // free-running LFOs advance at each control boundary (every kControl samples since reset), and the effects read
    // a snapshot of the global sources taken at each kFxChunk boundary.
    struct GlobalSnap { float lfo[2] = {0.f, 0.f}, wheel = 0.f, aftertouch = 0.f, bend = 0.f; };
    static constexpr int kFxChunk = 128;          // a multiple of kControl
    void controlTick();                           // one control boundary: global LFOs, then the voices
    void computeGlobal(const GlobalSnap& g, FxState& fx);
    // Calls f(offset, length, fx) for chunks of the block (ctlTime_ = block start) that end at absolute multiples of
    // kFxChunk; each chunk's fx comes from the snapshot in force at its start.
    template <typename F> void forEachFxChunk(int n, F&& f) {
        GlobalSnap g = snapHeld_;
        int idx = 0;
        for (int off = 0; off < n;) {
            const int64_t at = ctlTime_ + off;
            if (off > 0 && at % kFxChunk == 0 && idx < numSnaps_) g = snaps_[(size_t)idx++];
            const int c = std::min(n - off, kFxChunk - (int)(at % kFxChunk));
            FxState fx; computeGlobal(g, fx);
            f(off, c, fx);
            off += c;
        }
        if (idx < numSnaps_) g = snaps_[(size_t)(numSnaps_ - 1)];   // taken at the block's end: in force for the next block
        snapHeld_ = g;
    }
    static uint32_t directId(int note) { return 0x80000000u | (uint32_t)note; }

    SynthParams p_{};
    double fs_ = 48000.0;
    int maxBlock_ = 512;
    VoiceContext ctx_{};
    std::array<Voice, kMaxVoices> voices_{};
    uint64_t ageCounter_ = 0;
    int64_t clock_ = 0;
    int controlPhase_ = 0;
    int64_t ctlTime_ = 0;                         // samples since reset (control and effect boundaries are counted from here)
    GlobalSnap snapHeld_{};                       // the snapshot in force at the start of the block
    std::vector<GlobalSnap> snaps_;               // snapshots taken during this block, in time order (prepare sizes it)
    int numSnaps_ = 0;

    // Key layer.
    std::array<bool, 128> keyDown_{}, sustained_{};
    std::array<float, 128> keyVel_{};
    bool sustainPedal_ = false;
    float modWheel_ = 0.f, aftertouch_ = 0.f, bend_ = 0.f;
    std::array<float, 128> noteTune_{};          // per-key tuning in semitones (NoteTuning)
    static constexpr float kSleepLevel = 1e-6f;  // -120 dBFS
    bool sleeping_ = false;                      // silent: process() skips the voices and effects (see process)
    int64_t silentSamples_ = 0;
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
    Squash squash_;
    Svf sideHp_[2];                              // low mono: a 4th-order high-pass on the side signal
    Chorus chorus_;
    Echo echo_;
    ReverbStage reverb_;
    Limiter limiter_;
    float outGainSmooth_ = 1.f, widthSmooth_ = 1.f, bassLp_ = 0.f;
};

}  // namespace infra
