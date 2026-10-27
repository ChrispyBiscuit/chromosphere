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

Role R(const char* n, std::vector<Model> m, Range tune, Range decay, Range tone, Range snap, Range color, Range sweep, Range level,
       float sendB = 0.1f, int choke = 0, float pan = 0.f) {
    return {n, std::move(m), tune, decay, tone, snap, color, sweep, level, sendB, choke, pan};
}
// Engine-2 dial meanings: Core Kick Color = Punch, Gravity Kick Snap = Top and Color = Drive, Orbit Tom Color = Conga,
// Corona Cymbal Color = Crash, Spark Rim Sweep = Body, Solar Wind Color = noise texture.
Role kick(Range tune, Range decay, std::vector<Model> m, Range punch = {0.3f, 0.6f}, Range click = {0.35f, 0.65f}) {
    return R("Kick", std::move(m), tune, decay, {0.4f, 0.6f}, click, punch, {0.4f, 0.6f}, {-1.f, 0.f}, 0.f);
}
Role snare(const char* n = "Snare", Range level = {-6.f, -2.f}) { return R(n, {Model::FlareSnare}, {-1.f, 5.f}, {0.35f, 0.55f}, {0.5f, 0.65f}, {0.5f, 0.8f}, {0.5f, 0.7f}, {0.25f, 0.45f}, level, 0.15f); }
Role clap(Range level = {-4.f, -2.f}) { return R("Clap", {Model::BurstClap}, {-2.f, 2.f}, {0.35f, 0.5f}, {0.45f, 0.6f}, {0.5f, 0.65f}, {0.45f, 0.65f}, {0.4f, 0.6f}, level, 0.2f); }
Role hat(bool open, Range tone = {0.5f, 0.7f}, int choke = 1) {
    return R(open ? "Open Hat" : "Closed Hat", {Model::IonHat}, {-1.f, 2.f}, open ? Range{0.48f, 0.6f} : Range{0.15f, 0.24f}, tone,
             {0.35f, 0.55f}, {0.4f, 0.65f}, {0.1f, 0.3f}, open ? Range{-12.f, -8.f} : Range{-12.f, -9.f}, open ? 0.08f : 0.f, choke, 0.12f);
}
Role conga(float pan) { return R("Conga", {Model::OrbitTom}, {9.f, 17.f}, {0.3f, 0.42f}, {0.4f, 0.55f}, {0.4f, 0.6f}, {0.7f, 0.9f}, {0.2f, 0.4f}, {-10.f, -6.f}, 0.12f, 0, pan); }
Role rim(float pan) { return R("Rim", {Model::SparkRim}, {-3.f, 3.f}, {0.25f, 0.4f}, {0.4f, 0.6f}, {0.4f, 0.6f}, {0.05f, 0.35f}, {0.3f, 0.7f}, {-13.f, -9.f}, 0.1f, 0, pan); }
Role shaker(float pan) { return R("Shaker", {Model::SolarWind}, {8.f, 13.f}, {0.15f, 0.28f}, {0.3f, 0.5f}, {0.5f, 0.7f}, {0.6f, 0.8f}, {0.45f, 0.55f}, {-17.f, -13.f}, 0.05f, 0, pan); }
Role fmPerc(const char* n, Range tune, float pan) { return R(n, {Model::PulsarFm}, tune, {0.18f, 0.32f}, {0.35f, 0.6f}, {0.5f, 0.7f}, {0.2f, 0.7f}, {0.15f, 0.35f}, {-14.f, -10.f}, 0.12f, 0, pan); }
Role ride(Range level = {-16.f, -13.f}) { return R("Ride", {Model::CoronaCymbal}, {0.f, 3.f}, {0.35f, 0.5f}, {0.55f, 0.7f}, {0.35f, 0.5f}, {0.1f, 0.3f}, {0.3f, 0.45f}, level, 0.06f, 0, -0.2f); }

