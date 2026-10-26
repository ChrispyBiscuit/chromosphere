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
                const SunModel& m, Rgb& out, float& alpha) {
    const float r2 = px * px + py * py, r = std::sqrt(r2);
    const Rgb deep{0.42f, 0.08f, 0.02f}, orange{1.0f, 0.42f, 0.09f}, hot{1.0f, 0.85f, 0.55f}, coronaCol{1.0f, 0.5f, 0.15f};
    Rgb disc{0, 0, 0}, cor{0, 0, 0}; float corA = 0.f;
    const float ca = std::cos(rot), sa = std::sin(rot);
    const float psz = 1.f / std::sqrt(falloff);      // feature scale: a huge limb sphere keeps its features at screen size
    if (r < 1.f + aa) {
        const float z = std::sqrt(std::max(0.f, 1.f - r2));
        const float x = px * ca + z * sa, zz = -px * sa + z * ca;
        const float fq = 2.6f + 1.8f * e;
        const float f = fbm3(x * fq + t * 0.05f, py * fq, zz * fq + t * 0.03f, 3);
        const float g = fbm3(x * fq * 3.1f - t * 0.11f * (1.f + 2.f * e), py * fq * 3.1f + t * 0.07f, zz * fq * 3.1f, 2);
        const float limb = std::pow(z, 0.45f);
        float lum = (0.50f + 0.55f * f + 0.35f * (g - 0.5f) * (0.6f + e)) * limb * (0.85f + 0.55f * e);
        // Sunspots: dark umbra with a softer penumbra, fixed to the rotating surface.
        for (const auto& sp : m.spots()) {
            if (sp.life <= 0.f) continue;
            const float ddx = x - sp.dx, ddy = py - sp.dy, ddz = zz - sp.dz;
            const float dist = std::sqrt(ddx * ddx + ddy * ddy + ddz * ddz), sz = sp.size * psz * (0.5f + 0.5f * sp.life);
            if (dist > sz * 2.f) continue;
            const float umbra = 1.f - smooth(clamp01((dist - sz * 0.45f) / (sz * 0.35f)));
            const float pen = 1.f - smooth(clamp01((dist - sz * 0.8f) / (sz * 1.2f)));
            lum *= (1.f - 0.9f * umbra * sp.life) * (1.f - 0.45f * pen * (1.f - umbra) * sp.life);
        }
        lum = clamp01(lum);
        Rgb c = mix(deep, orange, smooth(clamp01(lum * 1.25f)));
        disc = mix(c, hot, clamp01((lum - 0.72f) * 3.5f) * (0.35f + 0.65f * e));
        // Flares: brief white-hot eruptions on the surface.
        for (const auto& f : m.flares()) {
            if (f.life <= 0.f) continue;
            const float ddx = x - f.dx, ddy = py - f.dy, ddz = zz - f.dz;
            const float d2 = ddx * ddx + ddy * ddy + ddz * ddz;
            const float v = std::exp(-d2 / (0.012f * psz * psz)) * std::pow(f.life, 0.7f) * 1.3f;
            disc.r += v; disc.g += v * 0.9f; disc.b += v * 0.7f;
        }
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
        // Coronal mass ejections: a bright front moving out with a fading body behind it.
        float cme = 0.f;
        for (const auto& c : m.cmes()) {
            if (c.progress > 1.f) continue;
            float da = ang - c.angle; da = da - 2.f * kPi * std::floor((da + kPi) / (2.f * kPi));
            const float wa = c.width * (1.f + 1.5f * c.progress) * psz;
            const float a = std::exp(-(da * da) / (wa * wa));
            if (a < 0.01f) continue;
            const float front = 0.05f + c.progress * 1.4f, thick = 0.12f + 0.25f * c.progress;
            const float shell = std::exp(-((s - front) * (s - front)) / (thick * thick));
            const float body = s < front ? 0.4f * (1.f - s / front) : 0.f;
            const float tex = 0.7f + 0.6f * fbm2(ang * 6.f + 1.f, s * 5.f - t * 0.4f, 3);
            cme += (shell + body) * a * tex * c.strength * std::pow(1.f - c.progress, 1.2f) * (0.6f + 0.8f * e);
        }
        const float total = glow + prom + cme;
        cor = {coronaCol.r * total + 0.25f * cme, coronaCol.g * total * (1.f - 0.3f * prom) + 0.2f * cme, coronaCol.b * total + 0.1f * cme};
        corA = clamp01(total * 1.2f);
    }
    // Flare glow spills past the limb: screen-space halo around each visible flare.
    for (const auto& f : m.flares()) {
        if (f.life <= 0.f) continue;
        const float z = f.dx * sa + f.dz * ca;           // depth toward the viewer
        if (z < -0.05f) continue;
        const float sx = f.dx * ca - f.dz * sa, sy = f.dy;
        const float ddx = px - sx, ddy = py - sy, d2 = ddx * ddx + ddy * ddy;
        const float v = std::exp(-d2 / (0.05f * psz * psz)) * f.life * f.life * 0.5f * (z < 0.2f ? 1.f : 0.35f);
        if (r > 1.f - aa) { cor.r += v; cor.g += v * 0.75f; cor.b += v * 0.4f; corA = clamp01(corA + v); }
    }
    const float w = smooth(clamp01((r - (1.f - aa)) / (2.f * aa)));   // 0 on the disc, 1 in the corona
    out = mix(disc, cor, w); alpha = 1.f + (corA - 1.f) * w;
    // Particles: additive soft dots.
    for (const auto& p : m.particles()) {
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
    // hot pixels: single pixels
    const float cx = std::floor(x), cy = std::floor(y);
    const float hh = hash3(cx, cy, hitF + 0.5f);
    const float density = 0.0004f + 0.003f * e * e;
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
        if (perp > 0.7f) continue;
        v += (1.f - perp / 0.7f) * (1.f - rayU) * (0.5f + 0.5f * hash3(sk, rayF, 6.6f));
    }
    return v;
}

