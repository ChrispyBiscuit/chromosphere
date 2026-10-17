#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>

class FancyReverbProcessor;

struct Preset {
    juce::String name, category, ir;         // ir: "" (keep the loaded one) or a factory IR id
    juce::StringArray tags;
    juce::NamedValueSet params;              // only the parameters that differ from the defaults
    bool factory = true;
    juce::File file;                         // user presets only
};

// Factory presets (embedded JSON), user presets (one XML file each), apply/save, undo and A/B compare.
// Everything here runs on the message thread.
class PresetManager {
public:
    explicit PresetManager(FancyReverbProcessor& p);

    void reload();                                         // re-scan the user preset folder
    const std::vector<Preset>& presets() const { return presets_; }
    int indexOf(const juce::String& name) const;
    bool apply(int index);                                 // resets every parameter to its default, then sets the preset's
    bool saveUser(const juce::String& name, const juce::String& category, const juce::StringArray& tags);
    bool deleteUser(int index);
    juce::File userFolder() const;

    int currentIndex() const { return current_; }
    juce::String currentName() const { return current_ >= 0 && current_ < (int)presets_.size() ? presets_[(size_t)current_].name : juce::String(); }
    void setCurrentName(const juce::String& n) { current_ = indexOf(n); }

    // A/B compare: two parameter snapshots, toggled.
    int activeSlot() const { return slot_; }
    void toggleAB();
    void copyToOtherSlot();

    juce::UndoManager& undo();

private:
    void setParameters(const juce::NamedValueSet& values);
    void applyOne(juce::RangedAudioParameter& prm, const juce::var& v);

    FancyReverbProcessor& proc_;
    std::vector<Preset> presets_;
    int current_ = -1, slot_ = 0;
    std::array<juce::ValueTree, 2> ab_;
};
