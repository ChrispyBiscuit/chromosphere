#include "voice.h"

namespace ultra {

namespace {
// Level calibration per model: default dials, full velocity, peak near -6 dBFS (measured by ultra_tests --levels).
constexpr float kModelGain[kNumModels] = {0.485f, 0.275f, 0.236f, 0.595f, 1.44f, 0.728f, 0.44f, 0.526f, 0.403f, 0.515f, 0.70f, 0.493f, 0.95f};
constexpr float kBase[kNumModels] = {50.f, 52.f, 185.f, 1100.f, 1.f, 1.f, 110.f, 220.f, 560.f, 1700.f, 3000.f, 180.f, 220.f};
constexpr float kDecayLo[kNumModels] = {0.1f, 0.1f, 0.1f, 0.1f, 0.035f, 0.4f, 0.08f, 0.04f, 0.05f, 0.02f, 0.04f, 0.04f, 0.08f};
constexpr float kDecayHi[kNumModels] = {3.f, 3.f, 2.f, 2.5f, 2.f, 8.f, 3.f, 3.f, 3.5f, 0.6f, 5.f, 2.f, 6.f};
// The 808's six square oscillators (cymbal and hats), in Hz at Tune 0.
constexpr float kMetal[6] = {205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f};
constexpr float kFmRatios[12] = {0.5f, 1.f, 1.5f, 2.f, 2.5f, 3.f, 3.5f, 4.f, 5.f, 6.f, 7.f, 8.f};
// Modal ratios: membrane (Bessel zeros), free bar, glass.
constexpr float kMembrane[8] = {1.f, 1.593f, 2.136f, 2.296f, 2.653f, 2.918f, 3.156f, 3.501f};
constexpr float kBar[8] = {1.f, 2.756f, 5.404f, 8.933f, 13.34f, 18.64f, 24.81f, 31.87f};
constexpr float kGlass[8] = {1.f, 2.32f, 4.25f, 6.63f, 9.38f, 12.1f, 15.3f, 18.8f};
// Ride bell partials (relative to the bell's fundamental) and their levels.
constexpr float kRideBell[4] = {1.f, 1.505f, 2.31f, 3.27f};
constexpr float kRideBellAmp[4] = {1.f, 0.6f, 0.45f, 0.3f};

inline float tauCoef(float tau, float fs) { return std::exp(-1.f / (std::max(tau, 1e-5f) * fs)); }

// sin(2 pi p) for any p: folded to a quarter wave, then a degree-9 Taylor series (error about 4e-6). Kicks and
// bells need a cleaner sine than fastSin.
inline float sin2pi(float p) {
    p -= std::floor(p);
    float x = p < 0.5f ? p : p - 1.f;
    if (x > 0.25f) x = 0.5f - x;
    else if (x < -0.25f) x = -0.5f - x;
    const float z = x * kTwoPi, z2 = z * z;
    return z * (1.f + z2 * (-1.f / 6.f + z2 * (1.f / 120.f + z2 * (-1.f / 5040.f + z2 * (1.f / 362880.f)))));
}
// The same for a phase already in [0, 1).
inline float sin2piU(float p) {
    float x = p < 0.5f ? p : p - 1.f;
    if (x > 0.25f) x = 0.5f - x;
    else if (x < -0.25f) x = -0.5f - x;
    const float z = x * kTwoPi, z2 = z * z;
    return z * (1.f + z2 * (-1.f / 6.f + z2 * (1.f / 120.f + z2 * (-1.f / 5040.f + z2 * (1.f / 362880.f)))));
}
// 2^x, relative error about 1e-4 (pitch envelopes in octaves).
inline float fastExp2(float x) {
    x = std::clamp(x, -60.f, 60.f);
    const float fi = std::floor(x), f = x - fi;
    const float p = 1.f + f * (0.6931472f + f * (0.2402265f + f * (0.0555041f + f * 0.0096181f)));
    return std::ldexp(p, (int)fi);
}
}  // namespace

float DrumVoice::baseHz(Model m) { return kBase[(int)m]; }
void DrumVoice::decayRange(Model m, float& lo, float& hi) { lo = kDecayLo[(int)m]; hi = kDecayHi[(int)m]; }

void DrumVoice::prepare(double fs, uint32_t seed) {
    fs_ = fs; fsf_ = (float)fs; invFs_ = 1.f / fsf_; nyq_ = 0.45f * fsf_;
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

void DrumVoice::retune(int i, float hz, float k) { c_[i] = Svf::make(std::tan(kPi * std::clamp(hz, 10.f, nyq_) * invFs_), k); }

void DrumVoice::trigger(const VoiceParams& v, float decayMul, float velocityGain) {
    const bool wasActive = active_;
    p_ = v;
    p_.decay = std::clamp(v.decay, 0.f, 1.f); p_.tone = std::clamp(v.tone, 0.f, 1.f); p_.snap = std::clamp(v.snap, 0.f, 1.f);
    p_.color = std::clamp(v.color, 0.f, 1.f); p_.sweep = std::clamp(v.sweep, 0.f, 1.f); p_.drive = std::clamp(v.drive, 0.f, 1.f);
    decayMul_ = std::max(0.05f, decayMul);
    const int mi = std::clamp((int)p_.model, 0, kNumModels - 1);
    // Analog variation per hit: about +-2 cents, +-3% decay, +-0.3 dB.
    const float hp = rng_.bi(), hd = rng_.bi(), hl = rng_.bi();
    gain_ = velocityGain * kModelGain[mi] * (1.f + 0.035f * hl);
    const float ratio = std::exp2(std::clamp(v.tune, -48.f, 48.f) / 12.f + hp * (2.f / 1200.f));
    f0_ = kBase[mi] * ratio;
    T_ = logMap(p_.decay, kDecayLo[mi], kDecayHi[mi]) * decayMul_ * (1.f + 0.03f * hd);
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
    for (int i = 0; i < kEnv; ++i) { e_[i] = 0.f; ec_[i] = 0.f; }
    for (int i = 0; i < kOsc; ++i) { ph_[i] = 0.f; inc_[i] = 0.f; }
    for (auto& s : s_) s.reset();
    for (auto& x : k_) x = 0.f;
    for (auto& x : lp_) x = 0.f;
    for (int i = 0; i < kRes; ++i) r1_[i] = r2_[i] = ramp_[i] = 0.f;
    modes_ = 0; levelEnvs_ = 0x3f; curved_ = false; count_ = 0; n1_ = 0; n2_ = 0; next_ = 0; last_ = 0.f;
    auto env = [&](int i, float start, float t60) { e_[i] = start; ec_[i] = decayCoef(t60, fs); };
    // Master envelope whose log-decay rate rises linearly: q = 0 is exponential, q = 1 Gaussian; -60 dB at t60.
    auto curvedEnv = [&](float t60, float q) {
        const double r0 = 6.9078 * (1.0 - q) / t60, r1 = 2.0 * 6.9078 * q / ((double)t60 * t60);
        curved_ = true; cEnv_ = 1.0; cCoef_ = std::exp(-r0 / fs_); cCoef2_ = std::exp(-r1 / (fs_ * fs_));
        e_[0] = 1.f; ec_[0] = 1.f;
    };
    auto envTau = [&](int i, float start, float tau) { e_[i] = start; ec_[i] = tauCoef(tau, fs); };
    // A two-pole resonator at hz falling 60 dB in t60; returns false above the band limit.
    auto resonator = [&](int k, float hz, float t60, float amp) {
        if (hz >= nyq_ || k >= kRes) return false;
        const float r = std::exp(-6.9078f / (std::max(t60, 1e-3f) * fs)), w = kTwoPi * hz * invFs_;
        ra1_[k] = 2.f * r * std::cos(w); ra2_[k] = r * r; rb_[k] = std::sin(w); ramp_[k] = amp;
        r1_[k] = r2_[k] = 0.f;
        return true;
    };

    switch (p.model) {
        case Model::CoreKick: {
            // Analog house kick: a sine whose pitch falls in two stages (a fast chirp, then the body settling),
            // saturated after the envelope so Shape adds weight and sustain, not just harmonics.
            const float sw = p.sweep;
            curvedEnv(T_, 0.55f);                                                     // amplitude: holds, then falls
            envTau(1, 1.f, 0.0012f + 0.003f * sw);                                    // chirp
            envTau(2, 1.f, 0.012f + 0.05f * sw);                                      // body pitch
            envTau(3, p.snap * p.snap * 1.4f, 0.0009f + 0.0012f * p.tone);            // beater click
            k_[0] = 1.5f + 8.f * sw; k_[1] = 0.25f + 1.f * sw;
            k_[2] = 1.f + 7.f * std::pow(p.color, 1.5f); k_[3] = 1.f / fastTanh(k_[2]);
            retune(0, 200.f * std::exp2(p.tone * 6.f), 1.414f);                       // tone low-pass (body)
            retune(1, 3800.f * std::exp2(p.tone - 0.5f), 1.2f);                       // click band
            break;
        }
        case Model::GravityKick: {
            // Hard kick (trance, techno, drum and bass): a steeper chirp, a clean sub in parallel with a two-stage
            // distortion whose brightness Tone sets, and a layered top (a falling "tok" and a noise tick).
            const float sw = p.sweep;
            curvedEnv(T_, 0.7f);
            envTau(1, 1.f, 0.0007f + 0.0018f * sw);
            envTau(2, 1.f, 0.018f + 0.06f * sw);
            envTau(3, p.snap * 0.75f, 0.004f);                                       // top "tok"
            envTau(4, 1.f, 0.0025f);                                                  // tok pitch
            envTau(5, p.snap * 0.6f, 0.0012f);                                       // noise tick
            k_[0] = 4.f + 10.f * sw; k_[1] = 0.4f + 1.4f * sw;
            k_[2] = 1.5f + 22.f * p.color * p.color;
            k_[4] = 1.f - 0.45f * p.color;                                            // clean sub share
            k_[5] = 0.35f + 0.6f * p.color;                                           // distortion share
            retune(0, 120.f * std::exp2(p.tone * 7.3f), 1.2f);
            retune(1, 6000.f, 1.4f);
            break;
        }
        case Model::FlareSnare: {
            // Two drum-head modes with a pitch drop, snare-wire noise (high-pass plus a presence band, modulated
            // by the head), a fast crack and a tail. Noise sets the head / wire balance.
            const float sw = p.sweep;
            env(0, 1.f, T_);                                                          // wire tail
            env(1, 1.f, (0.12f + 0.35f * p.decay) * decayMul_);                       // head
            envTau(2, 1.f, 0.01f + 0.02f * sw);                                       // head pitch
            envTau(3, p.snap * 1.3f, 0.0025f);                                        // crack
            envTau(4, 1.f, 0.012f);                                                   // early wire burst
            k_[0] = 0.08f + 0.9f * sw;
            k_[1] = 1.05f - 0.8f * p.color; k_[2] = 0.25f + 1.0f * p.color;
            retune(0, 900.f * std::exp2(p.tone * 2.2f), 1.414f);
            retune(1, 3000.f * std::exp2(p.tone * 1.5f), 0.8f);
            retune(2, 2500.f, 1.2f);
            break;
        }
        case Model::BurstClap: {
            // Several hands: band-passed noise bursts at slightly random spacing, each one a little detuned, then
            // a room tail from the last one. Bursts sets how many and how wide; Room the tail level.
            n1_ = 2 + (int)std::lround(p.color * 4.f);
            n2_ = std::max(8, (int)((0.0055f + 0.005f * p.color) * fs));
            next_ = 0;
            k_[0] = 0.25f + 0.65f * p.sweep;                                          // tail level
            k_[1] = 1.f / (0.9f + 1.6f * p.tone);                                     // band 1 / Q
            k_[2] = tauCoef(0.0012f + 0.004f * (1.f - p.snap), fs);                   // burst decay
            k_[3] = decayCoef(T_, fs);
            k_[4] = 0.15f + 0.5f * p.tone;                                            // sizzle (presence)
            retune(0, f0_, k_[1]);
            retune(1, f0_ * 0.88f, 1.25f);
            retune(2, 3000.f, 1.2f);
            retune(3, 350.f, 1.3f);                                                   // no low mud
            break;
        }
        case Model::IonHat: {
            // Six 808 squares and noise (Noise sets the mix) through a steep high-pass and a sheen band. Sweep starts
            // the filter brighter and closes it over the hit.
            for (int i = 0; i < 6; ++i) { inc_[i] = std::min(kMetal[i] * f0_, nyq_) * invFs_; ph_[i] = rng_.uni(); }
            env(0, 1.f + 0.06f * rng_.bi(), T_);
            envTau(2, 1.f, 0.025f);                                                   // filter sweep
            envTau(3, p.snap * 0.9f, 0.0012f);                                        // attack spike
            k_[0] = 4500.f * std::exp2(p.tone * 1.2f - 0.3f);
            k_[1] = 1.2f * p.sweep;
            retune(3, 8500.f * std::exp2(p.tone), 1.3f);                              // air roll-off: no fizz
            k_[4] = std::cos(p.color * kPi * 0.5f); k_[5] = std::sin(p.color * kPi * 0.5f);   // equal-power metal / noise
            break;
        }
        case Model::CoronaCymbal: {
            // Ride to crash: stick ping, bell partials and a wash of metal and noise. Shimmer moves ride to crash;
            // Wash sets the wash level; a soft Attack swells the crash in.
            for (int i = 0; i < 6; ++i) { inc_[i] = std::min(kMetal[i] * 1.55f * f0_, nyq_) * invFs_; ph_[i] = rng_.uni(); }
            const float bellHz = 560.f * f0_;
            for (int k = 0; k < 2; ++k) inc_[6 + k] = std::min(bellHz * kRideBell[k], nyq_) * invFs_;
            k_[6] = std::min(bellHz * kRideBell[2], nyq_) * invFs_; k_[7] = std::min(bellHz * kRideBell[3], nyq_) * invFs_;
            env(0, 1.f, T_);                                                          // wash
            env(1, 1.f, (0.12f + 0.25f * p.decay) * decayMul_);                       // stick
            env(2, 1.f, T_ * 0.45f);                                                  // bell
            envTau(3, p.snap * 0.6f, 0.002f);
            const float c = p.color;
            k_[0] = 0.25f + 0.75f * p.sweep;                                          // wash level
            k_[1] = 1.f - 0.6f * c;                                                   // stick level
            k_[2] = (1.f - c) * 0.45f;                                                // bell level
            n1_ = std::max(1, (int)(((1.f - p.snap) * 0.014f * c + 0.0005f) * fs));   // swell
            const float fw = 5200.f * std::exp2((p.tone - 0.5f) * 1.2f);
            retune(0, fw, 1.0f);
            retune(1, fw * 1.6f, 1.4f);
            retune(2, 3400.f * std::exp2(p.tone - 0.5f), 0.9f);
            retune(3, 320.f, 1.3f);
            break;
        }
        case Model::OrbitTom: {
            // Membrane: three modes whose pitch bends down as the head relaxes, a stick strike and (towards conga)
            // a slap. Shape moves tom to conga.
            const float c = p.color;
            const float r2 = 1.593f + (1.5f - 1.593f) * c, r3 = 2.136f + (2.f - 2.136f) * c;
            curvedEnv(T_, 0.35f);
            env(1, 0.45f - 0.15f * c, T_ / 2.2f);
            env(2, 0.25f + 0.1f * c, T_ / 3.5f);
            envTau(3, 0.25f + 0.9f * p.snap, 0.002f + 0.004f * (1.f - p.snap));      // strike
            envTau(4, 1.f, 0.015f + 0.03f * std::min(T_, 1.f));                      // pitch bend
            envTau(5, c * 0.6f * (0.3f + 0.7f * p.snap), 0.008f);                     // slap
            k_[0] = 0.04f + 0.9f * std::pow(p.sweep, 1.5f);
            k_[1] = r2; k_[2] = r3;
            retune(0, f0_ * 7.f * std::exp2(p.tone * 1.5f - 0.75f), 1.0f);
            retune(1, 2500.f * std::exp2(p.tone), 1.2f);
            break;
        }
        case Model::PulsarFm: {
            // Two-operator FM with a little modulator feedback: Index sets the body brightness and its decay, Attack
            // adds a short index burst, Ratio picks the modulator ratio, Sweep a pitch drop.
            k_[0] = kFmRatios[std::clamp((int)std::lround(p.color * 11.f), 0, 11)];
            k_[1] = p.tone * 7.f; k_[2] = p.snap * 9.f;
            k_[3] = p.sweep * 2.f;                                                    // octaves
            env(0, 1.f, T_);
            envTau(1, 1.f, 0.008f + 0.35f * T_);
            envTau(2, 1.f, 0.015f);
            envTau(3, 1.f, 0.0025f);
            levelEnvs_ = 1u;                                                          // the rest shape the index and pitch
            break;
        }
        case Model::QuasarMetal: {
            // 808 cowbell (two squares through a band-pass, a fast drop then a tail) to an FM bell with a detuned
            // twin (Detune) for beating.
            inc_[0] = std::min(f0_, nyq_) * invFs_;
            inc_[1] = std::min(f0_ * 1.4815f, nyq_) * invFs_;
            inc_[2] = std::min(f0_, nyq_) * invFs_;                                   // bell carrier
            inc_[3] = std::min(f0_ * 3.5f, nyq_) * invFs_;                            // bell modulator
            inc_[4] = std::min(f0_ * (1.f + 0.006f * p.sweep), nyq_) * invFs_;        // twin
            env(0, 1.f, T_);
            env(1, 1.f, 0.05f);
            envTau(2, 1.f, 0.06f + 0.4f * T_);                                        // bell index
            envTau(3, p.snap * 0.8f, 0.0015f);
            levelEnvs_ = 0b1011u;
            k_[0] = 2.5f * (0.4f + p.tone);
            retune(0, f0_ * 1.8f * std::exp2((p.tone - 0.5f) * 2.f), 0.8f);
            retune(1, 300.f, 1.4f);
            break;
        }
        case Model::SparkRim: {
            // Rimshot to clave: an impulse and a short noise tick into wooden resonators, high-passed. Body sets the
            // level of the lower modes.
            const float c = p.color, T = T_;
            const float body = 0.15f + 1.2f * p.sweep;                               // the woody low modes
            const float rimHz[3] = {0.3f, 0.62f, 1.f}, rimAmp[3] = {0.5f * body, 0.67f * body, 1.f};
            for (int k = 0; k < 3; ++k) if (resonator(modes_, f0_ * rimHz[k], T / (1.f + 0.4f * (float)k), rimAmp[k] * (1.f - c))) ++modes_;
            const float clave = std::min(T * 1.6f, 0.8f);
            if (resonator(modes_, f0_ * 1.47f, clave, 1.3f * c)) ++modes_;
            env(0, 1.f, std::max(T, clave));                                          // the longest mode
            envTau(1, 1.f, 0.0006f + 0.002f * p.snap);                                // excitation noise
            k_[0] = (0.3f + 0.7f * p.snap) * 0.5f;
            k_[1] = 0.4f * p.snap;
            k_[2] = 0.85f / std::max(1.f, (1.f - c) * 2.4f + c * 1.3f);               // keep the sum near 1
            retune(0, 250.f, 1.2f);
            retune(1, 2000.f * std::exp2(p.tone * 3.5f), 1.2f);
            retune(2, 3000.f, 1.2f);
            break;
        }
        case Model::SolarWind: {
            // Shaker, noise hit or riser: noise (white, pink or granular beads, set by Colour) through a band-pass
            // that sweeps up or down; Attack sets shaker attack against a slow swell.
            const float atkSec = std::min(2.f, (1.f - p.snap) * (1.f - p.snap) * 0.5f * T_) + 0.0005f;
            n1_ = std::max(1, (int)(atkSec * fs));
            env(0, 1.f, T_);
            envTau(2, 1.f, std::max(0.01f, T_ * 0.5f));
            k_[0] = (p.sweep - 0.5f) * 2.f;
            k_[1] = 1.5f - 1.3f * p.tone;
            k_[2] = 1.4f * std::sqrt(k_[1] / 0.85f);                                  // same loudness at any resonance
            break;
        }
        case Model::PlasmaZap:
            // Laser: a shaped oscillator diving from up to six octaves above the pitch, through a resonant low-pass.
            env(0, 1.f, T_);
            envTau(1, 1.f, 0.002f + 0.06f * (1.f - p.snap));
            levelEnvs_ = 1u;
            k_[0] = std::min(p.sweep * 6.f, std::log2(nyq_ / f0_));                   // never starts above the band
            k_[1] = 0.5f - 0.45f * p.color;
            k_[2] = std::exp2(p.tone * 5.f);                                          // low-pass, in multiples of the pitch
            break;
        case Model::NebulaModal: {
            // Eight modes, membrane to bar to glass (Material), struck by a mallet whose width Strike sets, at a
            // position along the body (Position). Damping shortens the upper modes.
            const float c = p.color, pos = 0.08f + 0.42f * p.sweep;
            float norm = 0.f;
            for (int k = 0; k < kRes; ++k) {
                const float r = c < 0.5f ? std::exp(std::log(kMembrane[k]) * (1.f - 2.f * c) + std::log(kBar[k]) * (2.f * c))
                                         : std::exp(std::log(kBar[k]) * (2.f - 2.f * c) + std::log(kGlass[k]) * (2.f * c - 1.f));
                const float Tk = T_ / (1.f + (float)k * (1.f - p.tone) * 1.5f);
                const float a = (std::abs(std::sin(kPi * (float)(k + 1) * pos)) + 0.05f) / (1.f + 0.3f * (float)k);
                if (!resonator(modes_, f0_ * r, Tk, a)) break;
                norm += a; ++modes_;
            }
            for (int k = 0; k < modes_; ++k) ramp_[k] *= 1.6f / std::max(norm, 0.1f);
            n1_ = std::max(1, (int)((0.0002f + 0.004f * (1.f - p.snap) * (1.f - p.snap)) * fs));   // mallet width
            k_[0] = kPi / (2.f * (float)n1_);
            env(0, 1.f, T_);
            break;
        }
        case Model::Count: break;
    }
}

float DrumVoice::renderSample() {
    const auto& p = p_;
    float out = 0.f;
    switch (p.model) {
        case Model::CoreKick: {
            const float f = std::min(f0_ * (1.f + k_[0] * e_[1] + k_[1] * e_[2]), nyq_);
            ph_[0] += f * invFs_; if (ph_[0] >= 1.f) ph_[0] -= 1.f;
            float body = fastTanh(k_[2] * sin2piU(ph_[0]) * e_[0]) * k_[3];
            body = s_[0].lowpassTick(body, c_[0]);
            float click = 0.f;
            if (e_[3] > 1e-6f) click = s_[1].bandpass(rng_.bi() * 0.6f + (t_ == 0 ? 4.f : 0.f), c_[1]) * e_[3];
            out = body + click;
            break;
        }
        case Model::GravityKick: {
            const float f = std::min(f0_ * (1.f + k_[0] * e_[1] + k_[1] * e_[2]), nyq_);
            ph_[0] += f * invFs_; if (ph_[0] >= 1.f) ph_[0] -= 1.f;
            const float clean = sin2piU(ph_[0]) * e_[0];
            const float d1 = fastTanh(k_[2] * clean);
            const float d2 = fastTanh(2.f * s_[0].lowpassTick(d1, c_[0]));
            float top = 0.f;
            if (e_[3] > 1e-6f || e_[5] > 1e-6f) {
                ph_[1] += std::min(900.f * (1.f + 2.f * e_[4]), nyq_) * invFs_; if (ph_[1] >= 1.f) ph_[1] -= 1.f;
                top = sin2piU(ph_[1]) * e_[3] + s_[1].highpass(rng_.bi(), c_[1]) * e_[5];
            }
            out = clean * k_[4] + d2 * k_[5] + top;
            break;
        }
        case Model::FlareSnare: {
            const float f = f0_ * (1.f + k_[0] * e_[2]);
            ph_[0] += f * invFs_; if (ph_[0] >= 1.f) ph_[0] -= 1.f;
            ph_[1] += std::min(f * 1.62f, nyq_) * invFs_; if (ph_[1] >= 1.f) ph_[1] -= 1.f;
            const float head = fastTanh(1.6f * (sin2piU(ph_[0]) + 0.5f * sin2piU(ph_[1])) * e_[1]) * 1.08f;
            const float n = rng_.bi();
            const float wires = (s_[0].highpass(n, c_[0]) * 0.8f + s_[1].bandpass(n, c_[1]) * 0.6f) * (0.75f + 0.5f * std::abs(head));
            const float crack = e_[3] > 1e-6f ? s_[2].highpass(n + (t_ == 0 ? 2.f : 0.f), c_[2]) * e_[3] : 0.f;
            out = head * k_[1] + wires * (0.55f * e_[0] + 0.45f * e_[4]) * k_[2] + crack;
            break;
        }
        case Model::BurstClap: {
            if (count_ < n1_ && t_ >= next_) {
                ++count_;
                const bool last = count_ == n1_;
                e_[1] = last ? 1.f : 0.8f + 0.2f * rng_.uni(); ec_[1] = k_[2];
                retune(0, f0_ * ((count_ & 1) ? 1.06f : 0.94f) * (1.f + 0.03f * rng_.bi()), k_[1]);
                next_ += (int)((float)n2_ * (0.75f + 0.5f * rng_.uni()));
                if (last) { e_[0] = k_[0]; ec_[0] = k_[3]; }
            }
            const float n = rng_.bi();
            float y = s_[0].bandpass(n, c_[0]) * e_[1] * 1.6f + s_[1].bandpass(n, c_[1]) * e_[0] * 1.4f;
            if (k_[4] > 0.f) y += s_[2].highpass(n, c_[2]) * (e_[1] + e_[0]) * k_[4];
            out = fastTanh(1.3f * s_[3].highpass(y, c_[3]));
            break;
        }
        case Model::IonHat: {
            float metal = 0.f;
            if (k_[4] > 1e-3f)
                for (int i = 0; i < 6; ++i) { metal += blepSquare(ph_[i], inc_[i]); ph_[i] += inc_[i]; if (ph_[i] >= 1.f) ph_[i] -= 1.f; }
            const float src = metal * (1.f / 6.f) * k_[4] + (k_[5] > 1e-3f ? rng_.bi() * 0.55f * k_[5] : 0.f);
            if ((count_++ & 7) == 0 && (count_ == 1 || k_[1] * e_[2] > 1e-3f)) {   // the sweep settles, then the filter stays
                const float fh = k_[0] * (1.f + k_[1] * e_[2]);
                retune(0, fh, 1.2f); retune(1, fh, 1.6f); retune(2, fh * 1.5f, 0.7f);
            }
            const float hp = s_[1].highpass(s_[0].highpass(src, c_[0]), c_[1]);
            const float y = hp * 0.9f + s_[2].bandpass(hp, c_[2]) * 0.5f;
            out = s_[3].lowpassTick(y, c_[3]) * (e_[0] + e_[3]);
            break;
        }
        case Model::CoronaCymbal: {
            float s[6];
            for (int i = 0; i < 6; ++i) { s[i] = blepSquare(ph_[i], inc_[i]); ph_[i] += inc_[i]; if (ph_[i] >= 1.f) ph_[i] -= 1.f; }
            const float sum = (s[0] + s[1] + s[2] + s[3] + s[4] + s[5]) * (1.f / 6.f);
            const float rm = (s[0] * s[1] + s[2] * s[3] + s[4] * s[5]) * (1.f / 3.f);
            const float metal = sum * (1.f - 0.5f * p.color) + rm * (0.5f + 0.5f * p.color);
            const float n = rng_.bi();
            const float washIn = metal + n * 0.35f;
            const float wash = s_[0].highpass(washIn, c_[0]) * 0.8f + s_[1].bandpass(washIn, c_[1]) * 0.4f;
            const float swell = t_ < n1_ ? (float)t_ / (float)n1_ : 1.f;
            const float stick = s_[2].bandpass(metal, c_[2]);
            float bell = 0.f;
            if (k_[2] > 0.001f && e_[2] > 1e-5f) {
                ph_[6] += inc_[6]; if (ph_[6] >= 1.f) ph_[6] -= 1.f;
                ph_[7] += inc_[7]; if (ph_[7] >= 1.f) ph_[7] -= 1.f;
                lp_[0] += k_[6]; if (lp_[0] >= 1.f) lp_[0] -= 1.f;
                lp_[1] += k_[7]; if (lp_[1] >= 1.f) lp_[1] -= 1.f;
                bell = (sin2piU(ph_[6]) * kRideBellAmp[0] + sin2piU(ph_[7]) * kRideBellAmp[1] + sin2piU(lp_[0]) * kRideBellAmp[2] +
                        sin2piU(lp_[1]) * kRideBellAmp[3]) * 0.4f;
            }
            out = s_[3].highpass(wash * e_[0] * k_[0] * swell + stick * e_[1] * k_[1], c_[3]) + bell * e_[2] * k_[2];
            out *= 1.f + e_[3];
            break;
        }
        case Model::OrbitTom: {
            const float f = f0_ * (1.f + k_[0] * e_[4]);
            ph_[0] += std::min(f, nyq_) * invFs_; if (ph_[0] >= 1.f) ph_[0] -= 1.f;
            ph_[1] += std::min(f * k_[1], nyq_) * invFs_; if (ph_[1] >= 1.f) ph_[1] -= 1.f;
            ph_[2] += std::min(f * k_[2], nyq_) * invFs_; if (ph_[2] >= 1.f) ph_[2] -= 1.f;
            const float body = sin2piU(ph_[0]) * e_[0] + sin2piU(ph_[1]) * e_[1] + sin2piU(ph_[2]) * e_[2];
            float hit = 0.f;
            if (e_[3] > 1e-6f || e_[5] > 1e-6f) {
                const float n = rng_.bi();
                hit = s_[0].bandpass(n + (t_ == 0 ? 3.f : 0.f), c_[0]) * e_[3] + s_[1].highpass(n, c_[1]) * e_[5];
            }
            out = fastTanh(1.2f * (body + hit)) * 1.2f;
            break;
        }
        case Model::PulsarFm: {
            if ((count_++ & 7) == 0) {
                k_[8] = std::min(f0_ * fastExp2(k_[3] * e_[2]), nyq_);
                k_[9] = std::max(0.f, (nyq_ - k_[8]) / std::max(k_[8] * k_[0], 1.f) - 1.f);
            }
            const float f = k_[8], fm = f * k_[0], iMax = k_[9];
            const float idx = std::min(k_[1] * e_[1] + k_[2] * e_[3], iMax);
            ph_[0] += f * invFs_; if (ph_[0] >= 1.f) ph_[0] -= 1.f;
            ph_[1] += std::min(fm, nyq_) * invFs_; if (ph_[1] >= 1.f) ph_[1] -= 1.f;
            const float mod = sin2pi(ph_[1] + 0.12f * last_);
            last_ = mod;
            out = sin2pi(ph_[0] + idx * 0.159155f * mod) * e_[0];
            break;
        }
        case Model::QuasarMetal: {
            const float c = p.color;
            float cow = 0.f, bell = 0.f;
            if (c < 0.999f) {
                const float sq = (blepSquare(ph_[0], inc_[0]) + blepSquare(ph_[1], inc_[1])) * 0.5f;
                const float bp = s_[0].bandpass(sq, c_[0]);
                cow = (bp * 1.2f + s_[1].highpass(sq, c_[1]) * 0.15f) * (0.7f * e_[1] + 0.45f * e_[0]);
            }
            ph_[0] += inc_[0]; if (ph_[0] >= 1.f) ph_[0] -= 1.f;
            ph_[1] += inc_[1]; if (ph_[1] >= 1.f) ph_[1] -= 1.f;
            if (c > 0.001f) {
                for (int i = 2; i < 5; ++i) { ph_[i] += inc_[i]; if (ph_[i] >= 1.f) ph_[i] -= 1.f; }
                const float m = sin2pi(ph_[3]) * k_[0] * e_[2] * 0.159155f;
                bell = (sin2pi(ph_[2] + m) + sin2pi(ph_[4] + m)) * 0.5f * e_[0];
            }
            out = (cow * (1.f - c) + bell * c) * (1.f + e_[3]);
            break;
        }
        case Model::SparkRim: {
            const float n = rng_.bi();
            const float exc = (t_ == 0 ? 1.f : 0.f) + n * e_[1] * k_[0];
            float y = 0.f;
            for (int k = 0; k < modes_; ++k) {
                const float r = rb_[k] * exc + ra1_[k] * r1_[k] - ra2_[k] * r2_[k];
                r2_[k] = r1_[k]; r1_[k] = r;
                y += r * ramp_[k];
            }
            y = s_[1].lowpassTick(s_[0].highpass(y * k_[2] * 2.2f, c_[0]), c_[1]);
            out = fastTanh(1.3f * (y + s_[2].highpass(n, c_[2]) * e_[1] * k_[1]));
            break;
        }
        case Model::SolarWind: {
            const float w = rng_.bi();
            // Pink (Paul Kellet's three-pole economy filter) and granular beads.
            lp_[0] = 0.99765f * lp_[0] + w * 0.0990460f; lp_[1] = 0.96300f * lp_[1] + w * 0.2965164f; lp_[2] = 0.57000f * lp_[2] + w * 1.0526913f;
            const float pink = (lp_[0] + lp_[1] + lp_[2] + w * 0.1848f) * 0.25f;
            const float c = p.color;
            float src;
            if (c < 0.5f) src = w * (1.f - 2.f * c) + pink * 2.f * c;
            else {
                const float bead = rng_.uni() < 0.06f ? rng_.bi() * 2.2f : 0.f;
                src = pink * (2.f - 2.f * c) + bead * (2.f * c - 1.f);
            }
            if ((count_++ & 7) == 0 && (count_ == 1 || std::abs(k_[0]) > 0.01f)) retune(0, f0_ * fastExp2(4.f * k_[0] * (1.f - e_[2])), k_[1]);
            float amp;
            if (t_ < n1_) { amp = (float)t_ / (float)n1_; }
            else { amp = e_[0]; e_[0] *= ec_[0]; }
            out = s_[0].bandpass(src, c_[0]) * amp * k_[2];
            break;
        }
        case Model::PlasmaZap: {
            if ((count_++ & 7) == 0) { k_[3] = std::min(f0_ * fastExp2(k_[0] * e_[1]), nyq_); retune(0, k_[3] * k_[2], 0.9f); }
            const float f = k_[3];
            ph_[0] += f * invFs_; if (ph_[0] >= 1.f) ph_[0] -= 1.f;
            const float ph = ph_[0], d = k_[1];
            const float pw = ph < d ? ph * 0.5f / d : 0.5f + (ph - d) * 0.5f / (1.f - d);
            out = s_[0].lowpassTick(sin2piU(pw), c_[0]) * e_[0];
            break;
        }
        case Model::NebulaModal: {
            const float exc = t_ < n1_ ? sin2piU(0.5f * (float)t_ / (float)n1_) * k_[0] : 0.f;
            for (int k = 0; k < modes_; ++k) {
                const float y = rb_[k] * exc + ra1_[k] * r1_[k] - ra2_[k] * r2_[k];
                r2_[k] = r1_[k]; r1_[k] = y;
                out += y * ramp_[k];
            }
            break;
        }
        case Model::Count: break;
    }
    // Advance the envelopes (Solar Wind holds its master envelope through the attack).
    const int first = p.model == Model::SolarWind ? 1 : 0;
    for (int i = first; i < kEnv; ++i) e_[i] *= ec_[i];
    if (curved_) { cEnv_ *= cCoef_; cCoef_ *= cCoef2_; e_[0] = (float)cEnv_; }
    return out;
}

bool DrumVoice::finished() const {
    // Done when every envelope has fallen 80 dB or more (the clap waits for its last burst, the noise for its
    // attack, resonators for their ring). Kicks saturate after the envelope, so they wait 20 dB longer.
    const float thr = (p_.model == Model::CoreKick || p_.model == Model::GravityKick) ? 1e-5f : 1e-4f;
    float m = 0.f;
    for (int i = 0; i < kEnv; ++i) if (levelEnvs_ & (1u << i)) m = std::max(m, e_[i]);
    bool done = m < thr;
    if (p_.model == Model::BurstClap) done = done && count_ >= n1_;
    if (p_.model == Model::SolarWind) done = t_ > n1_ && e_[0] < thr;
    if (p_.model == Model::NebulaModal || p_.model == Model::SparkRim) {
        done = t_ > n1_;
        for (int k = 0; k < modes_ && done; ++k) if (std::abs(r1_[k]) > 3e-5f || std::abs(r2_[k]) > 3e-5f) done = false;
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
