#include "machine.h"
#include "common/denormal.h"

namespace ultra {

namespace {
// Models that are monophonic on their track: a retrigger fades the previous hit (no phasing between two kicks).
bool monophonic(Model m) {
    return m == Model::CoreKick || m == Model::GravityKick || m == Model::OrbitTom || m == Model::PulsarFm || m == Model::PlasmaZap;
}
}  // namespace

DrumMachine::DrumMachine() = default;
DrumMachine::~DrumMachine() { release(); }

void DrumMachine::prepare(double fs, int maxBlock) {
    fs_ = fs; maxBlock_ = std::clamp(maxBlock, 32, 8192);
    for (int t = 0; t < kTracks; ++t)
        for (int s = 0; s < kSlots; ++s) voices_[(size_t)t][(size_t)s].prepare(fs, 0x1234u + 7919u * (uint32_t)(t * kSlots + s));
    for (auto* v : {&aL_, &aR_, &bL_, &bR_, &eL_, &eR_, &rL_, &rR_}) v->assign((size_t)maxBlock_, 0.f);
    seq_.prepare(fs);
    drive_.prepare(fs); glue_.prepare(fs); echo_.prepare(fs); limiter_.prepare(fs);
    const auto& m = p_.master;
    reverb_.setShape(m.reverbSize, m.reverbDecay, m.reverbDamp, m.reverbPreDelay);
    reverb_.prepare(fs, maxBlock_);
    lastLow_ = lastHigh_ = 1e9f;
    reset();
}

void DrumMachine::release() { reverb_.release(); }

void DrumMachine::reset() {
    for (auto& tr : voices_) for (auto& v : tr) v.kill();
    seq_.reset();
    drive_.reset(); glue_.reset(); echo_.reset();
    lowShelf_.reset(); highShelf_.reset();
    retASmooth_ = retBSmooth_ = 0.f;
    dcX_[0] = dcX_[1] = dcY_[0] = dcY_[1] = 0.f;
    sleeping_ = false; silentSamples_ = 0;
    trackPeak_.fill(0.f);
}

float DrumMachine::effectiveSwing() const { return std::clamp(p_.swing + 0.2f * p_.macro[2], 0.5f, 0.75f); }

void DrumMachine::trigger(int track, float vel, int pitch, float lock) {
    if (track < 0 || track >= kTracks) return;
    const auto& tp = p_.tracks[(size_t)track];
    const float heat = p_.macro[0], grav = p_.macro[1];
    VoiceParams vp = tp.v;
    vp.tone += 0.35f * heat;
    vp.snap += 0.25f * heat;
    float decayMul = 1.f;
    if (isWeighty(vp.model)) { decayMul *= 1.f + 1.5f * grav; vp.tune -= 5.f * grav; }
    vel = std::clamp(vel, 0.f, 1.f);
    vp.tone += 0.15f * vp.velSens * (vel - 0.75f);
    vp.tune += (float)pitch;
    float lockPan = 0.f, lockA = 0.f, lockB = 0.f, cutMul = 1.f;
    switch (tp.lock) {
        case LockTarget::Decay: vp.decay += lock; break;
        case LockTarget::Tone: vp.tone += lock; break;
        case LockTarget::Snap: vp.snap += lock; break;
        case LockTarget::Color: vp.color += lock; break;
        case LockTarget::Sweep: vp.sweep += lock; break;
        case LockTarget::Drive: vp.drive += lock; break;
        case LockTarget::Cutoff: cutMul = std::exp2(lock * 4.f); break;
        case LockTarget::Pan: lockPan = lock; break;
        case LockTarget::SendA: lockA = lock; break;
        case LockTarget::SendB: lockB = lock; break;
    }
    // Choke group: cut the other tracks of the group.
    if (tp.choke > 0)
        for (int u = 0; u < kTracks; ++u)
            if (u != track && p_.tracks[(size_t)u].choke == tp.choke)
                for (auto& v : voices_[(size_t)u]) v.choke(5.f);
    auto& slots = voices_[(size_t)track];
    if (monophonic(vp.model)) {
        for (auto& v : slots) v.choke(3.f);
    } else {
        // At most two hits ring fully: fade the oldest when two already do.
        int sounding = 0; DrumVoice* oldest = nullptr;
        for (auto& v : slots)
            if (v.active() && !v.fading()) { ++sounding; if (!oldest || v.age() < oldest->age()) oldest = &v; }
        if (sounding >= 2 && oldest) oldest->choke(3.f);
    }
    DrumVoice* pick = nullptr;
    for (auto& v : slots) if (!v.active()) { pick = &v; break; }
    if (!pick) for (auto& v : slots) if (!pick || v.age() < pick->age()) pick = &v;
    pick->lockPan = lockPan; pick->lockSendA = lockA; pick->lockSendB = lockB; pick->cutoffMul = cutMul;
    const float vs = std::clamp(tp.v.velSens, 0.f, 1.f);
    pick->trigger(vp, decayMul, 1.f - vs + vs * vel);
    pick->setAge(++ageCounter_);
    hitCount[(size_t)track].fetch_add(1, std::memory_order_relaxed);
    hitVelocity[(size_t)track].store(vel, std::memory_order_relaxed);
}

void DrumMachine::handleMidi(const MidiEvent& e) {
    switch (e.type) {
        case MidiEvent::NoteOn:
            if (e.value <= 0.f) break;
            if (e.number >= 24 && e.number < 24 + kPatterns) { seq_.requestPattern(e.number - 24); break; }
            for (int t = 0; t < kTracks; ++t) if (p_.tracks[(size_t)t].note == e.number) trigger(t, e.value);
            break;
        case MidiEvent::NoteOff: break;
        case MidiEvent::AllNotesOff:
            for (auto& tr : voices_) for (auto& v : tr) v.choke(5.f);
            break;
    }
}

void DrumMachine::renderTracks(const Outputs& out, int pos, int len, bool anySolo) {
    const float flux = p_.macro[3];
    for (int t = 0; t < kTracks; ++t) {
        const auto& tp = p_.tracks[(size_t)t];
        const bool audible = !tp.mute && (!anySolo || tp.solo);
        const float lvl = audible && tp.levelDb > -59.9f ? dbToGain(std::min(tp.levelDb, 12.f)) : 0.f;
        float* dL = out.L + pos; float* dR = out.R + pos;
        if (tp.output > 0 && tp.output <= kNumAux && out.auxL[(size_t)tp.output - 1] && out.auxR[(size_t)tp.output - 1]) {
            dL = out.auxL[(size_t)tp.output - 1] + pos; dR = out.auxR[(size_t)tp.output - 1] + pos;
        }
        const float sendA = tp.sendA * (1.f + 2.f * flux) + 0.2f * flux, sendB = tp.sendB * (1.f + 2.f * flux) + 0.2f * flux;
        float pk = 0.f;
        for (auto& v : voices_[(size_t)t]) {
            if (!v.active()) continue;
            v.setFilter(tp.v.filter, tp.v.cutoff, tp.v.resonance);
            std::fill(mono_.begin(), mono_.begin() + len, 0.f);
            v.render(mono_.data(), len);
            const float pan = std::clamp(tp.pan + v.lockPan, -1.f, 1.f);
            const float pl = std::min(1.f, 1.f - pan), pr = std::min(1.f, 1.f + pan);
            const float tl = lvl * pl, trr = lvl * pr;
            const float ta = lvl * std::clamp(sendA + v.lockSendA, 0.f, 1.f), tb = lvl * std::clamp(sendB + v.lockSendB, 0.f, 1.f);
            if (!v.mixInit) { v.mixL = tl; v.mixR = trr; v.mixA = ta; v.mixB = tb; v.mixInit = true; }
            const float inv = 1.f / (float)len;
            const float dl = (tl - v.mixL) * inv, dr = (trr - v.mixR) * inv, da = (ta - v.mixA) * inv, db = (tb - v.mixB) * inv;
            float gl = v.mixL, gr = v.mixR, ga = v.mixA, gb = v.mixB;
            float* al = aL_.data() + pos; float* ar = aR_.data() + pos; float* bl = bL_.data() + pos; float* br = bR_.data() + pos;
            for (int i = 0; i < len; ++i) {
                gl += dl; gr += dr; ga += da; gb += db;
                const float x = mono_[(size_t)i];
                dL[i] += x * gl; dR[i] += x * gr;
                al[i] += x * ga * pl; ar[i] += x * ga * pr;
                bl[i] += x * gb * pl; br[i] += x * gb * pr;
                pk = std::max(pk, std::abs(x) * lvl);
            }
            v.mixL = tl; v.mixR = trr; v.mixA = ta; v.mixB = tb;
        }
        trackPeak_[(size_t)t] = std::max(trackPeak_[(size_t)t], pk);
    }
}

void DrumMachine::process(const Outputs& out, int n, const MidiEvent* events, int numEvents, const Transport& t) {
    if (n <= 0) return;
    if (n > maxBlock_) {
        // Larger than prepared: in chunks, with the transport and the events moved along.
        Transport tr = t;
        int ev = 0;
        MidiEvent local[256];
        for (int off = 0; off < n; off += maxBlock_) {
            const int c = std::min(maxBlock_, n - off);
            int m = 0;
            while (ev < numEvents && events[ev].offset < off + c) { if (m < 256) { local[m] = events[ev]; local[m].offset -= off; ++m; } ++ev; }
            Outputs o = out;
            o.L += off; o.R += off;
            for (int k = 0; k < kNumAux; ++k) { if (o.auxL[(size_t)k]) o.auxL[(size_t)k] += off; if (o.auxR[(size_t)k]) o.auxR[(size_t)k] += off; }
            process(o, c, local, m, tr);
            if (tr.playing) tr.ppq += c * tr.bpm / 60.0 / fs_;
        }
        return;
    }
    fr::ScopedNoDenormals noDenormals;
    std::fill(out.L, out.L + n, 0.f); std::fill(out.R, out.R + n, 0.f);
    for (int k = 0; k < kNumAux; ++k) {
        if (out.auxL[(size_t)k]) std::fill(out.auxL[(size_t)k], out.auxL[(size_t)k] + n, 0.f);
        if (out.auxR[(size_t)k]) std::fill(out.auxR[(size_t)k], out.auxR[(size_t)k] + n, 0.f);
    }
    for (auto* v : {&aL_, &aR_, &bL_, &bR_}) std::fill(v->begin(), v->begin() + n, 0.f);
    const PatternBank& bank = bank_ ? *bank_ : empty_;
    const float heat = p_.macro[0], grav = p_.macro[1], orbit = p_.macro[2], flux = p_.macro[3];

    const int numHits = seq_.process(bank, p_, effectiveSwing(), orbit, 1.f + orbit, t, internalPlay.load(std::memory_order_relaxed),
                                     n, hits_.data(), (int)hits_.size());

    // Sleep on silence (EXO's optimization guide, 5.1): with no voice ringing, nothing to trigger this block and the
    // output below -120 dBFS for a quarter of a second, the voices and the master bus are not run. Any hit, MIDI note
    // or pad wakes the engine on the same block; its state was already below hearing, so there is no click.
    bool anyVoice = false;
    for (const auto& tr : voices_) for (const auto& v : tr) anyVoice |= v.active();
    const bool idle = !anyVoice && numHits == 0 && numEvents == 0;
    if (!idle) sleeping_ = false;
    else if (!sleeping_ && silentSamples_ >= (int64_t)(0.25 * fs_)) sleeping_ = true;
    if (sleeping_) {
        peak.store(peak.load(std::memory_order_relaxed) * 0.9f, std::memory_order_relaxed);
        bass.store(bass.load(std::memory_order_relaxed) * 0.9f, std::memory_order_relaxed);
        glueReduction.store(0.f, std::memory_order_relaxed);
        limiterGain.store(1.f, std::memory_order_relaxed);
        currentPattern.store(seq_.currentPattern(), std::memory_order_relaxed);
        queuedPattern.store(seq_.queuedPattern(), std::memory_order_relaxed);
        running.store(seq_.running(), std::memory_order_relaxed);
        position.store(seq_.position(), std::memory_order_relaxed);
        bpm.store(seq_.bpm(), std::memory_order_relaxed);
        for (int tr = 0; tr < kTracks; ++tr) {
            stepNow[(size_t)tr].store(seq_.stepOf(tr), std::memory_order_relaxed);
            trackLevel[(size_t)tr].store(trackLevel[(size_t)tr].load(std::memory_order_relaxed) * 0.85f, std::memory_order_relaxed);
        }
        activeVoices.store(0, std::memory_order_relaxed);
        return;
    }
    bool anySolo = false;
    for (const auto& tp : p_.tracks) anySolo |= tp.solo;

    int pos = 0, hi = 0, ei = 0;
    while (pos < n) {
        while (ei < numEvents && events[ei].offset <= pos) handleMidi(events[ei++]);
        while (hi < numHits && hits_[(size_t)hi].offset <= pos) {
            const auto& h = hits_[(size_t)hi++];
            trigger(h.track, h.velocity, h.pitch, h.lock);
        }
        int next = std::min(n, pos + kSub);
        if (ei < numEvents) next = std::min(next, std::max(pos + 1, events[ei].offset));
        if (hi < numHits) next = std::min(next, std::max(pos + 1, hits_[(size_t)hi].offset));
        renderTracks(out, pos, next - pos, anySolo);
        pos = next;
    }

    // ---- Master bus ----
    const auto& m = p_.master;
    float* L = out.L; float* R = out.R;
    drive_.process(L, R, n, m.driveMode, std::clamp(m.drive + 0.5f * heat, 0.f, 1.f), m.driveTone, m.driveMix);
    glue_.process(L, R, n, m.glueThresh, m.glueRatio, m.glueAttack, m.glueRelease, m.glueMakeup + 3.f * heat, m.glueMix);
    const double bpmNow = seq_.bpm();
    const double secs = m.echoSync ? kDivisionBeats[std::clamp(m.echoDivision, 0, kNumDivisions - 1)] * 60.0 / std::max(20.0, bpmNow)
                                   : m.echoMs * 0.001;
    echo_.process(aL_.data(), aR_.data(), eL_.data(), eR_.data(), n, std::clamp(secs, 0.001, 2.5) * fs_,
                  m.echoFeedback + 0.25f * flux, m.echoDamp, m.echoPingPong);
    const float toRev = std::clamp(m.echoToReverb, 0.f, 1.f);
    for (int i = 0; i < n; ++i) { rL_[(size_t)i] = bL_[(size_t)i] + eL_[(size_t)i] * toRev; rR_[(size_t)i] = bR_[(size_t)i] + eR_[(size_t)i] * toRev; }
    reverb_.setShape(m.reverbSize, m.reverbDecay * (1.f + flux), m.reverbDamp, m.reverbPreDelay);
    reverb_.process(rL_.data(), rR_.data(), rL_.data(), rR_.data(), n);
    const float gs = 1.f - std::exp(-1.f / (0.03f * (float)fs_));
    const float retA = std::clamp(m.echoReturn, 0.f, 1.5f), retB = std::clamp(m.reverbReturn, 0.f, 1.5f);
    for (int i = 0; i < n; ++i) {
        retASmooth_ += (retA - retASmooth_) * gs; retBSmooth_ += (retB - retBSmooth_) * gs;
        L[i] += eL_[(size_t)i] * retASmooth_ + rL_[(size_t)i] * retBSmooth_;
        R[i] += eR_[(size_t)i] * retASmooth_ + rR_[(size_t)i] * retBSmooth_;
    }
    const float lowDb = std::clamp(m.lowShelf + 6.f * grav, -18.f, 18.f), highDb = std::clamp(m.highShelf, -18.f, 18.f);
    if (lowDb != lastLow_) { lowShelf_.shelf(false, 100.f, lowDb, (float)fs_); lastLow_ = lowDb; }
    if (highDb != lastHigh_) { highShelf_.shelf(true, 8000.f, highDb, (float)fs_); lastHigh_ = highDb; }
    const bool eqLow = std::abs(lowDb) > 0.01f, eqHigh = std::abs(highDb) > 0.01f;
    const float width = std::clamp(m.width, 0.f, 2.f), outGain = dbToGain(std::clamp(m.outDb, -60.f, 18.f));
    const float dcR = 1.f - kTwoPi * 8.f / (float)fs_;          // DC blocker at 8 Hz: kick bodies start at phase 0
    for (int i = 0; i < n; ++i) {
        float l = L[i], r = R[i];
        if (eqLow) { l = lowShelf_.tick(l, 0); r = lowShelf_.tick(r, 1); }
        if (eqHigh) { l = highShelf_.tick(l, 0); r = highShelf_.tick(r, 1); }
        widthSmooth_ += (width - widthSmooth_) * gs;
        outGainSmooth_ += (outGain - outGainSmooth_) * gs;
        const float mid = 0.5f * (l + r), side = 0.5f * (l - r) * widthSmooth_;
        const float ol = (mid + side) * outGainSmooth_, orr = (mid - side) * outGainSmooth_;
        dcY_[0] = ol - dcX_[0] + dcR * dcY_[0]; dcX_[0] = ol;
        dcY_[1] = orr - dcX_[1] + dcR * dcY_[1]; dcX_[1] = orr;
        L[i] = dcY_[0]; R[i] = dcY_[1];
    }
    limiter_.process(L, R, n);
    float outPeak = 0.f;
    for (int i = 0; i < n; ++i) outPeak = std::max(outPeak, std::max(std::abs(L[i]), std::abs(R[i])));
    for (int k = 0; k < kNumAux; ++k)
        if (out.auxL[(size_t)k] && out.auxR[(size_t)k])
            for (int i = 0; i < n; ++i) outPeak = std::max(outPeak, std::max(std::abs(out.auxL[(size_t)k][i]), std::abs(out.auxR[(size_t)k][i])));
    silentSamples_ = outPeak > kSleepLevel ? 0 : silentSamples_ + n;

    // ---- UI taps ----
    float pk = 0.f, bs = 0.f;
    const float bc = 1.f - std::exp(-kTwoPi * 140.f / (float)fs_);
    int sp = scopePos.load(std::memory_order_relaxed);
    for (int i = 0; i < n; ++i) {
        const float mono = 0.5f * (L[i] + R[i]);
        pk = std::max(pk, std::max(std::abs(L[i]), std::abs(R[i])));
        bassLp_ += (mono - bassLp_) * bc;
        bs = std::max(bs, std::abs(bassLp_));
        scope[(size_t)sp].store(mono, std::memory_order_relaxed);
        if (++sp >= kScopeSize) sp = 0;
    }
    scopePos.store(sp, std::memory_order_release);
    peak.store(std::max(pk, peak.load(std::memory_order_relaxed) * 0.9f), std::memory_order_relaxed);
    bass.store(std::max(bs, bass.load(std::memory_order_relaxed) * 0.9f), std::memory_order_relaxed);
    limiterGain.store(limiter_.reduction(), std::memory_order_relaxed);
    glueReduction.store(glue_.reductionDb(), std::memory_order_relaxed);
    currentPattern.store(seq_.currentPattern(), std::memory_order_relaxed);
    queuedPattern.store(seq_.queuedPattern(), std::memory_order_relaxed);
    running.store(seq_.running(), std::memory_order_relaxed);
    position.store(seq_.position(), std::memory_order_relaxed);
    bpm.store(bpmNow, std::memory_order_relaxed);
    int act = 0;
    for (int tr = 0; tr < kTracks; ++tr) {
        stepNow[(size_t)tr].store(seq_.stepOf(tr), std::memory_order_relaxed);
        trackLevel[(size_t)tr].store(std::max(trackPeak_[(size_t)tr], trackLevel[(size_t)tr].load(std::memory_order_relaxed) * 0.85f), std::memory_order_relaxed);
        trackPeak_[(size_t)tr] = 0.f;
        for (auto& v : voices_[(size_t)tr]) act += v.active() ? 1 : 0;
    }
    activeVoices.store(act, std::memory_order_relaxed);
}

}  // namespace ultra
