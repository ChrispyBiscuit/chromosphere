#include "Ignite.h"
#include "PatternStore.h"
#include "ultra/params.h"

using namespace ultra;

namespace ignite {

namespace {
// A sound role: which models suit it and the ranges its dials may take.
struct Range { float lo, hi; };
struct Role {
    const char* name;
    std::vector<Model> models;
    Range tune, decay, tone, snap, color, sweep, level;
    float sendB = 0.05f;
    int choke = 0;
    float pan = 0.f;
};

float pick(juce::Random& r, Range x) { return x.lo + (x.hi - x.lo) * r.nextFloat(); }
float round3(float v) { return std::round(v * 1000.f) / 1000.f; }

struct Genre {
    double tempoLo, tempoHi;
    float swingLo, swingHi;
    std::array<Role, kTracks> roles;
};

Role kickRole(Range tune, Range decay, std::vector<Model> m = {Model::CoreKick, Model::GravityKick}) {
    return {"Kick", std::move(m), tune, decay, {0.35f, 0.65f}, {0.4f, 0.65f}, {0.15f, 0.5f}, {0.3f, 0.55f}, {-1.f, 0.f}, 0.f};
}
Role snareRole(const char* n = "Snare") { return {n, {Model::FlareSnare}, {-3.f, 5.f}, {0.3f, 0.5f}, {0.45f, 0.65f}, {0.45f, 0.65f}, {0.5f, 0.75f}, {0.2f, 0.4f}, {-4.f, -2.f}, 0.15f}; }
Role clapRole() { return {"Clap", {Model::BurstClap}, {-2.f, 2.f}, {0.35f, 0.55f}, {0.4f, 0.6f}, {0.45f, 0.65f}, {0.45f, 0.8f}, {0.3f, 0.6f}, {-5.f, -2.f}, 0.2f}; }
Role hatRole(bool open) {
    return {open ? "Open Hat" : "Closed Hat", {Model::IonHat}, {-2.f, 3.f}, open ? Range{0.45f, 0.65f} : Range{0.1f, 0.22f}, {0.5f, 0.75f},
            {0.35f, 0.6f}, {0.15f, 0.5f}, {0.05f, 0.3f}, open ? Range{-11.f, -8.f} : Range{-10.f, -7.f}, open ? 0.08f : 0.f, 1, 0.15f};
}
Role percRole(const char* n, std::vector<Model> m, Range tune, float pan) {
    return {n, std::move(m), tune, {0.25f, 0.5f}, {0.35f, 0.65f}, {0.4f, 0.7f}, {0.1f, 0.9f}, {0.1f, 0.5f}, {-12.f, -7.f}, 0.15f, 0, pan};
}

Genre genre(int g) {
    const std::vector<Model> metal{Model::QuasarMetal, Model::SparkRim, Model::PulsarFm};
    const std::vector<Model> skins{Model::OrbitTom, Model::NebulaModal, Model::SparkRim};
    switch (g) {
        case 0: return {118, 126, 0.52f, 0.6f, {kickRole({0, 3}, {0.38f, 0.5f}, {Model::CoreKick}), snareRole(), clapRole(), hatRole(false), hatRole(true),
                         percRole("Perc", skins, {-6, 4}, -0.25f), percRole("Rim", {Model::SparkRim}, {-3, 3}, 0.3f), percRole("Ride", {Model::CoronaCymbal, Model::SolarWind}, {-2, 4}, -0.2f)}};
        case 1: return {126, 138, 0.5f, 0.53f, {kickRole({-2, 2}, {0.3f, 0.45f}, {Model::GravityKick, Model::CoreKick}), snareRole(), clapRole(), hatRole(false), hatRole(true),
                         percRole("Perc", {Model::PulsarFm, Model::NebulaModal}, {-5, 12}, 0.3f), percRole("Rim", metal, {-5, 5}, -0.3f), percRole("Ride", {Model::CoronaCymbal}, {-2, 3}, 0.2f)}};
        case 2: return {84, 98, 0.55f, 0.63f, {kickRole({-3, 1}, {0.38f, 0.5f}, {Model::CoreKick}), snareRole(), percRole("Rim", {Model::SparkRim}, {-4, 2}, -0.2f), hatRole(false), hatRole(true),
                         percRole("Perc", skins, {-4, 4}, 0.3f), percRole("Shaker", {Model::SolarWind}, {0, 6}, 0.35f), percRole("Bell", {Model::QuasarMetal}, {-3, 5}, -0.3f)}};
        case 3: return {130, 150, 0.5f, 0.5f, {kickRole({-7, -3}, {0.75f, 0.92f}, {Model::CoreKick}), snareRole(), clapRole(), hatRole(false), hatRole(true),
                         percRole("Zap", {Model::PlasmaZap}, {5, 14}, 0.2f), percRole("Perc", {Model::PulsarFm}, {3, 9}, 0.35f), percRole("Riser", {Model::SolarWind}, {-3, 3}, 0.f)}};
        case 4: return {168, 176, 0.5f, 0.55f, {kickRole({2, 5}, {0.28f, 0.38f}, {Model::CoreKick}), snareRole(), snareRole("Ghost"), hatRole(false), hatRole(true),
                         percRole("Ride", {Model::CoronaCymbal}, {-1, 3}, -0.25f), percRole("Rim", {Model::SparkRim}, {0, 4}, -0.3f), percRole("Shaker", {Model::SolarWind}, {2, 6}, 0.35f)}};
        case 5: return {96, 136, 0.5f, 0.58f, {kickRole({-1, 2}, {0.36f, 0.48f}, {Model::CoreKick}), snareRole(), snareRole("Ghost"), hatRole(false), hatRole(true),
                         percRole("Crash", {Model::CoronaCymbal}, {-2, 2}, -0.2f), percRole("Tom", {Model::OrbitTom}, {-4, 3}, -0.3f), percRole("Rim", {Model::SparkRim}, {-2, 3}, 0.3f)}};
        case 6: return {110, 132, 0.5f, 0.56f, {kickRole({-3, 0}, {0.55f, 0.75f}, {Model::CoreKick}), snareRole(), clapRole(), hatRole(false), hatRole(true),
                         percRole("FM Tom", {Model::PulsarFm, Model::OrbitTom}, {-6, 3}, -0.2f), percRole("Rim", {Model::SparkRim}, {-2, 3}, 0.25f), percRole("Zap", {Model::PlasmaZap, Model::QuasarMetal}, {4, 12}, 0.2f)}};
        default: return {90, 140, 0.5f, 0.6f, {kickRole({-6, 6}, {0.3f, 0.7f}, {Model::GravityKick, Model::CoreKick, Model::NebulaModal}),
                         percRole("Snare", {Model::FlareSnare, Model::NebulaModal, Model::PulsarFm}, {-5, 7}, 0.f), percRole("Clap", {Model::BurstClap, Model::SolarWind}, {-6, 6}, -0.2f),
                         hatRole(false), percRole("Metal", {Model::CoronaCymbal, Model::QuasarMetal}, {-6, 8}, 0.25f),
                         percRole("Modal", {Model::NebulaModal}, {-8, 12}, -0.3f), percRole("FM", {Model::PulsarFm}, {-5, 12}, 0.3f), percRole("Zap", {Model::PlasmaZap, Model::SolarWind}, {-4, 12}, 0.f)}};
    }
}

// ---- Rhythm rules ---------------------------------------------------------------------------------------------
using Bank = std::array<std::array<TrackPattern, kTracks>, 4>;   // main, variation, break, fill

void hit(TrackPattern& tp, int s, int vel, int ratchet = 1, int chance = 100, Cond c = Cond::Always) {
    if (s < 0 || s >= kMaxSteps) return;
    auto& st = tp.steps[(size_t)s];
    st.vel = (uint8_t)juce::jlimit(1, 127, vel); st.ratchet = (uint8_t)ratchet; st.chance = (uint8_t)chance; st.cond = (uint8_t)c;
}

std::array<TrackPattern, kTracks> mainPattern(juce::Random& r, int g) {
    std::array<TrackPattern, kTracks> t{};
    auto every = [&](int track, int start, int step, int vel, int len = 16) { for (int s = start; s < len; s += step) hit(t[(size_t)track], s, vel); };
    switch (g) {
        case 0:   // house
            every(0, 0, 4, 110); every(2, 4, 8, 105);
            every(3, 2, 4, 85); if (r.nextBool()) every(3, 0, 4, 55);
            every(4, 2, 4, 80); for (int s = 0; s < 16; ++s) if (r.nextFloat() < 0.2f) hit(t[5], s, 70);
            for (int s : {3, 6, 11, 14}) if (r.nextFloat() < 0.5f) hit(t[6], s, 75);
            if (r.nextBool()) every(7, 0, 2, 70);
            break;
        case 1:   // techno
            every(0, 0, 4, 115); every(3, 2, 4, 85); if (r.nextBool()) for (int s = 0; s < 16; ++s) hit(t[3], s, s % 2 ? 85 : 55);
            if (r.nextBool()) every(2, 4, 8, 90);
            t[5].length = 10 + r.nextInt(4);
            for (int s = 0; s < t[5].length; ++s) if (r.nextFloat() < 0.35f) hit(t[5], s, 70 + r.nextInt(40), 1, r.nextBool() ? 100 : 70);
            for (int s : {6, 14}) hit(t[6], s, 80);
            if (r.nextBool()) every(7, 2, 4, 70);
            break;
        case 2: {  // hip-hop
            hit(t[0], 0, 115);
            const int extra[] = {6, 7, 10, 11, 13};
            for (int k = 0; k < 2; ++k) hit(t[0], extra[r.nextInt(5)], 100);
            every(1, 4, 8, 110); if (r.nextBool()) hit(t[1], 7 + 4 * r.nextInt(2), 45);
            every(3, 0, 2, 85); if (r.nextBool()) hit(t[3], 15, 55);
            for (int s : {3, 11}) if (r.nextFloat() < 0.4f) hit(t[5], s, 70);
            break;
        }
        case 3: {  // trap, 32 steps at 1/16 (half time)
            for (auto& tp : t) tp.length = 32;
            hit(t[0], 0, 120);
            for (int k = 0; k < 3; ++k) { const int s = 2 * r.nextInt(16); if (s != 8 && s != 24) hit(t[0], s, 105); }
            static const int slides[] = {0, -2, 3, 5, 7, -5};
            for (int s = 1; s < 32; ++s) if (t[0].steps[(size_t)s].vel) t[0].steps[(size_t)s].pitch = (int8_t)slides[r.nextInt(6)];
            hit(t[1], 8, 115); hit(t[1], 24, 115); hit(t[2], 8, 100); hit(t[2], 24, 100);
            for (int s = 0; s < 32; s += 2) hit(t[3], s, 85, r.nextFloat() < 0.18f ? 2 + r.nextInt(3) : 1);
            if (r.nextBool()) hit(t[4], 14, 75);
            for (int s : {11, 27}) if (r.nextFloat() < 0.4f) hit(t[5], s, 70);
            break;
        }
        case 4:   // drum and bass
            hit(t[0], 0, 115); hit(t[0], 10, 105); if (r.nextBool()) hit(t[0], 12, 90);
            every(1, 4, 8, 115);
            for (int s = 0; s < 16; ++s) if (s % 4 != 0 && r.nextFloat() < 0.22f) hit(t[2], s, 50);
            every(3, 0, 2, 85); if (r.nextBool()) hit(t[4], 14, 80);
            if (r.nextBool()) every(5, 0, 4, 70);
            every(7, 0, 1, 50);
            break;
        case 5:   // breakbeat
            hit(t[0], 0, 115); hit(t[0], 10, 100); if (r.nextBool()) hit(t[0], 2 + r.nextInt(2), 95);
            every(1, 4, 8, 115);
            for (int s : {6, 9, 13}) if (r.nextFloat() < 0.6f) hit(t[2], s, 50);
            every(3, 0, 2, 85); if (r.nextBool()) hit(t[4], 14, 80);
            break;
        case 6:   // electro
            hit(t[0], 0, 115); hit(t[0], 6, 100); hit(t[0], 10, 105); if (r.nextBool()) hit(t[0], 13, 90);
            every(2, 4, 8, 110);
            for (int s = 0; s < 16; ++s) hit(t[3], s, s % 2 ? 60 : 85);
            for (int s : {2, 5, 10}) if (r.nextFloat() < 0.6f) hit(t[7], s, 80);
            break;
        default: {  // experimental: polymeter, chance, conditions, odd rates
            static const int rates[] = {0, 1, 2, 3, 4, 5};
            for (int k = 0; k < kTracks; ++k) {
                auto& tp = t[(size_t)k];
                tp.length = 5 + r.nextInt(12);
                tp.rate = rates[r.nextInt(6)];
                for (int s = 0; s < tp.length; ++s)
                    if (r.nextFloat() < (k == 0 ? 0.3f : 0.25f))
                        hit(tp, s, 50 + r.nextInt(70), r.nextFloat() < 0.15f ? 2 + r.nextInt(3) : 1, r.nextFloat() < 0.3f ? 50 + r.nextInt(40) : 100,
                            r.nextFloat() < 0.15f ? (Cond)(5 + r.nextInt(9)) : Cond::Always);
                if (k == 0) hit(tp, 0, 115);
                if (r.nextFloat() < 0.3f) for (int s = 0; s < tp.length; ++s) tp.steps[(size_t)s].lock = (int8_t)(r.nextInt(161) - 80);
            }
            break;
        }
    }
    return t;
}

Bank makeBank(juce::Random& r, int g) {
    Bank b{};
    b[0] = mainPattern(r, g);
    // Variation: ghost notes, a moved kick, hat ratchets.
    b[1] = b[0];
    {
        auto& k = b[1][0];
        const int len = k.length;
        for (int tries = 0; tries < 8; ++tries) {
            const int s = 1 + r.nextInt(std::max(1, len - 1));
            if (k.steps[(size_t)s].vel == 0) { hit(k, s, 85); break; }
        }
        auto& gh = b[1][g == 4 || g == 5 ? 2 : 1];
        for (int tries = 0; tries < 2; ++tries) { const int s = r.nextInt(gh.length); if (gh.steps[(size_t)s].vel == 0) hit(gh, s, 45); }
        auto& h = b[1][3];
        for (int s = 0; s < h.length; ++s) if (h.steps[(size_t)s].vel && r.nextFloat() < 0.12f) h.steps[(size_t)s].ratchet = 2;
    }
    // Break: no kick, hats and percussion only, half the snares.
    b[2] = b[0];
    for (auto& st : b[2][0].steps) st = Step{};
    if (g != 7) hit(b[2][0], 0, 110);
    for (int s = 0; s < b[2][1].length; s += 2) b[2][1].steps[(size_t)s].vel = 0;
    // Fill: the main pattern with a snare roll over the last bar quarter, marked Fill so it also works as a fill.
    b[3] = b[0];
    {
        auto& sn = b[3][1];
        const int len = sn.length, from = len - std::max(2, len / 4);
        for (int s = from; s < len; ++s) hit(sn, s, 80 + 40 * (s - from) / std::max(1, len - from), s >= len - 2 ? 2 + r.nextInt(2) : 1, 100, Cond::Fill);
        if (b[3][5].length >= len) for (int s = from; s < len; s += 2) hit(b[3][5], s, 85, 1, 100, Cond::Fill);
    }
    return b;
}

juce::var bankToVar(const Bank& b) {
    juce::Array<juce::var> pats;
    for (const auto& p : b) {
        juce::Array<juce::var> tracks;
        for (const auto& tp : p) {
            auto* o = new juce::DynamicObject();
            o->setProperty("len", tp.length);
            o->setProperty("rate", kRateNames[juce::jlimit(0, kNumRates - 1, tp.rate)]);
            for (int l = 0; l < PatternStore::NumLanes; ++l) {
                const auto s = PatternStore::laneToString(tp, (PatternStore::Lane)l);
                if (s.isNotEmpty()) o->setProperty(PatternStore::laneKey((PatternStore::Lane)l), s);
            }
            tracks.add(juce::var(o));
        }
        pats.add(juce::var(tracks));
    }
    return pats;
}

const char* kAdjectives[] = {"Ionised", "Coronal", "Spectral", "Charged", "Polar", "Radiant", "Solar", "Magnetic", "Plasma", "Flaring", "Ultra", "Photonic"};
const char* kNouns[] = {"Pulse", "Burst", "Orbit", "Storm", "Flare", "Drift", "Wind", "Loop", "Signal", "Cascade", "Field", "Engine"};
}  // namespace

Kit generate(juce::Random& rng, int g) {
    if (g < 0 || g >= kNumGenres) g = rng.nextInt(kNumGenres);
    const Genre gen = genre(g);
    Kit kit;
    auto& P = kit.params;
    juce::Array<juce::var> names;
    for (int t = 0; t < kTracks; ++t) {
        const Role& role = gen.roles[(size_t)t];
        const juce::String x = "t" + juce::String(t + 1);
        const Model m = role.models[(size_t)rng.nextInt((int)role.models.size())];
        P.set(x + "Model", kModelNames[(int)m]);
        P.set(x + "Tune", round3(std::round(pick(rng, role.tune) * 2.f) / 2.f));
        P.set(x + "Decay", round3(pick(rng, role.decay)));
        P.set(x + "Tone", round3(pick(rng, role.tone)));
        P.set(x + "Snap", round3(pick(rng, role.snap)));
        P.set(x + "Color", round3(pick(rng, role.color)));
        P.set(x + "Sweep", round3(pick(rng, role.sweep)));
        P.set(x + "Level", round3(pick(rng, role.level)));
        P.set(x + "SendB", round3(role.sendB * (0.5f + rng.nextFloat())));
        P.set(x + "Pan", round3(role.pan * (0.6f + 0.8f * rng.nextFloat())));
        P.set(x + "Choke", kChokeNames[role.choke]);
        P.set(x + "Lock", kLockNames[rng.nextInt(5)]);
        if (g == 1 || g == 7) if (rng.nextFloat() < 0.25f) P.set(x + "Drive", round3(0.1f + 0.4f * rng.nextFloat()));
        if (g == 7 && rng.nextFloat() < 0.3f) {
            P.set(x + "Filter", kFilterNames[1 + rng.nextInt(kNumFilterTypes - 1)]);
            P.set(x + "Cutoff", round3(200.f * std::pow(40.f, rng.nextFloat())));
            P.set(x + "Res", round3(0.2f + 0.6f * rng.nextFloat()));
        }
        names.add(role.name);
    }
    P.set("tempo", std::round(gen.tempoLo + (gen.tempoHi - gen.tempoLo) * rng.nextDouble()));
    P.set("swing", round3(gen.swingLo + (gen.swingHi - gen.swingLo) * rng.nextFloat()));
    P.set("glMix", round3(0.3f + 0.4f * rng.nextFloat()));
    P.set("glThr", round3(-16.f + 6.f * rng.nextFloat()));
    P.set("rvDecay", round3(0.8f + (g == 7 ? 4.f : 1.8f) * rng.nextFloat()));
    P.set("rvRet", round3(0.5f + 0.3f * rng.nextFloat()));
    if (g == 1 || g == 7 || rng.nextFloat() < 0.3f) { P.set("dlyRet", round3(0.3f + 0.3f * rng.nextFloat())); P.set("t6SendA", round3(0.2f + 0.3f * rng.nextFloat())); }
    if (rng.nextFloat() < 0.35f) { P.set("drvAmt", round3(0.1f + 0.25f * rng.nextFloat())); P.set("drvMode", kDriveModeNames[rng.nextInt(3)]); }
    kit.patterns = bankToVar(makeBank(rng, g));
    kit.names = names;
    kit.category = kGenres[g];
    kit.name = juce::String(kAdjectives[rng.nextInt(12)]) + " " + kNouns[rng.nextInt(12)];
    return kit;
}

}  // namespace ignite
