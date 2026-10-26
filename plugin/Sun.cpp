#include "Sun.h"
#include <cmath>

namespace sun {
namespace {
constexpr float kPi = 3.14159265f;

// ---- Noise (same formulas in the shader) ----
inline float fract(float x) { return x - std::floor(x); }
inline float hash3(float x, float y, float z) { return fract(std::sin(x * 127.1f + y * 311.7f + z * 74.7f) * 43758.5453f); }
inline float smooth(float t) { return t * t * (3.f - 2.f * t); }
float noise3(float x, float y, float z) {
    const float ix = std::floor(x), iy = std::floor(y), iz = std::floor(z);
    const float fx = smooth(x - ix), fy = smooth(y - iy), fz = smooth(z - iz);
    auto n = [&](float dx, float dy, float dz) { return hash3(ix + dx, iy + dy, iz + dz); };
    const float x00 = n(0, 0, 0) + (n(1, 0, 0) - n(0, 0, 0)) * fx, x10 = n(0, 1, 0) + (n(1, 1, 0) - n(0, 1, 0)) * fx;
    const float x01 = n(0, 0, 1) + (n(1, 0, 1) - n(0, 0, 1)) * fx, x11 = n(0, 1, 1) + (n(1, 1, 1) - n(0, 1, 1)) * fx;
    const float y0 = x00 + (x10 - x00) * fy, y1 = x01 + (x11 - x01) * fy;
    return y0 + (y1 - y0) * fz;
}
float fbm3(float x, float y, float z, int oct) {
    float a = 0.5f, s = 0.f;
    for (int i = 0; i < oct; ++i) { s += a * noise3(x, y, z); x *= 2.03f; y *= 2.01f; z *= 1.97f; a *= 0.5f; }
    return s;
}
float noise2(float x, float y) { return noise3(x, y, 0.37f); }
float fbm2(float x, float y, int oct) { return fbm3(x, y, 0.37f, oct); }

// Colour for a pixel. Returns additive RGB in 0..~1.5 and alpha for the corona.
struct Rgb { float r, g, b; };
inline Rgb mix(Rgb a, Rgb b, float t) { return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t}; }
inline float clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

// The per-pixel model. px, py in units of the radius, relative to the centre; returns colour and alpha.
// falloff scales the corona distance (so a huge limb sphere still has a tight atmosphere), intensity dims the
// whole thing, aa is the rim softening width in radius units.
void shadePixel(float px, float py, float t, float e, float rot, float falloff, float intensity, float aa,
                const std::vector<Particle>& parts, Rgb& out, float& alpha) {
    const float r2 = px * px + py * py, r = std::sqrt(r2);
    const Rgb deep{0.42f, 0.08f, 0.02f}, orange{1.0f, 0.42f, 0.09f}, hot{1.0f, 0.85f, 0.55f}, coronaCol{1.0f, 0.5f, 0.15f};
    Rgb disc{0, 0, 0}, cor{0, 0, 0}; float corA = 0.f;
    if (r < 1.f + aa) {
        const float z = std::sqrt(std::max(0.f, 1.f - r2));
        const float ca = std::cos(rot), sa = std::sin(rot);
        const float x = px * ca + z * sa, zz = -px * sa + z * ca;
        const float fq = 2.6f + 1.8f * e;
        const float f = fbm3(x * fq + t * 0.05f, py * fq, zz * fq + t * 0.03f, 4);
        const float g = fbm3(x * fq * 3.1f - t * 0.11f * (1.f + 2.f * e), py * fq * 3.1f + t * 0.07f, zz * fq * 3.1f, 3);
        const float limb = std::pow(z, 0.45f);
        float lum = (0.50f + 0.55f * f + 0.35f * (g - 0.5f) * (0.6f + e)) * limb * (0.85f + 0.55f * e);
        lum = clamp01(lum);
        Rgb c = mix(deep, orange, smooth(clamp01(lum * 1.25f)));
        disc = mix(c, hot, clamp01((lum - 0.72f) * 3.5f) * (0.35f + 0.65f * e));
    }
    if (r > 1.f - aa) {
        const float s = std::max(0.f, r - 1.f) * falloff;
        const float ang = std::atan2(py, px);
        const float k = 7.5f - 4.5f * e;
        float glow = std::exp(-s * k) * (0.45f + 0.75f * e);
        const float st = fbm2(ang * 2.5f + 3.f, s * 4.f - t * 0.08f, 3);
        glow *= 0.65f + 0.7f * st;
        // Prominences: looped arcs hugging the limb, taller and more frequent with energy.
        const float hh0 = fbm2(ang * 5.f + t * 0.15f, t * 0.05f, 3);
        const float hh = clamp01((hh0 - 0.45f) * 2.2f);
        const float arcH = 0.03f + 0.26f * e * hh * hh;
        float prom = 0.f;
        if (s < arcH) { const float u = s / arcH; prom = (1.f - u) * (0.5f + 0.5f * std::sin(u * kPi)) * hh * (0.6f + 0.8f * e); }
        const float total = glow + prom;
        cor = {coronaCol.r * total, coronaCol.g * total * (1.f - 0.3f * prom), coronaCol.b * total};
        corA = clamp01(total * 1.2f);
    }
    const float w = smooth(clamp01((r - (1.f - aa)) / (2.f * aa)));   // 0 on the disc, 1 in the corona
    out = mix(disc, cor, w); alpha = 1.f + (corA - 1.f) * w;
    // Particles: additive soft dots.
    const float psz = 1.f / std::sqrt(falloff);
    for (const auto& p : parts) {
        if (p.life <= 0.f) continue;
        const float sz = p.size * psz;
        const float dx = (px - p.x) / sz, dy = (py - p.y) / sz, d2 = dx * dx + dy * dy;
        if (d2 > 9.f) continue;
        const float v = std::exp(-d2 * 1.6f) * p.life * (0.7f + 0.6f * e);
        out.r += v; out.g += v * 0.55f; out.b += v * 0.15f; alpha = clamp01(alpha + v);
    }
    out.r *= intensity; out.g *= intensity; out.b *= intensity;
}
}  // namespace

