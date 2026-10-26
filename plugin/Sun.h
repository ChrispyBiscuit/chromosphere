#pragma once
// A live sun: a shaded, rotating sphere with granulation, limb darkening, a corona, prominences
// and ejected particles. Its energy follows the reverb level: louder means brighter, faster, more
// violent; it calms as the tail decays. Rendered on the GPU (OpenGL fragment shader) when a context
// can be made, otherwise on the CPU at reduced resolution. Both paths use the same model
// (SunModel), so the look is the same and the CPU path can be checked headless.
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include "Glass.h"
#include <functional>
#include <vector>

namespace sun {

enum class Mode { Disc, Limb };   // full sphere (emblem) or the top of a large sphere rising from below (scope)

struct Particle { float x = 0, y = 0, vx = 0, vy = 0, life = 0, size = 0; };
// A sunspot: a direction on the sphere (in the sphere's own frame, so it rotates with the surface),
// a size and a life that grows in, holds and fades. Bass brings them out.
struct Spot { float dx = 0, dy = 0, dz = 1, size = 0, life = 0, age = 0, span = 1; };
// A flare: a brief bright eruption at a point on the surface. Level brings them.
struct Flare { float dx = 0, dy = 0, dz = 1, life = 0; };
// A coronal mass ejection: a lobe thrown out from the limb at a screen angle, expanding outward.
struct Cme { float angle = 0, progress = 2.f, width = 0.3f, strength = 1.f; };

// What drives the sun each frame: overall reverb level and its bass content, both 0..1.
struct Drive { float level = 0.f, bass = 0.f; };

// Time, energy and the state of everything happening on the sun. advance() runs once per frame
// on the message thread.
class SunModel {
public:
    static constexpr int kMaxParticles = 48, kMaxSpots = 12, kMaxFlares = 6, kMaxCmes = 6;
    void advance(double dt, Drive d);
    float energy() const { return energy_; }        // smoothed 0..1 level
    float bass() const { return bass_; }            // smoothed 0..1 bass
    float time() const { return time_; }
    float rotation() const { return rotation_; }
    const std::vector<Particle>& particles() const { return particles_; }
    const std::vector<Spot>& spots() const { return spots_; }
    const std::vector<Flare>& flares() const { return flares_; }
    const std::vector<Cme>& cmes() const { return cmes_; }
    void seed(unsigned s) { rng_ = s; }
private:
    float rnd();
    float energy_ = 0.f, bass_ = 0.f, time_ = 0.f, rotation_ = 0.f;
    float spawnAcc_ = 0.f, spotAcc_ = 0.f, flareAcc_ = 0.f, cmeAcc_ = 0.f;
    std::vector<Particle> particles_;
    std::vector<Spot> spots_;
    std::vector<Flare> flares_;
    std::vector<Cme> cmes_;
    unsigned rng_ = 12345;
};

// Geometry: centre and radius in pixels for a given component size and mode.
struct Geometry { float cx, cy, radius; float falloff = 1.f, intensity = 1.f, radiation = 0.f; };   // radiation: sensor-noise layer strength (0 = none)   // falloff: corona distance unit per radius; intensity: overall brightness
Geometry geometryFor(Mode m, int w, int h);

// CPU render of one frame into an ARGB image (the shader does the same per pixel).
void renderCpu(juce::Image& target, Mode m, const SunModel& model, float scale /* image px per component px */, int compW, int compH);

// Component that shows the sun. Level is read through the callback each frame.
class SunView : public juce::Component, private juce::OpenGLRenderer, private juce::Timer {
public:
    SunView(Mode mode, std::function<Drive()> drive, bool allowOpenGL = true);
    ~SunView() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    // Called from paint() after the sun, for the overlay (the scope draws its graphs here).
    std::function<void(juce::Graphics&)> overlay;
    bool usingOpenGL() const { return glActive_; }
    const SunModel& model() const { return model_; }
    // What lies beneath the view (texture, plate). The GL surface is an opaque native window, so on that
    // path the view paints this itself before the sun; the CPU path leaves it to the parent.
    std::function<void(juce::Graphics&)> background;
    void setGlass(bool on) { glassOn_ = on; }      // a pane in front that cracks on hot bass (scope)
    glass::Pane& glass() { return glass_; }
    void forceCpuPath();                           // tests and headless rendering
    void renderNow();                              // CPU path: render a frame immediately

private:
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;
    void timerCallback() override;
    void tick();

    Mode mode_;
    std::function<Drive()> drive_;
    SunModel model_;
    bool allowGl_, glActive_ = false, shaderOk_ = false;
    juce::OpenGLContext gl_;
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;
    juce::Image cpuImage_;
    double lastTick_ = 0;
    float cpuScale_ = 0.5f;
    bool glassOn_ = false;
    glass::Pane glass_;
    double glassClock_ = 0;
};

}  // namespace sun
