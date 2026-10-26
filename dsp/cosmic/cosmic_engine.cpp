#include "cosmic/cosmic_engine.h"
#include <algorithm>
#include <cmath>

namespace cs {

const char* recipeName(Recipe r) {
    static const char* names[kRecipeCount] = {"Shimmer", "Redshift", "Corona", "Wormhole", "Event Horizon", "Nebula", "Pulsar", "Solar Wind", "Aurora", "Supernova"};
    return names[(int)r];
}

std::array<const char*, 3> recipeMacroNames(Recipe r) {
    switch (r) {
        case Recipe::Shimmer:      return {"Shimmer", "Interval", "Tone"};
        case Recipe::Redshift:     return {"Shift", "Depth", "Darkness"};
        case Recipe::Corona:       return {"Halo", "Balance", "Brightness"};
        case Recipe::Wormhole:     return {"Detune", "Movement", "Width"};
        case Recipe::EventHorizon: return {"Gravity", "Collapse", "Sub"};
        case Recipe::Nebula:       return {"Density", "Scatter", "Octaves"};
        case Recipe::Pulsar:       return {"Rate", "Shape", "Duck"};
        case Recipe::SolarWind:    return {"Wind", "Drift", "Resonance"};
        case Recipe::Aurora:       return {"Sweep", "Colour", "Shimmer"};
        case Recipe::Supernova:    return {"Chunk", "Bloom", "Afterglow"};
    }
    return {"A", "B", "C"};
}

namespace {
double lerp(double a, double b, double t) { return a + (b - a) * t; }
double expmap(double lo, double hi, double t) { return lo * std::pow(hi / lo, t); }
}  // namespace

void CosmicEngine::configure(Recipe r, const ReverbParams& p, double fs, int maxBlock) {
    recipe_ = r; fs_ = fs; maxBlock_ = std::max(kChunk, maxBlock); t60mid_ = p.t60[1];
    ReverbParams tp = p; tp.dry = 0.f; tp.wet = 1.f; tp.preDelayMs = 0.0;
    tail_.configure(tp, fs, kChunk);
    pre_.configure((int)std::lround(p.preDelayMs * fs / 1000.0));
    for (int c = 0; c < 2; ++c) {
        shiftA_[c].configure(fs); shiftB_[c].configure(fs); postShift_[c].configure(fs, 60.0);
        sat_[c].configure(fs);
        loopFilter_[c].configure(fs, p.seed * 31 + c); postFilter_[c].configure(fs, p.seed * 37 + c);
        rev_[c].configure(fs, 2000.0);
    }
    gran_.configure(fs, p.seed * 41, 24);
    gate_.configure(fs);
    for (auto* v : {&fbL_, &fbR_, &xL_, &xR_, &tL_, &tR_, &t2L_, &t2R_, &pL_, &pR_}) v->assign((size_t)maxBlock_, 0.f);
    badCount_ = 0;
    applyMacros();
    reset();
}

void CosmicEngine::reset() {
    tail_.reset(); pre_.reset(); gran_.reset(); gate_.reset();
    for (int c = 0; c < 2; ++c) { shiftA_[c].reset(); shiftB_[c].reset(); postShift_[c].reset(); sat_[c].reset(); loopFilter_[c].reset(); postFilter_[c].reset(); rev_[c].reset(); }
    std::fill(fbL_.begin(), fbL_.end(), 0.f); std::fill(fbR_.begin(), fbR_.end(), 0.f);
    std::fill(std::begin(inAccL_), std::end(inAccL_), 0.f); std::fill(std::begin(inAccR_), std::end(inAccR_), 0.f);
    std::fill(std::begin(outAccL_), std::end(outAccL_), 0.f); std::fill(std::begin(outAccR_), std::end(outAccR_), 0.f);
    accPos_ = 0;
}

void CosmicEngine::setMacros(double a, double b, double c) {
    a_ = std::clamp(a, 0.0, 1.0); b_ = std::clamp(b, 0.0, 1.0); c_ = std::clamp(c, 0.0, 1.0);
    applyMacros();
}

void CosmicEngine::applyMacros() {
    fb_ = 0.f; shiftMixA_ = shiftMixB_ = 0.f; dryLoop_ = 1.f; subMix_ = 0.f; useSat_ = useLoopFilter_ = false;
    preMix_ = 0.f; postMix_ = 0.f; width_ = 1.f;
    static const double upIntervals[4] = {5, 7, 12, 19}, downIntervals[3] = {-5, -7, -12};
    switch (recipe_) {
        case Recipe::Shimmer: {
            fb_ = (float)(0.6 * a_); shiftMixA_ = 1.f; dryLoop_ = 0.f;
            for (auto& s : shiftA_) { s.setSemitones(upIntervals[std::min(3, (int)(b_ * 3.999))]); s.setGrainMs(70.0); }
            useLoopFilter_ = true;
            for (auto& f : loopFilter_) f.set(0, expmap(1500.0, 14000.0, c_), 0.6, 0.0, 0.0);
            break;
        }
        case Recipe::Redshift: {
            fb_ = (float)(0.55 * a_); shiftMixA_ = 1.f; dryLoop_ = 0.f;
            for (auto& s : shiftA_) { s.setSemitones(downIntervals[std::min(2, (int)(b_ * 2.999))]); s.setGrainMs(90.0); }
            useLoopFilter_ = true;
            for (auto& f : loopFilter_) f.set(0, expmap(6000.0, 900.0, c_), 0.6, 0.0, 0.0);
            break;
        }
        case Recipe::Corona: {
            fb_ = (float)(0.55 * a_); shiftMixA_ = (float)(1.0 - b_); shiftMixB_ = (float)b_; dryLoop_ = 0.f;
            for (auto& s : shiftA_) { s.setSemitones(7.0); s.setGrainMs(70.0); }
            for (auto& s : shiftB_) { s.setSemitones(12.0); s.setGrainMs(70.0); }
            useLoopFilter_ = true;
            for (auto& f : loopFilter_) f.set(0, expmap(3000.0, 16000.0, c_), 0.6, 0.0, 0.0);
            tail_.setModulation(1.5, 0.5);
            break;
        }
        case Recipe::Wormhole: {
            fb_ = (float)(0.45 * a_); shiftMixA_ = 0.5f; shiftMixB_ = 0.5f; dryLoop_ = 0.f;
            const double dt = lerp(0.08, 0.45, a_);
            for (auto& s : shiftA_) { s.setSemitones(dt); s.setGrainMs(80.0); }
            for (auto& s : shiftB_) { s.setSemitones(-dt); s.setGrainMs(80.0); }
            postMix_ = (float)(0.7 * b_);
            for (int ch = 0; ch < 2; ++ch) { postShift_[ch].setSemitones(ch ? -0.12 - 0.3 * b_ : 0.12 + 0.3 * b_); postShift_[ch].setGrainMs(35.0); }
            width_ = (float)lerp(0.6, 1.8, c_);
            break;
        }
        case Recipe::EventHorizon: {
            fb_ = (float)(0.35 + 0.15 * a_); dryLoop_ = 1.f; useSat_ = true;
            for (auto& s : sat_) s.setDrive(0.15 + 0.85 * a_);
            useLoopFilter_ = true;
            for (int ch = 0; ch < 2; ++ch) loopFilter_[ch].set(0, 1800.0, 0.8, expmap(0.01, 0.15, b_), 1.6, ch * 0.5, 0.0);
            subMix_ = (float)(0.5 * c_);
            for (auto& s : shiftA_) { s.setSemitones(-12.0); s.setGrainMs(100.0); }
            shiftMixA_ = subMix_;
            break;
        }
        case Recipe::Nebula: {
            preMix_ = 1.f;
            gran_.set(expmap(8.0, 90.0, a_), lerp(60.0, 140.0, b_), lerp(120.0, 900.0, b_), 0.45 * c_, 0.45 * c_, 1.0);
            break;
        }
        case Recipe::Pulsar: {
            gate_.set(expmap(0.5, 8.0, a_), b_, 1.0, c_, 180.0);
            break;
        }
        case Recipe::SolarWind: {
            fb_ = 0.45f; dryLoop_ = 1.f; useLoopFilter_ = true;
            for (int ch = 0; ch < 2; ++ch) loopFilter_[ch].set(1, 900.0, lerp(0.7, 4.0, c_), expmap(0.05, 1.2, b_), 2.5 * a_, ch * 0.5, 0.35);
            width_ = 1.4f;
            break;
        }
        case Recipe::Aurora: {
            postMix_ = (float)(0.85 * a_);
            for (int ch = 0; ch < 2; ++ch) postFilter_[ch].set(1, expmap(400.0, 4000.0, b_), 1.5, 0.18, 1.8, ch * 0.25, 0.0);
            fb_ = (float)(0.45 * c_); shiftMixA_ = 1.f; dryLoop_ = 0.f;
            for (auto& s : shiftA_) { s.setSemitones(12.0); s.setGrainMs(70.0); }
            if (c_ < 0.02) fb_ = 0.f;
            break;
        }
        case Recipe::Supernova: {
            preMix_ = (float)b_;
            for (auto& r : rev_) r.setChunkMs(expmap(200.0, 1500.0, a_));
            fb_ = (float)(0.35 * c_); shiftMixA_ = 1.f; dryLoop_ = 0.f;
            for (auto& s : shiftA_) { s.setSemitones(12.0); s.setGrainMs(80.0); }
            if (c_ < 0.02) fb_ = 0.f;
            break;
        }
    }
    // Keep the loop gain below one at every decay setting. The tail's amplitude gain for a
    // sustained input grows with the square root of its decay time (about sqrt(0.175 * T60 mid)
    // for this engine, measured), so the feedback is scaled down for long decays.
    const double tailGain = std::sqrt(0.175 * std::max(0.1, t60mid_));
    fb_ *= (float)std::min(1.0, 0.8 / tailGain);
}

void CosmicEngine::watchdog(float* l, float* r, int n) {
    // Safety net for a feedback loop: non-finite or runaway output resets the engine silently.
    bool bad = false;
    for (int i = 0; i < n; ++i) if (!std::isfinite(l[i]) || !std::isfinite(r[i]) || std::fabs(l[i]) > 16.f || std::fabs(r[i]) > 16.f) { bad = true; break; }
    if (bad) { reset(); for (int i = 0; i < n; ++i) l[i] = r[i] = 0.f; ++badCount_; }
}

void CosmicEngine::processChunk(const float* inL, const float* inR, float* outL, float* outR, int n) {
    // Pre-delay, then the pre-stage.
    pre_.process(inL, inR, xL_.data(), xR_.data(), n);
    if (recipe_ == Recipe::Nebula) {
        gran_.process(xL_.data(), xR_.data(), pL_.data(), pR_.data(), n);
        for (int i = 0; i < n; ++i) { xL_[i] = xL_[i] * (1.f - preMix_) * 0.5f + pL_[i] * preMix_; xR_[i] = xR_[i] * (1.f - preMix_) * 0.5f + pR_[i] * preMix_; }
    } else if (recipe_ == Recipe::Supernova) {
        rev_[0].process(xL_.data(), pL_.data(), n); rev_[1].process(xR_.data(), pR_.data(), n);
        for (int i = 0; i < n; ++i) { xL_[i] = xL_[i] * (1.f - preMix_) + pL_[i] * preMix_; xR_[i] = xR_[i] * (1.f - preMix_) + pR_[i] * preMix_; }
    }
    // Feedback from the previous chunk's output, through the loop inserts.
    if (fb_ > 0.f) {
        for (int ch = 0; ch < 2; ++ch) {
            float* f = ch ? fbR_.data() : fbL_.data();
            float* t = ch ? tR_.data() : tL_.data();
            float* t2 = ch ? t2R_.data() : t2L_.data();
            for (int i = 0; i < n; ++i) t[i] = f[i] * dryLoop_;
            if (shiftMixA_ > 0.f) { shiftA_[ch].process(f, t2, n); for (int i = 0; i < n; ++i) t[i] += t2[i] * shiftMixA_; }
            if (shiftMixB_ > 0.f) { shiftB_[ch].process(f, t2, n); for (int i = 0; i < n; ++i) t[i] += t2[i] * shiftMixB_; }
            if (useSat_) sat_[ch].process(t, t, n);
            if (useLoopFilter_) loopFilter_[ch].process(t, t, n);
            float* x = ch ? xR_.data() : xL_.data();
            for (int i = 0; i < n; ++i) {
                // Soft ceiling on what re-enters the tail (rational tanh, saturates at +-1.5).
                const float v = t[i] * fb_ * (2.f / 3.f), v2 = v * v;
                x[i] += 1.5f * v * (27.f + v2) / (27.f + 9.f * v2);
            }
        }
    }
    tail_.process(xL_.data(), xR_.data(), outL, outR, n);
    // Remember this chunk's output for the next chunk's feedback.
    std::copy(outL, outL + n, fbL_.begin()); std::copy(outR, outR + n, fbR_.begin());
    // Post stage.
    if (recipe_ == Recipe::Wormhole && postMix_ > 0.f) {
        postShift_[0].process(outL, pL_.data(), n); postShift_[1].process(outR, pR_.data(), n);
        for (int i = 0; i < n; ++i) { outL[i] = outL[i] * (1.f - postMix_ * 0.5f) + pL_[i] * postMix_; outR[i] = outR[i] * (1.f - postMix_ * 0.5f) + pR_[i] * postMix_; }
    } else if (recipe_ == Recipe::Aurora && postMix_ > 0.f) {
        postFilter_[0].process(outL, pL_.data(), n); postFilter_[1].process(outR, pR_.data(), n);
        for (int i = 0; i < n; ++i) { outL[i] = outL[i] * (1.f - postMix_) + pL_[i] * postMix_ * 1.6f; outR[i] = outR[i] * (1.f - postMix_) + pR_[i] * postMix_ * 1.6f; }
    } else if (recipe_ == Recipe::Pulsar) {
        gate_.process(outL, outR, inL, inR, n);
    }
    if (width_ != 1.f) {
        for (int i = 0; i < n; ++i) {
            const float m = 0.5f * (outL[i] + outR[i]), s = 0.5f * (outL[i] - outR[i]) * width_;
            outL[i] = m + s; outR[i] = m - s;
        }
    }
    watchdog(outL, outR, n);
}

void CosmicEngine::process(const float* inL, const float* inR, float* outL, float* outR, int n) {
    for (int i = 0; i < n; ++i) {
        const float l = inL[i], r = inR[i];           // read before writing: outputs may alias inputs
        outL[i] = outAccL_[accPos_]; outR[i] = outAccR_[accPos_];
        inAccL_[accPos_] = l; inAccR_[accPos_] = r;
        if (++accPos_ >= kChunk) {
            processChunk(inAccL_, inAccR_, outAccL_, outAccR_, kChunk);
            accPos_ = 0;
        }
    }
}

}  // namespace cs
