#pragma once
// A pane of glass in front of the scope. It is always there, faintly: a soft specular sheen and an
// edge reflection. When the bass gets very hot it fractures from an impact point: a tessellation of
// shards (radial cracks crossed by conchoidal rings) that each catch the light a little differently,
// crack edges drawn as a dark fracture side with a bright refracted edge, white scatter haze that is
// densest at the impact, micro-fractures around it, and branches racing outward. The fracture layer
// is rendered once into an image and composited each frame. Cracks stay while there is signal and
// fade away only after a quiet spell. Further spikes add more impacts.
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace glass {

class Pane {
public:
    void advance(double dt, float bass, float energy, int w, int h);   // once per paint
    void draw(juce::Graphics& g) const;                                // the pane plus any fractures
    bool active() const { return !impacts_.empty(); }
    void crackNow(int w, int h) { impact(w, h); }
    void seed(juce::int64 s) { rnd_.setSeed(s); }

private:
    struct Impact { juce::Point<float> at; float maxR = 0.f; juce::Image layer; float reveal = 0.f; };
    void impact(int w, int h);
    juce::Image buildLayer(juce::Point<float> at, float maxR, int w, int h);
    std::vector<Impact> impacts_;
    float strength_ = 0.f;
    double quiet_ = 0.0, cooldown_ = 0.0;
    bool armed_ = true;
    int w_ = 0, h_ = 0;
    juce::Random rnd_{0x61a55};
};

}  // namespace glass
