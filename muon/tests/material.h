#pragma once
// Test material for MUON's plug-in tests: short synthetic stereo loops, generated deterministically, one per kind of
// source the factory presets are written for, and a K-weighted loudness measure (BS.1770 pre-filter, ungated).
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace material {

struct Stereo { std::vector<float> L, R; };

namespace detail {
constexpr double kPi = 3.14159265358979323846;
struct Rng { uint32_t s; float next() { s = s * 1664525u + 1013904223u; return (float)((s >> 8) & 0xffffff) / 8388608.f - 1.f; } };
inline double env(double t, double a, double d) { return t < 0 ? 0 : t < a ? t / a : std::exp(-(t - a) / d); }

inline void kick(Stereo& o, double fs, double at, float g) {
    const int start = (int)(at * fs);
    double ph = 0;
    for (int i = 0; i < (int)(0.6 * fs) && start + i < (int)o.L.size(); ++i) {
        const double t = i / fs, f = 48.0 + 90.0 * std::exp(-t / 0.035);
        ph += 2 * kPi * f / fs;
        const float v = (float)(g * (std::sin(ph) * env(t, 0.001, 0.22) + 0.25 * std::exp(-t / 0.002)));
        o.L[(size_t)(start + i)] += v; o.R[(size_t)(start + i)] += v;
    }
}
inline void snare(Stereo& o, double fs, double at, float g, Rng& r) {
    const int start = (int)(at * fs);
    float lp = 0;
    for (int i = 0; i < (int)(0.4 * fs) && start + i < (int)o.L.size(); ++i) {
        const double t = i / fs;
        const float n = r.next(); lp += 0.35f * (n - lp);
        const float v = (float)(g * (0.5 * std::sin(2 * kPi * 190.0 * t) * env(t, 0.001, 0.07) + 0.6 * (n - lp) * env(t, 0.0005, 0.12)));
        o.L[(size_t)(start + i)] += v * 0.9f; o.R[(size_t)(start + i)] += v;
    }
}
inline void hat(Stereo& o, double fs, double at, float g, double decay, Rng& r) {
    const int start = (int)(at * fs);
    float x1 = 0, y1 = 0;
    for (int i = 0; i < (int)(decay * 6 * fs) && start + i < (int)o.L.size(); ++i) {
        const double t = i / fs;
        const float n = r.next(); const float hp = 0.8f * (y1 + n - x1); x1 = n; y1 = hp;
        const float v = (float)(g * hp * env(t, 0.0003, decay));
        o.L[(size_t)(start + i)] += v; o.R[(size_t)(start + i)] += v * 0.8f;
    }
}
inline void tone(Stereo& o, double fs, double at, double f, double dur, float g, double attack, double decay, int partials, float pan) {
    const int start = (int)(at * fs);
    for (int i = 0; i < (int)(dur * fs) && start + i < (int)o.L.size(); ++i) {
        const double t = i / fs;
        double v = 0;
        for (int k = 1; k <= partials; ++k) v += std::sin(2 * kPi * f * k * t) / k;
        const float s = (float)(g * v * env(t, attack, decay) * std::min(1.0, (dur - t) / 0.01));
        o.L[(size_t)(start + i)] += s * (1.f - pan); o.R[(size_t)(start + i)] += s * (1.f + pan);
    }
}
// Karplus-Strong pluck.
inline void pluck(Stereo& o, double fs, double at, double f, float g, Rng& r, float pan) {
    const int start = (int)(at * fs), N = std::max(2, (int)(fs / f));
    std::vector<float> d((size_t)N);
    for (auto& v : d) v = r.next();
    int p = 0;
    for (int i = 0; i < (int)(1.8 * fs) && start + i < (int)o.L.size(); ++i) {
        const int q = (p + 1) % N;
        const float y = 0.498f * (d[(size_t)p] + d[(size_t)q]);
        d[(size_t)p] = y; p = q;
        o.L[(size_t)(start + i)] += g * y * (1.f - pan); o.R[(size_t)(start + i)] += g * y * (1.f + pan);
    }
}
}  // namespace detail

// kind: kick, snare, toms, perc, hats, drums, bass, guitar, vocal, keys, mix. 4 seconds.
inline Stereo make(const std::string& kind, double fs = 48000.0, double seconds = 4.0) {
    using namespace detail;
    Stereo o; o.L.assign((size_t)(seconds * fs), 0.f); o.R.assign(o.L.size(), 0.f);
    Rng r{12345u};
    const double beat = 0.5;   // 120 bpm
    auto drums = [&](float g) {
        for (int b = 0; b < (int)(seconds / beat); ++b) {
            if (b % 2 == 0 || b % 8 == 7) kick(o, fs, b * beat, 0.8f * g);
            if (b % 2 == 1) snare(o, fs, b * beat, 0.6f * g, r);
            hat(o, fs, b * beat, 0.25f * g, 0.03, r); hat(o, fs, b * beat + beat / 2, 0.18f * g, 0.03, r);
        }
    };
    if (kind == "kick") for (int b = 0; b < (int)(seconds / beat); ++b) kick(o, fs, b * beat, 0.8f);
    else if (kind == "snare") for (int b = 0; b < (int)(seconds / beat); ++b) snare(o, fs, b * beat + 0.05, 0.7f, r);
    else if (kind == "toms") { const double f[4] = {150, 120, 95, 75}; for (int b = 0; b < (int)(seconds / 0.25); ++b) tone(o, fs, b * 0.25, f[b % 4], 0.6, 0.6f, 0.001, 0.18, 1, (b % 4 - 1.5f) * 0.2f); }
    else if (kind == "perc") for (int b = 0; b < (int)(seconds / 0.125); ++b) { if (b % 3 == 2) continue; tone(o, fs, b * 0.125, 400 + 300 * (b % 4), 0.15, 0.35f, 0.0005, 0.03, 3, (b % 2) ? 0.4f : -0.4f); hat(o, fs, b * 0.125, 0.1f, 0.01, r); }
    else if (kind == "hats") for (int b = 0; b < (int)(seconds / 0.125); ++b) hat(o, fs, b * 0.125, b % 4 == 2 ? 0.45f : 0.3f, b % 8 == 6 ? 0.15 : 0.035, r);
    else if (kind == "drums") drums(1.f);
    else if (kind == "bass") { const double n[8] = {55, 55, 65.4, 73.4, 55, 82.4, 73.4, 61.7}; for (int b = 0; b < (int)(seconds / 0.25); ++b) tone(o, fs, b * 0.25, n[b % 8], 0.24, 0.45f, 0.004, 0.25, 8, 0.f); }
    else if (kind == "guitar") { const double ch[3] = {110, 164.8, 220}; for (int b = 0; b < (int)(seconds / 0.5); ++b) for (int k = 0; k < 3; ++k) pluck(o, fs, b * 0.5 + k * 0.012, ch[k] * (b % 4 == 3 ? 1.122 : 1.0), 0.4f, r, (k - 1) * 0.3f); }
    else if (kind == "vocal") {
        // A pulse train through three formants, with vibrato, in phrases with gaps.
        double ph = 0; float s1 = 0, s2 = 0, s3 = 0, b1 = 0, b2 = 0, b3 = 0;
        for (size_t i = 0; i < o.L.size(); ++i) {
            const double t = i / fs, phrase = std::fmod(t, 1.0);
            const double f0 = 196.0 * (1.0 + 0.012 * std::sin(2 * kPi * 5.5 * t)) * (std::fmod(t, 2.0) < 1.0 ? 1.0 : 1.12);
            ph += f0 / fs; float pulse = 0.f; if (ph >= 1.0) { ph -= 1.0; pulse = 1.f; }
            auto bp = [&](float x, float f, float& s, float& b) { const float g = (float)std::tan(kPi * f / fs), k = 0.15f; const float hp = (x - (k + g) * s - b) / (1 + g * (k + g)); const float bpv = g * hp + s; s = g * hp + bpv; const float lpv = g * bpv + b; b = g * bpv + lpv; return bpv; };
            const float v = bp(pulse, 700, s1, b1) + 0.6f * bp(pulse, 1200, s2, b2) + 0.3f * bp(pulse, 2600, s3, b3) + 0.02f * r.next();
            const float e = (float)(phrase < 0.75 ? std::min(1.0, phrase / 0.03) * std::min(1.0, (0.75 - phrase) / 0.05) : 0.0);
            o.L[i] = o.R[i] = 0.9f * v * e;
        }
    }
    else if (kind == "keys") { const double n[4] = {261.6, 329.6, 392.0, 493.9}; for (int b = 0; b < (int)(seconds / 0.5); ++b) for (int k = 0; k < 4; ++k) tone(o, fs, b * 0.5, n[k] * (b % 2 ? 0.891 : 1.0), 0.48, 0.12f, 0.002, 0.5, 6, (k - 1.5f) * 0.2f); }
    else {  // mix
        drums(0.6f);
        const double n[8] = {55, 55, 65.4, 73.4, 55, 82.4, 73.4, 61.7};
        for (int b = 0; b < (int)(seconds / 0.25); ++b) tone(o, fs, b * 0.25, n[b % 8], 0.24, 0.3f, 0.004, 0.25, 8, 0.f);
        const double c[3] = {261.6, 329.6, 392.0};
        for (int b = 0; b < (int)(seconds / 1.0); ++b) for (int k = 0; k < 3; ++k) tone(o, fs, b * 1.0, c[k], 0.95, 0.06f, 0.02, 1.0, 5, (k - 1) * 0.4f);
    }
    return o;
}

// K-weighted loudness (BS.1770 pre-filter, ungated, both channels) of [from, end), in dB.
inline double loudness(const std::vector<float>& L, const std::vector<float>& R, double fs, size_t from) {
    struct Bq { double b0, b1, b2, a1, a2, z1 = 0, z2 = 0; double tick(double v) { const double y = b0 * v + z1; z1 = b1 * v - a1 * y + z2; z2 = b2 * v - a2 * y; return y; } };
    auto make = [&]() {
        const double G = 3.999843853973347, Q1 = 0.7071752369554196, f1 = 1681.974450955533, Q2 = 0.5003270373238773, f2 = 38.13547087602444;
        const double A = std::pow(10.0, G / 40), w = 2 * detail::kPi * f1 / fs, cw = std::cos(w), al = std::sin(w) / (2 * Q1), sa = std::sqrt(A);
        const double a0 = (A + 1) - (A - 1) * cw + 2 * sa * al;
        Bq s{A * ((A + 1) + (A - 1) * cw + 2 * sa * al) / a0, -2 * A * ((A - 1) + (A + 1) * cw) / a0, A * ((A + 1) + (A - 1) * cw - 2 * sa * al) / a0,
             2 * ((A - 1) - (A + 1) * cw) / a0, ((A + 1) - (A - 1) * cw - 2 * sa * al) / a0};
        const double w2 = 2 * detail::kPi * f2 / fs, c2 = std::cos(w2), al2 = std::sin(w2) / (2 * Q2), b0 = 1 + al2;
        Bq h{(1 + c2) / 2 / b0, -(1 + c2) / b0, (1 + c2) / 2 / b0, -2 * c2 / b0, (1 - al2) / b0};
        return std::make_pair(s, h);
    };
    auto [sL, hL] = make(); auto [sR, hR] = make();
    double e = 0; size_t n = 0;
    for (size_t i = 0; i < L.size(); ++i) {
        const double a = hL.tick(sL.tick(L[i])), b = hR.tick(sR.tick(R[i]));
        if (i >= from) { e += a * a + b * b; ++n; }
    }
    return 10.0 * std::log10(std::max(e / std::max<size_t>(n, 1), 1e-20));
}

}  // namespace material
