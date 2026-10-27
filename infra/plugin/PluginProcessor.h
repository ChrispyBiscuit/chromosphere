#pragma once
#include "Presets.h"
#include "Updater.h"
#include "infra/synth.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

// Chromosphere | INFRA. Parameters live in the APVTS; once per block they are copied into an infra::SynthParams
// and handed to the engine with the block's MIDI.
class InfraProcessor : public juce::AudioProcessor {
public:
    InfraProcessor();
    ~InfraProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Chromosphere | INFRA"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    // The engine's view of the current parameter values (reads the parameters' atomics).
    infra::SynthParams currentParams() const;

    // Keyboard on the editor (and tests) inject MIDI here; merged into the next block.
    juce::MidiKeyboardState keyboardState;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets;
    Updater updater;                              // message thread; shared settings across instances
    infra::Synth synth;

    // Host tempo as last seen (for the editor's status line).
    std::atomic<double> hostBpm{120.0};
    std::atomic<bool> hostPlaying{false};

private:
    struct Raw;                                   // cached parameter pointers
    std::unique_ptr<Raw> raw_;
    std::vector<infra::MidiEvent> events_;
    juce::MidiBuffer keyboardMidi_;
    double internalPpq_ = 0.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InfraProcessor)
};
