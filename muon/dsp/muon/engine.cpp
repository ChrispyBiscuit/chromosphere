#include "engine.h"
#include "EngineHost.h"
#include "common/denormal.h"

namespace muon {

namespace {
void kWeight(double fs, float* shelf, float* hp) {
    {   // high shelf, +4 dB at 1682 Hz
        const double G = 3.999843853973347, Q = 0.7071752369554196, fc = 1681.974450955533;
        const double A = std::pow(10.0, G / 40.0), w = 2.0 * kPi * fc / fs, cw = std::cos(w), al = std::sin(w) / (2.0 * Q), sa = std::sqrt(A);
        const double b0 = A * ((A + 1) + (A - 1) * cw + 2 * sa * al), b1 = -2 * A * ((A - 1) + (A + 1) * cw), b2 = A * ((A + 1) + (A - 1) * cw - 2 * sa * al);
        const double a0 = (A + 1) - (A - 1) * cw + 2 * sa * al, a1 = 2 * ((A - 1) - (A + 1) * cw), a2 = (A + 1) - (A - 1) * cw - 2 * sa * al;
        shelf[0] = (float)(b0 / a0); shelf[1] = (float)(b1 / a0); shelf[2] = (float)(b2 / a0); shelf[3] = (float)(a1 / a0); shelf[4] = (float)(a2 / a0);
    }
    {   // high-pass at 38 Hz
        const double Q = 0.5003270373238773, fc = 38.13547087602444;
        const double w = 2.0 * kPi * fc / fs, cw = std::cos(w), al = std::sin(w) / (2.0 * Q);
        const double a0 = 1 + al;
        hp[0] = (float)((1 + cw) / 2 / a0); hp[1] = (float)(-(1 + cw) / a0); hp[2] = hp[0]; hp[3] = (float)(-2 * cw / a0); hp[4] = (float)((1 - al) / a0);
    }
}
inline float smoothstep(float x) { x = std::clamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); }
inline float coefFor(double fs, double seconds) { return (float)std::exp(-1.0 / (seconds * fs)); }
}  // namespace

Engine::Engine() {
    glow_ = std::make_unique<fr::EngineHost>([this] {
        fr::EngineSettings s;
        s.mode = fr::EngineMode::Algorithmic;
        s.quality = fr::Quality::Normal;
        s.size = std::clamp((double)glowSize_.load(), 0.5, 3.0);
        s.preDelayMs = std::clamp((double)glowPre_.load(), 0.0, 250.0);
        s.diffusion = true;
        const double mid = std::clamp((double)glowDecay_.load(), 0.2, 30.0), damp = std::clamp((double)glowDamp_.load(), 0.0, 1.0);
        s.t60[0] = mid * 1.15; s.t60[1] = mid; s.t60[2] = std::max(0.15, mid * (1.0 - 0.85 * damp));
        return s;
    });
}

Engine::~Engine() { release(); }

int Engine::osIndex() const {
    const int f = kOversampleFactors[std::clamp(p_.oversample, 0, 3)];
    return f >= 8 ? 3 : f >= 4 ? 2 : f >= 2 ? 1 : 0;
}

int Engine::latencyFor(const Params& p) const {
    int lat = (int)std::lround(kLookaheadMs[std::clamp(p.lookahead, 0, 3)] * 0.001 * fs_);
    if (p.crossover == CrossoverType::LinearPhase) {
        lat += xover_.latency(CrossoverType::LinearPhase);
        lat += resamplerLatency(kOversampleFactors[std::clamp(p.oversample, 0, 3)], true);
    }
    return lat;
}

float Engine::bandScale(int b, int bands) const {
    if (bands == 1) return 1.f;
    if (bands == 2) return b == 0 ? 2.f : 0.8f;
    return b == 0 ? 2.f : b == 1 ? 1.f : 0.6f;
}

float Engine::bandEdge(int b, int bands) const {
    if (b == bands - 1) return 20000.f;
    return b == 0 ? p_.xoverLow : p_.xoverHigh;
}

int Engine::maxOrder(int b) const {
    const int bands = std::clamp(p_.bands, 1, kMaxBands);
    return maxCleanOrder(fs_, kOversampleFactors[std::clamp(p_.oversample, 0, 3)], bandEdge(b, bands));
}

