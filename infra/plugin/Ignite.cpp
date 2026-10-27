#include "Ignite.h"
#include <cmath>
#include <vector>

namespace ignite {
namespace {

struct Gen {
    juce::Random& r;
    juce::NamedValueSet p;
    float uni(float lo, float hi) { return lo + (hi - lo) * r.nextFloat(); }
    float logUni(float lo, float hi) { return lo * std::pow(hi / lo, r.nextFloat()); }
    int pick(int n) { return r.nextInt(n); }
    bool chance(float c) { return r.nextFloat() < c; }
    template <typename T> T one(std::initializer_list<T> l) { return *(l.begin() + pick((int)l.size())); }
    void set(const char* id, juce::var v) { p.set(id, v); }
    void env(const char* pre, float a, float d, float s, float rel) {
        const juce::String x(pre);
        set((x + "A").toRawUTF8(), a); set((x + "D").toRawUTF8(), d); set((x + "S").toRawUTF8(), s); set((x + "R").toRawUTF8(), rel);
    }
    int slot = 0;
    void mod(const char* src, const char* dst, float amt) {
        if (slot >= 8) return;
        const juce::String x = "m" + juce::String(++slot);
        p.set(x + "Src", src); p.set(x + "Dst", dst); p.set(x + "Amt", std::round(amt * 1000.f) / 1000.f);
    }
};

juce::String makeName(juce::Random& r) {
    static const char* a[] = {"Red", "Proto", "Hyper", "Ion", "Dark", "Solar", "Lunar", "Nova", "Magneto", "Astro", "Helio", "Cryo",
                              "Plasma", "Infra", "Photon", "Quasi", "Neutron", "Gamma", "Polar", "Stellar"};
    static const char* b[] = {"Drift", "Signal", "Orbit", "Flare", "Pulse", "Field", "Cascade", "Halo", "Comet", "Nebula", "Beacon",
                              "Core", "Wake", "Storm", "Relay", "Vector", "Horizon", "Corona", "Lattice", "Echo"};
    return juce::String(a[r.nextInt(20)]) + " " + b[r.nextInt(20)] + " " + juce::String(r.nextInt(90) + 10);
}
}  // namespace

Patch generate(juce::Random& rng, int archetype) {
    if (archetype < 0 || archetype >= kNumArchetypes) archetype = rng.nextInt(kNumArchetypes);
    Gen g{rng, {}};
    const juce::String kind = kArchetypes[archetype];

    // ---- Oscillators: tables that suit the archetype ----
    const char* tablesBass[] = {"Classic", "Classic", "Magnetar", "Event Horizon", "Plasma", "Fraunhofer", "Dark Matter"};
    const char* tablesBright[] = {"Classic", "Classic", "Fraunhofer", "Plasma", "Magnetar", "Pulsar", "Nebula"};
    const char* tablesSoft[] = {"Classic", "Nebula", "Ion Drive", "Quasar", "Dark Matter", "Fraunhofer"};
    const char* tablesBell[] = {"Quasar", "Magnetar", "Classic", "Ion Drive"};
    const char* const* pool = tablesBright; int poolN = 7;
    if (kind == "Bass") { pool = tablesBass; poolN = 7; }
    else if (kind == "Pad" || kind == "Drone") { pool = tablesSoft; poolN = 6; }
    else if (kind == "Keys") { pool = tablesBell; poolN = 4; }
    const juce::String tA = pool[g.pick(poolN)], tB = pool[g.pick(poolN)];
    g.set("aTable", tA); g.set("aMorph", g.uni(0.f, 1.f)); g.set("aLevel", g.uni(0.65f, 0.85f));
    const bool wide = kind == "Pad" || kind == "Drone" || (kind == "Lead" && g.chance(0.5f)) || (kind == "Pluck" && g.chance(0.4f));
    g.set("aUnison", wide ? g.one({3, 4, 5, 6, 7}) : g.one({1, 1, 2}));
    g.set("aDetune", wide ? g.uni(0.15f, 0.35f) : g.uni(0.05f, 0.2f));
    g.set("aWidth", wide ? g.uni(0.6f, 1.f) : g.uni(0.2f, 0.6f));
    if (g.chance(0.25f)) { g.set("aWarp", g.one({"Bend", "Mirror", "Quantize", "FM from B"})); g.set("aWarpAmt", g.uni(0.15f, 0.6f)); }
    const bool bOn = g.chance(0.75f);
    g.set("bOn", bOn);
    if (bOn) {
        g.set("bTable", tB); g.set("bMorph", g.uni(0.f, 1.f)); g.set("bLevel", g.uni(0.25f, 0.5f));
        g.set("bOct", kind == "Bass" ? -1 : g.one({-1, 0, 0, 1}));
        if (g.chance(0.2f)) g.set("bSemi", g.one({7, 5, 12, -5}));
        g.set("bFine", g.uni(-8.f, 8.f));
    }
    if (kind == "Bass" || g.chance(0.3f)) { g.set("subLevel", g.uni(0.2f, 0.6f)); g.set("subShape", g.one({"Sine", "Triangle", "Square"})); }
    if (kind == "Drone" || kind == "FX" || g.chance(0.15f)) {
        g.set("noiseType", g.one({"White", "Pink", "Solar Wind", "Crackle"}));
        g.set("noiseLevel", kind == "FX" ? g.uni(0.2f, 0.6f) : g.uni(0.05f, 0.3f));
    }

    // ---- Filter and envelopes ----
    const char* ftype = kind == "FX" ? g.one({"Comb", "BP 12", "Formant", "LP 24", "Notch"})
                      : kind == "Pad" || kind == "Drone" ? g.one({"LP 24", "LP 12", "LP 24", "Formant", "BP 12"})
                      : g.one({"LP 24", "LP 24", "LP 12", "BP 12"});
    g.set("fType", ftype);
    float cutoff = 1000.f, envAmt = 0.f;
    if (kind == "Bass") { cutoff = g.logUni(300.f, 1500.f); envAmt = g.uni(15.f, 45.f); g.env("amp", 0.002f, g.uni(0.2f, 0.8f), g.uni(0.5f, 0.95f), g.uni(0.06f, 0.2f)); g.env("fenv", 0.001f, g.uni(0.12f, 0.5f), g.uni(0.f, 0.3f), 0.15f); }
    else if (kind == "Lead") { cutoff = g.logUni(1500.f, 7000.f); envAmt = g.uni(5.f, 20.f); g.env("amp", g.uni(0.002f, 0.03f), 0.4f, g.uni(0.7f, 0.95f), g.uni(0.15f, 0.5f)); g.env("fenv", 0.01f, g.uni(0.3f, 1.f), 0.4f, 0.3f); }
    else if (kind == "Pad") { cutoff = g.logUni(700.f, 4000.f); envAmt = g.uni(0.f, 20.f); g.env("amp", g.uni(0.5f, 2.5f), 2.f, g.uni(0.7f, 1.f), g.uni(1.5f, 5.f)); g.env("fenv", g.uni(1.f, 4.f), 3.f, 0.4f, 3.f); }
    else if (kind == "Pluck") { cutoff = g.logUni(600.f, 2500.f); envAmt = g.uni(25.f, 45.f); g.env("amp", 0.001f, g.uni(0.3f, 1.2f), 0.f, g.uni(0.2f, 0.6f)); g.env("fenv", 0.001f, g.uni(0.15f, 0.5f), 0.f, 0.2f); }
    else if (kind == "Keys") { cutoff = g.logUni(2500.f, 8000.f); envAmt = g.uni(0.f, 15.f); g.env("amp", 0.002f, g.uni(1.f, 3.f), g.uni(0.f, 0.4f), g.uni(0.3f, 0.8f)); g.set("fVel", g.uni(0.2f, 0.5f)); }
    else if (kind == "Arp") { cutoff = g.logUni(900.f, 3000.f); envAmt = g.uni(15.f, 35.f); g.env("amp", 0.001f, g.uni(0.15f, 0.4f), g.uni(0.f, 0.4f), g.uni(0.1f, 0.3f)); g.env("fenv", 0.001f, g.uni(0.1f, 0.3f), 0.f, 0.15f); }
    else if (kind == "Drone") { cutoff = g.logUni(400.f, 2500.f); g.env("amp", g.uni(2.f, 4.f), 2.f, 1.f, g.uni(4.f, 8.f)); }
    else { cutoff = g.logUni(300.f, 6000.f); envAmt = g.uni(-60.f, 72.f); g.env("amp", g.uni(0.001f, 3.f), g.uni(0.5f, 4.f), g.uni(0.f, 1.f), g.uni(0.5f, 3.f)); g.env("fenv", g.uni(0.001f, 4.f), g.uni(0.5f, 4.f), g.uni(0.f, 1.f), 1.f); g.env("menv", g.uni(0.001f, 4.f), g.uni(0.5f, 4.f), g.uni(0.f, 1.f), 1.f); }
    g.set("fCutoff", cutoff); g.set("fEnv", envAmt); g.set("fRes", g.uni(0.05f, kind == "FX" ? 0.85f : 0.5f));
    g.set("fKey", g.uni(0.3f, 0.7f));
    if (g.chance(0.3f)) g.set("fDrive", g.uni(0.1f, 0.4f));
    if (std::string(ftype) == "Comb") g.set("fKey", 1.f);

    // ---- Voices ----
    if (kind == "Bass" || (kind == "Lead" && g.chance(0.7f))) { g.set("voiceMode", g.chance(0.5f) ? "Legato" : "Mono"); g.set("glide", g.uni(0.02f, 0.1f)); }

    // ---- LFOs and the mod envelope ----
    g.set("l1Shape", g.one({"Sine", "Triangle", "Drift", "S&H", "Sine"}));
    if (g.chance(0.5f)) { g.set("l1Sync", true); g.set("l1Div", g.one({"1/4", "1/8", "1/8.", "1/16", "1 bar", "2 bars"})); }
    else g.set("l1Rate", kind == "Pad" || kind == "Drone" ? g.logUni(0.03f, 0.4f) : g.logUni(0.2f, 6.f));
    g.set("l1Retrig", !(kind == "Pad" || kind == "Drone"));
    g.set("l2Shape", "Sine"); g.set("l2Rate", g.uni(4.6f, 5.8f)); g.set("l2Fade", g.uni(0.3f, 1.f));
    if (kind != "FX") g.env("menv", g.uni(0.001f, kind == "Pad" ? 4.f : 0.3f), g.uni(0.3f, kind == "Pad" ? 6.f : 1.2f), 0.f, 0.5f);

    // ---- Modulation: one movement slot, then the conventions ----
    const char* move = g.one({"Morph A", "Cutoff", "Morph B", "Pan", "Detune"});
    float moveAmt = std::string(move) == "Cutoff" ? g.uni(0.05f, 0.2f) : g.uni(0.15f, 0.5f);
    g.mod("LFO 1", move, moveAmt);
    if (kind != "Lead" && g.chance(0.6f)) g.mod("Mod Env", g.one({"Morph A", "Morph B", "Warp A"}), g.uni(0.2f, 0.6f));
    g.mod("Heat", g.one({"Cutoff", "Morph A", "Filter Drive"}), g.uni(0.3f, 0.5f));
    g.mod("Gravity", bOn ? g.one({"Sub Level", "Level B"}) : "Sub Level", g.uni(0.3f, 0.5f));
    g.mod("Orbit", g.one({"Detune", "LFO 1 Rate", "Morph B", "Morph A"}), g.uni(0.25f, 0.5f));
    g.mod("Flux", g.one({"FX Reverb", "FX Delay", "FX Chorus"}), g.uni(0.25f, 0.45f));
    if (kind == "Lead") { g.mod("LFO 2", "Pitch", 0.008f); g.mod("Mod Wheel", "LFO 2 Amount", 1.f); }   // wheel vibrato
    else g.mod("Mod Wheel", g.one({"Cutoff", "Morph A", "Resonance", "Detune"}), g.uni(0.25f, 0.45f));
    g.mod("Aftertouch", g.one({"Cutoff", "Morph A", "Level B", "Resonance"}), g.uni(0.2f, 0.35f));

    // ---- Arp ----
    if (kind == "Arp") {
        g.set("arpOn", true);
        g.set("arpMode", g.one({"Up", "Up/Down", "Orbit", "Pulsar", "Converge", "As Played", "Random"}));
        g.set("arpDiv", g.one({"1/16", "1/16", "1/8", "1/8T", "1/16T"}));
        g.set("arpOct", g.one({1, 2, 2, 3}));
        g.set("arpGate", g.uni(0.35f, 0.7f));
        if (g.chance(0.4f)) g.set("arpSwing", g.uni(0.1f, 0.4f));
        for (int s = 1; s <= 16; ++s) {
            const juce::String x = "s" + juce::String(s);
            const bool down = (s - 1) % 4 == 0;
            g.p.set(x + "Vel", down ? 1.f : g.chance(0.15f) ? 0.f : g.uni(0.4f, 0.9f));
            if (g.chance(0.12f)) g.p.set(x + "Tr", g.one({12, 7, -12, 5}));
            if (g.chance(0.08f)) g.p.set(x + "Rat", g.one({2, 3}));
        }
        if (g.slot < 8) g.mod("Arp Step", "Cutoff", g.uni(0.1f, 0.25f));
    }

    // ---- Effects ----
    const bool dry = kind == "Bass";
    g.set("rvMix", dry ? 0.f : kind == "Pad" || kind == "Drone" ? g.uni(0.25f, 0.45f) : g.uni(0.1f, 0.25f));
    g.set("rvSize", g.uni(1.f, 2.6f)); g.set("rvDecay", kind == "Drone" ? g.uni(6.f, 14.f) : g.uni(1.5f, 5.f));
    if (!dry && g.chance(0.6f)) { g.set("dlyDiv", g.one({"1/8.", "1/4.", "1/8", "1/4"})); g.set("dlyMix", g.uni(0.08f, 0.25f)); g.set("dlyFb", g.uni(0.25f, 0.5f)); }
    if (g.chance(0.4f)) { g.set("chMix", g.uni(0.15f, 0.4f)); g.set("chRate", g.uni(0.2f, 0.8f)); }
    if (g.chance(0.3f)) { g.set("drvMode", g.one({"Soft", "Tube", "Cosmic Ray", "Fold"})); g.set("drvAmt", g.uni(0.1f, 0.35f)); }
    g.set("outDb", -3.f);
    return {g.p, makeName(rng), "Ignite / " + kind};
}

}  // namespace ignite
