#pragma once
// VST3 note expression for INFRA: per-note tuning, the way hosts such as FL Studio send slide notes and per-note pitch
// bends to VST3 instruments. The edit controller advertises a Tuning expression (INoteExpressionController, through
// VST3ClientExtensions::queryIEditController); the host's raw input events arrive through
// VST3ClientExtensions::processInputEvents (a small JUCE patch, cmake/juce-vst3-input-events.patch: stock JUCE drops
// note expression events before processBlock). Tuning changes are collected per block and read by processBlock.
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <memory>

class NoteExpression : public juce::VST3ClientExtensions {
public:
    struct Change { int offset; int note; float semitones; };

    NoteExpression();
    ~NoteExpression() override;

    int32_t queryIEditController(const Steinberg::TUID iid, void** obj) override;
    void processInputEvents(Steinberg::Vst::IEventList& events) override;   // audio thread

    // Audio thread, inside the same process call: the tuning changes of this block, in event order.
    const Change* changes() const { return changes_.data(); }
    int numChanges() const { return numChanges_; }
    void clear() { numChanges_ = 0; }

    // VST3 tuning expression: normalised 0..1 covers -120..+120 semitones, 0.5 is in tune.
    static float toSemitones(double normalised) { return (float)((normalised - 0.5) * 240.0); }

private:
    void add(int offset, int note, float semitones);
    int pitchFor(int32_t noteId) const;

    struct Controller;
    std::unique_ptr<Controller> controller_;
    std::array<Change, 1024> changes_{};
    int numChanges_ = 0;
    // Note IDs of recent note-ons and their keys: expression events name the note by ID only.
    struct IdPitch { int32_t id = -1; int pitch = -1; };
    std::array<IdPitch, 256> ids_{};
    int nextId_ = 0;
};