void Engine::prepare(double fs, int maxBlock) {
    release();
    fs_ = fs; maxBlock_ = std::max(1, maxBlock);
    xover_.prepare(fs, kChunk, p_);
    const int maxLook = (int)std::ceil(0.005 * fs) + 2;
    const int maxOsLat = resamplerLatency(8, true) + 2;
    for (auto& b : band_) {
        for (int c = 0; c < 2; ++c) { b.look[c].prepare(maxLook); b.osDelay[c].prepare(maxOsLat); b.clip[c].curve = Curve::Tanh; }
        b.lastSpeed = b.lastScale = -1.f;
    }
    for (int lin = 0; lin < 2; ++lin)
        for (int o = 0; o < 4; ++o)
            for (int b = 0; b < kMaxBands; ++b)
                {
                    harm_[lin][o][b] = std::make_unique<HarmonicBand>();
                    harm_[lin][o][b]->prepare(fs, kOversampleFactors[o], lin == 1, kChunk);
                }
    harmLin_ = harmOs_ = -1;
    sub_.prepare(fs);
    res_.prepare(fs);
    tracker_.prepare(fs);
    glowSize_ = p_.glowSize; glowDecay_ = p_.glowDecay; glowDamp_ = p_.glowDamp; glowPre_ = p_.glowPre;
    glow_->prepare(fs, kChunk);
    const int maxLat = maxLook + xover_.latency(CrossoverType::LinearPhase) + maxOsLat + 4;
    for (int c = 0; c < 2; ++c) { dryDelay_[c].prepare(maxLat); auxDelay_[c].prepare(maxOsLat); }
    float sh[5], hp[5];
    kWeight(fs, sh, hp);
    for (int c = 0; c < 2; ++c) for (auto* k : {&kIn_[c][0], &kOut_[c][0]}) { k->b0 = sh[0]; k->b1 = sh[1]; k->b2 = sh[2]; k->a1 = sh[3]; k->a2 = sh[4]; }
    for (int c = 0; c < 2; ++c) for (auto* k : {&kIn_[c][1], &kOut_[c][1]}) { k->b0 = hp[0]; k->b1 = hp[1]; k->b2 = hp[2]; k->a1 = hp[3]; k->a2 = hp[4]; }
    agA_ = coefFor(fs, 0.4); agGainA_ = coefFor(fs, 0.6);
    limRel_ = coefFor(fs, 0.08);
    auto sz = [&](std::vector<float>& v) { v.assign(kChunk, 0.f); };
    for (int b = 0; b < kMaxBands; ++b) for (int c = 0; c < 2; ++c) { sz(bandBuf_[b][c]); sz(bandOut_[b][c]); sz(gainArr_[b][c]); sz(maskArr_[b][c]); sz(env2Arr_[b][c]); }
    for (int c = 0; c < 2; ++c) { sz(src_[c]); sz(oth_[c]); sz(direct_[c]); sz(tSum_[c]); sz(bSum_[c]); sz(wet_[c]); sz(dry_[c]); sz(aux_[c]); }
    for (auto* v : {&mono_, &subOut_, &resL_, &resR_, &glowL_, &glowR_, &lowMono_, &amt_, &drv_, &dmx_}) sz(*v);
    prepared_ = true;
    reset();
}

void Engine::release() {
    if (!prepared_) return;
    glow_->release();
    xover_.release();
    prepared_ = false;
}

void Engine::reset() {
    xover_.reset();
    for (auto& b : band_) {
        for (int c = 0; c < 2; ++c) {
            b.det[c].e1 = b.det[c].e2 = b.det[c].e3 = 0.f; b.det[c].g = 1.f; b.det[c].hold = 0;
            b.look[c].reset(); b.osDelay[c].reset(); b.clip[c].reset(); b.mask[c] = 0.f;
        }
        for (auto* s : {&b.attack, &b.sustain, &b.amount, &b.drive, &b.driveMix}) s->snap();
    }
    for (auto& a : harm_) for (auto& o : a) for (auto& b : o) if (b) b->reset();
    engage_ = 0;
    sub_.reset(); res_.reset(); tracker_.reset();
    for (int c = 0; c < 2; ++c) { dryDelay_[c].reset(); auxDelay_[c].reset(); cutHp_[c].reset(); monoHp_[c].reset(); }
    for (auto& ch : kIn_) for (auto& k : ch) k.z1 = k.z2 = 0.f;
    for (auto& ch : kOut_) for (auto& k : ch) k.z1 = k.z2 = 0.f;
    msIn_ = msOut_ = 0.f; agDb_ = 0.f;
    for (auto* s : {&mix_, &outGain_, &resMix_, &glowSend_, &glowMix_, &subL_, &sub2L_, &phantom_}) s->snap();
    sleeping_ = false; silent_ = 0; lastWetPeak_ = 1.f; sleepFrames_ = 0;
    limGain_ = 1.f; glowHold_ = 0; t_ = 0; frameCount_ = 0; frame_ = {}; prevMask_ = 0.f; bassEnv_ = 0.f;
}

