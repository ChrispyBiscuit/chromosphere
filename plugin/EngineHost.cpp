#include "EngineHost.h"
#include <chrono>
#include <cmath>
#include <cstdint>

namespace cs {
namespace {
bool sameEdit(const IrEdit& a, const IrEdit& b) {
    return a.startMs == b.startMs && a.maxLengthMs == b.maxLengthMs && a.fadeOutMs == b.fadeOutMs && a.reverse == b.reverse &&
           a.stretch == b.stretch && a.lowCutHz == b.lowCutHz && a.highCutHz == b.highCutHz && a.width == b.width &&
           a.decayScale == b.decayScale && a.normalize == b.normalize && a.targetPowerGain == b.targetPowerGain;
}
bool usesIr(const EngineSettings& s) { return (s.mode == EngineMode::Convolution || s.mode == EngineMode::Hybrid) && s.rawIr; }
// Everything that needs a new engine. Decay times are handled separately.
bool sameStructure(const EngineSettings& a, const EngineSettings& b) {
    if (a.mode != b.mode || a.quality != b.quality || a.size != b.size || a.preDelayMs != b.preDelayMs || a.diffusion != b.diffusion)
        return false;
    if (a.mode == EngineMode::Algorithmic) return true;
    if (a.mode == EngineMode::Cosmic) return a.recipe == b.recipe;
    return a.rawIr == b.rawIr && sameEdit(a.edit, b.edit) && (a.mode != EngineMode::Hybrid || a.erMs == b.erMs);
}
}  // namespace

// What the worker knows about the engine it last published.
struct EngineHost::Built {
    EngineSettings settings;
    EngineSpec spec;
    double effT60[3] = {1, 1, 1};              // decay times the FDN was built or last updated with
    std::shared_ptr<const Ir> edited;          // IR after resampling and edits (cached for decay updates)
};

void EngineHost::makeSpec(const EngineSettings& s, Built& out) const {
    out.settings = s;
    EngineSpec& sp = out.spec;
    sp = EngineSpec();
    sp.mode = s.mode;
    sp.reverb.quality = s.quality; sp.reverb.size = s.size; sp.reverb.preDelayMs = s.preDelayMs; sp.reverb.diffusion = s.diffusion;
    for (int i = 0; i < 3; ++i) sp.reverb.t60[i] = s.t60[i];
    sp.erMs = s.erMs;
    sp.recipe = s.recipe;
    for (int i = 0; i < 3; ++i) sp.macro[i] = s.macro[i];
    out.edited.reset();
    if (usesIr(s) && s.mode != EngineMode::Cosmic) {
        Ir ir = editIr(resampleIr(*s.rawIr, fs_), s.edit);
        out.edited = std::make_shared<const Ir>(std::move(ir));
        sp.ir = out.edited;
        if (s.mode == EngineMode::Hybrid) hybridTailT60(*out.edited, s.tailScale, sp.reverb.t60);
    }
    for (int i = 0; i < 3; ++i) out.effT60[i] = sp.reverb.t60[i];
}

std::unique_ptr<Engine> EngineHost::build(const EngineSettings& s, Built& out) const {
    makeSpec(s, out);
    return makeEngine(out.spec, fs_, kChunk);
}

void EngineHost::prepare(double fs, int maxBlock) {
    release();
    fs_ = fs; maxBlock_ = maxBlock;
    // The worker gets the same settings the first engine was built from. Reading them again on the
    // worker would miss a change made in between (the engine would never be rebuilt for it).
    Built first;
    const EngineSettings initial = settings_();
    active_ = build(initial, first);
    activeGen_ = 1; fading_.reset(); fadingGen_ = 0; fadePos_ = 0;
    fadeLen_ = std::max(1, (int)(kFadeMs * fs / 1000.0));
    for (auto* v : {&aL_, &aR_, &bL_, &bR_}) v->assign(kChunk, 0.f);
    lastEr_ = lastTail_ = -1; lastModDepth_ = lastModRate_ = -1; lastFreeze_ = -1;
    run_ = true;
    worker_ = std::thread([this, initial] { workerLoop(initial); });
}

void EngineHost::release() {
    run_ = false;
    if (worker_.joinable()) worker_.join();
    while (Msg* m = toAudio_.pop()) delete m;
    while (Msg* m = toWorkerMsgs_.pop()) delete m;
    while (Engine* r = toWorkerEngines_.pop()) delete r;
    active_.reset(); fading_.reset();
}

void EngineHost::workerLoop(EngineSettings builtFrom) {
    using clock = std::chrono::steady_clock;
    Built built;
    makeSpec(builtFrom, built);                  // exactly what the audio thread's first engine was built from
    EngineSettings seen = built.settings;        // last value read, for debounce
    double seenT60[3] = {built.effT60[0], built.effT60[1], built.effT60[2]};
    uint32_t gen = 1;                            // generation of the newest engine we published
    auto changedAt = clock::now(), lastBuild = clock::now() - std::chrono::seconds(1);
    auto effectiveT60 = [&](const EngineSettings& s, double out[3]) {
        if (s.mode == EngineMode::Hybrid && built.edited) hybridTailT60(*built.edited, s.tailScale, out);
        else for (int i = 0; i < 3; ++i) out[i] = s.t60[i];
    };
    auto differs = [](const double* a, const double* b) { for (int i = 0; i < 3; ++i) if (std::fabs(a[i] - b[i]) > 1e-4) return true; return false; };

    while (run_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        while (Msg* m = toWorkerMsgs_.pop()) delete m;      // free what the audio thread has used
        while (Engine* r = toWorkerEngines_.pop()) delete r;

        EngineSettings now = settings_();
        auto t = clock::now();
        double nowT60[3];
        effectiveT60(now, nowT60);
        if (!sameStructure(now, seen) || differs(nowT60, seenT60) || now.tailScale != seen.tailScale) changedAt = t;   // something moved since the last look
        seen = now;
        for (int i = 0; i < 3; ++i) seenT60[i] = nowT60[i];

        if (!sameStructure(now, built.settings)) {
            // Wait until the control has stopped moving, and keep builds at least a fade apart.
            if (t - changedAt < std::chrono::milliseconds(80) || t - lastBuild < std::chrono::milliseconds(250)) continue;
            Built next;
            auto* m = new Msg;
            m->engine = build(now, next);
            m->gen = ++gen;
            if (!toAudio_.push(m)) { delete m; continue; }
            built = std::move(next);
            lastBuild = clock::now();
            builds_.fetch_add(1);
        } else if (differs(nowT60, built.effT60)) {
            if (t - changedAt < std::chrono::milliseconds(20)) continue;
            EngineSpec sp = built.spec;
            for (int i = 0; i < 3; ++i) sp.reverb.t60[i] = nowT60[i];
            DecaySet ds = designEngineDecay(sp, fs_);
            if (ds.g0.empty()) { for (int i = 0; i < 3; ++i) built.effT60[i] = nowT60[i]; continue; }   // engine without FDN
            auto* m = new Msg;
            m->decay = std::make_unique<DecaySet>(std::move(ds));
            m->gen = gen;
            if (!toAudio_.push(m)) { delete m; continue; }
            for (int i = 0; i < 3; ++i) built.effT60[i] = nowT60[i];
            built.spec = sp;
        }
    }
}

void EngineHost::pickUpMessages() {
    while (Msg* m = toAudio_.pop()) {
        if (m->engine) {
            if (fading_) {            // previous fade unfinished: snap to it
                retire(active_.release());
                active_ = std::move(fading_); activeGen_ = fadingGen_;
            }
            fading_ = std::move(m->engine); fadingGen_ = m->gen; fadePos_ = 0;
            lastEr_ = lastTail_ = -1; lastModDepth_ = lastModRate_ = -1; lastFreeze_ = -1; lastMacro_[0] = lastMacro_[1] = lastMacro_[2] = -1;   // push live values into it
        } else if (m->decay) {
            const int ramp = (int)(0.05 * fs_);
            if (m->gen == activeGen_ && active_) active_->setDecay(*m->decay, ramp);
            if (m->gen == fadingGen_ && fading_) fading_->setDecay(*m->decay, ramp);
        }
        if (!toWorkerMsgs_.push(m)) { /* worker frees via queue only; leak is impossible unless the worker is stalled */ }
    }
}

void EngineHost::setMacros(double a, double b, double c) {
    if (a == lastMacro_[0] && b == lastMacro_[1] && c == lastMacro_[2]) return;
    lastMacro_[0] = a; lastMacro_[1] = b; lastMacro_[2] = c;
    if (active_) active_->setMacros(a, b, c);
    if (fading_) fading_->setMacros(a, b, c);
}

void EngineHost::process(const float* inL, const float* inR, float* outL, float* outR, int n,
                         float er, float tail, double modDepth, double modRate, bool freeze) {
    if (!active_) { for (int i = 0; i < n; ++i) { outL[i] = 0.f; outR[i] = 0.f; } return; }
    pickUpMessages();
    const int rampMix = (int)(0.02 * fs_);
    if (er != lastEr_ || tail != lastTail_) {
        active_->setLevels(er, tail, rampMix);
        if (fading_) fading_->setLevels(er, tail, rampMix);
        lastEr_ = er; lastTail_ = tail;
    }
    if (modDepth != lastModDepth_ || modRate != lastModRate_) {
        active_->setModulation(modDepth, modRate);
        if (fading_) fading_->setModulation(modDepth, modRate);
        lastModDepth_ = modDepth; lastModRate_ = modRate;
    }
    if ((int)freeze != lastFreeze_) {
        const int ramp = (int)(0.05 * fs_);
        active_->setFreeze(freeze, ramp);
        if (fading_) fading_->setFreeze(freeze, ramp);
        lastFreeze_ = (int)freeze;
    }
    for (int off = 0; off < n; off += kChunk) {
        const int c = std::min(kChunk, n - off);
        active_->process(inL + off, inR + off, aL_.data(), aR_.data(), c);
        if (!fading_) {
            for (int i = 0; i < c; ++i) { outL[off + i] = aL_[i]; outR[off + i] = aR_[i]; }
            continue;
        }
        fading_->process(inL + off, inR + off, bL_.data(), bR_.data(), c);
        for (int i = 0; i < c; ++i) {
            float x = std::min(1.f, (float)(fadePos_ + i) / (float)fadeLen_);
            float go = std::cos(1.5707963f * x), gn = std::sin(1.5707963f * x);
            outL[off + i] = go * aL_[i] + gn * bL_[i];
            outR[off + i] = go * aR_[i] + gn * bR_[i];
        }
        fadePos_ += c;
        if (fadePos_ >= fadeLen_) {
            retire(active_.release());
            active_ = std::move(fading_); activeGen_ = fadingGen_; fadePos_ = 0;
        }
    }
}

}  // namespace cs
