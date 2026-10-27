#include "synth.h"
#include "common/denormal.h"
#include <climits>

namespace infra {

Synth::Synth() = default;
Synth::~Synth() { release(); }

void Synth::prepare(double fs, int maxBlock) {
    fs_ = fs; maxBlock_ = std::max(1, maxBlock);
    WavetableBank::get();
    for (size_t i = 0; i < voices_.size(); ++i) voices_[i].prepare(fs, 1234u + 7919u * (uint32_t)i);
    drive_.prepare(fs); chorus_.prepare(fs); echo_.prepare(fs); limiter_.prepare(fs);
    reverb_.setShape(p_.fx.reverbSize, p_.fx.reverbDecay, p_.fx.reverbDamp, p_.fx.reverbPreDelay);
    reverb_.prepare(fs, maxBlock_);
    rng_.seed(0xC0FFEEu);
    reset();
}

void Synth::release() { reverb_.release(); }

void Synth::reset() {
    for (auto& v : voices_) v.kill();
    arp_.reset(); arpWasOn_ = false;
    keyDown_.fill(false); sustained_.fill(false); sustainPedal_ = false;
    stackN_ = 0; lastPitch_ = -1.f; bend_ = 0.f;
    drive_.reset(); chorus_.reset(); echo_.reset();
    controlPhase_ = 0;
    for (auto& l : globalLfo_) l.reset(0.f, rng_);
}

// ---- Key layer --------------------------------------------------------------------------------------
void Synth::handleMidi(const MidiEvent& e, int64_t now) {
    switch (e.type) {
        case MidiEvent::NoteOn: {
            const int n = std::clamp(e.number, 0, 127);
            if (e.value <= 0.f) { MidiEvent off = e; off.type = MidiEvent::NoteOff; handleMidi(off, now); break; }
            if (sustained_[(size_t)n]) { sustained_[(size_t)n] = false; keyOff(n, now); }   // re-strike under the pedal
            keyDown_[(size_t)n] = true; keyVel_[(size_t)n] = e.value;
            keyOn(n, e.value, now);
            break;
        }
        case MidiEvent::NoteOff: {
            const int n = std::clamp(e.number, 0, 127);
            if (!keyDown_[(size_t)n]) break;
            keyDown_[(size_t)n] = false;
            if (sustainPedal_) sustained_[(size_t)n] = true;
            else keyOff(n, now);
            break;
        }
        case MidiEvent::Controller:
            if (e.number == 1) modWheel_ = std::clamp(e.value, 0.f, 1.f);
            else if (e.number == 64) {
                const bool down = e.value >= 0.5f;
                if (sustainPedal_ && !down)
                    for (int n = 0; n < 128; ++n)
                        if (sustained_[(size_t)n]) { sustained_[(size_t)n] = false; keyOff(n, now); }
                sustainPedal_ = down;
            } else if (e.number == 120 || e.number == 123) {
                MidiEvent a; a.type = MidiEvent::AllNotesOff; a.number = e.number; handleMidi(a, now);
            }
            break;
        case MidiEvent::PitchBend: bend_ = std::clamp(e.value, -1.f, 1.f); break;
        case MidiEvent::ChannelPressure: aftertouch_ = std::clamp(e.value, 0.f, 1.f); break;
        case MidiEvent::AllNotesOff:
            keyDown_.fill(false); sustained_.fill(false);
            arp_.stop(now); arp_.clearKeys();
            allVoicesOff(e.number == 120);
            break;
    }
}

void Synth::keyOn(int note, float vel, int64_t now) {
    if (p_.arp.on) arp_.keyOn(note, vel, now);
    else voiceOn(note, vel, directId(note), 1.f);
}

void Synth::keyOff(int note, int64_t now) {
    if (p_.arp.on) arp_.keyOff(note, now);
    else voiceOff(directId(note));
}

// ---- Voices -----------------------------------------------------------------------------------------
void Synth::voiceOn(int note, float vel, uint32_t id, float stepValue) {
    ctx_.p = &p_;
    const VoiceMode mode = p_.voiceMode;
    if (mode == VoiceMode::Poly) {
        bool anyGated = false;
        for (auto& v : voices_) anyGated |= v.active() && v.gated();
        const float glideFrom = (p_.glide > 0.0005f && lastPitch_ >= 0.f && (!p_.glideLegatoOnly || anyGated)) ? lastPitch_ : -1.f;
        const int limit = std::clamp(p_.polyphony, 1, kMaxVoices);
        int active = 0;
        for (auto& v : voices_) active += v.active() ? 1 : 0;
        Voice* pick = nullptr;
        if (active < limit) {
            for (auto& v : voices_) if (!v.active()) { pick = &v; break; }
        }
        if (!pick) {
            // Steal: the quietest released voice, otherwise the oldest held one.
            float best = 1e9f;
            for (auto& v : voices_) if (v.active() && !v.gated() && v.level() < best) { best = v.level(); pick = &v; }
            if (!pick) {
                uint64_t oldest = UINT64_MAX;
                for (auto& v : voices_) if (v.active() && v.age() < oldest) { oldest = v.age(); pick = &v; }
            }
        }
        if (!pick) pick = &voices_[0];
        pick->start(ctx_, note, vel, id, stepValue, glideFrom, true);
        pick->setAge(++ageCounter_);
    } else {
        // Mono and Legato share voice 0 and a last-note-priority stack.
        for (int i = 0; i < stackN_; ++i)
            if (stack_[(size_t)i].id == id) { for (int j = i; j + 1 < stackN_; ++j) stack_[(size_t)j] = stack_[(size_t)j + 1]; --stackN_; break; }
        if (stackN_ >= (int)stack_.size()) { for (int j = 0; j + 1 < stackN_; ++j) stack_[(size_t)j] = stack_[(size_t)j + 1]; --stackN_; }
        stack_[(size_t)stackN_++] = {note, id, vel, stepValue};
        Voice& v = voices_[0];
        const bool sounding = v.active() && v.gated();
        if (mode == VoiceMode::Legato && sounding) {
            v.legatoTo(note, id, vel, p_.glide > 0.0005f ? 0.f : -1.f);
        } else {
            const float glideFrom = (p_.glide > 0.0005f && lastPitch_ >= 0.f && (!p_.glideLegatoOnly || sounding)) ? lastPitch_ : -1.f;
            v.start(ctx_, note, vel, id, stepValue, glideFrom, true);
            v.setAge(++ageCounter_);
        }
    }
    lastPitch_ = (float)note;
}

void Synth::voiceOff(uint32_t id) {
    if (p_.voiceMode != VoiceMode::Poly) {
        int idx = -1;
        for (int i = 0; i < stackN_; ++i) if (stack_[(size_t)i].id == id) { idx = i; break; }
        if (idx >= 0) {
            const bool wasTop = idx == stackN_ - 1;
            for (int j = idx; j + 1 < stackN_; ++j) stack_[(size_t)j] = stack_[(size_t)j + 1];
            --stackN_;
            Voice& v = voices_[0];
            if (stackN_ == 0) { if (v.id() == id) v.release(); }
            else if (wasTop && v.id() == id) {
                const auto& top = stack_[(size_t)stackN_ - 1];
                v.legatoTo(top.note, top.id, top.vel, p_.glide > 0.0005f ? 0.f : -1.f);
                lastPitch_ = (float)top.note;
            }
        }
    }
    // Poly voices (and voices left over from a mode switch).
    for (auto& v : voices_) if (v.active() && v.gated() && v.id() == id) v.release();
}

void Synth::allVoicesOff(bool hard) {
    for (auto& v : voices_) { if (hard) v.kill(); else v.release(); }
    stackN_ = 0;
}

void Synth::controlAll() {
    for (auto& v : voices_) if (v.active()) v.control(ctx_);
}

void Synth::renderVoices(float* L, float* R, int n) {
    for (auto& v : voices_) if (v.active()) v.render(L, R, n);
}

// ---- Global modulation (effects) ------------------------------------------------------------------------
void Synth::computeGlobal(int n, FxState& fx) {
    float src[kNumModSources] = {};
    for (int i = 0; i < 2; ++i) {
        const auto& lp = p_.lfo[(size_t)i];
        const double rate = lp.sync ? ctx_.bpm / 60.0 / kDivisionBeats[std::clamp(lp.division, 0, kNumDivisions - 1)] : (double)lp.rateHz;
        src[(int)ModSource::Lfo1 + i] = globalLfo_[i].advance(lp.shape, (float)(rate * n / fs_), rng_);
        ctx_.globalLfoPhase[i] = globalLfo_[i].phase();
    }
    src[(int)ModSource::ModWheel] = modWheel_;
    src[(int)ModSource::Aftertouch] = aftertouch_;
    src[(int)ModSource::PitchBend] = bend_;
    for (int m = 0; m < kNumMacros; ++m) src[(int)ModSource::Macro1 + m] = p_.macro[(size_t)m];
    float d[kNumModDests] = {};
    for (const auto& s : p_.mod) {
        const int di = (int)s.dst;
        if (s.src == ModSource::None || di < kFirstGlobalDest || di >= kNumModDests) continue;
        d[di] += s.amount * src[(int)s.src];
    }
    const auto& f = p_.fx;
    fx.driveMode = f.driveMode;
    fx.drive = std::clamp(f.drive + d[(int)ModDest::FxDrive], 0.f, 1.f);
    fx.driveTone = f.driveTone; fx.driveMix = f.driveMix;
    fx.chorusRate = f.chorusRate; fx.chorusDepth = f.chorusDepth;
    fx.chorusMix = std::clamp(f.chorusMix + d[(int)ModDest::FxChorus], 0.f, 1.f);
    const double secs = f.delaySync ? kDivisionBeats[std::clamp(f.delayDivision, 0, kNumDivisions - 1)] * 60.0 / std::max(20.0, ctx_.bpm)
                                    : f.delayMs * 0.001;
    fx.delaySamples = std::clamp(secs, 0.001, 2.5) * fs_;
    fx.delayFeedback = std::clamp(f.delayFeedback + d[(int)ModDest::FxFeedback], 0.f, 0.95f);
    fx.delayDamp = f.delayDamp;
    fx.delayMix = std::clamp(f.delayMix + d[(int)ModDest::FxDelay], 0.f, 1.f);
    fx.pingPong = f.delayPingPong;
    fx.reverbMix = std::clamp(f.reverbMix + d[(int)ModDest::FxReverb], 0.f, 1.f);
    fx.width = std::clamp(f.width, 0.f, 2.f);
    fx.outGain = std::pow(10.f, std::clamp(f.outDb, -60.f, 12.f) / 20.f);
}

// ---- Block ------------------------------------------------------------------------------------------
void Synth::process(float* L, float* R, int n, const MidiEvent* events, int numEvents, const Transport& t) {
    fr::ScopedNoDenormals noDenormals;
    std::fill(L, L + n, 0.f); std::fill(R, R + n, 0.f);
    if (n <= 0) return;
    ctx_.p = &p_; ctx_.fs = fs_; ctx_.bpm = std::clamp(t.bpm, 20.0, 999.0);
    ctx_.modWheel = modWheel_; ctx_.aftertouch = aftertouch_; ctx_.bend = bend_;

    // Arp on/off: keys that are down move between the arp and the voices.
    arp_.beginBlock(p_.arp, clock_, fs_, ctx_.bpm, t.playing, t.ppq);
    if (p_.arp.on != arpWasOn_) {
        if (p_.arp.on) {
            for (int k = 0; k < 128; ++k)
                if (keyDown_[(size_t)k] || sustained_[(size_t)k]) { voiceOff(directId(k)); arp_.keyOn(k, keyVel_[(size_t)k], clock_); }
        } else {
            arp_.stop(clock_);
            const int m = arp_.process(clock_, arpOut_.data(), (int)arpOut_.size());
            for (int i = 0; i < m; ++i) if (!arpOut_[(size_t)i].on) voiceOff(arpOut_[(size_t)i].id);
            for (auto& v : voices_) if (v.active() && v.gated() && (v.id() & 0x80000000u) == 0) v.release();
            arp_.clearKeys();
            for (int k = 0; k < 128; ++k)
                if (keyDown_[(size_t)k] || sustained_[(size_t)k]) voiceOn(k, keyVel_[(size_t)k], directId(k), 1.f);
        }
        arpWasOn_ = p_.arp.on;
    }

    int pos = 0, ev = 0;
    while (pos < n) {
        while (ev < numEvents && events[ev].offset <= pos) {
            ctx_.modWheel = modWheel_; ctx_.aftertouch = aftertouch_; ctx_.bend = bend_;
            handleMidi(events[ev++], clock_ + pos);
            ctx_.modWheel = modWheel_; ctx_.aftertouch = aftertouch_; ctx_.bend = bend_;
        }
        if (p_.arp.on || arp_.running()) {
            const int m = arp_.process(clock_ + pos, arpOut_.data(), (int)arpOut_.size());
            for (int i = 0; i < m; ++i) {
                const auto& a = arpOut_[(size_t)i];
                if (a.on) voiceOn(a.note, a.velocity, a.id, a.stepValue);
                else voiceOff(a.id);
            }
        }
        int next = n;
        if (ev < numEvents) next = std::min(next, std::max(pos + 1, events[ev].offset));
        const int64_t at = arp_.nextTime();
        if (at != INT64_MAX) next = std::min<int64_t>(next, std::max<int64_t>(pos + 1, at - clock_));
        next = std::min(next, pos + (kControl - controlPhase_));
        renderVoices(L + pos, R + pos, next - pos);
        controlPhase_ += next - pos;
        if (controlPhase_ >= kControl) { controlPhase_ = 0; controlAll(); }
        pos = next;
    }
    arp_.endBlock(n);

    // Effects, in short chunks so the global modulation (LFOs, macros on effect amounts) moves smoothly.
    reverb_.setShape(p_.fx.reverbSize, p_.fx.reverbDecay, p_.fx.reverbDamp, p_.fx.reverbPreDelay);
    FxState fx;
    for (int off = 0; off < n; off += 128) {
        const int c = std::min(128, n - off);
        computeGlobal(c, fx);
        float* l = L + off; float* r = R + off;
        drive_.process(l, r, c, fx);
        chorus_.process(l, r, c, fx);
        echo_.process(l, r, c, fx);
        reverb_.process(l, r, c, fx.reverbMix);
        const float gs = 1.f - std::exp(-1.f / (0.03f * (float)fs_));
        for (int i = 0; i < c; ++i) {
            widthSmooth_ += (fx.width - widthSmooth_) * gs;
            outGainSmooth_ += (fx.outGain - outGainSmooth_) * gs;
            const float m = 0.5f * (l[i] + r[i]), s = 0.5f * (l[i] - r[i]) * widthSmooth_;
            l[i] = (m + s) * outGainSmooth_; r[i] = (m - s) * outGainSmooth_;
        }
        limiter_.process(l, r, c);
    }

    // UI taps.
    float pk = 0.f, bs = 0.f;
    const float bc = 1.f - std::exp(-kTwoPi * 140.f / (float)fs_);
    int sp = scopePos.load(std::memory_order_relaxed);
    for (int i = 0; i < n; ++i) {
        const float m = 0.5f * (L[i] + R[i]);
        pk = std::max(pk, std::max(std::abs(L[i]), std::abs(R[i])));
        bassLp_ += (m - bassLp_) * bc;
        bs = std::max(bs, std::abs(bassLp_));
        scope[(size_t)sp].store(m, std::memory_order_relaxed);
        if (++sp >= kScopeSize) sp = 0;
    }
    scopePos.store(sp, std::memory_order_release);
    peak.store(std::max(pk, peak.load(std::memory_order_relaxed) * 0.9f), std::memory_order_relaxed);
    bass.store(std::max(bs, bass.load(std::memory_order_relaxed) * 0.9f), std::memory_order_relaxed);
    limiterGain.store(limiter_.reduction(), std::memory_order_relaxed);
    int act = 0; uint64_t newest = 0; int newestIdx = -1;
    for (size_t i = 0; i < voices_.size(); ++i)
        if (voices_[i].active()) { ++act; if (voices_[i].age() >= newest) { newest = voices_[i].age(); newestIdx = (int)i; } }
    activeVoices.store(act, std::memory_order_relaxed);
    arpLane.store(p_.arp.on ? arp_.currentLane() : -1, std::memory_order_relaxed);
    if (newestIdx >= 0)
        for (int o = 0; o < 2; ++o) morphShown[o].store(voices_[(size_t)newestIdx].morphShown(o), std::memory_order_relaxed);
    else
        for (int o = 0; o < 2; ++o) morphShown[o].store(p_.osc[(size_t)o].morph, std::memory_order_relaxed);
    clock_ += n;
}

}  // namespace infra
