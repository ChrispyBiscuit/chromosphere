#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>

class UltraProcessor;

struct Preset {
    juce::String name, category, description, author;
    juce::StringArray tags;
    juce::NamedValueSet params;              // only the parameters that differ from the defaults
    juce::var patterns, names;               // pattern bank and track names (PatternStore's var format)
    bool factory = true;
    juce::File file;                         // user presets only
};

// Factory presets (embedded JSON), user presets (one XML file each), apply / save, A/B compare, and loading a
// generated kit (Ignite). From INFRA, extended with patterns and track names. Everything here runs on the
// message thread.
class PresetManager {
public:
    explicit PresetManager(UltraProcessor& p);

    void reload();                                         // re-scan the user preset folder
    const std::vector<Preset>& presets() const { return presets_; }
    int indexOf(const juce::String& name) const;
    bool apply(int index);                                 // resets every parameter to its default, then sets the preset's
    void applyGenerated(const juce::NamedValueSet& values, const juce::var& patterns, const juce::var& names,
                        const juce::String& name, const juce::String& category);
    bool saveUser(const juce::String& name, const juce::String& category, const juce::StringArray& tags);
    bool deleteUser(int index);
    juce::File userFolder() const;

    int currentIndex() const { return current_; }
    juce::String currentName() const;
    juce::String currentCategory() const;
    juce::String currentDescription() const;
    void setCurrentName(const juce::String& n);

    int activeSlot() const { return slot_; }
    void toggleAB();
    void copyToOtherSlot();

    // "Keep patterns": loading a preset changes the kit (sounds, mixer, effects) but leaves the patterns,
    // the pattern selection, swing and tempo as they are.
    bool keepPatterns = false;
    static bool isGrooveParam(const juce::String& id);

private:
    void setParameters(const juce::NamedValueSet& values);
    static void applyOne(juce::RangedAudioParameter& prm, const juce::var& v);

    UltraProcessor& proc_;
    std::vector<Preset> presets_;
    int current_ = -1, slot_ = 0;
    juce::String generatedName_, generatedCategory_;      // shown when the kit came from Ignite
    std::array<juce::ValueTree, 2> ab_;
};