// Radiation hitting the sensor: fine grain re-rolled every frame, sparse flickering hot pixels, and
// short ray tracks that streak across and fade. Everything scales with energy. Coordinates in
// component pixels; returns an additive brightness.
float radiationAt(float x, float y, float t, float e, float w, float h) {
    const float grainF = std::floor(t * 24.f), hitF = std::floor(t * 12.f), rayF = std::floor(t * 3.f), rayU = t * 3.f - rayF;
    float v = 0.f;
    // grain
    v += (hash3(std::floor(x), std::floor(y), grainF) - 0.5f) * 0.09f * (0.6f + e);
    // hot pixels on a 2 px lattice
    const float cx = std::floor(x * 0.5f), cy = std::floor(y * 0.5f);
    const float hh = hash3(cx, cy, hitF + 0.5f);
    const float density = 0.0012f + 0.010f * e * e;
    if (hh < density) v += 0.25f + 0.6f * hash3(cy, cx, hitF + 7.3f);
    // ray tracks: a few slots, each a segment that lives a third of a second and drifts
    const int slots = 7;
    for (int k = 0; k < slots; ++k) {
        const float sk = (float)k * 13.7f;
        if (hash3(sk, rayF, 1.1f) > 0.12f + 0.55f * e) continue;
        const float x0 = hash3(sk, rayF, 2.2f) * w, y0 = hash3(sk, rayF, 3.3f) * h;
        const float ang = hash3(sk, rayF, 4.4f) * 6.2831853f, len = 6.f + 40.f * hash3(sk, rayF, 5.5f) * (0.5f + e);
        const float drift = 6.f * rayU;
        const float dx = std::cos(ang), dy = std::sin(ang);
        const float ox = x - (x0 + dx * drift), oy = y - (y0 + dy * drift);
        const float along = ox * dx + oy * dy;
        if (along < 0.f || along > len) continue;
        const float perp = std::abs(-ox * dy + oy * dx);
        if (perp > 1.2f) continue;
        v += (1.f - perp / 1.2f) * (1.f - rayU) * (0.5f + 0.5f * hash3(sk, rayF, 6.6f));
    }
    return v;
}

// ---- Model ------------------------------------------------------------------------------------
float SunModel::rnd() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (1.f / 16777216.f); }

