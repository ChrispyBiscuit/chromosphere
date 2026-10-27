#pragma once
#include "Presets.h"
#include "Updater.h"
#include "muon/engine.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>

// The four macros (PRD 4.8): each drives up to kMaxTargets parameters by an amount in normalised units, so the
// same macro can mean "a little more" on one preset and "much more" on another. Assignments are part of the
// preset and the saved state.
struct MacroTarget { int macro = 0; juce::String paramId; float amount = 0.f; };

// Chromosphere | MUON. Parameters live in the APVTS; once per block their values (plus the macro offsets) are
// copied into a muon::Params and handed to the engine.
class MuonProcessor : public juce::AudioProcessor {
public:
    static constexpr int kNumMacros = 4, kMaxTargets = 32;
    static const char* const kMacroNames[kNumMacros];      // Velocity, Mass, Charge, Decay
    static const char* const kMacroMeanings[kNumMacros];

    MuonProcessor();
    ~MuonProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Chromosphere | MUON"; }
    bool acceptsMidi() const override { return true; }          // resonator pitch only
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    // The engine's view of the current parameter values, macro offsets included.
    muon::Params currentParams();

    // Macro assignments (message thread).
    const std::vector<MacroTarget>& macroTargets() const { return targets_; }
    void setMacroTargets(std::vector<MacroTarget> t);
    static juce::var macrosToVar(const std::vector<MacroTarget>& t);
    static std::vector<MacroTarget> macrosFromVar(const juce::var& v);

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets;
    Updater updater;                              // message thread; shared settings across instances
    muon::Engine engine;
    std::atomic<int> latencyShown{0};

private:
    struct Slot { std::atomic<float>* value = nullptr; juce::RangedAudioParameter* param = nullptr; };
    std::vector<Slot> slots_;                     // in layout order
    int idx(const juce::String& id) const;
    // Macro table handed to the audio thread through a sequence lock (no allocation, no locks).
    struct Table { int n = 0; int macro[kMaxTargets] = {}; int slot[kMaxTargets] = {}; float amt[kMaxTargets] = {}; };
    Table shared_, local_;
    std::atomic<unsigned> seq_{0};
    std::vector<MacroTarget> targets_;
    std::vector<muon::NoteEvent> notes_;
    std::vector<float> values_;
    int macroSlot_[kNumMacros] = {};
    // Slot indices of everything currentParams() reads.
    struct Ids;
    std::unique_ptr<Ids> ids_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MuonProcessor)
};
