#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "EngineHost.h"

class FancyReverbProcessor : public juce::AudioProcessor {
public:
    FancyReverbProcessor();
    ~FancyReverbProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FancyReverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override;
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static constexpr int kStateVersion = 1;
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    fr::EngineSettings readSettings() const;     // thread-safe: reads atomics only

    std::atomic<float> *size_, *low_, *mid_, *high_, *pre_, *modDepth_, *modRate_,
        *er_, *tail_, *mix_, *out_, *quality_, *diffusion_;
    fr::EngineHost host_;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dryGain_, wetGain_, outGain_;
    std::vector<float> inL_, inR_, wetL_, wetR_;
    static constexpr int kChunk = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FancyReverbProcessor)
};
