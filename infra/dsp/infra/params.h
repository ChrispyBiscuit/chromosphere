#pragma once
// Plain parameter model for the INFRA engine. No JUCE. The plugin copies its parameter values into a
// SynthParams once per block; the engine reads only this struct.
//
// The name lists below are the single source of truth for every choice parameter: the plugin builds its
// choice parameters from them and the preset script uses the same strings.
#include <array>
#include <cstddef>

namespace infra {

constexpr int kNumTables = 10;
constexpr int kMaxVoices = 16;
constexpr int kMaxUnison = 8;
constexpr int kNumMacros = 4;
constexpr int kNumModSlots = 8;
constexpr int kArpSteps = 16;

inline constexpr const char* kTableNames[kNumTables] = {
    "Classic", "Fraunhofer", "Pulsar", "Nebula", "Ion Drive", "Magnetar", "Plasma", "Event Horizon", "Dark Matter", "Quasar"};

enum class Warp { Off, Bend, Mirror, Quantize, FM, Ring };
inline constexpr const char* kWarpNames[] = {"Off", "Bend", "Mirror", "Quantize", "FM from B", "Ring by B"};
constexpr int kNumWarps = 6;

enum class SubShape { Sine, Triangle, Square };
inline constexpr const char* kSubShapeNames[] = {"Sine", "Triangle", "Square"};

enum class NoiseColour { White, Pink, SolarWind, Crackle };
inline constexpr const char* kNoiseNames[] = {"White", "Pink", "Solar Wind", "Crackle"};

enum class FilterType { LP24, LP12, BP12, HP12, HP24, Notch, Comb, Formant };
inline constexpr const char* kFilterNames[] = {"LP 24", "LP 12", "BP 12", "HP 12", "HP 24", "Notch", "Comb", "Formant"};
constexpr int kNumFilterTypes = 8;

enum class LfoShape { Sine, Triangle, SawUp, SawDown, Square, SampleHold, Drift };
inline constexpr const char* kLfoShapeNames[] = {"Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H", "Drift"};

// Tempo divisions shared by the LFOs, the arp and the delay. Length in quarter notes.
inline constexpr const char* kDivisionNames[] = {"8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/2T", "1/4.", "1/4", "1/4T",
                                                 "1/8.", "1/8", "1/8T", "1/16.", "1/16", "1/16T", "1/32"};
inline constexpr double kDivisionBeats[] = {32.0, 16.0, 8.0, 4.0, 2.0, 4.0 / 3.0, 1.5, 1.0, 2.0 / 3.0,
                                            0.75, 0.5, 1.0 / 3.0, 0.375, 0.25, 1.0 / 6.0, 0.125};
constexpr int kNumDivisions = 16;

enum class ModSource {
    None, Lfo1, Lfo2, ModEnv, FilterEnv, AmpEnv, Velocity, Key, ModWheel, Aftertouch, PitchBend,
    Macro1, Macro2, Macro3, Macro4, Random, ArpStep, Drift, Count
};
inline constexpr const char* kModSourceNames[] = {"None", "LFO 1", "LFO 2", "Mod Env", "Filter Env", "Amp Env", "Velocity", "Key",
                                                  "Mod Wheel", "Aftertouch", "Pitch Bend", "Heat", "Gravity", "Orbit", "Flux",
                                                  "Random", "Arp Step", "Drift"};
constexpr int kNumModSources = (int)ModSource::Count;

enum class ModDest {
    None, Pitch, PitchA, PitchB, MorphA, MorphB, WarpA, WarpB, LevelA, LevelB, SubLevel, NoiseLevel, Detune,
    Cutoff, Resonance, FilterDrive, Amp, Pan, Lfo1Rate, Lfo2Rate, Lfo1Amt, Lfo2Amt,
    // LFO 1 / 2 Amount: when any slot targets one, that LFO's level is the sum of those slots (0..2) instead of 1,
    // so the mod wheel can bring in vibrato.
    // Global destinations (evaluated once per block with global sources; per-voice sources read as 0 there,
    // except the LFOs, which use the free-running global phase).
    FxDrive, FxChorus, FxDelay, FxFeedback, FxReverb, Count
};
inline constexpr const char* kModDestNames[] = {"None", "Pitch", "Pitch A", "Pitch B", "Morph A", "Morph B", "Warp A", "Warp B",
                                                "Level A", "Level B", "Sub Level", "Noise Level", "Detune", "Cutoff", "Resonance",
                                                "Filter Drive", "Amp", "Pan", "LFO 1 Rate", "LFO 2 Rate", "LFO 1 Amount", "LFO 2 Amount",
                                                "FX Drive", "FX Chorus", "FX Delay", "FX Feedback", "FX Reverb"};
constexpr int kNumModDests = (int)ModDest::Count;
constexpr int kFirstGlobalDest = (int)ModDest::FxDrive;

enum class VoiceMode { Poly, Mono, Legato };
inline constexpr const char* kVoiceModeNames[] = {"Poly", "Mono", "Legato"};

enum class ArpMode { Up, Down, UpDown, DownUp, Played, Random, Converge, Diverge, Chord, Orbit, Pulsar };
inline constexpr const char* kArpModeNames[] = {"Up", "Down", "Up/Down", "Down/Up", "As Played", "Random",
                                                "Converge", "Diverge", "Chord", "Orbit", "Pulsar"};
constexpr int kNumArpModes = 11;

enum class DriveMode { Soft, Tube, Fold, CosmicRay };
inline constexpr const char* kDriveModeNames[] = {"Soft", "Tube", "Fold", "Cosmic Ray"};

struct OscParams {
    int table = 0;
    float morph = 0.f;              // 0..1
    int octave = 0, semi = 0;       // -3..3, -12..12
    float fine = 0.f;               // cents -100..100
    float level = 0.8f;             // 0..1
    float pan = 0.f;                // -1..1
    int unison = 1;                 // 1..8
    float detune = 0.2f;            // 0..1 (1 = +-100 cents at the outer voices)
    float width = 0.5f;             // 0..1 stereo spread of the unison voices
    Warp warp = Warp::Off;
    float warpAmt = 0.f;            // 0..1
    float phase = 0.f;              // 0..1 start phase
    float phaseRand = 1.f;          // 0..1 random start phase amount
    bool on = true;
};

struct EnvParams { float a = 0.005f, d = 0.3f, s = 0.8f, r = 0.3f; };   // seconds, sustain 0..1

struct LfoParams {
    LfoShape shape = LfoShape::Sine;
    float rateHz = 2.f;
    bool sync = false;
    int division = 10;              // index into kDivisionBeats
    bool retrig = true;
    float fade = 0.f;               // seconds
};

struct ModSlot { ModSource src = ModSource::None; ModDest dst = ModDest::None; float amount = 0.f; };

struct ArpStep { float velocity = 1.f; int transpose = 0; int ratchet = 1; };

struct ArpParams {
    bool on = false;
    ArpMode mode = ArpMode::Up;
    int division = 13;              // 1/16
    int octaves = 1;                // 1..4
    float gate = 0.6f;              // 0.05..2
    float swing = 0.f;              // 0..1 (1 = odd steps late by half a step)
    bool latch = false;
    int length = 16;                // 1..16
    float probability = 1.f;        // 0..1
    std::array<ArpStep, kArpSteps> steps{};
};

struct FxParams {
    DriveMode driveMode = DriveMode::Soft;
    float drive = 0.f, driveTone = 0.7f, driveMix = 1.f;
    float chorusRate = 0.4f, chorusDepth = 0.5f, chorusMix = 0.f;
    bool delaySync = true; int delayDivision = 10; float delayMs = 375.f;
    float delayFeedback = 0.35f, delayDamp = 0.4f, delayMix = 0.f; bool delayPingPong = true;
    float reverbSize = 1.f, reverbDecay = 2.5f, reverbDamp = 0.4f, reverbPreDelay = 10.f, reverbMix = 0.f;
    float width = 1.f;              // 0..2
    float driveLowCut = 0.f;        // Hz, 0 = drive everything; else the lows below it pass the drive clean
    float squash = 0.f;             // OTT-style three-band compression, 0..1
    float monoBelow = 0.f;          // Hz, 0 = off; else no stereo below it
    float outDb = 0.f;              // output level
};

struct SynthParams {
    std::array<OscParams, 2> osc{};
    SubShape subShape = SubShape::Sine; int subOctave = 1; float subLevel = 0.f;
    NoiseColour noiseColour = NoiseColour::White; float noiseLevel = 0.f;

    FilterType filterType = FilterType::LP24;
    float cutoff = 8000.f, resonance = 0.1f, filterDrive = 0.f;
    float filterEnvAmt = 0.f;       // semitones -72..72
    float keyTrack = 0.5f;          // 0..1
    float filterVel = 0.f;          // 0..1: velocity adds up to 36 semitones

    EnvParams ampEnv{0.005f, 0.4f, 0.8f, 0.3f}, filterEnv{0.005f, 0.4f, 0.3f, 0.4f}, modEnv{0.2f, 0.6f, 0.f, 0.5f};
    float ampCurve = 0.f;           // -1..1 attack shape
    float velSens = 0.6f;           // 0..1: how much velocity scales the amp
    std::array<LfoParams, 2> lfo{};
    std::array<ModSlot, kNumModSlots> mod{};
    std::array<float, kNumMacros> macro{};      // 0..1

    VoiceMode voiceMode = VoiceMode::Poly;
    int polyphony = 12;
    float glide = 0.f;              // seconds
    bool glideLegatoOnly = false;
    int bendRange = 2;
    float drift = 0.2f;             // 0..1 analog drift
    float masterTune = 0.f;         // cents

    ArpParams arp{};
    FxParams fx{};
};

}  // namespace infra
