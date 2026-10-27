#include "voice.h"

namespace ultra {

namespace {
// Level calibration per model: default dials, full velocity, peak near -6 dBFS (measured by ultra_tests --levels).
constexpr float kModelGain[kNumModels] = {0.47f, 0.5f, 0.32f, 0.62f, 1.33f, 1.27f, 0.44f, 0.5f, 0.44f, 0.58f, 0.88f, 0.6f, 0.7f};
constexpr float kBase[kNumModels] = {50.f, 45.f, 185.f, 1100.f, 1.f, 1.f, 110.f, 220.f, 560.f, 1700.f, 3000.f, 300.f, 180.f};
constexpr float kDecayLo[kNumModels] = {0.06f, 0.1f, 0.08f, 0.08f, 0.012f, 0.3f, 0.08f, 0.04f, 0.05f, 0.015f, 0.03f, 0.03f, 0.05f};
constexpr float kDecayHi[kNumModels] = {4.f, 6.f, 1.2f, 1.5f, 1.6f, 6.f, 2.f, 2.5f, 3.f, 0.4f, 4.f, 1.5f, 4.f};
// The 808's six square oscillators (cymbal and hats), in Hz at Tune 0.
constexpr float kMetal[6] = {205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f};
constexpr float kFmRatios[12] = {0.5f, 1.f, 1.5f, 2.f, 2.5f, 3.f, 3.5f, 4.f, 5.f, 6.f, 7.f, 8.f};
// Modal ratios: membrane (Bessel zeros), free bar, glass.
constexpr float kMembrane[6] = {1.f, 1.593f, 2.136f, 2.296f, 2.653f, 2.918f};
constexpr float kBar[6] = {1.f, 2.756f, 5.404f, 8.933f, 13.34f, 18.64f};
constexpr float kGlass[6] = {1.f, 2.32f, 4.25f, 6.63f, 9.38f, 12.1f};

inline float tri(float p) { return p < 0.25f ? 4.f * p : p < 0.75f ? 2.f - 4.f * p : 4.f * p - 4.f; }
inline float frac(float p) { return p - std::floor(p); }
inline float onePole(float hz, float fs) { return 1.f - std::exp(-kTwoPi * std::min(hz, 0.45f * fs) / fs); }
inline float tauCoef(float tau, float fs) { return std::exp(-1.f / (std::max(tau, 1e-5f) * fs)); }
}  // namespace

float DrumVoice::baseHz(Model m) { return kBase[(int)m]; }
void DrumVoice::decayRange(Model m, float& lo, float& hi) { lo = kDecayLo[(int)m]; hi = kDecayHi[(int)m]; }

void DrumVoice::prepare(double fs, uint32_t seed) {
    fs_ = fs; fsf_ = (float)fs; invFs_ = 1.f / fsf_;
    rng_.seed(seed);
    filter_.prepare(fs);
    active_ = false;
}

void DrumVoice::setFilter(FilterType type, float cutoffHz, float res) { filter_.set(type, cutoffHz * cutoffMul, res); }

void DrumVoice::choke(float ms) {
    if (!active_) return;
    const float steps = std::max(1.f, ms * 0.001f * fsf_);
    fadeStep_ = std::max(fadeStep_, fade_ / steps);
}

void DrumVoice::trigger(const VoiceParams& v, float decayMul, float velocityGain) {
    const bool wasActive = active_;
    p_ = v;
    p_.decay = std::clamp(v.decay, 0.f, 1.f); p_.tone = std::clamp(v.tone, 0.f, 1.f); p_.snap = std::clamp(v.snap, 0.f, 1.f);
    p_.color = std::clamp(v.color, 0.f, 1.f); p_.sweep = std::clamp(v.sweep, 0.f, 1.f); p_.drive = std::clamp(v.drive, 0.f, 1.f);
    decayMul_ = std::max(0.05f, decayMul);
    const int mi = std::clamp((int)p_.model, 0, kNumModels - 1);
    gain_ = velocityGain * kModelGain[mi];
    const float ratio = std::exp2(std::clamp(v.tune, -48.f, 48.f) / 12.f);
    f0_ = kBase[mi] * ratio;
    T_ = logMap(p_.decay, kDecayLo[mi], kDecayHi[mi]) * decayMul_;
    driveG_ = 1.f + 7.f * p_.drive * p_.drive;
    driveNorm_ = 0.5f / fastTanh(driveG_ * 0.5f);
    t_ = 0; fade_ = 1.f; fadeStep_ = 0.f;
    atk_ = 0.f; atkStep_ = 1.f / std::max(1.f, 0.0002f * fsf_);
    if (!wasActive) filter_.reset();
    active_ = true;
    mixInit = false;
    startModel();
}

void DrumVoice::startModel() {
    const float fs = fsf_;
    const auto& p = p_;
    env_ = 1.f; envC_ = decayCoef(T_, fs);
    e2_ = 0.f; tr_ = 0.f; pe_ = 1.f; ie_ = 1.f;
    for (auto& x : ph_) x = 0.f;
    for (auto& x : inc_) x = 0.f;
    lp1_ = 0.f; count_ = 0; n1_ = 0; n2_ = 0; fcNow_ = -1.f;
    s1_.reset(); s2_.reset(); s3_.reset();
    pinkB0_ = pinkB1_ = pinkB2_ = crackle_ = 0.f;
    modes_ = 0;
    switch (p.model) {
        case Model::CoreKick:
            sweepSt_ = p.sweep * 48.f; peC_ = tauCoef(0.004f + 0.035f * p.sweep, fs);
            tr_ = p.snap; trC_ = tauCoef(0.0015f, fs);
            lpC_ = onePole(1500.f * std::exp2(p.tone * 3.f), fs);
            k1_ = 1.f + 6.f * p.color; k2_ = 1.f / fastTanh(k1_);
            break;
        case Model::GravityKick:
            sweepSt_ = p.sweep * 36.f; peC_ = tauCoef(0.012f + 0.03f * p.sweep, fs);
            a_ = p.color * 6.f; ieC_ = tauCoef(0.008f + 0.15f * p.tone, fs);
            tr_ = p.snap; trC_ = tauCoef(0.0015f, fs);
            lpC_ = onePole(3000.f, fs);
            break;
        case Model::FlareSnare:
            sweepSt_ = p.sweep * 12.f; peC_ = tauCoef(0.02f, fs);
            envC_ = decayCoef(logMap(p.decay, 0.05f, 0.8f) * 0.6f * decayMul_, fs);       // body
            e2_ = 1.f; e2C_ = decayCoef(T_, fs);                                            // noise
            tr_ = p.snap * 2.5f; trC_ = tauCoef(0.005f, fs);
            c1_ = Svf::make(std::tan(kPi * std::min(1200.f * std::exp2(p.tone * 2.7f), 0.45f * fs) / fs), 0.9f);
            a_ = 1.1f * (1.f - p.color) + 0.15f; b_ = 1.3f * p.color + 0.1f;
            break;
        case Model::BurstClap: {
            const float fc = std::clamp(f0_ * std::exp2((p.tone - 0.5f) * 1.5f), 200.f, 0.45f * fs);
            c1_ = Svf::make(std::tan(kPi * fc / fs), 0.6f);
            b_ = std::sqrt(1100.f / fc);                                                     // noise power grows with the band
            n1_ = 1 + (int)std::lround(p.color * 5.f);                                      // bursts
            n2_ = std::max(1, (int)((0.006f + 0.008f * p.color) * fs));                     // spacing
            trC_ = tauCoef(0.0015f + 0.004f * (1.f - p.snap), fs);
            a_ = 0.25f + 0.6f * p.sweep;                                                    // tail level
            e2C_ = envC_;
            env_ = 0.f;
            break;
        }
        case Model::IonHat: case Model::CoronaCymbal: {
            const float ratio = f0_;                       // base 1 Hz: f0_ is the tune ratio
            for (int i = 0; i < 6; ++i) inc_[i] = std::min(kMetal[i] * ratio, 0.45f * fs) * invFs_;
            for (int i = 0; i < 6; ++i) ph_[i] = rng_.uni();
            tr_ = p.snap; trC_ = tauCoef(0.0025f, fs);
            if (p.model == Model::IonHat) {
                fcBase_ = 3000.f * std::exp2(p.tone * 2.2f);
                pe_ = 1.f; peC_ = tauCoef(0.03f, fs);
            } else {
                fcBase_ = 7000.f * std::exp2((p.tone - 0.5f) * 1.5f);
                c2_ = Svf::make(std::tan(kPi * std::min(fcBase_ * 0.6f, 0.45f * fs) / fs), 1.4f);
                c3_ = Svf::make(std::tan(kPi * std::min(3200.f * std::exp2(p.tone - 0.5f), 0.45f * fs) / fs), 1.0f);
                e2_ = 1.f; e2C_ = decayCoef(0.12f + 0.2f * p.decay, fs);
                c1_ = Svf::make(std::tan(kPi * std::min(fcBase_, 0.45f * fs) / fs), 0.8f);
            }
            break;
        }
        case Model::OrbitTom:
            sweepSt_ = p.sweep * 24.f; peC_ = tauCoef(0.04f, fs);
            e2_ = 0.35f * p.color; e2C_ = decayCoef(T_ * 0.5f, fs);
            tr_ = 0.6f * p.snap; trC_ = decayCoef(std::max(0.01f, T_ * 0.1f), fs);
            c1_ = Svf::make(std::tan(kPi * std::min(f0_ * 6.f * std::exp2(p.tone * 1.5f), 0.45f * fs) / fs), 0.9f);
            break;
        case Model::PulsarFm: {
            sweepSt_ = p.sweep * 24.f; peC_ = tauCoef(0.015f, fs);
            a_ = kFmRatios[std::clamp((int)std::lround(p.color * 11.f), 0, 11)];
            b_ = p.tone * 7.f; ieC_ = tauCoef(0.01f + 0.3f * T_, fs);
            c_ = p.snap * 5.f; tr_ = 1.f; trC_ = tauCoef(0.003f, fs);
            break;
        }
        case Model::QuasarMetal: {
            inc_[0] = std::min(f0_, 0.45f * fs) * invFs_;
            inc_[1] = std::min(f0_ * 1.4815f, 0.45f * fs) * invFs_;
            c1_ = Svf::make(std::tan(kPi * std::min(2400.f * std::exp2((p.tone - 0.5f) * 1.5f) * f0_ / 560.f, 0.45f * fs) / fs), 0.5f);
            e2_ = 0.6f; e2C_ = tauCoef(0.02f, fs);
            static const float br[4] = {1.f, 2.76f, 5.40f, 8.93f}, ba[4] = {1.f, 0.55f, 0.35f, 0.22f};
            for (int k = 0; k < 4; ++k) {
                const float f = f0_ * br[k] * (1.f + p.sweep * 0.04f * (float)k);
                inc_[2 + k] = f < 0.45f * fs ? f * invFs_ : 0.f;
                amp_[k] = ba[k] * (k == 0 ? 1.f : 0.3f + 0.7f * p.tone);
                pEnv_[k] = 1.f; pC_[k] = decayCoef(T_ / (1.f + 0.8f * (float)k), fs);
            }
            tr_ = p.snap; trC_ = tauCoef(0.002f, fs);
            break;
        }
        case Model::SparkRim: {
            const float w = kTwoPi * std::min(f0_, 0.4f * fs);
            const float k = std::clamp(13.8f / (w * T_), 0.005f, 1.5f);
            c1_ = Svf::make(std::tan(kPi * std::min(f0_, 0.45f * fs) / fs), k);
            c2_ = Svf::make(std::tan(kPi * std::min(f0_ * 0.48f, 0.45f * fs) / fs), std::min(1.5f, k * 1.6f));
            a_ = 0.112f / std::max(0.01f, std::tan(kPi * std::min(f0_, 0.45f * fs) / fs));   // same ping level at any pitch
            sweepSt_ = p.sweep * 12.f; peC_ = tauCoef(0.01f, fs);
            tr_ = 1.f; trC_ = tauCoef(0.0008f * (1.f + 2.f * p.snap), fs);
            break;
        }
        case Model::SolarWind: {
            fcBase_ = f0_;
            const float atkSec = std::min(1.5f, (1.f - p.snap) * (1.f - p.snap) * 0.4f * T_) + 0.0005f;
            n1_ = std::max(1, (int)(atkSec * fs));
            env_ = 0.f;
            e2_ = 0.f;                                                                      // attack ramp position
            pe_ = 1.f; peC_ = tauCoef(std::max(0.01f, T_ * 0.5f), fs);
            a_ = (p.sweep - 0.5f) * 2.f;                                                    // sweep direction
            k1_ = 1.4f - 1.1f * p.tone;
            break;
        }
        case Model::PlasmaZap:
            sweepSt_ = p.sweep * 72.f; peC_ = tauCoef(0.002f + 0.05f * (1.f - p.snap), fs);
            a_ = 0.5f - 0.45f * p.color;
            lpC_ = onePole(400.f * std::exp2(p.tone * 5.5f), fs);
            break;
        case Model::NebulaModal: {
            const float c = p.color;
            const float pos = 0.08f + 0.42f * p.sweep;
            float norm = 0.f;
            for (int k = 0; k < 6; ++k) {
                const float r = c < 0.5f ? std::exp(std::log(kMembrane[k]) * (1.f - 2.f * c) + std::log(kBar[k]) * (2.f * c))
                                         : std::exp(std::log(kBar[k]) * (2.f - 2.f * c) + std::log(kGlass[k]) * (2.f * c - 1.f));
                const float f = f0_ * r;
                if (f >= 0.45f * fs) break;
                const float Tk = T_ / (1.f + (float)k * (1.f - p.tone) * 1.5f);
                const float rr = std::exp(-6.9078f / (Tk * fs)), w = kTwoPi * f / fs;
                ra1_[k] = 2.f * rr * std::cos(w); ra2_[k] = rr * rr; rb_[k] = std::sin(w);
                amp_[k] = (std::abs(std::sin(kPi * (float)(k + 1) * pos)) + 0.05f) / (1.f + 0.3f * (float)k);
                norm += amp_[k];
                r1_[k] = r2_[k] = 0.f;
                ++modes_;
            }
            for (int k = 0; k < modes_; ++k) amp_[k] *= 1.6f / std::max(norm, 0.1f);
            tr_ = 1.f; trC_ = tauCoef(0.003f, fs);
            break;
        }
        case Model::Count: break;
    }
}

float DrumVoice::renderSample() {
    const float fs = fsf_;
    const auto& p = p_;
    float out = 0.f;
    switch (p.model) {
        case Model::CoreKick: {
            const float f = f0_ * std::exp2(sweepSt_ * pe_ * (1.f / 12.f));
            pe_ *= peC_;
            ph_[0] = frac(ph_[0] + f * invFs_);
            const float body = fastTanh(fastSin(ph_[0]) * k1_) * k2_;
            float click = 0.f;
            if (tr_ > 1e-5f) {
                const float src = (t_ == 0 ? 1.f : 0.f) + rng_.bi() * 0.5f;
                lp1_ += (src - lp1_) * lpC_;
                click = lp1_ * tr_ * 1.4f;
                tr_ *= trC_;
            }
            out = body * env_ + click;
            env_ *= envC_;
            break;
        }
        case Model::GravityKick: {
            const float f = f0_ * std::exp2(sweepSt_ * pe_ * (1.f / 12.f));
            pe_ *= peC_;
            ph_[0] = frac(ph_[0] + f * invFs_);
            const float idx = a_ * ie_; ie_ *= ieC_;
            const float body = fastSin(frac(ph_[0] + idx * 0.159155f * fastSin(ph_[0])));
            float click = 0.f;
            if (tr_ > 1e-5f) {
                const float src = (t_ == 0 ? 1.f : 0.f) + rng_.bi() * 0.4f;
                lp1_ += (src - lp1_) * lpC_;
                click = lp1_ * tr_ * 1.2f;
                tr_ *= trC_;
            }
            out = body * env_ + click;
            env_ *= envC_;
            break;
        }
        case Model::FlareSnare: {
            const float f = f0_ * std::exp2(sweepSt_ * pe_ * (1.f / 12.f));
            pe_ *= peC_;
            ph_[0] = frac(ph_[0] + f * invFs_);
            ph_[1] = frac(ph_[1] + f * 1.58f * invFs_);
            const float body = (fastSin(ph_[0]) + 0.6f * fastSin(ph_[1])) * 0.65f * env_;
            float lp, bp, hp;
            s1_.tick(rng_.bi(), c1_, lp, bp, hp);
            const float noise = (bp * 0.9f + hp * 0.35f) * e2_ * (1.f + tr_);
            out = body * a_ + noise * b_;
            env_ *= envC_; e2_ *= e2C_; tr_ *= trC_;
            break;
        }
        case Model::BurstClap: {
            if (count_ < n1_ && t_ >= (int64_t)count_ * n2_) {
                tr_ = 1.f;
                if (++count_ == n1_) env_ = a_;          // the tail starts with the last burst
            }
            const float n = s1_.bandpass(rng_.bi(), c1_);
            out = n * std::max(tr_, env_) * 2.2f * b_;
            tr_ *= trC_; env_ *= e2C_;
            break;
        }
        case Model::IonHat: {
            float metal = 0.f;
            for (int i = 0; i < 6; ++i) { metal += blepSquare(ph_[i], inc_[i]); ph_[i] = frac(ph_[i] + inc_[i]); }
            metal *= (1.f / 6.f);
            const float x = (1.f - p.color) * metal + p.color * rng_.bi() * 0.55f;
            if ((count_++ & 7) == 0) {
                const float fc = std::min(fcBase_ * (1.f + p.sweep * 1.5f * pe_), 0.45f * fs);
                c1_ = Svf::make(std::tan(kPi * fc / fs), 0.7f);
                c2_ = Svf::make(std::tan(kPi * std::min(fc * 0.6f, 0.45f * fs) / fs), 1.4f);
            }
            pe_ *= peC_;
            const float y = s2_.highpass(s1_.bandpass(x, c1_), c2_);
            out = y * env_ * (1.f + tr_);
            env_ *= envC_; tr_ *= trC_;
            break;
        }
        case Model::CoronaCymbal: {
            float s[6];
            for (int i = 0; i < 6; ++i) { s[i] = blepSquare(ph_[i], inc_[i]); ph_[i] = frac(ph_[i] + inc_[i]); }
            const float sum = (s[0] + s[1] + s[2] + s[3] + s[4] + s[5]) * (1.f / 6.f);
            const float rm = (s[0] * s[1] + s[2] * s[3] + s[4] * s[5]) * (0.8f / 3.f);
            const float metal = sum * (1.f - p.color) + rm * p.color;
            const float wash = s2_.highpass(s1_.bandpass(metal + rng_.bi() * 0.25f, c1_), c2_);
            const float stick = s3_.bandpass(metal, c3_);
            out = (wash * env_ * (0.4f + 0.6f * p.sweep) + stick * e2_ * (1.f - 0.7f * p.sweep)) * (1.f + tr_);
            env_ *= envC_; e2_ *= e2C_; tr_ *= trC_;
            break;
        }
        case Model::OrbitTom: {
            const float f = f0_ * std::exp2(sweepSt_ * pe_ * (1.f / 12.f));
            pe_ *= peC_;
            ph_[0] = frac(ph_[0] + f * invFs_);
            ph_[1] = frac(ph_[1] + f * 1.5f * invFs_);
            const float shape = p.color * 0.8f;
            const float body = fastSin(ph_[0]) * (1.f - shape) + tri(ph_[0]) * shape;
            const float skin = s1_.bandpass(rng_.bi(), c1_);
            out = body * env_ + fastSin(ph_[1]) * e2_ + skin * tr_;
            env_ *= envC_; e2_ *= e2C_; tr_ *= trC_;
            break;
        }
        case Model::PulsarFm: {
            const float f = f0_ * std::exp2(sweepSt_ * pe_ * (1.f / 12.f));
            pe_ *= peC_;
            ph_[0] = frac(ph_[0] + f * invFs_);
            ph_[1] = frac(ph_[1] + std::min(f * a_, 0.45f * fs) * invFs_);
            const float idx = b_ * ie_ + c_ * tr_;
            ie_ *= ieC_; tr_ *= trC_;
            out = fastSin(frac(ph_[0] + idx * 0.159155f * fastSin(ph_[1]))) * env_;
            env_ *= envC_;
            break;
        }
        case Model::QuasarMetal: {
            const float bell = p.color;
            float cow = 0.f;
            if (bell < 0.999f) {
                const float sq = blepSquare(ph_[0], inc_[0]) + blepSquare(ph_[1], inc_[1]);
                cow = s1_.bandpass(sq * 0.5f, c1_) * (e2_ + env_ * 0.55f) * 1.6f;
            }
            ph_[0] = frac(ph_[0] + inc_[0]); ph_[1] = frac(ph_[1] + inc_[1]);
            float b = 0.f;
            for (int k = 0; k < 4; ++k) {
                if (bell > 0.001f && inc_[2 + k] > 0.f) {
                    ph_[2 + k] = frac(ph_[2 + k] + inc_[2 + k]);
                    b += fastSin(ph_[2 + k]) * amp_[k] * pEnv_[k];
                }
                pEnv_[k] *= pC_[k];
            }
            out = (cow * (1.f - bell) + b * 0.55f * bell) * (1.f + tr_);
            env_ *= envC_; e2_ *= e2C_; tr_ *= trC_;
            break;
        }
        case Model::SparkRim: {
            const float exc = ((t_ == 0 ? 1.f : 0.f) + rng_.bi() * tr_ * (0.2f + 0.6f * p.snap)) * a_;
            tr_ *= trC_;
            const float r1 = s1_.bandpass(exc, c1_), r2 = s2_.bandpass(exc, c2_);
            const float rim = (r1 * (0.6f + 0.4f * p.tone) + r2 * (0.6f - 0.4f * p.tone)) * 2.5f;
            const float f = f0_ * 1.45f * std::exp2(sweepSt_ * pe_ * (1.f / 12.f));
            pe_ *= peC_;
            ph_[0] = frac(ph_[0] + std::min(f, 0.45f * fs) * invFs_);
            const float clave = fastSin(ph_[0]) * env_;
            out = rim * (1.f - p.color) + clave * p.color;
            env_ *= envC_;
            break;
        }
        case Model::SolarWind: {
            const float w = rng_.bi();
            pinkB0_ = 0.99765f * pinkB0_ + w * 0.0990460f; pinkB1_ = 0.96300f * pinkB1_ + w * 0.2965164f; pinkB2_ = 0.57000f * pinkB2_ + w * 1.0526913f;
            const float pink = (pinkB0_ + pinkB1_ + pinkB2_ + w * 0.1848f) * 0.25f;
            crackle_ *= 0.86f;
            if (rng_.uni() < 0.004f) crackle_ = rng_.bi();
            const float c = p.color;
            const float src = c < 0.5f ? w * (1.f - 2.f * c) + pink * 2.f * c : pink * (2.f - 2.f * c) + crackle_ * (2.f * c - 1.f);
            if ((count_++ & 7) == 0) {
                const float fc = std::clamp(fcBase_ * std::exp2(4.f * a_ * (1.f - pe_)), 40.f, 0.45f * fs);
                c1_ = Svf::make(std::tan(kPi * fc / fs), k1_);
            }
            pe_ *= peC_;
            // Linear attack (shaker vs riser), then the exponential decay.
            float amp;
            if (t_ < n1_) { amp = (float)t_ / (float)n1_; env_ = 1.f; e2_ = amp; }
            else { amp = env_; env_ *= envC_; e2_ = amp; }
            out = s1_.bandpass(src, c1_) * amp * 1.4f;
            break;
        }
        case Model::PlasmaZap: {
            const float f = std::min(f0_ * std::exp2(sweepSt_ * pe_ * (1.f / 12.f)), 0.45f * fs);
            pe_ *= peC_;
            ph_[0] = frac(ph_[0] + f * invFs_);
            const float ph = ph_[0], d = a_;
            const float pw = ph < d ? ph * 0.5f / d : 0.5f + (ph - d) * 0.5f / (1.f - d);
            lp1_ += (fastSin(pw) - lp1_) * lpC_;
            out = lp1_ * env_;
            env_ *= envC_;
            break;
        }
        case Model::NebulaModal: {
            float exc = (t_ == 0 ? 0.5f + 0.5f * p.snap : 0.f) + rng_.bi() * tr_ * (1.f - p.snap) * 0.15f;
            tr_ *= trC_;
            for (int k = 0; k < modes_; ++k) {
                const float y = rb_[k] * exc + ra1_[k] * r1_[k] - ra2_[k] * r2_[k];
                r2_[k] = r1_[k]; r1_[k] = y;
                out += y * amp_[k];
            }
            env_ *= envC_;
            break;
        }
        case Model::Count: break;
    }
    return out;
}

bool DrumVoice::finished() const {
    // Done when the longest component has fallen 80 dB (the clap waits for its last burst, the noise for its attack).
    bool done = env_ < 1e-4f && e2_ < 1e-4f && tr_ < 1e-4f;
    if (p_.model == Model::BurstClap) done = done && count_ >= n1_;
    if (p_.model == Model::SolarWind) done = t_ > n1_ && env_ < 1e-4f;
    if (p_.model == Model::QuasarMetal) done = done && pEnv_[0] < 1e-4f;
    if (p_.model == Model::NebulaModal) {
        done = env_ < 1e-4f;
        if (done) for (int k = 0; k < modes_; ++k) if (std::abs(r1_[k]) > 1e-5f) done = false;
    }
    return done;
}

void DrumVoice::render(float* out, int n) {
    if (!active_) return;
    const bool drive = p_.drive > 0.001f;
    for (int i = 0; i < n; ++i) {
        float x = renderSample();
        ++t_;
        if (drive) x = fastTanh(x * driveG_) * driveNorm_;
        x = filter_.process(x);
        if (atk_ < 1.f) { atk_ = std::min(1.f, atk_ + atkStep_); x *= atk_; }
        if (fadeStep_ > 0.f) {
            fade_ -= fadeStep_;
            if (fade_ <= 0.f) { active_ = false; return; }
            x *= fade_;
        }
        out[i] += x * gain_;
        // Checked every 64 samples from the trigger, so a hit ends at the same sample whatever the block size.
        if ((t_ & 63) == 0 && finished()) { active_ = false; return; }
    }
}

}  // namespace ultra