Genre genre(int g) {
    switch (g) {
        case 0: return {118, 124, 0.54f, 0.6f, {kick({-2, 1}, {0.45f, 0.6f}, {Model::CoreKick}, {0.15f, 0.4f}, {0.2f, 0.4f}), snare("Snare", {-11.f, -7.f}), clap(),
                         hat(false, {0.4f, 0.55f}), hat(true, {0.4f, 0.55f}), conga(-0.3f), rim(0.3f), shaker(-0.2f)}};
        case 1: return {124, 128, 0.52f, 0.57f, {kick({-1, 1}, {0.4f, 0.5f}, {Model::CoreKick}, {0.45f, 0.7f}, {0.45f, 0.65f}), snare("Snare", {-9.f, -6.f}), clap(),
                         hat(false), hat(true), fmPerc("Perc", {3, 12}, 0.3f),
                         R("Bongo", {Model::OrbitTom, Model::SparkRim}, {12.f, 19.f}, {0.25f, 0.35f}, {0.45f, 0.6f}, {0.45f, 0.65f}, {0.65f, 0.85f}, {0.3f, 0.5f}, {-11.f, -8.f}, 0.1f, 0, -0.3f),
                         shaker(0.2f)}};
        case 2: return {128, 138, 0.5f, 0.53f, {kick({-2, 2}, {0.38f, 0.5f}, {Model::GravityKick, Model::CoreKick}, {0.4f, 0.75f}), snare(), clap(), hat(false), hat(true),
                         fmPerc("Perc", {-5, 12}, 0.3f), R("Rim", {Model::SparkRim, Model::QuasarMetal, Model::NebulaModal}, {-5.f, 5.f}, {0.25f, 0.4f}, {0.4f, 0.65f}, {0.5f, 0.7f}, {0.1f, 0.5f}, {0.4f, 0.7f}, {-13.f, -9.f}, 0.15f, 0, -0.3f),
                         ride()}};
        case 3: return {134, 142, 0.5f, 0.5f, {kick({-2, 1}, {0.42f, 0.55f}, {Model::GravityKick}, {0.45f, 0.75f}, {0.55f, 0.8f}), snare("Snare", {-8.f, -4.f}), clap(),
                         hat(false, {0.6f, 0.8f}, 0), hat(true, {0.6f, 0.8f}, 0), fmPerc("Perc", {7, 19}, 0.3f), ride({-15.f, -12.f}),
                         R("Riser", {Model::SolarWind}, {-2.f, 2.f}, {0.85f, 0.95f}, {0.35f, 0.55f}, {0.f, 0.1f}, {0.15f, 0.35f}, {0.9f, 1.f}, {-18.f, -15.f}, 0.3f)}};
        case 4: return {170, 176, 0.5f, 0.52f, {kick({2, 5}, {0.28f, 0.38f}, {Model::GravityKick, Model::CoreKick}, {0.35f, 0.6f}, {0.6f, 0.85f}),
                         R("Snare", {Model::FlareSnare}, {2.f, 6.f}, {0.45f, 0.6f}, {0.55f, 0.7f}, {0.7f, 0.9f}, {0.55f, 0.7f}, {0.3f, 0.5f}, {-3.f, 0.f}, 0.15f),
                         snare("Ghost", {-14.f, -10.f}), hat(false), hat(true), ride(), rim(-0.3f), shaker(0.35f)}};
        case 5: return {120, 136, 0.5f, 0.56f, {kick({-1, 3}, {0.38f, 0.48f}, {Model::CoreKick, Model::GravityKick}, {0.45f, 0.7f}), snare("Snare", {-4.f, -1.f}), snare("Ghost", {-14.f, -10.f}),
                         hat(false), hat(true), R("Crash", {Model::CoronaCymbal}, {-2.f, 2.f}, {0.55f, 0.7f}, {0.5f, 0.65f}, {0.4f, 0.6f}, {0.7f, 0.9f}, {0.6f, 0.8f}, {-15.f, -12.f}, 0.1f, 0, -0.2f),
                         R("Tom", {Model::OrbitTom}, {-4.f, 3.f}, {0.4f, 0.5f}, {0.4f, 0.55f}, {0.4f, 0.55f}, {0.15f, 0.4f}, {0.35f, 0.5f}, {-9.f, -6.f}, 0.1f, 0, -0.3f), rim(0.3f)}};
        case 6: return {128, 136, 0.58f, 0.64f, {kick({-1, 2}, {0.4f, 0.5f}, {Model::CoreKick}, {0.35f, 0.6f}), snare(), clap({-6.f, -3.f}), hat(false), hat(true),
                         rim(-0.3f), R("Tom", {Model::OrbitTom}, {0.f, 5.f}, {0.35f, 0.45f}, {0.4f, 0.55f}, {0.4f, 0.55f}, {0.25f, 0.5f}, {0.35f, 0.5f}, {-10.f, -7.f}, 0.1f, 0, 0.25f), shaker(-0.25f)}};
        default: return {90, 140, 0.5f, 0.6f, {kick({-6, 6}, {0.3f, 0.7f}, {Model::GravityKick, Model::CoreKick, Model::NebulaModal}),
                         R("Snare", {Model::FlareSnare, Model::NebulaModal, Model::PulsarFm}, {-5.f, 7.f}, {0.3f, 0.6f}, {0.35f, 0.65f}, {0.4f, 0.7f}, {0.1f, 0.9f}, {0.1f, 0.5f}, {-8.f, -3.f}),
                         R("Clap", {Model::BurstClap, Model::SolarWind}, {-6.f, 6.f}, {0.3f, 0.6f}, {0.35f, 0.65f}, {0.4f, 0.7f}, {0.1f, 0.9f}, {0.1f, 0.6f}, {-10.f, -5.f}, 0.2f, 0, -0.2f),
                         hat(false), R("Metal", {Model::CoronaCymbal, Model::QuasarMetal}, {-6.f, 8.f}, {0.25f, 0.5f}, {0.35f, 0.65f}, {0.4f, 0.7f}, {0.1f, 0.9f}, {0.1f, 0.5f}, {-14.f, -9.f}, 0.15f, 0, 0.25f),
                         R("Modal", {Model::NebulaModal}, {-8.f, 12.f}, {0.25f, 0.5f}, {0.35f, 0.65f}, {0.4f, 0.7f}, {0.1f, 0.9f}, {0.1f, 0.5f}, {-13.f, -8.f}, 0.15f, 0, -0.3f),
                         fmPerc("FM", {-5, 12}, 0.3f),
                         R("Zap", {Model::PlasmaZap, Model::SolarWind}, {-4.f, 12.f}, {0.25f, 0.5f}, {0.35f, 0.65f}, {0.4f, 0.7f}, {0.1f, 0.9f}, {0.1f, 0.6f}, {-14.f, -9.f}, 0.15f)}};
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
    auto some = [&](int track, std::initializer_list<int> steps, float p, int vel) { for (int s : steps) if (r.nextFloat() < p) hit(t[(size_t)track], s, vel); };
    switch (g) {
        case 0: {  // deep house
            every(0, 0, 4, 110); every(2, 4, 8, 100); if (r.nextFloat() < 0.3f) hit(t[1], 11, 40);
            for (int s = 0; s < 16; ++s) if (s % 4 == 0) hit(t[3], s, 60); else if (s % 4 == 3) hit(t[3], s, 40);
            every(4, 2, 4, 80);
            some(5, {3, 6, 10, 13, 14}, 0.55f, 85);
            static const int8_t congaPitch[] = {0, 0, 5, -2, 3, 7};
            for (int s = 0; s < 16; ++s) if (t[5].steps[(size_t)s].vel) t[5].steps[(size_t)s].pitch = congaPitch[r.nextInt(6)];
            some(6, {3, 9, 11, 14}, 0.5f, 80);
            for (int s = 0; s < 16; ++s) hit(t[7], s, s % 2 ? 82 : 40);
            break;
        }
        case 1:   // tech house
            every(0, 0, 4, 115); every(2, 4, 8, 105); if (r.nextBool()) hit(t[1], 15, 55);
            for (int s = 0; s < 16; ++s) hit(t[3], s, s % 2 ? 60 : 95);
            every(4, 2, 4, 80);
            t[5].length = r.nextBool() ? 7 : 16;
            for (int s = 0; s < t[5].length; ++s) if (r.nextFloat() < 0.35f) { hit(t[5], s, 70 + r.nextInt(40)); t[5].steps[(size_t)s].lock = (int8_t)(r.nextInt(81) - 40); }
            some(6, {3, 6, 10, 11, 14}, 0.5f, 85);
            for (int s = 0; s < 16; ++s) hit(t[7], s, s % 2 ? 40 : 82);
            break;
        case 2:   // techno
            every(0, 0, 4, 115); every(3, 2, 4, 85); if (r.nextBool()) for (int s = 0; s < 16; ++s) hit(t[3], s, s % 2 ? 85 : 55);
            if (r.nextBool()) every(2, 4, 8, 90);
            t[5].length = 10 + r.nextInt(4);
            for (int s = 0; s < t[5].length; ++s) if (r.nextFloat() < 0.35f) hit(t[5], s, 70 + r.nextInt(40), 1, r.nextBool() ? 100 : 70);
            for (int s : {6, 14}) hit(t[6], s, 80);
            if (r.nextBool()) every(7, 2, 4, 70);
            break;
        case 3: {  // trance
            every(0, 0, 4, 120); every(2, 4, 8, 105);
            if (r.nextBool()) for (int s = 0; s < 16; ++s) hit(t[3], s, s % 4 == 2 ? 60 : 90);
            else for (int s : {0, 2, 3, 4, 6, 7, 8, 10, 11, 12, 14, 15}) hit(t[3], s, 85);
            every(4, 2, 4, 95);
            static const int8_t arp[] = {0, 5, 7, 12, 3, 10};
            for (int s = 1; s < 16; s += 3) if (r.nextFloat() < 0.6f) { hit(t[5], s, 80); t[5].steps[(size_t)s].pitch = arp[r.nextInt(6)]; }
            if (r.nextBool()) every(6, 2, 4, 75); else every(6, 0, 2, 65);
            t[7].length = 32; hit(t[7], 0, 90, 1, 100, Cond::C14);
            break;
        }
        case 4:   // drum and bass
            hit(t[0], 0, 115); hit(t[0], 10, 105); if (r.nextBool()) hit(t[0], 12, 90);
            every(1, 4, 8, 115);
            for (int s = 0; s < 16; ++s) if (s % 4 != 0 && r.nextFloat() < 0.22f) hit(t[2], s, 50);
            every(3, 0, 2, 85); if (r.nextBool()) hit(t[4], 14, 80);
            if (r.nextBool()) every(5, 0, 4, 70);
            some(6, {3, 7, 11}, 0.4f, 75);
            for (int s = 0; s < 16; ++s) hit(t[7], s, s % 2 ? 50 : 80);
            break;
        case 5:   // breakbeat
            hit(t[0], 0, 115); hit(t[0], 10, 100); if (r.nextBool()) hit(t[0], 2 + r.nextInt(2), 95);
            every(1, 4, 8, 115);
            some(2, {6, 9, 13}, 0.6f, 50);
            every(3, 0, 2, 85); if (r.nextBool()) hit(t[4], 14, 80);
            break;
        case 6:   // UK garage (two-step)
            hit(t[0], 0, 115); hit(t[0], 10, 100); if (r.nextBool()) hit(t[0], 13, 85);
            every(1, 4, 8, 110); if (r.nextBool()) every(2, 4, 8, 90);
            for (int s = 0; s < 16; s += 2) { hit(t[3], s, 80); if (s % 4 == 2) t[3].steps[(size_t)s].nudge = 8; }
            if (r.nextBool()) hit(t[4], 14, 80);
            some(5, {3, 6, 11, 14}, 0.6f, 80);
            some(6, {7, 10}, 0.4f, 75);
            for (int s = 0; s < 16; ++s) hit(t[7], s, s % 2 ? 40 : 82);
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
        if (g == 2 || g == 7) if (rng.nextFloat() < 0.25f) P.set(x + "Drive", round3(0.1f + 0.4f * rng.nextFloat()));
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
    if (g == 2 || g == 3 || g == 7 || rng.nextFloat() < 0.3f) { P.set("dlyRet", round3(0.3f + 0.3f * rng.nextFloat())); P.set("t6SendA", round3(0.2f + 0.3f * rng.nextFloat())); }
    if (rng.nextFloat() < 0.35f) { P.set("drvAmt", round3(0.1f + 0.25f * rng.nextFloat())); P.set("drvMode", kDriveModeNames[rng.nextInt(3)]); }
    kit.patterns = bankToVar(makeBank(rng, g));
    kit.names = names;
    kit.category = kGenres[g];
    kit.name = juce::String(kAdjectives[rng.nextInt(12)]) + " " + kNouns[rng.nextInt(12)];
    return kit;
}

}  // namespace ignite