void SunModel::advance(double dt, float level) {
    dt = std::min(dt, 0.1);
    const float target = clamp01(level);
    const float coef = target > energy_ ? (float)(1.0 - std::exp(-dt / 0.04)) : (float)(1.0 - std::exp(-dt / 1.6));
    energy_ += (target - energy_) * coef;
    time_ += (float)dt * (1.f + 1.5f * energy_);
    rotation_ += (float)dt * (0.03f + 0.12f * energy_);
    if (particles_.empty()) particles_.assign(kMaxParticles, Particle());
    // Spawn at the rim; rate grows with energy.
    spawnAcc_ += (float)dt * (1.5f + 45.f * energy_ * energy_);
    while (spawnAcc_ >= 1.f) {
        spawnAcc_ -= 1.f;
        for (auto& p : particles_) {
            if (p.life > 0.f) continue;
            const float a = rnd() * 2.f * kPi, sp = (0.25f + 0.9f * rnd()) * (0.4f + energy_);
            p.x = std::cos(a); p.y = std::sin(a);
            p.vx = std::cos(a) * sp + (rnd() - 0.5f) * 0.3f; p.vy = std::sin(a) * sp + (rnd() - 0.5f) * 0.3f;
            p.life = 1.f; p.size = 0.012f + 0.035f * rnd() * (0.5f + energy_);
            break;
        }
    }
    for (auto& p : particles_) {
        if (p.life <= 0.f) continue;
        const float r = std::sqrt(p.x * p.x + p.y * p.y) + 1e-4f;
        p.vx -= p.x / r * (float)dt * 0.9f; p.vy -= p.y / r * (float)dt * 0.9f;       // pulled back toward the surface
        p.x += p.vx * (float)dt; p.y += p.vy * (float)dt;
        p.life -= (float)dt * (0.55f + 0.4f * rnd());
        if (r < 0.98f && p.vx * p.x + p.vy * p.y < 0) p.life = 0.f;                   // fell back in
    }
}

Geometry geometryFor(Mode m, int w, int h) {
    if (m == Mode::Disc) { const float R = 0.40f * std::min(w, h); return {w * 0.5f, h * 0.5f, R, 1.f, 1.f}; }
    const float R = 0.95f * w;                       // a big sphere whose top rises into the view
    return {w * 0.5f, h + R - 0.30f * h, R, R / (0.45f * h), 0.65f, 1.f};   // atmosphere measured against the view height; dimmed so readings stay legible
}

// ---- CPU path -----------------------------------------------------------------------------------
void renderCpu(juce::Image& target, Mode m, const SunModel& model, float scale, int compW, int compH) {
    const int iw = std::max(2, (int)(compW * scale)), ih = std::max(2, (int)(compH * scale));
    if (!target.isValid() || target.getWidth() != iw || target.getHeight() != ih) target = juce::Image(juce::Image::ARGB, iw, ih, true);
    const Geometry geo = geometryFor(m, compW, compH);
    const float t = model.time(), e = model.energy(), rot = model.rotation();
    const auto& parts = model.particles();
    juce::Image::BitmapData bd(target, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < ih; ++y) {
        auto* row = bd.getLinePointer(y);
        for (int x = 0; x < iw; ++x) {
            const float px = ((x + 0.5f) / scale - geo.cx) / geo.radius, py = ((y + 0.5f) / scale - geo.cy) / geo.radius;
            Rgb c; float a;
            shadePixel(px, py, t, e, rot, geo.falloff, geo.intensity, 1.5f / (geo.radius * scale), parts, c, a);
            if (geo.radiation > 0.f) {
                const float v = radiationAt((x + 0.5f) / scale, (y + 0.5f) / scale, t, e, (float)compW, (float)compH) * geo.radiation;
                c.r += v; c.g += v * 0.9f; c.b += v * 0.75f; a = clamp01(a + std::abs(v));
            }
            // Premultiplied ARGB, as JUCE stores it.
            auto* p = (juce::PixelARGB*)(row + x * 4);
            p->setARGB((juce::uint8)(clamp01(a) * 255.f), (juce::uint8)(clamp01(c.r) * 255.f * clamp01(a)),
                       (juce::uint8)(clamp01(c.g) * 255.f * clamp01(a)), (juce::uint8)(clamp01(c.b) * 255.f * clamp01(a)));
        }
    }
}

