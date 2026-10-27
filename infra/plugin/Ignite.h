#pragma once
// Ignite: builds a new patch from musical rules. It picks an archetype (bass, lead, pad ...), then chooses tables,
// filter, envelopes, modulation and effects within ranges that suit it, and always wires the four macros, the mod
// wheel and aftertouch the way the factory presets do. Never a plain random fill.
#include <juce_core/juce_core.h>

namespace ignite {

inline constexpr const char* kArchetypes[] = {"Bass", "Lead", "Pad", "Pluck", "Keys", "Arp", "Drone", "FX"};
constexpr int kNumArchetypes = 8;

struct Patch {
    juce::NamedValueSet params;     // parameter id -> value (choices by name)
    juce::String name, category;
};

// archetype < 0: pick one at random.
Patch generate(juce::Random& rng, int archetype = -1);

}  // namespace ignite
