#pragma once
// WAVE harmony: keys and scales, the note quantizer (with hysteresis), the chord builder and the key finder.
// Plain functions on small fixed arrays: nothing here allocates, so all of it can run on the audio thread.
#include <cstdint>

namespace wave {

enum class Scale : uint8_t {
    Chromatic, Major, NaturalMinor, HarmonicMinor, MelodicMinor, Dorian, Phrygian, Lydian, Mixolydian, Locrian,
    MajorPentatonic, MinorPentatonic, Blues, Count
};
inline constexpr const char* kScaleNames[] = {
    "Chromatic", "Major", "Natural minor", "Harmonic minor", "Melodic minor", "Dorian", "Phrygian", "Lydian",
    "Mixolydian", "Locrian", "Major pentatonic", "Minor pentatonic", "Blues"};
inline constexpr const char* kKeyNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

enum class ChordType : uint8_t { Triad, Seventh, Sus2, Sus4, Add9, Ninth, Power, Octaves, Count };
inline constexpr const char* kChordTypeNames[] = {"Triad", "Seventh", "Sus2", "Sus4", "Add9", "Ninth", "Power", "Octaves"};
enum class Role : uint8_t { Root, Top, Count };                 // the sung note is the chord's root, or its top voice
inline constexpr const char* kRoleNames[] = {"Root", "Top"};
enum class Voicing : uint8_t { Close, Open, Smooth, Count };
inline constexpr const char* kVoicingNames[] = {"Close", "Open", "Smooth"};

// The pitch classes of a scale, relative to its key, ascending.
struct ScaleTones { int n = 0; int pc[12] = {}; };
ScaleTones scaleTones(Scale s);
bool inScale(int note, int key, Scale s);
inline int mod12(int x) { return ((x % 12) + 12) % 12; }

// The scale tone nearest to pitch (fractional MIDI note). With current >= 0, the current note holds until the pitch
// passes the midpoint to a neighbouring scale tone by hysteresis x the gap to that tone (at most 0.45 semitones):
// 0.22 semitones between semitone neighbours, 0.44 across a whole tone, so vibrato between wide scale steps does not
// flip the note.
int quantize(float pitch, int key, Scale s, int current = -1, float hysteresis = 0.22f);

// Moves a scale tone by a number of scale steps (note must be in the scale).
int scaleStep(int note, int steps, int key, const ScaleTones& sc);

struct ChordSpec {
    ChordType type = ChordType::Triad;
    Role role = Role::Root;
    Voicing voicing = Voicing::Smooth;
    int bass = 0;                              // 0 off, 1 or 2 octaves below the chord root
    int key = 0;
    Scale scale = Scale::Major;
};
struct Chord {
    static constexpr int kMax = 8;
    int n = 0;
    int notes[kMax] = {};                      // ascending, MIDI 0..127 (bass included)
    int root = -1;                             // the chord root (a MIDI note within the chord's register)
    int mask = 0;                              // pitch classes relative to the root (bit 0 = root), for the name
    int bassN = 0;                             // 1 when notes[0] is the added bass note
};
// Builds the chord for a sung note (a MIDI note, already quantized). prev is the previous chord (for Smooth voicing),
// or nullptr.
Chord buildChord(int sung, const ChordSpec& spec, const Chord* prev);

// Chord suffix for a pitch-class mask relative to the root ("", "m", "7", "maj7", "sus4" ...); "*" if unknown.
const char* chordSuffix(int mask);

// Key finding: correlation of a pitch-class duration histogram with the Krumhansl-Kessler major and minor profiles.
struct KeyGuess { int key = 0; bool minor = false; float score = 0.f; };
KeyGuess findKey(const float histogram[12]);

}  // namespace wave
