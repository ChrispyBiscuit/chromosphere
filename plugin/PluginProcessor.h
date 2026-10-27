#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "EngineHost.h"
#include <array>
#include <mutex>
#include "Presets.h"
#include "Updater.h"

class ChromosphereProcessor : public juce::AudioProcessor {
public:
    ChromosphereProcessor();
    ~ChromosphereProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Chromosphere"; }
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

    // Impulse responses (Convolution and Hybrid modes). Safe from any non-audio thread.
    // Returns an empty string on success, otherwise the reason it failed.
    juce::String loadImpulseResponse(const juce::File& file);
    // Factory impulse responses are embedded; ids are the file names without extension.
    juce::String loadFactoryImpulseResponse(const juce::String& id);
    static juce::StringArray factoryImpulseResponses();
    void clearImpulseResponse();
    juce::String getImpulseResponsePath() const;

    static constexpr int kStateVersion = 4;   // 4: Acoustic engine and room parameters
    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets{*this};
    juce::SharedResourcePointer<Updater> updater;   // one per process; message thread only

    // ---- Data for the editor's displays. Message thread unless noted. ----
    struct IrInfo {
        std::vector<float> preview;           // |IR| envelope of the first 500 ms in 5 ms steps, dB relative to the peak
        double t60[3] = {1, 1, 1};            // T30 fits at 125 Hz, 1 kHz, 8 kHz
        double seconds = 0;
    };
    std::shared_ptr<const IrInfo> getIrInfo() const;
    // Decay time at f (Hz) as the current settings would give it.
    double displayedT60(double f) const;
    cs::RoomSpec roomSpec() const;            // the Acoustic room as set (before the size scale)
    cs::RoomAcoustics roomAcoustics() const;  // derived acoustics of the room in use (size applied)
    // Audio thread writes, UI reads (lock-free).
    std::atomic<float> inPeak{0.f}, outPeak{0.f}, tailPeak{0.f}, bassPeak{0.f};   // bassPeak: wet signal in the 15–200 Hz band
    static constexpr int kEnvSize = 256;
    std::array<std::atomic<float>, kEnvSize> envRing;      // wet output level in dB, one entry per ~21 ms
    std::atomic<int> envPos{0};

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    cs::EngineSettings readSettings() const;     // thread-safe: reads atomics only

    std::atomic<float> *size_, *low_, *mid_, *high_, *pre_, *modDepth_, *modRate_,
        *er_, *tail_, *mix_, *out_, *quality_, *diffusion_, *freeze_, *mode_, *cosmicA_, *cosmicB_, *cosmicC_, *erMs_, *tailScale_,
        *irStart_, *irLength_, *irStretch_, *irReverse_, *irLowCut_, *irHighCut_, *irWidth_, *irDecay_,
        *roomL_, *roomW_, *roomH_, *roomDist_, *roomOffset_, *roomSpread_, *roomWalls_, *roomFloor_, *roomCeil_, *roomMic_, *roomDirect_,
        *roomHum_, *roomTemp_, *roomFurnish_;
    mutable std::mutex roomCacheMutex_;
    mutable cs::RoomAcoustics roomCache_; mutable cs::RoomSpec roomCacheSpec_;
    mutable double roomCacheSize_ = -1; mutable bool roomCacheScatter_ = true, roomCacheValid_ = false;
    mutable std::mutex irMutex_;
    std::shared_ptr<const IrInfo> irInfo_;      // guarded by irMutex_
    juce::String loadImpulseResponseFromStream(std::unique_ptr<juce::InputStream> s, const juce::String& label, const juce::String& storedPath);
    int envCount_ = 0; float envAcc_ = 0.f;     // audio thread
    float bassL_ = 0.f, bassR_ = 0.f, dcL_ = 0.f, dcR_ = 0.f;   // 15–200 Hz band state for the bass meter
    std::shared_ptr<const cs::Ir> ir_;          // guarded by irMutex_
    juce::String irPath_;                       // guarded by irMutex_
    cs::EngineHost host_;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dryGain_, wetGain_, outGain_;
    std::vector<float> inL_, inR_, wetL_, wetR_;
    static constexpr int kChunk = 512;
    static constexpr float kSleepLevel = 1e-6f;          // -120 dBFS
    juce::int64 silentSamples_ = 0; float lastWetPeak_ = 1.f; bool sleeping_ = false;
public:
    bool isSleeping() const { return sleeping_; }       // tests
private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChromosphereProcessor)
};
