#pragma once
#include "PatternStore.h"
#include "Presets.h"
#include "Updater.h"
#include "ultra/machine.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

// Chromosphere | ULTRA. Parameters live in the APVTS and patterns in its value tree; once per block the
// parameters are copied into an ultra::KitParams and the newest pattern bank is taken from the triple buffer.
class UltraProcessor : public juce::AudioProcessor {
public:
    UltraProcessor();
    ~UltraProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Chromosphere | ULTRA"; }
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
    static juce::String tid(int track, const char* name) { return "t" + juce::String(track + 1) + name; }
    // The engine's view of the current parameter values (reads the parameters' atomics).
    ultra::KitParams currentParams() const;

    // Pads on the editor: a hit for a track, picked up at the start of the next block.
    void playPad(int track, float velocity);
    // The editor's Play button (internal clock while the host is stopped).
    void setPlaying(bool on) { machine.internalPlay.store(on); }
    bool isPlaying() const { return machine.internalPlay.load(); }

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;
    PatternStore patterns;
    PresetManager presets;
    juce::SharedResourcePointer<Updater> updater; // one per process, shared by every instance (EXO's guide, 4)
    ultra::DrumMachine machine;

    std::atomic<double> hostBpm{120.0};
    std::atomic<bool> hostPlaying{false};

private:
    static BusesProperties makeBuses();         // main stereo out plus 8 optional stereo aux outs
    struct Raw;                                   // cached parameter pointers
    std::unique_ptr<Raw> raw_;
    std::vector<ultra::MidiEvent> events_;
    std::array<std::atomic<float>, ultra::kTracks> pads_{};
    juce::AudioBuffer<float> monoScratch_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UltraProcessor)
};