// ---- Shader (GLSL 1.20 style; JUCE translates for core profiles) ----------------------------------
namespace {
const char* kVertex = R"(
attribute vec2 position;
void main() { gl_Position = vec4(position, 0.0, 1.0); }
)";
const char* kFragment = R"(
uniform vec2 resolution;
uniform vec3 geo;        // cx, cy, radius in pixels (origin top-left)
uniform float time;
uniform float energy;
uniform float falloff;
uniform float intensity;
uniform float aa;
uniform float radiation;
uniform float pixScale;     // component pixels per device pixel
uniform float rotation;
uniform vec4 particles[48];   // x, y (radius units), size, life
uniform int particleCount;
float fract1(float x) { return x - floor(x); }
float hash3(vec3 p) { return fract1(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453); }
vec3 smooth3(vec3 t) { return t * t * (3.0 - 2.0 * t); }
float noise3(vec3 p) {
    vec3 i = floor(p), f = smooth3(p - i);
    float x00 = mix(hash3(i), hash3(i + vec3(1,0,0)), f.x), x10 = mix(hash3(i + vec3(0,1,0)), hash3(i + vec3(1,1,0)), f.x);
    float x01 = mix(hash3(i + vec3(0,0,1)), hash3(i + vec3(1,0,1)), f.x), x11 = mix(hash3(i + vec3(0,1,1)), hash3(i + vec3(1,1,1)), f.x);
    return mix(mix(x00, x10, f.y), mix(x01, x11, f.y), f.z);
}
float fbm3(vec3 p, int oct) { float a = 0.5, s = 0.0; for (int i = 0; i < 4; ++i) { if (i >= oct) break; s += a * noise3(p); p *= vec3(2.03, 2.01, 1.97); a *= 0.5; } return s; }
float fbm2(vec2 p, int oct) { return fbm3(vec3(p, 0.37), oct); }
float sm(float t) { return t * t * (3.0 - 2.0 * t); }
float radiationAt(vec2 p, float t, float e, vec2 res) {
    float grainF = floor(t * 24.0), hitF = floor(t * 12.0), rayF = floor(t * 3.0), rayU = t * 3.0 - rayF;
    float v = (hash3(vec3(floor(p), grainF)) - 0.5) * 0.09 * (0.6 + e);
    vec2 cell = floor(p * 0.5);
    float hh = hash3(vec3(cell, hitF + 0.5));
    float density = 0.0012 + 0.010 * e * e;
    if (hh < density) v += 0.25 + 0.6 * hash3(vec3(cell.y, cell.x, hitF + 7.3));
    for (int k = 0; k < 7; ++k) {
        float sk = float(k) * 13.7;
        if (hash3(vec3(sk, rayF, 1.1)) > 0.12 + 0.55 * e) continue;
        vec2 p0 = vec2(hash3(vec3(sk, rayF, 2.2)) * res.x, hash3(vec3(sk, rayF, 3.3)) * res.y);
        float ang = hash3(vec3(sk, rayF, 4.4)) * 6.2831853, len = 6.0 + 40.0 * hash3(vec3(sk, rayF, 5.5)) * (0.5 + e);
        vec2 d = vec2(cos(ang), sin(ang));
        vec2 o = p - (p0 + d * 6.0 * rayU);
        float along = dot(o, d);
        if (along < 0.0 || along > len) continue;
        float perp = abs(-o.x * d.y + o.y * d.x);
        if (perp > 1.2) continue;
        v += (1.0 - perp / 1.2) * (1.0 - rayU) * (0.5 + 0.5 * hash3(vec3(sk, rayF, 6.6)));
    }
    return v;
}
void main() {
    vec2 pix = vec2(gl_FragCoord.x, resolution.y - gl_FragCoord.y);
    float px = (pix.x - geo.x) / geo.z, py = (pix.y - geo.y) / geo.z;
    float r2 = px * px + py * py, r = sqrt(r2);
    vec3 deep = vec3(0.42, 0.08, 0.02), orange = vec3(1.0, 0.42, 0.09), hot = vec3(1.0, 0.85, 0.55), coronaCol = vec3(1.0, 0.5, 0.15);
    float e = energy; float t = time;
    vec3 disc = vec3(0.0), cor = vec3(0.0); float corA = 0.0;
    if (r < 1.0 + aa) {
        float z = sqrt(max(0.0, 1.0 - r2));
        float ca = cos(rotation), sa = sin(rotation);
        float x = px * ca + z * sa, zz = -px * sa + z * ca;
        float fq = 2.6 + 1.8 * e;
        float f = fbm3(vec3(x * fq + t * 0.05, py * fq, zz * fq + t * 0.03), 4);
        float g = fbm3(vec3(x * fq * 3.1 - t * 0.11 * (1.0 + 2.0 * e), py * fq * 3.1 + t * 0.07, zz * fq * 3.1), 3);
        float limb = pow(z, 0.45);
        float lum = clamp((0.50 + 0.55 * f + 0.35 * (g - 0.5) * (0.6 + e)) * limb * (0.85 + 0.55 * e), 0.0, 1.0);
        vec3 c = mix(deep, orange, sm(clamp(lum * 1.25, 0.0, 1.0)));
        disc = mix(c, hot, clamp((lum - 0.72) * 3.5, 0.0, 1.0) * (0.35 + 0.65 * e));
    }
    if (r > 1.0 - aa) {
        float s = max(0.0, r - 1.0) * falloff, ang = atan(py, px);
        float k = 7.5 - 4.5 * e;
        float glow = exp(-s * k) * (0.45 + 0.75 * e);
        float st = fbm2(vec2(ang * 2.5 + 3.0, s * 4.0 - t * 0.08), 3);
        glow *= 0.65 + 0.7 * st;
        float h = fbm2(vec2(ang * 5.0 + t * 0.15, t * 0.05), 3);
        float hh = clamp((h - 0.45) * 2.2, 0.0, 1.0);
        float arcH = 0.03 + 0.26 * e * hh * hh;
        float prom = 0.0;
        if (s < arcH) { float u = s / arcH; prom = (1.0 - u) * (0.5 + 0.5 * sin(u * 3.14159265)) * hh * (0.6 + 0.8 * e); }
        float total = glow + prom;
        cor = vec3(coronaCol.r * total, coronaCol.g * total * (1.0 - 0.3 * prom), coronaCol.b * total);
        corA = clamp(total * 1.2, 0.0, 1.0);
    }
    float w = sm(clamp((r - (1.0 - aa)) / (2.0 * aa), 0.0, 1.0));
    vec3 c = mix(disc, cor, w); float alpha = 1.0 + (corA - 1.0) * w;
    float psz = 1.0 / sqrt(falloff);
    for (int i = 0; i < 48; ++i) {
        if (i >= particleCount) break;
        vec4 p = particles[i];
        if (p.w <= 0.0) continue;
        float sz = p.z * psz;
        float dx = (px - p.x) / sz, dy = (py - p.y) / sz, d2 = dx * dx + dy * dy;
        if (d2 > 9.0) continue;
        float v = exp(-d2 * 1.6) * p.w * (0.7 + 0.6 * e);
        c += vec3(v, v * 0.55, v * 0.15); alpha = clamp(alpha + v, 0.0, 1.0);
    }
    c *= intensity;
    if (radiation > 0.0) { float v = radiationAt(pix * pixScale, t, e, resolution * pixScale) * radiation; c += vec3(v, v * 0.9, v * 0.75); alpha = clamp(alpha + abs(v), 0.0, 1.0); }
    gl_FragColor = vec4(clamp(c, 0.0, 1.0) * alpha, alpha);   // premultiplied, blended over the clear colour
}
)";
}  // namespace

