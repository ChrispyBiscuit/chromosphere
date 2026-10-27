#pragma once
// The WAVE engine: a voice in, notes out. The tracker gives a frame every hop; the engine gates it, segments it into
// notes (onset, legato note changes after a stability time, re-articulation on level onsets, release after a short
// unvoiced gap), quantizes to the key and scale, builds the melody note or chord, and writes timestamped events:
// note on/off, per-note tuning (the voice's pitch relative to the note, scaled by Follow), and expression
// controllers. Everything is decided on the absolute sample timeline, so the events do not depend on the host's
// block size. prepare() allocates; process() does not.
#include "harmony.h"
#include "tracker.h"
#include <array>
#include <atomic>
#include <cstdint>

namespace wave {

enum class Mode : uint8_t { Melody, Chords, Count };
inline constexpr const char* kModeNames[] = {"Melody", "Chords"};
enum class Expr : uint8_t { Off, Aftertouch, ModWheel, Count };
inline constexpr const char* kExprNames[] = {"Off", "Aftertouch", "Mod wheel"};
inline constexpr const char* kBassNames[] = {"Off", "-1 oct", "-2 oct"};

struct Params {
    int range = 1;                       // index into kRangeNames
    float gainDb = 0.f;                  // input gain
    float gateDb = -45.f;                // level below which nothing plays (6 dB hysteresis)
    int key = 0;
    Scale scale = Scale::Chromatic;
    float follow = 0.f;                  // 0 = locked notes, 1 = the voice's exact pitch (per-note tuning)
    float stabilityMs = 50.f;            // how long a new pitch must hold before the note changes
    int octave = 0;                      // output transpose in octaves
    float dynamics = 0.7f;               // 0 = fixed velocity, 1 = velocity follows loudness fully
    Mode mode = Mode::Melody;
    ChordType chord = ChordType::Triad;
    Role role = Role::Root;
    Voicing voicing = Voicing::Smooth;
    int bass = 0;                        // 0 off, 1 or 2 octaves below the chord root
    float strumMs = 0.f;                 // delay between successive chord tones, low to high
    bool hold = false;                   // notes sound until the next sung note
    Expr levelTo = Expr::Aftertouch, brightTo = Expr::Off;
};

struct Event {
    enum Type : uint8_t { NoteOn, NoteOff, Tuning, Aftertouch, ModWheel };
    int offset = 0;                      // sample offset in the block
    Type type = NoteOn;
    int note = 0;
    float value = 0.f;                   // velocity 0..1, tuning in semitones, controller 0..1
};

class Engine {
public:
    Engine();
    void prepare(double fs);
    void reset();                                         // forgets notes without events (after prepare, or a host reset)
    void setParams(const Params& p);
    const Params& params() const { return p_; }
    // Mono input (nullptr = silence). Writes events in time order; returns their number (at most maxOut).
    int process(const float* in, int n, Event* out, int maxOut);
    // Note-offs for every sounding note (WAVE switched off, mode changes), at the given offset.
    int releaseAll(Event* out, int maxOut, int offset = 0);
    int soundingCount() const { return soundingN_; }
    double hopSeconds() const { return tr_.hopSeconds(); }
    double analysisRate() const { return tr_.analysisRate(); }

    // ---- Key listening (UI thread) ----
    void listen(bool on);
    bool listening() const { return listening_.load(); }
    KeyGuess guessKey() const;
    float listenedSeconds() const;

    // ---- Read by the UI (any thread) ----
    static constexpr int kTrace = 2048;                   // per-hop history: about 5.5 s
    std::array<std::atomic<float>, kTrace> tracePitch{};  // fractional MIDI note, -1 unvoiced
    std::array<std::atomic<float>, kTrace> traceLevel{};  // dB
    std::array<std::atomic<int>, kTrace> traceNote{};     // the sung (quantized) note, -1 none
    std::atomic<int> tracePos{0};
    std::atomic<float> pitchNow{-1.f}, levelNow{-120.f}, brightNow{0.f};
    std::atomic<int> noteNow{-1}, chordRoot{-1}, chordMask{0};

private:
    struct Out {
        Event* ev; int max; int n = 0;
        void add(int offset, Event::Type t, int note, float v) { if (n < max) ev[n++] = {offset, t, note, v}; }
    };
    void onFrame(int offset, Out& o);
    void startNote(int offset, float pitch, float peakDb, Out& o);
    void changeNote(int offset, int sung, float pitch, Out& o);
    void endNote(int offset, Out& o);
    Chord makeChord(int sung);
    void play(int offset, const Chord& c, float vel, bool legato, Out& o);
    void noteOn(int offset, int note, float vel, Out& o);
    void noteOff(int offset, int note, Out& o);
    void cancelPending() { pendingN_ = pendingHead_ = 0; }
    float velocity(float peakDb) const;
    void expression(int offset, const Frame& f, Out& o);

    Params p_;
    Tracker tr_;
    double fs_ = 48000.0;
    int64_t now_ = 0;
    float gain_ = 1.f;
    bool gateOpen_ = false;

    // Segmentation.
    bool active_ = false;                 // a sung note is in progress
    int sung_ = -1;                       // its quantized note (before the octave shift)
    float vel_ = 0.8f;
    int unvoicedHops_ = 0, changeTarget_ = -1, changeHops_ = 0, hopsSinceOnset_ = 0;
    float cand_[3] = {}; int candN_ = 0; float candPeak_ = -120.f;
    float med_[3] = {-1.f, -1.f, -1.f};   // the last three pitches (-1 unvoiced)
    bool dipped_ = false;                 // re-articulation: the level fell well under the note's peak
    float peakDb_ = -120.f, dipLow_ = -120.f, lastDb_ = -120.f;
    Chord prevChord_{};                   // before the octave shift, for Smooth voicing

    // Output notes.
    std::array<bool, 128> on_{};
    int sounding_[16] = {}; int soundingN_ = 0;
    struct Pending { int64_t due; int note; float vel; };
    Pending pending_[16] = {}; int pendingN_ = 0, pendingHead_ = 0;
    float tune_ = 0.f, tuneSent_ = 0.f;
    int exprCount_ = 0;
    float sentAt_ = 0.f, sentMw_ = 0.f;

    // Key listening.
    std::array<std::atomic<float>, 12> hist_{};
    std::atomic<bool> listening_{false};
    std::atomic<bool> clearHist_{false};
};

}  // namespace wave
