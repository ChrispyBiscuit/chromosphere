#include "Glass.h"
#include <cmath>

namespace glass {
namespace {
constexpr float kPi = 3.14159265f;
constexpr float kCrackBass = 0.78f, kRearmBass = 0.5f;
constexpr double kHoldQuiet = 2.5, kFade = 1.6;
constexpr float kReveal = 7.f;              // 1/s: the fracture races out in about 150 ms

// Smooth 1-D noise for the jitter of rays and rings.
struct Wobble {
    std::vector<float> v; float step;
    Wobble(juce::Random& r, int n, float stepPx, float amp) : step(stepPx) {
        v.resize((size_t)n); float d = 0.f, x = 0.f;
        for (auto& e : v) { d += (r.nextFloat() - 0.5f) * amp; d *= 0.8f; x += d; x *= 0.96f; if (r.nextFloat() < 0.04f) x += (r.nextBool() ? 1.f : -1.f) * amp * 1.5f; e = x; }   // small drift, reverting, with rare kinks
    }
    float at(float dist) const {
        const float f = dist / step; const int i = (int)f; const float t = f - i;
        if (i + 1 >= (int)v.size()) return v.back();
        return v[(size_t)i] + (v[(size_t)i + 1] - v[(size_t)i]) * t;
    }
};
}

void Pane::advance(double dt, float bass, float energy, int w, int h) {
    dt = std::min(dt, 0.1); w_ = w; h_ = h;
    cooldown_ = std::max(0.0, cooldown_ - dt);
    if (bass < kRearmBass) armed_ = true;
    if (armed_ && bass > kCrackBass && cooldown_ <= 0.0 && impacts_.size() < 3) { impact(w, h); armed_ = false; cooldown_ = 1.5; }
    for (auto& i : impacts_) i.reveal = std::min(1.f, i.reveal + (float)dt * kReveal);
    if (impacts_.empty()) return;
    if (energy > 0.12f) { quiet_ = 0.0; strength_ = std::min(1.f, strength_ + (float)dt * 6.f); }
    else {
        quiet_ += dt;
        if (quiet_ > kHoldQuiet) strength_ -= (float)(dt / kFade);
        if (strength_ <= 0.f) { impacts_.clear(); strength_ = 0.f; quiet_ = 0.0; }
    }
}

void Pane::impact(int w, int h) {
    Impact im;
    im.at = {w * (0.2f + 0.6f * rnd_.nextFloat()), h * (0.25f + 0.5f * rnd_.nextFloat())};
    im.maxR = 0.55f * std::max(w, h);
    im.layer = buildLayer(im.at, im.maxR, w, h);
    impacts_.push_back(std::move(im));
}

// The fracture, drawn once. Geometry: N rays with smooth angular jitter, K rings with radial jitter;
// the cells between them are the shards.
juce::Image Pane::buildLayer(juce::Point<float> at, float maxR, int w, int h) {
    juce::Image img(juce::Image::ARGB, std::max(1, w), std::max(1, h), true);
    juce::Graphics g(img);
    juce::Random& r = rnd_;
    const int N = 9 + r.nextInt(5);
    // Rays: base angle, length, jitter table.
    struct Ray { float a, len; Wobble jit; float bright; };
    std::vector<Ray> rays;
    for (int i = 0; i < N; ++i) {
        const float a = 2.f * kPi * (i + 0.7f * (r.nextFloat() - 0.5f)) / N;
        const float len = maxR * (r.nextFloat() < 0.35f ? 1.0f + 0.8f * r.nextFloat() : 0.3f + 0.6f * r.nextFloat());
        rays.push_back({a, len, Wobble(r, 300, 5.f, 0.022f), 0.55f + 0.45f * r.nextFloat()});
    }
    auto rayPt = [&](const Ray& ry, float d) { const float a = ry.a + ry.jit.at(d); return juce::Point<float>(at.x + std::cos(a) * d, at.y + std::sin(a) * d); };
    // Rings: radii growing outward, each with its own jitter around the circle.
    const int K = 3 + r.nextInt(3);
    std::vector<float> ringR; std::vector<Wobble> ringJ;
    float rr = 9.f + 6.f * r.nextFloat();
    for (int k = 0; k < K; ++k) { ringR.push_back(rr); ringJ.emplace_back(r, 128, 2.f * kPi / 120.f, 0.035f); rr *= 1.7f + 0.5f * r.nextFloat(); }
    auto ringPt = [&](int k, float a) { const float rad = ringR[(size_t)k] * (1.f + 0.6f * ringJ[(size_t)k].at(std::fmod(a + 4.f * kPi, 2.f * kPi))); return juce::Point<float>(at.x + std::cos(a) * rad, at.y + std::sin(a) * rad); };
    auto ringRadAt = [&](int k, float a) { return ringR[(size_t)k] * (1.f + 0.6f * ringJ[(size_t)k].at(std::fmod(a + 4.f * kPi, 2.f * kPi))); };

    // ---- Shards as lit facets: each cell gets its own tilt, so its brightness differs a little. ----
    const juce::Point<float> light(-0.6f, -0.8f);
    for (int j = 0; j < N; ++j) {
        const Ray& A = rays[(size_t)j]; const Ray& B = rays[(size_t)((j + 1) % N)];
        const float aA = A.a, aB = B.a + (j + 1 == N ? 2.f * kPi : 0.f);
        for (int k = 0; k <= K; ++k) {
            const float r0 = k == 0 ? 0.f : ringR[(size_t)k - 1], r1 = k == K ? std::min(A.len, B.len) : ringR[(size_t)k];
            if (r1 <= r0 + 2.f) continue;
            juce::Path p;
            // up ray A
            for (float d = r0; d <= r1; d += 4.f) { const auto q = rayPt(A, k == 0 ? d : (k == K ? d : std::min(d, ringRadAt(k, A.a + A.jit.at(d)) ))); d == r0 ? p.startNewSubPath(q) : p.lineTo(q); }
            // along ring k (outer edge) from A to B
            if (k < K) for (float a = aA; a <= aB; a += 0.08f) p.lineTo(ringPt(k, a));
            else p.lineTo(rayPt(B, r1));
            // down ray B
            for (float d = r1; d >= r0; d -= 4.f) p.lineTo(rayPt(B, d));
            // back along ring k-1 (inner edge) from B to A
            if (k > 0) for (float a = aB; a >= aA; a -= 0.08f) p.lineTo(ringPt(k - 1, a));
            p.closeSubPath();
            // Facet tilt: a random normal; brightness from the light direction.
            const float tx = (r.nextFloat() - 0.5f), ty = (r.nextFloat() - 0.5f);
            const float lit = (tx * light.x + ty * light.y) * 1.6f;          // -0.8 .. 0.8
            const float near = 1.f - std::min(1.f, (0.5f * (r0 + r1)) / (0.6f * maxR));   // inner shards shifted more
            const juce::Colour c = lit >= 0 ? juce::Colours::white.withAlpha(0.03f + 0.11f * lit * (0.4f + 0.6f * near))
                                            : juce::Colours::black.withAlpha(0.02f + 0.10f * -lit * (0.4f + 0.6f * near));
            const auto b = p.getBounds();
            g.setGradientFill(juce::ColourGradient(c, b.getX(), b.getY(), c.withMultipliedAlpha(0.25f), b.getRight(), b.getBottom(), false));
            g.fillPath(p);
        }
    }

    // ---- Scatter haze: crushed glass goes white; densest at the impact, and a soft fringe along cracks. ----
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.45f), at.x, at.y, juce::Colours::white.withAlpha(0.f), at.x + ringR[1], at.y, true));
    g.fillEllipse(at.x - ringR[1], at.y - ringR[1], 2 * ringR[1], 2 * ringR[1]);
    auto strokeSoft = [&](const juce::Path& p, float alpha, float width) { g.setColour(juce::Colour(0xffeaf1ff).withAlpha(alpha)); g.strokePath(p, juce::PathStrokeType(width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); };
    // ---- Crack lines: dark fracture side, then the bright refracted edge, segment by segment. ----
    auto crack = [&](const std::vector<juce::Point<float>>& pts, float w0, float w1, float bright) {
        if (pts.size() < 2) return;
        juce::Path whole; whole.startNewSubPath(pts[0]); for (size_t i = 1; i < pts.size(); ++i) whole.lineTo(pts[i]);
        strokeSoft(whole, 0.10f * bright, 3.5f);
        float sh = 0.6f + 0.4f * r.nextFloat();
        for (size_t i = 1; i < pts.size(); ++i) {
            const float t = (float)i / (float)pts.size(), w = w0 + (w1 - w0) * t;
            sh += (r.nextFloat() - 0.5f) * 0.45f; sh = juce::jlimit(0.2f, 1.f, sh);
            const juce::Line<float> seg(pts[i - 1], pts[i]);
            g.setColour(juce::Colours::black.withAlpha(0.5f * sh)); g.drawLine(juce::Line<float>(seg.getStart().translated(0.6f, 0.8f), seg.getEnd().translated(0.6f, 0.8f)), w + 0.6f);
            g.setColour(juce::Colour(0xfff4f7ff).withAlpha((0.45f + 0.55f * sh) * bright)); g.drawLine(seg, w * 0.85f);
        }
    };
    for (const auto& ry : rays) {
        std::vector<juce::Point<float>> pts;
        for (float d = 0.f; d <= ry.len; d += 3.f + 3.f * r.nextFloat()) pts.push_back(rayPt(ry, d));
        crack(pts, 1.4f, 0.35f, ry.bright);
        // Branches off the ray, thinner, with their own jitter; some branch again.
        const int nb = 1 + r.nextInt(3);
        for (int b = 0; b < nb; ++b) {
            const float d0 = ry.len * (0.2f + 0.6f * r.nextFloat());
            float a = ry.a + ry.jit.at(d0) + (r.nextBool() ? 1.f : -1.f) * (0.35f + 0.7f * r.nextFloat());
            auto q = rayPt(ry, d0); std::vector<juce::Point<float>> bp{q};
            const float blen = ry.len * (0.1f + 0.3f * r.nextFloat()); float drift = 0.f;
            for (float d = 0.f; d < blen; d += 3.f + 3.f * r.nextFloat()) { drift += (r.nextFloat() - 0.5f) * 0.5f; drift *= 0.8f; q = {q.x + std::cos(a + drift) * 4.f, q.y + std::sin(a + drift) * 4.f}; bp.push_back(q); }
            crack(bp, 0.7f, 0.3f, ry.bright * 0.8f);
        }
    }
    for (int k = 0; k < K; ++k) {
        // Rings are broken into arcs of uneven visibility, like real conchoidal fractures.
        float a = 0.f;
        while (a < 2.f * kPi) {
            const float span = 0.3f + 1.2f * r.nextFloat();
            std::vector<juce::Point<float>> pts;
            for (float t = a; t <= a + span; t += 0.05f) pts.push_back(ringPt(k, t));
            crack(pts, 0.8f - 0.1f * k, 0.5f, 0.35f + 0.6f * r.nextFloat());
            a += span + 0.1f * r.nextFloat();
        }
    }
    // ---- Micro-fractures around the impact. ----
    for (int i = 0; i < 40; ++i) {
        const float a = r.nextFloat() * 2.f * kPi, d0 = 3.f + ringR[std::min(2, K - 1)] * r.nextFloat() * r.nextFloat();
        const juce::Point<float> p0(at.x + std::cos(a) * d0, at.y + std::sin(a) * d0);
        const float ang = r.nextFloat() * 2.f * kPi, len = 2.f + 9.f * r.nextFloat();
        g.setColour(juce::Colours::white.withAlpha(0.25f + 0.5f * r.nextFloat()));
        g.drawLine(p0.x, p0.y, p0.x + std::cos(ang) * len, p0.y + std::sin(ang) * len, 0.5f);
    }
    // ---- Crater. ----
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.95f), at.x, at.y, juce::Colours::white.withAlpha(0.f), at.x + 5.f, at.y, true));
    g.fillEllipse(at.x - 5.f, at.y - 5.f, 10.f, 10.f);
    return img;
}

void Pane::draw(juce::Graphics& g) const {
    // The pane itself: a soft diagonal sheen and a faint lit top edge. Always present, barely.
    if (w_ > 0 && h_ > 0) {
        const float W = (float)w_, H = (float)h_;
        g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.f), W * 0.15f, H, juce::Colours::white.withAlpha(0.035f), W * 0.55f, 0.f, false));
        g.fillRect(0.f, 0.f, W, H);
        g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.06f), 0.f, 0.f, juce::Colours::white.withAlpha(0.f), 0.f, 14.f, false));
        g.fillRect(0.f, 0.f, W, 14.f);
    }
    if (impacts_.empty() || strength_ <= 0.f) return;
    for (const auto& im : impacts_) {
        juce::Graphics::ScopedSaveState ss(g);
        if (im.reveal < 1.f) {   // the fracture races outward from the impact
            const float R = 8.f + im.reveal * im.reveal * im.maxR * 1.6f;
            juce::Path clip; clip.addEllipse(im.at.x - R, im.at.y - R, 2 * R, 2 * R); g.reduceClipRegion(clip);
        }
        g.setOpacity(strength_);
        g.drawImageAt(im.layer, 0, 0);
    }
}

}  // namespace glass