// ---- View ----------------------------------------------------------------------------------------
SunView::SunView(Mode mode, std::function<float()> level, bool allowOpenGL) : mode_(mode), level_(std::move(level)), allowGl_(allowOpenGL) {
    setOpaque(false);
    setInterceptsMouseClicks(false, false);
    model_.seed(mode == Mode::Disc ? 7u : 99u);
    lastTick_ = juce::Time::getMillisecondCounterHiRes() * 0.001;
    if (allowGl_) {
        gl_.setRenderer(this);
        gl_.setComponentPaintingEnabled(true);
        gl_.setContinuousRepainting(true);
        gl_.setMultisamplingEnabled(false);
        gl_.attachTo(*this);
    } else {
        startTimerHz(24);
    }
}

SunView::~SunView() { stopTimer(); gl_.detach(); }

void SunView::forceCpuPath() {
    if (allowGl_) { gl_.detach(); allowGl_ = false; }
    glActive_ = false;
    if (!isTimerRunning()) startTimerHz(24);
}

void SunView::renderNow() { tick(); renderCpu(cpuImage_, mode_, model_, cpuScale_, getWidth(), getHeight()); }

void SunView::tick() {
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const double dt = std::clamp(now - lastTick_, 0.0, 0.1);
    lastTick_ = now;
    model_.advance(dt, level_ ? level_() : 0.f);
}

