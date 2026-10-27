#include "resonance.h"

namespace muon {

namespace {
inline float noteHz(float note) { return 440.f * std::pow(2.f, (note - 69.f) / 12.f); }

struct IntervalSet { int n; float st[13]; };
IntervalSet chordIntervals(Chord c) {
    switch (c) {
        case Chord::Unison: return {3, {0, 0, 0}};
        case Chord::Octaves: return {4, {0, 12, 24, 36}};
        case Chord::Fifths: return {5, {0, 7, 12, 19, 24}};
        case Chord::Major: return {6, {0, 4, 7, 12, 16, 19}};
        case Chord::Minor: return {6, {0, 3, 7, 12, 15, 19}};
        case Chord::Sus: return {6, {0, 5, 7, 12, 17, 19}};
        case Chord::Harmonic: return {8, {0, 12, 19.02f, 24, 27.86f, 31.02f, 33.69f, 36}};   // partials 1..8
    }
    return {1, {0}};
}
IntervalSet scaleIntervals(Scale s) {
    switch (s) {
        case Scale::Major: return {8, {0, 2, 4, 5, 7, 9, 11, 12}};
        case Scale::Minor: return {8, {0, 2, 3, 5, 7, 8, 10, 12}};
        case Scale::Dorian: return {8, {0, 2, 3, 5, 7, 9, 10, 12}};
        case Scale::Pentatonic: return {6, {0, 3, 5, 7, 10, 12}};
        case Scale::Chromatic: return {13, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}};
    }
    return {1, {0}};
}
// Mode frequency ratios of the materials (PRD 4.5).
constexpr float kRatios[5][16] = {
    // Wood: free-free bar (Euler-Bernoulli).
    {1.f, 2.756f, 5.404f, 8.933f, 13.34f, 18.64f, 24.81f, 31.87f, 39.81f, 48.63f, 58.33f, 68.92f, 80.39f, 92.74f, 106.0f, 120.1f},
    // Skin: ideal circular membrane (Bessel zeros relative to the (0,1) mode).
    {1.f, 1.594f, 2.136f, 2.296f, 2.653f, 2.918f, 3.156f, 3.501f, 3.600f, 3.652f, 4.060f, 4.154f, 4.601f, 4.832f, 4.903f, 5.131f},
    // Metal: a free plate, inharmonic and dense.
    {1.f, 1.470f, 1.810f, 2.560f, 2.800f, 3.250f, 3.660f, 4.230f, 4.580f, 5.110f, 5.630f, 6.070f, 6.450f, 7.020f, 7.640f, 8.120f},
    // Glass: bell-like pairs that beat slowly.
    {1.f, 1.004f, 2.320f, 2.329f, 4.250f, 4.262f, 6.630f, 6.651f, 9.380f, 9.410f, 12.50f, 12.55f, 15.98f, 16.03f, 19.80f, 19.86f},
    // Tube: closed cylinder, odd harmonics.
    {1.f, 3.f, 5.f, 7.f, 9.f, 11.f, 13.f, 15.f, 17.f, 19.f, 21.f, 23.f, 25.f, 27.f, 29.f, 31.f},
};
inline float soft(float y) { return y / std::sqrt(1.f + 0.1f * y * y); }
}  // namespace

// ---- Pitch tracker -------------------------------------------------------------------------------------------
void PitchTracker::prepare(double fs) {
    fs_ = fs;
    dec_ = std::max(1, (int)std::lround(fs / 6000.0));
    fsd_ = fs / dec_;
    tauMax_ = (int)(fsd_ / 30.0); tauMin_ = std::max(2, (int)(fsd_ / 1000.0));
    win_ = tauMax_;
    hop_ = 128;
    aa1_.set(fs, 0.4 * fsd_, 1.41421356); aa2_.set(fs, 0.4 * fsd_, 1.41421356);
    ring_.assign((size_t)(win_ + tauMax_) * 2, 0.f);
    d_.assign((size_t)tauMax_ + 2, 0.f);
    reset();
}
void PitchTracker::reset() {
    aa1_.reset(); aa2_.reset();
    std::fill(ring_.begin(), ring_.end(), 0.f);
    pos_ = 0; phase_ = 0; sinceHop_ = 0; hz_ = 0.f; voiced_ = false;
}
void PitchTracker::push(const float* x, int n) {
    const int len = win_ + tauMax_;
    for (int i = 0; i < n; ++i) {
        const float y = aa2_.lp(aa1_.lp(x[i]));
        if (++phase_ < dec_) continue;
        phase_ = 0;
        ring_[(size_t)pos_] = ring_[(size_t)(pos_ + len)] = y;
        pos_ = (pos_ + 1) % len;
        if (++sinceHop_ >= hop_) { sinceHop_ = 0; analyse(); }
    }
}
void PitchTracker::analyse() {
    const int len = win_ + tauMax_;
    const float* w = &ring_[(size_t)pos_];     // oldest .. newest, contiguous through the mirror
    double energy = 0.0;
    for (int j = 0; j < len; ++j) energy += (double)w[j] * w[j];
    if (energy / len < 1e-7) { voiced_ = false; return; }
    d_[0] = 1.f;
    double run = 0.0;
    int best = -1;
    for (int tau = 1; tau <= tauMax_; ++tau) {
        double s = 0.0;
        for (int j = 0; j < win_; ++j) { const double e = (double)w[j] - w[j + tau]; s += e * e; }
        run += s;
        d_[(size_t)tau] = run > 0.0 ? (float)(s * tau / run) : 1.f;
    }
    for (int tau = tauMin_; tau < tauMax_; ++tau) {
        if (d_[(size_t)tau] < 0.15f) {
            while (tau + 1 < tauMax_ && d_[(size_t)tau + 1] < d_[(size_t)tau]) ++tau;
            best = tau; break;
        }
    }
    if (best < 0) {
        float m = 0.35f;
        for (int tau = tauMin_; tau < tauMax_; ++tau) if (d_[(size_t)tau] < m) { m = d_[(size_t)tau]; best = tau; }
    }
    if (best < 0) { voiced_ = false; return; }
    double t = best;
    if (best > 1 && best + 1 <= tauMax_) {
        const double a = d_[(size_t)best - 1], b = d_[(size_t)best], c = d_[(size_t)best + 1], den = a - 2 * b + c;
        if (std::fabs(den) > 1e-12) t += 0.5 * (a - c) / den;
    }
    hz_ = (float)(fsd_ / t);
    voiced_ = true;
}

