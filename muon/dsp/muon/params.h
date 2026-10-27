#pragma once
// Chromosphere | MUON engine parameters: a plain struct the plugin fills once per block.
// Names and ranges follow the PRD (muon/docs/PRD.md, Section 4).
#include <array>

namespace muon {

constexpr int kMaxBands = 3;
constexpr int kOrders = 8;          // H1 .. H8
constexpr int kMaxResonators = 16;

enum class StereoMode { Stereo, Dual, MidSide };
enum class CrossoverType { MinPhase, LinearPhase };
enum class HarmSource { Both, Transient, Body };
enum class DriveCurve { Soft, Tube, Tape, Diode, Fold, Rectify };
enum class ResMode { Tuned, Body, Comb };
enum class Chord { Unison, Octaves, Fifths, Major, Minor, Sus, Harmonic };
enum class Material { Wood, Skin, Metal, Glass, Tube };
enum class PitchSource { Fixed, Key, Midi, Track };
enum class Scale { Major, Minor, Dorian, Pentatonic, Chromatic };
enum class ResInput { Transients, Full };
enum class GlowSource { Resonance, Transients, Full };
enum class Listen { All, Delta, Transients, Body, Band1, Band2, Band3, Sub, Resonance, Afterglow };

inline constexpr const char* kStereoModeNames[] = {"Stereo", "Dual", "Mid/Side"};
inline constexpr const char* kCrossoverNames[] = {"Minimum phase", "Linear phase"};
inline constexpr const char* kHarmSourceNames[] = {"Both", "Transient", "Body"};
inline constexpr const char* kDriveCurveNames[] = {"Soft", "Tube", "Tape", "Diode", "Fold", "Rectify"};
inline constexpr const char* kResModeNames[] = {"Tuned", "Body", "Comb"};
inline constexpr const char* kChordNames[] = {"Unison", "Octaves", "Fifths", "Major", "Minor", "Sus", "Harmonic"};
inline constexpr const char* kMaterialNames[] = {"Wood", "Skin", "Metal", "Glass", "Tube"};
inline constexpr const char* kPitchSourceNames[] = {"Fixed", "Key", "MIDI", "Track"};
inline constexpr const char* kScaleNames[] = {"Major", "Minor", "Dorian", "Pentatonic", "Chromatic"};
inline constexpr const char* kResInputNames[] = {"Transients", "Full"};
inline constexpr const char* kGlowSourceNames[] = {"Resonance", "Transients", "Full"};
inline constexpr const char* kListenNames[] = {"All", "Delta", "Transients", "Body", "Band 1", "Band 2", "Band 3", "Sub", "Resonance", "Afterglow"};
inline constexpr int kOversampleFactors[] = {1, 2, 4, 8};
inline constexpr const char* kOversampleNames[] = {"1x", "2x", "4x", "8x"};
inline constexpr float kLookaheadMs[] = {0.f, 1.f, 2.f, 5.f};
inline constexpr const char* kLookaheadNames[] = {"Off", "1 ms", "2 ms", "5 ms"};

struct BandParams {
    // Shape.
    float attack = 0.f;              // -1..1
    float sustain = 0.f;             // -1..1
    float speed = 0.5f;              // 0..1: slow follower 5..50 ms
    float clip = 0.f;                // 0..1: transient clip, ceiling from +18 dB over the body down to 0 dB
    // Harmonics.
    HarmSource source = HarmSource::Both;
    std::array<float, kOrders> orders{1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};   // H1 (fundamental) .. H8, 0..1
    std::array<bool, kOrders> invert{};                                         // 180 degrees per order
    float harmonics = 0.f;           // 0..1, amount of the order bank
    DriveCurve curve = DriveCurve::Soft;
    float drive = 0.f;               // 0..1: 0..36 dB into the curve
    float bias = 0.f;                // -1..1: asymmetry (Tube, Rectify: half vs full)
    float tone = 0.5f;               // 0..1: tilt after the curve, 0.5 flat
    float driveMix = 1.f;            // 0..1
    bool mute = false;
};

struct Params {
    int bands = 3;                   // 1..3
    float xoverLow = 120.f, xoverHigh = 2500.f;   // Hz
    CrossoverType crossover = CrossoverType::MinPhase;
    StereoMode stereo = StereoMode::Stereo;
    float sensitivity = 0.5f;        // 0..1: knee of the transient mask
    int lookahead = 0;               // index into kLookaheadMs
    int oversample = 2;              // index into kOversampleFactors (4x)
    float punch = 0.f, tail = 0.f;   // -1..1, global attack and sustain added to every band
    std::array<BandParams, kMaxBands> band{};

    // Sub.
    float subLevel = 0.f;            // 0..1: one octave down
    float sub2Level = 0.f;           // 0..1: two octaves down
    float subPurity = 0.8f;          // 0 gritty square .. 1 clean
    float subRange = 150.f;          // Hz: upper edge of the band the divider listens to
    float phantom = 0.f;             // 0..1: psychoacoustic bass (H2-H4 of the sub band)
    float phantomCut = 0.f;          // 0..1: remove the fundamental under the phantom harmonics
    float monoBelow = 0.f;           // Hz, 0 = off

    // Resonance.
    ResMode resMode = ResMode::Tuned;
    ResInput resInput = ResInput::Transients;
    PitchSource pitch = PitchSource::Fixed;
    int root = 45;                   // MIDI note (A2)
    Chord chord = Chord::Octaves;
    Scale scale = Scale::Minor;
    Material material = Material::Skin;
    float size = 0.5f;               // 0..1: Body fundamental -2..+2 octaves around the root
    float spread = 0.f;              // 0..1: detune between resonators (0..30 cents)
    float resStereo = 0.5f;          // 0..1
    float resDecay = 0.6f;           // seconds (T60), 0.01..4
    float resDamping = 0.4f;         // 0..1
    float resTone = 0.5f;            // 0..1
    float strike = 0.6f;             // 0..1
    float resMix = 0.f;              // 0..1

    // Afterglow (reverb send).
    GlowSource glowSource = GlowSource::Resonance;
    float glowSend = 0.f;            // 0..1
    float glowSize = 1.f;            // 0.5..3
    float glowDecay = 2.f;           // seconds
    float glowDamp = 0.4f;           // 0..1
    float glowPre = 10.f;            // ms
    float glowMix = 0.5f;            // 0..1 return level

    // Output.
    bool autoGain = true;
    float mix = 1.f;                 // 0..1
    float outDb = 0.f;
    bool clipGuard = false;
    Listen listen = Listen::All;
};

// A MIDI note event for the resonator's pitch (MIDI pitch source). Offsets are ignored: notes apply per block.
struct NoteEvent { bool on = true; int note = 60; };

// The maximum harmonic order the order bank may use without aliasing into the audible band, for a band whose
// upper edge is `edgeHz`, at `fs` and oversampling factor `os` (PRD 4.4.4).
inline int maxCleanOrder(double fs, int os, double edgeHz) {
    const double top = (os - 0.5) * fs;              // components above this fold below fs/2
    const double edge = edgeHz < 0.45 * fs ? edgeHz : 0.45 * fs;
    int n = (int)(top / edge);
    return n < 1 ? 1 : n > kOrders ? kOrders : n;
}

}  // namespace muon
