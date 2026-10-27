#include "vintage/vintage_engine.h"
#include <algorithm>
#include <cmath>

namespace cs {

const char* unitName(Unit u) {
    static const char* names[kUnitCount] = {"Steel Plate", "Spring Tank", "Digital '76", "Concert '78", "Random Hall", "Nonlinear '81", "Space Station", "Hall '85"};
    return names[(int)u];
}

std::array<const char*, 3> unitMacroNames(Unit u) {
    switch (u) {
        case Unit::SteelPlate:   return {"Damper", "Stiffness", "Drive"};
        case Unit::SpringTank:   return {"Springs", "Boing", "Tension"};
        case Unit::Digital76:    return {"Bandwidth", "Bits", "Chorus"};
        case Unit::Concert78:    return {"Chorus", "Bass", "Bandwidth"};
        case Unit::RandomHall86: return {"Spin", "Wander", "Shape"};
        case Unit::Nonlinear81:  return {"Duration", "Density", "Reverse"};
        case Unit::SpaceStation: return {"Span", "Recirc", "Grain"};
        case Unit::Hall85:       return {"Bits", "Rate", "Grit"};
    }
    return {"A", "B", "C"};
}

ReverbParams unitReverbParams(Unit u, const ReverbParams& p) {
    ReverbParams t = p; t.dry = 0.f; t.wet = 1.f; t.preDelayMs = 0.0;
    switch (u) {
        case Unit::SteelPlate:   t.size = p.size * 0.45; t.erLevel = 0.f; t.modDepth = 0.4; t.modRateHz = 0.3; t.diffusion = true; break;   // dense at once, no room
        case Unit::Digital76:    t.size = p.size * 0.8;  t.erLevel = 0.15f; t.modRateHz = 0.35; break;
        case Unit::Concert78:    t.size = p.size * 1.1;  t.erLevel = 0.25f; t.modRateHz = 0.9; break;
        case Unit::RandomHall86: t.size = p.size * 1.0;  t.erLevel = 0.4f; break;
        case Unit::Hall85:       t.size = p.size * 0.9;  t.erLevel = 0.5f; t.modDepth = 0.0; break;                                       // no modulation: the cheap digital hall
        default: break;
    }
    return t;
}

namespace {
double lerp(double a, double b, double t) { return a + (b - a) * t; }
double expmap(double lo, double hi, double t) { return lo * std::pow(hi / lo, t); }
constexpr double kPi = 3.14159265358979323846;
}

void VintageEngine::configure(Unit u, const ReverbParams& p, double fs, int maxBlock) {
    unit_ = u; fs_ = fs; maxBlock_ = std::max(1, maxBlock); p_ = p; t60mid_ = p.t60[1]; size_ = p.size;
    if (usesFdn()) tail_.configure(unitReverbParams(u, p), fs, maxBlock_);
    pre_.configure((int)std::lround(p.preDelayMs * fs / 1000.0));
    for (int c = 0; c < 2; ++c) {
        bw_[c].configure(fs); damper_[c].configure(fs);
        q_[c].configure(fs, p.seed * 53 + (uint64_t)c);
        disp_[c].configure(64);
        sat_[c].configure(fs);
    }
    burst_.configure(fs, p.seed * 61, 1.2);
    springs_.configure(fs, p.seed * 59, p.size);
    lsK_ = (float)(1.0 - std::exp(-2.0 * kPi * 250.0 / fs));
    for (auto* v : {&aL_, &aR_, &bL_, &bR_}) v->assign((size_t)maxBlock_, 0.f);
    erLevel_ = p.erLevel; tailLevel_ = p.tailLevel; gain_ = gainTarget_ = p.tailLevel;
    applyMacros();
    reset();
}

void VintageEngine::reset() {
    if (usesFdn()) tail_.reset();
    pre_.reset(); burst_.reset(); springs_.reset();
    for (int c = 0; c < 2; ++c) { bw_[c].reset(); damper_[c].reset(); q_[c].reset(); disp_[c].reset(); sat_[c].reset(); lsState_[c] = 0.f; }
}

void VintageEngine::setMacros(double a, double b, double c) {
    a_ = std::clamp(a, 0.0, 1.0); b_ = std::clamp(b, 0.0, 1.0); c_ = std::clamp(c, 0.0, 1.0);
    applyMacros();
}

void VintageEngine::setLevels(float er, float tail, int ramp) {
    erLevel_ = er; tailLevel_ = tail;
    if (usesFdn()) tail_.setMix(0.f, 1.f, unit_ == Unit::RandomHall86 ? (float)(er * lerp(0.2, 1.6, c_)) : er, tail, ramp);
    gainTarget_ = tail; gainLeft_ = ramp; gainStep_ = ramp > 0 ? (gainTarget_ - gain_) / (float)ramp : 0.f; if (ramp <= 0) gain_ = tail;
}

void VintageEngine::setModulation(double depth, double rate) {
    // Units whose modulation is part of their character keep it; the others follow the panel.
    if (unit_ == Unit::SteelPlate || unit_ == Unit::Hall85) if (usesFdn()) tail_.setModulation(depth, rate);
}

void VintageEngine::setDecay(const DecaySet& d, int ramp) {
    t60mid_ = d.t60mid;
    if (usesFdn()) tail_.setDecay(d, ramp);
    applyMacros();
}

void VintageEngine::setFreeze(bool on, int ramp) { if (usesFdn()) tail_.setFreeze(on, ramp); }

void VintageEngine::applyMacros() {
    useBw_ = useDamper_ = useQ_ = useDisp_ = useSat_ = false; lowShelfGain_ = 1.f;
    switch (unit_) {
        case Unit::SteelPlate: {
            // EMT 140 manner: a damper plate pressed closer takes the highs out of the decay; stiffness is the
            // metallic ring from dispersion at the input; drive is the transducer pushed.
            useDamper_ = true; for (auto& f : damper_) f.set(expmap(14000.0, 2200.0, a_));
            useDisp_ = true;   for (auto& d : disp_) d.set((int)std::lround(8.0 + 40.0 * b_), (float)(-0.35 - 0.3 * b_));
            useSat_ = c_ > 0.02; for (auto& s : sat_) s.setDrive(0.05 + 0.6 * c_);
            break;
        }
        case Unit::SpringTank: {
            springs_.set(1 + (int)std::lround(2.0 * a_), b_, expmap(2200.0, 7000.0, c_), t60mid_);
            useBw_ = true; for (auto& f : bw_) f.set(expmap(3500.0, 9000.0, c_));
            break;
        }
        case Unit::Digital76: {
            // EMT 250 manner: 12-bit converters at a low rate, a soft band limit, and a gentle chorus on the tail.
            useBw_ = true; for (auto& f : bw_) f.set(expmap(5500.0, 12000.0, a_));
            useQ_ = true;  for (auto& q : q_) q.set(lerp(16.0, 10.0, b_), 24000.0);
            if (usesFdn()) tail_.setModulation(lerp(0.3, 5.0, c_), 0.35);
            break;
        }
        case Unit::Concert78: {
            // Lexicon 224 manner: chorused tail, a bass control, and the 15 kHz or so of the day.
            if (usesFdn()) tail_.setModulation(lerp(0.6, 6.0, a_), 0.9);
            lowShelfGain_ = (float)std::pow(10.0, lerp(-6.0, 6.0, b_) / 20.0);
            useBw_ = true; for (auto& f : bw_) f.set(expmap(7000.0, 18000.0, c_));
            break;
        }
        case Unit::RandomHall86: {
            // 480L manner: spin and wander drive the random modulation; shape brings the early build-up forward.
            if (usesFdn()) { tail_.setModulation(lerp(0.5, 7.0, b_), expmap(0.15, 2.5, a_)); tail_.setMix(0.f, 1.f, (float)(erLevel_ * lerp(0.2, 1.6, c_)), tailLevel_, 256); }
            break;
        }
        case Unit::Nonlinear81: {
            // AMS RMX16 manner: a dense burst with a fixed envelope, flat and cut, or ramping up for reverse.
            burst_.set(expmap(0.08, 0.7, a_), (int)std::lround(16.0 + 96.0 * b_), c_, 0.0, 15000.0);
            break;
        }
        case Unit::SpaceStation: {
            // Ursa Major manner: a cloud of taps over a span, recirculated through a narrow band, grainy.
            burst_.set(expmap(0.06, 0.4, a_), 32, -0.5, 0.85 * b_, expmap(8000.0, 3000.0, c_));
            useQ_ = c_ > 0.5; for (auto& q : q_) q.set(12.0, 20000.0);
            break;
        }
        case Unit::Hall85: {
            // SPX90 manner: 12-bit at 31 kHz, a sparse early pattern, and grit from a coarse input band.
            useQ_ = true;  for (auto& q : q_) q.set(lerp(16.0, 9.0, a_), expmap(31250.0, 15000.0, b_));
            useBw_ = true; for (auto& f : bw_) f.set(expmap(12000.0, 5000.0, c_));
            if (usesFdn()) tail_.setMix(0.f, 1.f, (float)(erLevel_ * lerp(0.6, 1.8, c_)), tailLevel_, 256);
            break;
        }
    }
}

void VintageEngine::process(const float* inL, const float* inR, float* outL, float* outR, int n) {
    for (int off = 0; off < n; off += maxBlock_) {
        const int m = std::min(maxBlock_, n - off);
        float* a = aL_.data(); float* b = aR_.data(); float* x = bL_.data(); float* y = bR_.data();
        pre_.process(inL + off, inR + off, a, b, m);
        if (useBw_) { bw_[0].process(a, a, m); bw_[1].process(b, b, m); }
        if (useSat_) { sat_[0].process(a, a, m); sat_[1].process(b, b, m); }
        if (useDisp_) { disp_[0].process(a, a, m); disp_[1].process(b, b, m); }
        if (usesFdn()) tail_.process(a, b, x, y, m);
        else if (unit_ == Unit::SpringTank) springs_.process(a, b, x, y, m);
        else burst_.process(a, b, x, y, m);
        if (!usesFdn()) for (int i = 0; i < m; ++i) {                                  // tail level for the non-FDN units
            if (gainLeft_ > 0) { gain_ += gainStep_; if (--gainLeft_ == 0) gain_ = gainTarget_; }
            x[i] *= gain_; y[i] *= gain_;
        }
        if (lowShelfGain_ != 1.f) {                                                      // Concert '78 bass: a one-pole low shelf
            const float k = lsK_, g = lowShelfGain_ - 1.f;
            for (int i = 0; i < m; ++i) { lsState_[0] += k * (x[i] - lsState_[0]); lsState_[1] += k * (y[i] - lsState_[1]); x[i] += g * lsState_[0]; y[i] += g * lsState_[1]; }
        }
        if (useDamper_) { damper_[0].process(x, x, m); damper_[1].process(y, y, m); }
        if (useQ_) { q_[0].process(x, x, m); q_[1].process(y, y, m); }
        for (int i = 0; i < m; ++i) {
            float l = x[i], r = y[i];
            if (!std::isfinite(l) || !std::isfinite(r)) { l = r = 0.f; reset(); }
            outL[off + i] = l; outR[off + i] = r;
        }
    }
}

}  // namespace cs