// ---- Resonator bank ------------------------------------------------------------------------------------------
void ResonatorBank::prepare(double fs) {
    fs_ = fs;
    toneLp_.setHz(fs, 1500.0);
    for (auto& c : comb_) c.prepare((int)(fs / 20.0) + 4);
    for (auto& d : combDc_) d.setHz(fs, 10.0);
    reset();
}

void ResonatorBank::reset() {
    for (auto& r : res_) r.y1 = r.y2 = 0.f;
    toneLp_.reset();
    for (auto& c : comb_) c.reset();
    for (auto& d : combDc_) d.reset();
    combLp_[0] = combLp_[1] = 0.f;
    nHeld_ = 0; first_ = true; t_ = 0;
}

void ResonatorBank::noteOn(int note) {
    for (int i = 0; i < nHeld_; ++i) if (held_[i] == note) return;
    if (nHeld_ == 8) { for (int i = 1; i < 8; ++i) held_[i - 1] = held_[i]; --nHeld_; }
    held_[nHeld_++] = note;
    nLast_ = nHeld_;
    for (int i = 0; i < nHeld_; ++i) lastNotes_[i] = held_[i];
}
void ResonatorBank::noteOff(int note) {
    for (int i = 0; i < nHeld_; ++i) if (held_[i] == note) { for (int j = i + 1; j < nHeld_; ++j) held_[j - 1] = held_[j]; --nHeld_; break; }
}

int ResonatorBank::targets(const Params& p, float trackedHz, float* hz, float* amp) const {
    float root = noteHz((float)p.root);
    if (p.pitch == PitchSource::Track && trackedHz > 0.f) root = trackedHz;
    if (p.pitch == PitchSource::Midi && nLast_ > 0) root = noteHz((float)lastNotes_[nLast_ - 1]);
    const float nyq = (float)(0.45 * fs_);
    int n = 0;
    if (p.resMode == ResMode::Body) {
        const float base = root * std::pow(2.f, (p.size - 0.5f) * 4.f);
        const auto& r = kRatios[(int)p.material];
        for (int k = 0; k < 16; ++k) {
            const float f = base * r[k];
            if (f >= nyq) break;
            hz[n] = f; amp[n] = 1.f / (1.f + 0.35f * k); ++n;
        }
        return n;
    }
    if (p.resMode == ResMode::Comb) return 0;
    if (p.pitch == PitchSource::Midi) {
        for (int i = 0; i < nLast_ && n < 8; ++i) { const float f = noteHz((float)lastNotes_[i]); if (f < nyq) { hz[n] = f; amp[n] = 1.f; ++n; } }
        return n;
    }
    const IntervalSet set = p.pitch == PitchSource::Key ? scaleIntervals(p.scale) : chordIntervals(p.chord);
    const int maxN = p.pitch == PitchSource::Key ? kMaxResonators : 8;
    for (int i = 0; i < set.n && n < maxN; ++i) {
        float cents = 0.f;
        if (p.chord == Chord::Unison && p.pitch != PitchSource::Key) cents = (float)(i - 1) * p.spread * 30.f;
        else cents = (float)((i % 3) - 1) * p.spread * 30.f;
        const float f = root * std::pow(2.f, (set.st[i] * 100.f + cents) / 1200.f);
        if (f >= nyq) break;
        hz[n] = f;
        amp[n] = p.chord == Chord::Harmonic && p.pitch != PitchSource::Key ? 1.f / (float)(i + 1) : 1.f;
        ++n;
    }
    return n;
}