void Engine::setParams(const Params& p) {
    p_ = p;
    for (int b = 0; b < kMaxBands; ++b) {
        auto& s = band_[b]; const auto& q = p.band[(size_t)b];
        s.attack.t = std::clamp(q.attack + p.punch, -1.f, 1.f);
        s.sustain.t = std::clamp(q.sustain + p.tail, -1.f, 1.f);
        s.amount.t = q.harmonics; s.drive.t = q.drive; s.driveMix.t = q.driveMix;
    }
    mix_.t = p.mix; outGain_.t = dbToGain(std::clamp(p.outDb, -60.f, 24.f)); if (p.outDb == 0.f) outGain_.t = 1.f;
    resMix_.t = p.resMix; glowSend_.t = p.glowSend; glowMix_.t = p.glowMix;
    subL_.t = p.subLevel; sub2L_.t = p.sub2Level; phantom_.t = p.phantom;
    glowSize_.store(p.glowSize, std::memory_order_relaxed); glowDecay_.store(p.glowDecay, std::memory_order_relaxed);
    glowDamp_.store(p.glowDamp, std::memory_order_relaxed); glowPre_.store(p.glowPre, std::memory_order_relaxed);
    if (!prepared_) {
        for (auto& b : band_) for (auto* s : {&b.attack, &b.sustain, &b.amount, &b.drive, &b.driveMix}) s->snap();
        for (auto* s : {&mix_, &outGain_, &resMix_, &glowSend_, &glowMix_, &subL_, &sub2L_, &phantom_}) s->snap();
    }
}

void Engine::setTimes(BandState& b, float speed, float scale) {
    if (speed == b.lastSpeed && scale == b.lastScale) return;
    b.lastSpeed = speed; b.lastScale = scale;
    const double slow = 0.005 * std::pow(10.0, std::clamp((double)speed, 0.0, 1.0)) * scale;
    for (auto& d : b.det) {
        d.aSlow = coefFor(fs_, slow);
        d.aRel = coefFor(fs_, 0.04 * scale);
        d.aLong = coefFor(fs_, 0.6 * scale);
        d.aGain = coefFor(fs_, 0.0003 * scale);
    }
}

void Engine::process(float* L, float* R, int n, const NoteEvent* notes, int nNotes) {
    if (!prepared_ || n <= 0) return;
    fr::ScopedNoDenormals noDenormals;          // decaying filter states must not fall into denormals
    for (int i = 0; i < nNotes; ++i) { if (notes[i].on) res_.noteOn(notes[i].note); else res_.noteOff(notes[i].note); }
    constexpr float kSleepLevel = 1e-6f;   // -120 dBFS
    float peak = 0.f;
    for (int i = 0; i < n; ++i) peak = std::max(peak, std::max(std::fabs(L[i]), std::fabs(R[i])));
    if (peak > kSleepLevel) { silent_ = 0; sleeping_ = false; }
    else {
        silent_ += n;
        if (!sleeping_ && silent_ > (long long)(0.25 * fs_) + latency() && lastWetPeak_ < kSleepLevel) sleeping_ = true;
    }
    if (sleeping_) {
        // Silence in, silence out (the dry path has been silent for longer than the latency). The meters fall and
        // the track view keeps scrolling.
        std::fill_n(L, n, 0.f); std::fill_n(R, n, 0.f);
        meters.inPeak.store(0.f, std::memory_order_relaxed); meters.outPeak.store(0.f, std::memory_order_relaxed);
        meters.resonance.store(0.f, std::memory_order_relaxed); meters.bass.store(0.f, std::memory_order_relaxed);
        for (sleepFrames_ += n; sleepFrames_ >= 128; sleepFrames_ -= 128) track.push(TrackFrame{});
        return;
    }
    float wet = 0.f;
    for (int off = 0; off < n; off += kChunk) {
        const int c = std::min(kChunk, n - off);
        processChunk(L + off, R + off, c);
        for (int i = 0; i < c; ++i) wet = std::max(wet, std::max(std::fabs(L[off + i]), std::fabs(R[off + i])));
    }
    lastWetPeak_ = wet;
}

