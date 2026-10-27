#pragma once
// Ignite: builds a new kit and its patterns from genre rules. It picks a model for each role (kick, snare, hats ...),
// designs each sound within ranges that suit the genre, writes four patterns (main, variation, break, fill) from
// the genre's rhythmic rules, and sets the mix, the effects and the macros' starting points. Never a plain random
// fill.
#include <juce_core/juce_core.h>

namespace ignite {

inline constexpr const char* kGenres[] = {"Deep House", "Tech House", "Techno", "Trance", "Drum & Bass", "Breakbeat", "UK Garage", "Experimental"};
constexpr int kNumGenres = 8;

struct Kit {
    juce::NamedValueSet params;     // parameter id -> value (choices by name)
    juce::var patterns, names;      // PatternStore's var format
    juce::String name, category;
};

// genre < 0: pick one at random.
Kit generate(juce::Random& rng, int genre = -1);

}  // namespace ignite
