#include "harmony.h"
#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace wave {

namespace {
constexpr int kScaleTable[][13] = {
    // count, then pitch classes
    {12, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    {7, 0, 2, 4, 5, 7, 9, 11}, {7, 0, 2, 3, 5, 7, 8, 10}, {7, 0, 2, 3, 5, 7, 8, 11}, {7, 0, 2, 3, 5, 7, 9, 11},
    {7, 0, 2, 3, 5, 7, 9, 10}, {7, 0, 1, 3, 5, 7, 8, 10}, {7, 0, 2, 4, 6, 7, 9, 11}, {7, 0, 2, 4, 5, 7, 9, 10},
    {7, 0, 1, 3, 5, 6, 8, 10},
    {5, 0, 2, 4, 7, 9}, {5, 0, 3, 5, 7, 10}, {6, 0, 3, 5, 6, 7, 10},
};
static_assert(sizeof(kScaleTable) / sizeof(kScaleTable[0]) == (size_t)Scale::Count);

int floorDiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

int nearestInScale(float pitch, int key, Scale s) {
    const int c = (int)std::lround(pitch);
    int best = c;
    float bd = 1e9f;
    for (int d = -7; d <= 7; ++d) {
        const int m = c + d;
        if (!inScale(m, key, s)) continue;
        const float dist = std::abs(pitch - (float)m);
        if (dist < bd) { bd = dist; best = m; }
    }
    return best;
}

int indexIn(const ScaleTones& sc, int pc) {
    for (int i = 0; i < sc.n; ++i) if (sc.pc[i] == pc) return i;
    return -1;
}

// Root-position chord tones (absolute MIDI notes, ascending) on root r.
int chordTones(int r, const ChordSpec& spec, const ScaleTones& sc, bool diatonic, int* out) {
    int n = 0;
    auto add = [&](int t) { if (n < Chord::kMax) out[n++] = t; };
    switch (spec.type) {
        case ChordType::Power: add(r); add(r + 7); add(r + 12); return n;
        case ChordType::Octaves: add(r); add(r + 12); return n;
        default: break;
    }
    if (diatonic) {
        static constexpr int steps[][6] = {
            {3, 0, 2, 4}, {4, 0, 2, 4, 6}, {3, 0, 1, 4}, {3, 0, 3, 4}, {4, 0, 2, 4, 8}, {5, 0, 2, 4, 6, 8}};
        const int* s = steps[(int)spec.type];
        for (int i = 1; i <= s[0]; ++i) add(scaleStep(r, s[i], spec.key, sc));
        return n;
    }
    // Scales without seven tones (and chromatic): the quality whose tones fit the scale best.
    auto fits = [&](int i) { return inScale(r + i, spec.key, spec.scale); };
    if (spec.type == ChordType::Sus2) { add(r); add(r + 2); add(r + 7); return n; }
    if (spec.type == ChordType::Sus4) { add(r); add(r + 5); add(r + 7); return n; }
    const bool maj = fits(4) && fits(7), min = !maj && fits(3) && fits(7);
    add(r);
    if (maj) add(r + 4); else if (min) add(r + 3);
    if (fits(7)) add(r + 7); else if (!maj && !min) { add(r + 12); return n; }
    const bool third = maj || min;
    if (third && (spec.type == ChordType::Seventh || spec.type == ChordType::Ninth)) {
        if (fits(10)) add(r + 10); else if (fits(11)) add(r + 11);
    }
    if (third && (spec.type == ChordType::Add9 || spec.type == ChordType::Ninth) && fits(2)) add(r + 14);
    return n;
}

bool containsPc(const int* t, int n, int pc) {
    for (int i = 0; i < n; ++i) if (mod12(t[i]) == pc) return true;
    return false;
}

// Symmetric nearest-tone distance between two voicings, plus a pull towards a register centre.
float voiceCost(const int* a, int na, const int* b, int nb, float centre) {
    float c = 0.f, mean = 0.f;
    for (int i = 0; i < na; ++i) {
        int d = 1000;
        for (int j = 0; j < nb; ++j) d = std::min(d, std::abs(a[i] - b[j]));
        c += (float)d; mean += (float)a[i];
    }
    for (int j = 0; j < nb; ++j) {
        int d = 1000;
        for (int i = 0; i < na; ++i) d = std::min(d, std::abs(a[i] - b[j]));
        c += (float)d;
    }
    return c + 0.35f * na * std::abs(mean / (float)std::max(1, na) - centre);
}
}  // namespace

ScaleTones scaleTones(Scale s) {
    ScaleTones t;
    const auto& row = kScaleTable[std::min((int)s, (int)Scale::Count - 1)];
    t.n = row[0];
    for (int i = 0; i < t.n; ++i) t.pc[i] = row[i + 1];
    return t;
}

bool inScale(int note, int key, Scale s) {
    const auto& row = kScaleTable[std::min((int)s, (int)Scale::Count - 1)];
    const int pc = mod12(note - key);
    for (int i = 1; i <= row[0]; ++i) if (row[i] == pc) return true;
    return false;
}

int quantize(float pitch, int key, Scale s, int current, float hysteresis) {
    if (current >= 0 && inScale(current, key, s)) {
        int lo = current - 1, hi = current + 1;
        while (!inScale(lo, key, s)) --lo;
        while (!inScale(hi, key, s)) ++hi;
        const float up = 0.5f * (float)(current + hi) + std::min(0.45f, hysteresis * (float)(hi - current));
        const float down = 0.5f * (float)(current + lo) - std::min(0.45f, hysteresis * (float)(current - lo));
        if (pitch < up && pitch > down) return current;
    }
    return std::clamp(nearestInScale(pitch, key, s), 0, 127);
}

int scaleStep(int note, int steps, int key, const ScaleTones& sc) {
    const int rel = note - key;
    int oct = floorDiv(rel, 12);
    const int i = indexIn(sc, mod12(rel));
    if (i < 0 || sc.n == 0) return note + steps;           // not a scale tone: treat steps as semitones
    int idx = i + steps;
    oct += floorDiv(idx, sc.n);
    idx -= floorDiv(idx, sc.n) * sc.n;
    return key + oct * 12 + sc.pc[idx];
}

Chord buildChord(int sung, const ChordSpec& spec, const Chord* prev) {
    const ScaleTones sc = scaleTones(spec.scale);
    const bool diatonic = sc.n == 7 && inScale(sung, spec.key, spec.scale);
    const int sungPc = mod12(sung);
    int t[Chord::kMax];

    // Root: the sung note, or (melody on top) the most central chord of the scale that contains it.
    int root = sung;
    if (spec.role == Role::Top && diatonic && spec.type != ChordType::Octaves) {
        static constexpr int rank[7] = {0, 4, 5, 1, 2, 3, 6};   // I, IV, V, vi, ii, iii, vii (by scale degree)
        int bestRank = 99;
        for (int k = 0; k < 7; ++k) {
            const int r = scaleStep(sung, -k, spec.key, sc);
            const int n = chordTones(r, spec, sc, true, t);
            if (!containsPc(t, n, sungPc)) continue;
            const int deg = indexIn(sc, mod12(r - spec.key));
            if (deg >= 0 && rank[deg] < bestRank) { bestRank = rank[deg]; root = r; }
        }
    }
    int n = chordTones(root, spec, sc, diatonic, t);

    if (spec.role == Role::Top) {
        // Every tone in the octave below the sung note, the sung note on top.
        for (int i = 0; i < n; ++i) t[i] = mod12(t[i]) == sungPc ? sung : sung - mod12(sung - t[i]);
        if (!containsPc(t, n, sungPc) && n < Chord::kMax) t[n++] = sung;
        std::sort(t, t + n);
        n = (int)(std::unique(t, t + n) - t);
    } else {
        std::sort(t, t + n);
        if (spec.voicing == Voicing::Open && n >= 3) {
            t[n - 2] -= 12;                                  // drop-2
            std::sort(t, t + n);
        } else if (spec.voicing == Voicing::Smooth && prev && prev->n > 0) {
            // The inversion (and octave) nearest the previous chord's upper voices, kept near the sung register.
            const int pb = prev->bassN;                      // the previous chord's upper voices only
            int best[Chord::kMax]; float bestCost = 1e9f;
            for (int inv = 0; inv < n; ++inv)
                for (int shift = -12; shift <= 12; shift += 12) {
                    int c[Chord::kMax];
                    for (int i = 0; i < n; ++i) c[i] = t[i] + (i < inv ? 12 : 0) + shift;
                    std::sort(c, c + n);
                    const float cost = voiceCost(c, n, prev->notes + pb, prev->n - pb, (float)sung + 4.f);
                    if (cost < bestCost) { bestCost = cost; std::copy(c, c + n, best); }
                }
            std::copy(best, best + n, t);
        }
    }

    Chord ch;
    ch.root = root;
    for (int i = 0; i < n; ++i) ch.mask |= 1 << mod12(t[i] - root);
    if (spec.bass > 0 && n > 0) {
        const int r0 = t[0] - mod12(t[0] - root);            // the root at or below the lowest tone
        const int b = r0 == t[0] ? r0 - 12 * spec.bass : r0 - 12 * (spec.bass - 1);
        if (b >= 0) { ch.notes[ch.n++] = b; ch.bassN = 1; }
    }
    for (int i = 0; i < n && ch.n < Chord::kMax; ++i)
        if (t[i] >= 0 && t[i] <= 127) ch.notes[ch.n++] = t[i];
    std::sort(ch.notes, ch.notes + ch.n);
    ch.n = (int)(std::unique(ch.notes, ch.notes + ch.n) - ch.notes);
    return ch;
}

const char* chordSuffix(int mask) {
    struct Name { int mask; const char* s; };
    auto m = [](std::initializer_list<int> pcs) { int x = 0; for (int p : pcs) x |= 1 << p; return x; };
    static const Name names[] = {
        {m({0, 4, 7}), ""}, {m({0, 3, 7}), "m"}, {m({0, 3, 6}), "dim"}, {m({0, 4, 8}), "aug"},
        {m({0, 4, 7, 10}), "7"}, {m({0, 4, 7, 11}), "maj7"}, {m({0, 3, 7, 10}), "m7"}, {m({0, 3, 6, 10}), "m7b5"},
        {m({0, 3, 6, 9}), "dim7"}, {m({0, 3, 7, 11}), "m(maj7)"}, {m({0, 4, 8, 11}), "maj7#5"},
        {m({0, 2, 7}), "sus2"}, {m({0, 5, 7}), "sus4"}, {m({0, 2, 6}), "sus2b5"}, {m({0, 5, 6}), "sus4b5"},
        {m({0, 1, 7}), "sus b2"}, {m({0, 6, 7}), "sus#4"},
        {m({0, 2, 4, 7}), "add9"}, {m({0, 2, 3, 7}), "madd9"}, {m({0, 1, 3, 7}), "m(addb9)"}, {m({0, 2, 3, 6}), "dim(add9)"},
        {m({0, 1, 3, 6}), "dim(addb9)"}, {m({0, 2, 4, 8}), "aug(add9)"},
        {m({0, 2, 4, 7, 10}), "9"}, {m({0, 2, 4, 7, 11}), "maj9"}, {m({0, 2, 3, 7, 10}), "m9"},
        {m({0, 1, 3, 7, 10}), "m7b9"}, {m({0, 1, 3, 6, 10}), "m7b5b9"}, {m({0, 2, 3, 6, 10}), "m9b5"},
        {m({0, 2, 4, 6, 11}), "maj9#11"}, {m({0, 2, 3, 7, 11}), "m(maj9)"}, {m({0, 1, 4, 7, 10}), "7b9"},
        {m({0, 2, 4, 8, 11}), "maj9#5"},
        {m({0, 7}), "5"}, {m({0}), ""},
    };
    for (const auto& nm : names) if (nm.mask == mask) return nm.s;
    return "*";
}

KeyGuess findKey(const float h[12]) {
    static constexpr float major[12] = {6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f};
    static constexpr float minor[12] = {6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f};
    auto corr = [&](const float* prof, int key) {
        float mh = 0.f, mp = 0.f;
        for (int i = 0; i < 12; ++i) { mh += h[i]; mp += prof[i]; }
        mh /= 12.f; mp /= 12.f;
        float num = 0.f, dh = 0.f, dp = 0.f;
        for (int i = 0; i < 12; ++i) {
            const float a = h[(i + key) % 12] - mh, b = prof[i] - mp;
            num += a * b; dh += a * a; dp += b * b;
        }
        return dh > 0.f ? num / std::sqrt(dh * dp) : 0.f;
    };
    KeyGuess g; g.score = -2.f;
    for (int k = 0; k < 12; ++k) {
        const float a = corr(major, k), b = corr(minor, k);
        if (a > g.score) g = {k, false, a};
        if (b > g.score) g = {k, true, b};
    }
    return g;
}

}  // namespace wave
