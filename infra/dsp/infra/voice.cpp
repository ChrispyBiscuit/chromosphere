#include "voice.h"

namespace infra {
namespace {
// Sine over one cycle, 4096 points plus a guard point.
struct SineTable {
    float v[4097];
    SineTable() { for (int i = 0; i <= 4096; ++i) v[i] = (float)std::sin(6.283185307179586 * i / 4096.0); }
};
const SineTable& sineTable() { static const SineTable t; return t; }
inline float sin2pi(const SineTable& t, float p) {   // p in [0, 1)
    const float x = p * 4096.f;
    const int i = (int)x;
    return t.v[i] + (t.v[i + 1] - t.v[i]) * (x - (float)i);
}
// Wrap to [0, 1) without std::floor (a library call on baseline x86-64).
inline float wrap01(float p) {
    p -= (float)(int)p;
    if (p < 0.f) p += 1.f;
    return p >= 1.f ? 0.f : p;
}
inline float polyBlep(float t, float dt) {
    if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}
inline float warpPhase(const SineTable& s, Warp w, float a, float p) {
    switch (w) {
        case Warp::Bend: return p + a * 0.95f * sin2pi(s, p) * (1.f / kTwoPi);
        case Warp::Mirror: return p + ((p < 0.5f ? p : 1.f - p) - p) * a;
        case Warp::Quantize: { const float steps = 2.f + (1.f - a) * (1.f - a) * 2046.f; return (float)(int)(p * steps) / steps; }
        default: return p;
    }
}
constexpr float kPreFilterGain = 0.5f;
constexpr float kVoiceGain = 0.9f;
}  // namespace

void Voice::prepare(double fs, uint32_t seed) {
    fs_ = fs;
    rng_.seed(seed);
    filter_.prepare(fs);
    noise_.setup((float)fs);
    noise_.reset(seed * 31u + 7u);
    sineTable(); WavetableBank::get();
    active_ = false;
}

void Voice::start(const VoiceContext& c, int note, float velocity, uint32_t id, float stepValue, float glideFrom, bool retrigger) {
    const SynthParams& p = *c.p;
    const bool wasSounding = active_ && amp_.value() > 1e-3f;
    note_ = note; id_ = id; velocity_ = velocity; stepValue_ = stepValue; gate_ = true;
    velCurve_ = std::pow(std::clamp(velocity, 0.f, 1.f), 1.3f);
    target_ = (float)note;
    pitch_ = glideFrom >= 0.f ? glideFrom : (float)note;
    random_ = rng_.bi();
    timeSinceOn_ = 0.f;
    if (!wasSounding) {
        for (int o = 0; o < 2; ++o) {
            const auto& op = p.osc[(size_t)o];
            for (int u = 0; u < kMaxUnison; ++u) {
                // The first unison voice honours the start phase; the others always start at random places,
                // otherwise a stack of detuned voices begins as one loud flanged spike.
                const float rnd = u == 0 ? op.phaseRand : 1.f;
                osc_[o].phase[u] = wrap01(op.phase + rnd * rng_.uni());
                osc_[o].detuneRand[u] = rng_.bi();
            }
            osc_[o].cacheN = -1;
        }
        subPhase_ = 0.f;
        filter_.reset();
        noise_.reset(rng_.nextU());
        firstControl_ = true;
        amp_.kill(); fenv_.kill(); menv_.kill();
    }
    for (int i = 0; i < 2; ++i) {
        const auto& lp = p.lfo[(size_t)i];
        if (lp.retrig || !wasSounding) lfo_[i].reset(lp.retrig ? 0.f : c.globalLfoPhase[i], rng_);
    }
    if (!wasSounding) { drift_.reset(rng_.uni(), rng_); driftRate_ = 0.2f + 0.5f * rng_.uni(); driftStatic_ = rng_.bi(); }
    active_ = true;
    if (retrigger || !wasSounding) {
        amp_.setup(p.ampEnv, fs_, 1, p.ampCurve);
        amp_.gateOn(); fenv_.gateOn(); menv_.gateOn();
    }
    rampPos_ = 0;
    control(c);
}

void Voice::legatoTo(int note, uint32_t id, float velocity, float glideFrom) {
    note_ = note; id_ = id; gate_ = true;
    (void)velocity;
    target_ = (float)note;
    if (glideFrom < 0.f) pitch_ = (float)note;
}

void Voice::release() {
    if (!gate_) return;
    gate_ = false;
    amp_.gateOff(); fenv_.gateOff(); menv_.gateOff();
}

void Voice::control(const VoiceContext& c) {
    const SynthParams& p = *c.p;
    const int step = kControl;
    const float dtStep = (float)(step / fs_);
    if (!firstControl_) timeSinceOn_ += dtStep;
    rampPos_ = 0;

    amp_.setup(p.ampEnv, fs_, 1, p.ampCurve);
    fenv_.setup(p.filterEnv, fs_, step);
    menv_.setup(p.modEnv, fs_, step);
    const float fe = fenv_.next(), me = menv_.next();

    // LFOs (their rate modulation comes from the previous control step).
    float lv[2];
    for (int i = 0; i < 2; ++i) {
        const auto& lp = p.lfo[(size_t)i];
        double rate = lp.sync ? c.bpm / 60.0 / kDivisionBeats[std::clamp(lp.division, 0, kNumDivisions - 1)] : (double)lp.rateHz;
        rate *= std::exp2(std::clamp(lfoRateMod_[i], -1.f, 1.f) * 4.f);
        const float v = lfo_[i].advance(lp.shape, (float)(rate * step / fs_), rng_);
        const float fade = lp.fade > 0.001f ? std::min(1.f, timeSinceOn_ / lp.fade) : 1.f;
        lv[i] = v * fade;
    }
    const float driftV = drift_.advance(LfoShape::Drift, driftRate_ * dtStep, rng_);

    // ---- Modulation matrix (per-voice destinations) ----
    float src[kNumModSources] = {};
    src[(int)ModSource::Lfo1] = lv[0];
    src[(int)ModSource::Lfo2] = lv[1];
    src[(int)ModSource::ModEnv] = me;
    src[(int)ModSource::FilterEnv] = fe;
    src[(int)ModSource::AmpEnv] = amp_.value();
    src[(int)ModSource::Velocity] = velocity_;
    src[(int)ModSource::Key] = std::clamp(((float)note_ - 60.f) / 48.f, -1.f, 1.f);
    src[(int)ModSource::ModWheel] = c.modWheel;
    src[(int)ModSource::Aftertouch] = c.aftertouch;
    src[(int)ModSource::PitchBend] = c.bend;
    for (int m = 0; m < kNumMacros; ++m) src[(int)ModSource::Macro1 + m] = p.macro[(size_t)m];
    src[(int)ModSource::Random] = random_;
    src[(int)ModSource::ArpStep] = stepValue_;
    src[(int)ModSource::Drift] = driftV;
    float d[kNumModDests] = {};
    for (const auto& s : p.mod) {
        const int di = (int)s.dst;
        if (s.src == ModSource::None || di <= 0 || di >= kFirstGlobalDest) continue;
        d[di] += s.amount * src[(int)s.src];
    }
    lfoRateMod_[0] = d[(int)ModDest::Lfo1Rate];
    lfoRateMod_[1] = d[(int)ModDest::Lfo2Rate];

    // ---- Pitch ----
    if (p.glide > 0.0005f) pitch_ += (target_ - pitch_) * (float)(1.0 - std::exp(-step / (p.glide * fs_ / 3.0)));
    else pitch_ = target_;
    const float driftAmt = p.drift;
    const float base = pitch_ + c.bend * (float)p.bendRange + p.masterTune * 0.01f + driftAmt * (driftV * 0.10f + driftStatic_ * 0.04f) +
                       d[(int)ModDest::Pitch] * 24.f;
    const float detMod = d[(int)ModDest::Detune];
    for (int o = 0; o < 2; ++o) {
        const auto& op = p.osc[(size_t)o];
        Osc& os = osc_[o];
        const float lvl = std::clamp(op.level + d[(int)ModDest::LevelA + o], 0.f, 1.f);
        os.levelPrev = firstControl_ ? lvl : os.level;
        os.level = lvl;
        os.on = op.on && (lvl > 0.f || os.levelPrev > 0.f);
        const float pitchO = base + (float)(op.octave * 12 + op.semi) + op.fine * 0.01f + d[(int)ModDest::PitchA + o] * 24.f;
        const float inc = std::min(0.45f, noteToHz(pitchO) / (float)fs_);
        os.unison = std::clamp(op.unison, 1, kMaxUnison);
        const int n = os.unison;
        const float det = std::clamp(op.detune + detMod, 0.f, 1.f);
        // Detune ratios and pan gains only change with their controls: recompute them only then.
        if (det != os.cacheDet || n != os.cacheN) {
            os.cacheDet = det;
            for (int u = 0; u < n; ++u) {
                const float x = n == 1 ? 0.f : 2.f * (float)u / (float)(n - 1) - 1.f;
                const float cents = det * det * 100.f * x * (1.f + 0.12f * os.detuneRand[u]) + (n > 1 ? det * 3.f * os.detuneRand[u] : 0.f);
                os.ratio[u] = std::exp2(cents * (1.f / 1200.f));
            }
        }
        if (op.pan != os.cachePan || op.width != os.cacheWidth || n != os.cacheN) {
            os.cachePan = op.pan; os.cacheWidth = op.width;
            const float norm = 1.f / std::sqrt((float)n);
            for (int u = 0; u < n; ++u) {
                const float x = n == 1 ? 0.f : 2.f * (float)u / (float)(n - 1) - 1.f;
                const float pan = std::clamp(op.pan + x * op.width, -1.f, 1.f);
                const float ang = (pan + 1.f) * (kPi * 0.25f);
                os.gl[u] = std::cos(ang) * norm * 1.41421356f;
                os.gr[u] = std::sin(ang) * norm * 1.41421356f;
            }
        }
        os.cacheN = n;
        for (int u = 0; u < n; ++u) os.inc[u] = inc * os.ratio[u];
        os.table = std::clamp(op.table, 0, kNumTables - 1);
        const float m = std::clamp(op.morph + d[(int)ModDest::MorphA + o], 0.f, 1.f);
        os.morphPrev = firstControl_ ? m : os.morph;
        os.morph = m;
        WavetableBank::levelFor(inc, os.L0, os.wl);
        os.warp = op.warp;
        os.warpAmt = std::clamp(op.warpAmt + d[(int)ModDest::WarpA + o], 0.f, 1.f);
    }
    // Sub and noise.
    subShape_ = p.subShape;
    subInc_ = std::min(0.45f, noteToHz(base) / (float)fs_) / (p.subOctave >= 2 ? 4.f : 2.f);
    const float sl = std::clamp(p.subLevel + d[(int)ModDest::SubLevel], 0.f, 1.f);
    subLevelPrev_ = firstControl_ ? sl : subLevel_; subLevel_ = sl;
    noiseColour_ = p.noiseColour;
    const float nl = std::clamp(p.noiseLevel + d[(int)ModDest::NoiseLevel], 0.f, 1.f);
    noiseLevelPrev_ = firstControl_ ? nl : noiseLevel_; noiseLevel_ = nl;

    // ---- Filter ----
    const float semis = p.keyTrack * ((float)note_ - 60.f) + p.filterEnvAmt * fe + p.filterVel * 36.f * velocity_ +
                        d[(int)ModDest::Cutoff] * 72.f + driftAmt * driftV * 0.6f;
    const float fc = std::clamp(p.cutoff * std::exp2(semis / 12.f), 16.f, 22000.f);
    filter_.set(p.filterType, fc, std::clamp(p.resonance + d[(int)ModDest::Resonance], 0.f, 1.f),
                std::clamp(p.filterDrive + d[(int)ModDest::FilterDrive], 0.f, 1.f));

    // ---- Amp ----
    const float vs = std::clamp(p.velSens, 0.f, 1.f);
    const float velGain = (1.f - vs) + vs * velCurve_;
    const float ampMod = std::clamp(1.f + d[(int)ModDest::Amp], 0.f, 2.f);
    const float pan = std::clamp(d[(int)ModDest::Pan], -1.f, 1.f);
    const float g = velGain * ampMod * kVoiceGain;
    const float nL = g * (pan > 0.f ? 1.f - pan : 1.f), nR = g * (pan < 0.f ? 1.f + pan : 1.f);
    gainLPrev_ = firstControl_ ? nL : gainL_; gainRPrev_ = firstControl_ ? nR : gainR_;
    gainL_ = nL; gainR_ = nR;
    firstControl_ = false;
}

namespace {
// One oscillator's unison stack over n samples (n <= kControl), added into local buffers. The warp is a template
// parameter so the inner loop carries no per-sample switch.
struct OscPass {
    const float* t0[kControl];      // frame f0 at the chosen level, per sample (the morph is ramped)
    float wf[kControl];             // weight of frame f0 + 1
    float wl;                       // weight of the next (smoother) level
    float amt;
    const float* mod;               // OSC B mono output for FM / ring, or null
};
constexpr int kFrameStep = WavetableBank::kLevels * WavetableBank::kStride;

inline float readPair(const float* t0, float wf, float wl, float ph) {
    float v = readTable(t0, ph);
    v += (readTable(t0 + kFrameStep, ph) - v) * wf;
    if (wl > 0.f) {
        const float* c0 = t0 + WavetableBank::kStride;
        float v2 = readTable(c0, ph);
        v2 += (readTable(c0 + kFrameStep, ph) - v2) * wf;
        v += (v2 - v) * wl;
    }
    return v;
}

template <int W>
void oscLoop(const OscPass& ps, const SineTable& sine, float& phase, float inc, float gl, float gr, int n,
             float* __restrict outL, float* __restrict outR, float* __restrict outMono) {
    float ph = phase;
    for (int i = 0; i < n; ++i) {
        float p = ph;
        if constexpr (W == (int)Warp::FM) p = wrap01(ph + ps.amt * 1.5f * ps.mod[i]);
        else if constexpr (W == (int)Warp::Bend || W == (int)Warp::Mirror || W == (int)Warp::Quantize) {
            p = warpPhase(sine, (Warp)W, ps.amt, ph);
            if (!(p < 1.f && p >= 0.f)) p = 0.f;
        }
        float v = readPair(ps.t0[i], ps.wf[i], ps.wl, p);
        if constexpr (W == (int)Warp::Ring) v *= (1.f - ps.amt) + ps.amt * ps.mod[i] * 1.4f;
        outL[i] += v * gl; outR[i] += v * gr;
        if (outMono) outMono[i] += v;
        ph += inc;
        if (ph >= 1.f) ph -= 1.f;
    }
    phase = ph;
}
}  // namespace

void Voice::renderOsc(Osc& o, int n, int ramp0, bool allowCross, const float* mod, float* outL, float* outR, float* outMono) {
    const auto& bank = WavetableBank::get();
    OscPass ps;
    for (int i = 0; i < n; ++i) {
        const float fr = (float)std::min(ramp0 + i + 1, kControl) * (1.f / (float)kControl);
        const float pos = (o.morphPrev + (o.morph - o.morphPrev) * fr) * (float)(WavetableBank::kFrames - 1);
        int f0 = (int)pos; float w = pos - (float)f0;
        if (f0 >= WavetableBank::kFrames - 1) { f0 = WavetableBank::kFrames - 2; w = 1.f; }
        ps.t0[i] = bank.frame(o.table, f0, o.L0);
        ps.wf[i] = w;
    }
    ps.wl = o.wl; ps.amt = o.warpAmt; ps.mod = mod;
    Warp w = o.warp;
    if (!allowCross && (w == Warp::FM || w == Warp::Ring)) w = Warp::Off;
    if (w == Warp::Bend && ps.amt <= 0.f) w = Warp::Off;
    const auto& sine = sineTable();
    for (int u = 0; u < o.unison; ++u) {
        float& ph = o.phase[u];
        switch (w) {
            case Warp::Off: oscLoop<(int)Warp::Off>(ps, sine, ph, o.inc[u], o.gl[u], o.gr[u], n, outL, outR, outMono); break;
            case Warp::Bend: oscLoop<(int)Warp::Bend>(ps, sine, ph, o.inc[u], o.gl[u], o.gr[u], n, outL, outR, outMono); break;
            case Warp::Mirror: oscLoop<(int)Warp::Mirror>(ps, sine, ph, o.inc[u], o.gl[u], o.gr[u], n, outL, outR, outMono); break;
            case Warp::Quantize: oscLoop<(int)Warp::Quantize>(ps, sine, ph, o.inc[u], o.gl[u], o.gr[u], n, outL, outR, outMono); break;
            case Warp::FM: oscLoop<(int)Warp::FM>(ps, sine, ph, o.inc[u], o.gl[u], o.gr[u], n, outL, outR, outMono); break;
            case Warp::Ring: oscLoop<(int)Warp::Ring>(ps, sine, ph, o.inc[u], o.gl[u], o.gr[u], n, outL, outR, outMono); break;
        }
    }
}

void Voice::render(float* L, float* R, int n) {
    if (!active_) return;
    const auto& sine = sineTable();
    Osc& A = osc_[0];
    Osc& B = osc_[1];
    const bool aOn = A.on, bOn = B.on;
    const bool crossMod = aOn && (A.warp == Warp::FM || A.warp == Warp::Ring);
    const int ramp0 = rampPos_;
    rampPos_ = std::min(ramp0 + n, kControl - 1);
    float aL[kControl] = {}, aR[kControl] = {}, bL[kControl] = {}, bR[kControl] = {}, bM[kControl] = {};
    if (bOn || crossMod) {
        renderOsc(B, n, ramp0, false, nullptr, bL, bR, crossMod ? bM : nullptr);
        if (crossMod) { const float inv = 1.f / (float)B.unison; for (int i = 0; i < n; ++i) bM[i] *= inv; }
    }
    if (aOn) renderOsc(A, n, ramp0, true, bM, aL, aR, nullptr);
    for (int i = 0; i < n; ++i) {
        const float fr = (float)std::min(ramp0 + i + 1, kControl) * (1.f / (float)kControl);
        const float la = A.levelPrev + (A.level - A.levelPrev) * fr, lb = bOn ? B.levelPrev + (B.level - B.levelPrev) * fr : 0.f;
        float sl = aL[i] * la + bL[i] * lb, sr = aR[i] * la + bR[i] * lb;
        const float subL = subLevelPrev_ + (subLevel_ - subLevelPrev_) * fr;
        float mono = 0.f;
        if (subL > 0.f) {
            float s;
            switch (subShape_) {
                case SubShape::Sine: s = sin2pi(sine, subPhase_); break;
                case SubShape::Triangle: s = subPhase_ < 0.5f ? 4.f * subPhase_ - 1.f : 3.f - 4.f * subPhase_; break;
                default: {
                    s = subPhase_ < 0.5f ? 1.f : -1.f;
                    s += polyBlep(subPhase_, subInc_) - polyBlep(wrap01(subPhase_ + 0.5f), subInc_);
                    s *= 0.7f;
                    break;
                }
            }
            mono += s * subL;
        }
        subPhase_ += subInc_; if (subPhase_ >= 1.f) subPhase_ -= 1.f;
        const float nzL = noiseLevelPrev_ + (noiseLevel_ - noiseLevelPrev_) * fr;
        if (nzL > 0.f) mono += noise_.tick(noiseColour_) * nzL;
        sl = (sl + mono) * kPreFilterGain; sr = (sr + mono) * kPreFilterGain;
        filter_.process(sl, sr);
        const float env = amp_.next();
        const float gl = gainLPrev_ + (gainL_ - gainLPrev_) * fr, gr = gainRPrev_ + (gainR_ - gainRPrev_) * fr;
        L[i] += sl * env * gl;
        R[i] += sr * env * gr;
    }
    if (!amp_.active()) { active_ = false; gate_ = false; }
}

}  // namespace infra