void SunView::timerCallback() {
    if (glActive_) return;
    renderNow();
    repaint();
}

void SunView::resized() { if (!glActive_) cpuImage_ = juce::Image(); }

void SunView::paint(juce::Graphics& g) {
    if (!glActive_ && cpuImage_.isValid()) {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImageTransformed(cpuImage_, juce::AffineTransform::scale((float)getWidth() / cpuImage_.getWidth(), (float)getHeight() / cpuImage_.getHeight()), false);
    }
    if (overlay) overlay(g);
}

void SunView::newOpenGLContextCreated() {
    shader_ = std::make_unique<juce::OpenGLShaderProgram>(gl_);
    shaderOk_ = shader_->addVertexShader(juce::OpenGLHelpers::translateVertexShaderToV3(kVertex))
             && shader_->addFragmentShader(juce::OpenGLHelpers::translateFragmentShaderToV3(kFragment))
             && shader_->link();
    glActive_ = shaderOk_;
    if (!shaderOk_) { juce::MessageManager::callAsync([sp = juce::Component::SafePointer<SunView>(this)] { if (sp) sp->forceCpuPath(); }); }
}

void SunView::openGLContextClosing() { shader_.reset(); glActive_ = false; }

void SunView::renderOpenGL() {
    using namespace juce::gl;
    const float scale = (float)gl_.getRenderingScale();
    const int w = juce::roundToInt(getWidth() * scale), h = juce::roundToInt(getHeight() * scale);
    glViewport(0, 0, w, h);
    juce::OpenGLHelpers::clear(juce::Colour(0xff0c0b0d));
    if (background) {   // the native GL surface hides what the parent painted, so paint it again here
        std::unique_ptr<juce::LowLevelGraphicsContext> ctx(juce::createOpenGLGraphicsContext(gl_, w, h));
        juce::Graphics g(*ctx);
        g.addTransform(juce::AffineTransform::scale(scale));
        background(g);
    }
    if (!shaderOk_) return;
    tick();
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    shader_->use();
    const Geometry geo = geometryFor(mode_, getWidth(), getHeight());
    if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, "resolution"); u.uniformID >= 0) u.set((float)w, (float)h);
    if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, "geo"); u.uniformID >= 0) u.set(geo.cx * scale, geo.cy * scale, geo.radius * scale);
    if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, "time"); u.uniformID >= 0) u.set(model_.time());
    if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, "energy"); u.uniformID >= 0) u.set(model_.energy());
    if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, "falloff"); u.uniformID >= 0) u.set(geo.falloff);
    if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, "intensity"); u.uniformID >= 0) u.set(geo.intensity);
    if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, "aa"); u.uniformID >= 0) u.set(1.5f / (geo.radius * scale));
    if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, "radiation"); u.uniformID >= 0) u.set(geo.radiation);
    if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, "pixScale"); u.uniformID >= 0) u.set(1.f / scale);
    if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, "rotation"); u.uniformID >= 0) u.set(model_.rotation());
    float parts[SunModel::kMaxParticles * 4]; int n = 0;
    for (const auto& p : model_.particles()) { if (n >= SunModel::kMaxParticles) break; parts[n * 4] = p.x; parts[n * 4 + 1] = p.y; parts[n * 4 + 2] = p.size; parts[n * 4 + 3] = p.life; ++n; }
    if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, "particles"); u.uniformID >= 0) u.set(parts, n);
    if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, "particleCount"); u.uniformID >= 0) u.set(n);
    // Full-screen quad.
    GLuint vbo = 0;
    const float quad[8] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};
    glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STREAM_DRAW);
    juce::OpenGLShaderProgram::Attribute pos(*shader_, "position");
    glVertexAttribPointer((GLuint)pos.attributeID, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray((GLuint)pos.attributeID);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray((GLuint)pos.attributeID);
    glBindBuffer(GL_ARRAY_BUFFER, 0); glDeleteBuffers(1, &vbo);
}

}  // namespace sun
