#pragma once
#include "NoteExpression.h"
#include "Presets.h"
#include "Updater.h"
#include "infra/synth.h"
#include "wave/engine.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

// Chromosphere | INFRA. Parameters live in the APVTS; once per block they are copied into an infra::SynthParams
// and handed to the engine with the block's MIDI. WAVE (wave/docs/PRD.md) reads the input bus, turns the voice into
// notes and merges them with the host's MIDI; with WAVE MIDI Out on, the same notes go to the MIDI output.
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
    juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &noteExpression_; }
    bool producesMidi() const override { return true; }      // WAVE's notes, when WAVE MIDI Out is on
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
    wave::Params currentWaveParams() const;
    // WAVE settings (ids "wv...") are a performance setup, not part of a sound: presets, Ignite and A/B leave them.
    static bool isPerformanceParameter(const juce::String& id) { return id.startsWith("wv"); }

    // Keyboard on the editor (and tests) inject MIDI here; merged into the next block.
    juce::MidiKeyboardState keyboardState;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets;
    juce::SharedResourcePointer<Updater> updater; // one per process, shared by every instance (message thread)
    infra::Synth synth;
    wave::Engine wave;                            // the voice-to-notes engine (the editor reads its trace)

    // Host tempo as last seen (for the editor's status line).
    std::atomic<double> hostBpm{120.0};
    std::atomic<bool> hostPlaying{false};

private:
    struct Raw;                                   // cached parameter pointers
    std::unique_ptr<Raw> raw_;
    std::vector<infra::MidiEvent> events_;
    NoteExpression noteExpression_;               // VST3 per-note tuning (see NoteExpression.h)
    juce::MidiBuffer keyboardMidi_;
    double internalPpq_ = 0.0;
    // WAVE (audio thread; sized in prepareToPlay).
    std::vector<float> waveIn_;                   // the input's mono sum, kept for Monitor after the synth overwrites the buffer
    std::vector<wave::Event> waveEvents_;
    std::vector<infra::MidiEvent> merged_;
    bool waveWasOn_ = false;
    float monitorGain_ = 0.f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InfraProcessor)
};
