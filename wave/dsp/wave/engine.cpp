#include "engine.h"
#include <algorithm>
#include <cmath>

namespace wave {

namespace {
constexpr float kVoicedConf = 0.75f;        // 1 - YIN's normalised difference, to keep a note going
constexpr float kOnsetConf = 0.85f;         // ... and to start one (breath and hiss reach 0.7-0.8)
constexpr float kGateHysteresisDb = 6.f;
constexpr float kReleaseSec = 0.045f;       // unvoiced this long ends the note
constexpr float kOnsetDipDb = 9.f;          // re-articulation: the level rising out of a dip at least this deep
constexpr float kOnsetMinSec = 0.08f;       // ... at least this long after the last onset
constexpr float kPeakDecayDb = 0.15f;       // the note's level peak decays this much per hop (~55 dB/s)
constexpr float kTuneSec = 0.01f;           // per-note tuning smoothing
constexpr int kExprHops = 4;                // expression controllers at most every 4 hops (~11 ms)

float median3(float a, float b, float c) { return std::max(std::min(a, b), std::min(std::max(a, b), c)); }
}  // namespace

Engine::Engine() {
    for (auto& v : tracePitch) v.store(-1.f, std::memory_order_relaxed);
    for (auto& v : traceLevel) v.store(-120.f, std::memory_order_relaxed);
    for (auto& v : traceNote) v.store(-1, std::memory_order_relaxed);
}

void Engine::prepare(double fs) {
    fs_ = fs;
    tr_.prepare(fs);
    tr_.setRange(p_.range);
    reset();
}

void Engine::reset() {
    tr_.reset();
    now_ = 0;
    gateOpen_ = active_ = false;
    sung_ = changeTarget_ = -1;
    unvoicedHops_ = changeHops_ = hopsSinceOnset_ = candN_ = 0;
    candPeak_ = -120.f;
    std::fill(std::begin(med_), std::end(med_), -1.f);
    dipped_ = false; peakDb_ = dipLow_ = lastDb_ = -120.f;
    prevChord_ = Chord{};
    on_.fill(false);
    soundingN_ = 0;
    cancelPending();
    tune_ = tuneSent_ = 0.f;
    exprCount_ = 0;
    sentAt_ = sentMw_ = 0.f;
    noteNow.store(-1); chordRoot.store(-1); pitchNow.store(-1.f);
}

void Engine::setParams(const Params& p) {
    if (p.range != p_.range) tr_.setRange(p.range);
    p_ = p;
    gain_ = std::pow(10.f, p.gainDb / 20.f);
}

// ---- Key listening ----------------------------------------------------------------------------------------
void Engine::listen(bool on) {
    if (on) clearHist_.store(true);
    listening_.store(on);
}
KeyGuess Engine::guessKey() const {
    float h[12];
    for (int i = 0; i < 12; ++i) h[i] = hist_[(size_t)i].load();
    return findKey(h);
}
float Engine::listenedSeconds() const {
    float s = 0.f;
    for (const auto& v : hist_) s += v.load();
    return s;
}

// ---- Processing -------------------------------------------------------------------------------------------
int Engine::process(const float* in, int n, Event* out, int maxOut) {
    Out o{out, maxOut};
    if (clearHist_.exchange(false)) for (auto& v : hist_) v.store(0.f);
    for (int i = 0; i < n; ++i) {
        while (pendingHead_ < pendingN_ && pending_[pendingHead_].due <= now_) {
            const auto& q = pending_[pendingHead_++];
            noteOn(i, q.note, q.vel, o);
        }
        if (pendingHead_ >= pendingN_) cancelPending();
        if (tr_.push(in ? in[i] * gain_ : 0.f)) onFrame(i, o);
        ++now_;
    }
    return o.n;
}

int Engine::releaseAll(Event* out, int maxOut, int offset) {
    Out o{out, maxOut};
    cancelPending();
    while (soundingN_ > 0) noteOff(offset, sounding_[soundingN_ - 1], o);
    active_ = false;
    candN_ = 0;
    sung_ = -1;
    noteNow.store(-1); chordRoot.store(-1);
    return o.n;
}

void Engine::onFrame(int offset, Out& o) {
    const Frame& f = tr_.frame();
    const float hop = (float)tr_.hopSeconds();
    if (!gateOpen_ && f.levelDb > p_.gateDb) gateOpen_ = true;
    else if (gateOpen_ && f.levelDb < p_.gateDb - kGateHysteresisDb) gateOpen_ = false;
    const bool voiced = gateOpen_ && f.pitch > 20.f && f.pitch < 110.f && f.conf >= kVoicedConf;

    // Median of the last three pitches removes single-hop octave glitches.
    med_[0] = med_[1]; med_[1] = med_[2]; med_[2] = voiced ? f.pitch : -1.f;
    float pm = -1.f;
    if (voiced) {
        if (med_[0] > 0.f && med_[1] > 0.f) pm = median3(med_[0], med_[1], med_[2]);
        else if (med_[1] > 0.f && std::abs(med_[1] - med_[2]) < 1.f) pm = 0.5f * (med_[1] + med_[2]);
        else pm = med_[2];
    }
    // Re-articulation tracking: a dip is the level falling kOnsetDipDb under the note's (slowly decaying) peak.
    const float prevDb = lastDb_;
    lastDb_ = f.levelDb;
    if (active_) {
        peakDb_ = std::max(f.levelDb, peakDb_ - kPeakDecayDb);
        if (!dipped_ && f.levelDb < peakDb_ - kOnsetDipDb) { dipped_ = true; dipLow_ = f.levelDb; }
        if (dipped_) dipLow_ = std::min(dipLow_, f.levelDb);
    }

    if (voiced && listening_.load(std::memory_order_relaxed)) {
        auto& h = hist_[(size_t)mod12((int)std::lround(pm))];
        h.store(h.load(std::memory_order_relaxed) + hop * f.conf, std::memory_order_relaxed);
    }

    if (!active_) {
        if (voiced && f.conf >= kOnsetConf) {
            if (candN_ == 3) { cand_[0] = cand_[1]; cand_[1] = cand_[2]; candN_ = 2; }
            cand_[candN_++] = pm;
            candPeak_ = std::max(candPeak_, f.levelDb);
            if (candN_ >= 2 && std::abs(cand_[candN_ - 1] - cand_[candN_ - 2]) <= 1.f) {
                const float pitch = candN_ == 3 ? median3(cand_[0], cand_[1], cand_[2]) : 0.5f * (cand_[0] + cand_[1]);
                startNote(offset, pitch, candPeak_, o);
            }
        } else {
            candN_ = 0; candPeak_ = -120.f;
        }
    } else if (voiced) {
        unvoicedHops_ = 0;
        ++hopsSinceOnset_;
        // Re-articulation ("da da da"): the level climbs back out of a dip within this note.
        const bool onset = dipped_ && f.levelDb >= dipLow_ + kOnsetDipDb && f.levelDb > prevDb + 0.5f &&
                           hopsSinceOnset_ * hop >= kOnsetMinSec;
        if (onset) {
            startNote(offset, pm, f.levelDb, o);
        } else {
            const int target = quantize(pm, p_.key, p_.scale, sung_);
            if (target != sung_) {
                if (target == changeTarget_) ++changeHops_; else { changeTarget_ = target; changeHops_ = 1; }
                if (changeHops_ * hop * 1000.f >= p_.stabilityMs) changeNote(offset, target, pm, o);
            } else {
                changeHops_ = 0; changeTarget_ = -1;
            }
        }
        // Per-note tuning: the voice's pitch relative to the sung note, scaled by Follow, on every sounding note.
        const float dev = std::clamp(pm - (float)sung_, -24.f, 24.f) * p_.follow;
        tune_ += (dev - tune_) * std::min(1.f, hop / kTuneSec);
        if (std::abs(tune_ - tuneSent_) > 0.004f) {
            for (int i = 0; i < soundingN_; ++i) o.add(offset, Event::Tuning, sounding_[i], tune_);
            tuneSent_ = tune_;
        }
    } else if (++unvoicedHops_ * hop > kReleaseSec) {
        endNote(offset, o);
    }

    expression(offset, f, o);

    // UI history.
    const int pos = tracePos.load(std::memory_order_relaxed);
    tracePitch[(size_t)pos].store(pm, std::memory_order_relaxed);
    traceLevel[(size_t)pos].store(f.levelDb, std::memory_order_relaxed);
    traceNote[(size_t)pos].store(active_ ? sung_ : -1, std::memory_order_relaxed);
    tracePos.store((pos + 1) % kTrace, std::memory_order_release);
    pitchNow.store(pm, std::memory_order_relaxed);
    levelNow.store(f.levelDb, std::memory_order_relaxed);
    brightNow.store(f.bright, std::memory_order_relaxed);
    noteNow.store(active_ ? sung_ : -1, std::memory_order_relaxed);
}

float Engine::velocity(float peakDb) const {
    const float x = std::clamp((peakDb - p_.gateDb) / 36.f, 0.f, 1.f);
    return (1.f - p_.dynamics) * (100.f / 127.f) + p_.dynamics * (0.25f + 0.75f * x);
}

Chord Engine::makeChord(int sung) {
    Chord c;
    if (p_.mode == Mode::Melody) {
        c.n = 1; c.notes[0] = sung; c.root = -1;
    } else {
        ChordSpec s;
        s.type = p_.chord; s.role = p_.role; s.voicing = p_.voicing; s.bass = p_.bass; s.key = p_.key; s.scale = p_.scale;
        c = buildChord(sung, s, prevChord_.n > 0 ? &prevChord_ : nullptr);
        prevChord_ = c;
    }
    chordRoot.store(c.root, std::memory_order_relaxed);
    chordMask.store(c.mask, std::memory_order_relaxed);
    // Octave shift (notes outside MIDI's range are dropped).
    Chord o = c; o.n = 0;
    for (int i = 0; i < c.n; ++i) {
        const int m = c.notes[i] + 12 * p_.octave;
        if (m >= 0 && m <= 127) o.notes[o.n++] = m;
    }
    return o;
}

void Engine::startNote(int offset, float pitch, float peakDb, Out& o) {
    sung_ = quantize(pitch, p_.key, p_.scale);
    vel_ = velocity(peakDb);
    active_ = true;
    unvoicedHops_ = changeHops_ = hopsSinceOnset_ = 0;
    changeTarget_ = -1;
    candN_ = 0; candPeak_ = -120.f;
    dipped_ = false; peakDb_ = peakDb;                    // a re-articulation needs a dip within this note
    tune_ = std::clamp(pitch - (float)sung_, -24.f, 24.f) * p_.follow;
    tuneSent_ = 0.f;
    play(offset, makeChord(sung_), vel_, false, o);
}

void Engine::changeNote(int offset, int sung, float pitch, Out& o) {
    sung_ = sung;
    changeHops_ = 0; changeTarget_ = -1;
    tune_ = std::clamp(pitch - (float)sung_, -24.f, 24.f) * p_.follow;
    tuneSent_ = 0.f;
    play(offset, makeChord(sung_), vel_, true, o);
}

void Engine::endNote(int offset, Out& o) {
    active_ = false;
    candN_ = 0; candPeak_ = -120.f;
    if (p_.hold) return;                                  // held until the next sung note
    cancelPending();
    while (soundingN_ > 0) noteOff(offset, sounding_[soundingN_ - 1], o);
    chordRoot.store(-1, std::memory_order_relaxed);
}

// Starts a chord. A fresh articulation ends every sounding note first; a legato change starts the new tones first
// and then ends the old ones that are not part of the new chord (common tones keep sounding).
void Engine::play(int offset, const Chord& c, float vel, bool legato, Out& o) {
    cancelPending();
    if (!legato) while (soundingN_ > 0) noteOff(offset, sounding_[soundingN_ - 1], o);
    // Common tones (legato) keep sounding; they get the new tuning below.
    int kept[16]; int keptN = 0;
    for (int i = soundingN_ - 1; i >= 0; --i) {
        const int m = sounding_[i];
        bool keep = false;
        for (int j = 0; j < c.n; ++j) keep |= c.notes[j] == m;
        if (keep) kept[keptN++] = m;
    }
    const int64_t strum = (int64_t)std::lround(p_.strumMs * 0.001 * fs_);
    int k = 0;
    for (int i = 0; i < c.n; ++i) {
        const int m = c.notes[i];
        if (on_[(size_t)m]) continue;
        if (k == 0 || strum <= 0) noteOn(offset, m, vel, o);
        else if (pendingN_ < 16) pending_[pendingN_++] = {now_ + strum * k, m, vel};
        ++k;
    }
    for (int i = soundingN_ - 1; i >= 0; --i) {
        const int m = sounding_[i];
        bool keep = false;
        for (int j = 0; j < c.n; ++j) keep |= c.notes[j] == m;
        if (!keep) noteOff(offset, m, o);
    }
    if (p_.follow > 0.f) for (int i = 0; i < keptN; ++i) o.add(offset, Event::Tuning, kept[i], tune_);
    tuneSent_ = tune_;
}

void Engine::noteOn(int offset, int note, float vel, Out& o) {
    if (on_[(size_t)note] || soundingN_ >= 16) return;
    o.add(offset, Event::NoteOn, note, vel);
    on_[(size_t)note] = true;
    sounding_[soundingN_++] = note;
    if (p_.follow > 0.f && tune_ != 0.f) o.add(offset, Event::Tuning, note, tune_);
}

void Engine::noteOff(int offset, int note, Out& o) {
    for (int i = 0; i < soundingN_; ++i)
        if (sounding_[i] == note) {
            o.add(offset, Event::NoteOff, note, 0.f);
            on_[(size_t)note] = false;
            sounding_[i] = sounding_[--soundingN_];
            return;
        }
}

void Engine::expression(int offset, const Frame& f, Out& o) {
    if (++exprCount_ < kExprHops) return;
    exprCount_ = 0;
    const float level = gateOpen_ ? std::clamp((f.levelDb - p_.gateDb) / 40.f, 0.f, 1.f) : 0.f;
    const float bright = gateOpen_ ? f.bright : 0.f;
    float at = -1.f, mw = -1.f;                           // -1: no source on this controller
    auto route = [&](Expr e, float v) {
        if (e == Expr::Aftertouch) at = std::max(at, v);
        else if (e == Expr::ModWheel) mw = std::max(mw, v);
    };
    route(p_.levelTo, level);
    route(p_.brightTo, bright);
    // A controller that lost its source returns to zero once.
    if (at < 0.f && sentAt_ > 0.f) at = 0.f;
    if (mw < 0.f && sentMw_ > 0.f) mw = 0.f;
    if (at >= 0.f && std::abs(at - sentAt_) >= 1.f / 127.f) { o.add(offset, Event::Aftertouch, 0, at); sentAt_ = at; }
    if (mw >= 0.f && std::abs(mw - sentMw_) >= 1.f / 127.f) { o.add(offset, Event::ModWheel, 0, mw); sentMw_ = mw; }
}

}  // namespace wave
