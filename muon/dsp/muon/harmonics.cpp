#include "harmonics.h"

namespace muon {

namespace {
Curve curveFor(DriveCurve c) {
    switch (c) {
        case DriveCurve::Soft: case DriveCurve::Tube: return Curve::Tanh;
        case DriveCurve::Tape: return Curve::Rational;
        case DriveCurve::Diode: return Curve::Clip;
        case DriveCurve::Fold: return Curve::Sine;
        case DriveCurve::Rectify: return Curve::Rectify;
    }
    return Curve::Tanh;
}
}  // namespace

void HarmonicBand::prepare(double fs, int osFactor, bool linear, int maxBlock) {
    up_.prepare(osFactor, linear, maxBlock);
    down_.prepare(osFactor, linear, maxBlock);
    const int os = up_.factor();
    fsHi_ = fs * os;
    for (auto& h : hil_) h.prepare(fs);
    in_.assign((size_t)maxBlock * kLanes, 0.f);
    hi_.assign((size_t)maxBlock * os * kLanes, 0.f);
    y_.assign((size_t)maxBlock * os * 2, 0.f);
    out_.assign((size_t)maxBlock * 2, 0.f);
    gain_.assign((size_t)maxBlock, 0.f); post_.assign((size_t)maxBlock, 0.f);
    for (auto* v : {&s_, &x_, &f_, &d_, &a_, &b_, &ctl_}) v->assign((size_t)maxBlock * os * 2 + 2, 0.f);
    for (auto& d : dc_) d.setHz(fsHi_, 5.0);
    for (auto& t : tilt_) t.setHz(fsHi_, 1000.0);
    reset();
}

void HarmonicBand::reset() {
    for (auto& h : hil_) h.reset();
    up_.reset(); down_.reset();
    for (int c = 0; c < 2; ++c) { adaa_[c].reset(); tape_[c].reset(); tilt_[c].reset(); dc_[c].reset(); }
    bankWasOn_ = false;
}

void HarmonicBand::process(const float* const src[2], const float* const oth[2], float* const out[2], int n, const HarmControl& c) {
    const int os = up_.factor(), N = n * os;
    // Interleave the base-rate lanes; the analytic signal only when the bank is on.
    if (c.bank && !bankWasOn_) for (auto& h : hil_) h.reset();
    bankWasOn_ = c.bank;
    for (int i = 0; i < n; ++i) {
        float* f = &in_[(size_t)i * kLanes];
        for (int ch = 0; ch < 2; ++ch) {
            f[ch] = src[ch][i];
            f[2 + ch] = c.hasOther ? oth[ch][i] : 0.f;
            if (c.bank) hil_[ch].process(src[ch][i], f[4 + ch], f[6 + ch]);
            else f[4 + ch] = f[6 + ch] = 0.f;
        }
    }
    up_.up(in_.data(), hi_.data(), n);

    const Curve cv = curveFor(c.curve);
    for (auto& a : adaa_) a.setCurve(cv);
    const float w1 = c.weights[0] - 1.f;
    if (c.driveOn && c.curve == DriveCurve::Tape) for (auto& t : tape_) t.setHz(fsHi_, 20000.0 * (1.0 - 0.75 * c.drive[n - 1]));
    const float tilt = 2.f * c.tone - 1.f, gLow = dbToGain(-6.f * tilt), gHigh = dbToGain(6.f * tilt);
    // Tube leans asymmetric (even harmonics) even at zero bias; Rectify uses bias as its half/full blend.
    const float bias = c.curve == DriveCurve::Rectify ? 0.f : c.curve == DriveCurve::Tube ? 0.35f + 0.65f * c.bias : c.bias;
    const float rectK = 0.5f * (c.bias + 1.f);
    const float fb = curveFf(cv, bias);
    const bool rect = c.curve == DriveCurve::Rectify, tape = c.curve == DriveCurve::Tape;
    if (c.driveOn) {
        for (int i = 0; i < n; ++i) {
            const float g = dbToGain(36.f * c.drive[i]);
            gain_[(size_t)i] = g;
            post_[(size_t)i] = rect ? 1.f / g : 1.f / std::sqrt(g);    // small signals rise with drive, peaks fall
        }
    }
    float wts[kOrders];
    for (int k = 0; k < kOrders; ++k) wts[k] = c.weights[(size_t)k];
    const int sh = os == 8 ? 3 : os == 4 ? 2 : os == 2 ? 1 : 0;   // high-rate index -> base-rate index
    const int K = 2 * N;                                           // interleaved L/R values at the high rate
    const float act[2] = {c.active[0] ? 1.f : 0.f, c.active[1] ? 1.f : 0.f};
    float* S = s_.data();
    float* A = a_.data(); float* Bq = b_.data(); float* C = ctl_.data();
    // Pass 0: deinterleave the source (and the analytic pair) into contiguous L/R pairs, and expand the per-sample
    // controls to the high rate, so the passes below are straight stride-1 loops that vectorise.
    for (int k = 0; k < K; ++k) S[k] = hi_[(size_t)(k >> 1) * kLanes + (k & 1)];
    // Pass 1: the order bank.
    if (c.bank) {
        for (int k = 0; k < K; ++k) {
            const float* f = &hi_[(size_t)(k >> 1) * kLanes];
            A[k] = f[4 + (k & 1)]; Bq[k] = f[6 + (k & 1)];
            C[k] = act[k & 1] * c.amount[k >> 1 >> sh];
        }
        for (int k = 0; k < K; ++k) {
            const float a = A[k], b = Bq[k], amp = std::sqrt(a * a + b * b);
            const float u = std::min(std::max(a / std::max(amp, 1e-20f), -1.f), 1.f);
            float t0 = 1.f, t1 = u, h = 0.f;
            for (int o = 1; o < kOrders; ++o) { const float t2 = 2.f * u * t1 - t0; h += wts[o] * t2; t0 = t1; t1 = t2; }
            h = amp > 1e-9f ? h * amp : 0.f;
            S[k] += C[k] * (w1 * S[k] + h);
        }
    }
    // Pass 2: drive. The curve's antiderivative for every value, then the ADAA differences against the previous
    // value of the same channel (carried across blocks), with the midpoint fallback where the step is too small.
    if (c.driveOn) {
        // X and F carry the previous block's last value per channel in their first two slots, so the ADAA difference
        // loop has no branch on the block edge.
        float* Xb = x_.data(); float* Fb = f_.data(); float* Y = d_.data();
        Xb[0] = adaa_[0].x1; Xb[1] = adaa_[1].x1; Fb[0] = adaa_[0].f1; Fb[1] = adaa_[1].f1;
        float* X = Xb + 2; float* F = Fb + 2;
        float* G = A; float* Po = Bq;      // reuse the bank's scratch: per-value gain and post gain
        for (int k = 0; k < K; ++k) { G[k] = gain_[(size_t)(k >> 1 >> sh)]; Po[k] = post_[(size_t)(k >> 1 >> sh)]; C[k] = act[k & 1] * c.driveMix[k >> 1 >> sh]; }
        if (rect) for (int k = 0; k < K; ++k) X[k] = G[k] * S[k];
        else for (int k = 0; k < K; ++k) X[k] = G[k] * S[k] + bias;
        // One loop per curve (the switch hoisted out) so each vectorises.
        auto pass = [&](auto F1, auto Fc) {
            for (int k = 0; k < K; ++k) F[k] = F1(X[k]);
            for (int k = 0; k < K; ++k) {
                const float xp = X[k - 2], fp = F[k - 2];
                const float d = X[k] - xp, mid = 0.5f * (X[k] + xp);
                const bool big = std::fabs(d) > 1e-3f * std::max(1.f, std::fabs(X[k]));
                const float q = (F[k] - fp) / (big ? d : 1.f), m = Fc(mid);
                Y[k] = big ? q : m;
            }
        };
        switch (cv) {
            case Curve::Tanh: pass([](float x) { return curveF1f(Curve::Tanh, x); }, [](float x) { return curveFf(Curve::Tanh, x); }); break;
            case Curve::Rational: pass([](float x) { return curveF1f(Curve::Rational, x); }, [](float x) { return curveFf(Curve::Rational, x); }); break;
            case Curve::Clip: pass([](float x) { return curveF1f(Curve::Clip, x); }, [](float x) { return curveFf(Curve::Clip, x); }); break;
            case Curve::Sine: pass([](float x) { return curveF1f(Curve::Sine, x); }, [](float x) { return curveFf(Curve::Sine, x); }); break;
            case Curve::Rectify: pass([](float x) { return curveF1f(Curve::Rectify, x); }, [](float x) { return curveFf(Curve::Rectify, x); }); break;
        }
        for (int ch = 0; ch < 2; ++ch) { adaa_[ch].x1 = X[K - 2 + ch]; adaa_[ch].f1 = F[K - 2 + ch]; }
        if (rect) for (int k = 0; k < K; ++k) Y[k] = (0.5f * (1.f - rectK) * X[k] + 0.5f * (1.f + rectK) * Y[k]) * Po[k];
        else for (int k = 0; k < K; ++k) Y[k] = (Y[k] - fb) * Po[k];
        // Filters after the curve run in time order per channel.
        for (int k = 0; k < K; ++k) {
            const int ch = k & 1;
            float y = Y[k];
            if (tape) y = tape_[ch].lp(y);
            if (tilt != 0.f) { const float lo = tilt_[ch].lp(y); y = gLow * lo + gHigh * (y - lo); }
            Y[k] = y;
        }
        for (int k = 0; k < K; ++k) S[k] += C[k] * (Y[k] - S[k]);
    }
    // Pass 3: what the section added passes the DC blocker; the rest of the band is added back.
    for (int k = 0; k < K; ++k) {
        const int ch = k & 1;
        const float* f = &hi_[(size_t)(k >> 1) * kLanes];
        y_[(size_t)k] = f[ch] + dc_[ch].tick(S[k] - f[ch]) + f[2 + ch];
    }
    down_.down(y_.data(), out_.data(), n);
    for (int i = 0; i < n; ++i) { out[0][i] = out_[(size_t)i * 2]; out[1][i] = out_[(size_t)i * 2 + 1]; }
}

}  // namespace muon
