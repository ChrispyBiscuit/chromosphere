#include "EngineHost.h"
#include <chrono>
#include <cmath>

namespace fr {
namespace {
bool sameStructure(const EngineSettings& a, const EngineSettings& b) {
    return a.quality == b.quality && a.size == b.size && a.preDelayMs == b.preDelayMs && a.diffusion == b.diffusion;
}
bool sameDecay(const EngineSettings& a, const EngineSettings& b) {
    for (int i = 0; i < 3; ++i) if (std::fabs(a.t60[i] - b.t60[i]) > 1e-4) return false;
    return true;
}
ReverbParams toParams(const EngineSettings& s) {
    ReverbParams p;
    p.quality = s.quality; p.size = s.size; p.preDelayMs = s.preDelayMs; p.diffusion = s.diffusion;
    for (int i = 0; i < 3; ++i) p.t60[i] = s.t60[i];
    p.dry = 0.f; p.wet = 1.f;
    return p;
}
}  // namespace

std::unique_ptr<Reverb> EngineHost::build(const EngineSettings& s) const {
    auto r = std::make_unique<Reverb>();
    r->configure(toParams(s), fs_, kChunk);
    return r;
}

void EngineHost::prepare(double fs, int maxBlock) {
    release();
    fs_ = fs; maxBlock_ = maxBlock;
    active_ = build(settings_());
    activeGen_ = 1; fading_.reset(); fadingGen_ = 0; fadePos_ = 0;
    fadeLen_ = std::max(1, (int)(kFadeMs * fs / 1000.0));
    for (auto* v : {&aL_, &aR_, &bL_, &bR_}) v->assign(kChunk, 0.f);
    lastEr_ = lastTail_ = -1; lastModDepth_ = lastModRate_ = -1;
    run_ = true;
    worker_ = std::thread([this] { workerLoop(); });
}

void EngineHost::release() {
    run_ = false;
    if (worker_.joinable()) worker_.join();
    while (Msg* m = toAudio_.pop()) delete m;
    while (Msg* m = toWorkerMsgs_.pop()) delete m;
    while (Reverb* r = toWorkerEngines_.pop()) delete r;
    active_.reset(); fading_.reset();
}

void EngineHost::workerLoop() {
    using clock = std::chrono::steady_clock;
    EngineSettings built = settings_();          // what the active engine was built from
    EngineSettings seen = built;                 // last value read, for debounce
    uint32_t gen = 1;                            // generation of the newest engine we published
    auto changedAt = clock::now(), lastBuild = clock::now() - std::chrono::seconds(1);
    std::vector<int> delays;
    auto computeDelays = [&](const EngineSettings& s) { return Fdn::computeDelays(Reverb::fdnConfigFor(toParams(s)), fs_); };
    delays = computeDelays(built);
    EngineSettings designed = built;             // decay values last handed to the audio thread

    while (run_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        while (Msg* m = toWorkerMsgs_.pop()) delete m;      // free what the audio thread has used
        while (Reverb* r = toWorkerEngines_.pop()) delete r;

        EngineSettings now = settings_();
        auto t = clock::now();
        if (!sameStructure(now, seen) || !sameDecay(now, seen)) changedAt = t;
        seen = now;

        if (!sameStructure(now, built)) {
            // Wait until the control has stopped moving, and keep builds at least a fade apart.
            if (t - changedAt < std::chrono::milliseconds(80) || t - lastBuild < std::chrono::milliseconds(250)) continue;
            auto* m = new Msg;
            m->engine = build(now);
            m->gen = ++gen;
            if (!toAudio_.push(m)) { delete m; continue; }
            built = now; designed = now; delays = computeDelays(now);
            lastBuild = clock::now();
            builds_.fetch_add(1);
        } else if (!sameDecay(now, designed)) {
            if (t - changedAt < std::chrono::milliseconds(20)) continue;
            auto* m = new Msg;
            m->decay = std::make_unique<DecaySet>(Fdn::designDecaySet(delays, now.t60, fs_));
            m->gen = gen;
            if (!toAudio_.push(m)) { delete m; continue; }
            designed = now;
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
            lastEr_ = lastTail_ = -1; lastModDepth_ = lastModRate_ = -1;   // push live values into it
        } else if (m->decay) {
            const int ramp = (int)(0.05 * fs_);
            if (m->gen == activeGen_ && active_) active_->setDecay(*m->decay, ramp);
            if (m->gen == fadingGen_ && fading_) fading_->setDecay(*m->decay, ramp);
        }
        if (!toWorkerMsgs_.push(m)) { /* worker frees via queue only; leak is impossible unless the worker is stalled */ }
    }
}

void EngineHost::process(const float* inL, const float* inR, float* outL, float* outR, int n,
                         float er, float tail, double modDepth, double modRate) {
    if (!active_) { for (int i = 0; i < n; ++i) { outL[i] = 0.f; outR[i] = 0.f; } return; }
    pickUpMessages();
    const int rampMix = (int)(0.02 * fs_);
    if (er != lastEr_ || tail != lastTail_) {
        active_->setMix(0.f, 1.f, er, tail, rampMix);
        if (fading_) fading_->setMix(0.f, 1.f, er, tail, rampMix);
        lastEr_ = er; lastTail_ = tail;
    }
    if (modDepth != lastModDepth_ || modRate != lastModRate_) {
        active_->setModulation(modDepth, modRate);
        if (fading_) fading_->setModulation(modDepth, modRate);
        lastModDepth_ = modDepth; lastModRate_ = modRate;
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

}  // namespace fr
