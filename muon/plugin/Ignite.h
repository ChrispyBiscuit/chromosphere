#pragma once
// Ignite for MUON (PRD 6): builds a setting from rules for a chosen source, never a plain random fill. Each source
// has its own band layout, shaping direction, harmonic recipe and resonance, varied within ranges that suit it,
// and the four macros are assigned with their fixed meanings.
#include <juce_core/juce_core.h>

namespace ignite {

inline constexpr int kNumSources = 6;
inline constexpr const char* kSources[kNumSources] = {"Kick", "Snare", "Bass", "Vocal", "Bus", "Weird"};

struct Setting {
    juce::NamedValueSet params;
    juce::var macros;
    juce::String name, category;
};

// source: 0 .. kNumSources - 1, or -1 for any.
Setting generate(juce::Random& rng, int source);

}  // namespace ignite
