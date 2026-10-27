#pragma once
// The WAVE page: voice to notes (wave/docs/PRD.md). A voice trace (the sung pitch over the last seconds as a piano
// roll, the played notes, the scale's rows, the input level) beside the Input, Notes, Chords and Expression controls.
#include "Panels.h"

namespace panels {

class VoiceTrace : public juce::Component {
public:
    explicit VoiceTrace(InfraProcessor& p);
    void paint(juce::Graphics&) override;
    void tick();                                 // 30 Hz: repaints when the trace or the settings changed
    juce::String status;                         // a line shown over the trace (key listening)
private:
    InfraProcessor& proc_;
    float centre_ = 57.f;                        // the middle row, following the voice
    int lastPos_ = -1;
    uint64_t lastSig_ = 0;
};

class WavePage : public Page {
public:
    explicit WavePage(InfraProcessor&);
    void resized() override;
    void tick() override;
private:
    void toggleListen();
    VoiceTrace trace_;
    juce::TextButton listen_;
    bool wasOn_ = false;
};

// Message thread, from the editor's timer: in the standalone app, switching WAVE on unmutes the app's audio input.
void waveInputCheck(InfraProcessor& p, bool& wasOn);

}  // namespace panels
