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
    for (auto& u : up_) u.prepare(osFactor, linear, maxBlock);
    down_.prepare(osFactor, linear, maxBlock);
    const int os = up_[0].factor();
    fsHi_ = fs * os;
    hil_.prepare(fs);
    for (auto* v : {&a_, &b_}) v->assign((size_t)maxBlock, 0.f);
    for (auto* v : {&upS_, &upO_, &upA_, &upB_, &y_}) v->assign((size_t)maxBlock * os, 0.f);
    dc_.setHz(fsHi_, 5.0);
    tilt_.setHz(fsHi_, 1000.0);
    reset();
}

void HarmonicBand::reset() {
    hil_.reset();
    for (auto& u : up_) u.reset();
    down_.reset();
    adaa_.reset(); tape_.reset(); tilt_.reset(); dc_.reset();
    bankWasOn_ = false;
}

void HarmonicBand::process(const float* xSrc, const float* xOther, float* out, int n, const HarmControl& c) {
    const int os = up_[0].factor(), N = n * os;
    up_[0].up(xSrc, upS_.data(), n);
    if (c.hasOther) up_[1].up(xOther, upO_.data(), n);
    if (c.bank) {
        if (!bankWasOn_) hil_.reset();
        for (int i = 0; i < n; ++i) hil_.process(xSrc[i], a_[(size_t)i], b_[(size_t)i]);
        up_[2].up(a_.data(), upA_.data(), n);
        up_[3].up(b_.data(), upB_.data(), n);
    }
    bankWasOn_ = c.bank;

    const Curve cv = curveFor(c.curve);
    if (c.curve != lastCurve_) { adaa_.setCurve(cv); lastCurve_ = c.curve; }
    adaa_.setCurve(cv);
    const float w1 = c.weights[0] - 1.f;
    // Tape: high-frequency loss that grows with drive; set once per block from the block's last drive value.
    if (c.driveOn && c.curve == DriveCurve::Tape) tape_.setHz(fsHi_, 20000.0 * (1.0 - 0.75 * c.drive[n - 1]));
    const float tilt = 2.f * c.tone - 1.f, gLow = dbToGain(-6.f * tilt), gHigh = dbToGain(6.f * tilt);
    // Tube leans asymmetric (even harmonics) even at zero bias; Rectify uses bias as its half/full blend.
    const float bias = c.curve == DriveCurve::Rectify ? 0.f : c.curve == DriveCurve::Tube ? 0.35f + 0.65f * c.bias : c.bias;
    const float rectK = 0.5f * (c.bias + 1.f);      // Rectify: 0 half wave .. 1 full wave
    const float fb = (float)curveF(cv, bias);

    for (int j = 0; j < N; ++j) {
        const int i = j / os;
        const float xs = upS_[(size_t)j];
        float s = xs;
        if (c.bank) {
            const float a = upA_[(size_t)j], b = upB_[(size_t)j], amp = std::sqrt(a * a + b * b);
            float h = 0.f;
            if (amp > 1e-9f) {
                const float u = std::clamp(a / amp, -1.f, 1.f);
                float t0 = 1.f, t1 = u;
                for (int k = 1; k < kOrders; ++k) {               // T_{k+1}
                    const float t2 = 2.f * u * t1 - t0;
                    h += c.weights[(size_t)k] * t2;
                    t0 = t1; t1 = t2;
                }
                h *= amp;
            }
            s += c.amount[i] * (w1 * xs + h);
        }
        if (c.driveOn) {
            const float d = c.drive[i], g = dbToGain(36.f * d);
            float y;
            if (c.curve == DriveCurve::Rectify) {
                const float full = adaa_.tick(g * s);             // |g s| (anti-aliased)
                y = (0.5f * (1.f - rectK) * g * s + 0.5f * (1.f + rectK) * full) / g;
            } else {
                y = (adaa_.tick(g * s + bias) - fb) / std::min(g, 2.f);
                if (c.curve == DriveCurve::Tape) y = tape_.lp(y);
            }
            if (tilt != 0.f) { const float lo = tilt_.lp(y); y = gLow * lo + gHigh * (y - lo); }
            s += c.driveMix[i] * (y - s);
        }
        const float add = dc_.tick(s - xs);
        y_[(size_t)j] = xs + add + (c.hasOther ? upO_[(size_t)j] : 0.f);
    }
    down_.down(y_.data(), out, n);
}

}  // namespace muon
