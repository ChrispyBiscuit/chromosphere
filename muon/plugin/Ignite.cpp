#include "Ignite.h"

namespace ignite {

namespace {
struct Builder {
    juce::Random& r;
    juce::NamedValueSet p;
    juce::Array<juce::var> macros;
    float in(float a, float b) { return a + (b - a) * r.nextFloat(); }
    int pick(int n) { return r.nextInt(n); }
    void set(const juce::String& id, juce::var v) { p.set(id, v); }
    void macro(int m, const juce::String& id, float amt) {
        auto* o = new juce::DynamicObject();
        o->setProperty("macro", m); o->setProperty("id", id); o->setProperty("amount", amt);
        macros.add(juce::var(o));
    }
    // The fixed macro meanings, applied to whichever bands are in use.
    void standardMacros(int bands) {
        for (int b = 1; b <= bands; ++b) {
            const juce::String x = "b" + juce::String(b);
            macro(0, x + "Atk", 0.35f);                                     // Velocity: the arrival
            if (b == 1) macro(1, x + "Sus", 0.25f);                          // Mass: weight
            macro(2, x + "Drv", 0.3f);                                      // Charge: energy
            macro(3, x + "Sus", b == 1 ? 0.15f : 0.3f);                     // Decay: what is left
        }
        macro(1, "sub", 0.4f); macro(1, "phantom", 0.3f);
        macro(2, "b" + juce::String(bands) + "Harm", 0.35f);
        macro(3, "resMix", 0.35f); macro(3, "resDecay", 0.3f); macro(3, "glowSend", 0.3f);
    }
};

const char* const kFirst[] = {"Cherenkov", "Cosmic", "Lepton", "Pion", "Air", "Cascade", "Drift", "Tau", "Spin", "Halo", "Ionised", "Polar",
                              "Neutrino", "Shower", "Track", "Muonic", "Proton", "Heavy", "Swift", "Graviton"};
const char* const kSecond[] = {"Strike", "Hammer", "Bloom", "Ring", "Pulse", "Edge", "Front", "Wake", "Trail", "Core", "Glow", "Burst",
                               "Shell", "Spark", "Arc", "Decay", "Event", "Flash", "Echo", "Weight"};
}  // namespace

Setting generate(juce::Random& rng, int source) {
    if (source < 0 || source >= kNumSources) source = rng.nextInt(kNumSources);
    Builder b{rng, {}, {}};
    Setting s;
    s.category = juce::String(kSources[source]);
    int bands = 3;
    switch (source) {
        case 0: {   // Kick: punch on the low band, a tuned ring, sub weight
            bands = 2;
            b.set("bands", 1); b.set("xLow", b.in(90.f, 160.f));
            b.set("b1Atk", b.in(0.3f, 0.7f)); b.set("b1Sus", b.in(-0.4f, 0.2f)); b.set("b1Spd", b.in(0.4f, 0.8f));
            b.set("b2Atk", b.in(0.2f, 0.8f)); b.set("b2Sus", b.in(-0.6f, -0.1f)); b.set("b2Clip", b.in(0.f, 0.4f));
            b.set("b1H2", b.in(0.2f, 0.6f)); b.set("b1H3", b.in(0.f, 0.3f)); b.set("b1Harm", b.in(0.2f, 0.5f));
            b.set("sub", b.in(0.f, 0.4f)); b.set("subRange", b.in(90.f, 140.f));
            b.set("resMode", 0); b.set("chord", b.pick(2)); b.set("root", 28 + b.pick(12)); b.set("resDecay", b.in(0.15f, 0.6f));
            b.set("resMix", b.in(0.f, 0.3f)); b.set("strike", b.in(0.4f, 0.8f));
            break;
        }
        case 1: {   // Snare: crack on top, a skin body, room in the tail
            b.set("xLow", b.in(150.f, 250.f)); b.set("xHigh", b.in(2500.f, 5000.f));
            b.set("b1Atk", b.in(0.f, 0.4f)); b.set("b2Atk", b.in(0.3f, 0.8f)); b.set("b3Atk", b.in(0.3f, 0.9f));
            b.set("b2Sus", b.in(-0.2f, 0.5f)); b.set("b3Sus", b.in(-0.3f, 0.4f)); b.set("b3Clip", b.in(0.f, 0.5f));
            b.set("b3Curve", 1); b.set("b3Drv", b.in(0.1f, 0.4f));
            b.set("resMode", 1); b.set("material", 1); b.set("root", 50 + b.pick(14)); b.set("size", b.in(0.3f, 0.7f));
            b.set("resDecay", b.in(0.1f, 0.4f)); b.set("resMix", b.in(0.1f, 0.4f));
            b.set("glowSrc", 1); b.set("glowSend", b.in(0.f, 0.35f)); b.set("glowDecay", b.in(0.6f, 1.6f)); b.set("glowSize", b.in(0.6f, 1.2f));
            break;
        }
        case 2: {   // Bass: weight, phantom harmonics, mono low end
            bands = 2;
            b.set("bands", 1); b.set("xLow", b.in(120.f, 300.f));
            b.set("b1Sus", b.in(0.f, 0.3f)); b.set("b1Atk", b.in(-0.2f, 0.3f));
            b.set("b2Atk", b.in(0.1f, 0.5f)); b.set("b2Curve", b.pick(3)); b.set("b2Drv", b.in(0.15f, 0.5f));
            b.set("b1H2", b.in(0.2f, 0.6f)); b.set("b1H3", b.in(0.1f, 0.5f)); b.set("b1Harm", b.in(0.2f, 0.6f));
            b.set("phantom", b.in(0.1f, 0.5f)); b.set("sub", b.in(0.f, 0.3f)); b.set("mono", b.in(80.f, 160.f));
            break;
        }
        case 3: {   // Vocal: gentle level, presence harmonics, a little air
            b.set("xLow", b.in(180.f, 300.f)); b.set("xHigh", b.in(3000.f, 6000.f));
            b.set("b2Atk", b.in(0.f, 0.3f)); b.set("b3Atk", b.in(-0.3f, 0.2f)); b.set("b1Sus", b.in(-0.3f, 0.f));
            b.set("b2H2", b.in(0.1f, 0.35f)); b.set("b2H3", b.in(0.f, 0.2f)); b.set("b2Harm", b.in(0.2f, 0.5f));
            b.set("b3Curve", 2); b.set("b3Drv", b.in(0.1f, 0.35f)); b.set("b3Tone", b.in(0.55f, 0.8f));
            b.set("glowSrc", 2); b.set("glowSend", b.in(0.f, 0.25f)); b.set("glowDecay", b.in(1.f, 2.5f));
            break;
        }
        case 4: {   // Bus: glue and snap, tube warmth, parallel mix
            b.set("b1Atk", b.in(0.1f, 0.4f)); b.set("b2Atk", b.in(0.1f, 0.5f)); b.set("b3Atk", b.in(0.f, 0.4f));
            b.set("b1Sus", b.in(-0.2f, 0.2f)); b.set("b2Sus", b.in(-0.3f, 0.1f));
            for (const char* x : {"b1", "b2", "b3"}) { b.set(juce::String(x) + "Curve", 1); b.set(juce::String(x) + "Drv", b.in(0.05f, 0.3f)); }
            b.set("b2Clip", b.in(0.f, 0.3f)); b.set("mix", b.in(0.6f, 1.f)); b.set("look", 1);
            break;
        }
        default: {  // Weird: struck metal and glass, folded drive, long afterglow
            b.set("bands", b.pick(3)); bands = 3;
            for (const char* x : {"b1", "b2", "b3"}) {
                b.set(juce::String(x) + "Atk", b.in(-0.5f, 1.f)); b.set(juce::String(x) + "Sus", b.in(-1.f, 0.6f));
                b.set(juce::String(x) + "Curve", b.pick(6)); b.set(juce::String(x) + "Drv", b.in(0.f, 0.7f));
                b.set(juce::String(x) + "H" + juce::String(2 + b.pick(7)), b.in(0.3f, 1.f)); b.set(juce::String(x) + "Harm", b.in(0.2f, 0.9f));
            }
            b.set("resMode", b.pick(3)); b.set("material", b.pick(5)); b.set("chord", b.pick(7)); b.set("root", 36 + b.pick(36));
            b.set("resDecay", b.in(0.3f, 3.f)); b.set("resMix", b.in(0.3f, 0.8f)); b.set("strike", b.in(0.5f, 1.f)); b.set("spread", b.in(0.f, 0.6f));
            b.set("glowSend", b.in(0.2f, 0.7f)); b.set("glowDecay", b.in(2.f, 8.f)); b.set("glowSize", b.in(1.f, 2.5f));
            break;
        }
    }
    b.set("clipGuard", true);
    b.standardMacros(bands);
    s.params = b.p;
    s.macros = b.macros;
    s.name = juce::String(kFirst[rng.nextInt((int)std::size(kFirst))]) + " " + kSecond[rng.nextInt((int)std::size(kSecond))];
    return s;
}

}  // namespace ignite
