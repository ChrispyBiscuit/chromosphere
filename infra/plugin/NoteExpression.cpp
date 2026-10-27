#include "NoteExpression.h"
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow", "-Wfloat-equal", "-Wnon-virtual-dtor")
JUCE_BEGIN_IGNORE_WARNINGS_MSVC(4996)
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstnoteexpression.h>
JUCE_END_IGNORE_WARNINGS_MSVC
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace {
// Interface IDs written out here (the SDK's FUID statics are only defined inside the VST3 wrapper's translation unit,
// which the other plug-in formats and the tests do not link).
const TUID kFUnknownIid = INLINE_UID(0x00000000, 0x00000000, 0xC0000000, 0x00000046);
const TUID kNoteExpressionControllerIid = INLINE_UID(0xB7F8F859, 0x41234872, 0x91169581, 0x4F3721A3);

bool sameIid(const TUID a, const TUID b) { return std::memcmp(a, b, sizeof(TUID)) == 0; }

void copyTitle(String128 dst, const char* src) {
    int i = 0;
    for (; src[i] != 0 && i < 127; ++i) dst[i] = (TChar)src[i];
    dst[i] = 0;
}
}  // namespace

// The controller side: tells the host that INFRA takes a Tuning expression on every channel of its event input.
struct NoteExpression::Controller : public INoteExpressionController {
    tresult PLUGIN_API queryInterface(const TUID wanted, void** obj) override {
        if (sameIid(wanted, kFUnknownIid) || sameIid(wanted, kNoteExpressionControllerIid)) { *obj = this; addRef(); return kResultOk; }
        *obj = nullptr;
        return kNoInterface;
    }
    // Owned by the NoteExpression object, which lives as long as the processor and so outlasts the edit controller.
    uint32 PLUGIN_API addRef() override { return 1; }
    uint32 PLUGIN_API release() override { return 1; }

    int32 PLUGIN_API getNoteExpressionCount(int32 busIndex, int16) override { return busIndex == 0 ? 1 : 0; }

    tresult PLUGIN_API getNoteExpressionInfo(int32 busIndex, int16, int32 index, NoteExpressionTypeInfo& info) override {
        if (busIndex != 0 || index != 0) return kInvalidArgument;
        std::memset(&info, 0, sizeof(info));
        info.typeId = kTuningTypeID;
        copyTitle(info.title, "Tuning");
        copyTitle(info.shortTitle, "Tune");
        copyTitle(info.units, "st");
        info.unitId = -1;
        info.valueDesc.minimum = 0.0;
        info.valueDesc.maximum = 1.0;
        info.valueDesc.defaultValue = 0.5;
        info.valueDesc.stepCount = 0;
        info.associatedParameterId = kNoParamId;
        info.flags = NoteExpressionTypeInfo::kIsBipolar;
        return kResultTrue;
    }

    tresult PLUGIN_API getNoteExpressionStringByValue(int32, int16, NoteExpressionTypeID id, NoteExpressionValue v, String128 out) override {
        if (id != kTuningTypeID) return kInvalidArgument;
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%+.2f st", (double)NoteExpression::toSemitones(v));
        copyTitle(out, buf);
        return kResultTrue;
    }

    tresult PLUGIN_API getNoteExpressionValueByString(int32, int16, NoteExpressionTypeID id, const TChar* s, NoteExpressionValue& v) override {
        if (id != kTuningTypeID || s == nullptr) return kInvalidArgument;
        char buf[64]; int i = 0;
        for (; s[i] != 0 && i < 63; ++i) buf[i] = (char)s[i];
        buf[i] = 0;
        v = juce::jlimit(0.0, 1.0, std::atof(buf) / 240.0 + 0.5);
        return kResultTrue;
    }
};

NoteExpression::NoteExpression() : controller_(std::make_unique<Controller>()) {}
NoteExpression::~NoteExpression() = default;

int32_t NoteExpression::queryIEditController(const TUID iid, void** obj) {
    if (sameIid(iid, kNoteExpressionControllerIid)) return controller_->queryInterface(iid, obj);
    *obj = nullptr;
    return kNoInterface;
}

void NoteExpression::add(int offset, int note, float semitones) {
    if (numChanges_ < (int)changes_.size()) changes_[(size_t)numChanges_++] = {offset, note, semitones};
}

int NoteExpression::pitchFor(int32_t noteId) const {
    for (int k = 1; k <= (int)ids_.size(); ++k) {          // newest first: a host may reuse an ID
        const auto& e = ids_[(size_t)((nextId_ - k + (int)ids_.size()) % (int)ids_.size())];
        if (e.id == noteId) return e.pitch;
    }
    return -1;
}

void NoteExpression::processInputEvents(IEventList& events) {
    numChanges_ = 0;
    const int32 count = events.getEventCount();
    for (int32 i = 0; i < count; ++i) {
        Event e{};
        if (events.getEvent(i, e) != kResultOk) continue;
        if (e.type == Event::kNoteOnEvent) {
            if (e.noteOn.noteId != -1) { ids_[(size_t)nextId_] = {e.noteOn.noteId, e.noteOn.pitch}; nextId_ = (nextId_ + 1) % (int)ids_.size(); }
            if (std::abs(e.noteOn.tuning) > 1e-4f) add(e.sampleOffset, e.noteOn.pitch, e.noteOn.tuning * 0.01f);   // cents
        } else if (e.type == Event::kNoteExpressionValueEvent && e.noteExpressionValue.typeId == kTuningTypeID) {
            const int pitch = pitchFor(e.noteExpressionValue.noteId);
            if (pitch >= 0) add(e.sampleOffset, pitch, toSemitones(e.noteExpressionValue.value));
        }
    }
}