void Engine::processChunk(float* L, float* R, int n) {
    const Params& p = p_;
    const int bands = std::clamp(p.bands, 1, kMaxBands);
    const bool ms = p.stereo == StereoMode::MidSide, linked = p.stereo == StereoMode::Stereo;
    const bool linear = p.crossover == CrossoverType::LinearPhase;
    const int look = (int)std::lround(kLookaheadMs[std::clamp(p.lookahead, 0, 3)] * 0.001 * fs_);
    const int oi = osIndex();
    const int osLat = linear ? resamplerLatency(kOversampleFactors[oi], true) : 0;
    const int total = latencyFor(p);
    const float a20 = coefFor(fs_, 0.02);

    // Input meters and the delay-matched dry signal.
    float inPk = 0.f;
    for (int i = 0; i < n; ++i) {
        inPk = std::max(inPk, std::max(std::fabs(L[i]), std::fabs(R[i])));
        dry_[0][(size_t)i] = dryDelay_[0].tick(L[i], total);
        dry_[1][(size_t)i] = dryDelay_[1].tick(R[i], total);
    }
    // Mid/side: process (M, S); the side passes every stage at neutral settings so its delay matches.
    if (ms) for (int i = 0; i < n; ++i) { const float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]); L[i] = m; R[i] = s; }

    float* const bandPtrs[kMaxBands][2] = {{bandBuf_[0][0].data(), bandBuf_[0][1].data()},
                                           {bandBuf_[1][0].data(), bandBuf_[1][1].data()},
                                           {bandBuf_[2][0].data(), bandBuf_[2][1].data()}};
    xover_.process(L, R, n, p, bandPtrs);

    // Harmonic units: which set, and whether the oversampled path is engaged.
    bool want = false;
    for (int b = 0; b < bands; ++b) {
        const auto& q = p.band[(size_t)b];
        if (q.harmonics > 0.f || q.drive > 0.f || band_[b].amount.v > 0.f || band_[b].drive.v > 0.f) want = true;
    }
    if (harmLin_ != (int)linear || harmOs_ != oi) {
        harmLin_ = (int)linear; harmOs_ = oi;
        for (int b = 0; b < kMaxBands; ++b) harm_[harmLin_][harmOs_][b]->reset();
        engage_ = 0;
    }
    const int engLen = std::max(1, (int)(0.01 * fs_));

    for (int c = 0; c < 2; ++c) { std::fill_n(tSum_[c].data(), n, 0.f); std::fill_n(bSum_[c].data(), n, 0.f); std::fill_n(wet_[c].data(), n, 0.f); }
    std::fill_n(lowMono_.data(), n, 0.f);
    std::fill_n(mono_.data(), n, 0.f);
    float maskMax = 0.f, gainLoud = 0.f, loudest = -1.f;
    const float knee = 12.f - 10.f * std::clamp(p.sensitivity, 0.f, 1.f);

    for (int b = 0; b < bands; ++b) {
        BandState& bs = band_[b];
        const BandParams& q = p.band[(size_t)b];
        setTimes(bs, q.speed, bandScale(b, bands));
        const int nDet = linked ? 1 : 2;
        const float lowEdge = b == 0 ? 40.f : b == 1 ? p.xoverLow : p.xoverHigh;
        const int holdLen = (int)(1.1 * fs_ / lowEdge);
        // Smoothed controls for this band (shared by the channels).
        float att[kChunk], sus[kChunk];
        for (int i = 0; i < n; ++i) {
            att[i] = bs.attack.next(a20); sus[i] = bs.sustain.next(a20);
            amt_[(size_t)i] = bs.amount.next(a20); drv_[(size_t)i] = bs.drive.next(a20); dmx_[(size_t)i] = bs.driveMix.next(a20);
        }
        const bool shaping = att[0] != 0.f || att[n - 1] != 0.f || sus[0] != 0.f || sus[n - 1] != 0.f;
        // Detection and gain, per detector.
        for (int d = 0; d < nDet; ++d) {
            Detector& D = bs.det[d];
            const float* xa = bandBuf_[b][linked ? 0 : d].data();
            const float* xb = bandBuf_[b][1].data();
            float* g = gainArr_[b][d].data();
            float bandMask = 0.f;
            for (int i = 0; i < n; ++i) {
                float lv = std::fabs(xa[i]);
                if (linked) lv = std::max(lv, std::fabs(xb[i]));
                else if (ms && d == 1) lv = 0.f;      // the side is not shaped
                // e1: the level, instant attack, held for one period of the band's lowest frequency, then released.
                // e2 follows e1 with the slow attack (Speed); e3 follows e1 with a long release. Following e1 rather
                // than the raw level keeps waveform ripple out of the differences.
                if (lv >= D.e1) { D.e1 = lv; D.hold = holdLen; }
                else if (D.hold > 0) --D.hold;
                else D.e1 = lv + D.aRel * (D.e1 - lv);
                D.e2 = D.e1 > D.e2 ? D.e1 + D.aSlow * (D.e2 - D.e1) : D.e1 + D.aRel * (D.e2 - D.e1);
                D.e3 = D.e1 > D.e3 ? D.e1 : D.e1 + D.aLong * (D.e3 - D.e1);
                const float l1 = std::max(toDb(D.e1), -80.f), l2 = std::max(toDb(D.e2), -80.f), l3 = std::max(toDb(D.e3), -80.f);
                const float onset = std::max(0.f, l1 - l2 - 0.5f), tail = std::max(0.f, l3 - l1 - 0.5f);
                const float m = smoothstep(onset / knee);
                bs.mask[d] = m;
                bandMask = std::max(bandMask, m);
                if (shaping) {
                    const float gdb = std::clamp(att[i] * onset, -24.f, 15.f) + std::clamp(sus[i] * 1.5f * tail, -24.f, 24.f);
                    const float target = dbToGain(gdb);
                    D.g = target + D.aGain * (D.g - target);
                    g[i] = D.g;
                } else {
                    D.g = 1.f + D.aGain * (D.g - 1.f);
                    if (std::fabs(D.g - 1.f) < 1e-7f) D.g = 1.f;
                    g[i] = D.g;
                }
                maskArr_[b][d][(size_t)i] = m;
                env2Arr_[b][d][(size_t)i] = D.e2;
            }
            maskMax = std::max(maskMax, bandMask);
            meters.bandMask[b].store(bandMask, std::memory_order_relaxed);
            meters.bandGainDb[b].store(toDb(D.g), std::memory_order_relaxed);
            if (D.e1 > loudest) { loudest = D.e1; gainLoud = toDb(D.g); }
        }
        // Shape and split, per channel.
        HarmControl hc;
        const int mo = maxOrder(b);
        hc.amount = amt_.data(); hc.drive = drv_.data(); hc.driveMix = dmx_.data();
        for (int k = 0; k < kOrders; ++k) hc.weights[(size_t)k] = k < mo ? q.orders[(size_t)k] * (q.invert[(size_t)k] ? -1.f : 1.f) : 0.f;
        hc.weights[0] = q.orders[0];
        hc.bank = amt_[0] > 0.f || amt_[(size_t)n - 1] > 0.f;
        hc.driveOn = drv_[0] > 0.f || drv_[(size_t)n - 1] > 0.f;
        hc.curve = q.curve; hc.bias = q.bias; hc.tone = q.tone;
        hc.hasOther = q.source != HarmSource::Both;
        for (int c = 0; c < 2; ++c) {
            const bool side = ms && c == 1;
            const int d = linked ? 0 : c;
            const float* x = bandBuf_[b][c].data();
            const float* g = gainArr_[b][d].data();
            const float* mk = maskArr_[b][d].data();
            float* src = src_[c].data(); float* oth = oth_[c].data();
            Adaa& clipper = bs.clip[c];
            const float clipAmt = q.clip;
            const float ceilK = 1.4f * dbToGain(18.f * (1.f - clipAmt));
            for (int i = 0; i < n; ++i) {
                const float xd = look > 0 ? bs.look[c].tick(x[i], look) : x[i];
                if (!ms || c == 0) {                     // mono: (L + R) / 2, or M
                    const float w = ms ? 1.f : 0.5f;
                    if (b == 0) lowMono_[(size_t)i] += w * xd;
                    mono_[(size_t)i] += w * xd;
                }
                if (side) { src[i] = xd; oth[i] = 0.f; continue; }
                const float y = g[i] == 1.f ? xd : xd * g[i];
                float tr = mk[i] * y;
                const float body = y - tr;
                if (clipAmt > 0.f) {
                    const float ceil = std::max(1e-6f, env2Arr_[b][d][(size_t)i] * ceilK);
                    tr = ceil * clipper.tick(tr / ceil);
                }
                tSum_[c][(size_t)i] += tr; bSum_[c][(size_t)i] += body;
                const float shaped = clipAmt > 0.f ? tr + body : y;
                switch (q.source) {
                    case HarmSource::Both: src[i] = shaped; oth[i] = 0.f; break;
                    case HarmSource::Transient: src[i] = tr; oth[i] = body; break;
                    case HarmSource::Body: src[i] = body; oth[i] = tr; break;
                }
            }
            // Direct path (delayed in linear mode to match the oversampling latency).
            float* dir = direct_[c].data();
            for (int i = 0; i < n; ++i) {
                const float v = src[i] + oth[i];
                dir[i] = osLat > 0 ? bs.osDelay[c].tick(v, osLat) : v;
            }
        }
        float* const outs[2] = {bandOut_[b][0].data(), bandOut_[b][1].data()};
        if (want || engage_ > 0) {
            HarmControl hcc = hc;
            hcc.active[1] = !ms;                         // mid/side: the side passes untouched
            const float* const srcs[2] = {src_[0].data(), src_[1].data()};
            const float* const oths[2] = {oth_[0].data(), oth_[1].data()};
            harm_[harmLin_][harmOs_][b]->process(srcs, oths, outs, n, hcc);
            if (engage_ < engLen || !want) {
                for (int c = 0; c < 2; ++c) {
                    int e = engage_;
                    const float* dir = direct_[c].data();
                    for (int i = 0; i < n; ++i) {
                        e = std::clamp(e + (want ? 1 : -1), 0, engLen);
                        outs[c][i] = dir[i] + ((float)e / (float)engLen) * (outs[c][i] - dir[i]);
                    }
                }
            }
        } else {
            for (int c = 0; c < 2; ++c) std::copy(direct_[c].data(), direct_[c].data() + n, outs[c]);
        }
        if (q.mute) for (int c = 0; c < 2; ++c) std::fill_n(outs[c], n, 0.f);
    }
    // Advance the engage ramp once per chunk (the per-band loops above used the same start value).
    if (want || engage_ > 0) engage_ = std::clamp(engage_ + (want ? n : -n), 0, engLen);

    // Band sum, back to L/R.
    for (int c = 0; c < 2; ++c) for (int b = 0; b < bands; ++b) { const float* o = bandOut_[b][c].data(); for (int i = 0; i < n; ++i) wet_[c][(size_t)i] += o[i]; }
    if (ms) {
        for (int i = 0; i < n; ++i) {
            const float m = wet_[0][(size_t)i], s = wet_[1][(size_t)i];
            wet_[0][(size_t)i] = m + s; wet_[1][(size_t)i] = m - s;
            const float tm = tSum_[0][(size_t)i]; tSum_[1][(size_t)i] = tm;
            const float bm = bSum_[0][(size_t)i]; bSum_[1][(size_t)i] = bm;
        }
    }

    // Phantom cut (a high-pass under the generated harmonics) and mono below, on the band sum.
    const float cutHz = p.phantomCut > 0.f ? 20.f + (std::clamp(p.subRange, 40.f, 400.f) - 20.f) * std::clamp(p.phantomCut, 0.f, 1.f) : 0.f;
    if (cutHz > 0.f) {
        if (cutHz != cutHz_) { cutHz_ = cutHz; for (auto& f : cutHp_) f.set(fs_, cutHz, 1.41421356); }
        for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) wet_[c][(size_t)i] = cutHp_[c].hp(wet_[c][(size_t)i]);
    }
    if (p.monoBelow > 0.f) {
        if (p.monoBelow != monoHz_) { monoHz_ = p.monoBelow; for (auto& f : monoHp_) f.set(fs_, monoHz_, 1.41421356); }
        for (int i = 0; i < n; ++i) {
            const float m = 0.5f * (wet_[0][(size_t)i] + wet_[1][(size_t)i]);
            float s = 0.5f * (wet_[0][(size_t)i] - wet_[1][(size_t)i]);
            s = monoHp_[1].hp(monoHp_[0].hp(s));
            wet_[0][(size_t)i] = m + s; wet_[1][(size_t)i] = m - s;
        }
    }

    // Sub / phantom (mono) from the delayed input, the resonance from the transients (or the shaped signal).
    const bool subOn = p.subLevel > 0.f || p.sub2Level > 0.f || p.phantom > 0.f || subL_.v > 0.f || sub2L_.v > 0.f || phantom_.v > 0.f;
    std::fill_n(aux_[0].data(), n, 0.f); std::fill_n(aux_[1].data(), n, 0.f);
    if (subOn) {
        // The generator runs at the block's target levels; the smoothed levels scale it during a ramp.
        sub_.process(mono_.data(), subOut_.data(), n, p);
        const float tgt = p.subLevel + p.sub2Level + p.phantom;
        for (int i = 0; i < n; ++i) {
            const float sm = subL_.next(a20) + sub2L_.next(a20) + phantom_.next(a20);
            const float v = tgt > 0.f ? subOut_[(size_t)i] * (sm / tgt) : 0.f;
            subOut_[(size_t)i] = v;
            aux_[0][(size_t)i] += v; aux_[1][(size_t)i] += v;
        }
    } else {
        std::fill_n(subOut_.data(), n, 0.f);
    }
    const bool resOn = p.resMix > 0.f || resMix_.v > 0.f || (p.glowSend > 0.f && p.glowSource == GlowSource::Resonance);
    if (p.pitch == PitchSource::Track && resOn) tracker_.push(lowMono_.data(), n);
    meters.trackedHz.store(tracker_.voiced() ? tracker_.hz() : 0.f, std::memory_order_relaxed);
    if (resOn) {
        float exc[kChunk];
        for (int i = 0; i < n; ++i)
            exc[i] = p.resInput == ResInput::Transients ? 0.5f * (tSum_[0][(size_t)i] + tSum_[1][(size_t)i]) : 0.5f * (wet_[0][(size_t)i] + wet_[1][(size_t)i]);
        res_.process(exc, resL_.data(), resR_.data(), n, p, tracker_.hz());
        float e = 0.f;
        for (int i = 0; i < n; ++i) {
            const float m = resMix_.next(a20);
            aux_[0][(size_t)i] += m * resL_[(size_t)i]; aux_[1][(size_t)i] += m * resR_[(size_t)i];
            e = std::max(e, std::fabs(resL_[(size_t)i]) + std::fabs(resR_[(size_t)i]));
        }
        meters.resonance.store(0.5f * e, std::memory_order_relaxed);
        const int cnt = p.resMode == ResMode::Comb ? 0 : res_.activeCount();
        for (int k = 0; k < cnt; ++k) meters.resHz[k].store(res_.frequency(k), std::memory_order_relaxed);
        meters.resCount.store(cnt, std::memory_order_relaxed);
    } else {
        meters.resCount.store(0, std::memory_order_relaxed);
        std::fill_n(resL_.data(), n, 0.f); std::fill_n(resR_.data(), n, 0.f);
        meters.resonance.store(0.f, std::memory_order_relaxed);
    }
    // In linear mode the aux signals skipped the oversampling: delay them to match.
    if (osLat > 0) {
        for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) aux_[c][(size_t)i] = auxDelay_[c].tick(aux_[c][(size_t)i], osLat);
    }
    for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) wet_[c][(size_t)i] += aux_[c][(size_t)i];

    // Auto-gain: K-weighted loudness of the dry and wet signals.
    for (int i = 0; i < n; ++i) {
        float pi = 0.f, po = 0.f;
        for (int c = 0; c < 2; ++c) {
            const float a = kIn_[c][1].tick(kIn_[c][0].tick(dry_[c][(size_t)i]));
            const float b = kOut_[c][1].tick(kOut_[c][0].tick(wet_[c][(size_t)i]));
            pi += a * a; po += b * b;
        }
        msIn_ = pi + agA_ * (msIn_ - pi); msOut_ = po + agA_ * (msOut_ - po);
        if (p.autoGain) {
            // No hard threshold: near silence the ratio tends to 1 and the update slows, both continuously, so tiny
            // differences in the signal stay tiny in the gain.
            const float target = std::clamp(10.f * std::log10((msIn_ + 1e-8f) / (msOut_ + 1e-8f)), -12.f, 12.f);
            const float w = msIn_ / (msIn_ + 1e-6f), a = 1.f - (1.f - agGainA_) * w;
            agDb_ = target + a * (agDb_ - target);
        } else {
            agDb_ = agGainA_ * agDb_;
            if (std::fabs(agDb_) < 1e-5f) agDb_ = 0.f;
        }
        if (agDb_ != 0.f) { const float g = dbToGain(agDb_); wet_[0][(size_t)i] *= g; wet_[1][(size_t)i] *= g; }
    }
    meters.autoGainDb.store(agDb_, std::memory_order_relaxed);

    // Afterglow.
    const bool glowWanted = p.glowSend > 0.f || glowSend_.v > 0.f;
    if (glowWanted) glowHold_ = (int)((std::clamp(p.glowDecay, 0.2f, 30.f) * 1.5f + 0.5f) * fs_);
    if (glowWanted || glowHold_ > 0) {
        glowHold_ -= n;
        for (int i = 0; i < n; ++i) {
            const float s = glowSend_.next(a20);
            float l, r;
            switch (p.glowSource) {
                case GlowSource::Resonance: l = resL_[(size_t)i]; r = resR_[(size_t)i]; break;
                case GlowSource::Transients: l = tSum_[0][(size_t)i]; r = tSum_[1][(size_t)i]; break;
                default: l = wet_[0][(size_t)i]; r = wet_[1][(size_t)i]; break;
            }
            glowL_[(size_t)i] = l * s; glowR_[(size_t)i] = r * s;
        }
        glow_->process(glowL_.data(), glowR_.data(), glowL_.data(), glowR_.data(), n, 0.55f, 1.f, 1.0, 0.6);
        for (int i = 0; i < n; ++i) {
            const float m = glowMix_.next(a20);
            wet_[0][(size_t)i] += m * glowL_[(size_t)i]; wet_[1][(size_t)i] += m * glowR_[(size_t)i];
        }
    } else {
        std::fill_n(glowL_.data(), n, 0.f); std::fill_n(glowR_.data(), n, 0.f);
    }

    // Mix, listen, output.
    float outPk = 0.f, bass = bassEnv_;
    const float bassA = coefFor(fs_, 0.25);
    for (int i = 0; i < n; ++i) {
        const float m = mix_.next(a20), og = outGain_.next(a20);
        float l = wet_[0][(size_t)i], r = wet_[1][(size_t)i];
        const float dl = dry_[0][(size_t)i], dr = dry_[1][(size_t)i];
        switch (p.listen) {
            case Listen::All: if (m != 1.f) { l = dl + m * (l - dl); r = dr + m * (r - dr); } break;
            case Listen::Delta: l -= dl; r -= dr; break;
            case Listen::Transients: l = tSum_[0][(size_t)i]; r = tSum_[1][(size_t)i]; break;
            case Listen::Body: l = bSum_[0][(size_t)i]; r = bSum_[1][(size_t)i]; break;
            case Listen::Band1: case Listen::Band2: case Listen::Band3: {
                const int b = (int)p.listen - (int)Listen::Band1;
                if (b < bands) {
                    l = bandOut_[b][0][(size_t)i]; r = bandOut_[b][1][(size_t)i];
                    if (ms) { const float mm = l, ss = r; l = mm + ss; r = mm - ss; }
                } else { l = r = 0.f; }
                break;
            }
            case Listen::Sub: l = r = subOut_[(size_t)i]; break;
            case Listen::Resonance: l = resL_[(size_t)i]; r = resR_[(size_t)i]; break;
            case Listen::Afterglow: l = glowL_[(size_t)i]; r = glowR_[(size_t)i]; break;
        }
        if (og != 1.f) { l *= og; r *= og; }
        if (p.clipGuard) {
            constexpr float thr = 0.891f;            // -1 dBFS
            const float pk = std::max(std::fabs(l), std::fabs(r));
            const float target = pk > thr ? thr / pk : 1.f;
            limGain_ = target < limGain_ ? target : target + (limGain_ - target) * limRel_;
            l *= limGain_; r *= limGain_;
        }
        L[i] = l; R[i] = r;
        outPk = std::max(outPk, std::max(std::fabs(l), std::fabs(r)));
        const float lowAbs = std::fabs(lowMono_[(size_t)i]);
        bass = lowAbs > bass ? lowAbs : lowAbs + bassA * (bass - lowAbs);
    }
    bassEnv_ = bass;
    {
        int pos = meters.scopePos.load(std::memory_order_relaxed);
        for (int i = 0; i < n; ++i) { meters.scope[pos].store(0.5f * (L[i] + R[i]), std::memory_order_relaxed); pos = (pos + 1) % Meters::kScopeSize; }
        meters.scopePos.store(pos, std::memory_order_release);
    }
    meters.limiterGain.store(limGain_, std::memory_order_relaxed);
    meters.inPeak.store(inPk, std::memory_order_relaxed);
    meters.outPeak.store(outPk, std::memory_order_relaxed);
    meters.bass.store(std::min(1.f, bass * 2.f), std::memory_order_relaxed);

    // Track view frames.
    for (int i = 0; i < n; ++i) {
        const float mi = 0.5f * (dry_[0][(size_t)i] + dry_[1][(size_t)i]), mo = 0.5f * (L[i] + R[i]);
        if (frameCount_ == 0) { frame_ = {}; frame_.inMin = frame_.inMax = mi; frame_.outMin = frame_.outMax = mo; }
        frame_.inMin = std::min(frame_.inMin, mi); frame_.inMax = std::max(frame_.inMax, mi);
        frame_.outMin = std::min(frame_.outMin, mo); frame_.outMax = std::max(frame_.outMax, mo);
        if (++frameCount_ >= 128) {
            frame_.mask = maskMax; frame_.gainDb = gainLoud;
            frame_.onset = maskMax > 0.5f && prevMask_ <= 0.5f;
            prevMask_ = maskMax;
            track.push(frame_);
            frameCount_ = 0;
        }
    }
    t_ += n;
}

}  // namespace muon
