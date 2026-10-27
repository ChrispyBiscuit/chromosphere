#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>

class MuonProcessor;
struct MacroTarget;
struct UpdaterProduct;

// The settings the shared updater needs for MUON (repository, release tag, names).
UpdaterProduct muonUpdaterProduct();

struct Preset {
    juce::String name, category, description, author;
    juce::StringArray tags;
    juce::NamedValueSet params;              // only the parameters that differ from the defaults
    juce::var macros;                        // [{macro, id, amount}, ...]
    bool factory = true;
    juce::File file;                         // user presets only
};

// Factory presets (embedded JSON), user presets (one XML file each), apply / save, A/B compare, and loading a
// generated setting (Ignite). Message thread only.
class PresetManager {
public:
    explicit PresetManager(MuonProcessor& p);

    void reload();
    const std::vector<Preset>& presets() const { return presets_; }
    int indexOf(const juce::String& name) const;
    bool apply(int index);                                 // resets every parameter to its default, then sets the preset's
    void applyGenerated(const juce::NamedValueSet& values, const juce::var& macros, const juce::String& name, const juce::String& category);
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

private:
    void setParameters(const juce::NamedValueSet& values);
    static void applyOne(juce::RangedAudioParameter& prm, const juce::var& v);
    struct Slot { juce::ValueTree state; juce::var macros; };
    Slot capture() const;

    MuonProcessor& proc_;
    std::vector<Preset> presets_;
    int current_ = -1, slot_ = 0;
    juce::String generatedName_, generatedCategory_;
    std::array<Slot, 2> ab_;
};
