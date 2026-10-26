#pragma once
// A pane of glass in front of the scope. When the bass gets very hot it cracks from an impact point:
// radial fractures that race outward in a few frames, branching, with conchoidal rings near the
// impact, drawn as refracted edges (a dark side and a bright highlight) with faint sheen on the
// shards. The cracks stay while there is signal and fade away only after a quiet spell. Further
// spikes add more impacts.
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace glass {

class Pane {
public:
    // Call once per paint with wall-clock dt, the current bass (0..1) and overall energy (0..1).
    void advance(double dt, float bass, float energy, int w, int h);
    void draw(juce::Graphics& g) const;
    bool active() const { return !impacts_.empty(); }
    void crackNow(int w, int h) { impact(w, h); }      // tests
    void seed(juce::int64 s) { rnd_.setSeed(s); }

private:
    struct Line { std::vector<juce::Point<float>> pts; std::vector<float> widths, shine; float width = 1.f; float delay = 0.f; };   // delay: reveal start, 0..1
    struct Impact { juce::Point<float> at; std::vector<Line> lines; std::vector<juce::Path> shards; float reveal = 0.f; };
    void impact(int w, int h);
    std::vector<Impact> impacts_;
    float strength_ = 0.f;          // overall opacity
    double quiet_ = 0.0;            // seconds since the signal went away
    double cooldown_ = 0.0;         // seconds until another impact may happen
    bool armed_ = true;             // re-arms when the bass drops back
    juce::Random rnd_{0x61a55};
};

}  // namespace glass
