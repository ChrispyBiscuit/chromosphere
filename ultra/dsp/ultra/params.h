#pragma once
// Plain parameter and pattern model for the ULTRA engine. No JUCE. The plugin copies its parameter values into a
// KitParams once per block and publishes pattern edits as a PatternBank; the engine reads only these structs.
//
// The name lists below are the single source of truth for every choice parameter: the plugin builds its choice
// parameters from them and the preset script uses the same strings.
#include <array>
#include <cstddef>
#include <cstdint>

namespace ultra {

constexpr int kTracks = 8;
constexpr int kPatterns = 8;
constexpr int kMaxSteps = 32;
constexpr int kNumMacros = 4;
constexpr int kNumAux = 8;
constexpr int kChokeGroups = 4;

// ---- Drum models ----
enum class Model { CoreKick, GravityKick, FlareSnare, BurstClap, IonHat, CoronaCymbal, OrbitTom, PulsarFm,
                   QuasarMetal, SparkRim, SolarWind, PlasmaZap, NebulaModal, Count };
constexpr int kNumModels = (int)Model::Count;
inline constexpr const char* kModelNames[kNumModels] = {
    "Core Kick", "Gravity Kick", "Flare Snare", "Burst Clap", "Ion Hat", "Corona Cymbal", "Orbit Tom",
    "Pulsar FM", "Quasar Metal", "Spark Rim", "Solar Wind", "Plasma Zap", "Nebula Modal"};
// What the six dials mean on each model (Tune, Decay, Tone, Snap, Color, Sweep): the VOICE page shows these.
inline constexpr const char* kModelDialNames[kNumModels][6] = {
    {"Tune", "Decay", "Tone", "Click", "Shape", "Sweep"},
    {"Tune", "Decay", "FM decay", "Click", "FM", "Sweep"},
    {"Tune", "Decay", "Tone", "Snappy", "Noise", "Sweep"},
    {"Tune", "Tail", "Tone", "Sharp", "Bursts", "Room"},
    {"Tune", "Decay", "Tone", "Attack", "Noise", "Sweep"},
    {"Tune", "Decay", "Tone", "Attack", "Shimmer", "Wash"},
    {"Tune", "Decay", "Skin", "Stick", "Shape", "Sweep"},
    {"Tune", "Decay", "Index", "Attack", "Ratio", "Sweep"},
    {"Tune", "Decay", "Tone", "Attack", "Bell", "Detune"},
    {"Tune", "Decay", "Tone", "Click", "Clave", "Sweep"},
    {"Tune", "Decay", "Tone", "Attack", "Colour", "Sweep"},
    {"Tune", "Decay", "Tone", "Speed", "Shape", "Depth"},
    {"Tune", "Decay", "Damping", "Strike", "Material", "Position"},
};
// Kick-like and tom-like models: the GRAVITY macro lengthens and lowers these.
inline bool isWeighty(Model m) { return m == Model::CoreKick || m == Model::GravityKick || m == Model::OrbitTom; }

// ---- Per-track filter: Off plus INFRA's eight types ----
enum class FilterType { Off, LP24, LP12, BP12, HP12, HP24, Notch, Comb, Formant };
inline constexpr const char* kFilterNames[] = {"Off", "LP 24", "LP 12", "BP 12", "HP 12", "HP 24", "Notch", "Comb", "Formant"};
constexpr int kNumFilterTypes = 9;

// ---- Lock lane target ----
enum class LockTarget { Decay, Tone, Snap, Color, Sweep, Drive, Cutoff, Pan, SendA, SendB };
inline constexpr const char* kLockNames[] = {"Decay", "Tone", "Snap", "Color", "Sweep", "Drive", "Cutoff", "Pan", "Send A", "Send B"};
constexpr int kNumLockTargets = 10;

// ---- Track rates (step length in quarter notes) ----
inline constexpr const char* kRateNames[] = {"1/32", "1/16T", "1/16", "1/16.", "1/8T", "1/8", "1/4"};
inline constexpr double kRateBeats[] = {0.125, 1.0 / 6.0, 0.25, 0.375, 1.0 / 3.0, 0.5, 1.0};
constexpr int kNumRates = 7;
constexpr int kDefaultRate = 2;
// Swing applies to straight 1/32 and 1/16 tracks.
inline bool rateSwings(int rate) { return rate == 0 || rate == 2; }

// ---- Trig conditions ----
enum class Cond { Always, Fill, NotFill, First, NotFirst, C12, C22, C13, C23, C33, C14, C24, C34, C44 };
inline constexpr const char* kCondNames[] = {"Always", "Fill", "Not fill", "1st", "Not 1st", "1:2", "2:2",
                                             "1:3", "2:3", "3:3", "1:4", "2:4", "3:4", "4:4"};
constexpr int kNumConds = 14;

// ---- Master effects ----
enum class DriveMode { Soft, Tube, Fold, CosmicRay };
inline constexpr const char* kDriveModeNames[] = {"Soft", "Tube", "Fold", "Cosmic Ray"};

// Tempo divisions for the echo. Length in quarter notes (INFRA's list).
inline constexpr const char* kDivisionNames[] = {"8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/2T", "1/4.", "1/4", "1/4T",
                                                 "1/8.", "1/8", "1/8T", "1/16.", "1/16", "1/16T", "1/32"};
inline constexpr double kDivisionBeats[] = {32.0, 16.0, 8.0, 4.0, 2.0, 4.0 / 3.0, 1.5, 1.0, 2.0 / 3.0,
                                            0.75, 0.5, 1.0 / 3.0, 0.375, 0.25, 1.0 / 6.0, 0.125};
constexpr int kNumDivisions = 16;

inline constexpr const char* kOutputNames[] = {"Main", "Aux 1", "Aux 2", "Aux 3", "Aux 4", "Aux 5", "Aux 6", "Aux 7", "Aux 8"};
inline constexpr const char* kChokeNames[] = {"None", "1", "2", "3", "4"};
inline constexpr const char* kPatternNames[] = {"A", "B", "C", "D", "E", "F", "G", "H"};
inline constexpr const char* kMacroNames[kNumMacros] = {"Heat", "Gravity", "Orbit", "Flux"};

// ---- Sound of one track (captured at trigger time, except where noted) ----
struct VoiceParams {
    Model model = Model::CoreKick;
    float tune = 0.f;               // semitones -24..24
    float decay = 0.5f, tone = 0.5f, snap = 0.5f, color = 0.5f, sweep = 0.5f;   // 0..1
    float drive = 0.f;              // 0..1
    FilterType filter = FilterType::Off;
    float cutoff = 20000.f;         // Hz (live)
    float resonance = 0.1f;         // 0..1 (live)
    float velSens = 0.7f;           // 0..1
};

struct TrackParams {
    VoiceParams v{};
    float levelDb = 0.f;            // -60..+6 (live)
    float pan = 0.f;                // -1..1 (live)
    float sendA = 0.f, sendB = 0.f; // 0..1 echo and reverb sends (live)
    bool mute = false, solo = false;
    int choke = 0;                  // 0 none, 1..4
    int note = 36;                  // MIDI note that plays this track
    int output = 0;                 // 0 main, 1..8 aux
    LockTarget lock = LockTarget::Decay;
};

struct MasterParams {
    DriveMode driveMode = DriveMode::Soft;
    float drive = 0.f, driveTone = 0.7f, driveMix = 1.f;
    float glueThresh = -12.f;       // dB
    float glueRatio = 2.f;          // 1..10
    float glueAttack = 10.f;        // ms
    float glueRelease = 200.f;      // ms
    float glueMakeup = 0.f;         // dB
    float glueMix = 0.f;            // 0..1 (0 = bypass)
    bool echoSync = true; int echoDivision = 9; float echoMs = 300.f;
    float echoFeedback = 0.35f, echoDamp = 0.4f, echoReturn = 0.8f, echoToReverb = 0.f;
    bool echoPingPong = true;
    float reverbSize = 1.f, reverbDecay = 1.6f, reverbDamp = 0.4f, reverbPreDelay = 8.f, reverbReturn = 0.8f;
    float lowShelf = 0.f, highShelf = 0.f;   // dB
    float width = 1.f;              // 0..2
    float outDb = 0.f;
};

struct KitParams {
    std::array<TrackParams, kTracks> tracks{};
    MasterParams master{};
    std::array<float, kNumMacros> macro{};   // 0..1
    int pattern = 0;                // 0..7 (queued to the next bar while playing)
    bool fill = false;
    float swing = 0.5f;             // 0.5..0.75
    float tempo = 120.f;            // used when the host gives no tempo
};

// ---- Patterns ----
struct Step {
    uint8_t vel = 0;                // 0..127, 0 = no hit
    uint8_t chance = 100;           // 0..100
    uint8_t ratchet = 1;            // 1..4
    int8_t pitch = 0;               // -24..24
    int8_t nudge = 0;               // -50..50 (% of a step)
    uint8_t cond = 0;               // Cond
    int8_t lock = 0;                // -100..100 (% offset of the lock target)
};

struct TrackPattern {
    int length = 16;                // 1..32
    int rate = kDefaultRate;        // index into kRateBeats
    std::array<Step, kMaxSteps> steps{};
};

struct Pattern { std::array<TrackPattern, kTracks> tracks{}; };
struct PatternBank { std::array<Pattern, kPatterns> patterns{}; };

}  // namespace ultra