void ResonatorBank::updateCoefs(const Params& p) {
    const float decay = std::clamp(p.resDecay, 0.01f, 4.f), damp = std::clamp(p.resDamping, 0.f, 1.f);
    const float f0 = count_ > 0 ? freq_[0] : 100.f;
    const float kExc = (float)(2000.0 / fs_);
    for (int i = 0; i < count_; ++i) {
        Res& r = res_[i];
        const float f = freq_[i];
        const float t60 = std::max(0.01f, decay * std::pow(std::max(1.f, f / std::max(f0, 1.f)), -1.2f * damp));
        const double rr = std::exp(-6.907755 / (t60 * fs_)), w = 2.0 * kPi * f / fs_;
        r.c1 = (float)(2.0 * rr * std::cos(w)); r.c2 = (float)(-rr * rr);
        r.g = (float)std::sin(w) * kExc * amp_[i];
        const float pan = (i % 2 == 0 ? -1.f : 1.f) * p.resStereo * (i == 0 && count_ > 1 ? 0.5f : 1.f);
        r.panL = std::min(1.f, 1.f - pan); r.panR = std::min(1.f, 1.f + pan);
    }
}

void ResonatorBank::process(const float* x, float* outL, float* outR, int n, const Params& p, float trackedHz) {
    // Targets at block start; frequencies glide (20 ms) at control rate.
    float hz[kMaxResonators], amp[kMaxResonators];
    const int cnt = targets(p, trackedHz, hz, amp);
    if (cnt != count_) {                                  // a new set: start from the targets, keep the ringing state
        for (int i = count_; i < cnt; ++i) { res_[i].y1 = res_[i].y2 = 0.f; freq_[i] = hz[i]; }
        count_ = cnt;
    }
    for (int i = 0; i < cnt; ++i) { target_[i] = hz[i]; amp_[i] = amp[i]; if (first_) freq_[i] = hz[i]; }
    first_ = false;
    const float glide = (float)(1.0 - std::exp(-16.0 / (0.02 * fs_)));
    const float tilt = 2.f * p.resTone - 1.f, gL = dbToGain(-9.f * tilt), gH = dbToGain(9.f * tilt);
    const float strike = p.strike * p.strike * 2.f;
    const bool comb = p.resMode == ResMode::Comb;
    float combHz = noteHz((float)p.root);
    if (p.pitch == PitchSource::Track && trackedHz > 0.f) combHz = trackedHz;
    if (p.pitch == PitchSource::Midi && nLast_ > 0) combHz = noteHz((float)lastNotes_[nLast_ - 1]);
    combHz = std::clamp(combHz * std::pow(2.f, (p.size - 0.5f) * 4.f), 20.f, (float)(0.2 * fs_));
    const float combT60 = std::clamp(p.resDecay, 0.01f, 4.f);
    const float lpA = 0.7f * std::clamp(p.resDamping, 0.f, 1.f);

    for (int i = 0; i < n; ++i, ++t_) {
        if ((t_ & 15) == 0) {
            for (int k = 0; k < count_; ++k) freq_[k] *= std::pow(target_[k] / freq_[k], glide);
            combHz_ += (combHz - combHz_) * glide;
            updateCoefs(p);
        }
        const float lo = toneLp_.lp(x[i]);
        const float e = (gL * lo + gH * (x[i] - lo)) * strike;
        float l = 0.f, r = 0.f;
        if (comb) {
            for (int c = 0; c < 2; ++c) {
                const float f = combHz_ * (c == 0 ? 1.f : 1.f + 0.01f * p.spread);
                const float d = (float)(fs_ / f) - 1.f;
                const float fb = std::pow(10.f, -3.f / (combT60 * f));
                const float tap = comb_[c].readFrac(std::max(1.f, d));
                combLp_[c] = tap + lpA * (combLp_[c] - tap);
                const float y = std::clamp(0.5f * e + fb * combLp_[c], -8.f, 8.f);
                comb_[c].write(y);
                const float o = combDc_[c].tick(y);
                (c == 0 ? l : r) = o;
            }
            const float w = p.resStereo;
            const float m = 0.5f * (l + r);
            l = m + w * (l - m); r = m + w * (r - m);
        } else {
            for (int k = 0; k < count_; ++k) {
                Res& s = res_[k];
                const float y = std::clamp(s.c1 * s.y1 + s.c2 * s.y2 + s.g * e, -8.f, 8.f);
                s.y2 = s.y1; s.y1 = y;
                l += y * s.panL; r += y * s.panR;
            }
        }
        outL[i] = soft(l); outR[i] = soft(r);
    }
}

}  // namespace muon
