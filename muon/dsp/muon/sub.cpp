#include "sub.h"

namespace muon {

void SubGenerator::prepare(double fs) {
    fs_ = fs;
    hp_.set(fs, 25.0, 1.41421356);
    env_.setTime(fs, 0.03);
    gateRel_ = (float)std::exp(-1.0 / (0.003 * fs));
    hil_.prepare(fs);
    range_ = -1.f;
    reset();
}

void SubGenerator::reset() {
    hp_.reset(); lpA_.reset(); lpB_.reset();
    for (auto& f : clean1_) f.reset();
    for (auto& f : clean2_) f.reset();
    env_.reset(); hil_.reset();
    ff1_ = ff2_ = 1.f; armed_ = false; gateEnv_ = 0.f; since_ = 0; lastCross_ = 0; lastHz_ = 0.f; t_ = 0; gate_ = 0.f;
}

void SubGenerator::process(const float* x, float* out, int n, const Params& p) {
    const float range = std::clamp(p.subRange, 40.f, 400.f);
    if (range != range_) {
        range_ = range;
        lpA_.set(fs_, range, 1.41421356); lpB_.set(fs_, range, 1.41421356);
        if (lastHz_ <= 0.f) for (int k = 0; k < 2; ++k) { clean1_[k].set(fs_, range * 0.6, 1.41421356); clean2_[k].set(fs_, range * 0.3, 1.41421356); }
    }
    const int minPeriod = (int)(fs_ / (range * 1.2));
    const float purity = std::clamp(p.subPurity, 0.f, 1.f), l1 = p.subLevel, l2 = p.sub2Level, ph = p.phantom;
    const float gateStep = (float)(1.0 / (0.005 * fs_));
    for (int i = 0; i < n; ++i, ++t_) {
        const float s = lpB_.lp(lpA_.lp(hp_.hp(x[i])));
        const float a = std::fabs(s);
        const float e = a > env_.z ? a : env_.lp(a);
        if (a > env_.z) env_.z = a;
        const float h = 0.1f * e;
        ++since_;
        if (s < -h) armed_ = true;
        if (armed_ && s > h && since_ >= minPeriod && e > 1e-5f) {
            armed_ = false; since_ = 0;
            ff1_ = -ff1_;
            if (ff1_ > 0.f) {
                ff2_ = -ff2_;
                const long long interval = t_ - lastCross_;
                lastCross_ = t_;
                if (interval > 0 && interval < (long long)(fs_ / 8.0)) {
                    lastHz_ = (float)(fs_ / (double)interval);
                    const double fc = std::clamp(1.3 * lastHz_, 12.0, (double)range);
                    for (int k = 0; k < 2; ++k) { clean1_[k].set(fs_, fc, 1.41421356); clean2_[k].set(fs_, fc * 0.5, 1.41421356); }
                }
            }
        }
        // Gate: a fast envelope of the input (3 ms release) under -80 dBFS closes the output within a few milliseconds.
        const float ax = std::fabs(x[i]);
        gateEnv_ = ax > gateEnv_ ? ax : ax + gateRel_ * (gateEnv_ - ax);
        gate_ = std::clamp(gate_ + (gateEnv_ > 1e-4f ? gateStep : -gateStep), 0.f, 1.f);
        float y = 0.f;
        if (l1 > 0.f || l2 > 0.f) {
            const float c1 = clean1_[1].lp(clean1_[0].lp(ff1_ * s)) * 1.59f;
            const float c2 = clean2_[1].lp(clean2_[0].lp(ff2_ * ff1_ * s)) * 1.6f;
            y += l1 * (purity * c1 + (1.f - purity) * 0.8f * ff1_ * e);
            y += l2 * (purity * c2 + (1.f - purity) * 0.8f * ff2_ * e);
        }
        if (ph > 0.f) {
            float ha, hb; hil_.process(s, ha, hb);
            const float amp = std::sqrt(ha * ha + hb * hb);
            if (amp > 1e-9f) {
                const float u = std::clamp(ha / amp, -1.f, 1.f), t2 = 2.f * u * u - 1.f, t3 = 2.f * u * t2 - u, t4 = 2.f * u * t3 - t2;
                y += ph * 0.8f * amp * (t2 + 0.6f * t3 + 0.35f * t4);
            }
        }
        out[i] = y * gate_;
    }
}

}  // namespace muon