// ---- Model ------------------------------------------------------------------------------------
float SunModel::rnd() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (1.f / 16777216.f); }

void SunModel::advance(double dt, Drive d) {
    dt = std::min(dt, 0.1);
    const float fdt = (float)dt;
    const float target = clamp01(d.level);
    const float coef = target > energy_ ? (float)(1.0 - std::exp(-dt / 0.04)) : (float)(1.0 - std::exp(-dt / 1.6));
    energy_ += (target - energy_) * coef;
    const float btarget = clamp01(d.bass);
    const float bcoef = btarget > bass_ ? (float)(1.0 - std::exp(-dt / 0.12)) : (float)(1.0 - std::exp(-dt / 2.5));
    bass_ += (btarget - bass_) * bcoef;
    time_ += fdt * (1.f + 1.5f * energy_);
    rotation_ += fdt * (0.03f + 0.12f * energy_);
    if (particles_.empty()) { particles_.assign(kMaxParticles, Particle()); spots_.assign(kMaxSpots, Spot()); flares_.assign(kMaxFlares, Flare()); cmes_.assign(kMaxCmes, Cme()); }

    // ---- Particles: spawn at the rim; rate grows with energy. ----
    spawnAcc_ += fdt * (1.5f + 45.f * energy_ * energy_);
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
        p.vx -= p.x / r * fdt * 0.9f; p.vy -= p.y / r * fdt * 0.9f;       // pulled back toward the surface
        p.x += p.vx * fdt; p.y += p.vy * fdt;
        p.life -= fdt * (0.55f + 0.4f * rnd());
        if (r < 0.98f && p.vx * p.x + p.vy * p.y < 0) p.life = 0.f;      // fell back in
    }

    // ---- Sunspots: bass brings them out in the active latitudes; they grow in, hold, fade. ----
    spotAcc_ += fdt * (0.04f + 2.2f * bass_ * std::sqrt(bass_));
    while (spotAcc_ >= 1.f) {
        spotAcc_ -= 1.f;
        for (auto& sp : spots_) {
            if (sp.life > 0.f || sp.age > 0.f) continue;
            const float lat = (rnd() - 0.5f) * 1.2f, lon = rnd() * 2.f * kPi;   // within about ±35 degrees
            sp.dx = std::cos(lat) * std::sin(lon); sp.dy = std::sin(lat); sp.dz = std::cos(lat) * std::cos(lon);
            sp.size = (0.07f + 0.10f * rnd()) * (0.6f + 0.6f * bass_);
            sp.span = 5.f + 9.f * rnd(); sp.age = 1e-3f; sp.life = 0.f;
            break;
        }
    }
    for (auto& sp : spots_) {
        if (sp.age <= 0.f) continue;
        sp.age += fdt;
        const float grow = std::min(1.f, sp.age / 1.2f);
        const float fade = clamp01((sp.span - sp.age) / 2.5f);
        sp.life = grow * fade;
        if (sp.age >= sp.span) { sp.age = 0.f; sp.life = 0.f; }
    }

    // ---- Flares: brief bright eruptions, mostly from spots, more with level. ----
    flareAcc_ += fdt * (0.03f + 2.5f * energy_ * energy_);
    while (flareAcc_ >= 1.f) {
        flareAcc_ -= 1.f;
        for (auto& f : flares_) {
            if (f.life > 0.f) continue;
            const Spot* host = nullptr; int live = 0;
            for (const auto& sp : spots_) if (sp.life > 0.3f && rnd() < 1.f / (float)(++live)) host = &sp;
            float x, y, z;
            if (host) { x = host->dx + (rnd() - 0.5f) * 0.25f; y = host->dy + (rnd() - 0.5f) * 0.25f; z = host->dz + (rnd() - 0.5f) * 0.25f; }
            else { const float lat = (rnd() - 0.5f) * 1.6f, lon = rnd() * 2.f * kPi; x = std::cos(lat) * std::sin(lon); y = std::sin(lat); z = std::cos(lat) * std::cos(lon); }
            const float n = std::sqrt(x * x + y * y + z * z) + 1e-6f;
            f.dx = x / n; f.dy = y / n; f.dz = z / n; f.life = 1.f;
            break;
        }
    }
    for (auto& f : flares_) if (f.life > 0.f) f.life -= fdt * (2.2f + 1.5f * rnd());

    // ---- CMEs: lobes thrown from the limb, more and faster with level. ----
    cmeAcc_ += fdt * (0.015f + 1.1f * energy_ * energy_);
    while (cmeAcc_ >= 1.f) {
        cmeAcc_ -= 1.f;
        for (auto& c : cmes_) {
            if (c.progress <= 1.f) continue;
            c.angle = rnd() * 2.f * kPi; c.progress = 0.f;
            c.width = 0.18f + 0.3f * rnd(); c.strength = 0.6f + 0.6f * rnd() * (0.5f + energy_);
            // the eruption lights the limb where it leaves
            for (auto& f : flares_) if (f.life <= 0.f) {
                const float ca = std::cos(rotation_), sa = std::sin(rotation_);
                const float px = std::cos(c.angle), py = std::sin(c.angle);
                f.dx = px * ca; f.dy = py; f.dz = -px * sa; f.life = 1.f; break;
            }
            break;
        }
    }
    for (auto& c : cmes_) if (c.progress <= 1.f) c.progress += fdt * (0.3f + 0.35f * energy_);
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
    juce::Image::BitmapData bd(target, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < ih; ++y) {
        auto* row = bd.getLinePointer(y);
        for (int x = 0; x < iw; ++x) {
            const float px = ((x + 0.5f) / scale - geo.cx) / geo.radius, py = ((y + 0.5f) / scale - geo.cy) / geo.radius;
            Rgb c; float a;
            shadePixel(px, py, t, e, rot, geo.falloff, geo.intensity, 1.5f / (geo.radius * scale), model, c, a);
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
uniform vec4 spots[12];       // direction (sphere frame), size; life packed in spotLife
uniform float spotLife[12];
uniform int spotCount;
uniform vec4 flares[6];       // direction, life
uniform int flareCount;
uniform vec4 cmes[6];         // angle, progress, width, strength
uniform int cmeCount;
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
    vec2 cell = floor(p);
    float hh = hash3(vec3(cell, hitF + 0.5));
    float density = 0.0004 + 0.003 * e * e;
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
        if (perp > 0.7) continue;
        v += (1.0 - perp / 0.7) * (1.0 - rayU) * (0.5 + 0.5 * hash3(vec3(sk, rayF, 6.6)));
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
    float ca = cos(rotation), sa = sin(rotation);
    float psz = 1.0 / sqrt(falloff);
    if (r < 1.0 + aa) {
        float z = sqrt(max(0.0, 1.0 - r2));
        float x = px * ca + z * sa, zz = -px * sa + z * ca;
        float fq = 2.6 + 1.8 * e;
        float f = fbm3(vec3(x * fq + t * 0.05, py * fq, zz * fq + t * 0.03), 4);
        float g = fbm3(vec3(x * fq * 3.1 - t * 0.11 * (1.0 + 2.0 * e), py * fq * 3.1 + t * 0.07, zz * fq * 3.1), 3);
        float limb = pow(z, 0.45);
        float lum = (0.50 + 0.55 * f + 0.35 * (g - 0.5) * (0.6 + e)) * limb * (0.85 + 0.55 * e);
        vec3 sp3 = vec3(x, py, zz);
        for (int i = 0; i < 12; ++i) {
            if (i >= spotCount) break;
            float life = spotLife[i]; if (life <= 0.0) continue;
            float dist = length(sp3 - spots[i].xyz), sz = spots[i].w * psz * (0.5 + 0.5 * life);
            if (dist > sz * 2.0) continue;
            float umbra = 1.0 - sm(clamp((dist - sz * 0.45) / (sz * 0.35), 0.0, 1.0));
            float pen = 1.0 - sm(clamp((dist - sz * 0.8) / (sz * 1.2), 0.0, 1.0));
            lum *= (1.0 - 0.9 * umbra * life) * (1.0 - 0.45 * pen * (1.0 - umbra) * life);
        }
        lum = clamp(lum, 0.0, 1.0);
        vec3 c = mix(deep, orange, sm(clamp(lum * 1.25, 0.0, 1.0)));
        disc = mix(c, hot, clamp((lum - 0.72) * 3.5, 0.0, 1.0) * (0.35 + 0.65 * e));
        for (int i = 0; i < 6; ++i) {
            if (i >= flareCount) break;
            float life = flares[i].w; if (life <= 0.0) continue;
            vec3 dd = sp3 - flares[i].xyz;
            float v = exp(-dot(dd, dd) / (0.012 * psz * psz)) * pow(life, 0.7) * 1.3;
            disc += vec3(v, v * 0.9, v * 0.7);
        }
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
        float cme = 0.0;
        for (int i = 0; i < 6; ++i) {
            if (i >= cmeCount) break;
            vec4 c4 = cmes[i]; if (c4.y > 1.0) continue;
            float da = ang - c4.x; da = da - 6.2831853 * floor((da + 3.14159265) / 6.2831853);
            float wa = c4.z * (1.0 + 1.5 * c4.y) * psz;
            float a = exp(-(da * da) / (wa * wa));
            if (a < 0.01) continue;
            float front = 0.05 + c4.y * 1.4, thick = 0.12 + 0.25 * c4.y;
            float shell = exp(-((s - front) * (s - front)) / (thick * thick));
            float body = s < front ? 0.4 * (1.0 - s / front) : 0.0;
            float tex = 0.7 + 0.6 * fbm2(vec2(ang * 6.0 + 1.0, s * 5.0 - t * 0.4), 3);
            cme += (shell + body) * a * tex * c4.w * pow(1.0 - c4.y, 1.2) * (0.6 + 0.8 * e);
        }
        float total = glow + prom + cme;
        cor = vec3(coronaCol.r * total + 0.25 * cme, coronaCol.g * total * (1.0 - 0.3 * prom) + 0.2 * cme, coronaCol.b * total + 0.1 * cme);
        corA = clamp(total * 1.2, 0.0, 1.0);
        for (int i = 0; i < 6; ++i) {
            if (i >= flareCount) break;
            vec4 f = flares[i]; if (f.w <= 0.0) continue;
            float z = f.x * sa + f.z * ca; if (z < -0.05) continue;
            vec2 dd = vec2(px, py) - vec2(f.x * ca - f.z * sa, f.y);
            float v = exp(-dot(dd, dd) / (0.05 * psz * psz)) * f.w * f.w * 0.5 * (z < 0.2 ? 1.0 : 0.35);
            cor += vec3(v, v * 0.75, v * 0.4); corA = clamp(corA + v, 0.0, 1.0);
        }
    }
    float w = sm(clamp((r - (1.0 - aa)) / (2.0 * aa), 0.0, 1.0));
    vec3 c = mix(disc, cor, w); float alpha = 1.0 + (corA - 1.0) * w;
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
SunView::SunView(Mode mode, std::function<Drive()> drive, bool allowOpenGL)
    : juce::Thread("Chromosphere sun"), mode_(mode), drive_(std::move(drive)), allowGl_(allowOpenGL) {
    setOpaque(false);
    setInterceptsMouseClicks(false, false);
    model_.seed(mode == Mode::Disc ? 7u : 99u);
    cpuScale_ = mode == Mode::Disc ? 0.75f : 0.35f;   // the scope is wide; a third of its resolution reads fine once upscaled
    lastTick_ = juce::Time::getMillisecondCounterHiRes() * 0.001;
    if (allowGl_) {
        gl_.setRenderer(this);
        // Component painting (the overlay drawn into the GL frame) needs the message-manager lock each
        // frame, which a DAW's UI also needs: only the scope takes it, the emblem has nothing to overlay.
        gl_.setComponentPaintingEnabled(mode == Mode::Limb);
        // No continuous repainting and no vsync wait: a timer asks for frames at a fixed 30 fps so
        // the GL thread never stalls the host's UI.
        gl_.setContinuousRepainting(false);
        gl_.setSwapInterval(0);
        gl_.setMultisamplingEnabled(false);
        gl_.attachTo(*this);
        startTimerHz(30);
    } else {
        // Default path: frames are rendered on this view's own low-priority thread and only copied on
        // the message thread, so the editor costs the host's UI nothing but a blit.
        startThread(juce::Thread::Priority::low);
        startTimerHz(24);
    }
}

SunView::~SunView() { stopTimer(); stopThread(3000); gl_.detach(); }

void SunView::forceCpuPath() {
    if (allowGl_) { gl_.detach(); allowGl_ = false; }
    glActive_ = false;
    stopThread(3000);
    syncCpu_ = true;
    if (!isTimerRunning()) startTimerHz(24);
}

void SunView::renderNow() {
    tick();
    renderCpu(cpuImage_, mode_, model_, cpuScale_, getWidth(), getHeight());
    pubEnergy_.store(model_.energy(), std::memory_order_relaxed); pubBass_.store(model_.bass(), std::memory_order_relaxed);
}

void SunView::tick() {
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const double dt = std::clamp(now - lastTick_, 0.0, 0.1);
    lastTick_ = now;
    model_.advance(dt, drive_ ? drive_() : Drive{});
}

void SunView::run() {
    const int periodMs = mode_ == Mode::Disc ? 42 : 50;    // 24 / 20 fps
    while (!threadShouldExit()) {
        const int w = compW_.load(), h = compH_.load();
        if (w > 1 && h > 1) {
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            tick();
            juce::Image frame;                                // a fresh image each frame: nothing shared with the painter
            renderCpu(frame, mode_, model_, cpuScale_, w, h);
            pubEnergy_.store(model_.energy(), std::memory_order_relaxed); pubBass_.store(model_.bass(), std::memory_order_relaxed);
            { const juce::ScopedLock sl(frameLock_); latest_ = frame; }
            const int spent = (int)(juce::Time::getMillisecondCounterHiRes() - t0);
            wait(std::max(5, periodMs - spent));
        } else wait(100);
    }
}

void SunView::timerCallback() {
    if (glActive_) { gl_.triggerRepaint(); return; }
    if (syncCpu_) { renderNow(); repaint(); return; }
    juce::Image f;
    { const juce::ScopedLock sl(frameLock_); f = latest_; }
    if (f.isValid() && f != cpuImage_) { cpuImage_ = f; repaint(); }
}

void SunView::resized() {
    compW_.store(getWidth()); compH_.store(getHeight());
    if (syncCpu_) cpuImage_ = juce::Image();
}

void SunView::paint(juce::Graphics& g) {
    if (!glActive_ && cpuImage_.isValid()) {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImageTransformed(cpuImage_, juce::AffineTransform::scale((float)getWidth() / cpuImage_.getWidth(), (float)getHeight() / cpuImage_.getHeight()), false);
    }
    if (overlay) overlay(g);
}

void SunView::newOpenGLContextCreated() {
    shader_ = std::make_unique<juce::OpenGLShaderProgram>(gl_);
    gl_.setSwapInterval(0);
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
    pubEnergy_.store(model_.energy(), std::memory_order_relaxed); pubBass_.store(model_.bass(), std::memory_order_relaxed);
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
    auto setVec4s = [&](const char* name, const float* v, int count) { if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, name); u.uniformID >= 0 && count > 0) glUniform4fv(u.uniformID, count, v); };
    auto setInt = [&](const char* name, int v) { if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, name); u.uniformID >= 0) u.set(v); };
    {
        float parts[SunModel::kMaxParticles * 4]; int n = 0;
        for (const auto& p : model_.particles()) { if (n >= SunModel::kMaxParticles) break; parts[n * 4] = p.x; parts[n * 4 + 1] = p.y; parts[n * 4 + 2] = p.size; parts[n * 4 + 3] = p.life; ++n; }
        setVec4s("particles", parts, n); setInt("particleCount", n);
    }
    {
        float sp[SunModel::kMaxSpots * 4], life[SunModel::kMaxSpots]; int n = 0;
        for (const auto& x : model_.spots()) { if (n >= SunModel::kMaxSpots) break; sp[n * 4] = x.dx; sp[n * 4 + 1] = x.dy; sp[n * 4 + 2] = x.dz; sp[n * 4 + 3] = x.size; life[n] = x.life; ++n; }
        setVec4s("spots", sp, n); setInt("spotCount", n);
        if (auto u = juce::OpenGLShaderProgram::Uniform(*shader_, "spotLife"); u.uniformID >= 0 && n > 0) u.set(life, n);
    }
    {
        float fl[SunModel::kMaxFlares * 4]; int n = 0;
        for (const auto& x : model_.flares()) { if (n >= SunModel::kMaxFlares) break; fl[n * 4] = x.dx; fl[n * 4 + 1] = x.dy; fl[n * 4 + 2] = x.dz; fl[n * 4 + 3] = x.life; ++n; }
        setVec4s("flares", fl, n); setInt("flareCount", n);
    }
    {
        float cm[SunModel::kMaxCmes * 4]; int n = 0;
        for (const auto& x : model_.cmes()) { if (n >= SunModel::kMaxCmes) break; cm[n * 4] = x.angle; cm[n * 4 + 1] = x.progress; cm[n * 4 + 2] = x.width; cm[n * 4 + 3] = x.strength; ++n; }
        setVec4s("cmes", cm, n); setInt("cmeCount", n);
    }
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
