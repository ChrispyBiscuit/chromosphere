#include "space/space_engine.h"
#include "common/denormal.h"
#include "common/rng.h"
#include "conv/ir_tools.h"
#include <algorithm>
#include <cmath>

namespace cs {
namespace {
constexpr double kPi = 3.14159265358979323846;

// Coefficient p of the one-pole low-pass y = (1 - p) x + p y' whose gain at f is g (0 < g <= 1).
float onePoleForGain(double g, double f, double fs) {
    if (g >= 0.9999) return 0.f;
    const double w = 2.0 * kPi * std::min(f, 0.45 * fs) / fs;
    double lo = 0, hi = 0.999;
    for (int it = 0; it < 50; ++it) {
        const double p = 0.5 * (lo + hi);
        const double mag = (1.0 - p) / std::sqrt(1.0 - 2.0 * p * std::cos(w) + p * p);
        (mag > g ? lo : hi) = p;
    }
    return (float)(0.5 * (lo + hi));
}
}  // namespace

FdnConfig SpaceEngine::fdnConfig(const SpaceParams& p, const RoomRender& r) {
    FdnConfig fc;
    fc.lines = p.lines; fc.size = 1.0; fc.seed = p.seed;
    fc.minMs = r.fdnMinMs; fc.maxMs = r.fdnMaxMs;
    fc.t60[0] = spaceT60At(r.acoustics, 125.0); fc.t60[1] = spaceT60At(r.acoustics, 1000.0); fc.t60[2] = spaceT60At(r.acoustics, 8000.0);
    return fc;
}

DecaySet SpaceEngine::designDecay(const SpaceParams& p, double fs) {
    RoomRender r;
    r.acoustics = analyseRoom(p.room);
    spaceFdnSpread(r.acoustics, r.fdnMinMs, r.fdnMaxMs);
    const RoomAcoustics a = r.acoustics;
    return Fdn::designDecaySet(Fdn::computeDelays(fdnConfig(p, r), fs), T60Curve([a](double f) { return spaceT60At(a, f); }), fs);
}

void SpaceEngine::configure(const SpaceParams& p, double fs, int maxBlock) {
    fs_ = fs; maxBlock_ = std::max(1, maxBlock);
    render_ = renderRoom(p.room, fs);
    const RoomRender& R = render_;
    conv_.configure(R.early);
    pre_.configure((int)std::lround(p.preDelayMs * fs / 1000.0));

    // Late field.
    const FdnConfig fc = fdnConfig(p, R);
    fdn_.configure(fc, fs);
    const RoomAcoustics a = R.acoustics;
    const DecaySet ds = Fdn::designDecaySet(fdn_.delays(), T60Curve([a](double f) { return spaceT60At(a, f); }), fs);
    fdn_.setDecay(ds, 0);
    feed_.configure((int)std::lround(std::max(0.0, R.tailStartMs - R.fdnMinMs) * fs / 1000.0));
    const double phi = 0.5 * std::asin(std::clamp(R.tailCoherence, 0.0, 1.0));
    mixA_ = (float)std::cos(phi); mixB_ = (float)std::sin(phi);
    calibrateTail(p, ds);

    // Tail high-pass where the modes take over: 4th-order Linkwitz-Riley (two Butterworth sections).
    highPass_ = R.modeCrossoverHz > 0;
    if (highPass_) {
        const double w = 2.0 * kPi * R.modeCrossoverHz / fs, alpha = std::sin(w) / (2.0 * std::sqrt(0.5)), cw = std::cos(w), a0 = 1.0 + alpha;
        for (Biquad& b : hp_) {
            b = Biquad();
            b.b0 = (float)((1.0 + cw) / 2.0 / a0); b.b1 = (float)(-(1.0 + cw) / a0); b.b2 = b.b0;
            b.a1 = (float)(-2.0 * cw / a0); b.a2 = (float)((1.0 - alpha) / a0);
        }
    }
    // Modes.
    nModes_ = (int)R.modes.size();
    for (auto* v : {&mRe_, &mIm_, &mCr_, &mCi_, &mInL_, &mInR_, &mOutL_, &mOutR_}) v->assign((size_t)nModes_, 0.f);
    for (int i = 0; i < nModes_; ++i) {
        const RoomMode& m = R.modes[(size_t)i];
        const double r = std::exp(-m.decay / fs), w = 2.0 * kPi * m.freq / fs;
        mCr_[(size_t)i] = (float)(r * std::cos(w)); mCi_[(size_t)i] = (float)(r * std::sin(w));
        mInL_[(size_t)i] = m.in[0]; mInR_[(size_t)i] = m.in[1]; mOutL_[(size_t)i] = m.out[0]; mOutR_[(size_t)i] = m.out[1];
    }
    // Direct paths.
    int maxD = 0;
    for (int k = 0; k < 4; ++k) {
        dirDelay_[k] = (int)std::lround(R.direct[k].delaySec * fs);
        maxD = std::max(maxD, dirDelay_[k]);
        dirGain_[k] = R.direct[k].gain;
        dirLp_[k] = onePoleForGain(R.direct[k].airAt8k, 8000.0, fs);
        dirState_[k] = 0.f;
    }
    histLen_ = maxD + 1; histPos_ = 0;
    histL_.assign((size_t)histLen_, 0.f); histR_.assign((size_t)histLen_, 0.f);

    for (auto* v : {&gL_, &gR_, &xL_, &xR_, &eL_, &eR_, &fL_, &fR_, &tL_, &tR_}) v->assign((size_t)maxBlock_, 0.f);
    er_.set(1.f, 0); tail_.set(1.f, 0); direct_.set(0.f, 0); gate_.set(1.f, 0);
}

// Gain and two shelves that make the FDN's band powers (125 Hz, 1 kHz, 8 kHz) in a window after the early
// part equal the room's statistical late energy there (one source, per microphone).
void SpaceEngine::calibrateTail(const SpaceParams& p, const DecaySet& ds) {
    const RoomRender& R = render_;
    Fdn probe;
    const FdnConfig fc = fdnConfig(p, R);
    probe.configure(fc, fs_);
    const RoomAcoustics a = R.acoustics;
    probe.setDecay(ds, 0);
    const double feedMs = std::max(0.0, R.tailStartMs - R.fdnMinMs);
    const double w0 = R.earlyMs + 0.5 * R.fdnMaxMs + 20.0;
    const double w1 = w0 + std::clamp(0.15 * a.t60[4] * 1000.0, 20.0, 120.0);
    const size_t len = (size_t)((w1 - feedMs) * fs_ / 1000.0) + 1;
    std::vector<float> in(len, 0.f), zero(len, 0.f), l(len), r(len);
    in[0] = 1.f;
    probe.process(in.data(), zero.data(), l.data(), r.data(), (int)len);
    const size_t p0 = (size_t)((w0 - feedMs) * fs_ / 1000.0), p1 = len;
    // Band-power fraction of unit white noise, so the broadband per-sample target converts to band power.
    std::vector<float> noise(16384); Rng rng(77);
    for (float& v : noise) v = (float)rng.gauss();
    const double centres[3] = {125.0, 1000.0, 8000.0};
    double g[3];
    for (int b = 0; b < 3; ++b) {
        const int band = b == 0 ? 1 : b == 1 ? 4 : 7;
        const double frac = bandPower(noise, centres[b], fs_, 4096, noise.size());
        double mean = 0; const int steps = 32;
        for (int k = 0; k < steps; ++k) {
            const double t = (w0 + (w1 - w0) * (k + 0.5) / steps) / 1000.0;
            mean += std::exp(-13.815510557964274 * t / a.t60[band]);
        }
        mean /= steps;
        const double target = R.lateEnergy0[band] * R.norm * R.norm * mean * frac;
        const double have = 0.5 * (bandPower(l, centres[b], fs_, p0, p1) + bandPower(r, centres[b], fs_, p0, p1));
        g[b] = have > 0 && target > 0 ? std::clamp(std::sqrt(target / have), 1e-4, 1e4) : 0.0;
    }
    tailMatch_ = (float)g[1];
    const double lo = g[1] > 0 ? std::clamp(g[0] / g[1], 0.03, 30.0) : 1.0, hi = g[1] > 0 ? std::clamp(g[2] / g[1], 0.03, 30.0) : 1.0;
    const Shelf sl = makeShelf(true, lo, 400.0, fs_), sh = makeShelf(false, hi, 3000.0, fs_);
    shLow_[0] = (float)sl.b0; shLow_[1] = (float)sl.b1; shLow_[2] = (float)sl.a1;
    shHigh_[0] = (float)sh.b0; shHigh_[1] = (float)sh.b1; shHigh_[2] = (float)sh.a1;
}

void SpaceEngine::process(const float* inL, const float* inR, float* outL, float* outR, int n) {
    ScopedNoDenormals guard;
    for (int off = 0; off < n; off += maxBlock_) {
        const int c = std::min(maxBlock_, n - off);
        chunk(inL + off, inR + off, outL + off, outR + off, c);
    }
}

void SpaceEngine::chunk(const float* inL, const float* inR, float* outL, float* outR, int n) {
    // Gate (freeze mutes the reverb's input), pre-delay.
    for (int i = 0; i < n; ++i) { const float g = gate_.next(); gL_[(size_t)i] = inL[i] * g; gR_[(size_t)i] = inR[i] * g; }
    pre_.process(gL_.data(), gR_.data(), xL_.data(), xR_.data(), n);
    // Early part.
    conv_.process(xL_.data(), xR_.data(), eL_.data(), eR_.data(), n);
    // Late field.
    feed_.process(xL_.data(), xR_.data(), fL_.data(), fR_.data(), n);
    fdn_.process(fL_.data(), fR_.data(), tL_.data(), tR_.data(), n);
    float* tc[2] = {tL_.data(), tR_.data()};
    for (int ch = 0; ch < 2; ++ch) {
        float* t = tc[ch];
        float s0 = shState_[ch][0], s1 = shState_[ch][1];
        for (int i = 0; i < n; ++i) {
            const float x = t[i];
            const float y = shLow_[0] * x + s0; s0 = shLow_[1] * x - shLow_[2] * y;
            const float z = shHigh_[0] * y + s1; s1 = shHigh_[1] * y - shHigh_[2] * z;
            t[i] = z * tailMatch_;
        }
        shState_[ch][0] = s0; shState_[ch][1] = s1;
        if (highPass_)
            for (Biquad& b : hp_) {
                float z1 = b.z1[ch], z2 = b.z2[ch];
                for (int i = 0; i < n; ++i) {   // transposed direct form II
                    const float x = t[i], y = b.b0 * x + z1;
                    z1 = b.b1 * x - b.a1 * y + z2; z2 = b.b2 * x - b.a2 * y;
                    t[i] = y;
                }
                b.z1[ch] = z1; b.z2[ch] = z2;
            }
    }
    for (int i = 0; i < n; ++i) {        // late-field coherence of the microphone pair
        const float l = tL_[(size_t)i], r = tR_[(size_t)i];
        tL_[(size_t)i] = mixA_ * l + mixB_ * r; tR_[(size_t)i] = mixB_ * l + mixA_ * r;
    }
    // Modes, added to the tail.
    if (nModes_ > 0) {
        float* __restrict re = mRe_.data(); float* __restrict im = mIm_.data();
        const float* __restrict cr = mCr_.data(); const float* __restrict ci = mCi_.data();
        const float* __restrict il = mInL_.data(); const float* __restrict ir = mInR_.data();
        const float* __restrict ol = mOutL_.data(); const float* __restrict orr = mOutR_.data();
        const int M = nModes_;
        for (int i = 0; i < n; ++i) {
            const float xl = xL_[(size_t)i], xr = xR_[(size_t)i];
            float accL = 0.f, accR = 0.f;
            for (int k = 0; k < M; ++k) {
                const float x = il[k] * xl + ir[k] * xr;
                const float nr = re[k] * cr[k] - im[k] * ci[k] + x;
                const float ni = re[k] * ci[k] + im[k] * cr[k];
                re[k] = nr; im[k] = ni;
                accL += nr * ol[k]; accR += nr * orr[k];
            }
            tL_[(size_t)i] += accL; tR_[(size_t)i] += accR;
        }
    }
    // Sum with the direct paths.
    for (int i = 0; i < n; ++i) {
        histL_[(size_t)histPos_] = inL[i]; histR_[(size_t)histPos_] = inR[i];
        float d[2] = {0.f, 0.f};
        for (int k = 0; k < 4; ++k) {
            int rd = histPos_ - dirDelay_[k]; if (rd < 0) rd += histLen_;
            const float x = (k < 2 ? histL_ : histR_)[(size_t)rd] * dirGain_[k];
            dirState_[k] = (1.f - dirLp_[k]) * x + dirLp_[k] * dirState_[k];
            d[k & 1] += dirState_[k];
        }
        if (++histPos_ >= histLen_) histPos_ = 0;
        const float e = er_.next(), t = tail_.next(), dg = direct_.next();
        outL[i] = e * eL_[(size_t)i] + t * tL_[(size_t)i] + dg * d[0];
        outR[i] = e * eR_[(size_t)i] + t * tR_[(size_t)i] + dg * d[1];
    }
}

}  // namespace cs
