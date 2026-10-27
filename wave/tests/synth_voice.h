#pragma once
// Synthetic singing voice for the WAVE tests: a harmonic source shaped by vowel formants, with vibrato, pitch
// jitter, breath noise, attack and release ramps, glides, and an optional weak fundamental. Deterministic.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace wavetest {

struct Note {
    double start = 0.0, dur = 0.5;   // seconds
    float midi = 57.f;               // pitch at the start
    float midiEnd = -1.f;            // glide target (-1: no glide)
    double glide = 0.08;             // glide time
    double glideAt = -1.0;           // glide start within the note (-1: at the end)
    float db = -12.f;                // level (peak of the waveform, roughly)
    float vibCents = 0.f, vibHz = 5.5f;
    int vowel = 0;                   // 0 a, 1 e, 2 i, 3 o, 4 u
    float fundamental = 1.f;         // relative level of the first harmonic (weak-fundamental voices)
    float breathDb = -40.f;          // breath noise relative to the level
    double attack = 0.012, release = 0.025;
};

inline float formantGain(double f, int vowel) {
    static const double F[5][3] = {{730, 1090, 2440}, {530, 1840, 2480}, {270, 2290, 3010}, {570, 840, 2410}, {300, 870, 2240}};
    static const double B[3] = {90, 110, 150};
    double g = 0.06;
    for (int i = 0; i < 3; ++i) { const double x = (f - F[vowel][i]) / B[i]; g += (i == 0 ? 1.0 : i == 1 ? 0.6 : 0.3) / (1.0 + x * x); }
    return (float)g;
}

// Renders the notes into a buffer of `seconds` at fs (mono).
inline std::vector<float> render(const std::vector<Note>& notes, double seconds, double fs, uint32_t seed = 1) {
    std::vector<float> out((size_t)(seconds * fs), 0.f);
    uint32_t rng = seed;
    auto rnd = [&] { rng = rng * 1664525u + 1013904223u; return (float)((rng >> 8) * (1.0 / 16777216.0)) * 2.f - 1.f; };
    for (const auto& n : notes) {
        const int64_t a = (int64_t)(n.start * fs), len = (int64_t)(n.dur * fs);
        double phase[64] = {};
        double jitter = 0.0, nlp = 0.0;
        const float amp = std::pow(10.f, n.db / 20.f);
        for (int64_t i = 0; i < len && a + i < (int64_t)out.size(); ++i) {
            const double t = i / fs;
            double m = n.midi;
            const double g0 = n.glideAt >= 0.0 ? n.glideAt : n.dur - n.glide;
            if (n.midiEnd >= 0.f && t > g0) {
                const double x = std::min(1.0, (t - g0) / std::max(1e-6, n.glide));
                m = n.midi + (n.midiEnd - n.midi) * (0.5 - 0.5 * std::cos(3.14159265 * x));
            }
            m += n.vibCents / 100.0 * std::sin(2.0 * 3.14159265 * n.vibHz * t);
            jitter += (rnd() * 0.02 - jitter * 0.001);       // slow random walk, a few cents
            const double f0 = 440.0 * std::pow(2.0, (m - 69.0 + jitter * 0.01) / 12.0);
            double s = 0.0, norm = 0.0;
            for (int k = 1; k <= 60; ++k) {
                const double fk = f0 * k;
                if (fk > std::min(5000.0, fs * 0.45)) break;
                phase[k] += 2.0 * 3.14159265 * fk / fs;
                if (phase[k] > 6.283185307) phase[k] -= 6.283185307;
                const double g = formantGain(fk, n.vowel) / k * (k == 1 ? n.fundamental : 1.0);
                s += g * std::sin(phase[k]);
                norm += g;
            }
            nlp += 0.3 * (rnd() - nlp);
            double env = std::min(1.0, t / n.attack) * std::min(1.0, (n.dur - t) / n.release);
            env = std::max(0.0, env);
            const double breath = nlp * std::pow(10.0, n.breathDb / 20.0);
            out[(size_t)(a + i)] += (float)(amp * env * (s / std::max(1e-9, norm) * 2.0 + breath));
        }
    }
    return out;
}

}  // namespace wavetest
