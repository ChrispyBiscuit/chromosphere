#include "Glass.h"
#include <cmath>
#include <functional>

namespace glass {
namespace {
constexpr float kPi = 3.14159265f;
constexpr float kCrackBass = 0.78f;     // very elevated
constexpr float kRearmBass = 0.5f;
constexpr double kHoldQuiet = 2.5;      // seconds of near-silence before the cracks start to fade
constexpr double kFade = 1.6;           // seconds to fade out
constexpr float kReveal = 9.f;          // reveal speed (1/s): the pane fractures in about a tenth of a second
}

void Pane::advance(double dt, float bass, float energy, int w, int h) {
    dt = std::min(dt, 0.1);
    cooldown_ = std::max(0.0, cooldown_ - dt);
    if (bass < kRearmBass) armed_ = true;
    if (armed_ && bass > kCrackBass && cooldown_ <= 0.0 && impacts_.size() < 4) { impact(w, h); armed_ = false; cooldown_ = 1.2; }
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
    im.at = {w * (0.15f + 0.7f * rnd_.nextFloat()), h * (0.2f + 0.6f * rnd_.nextFloat())};
    const float maxLen = 0.5f * std::max(w, h);
    const int rays = 8 + rnd_.nextInt(6);
    std::vector<juce::Point<float>> rayMid;
    // A fracture: a jittery walk with a tapering width and a highlight that comes and goes along it.
    auto walk = [&](juce::Point<float> from, float ang, float len, float w0, float delay, float wobble) {
        Line L; L.width = w0; L.delay = delay;
        L.pts.push_back(from); L.widths.push_back(w0); L.shine.push_back(1.f);
        float a = ang, travelled = 0.f, drift = 0.f, shine = 0.8f + 0.2f * rnd_.nextFloat();
        while (travelled < len) {
            const float step = 3.f + 5.f * rnd_.nextFloat();
            drift += (rnd_.nextFloat() - 0.5f) * wobble; drift *= 0.8f;        // jittery, mean-reverting
            if (rnd_.nextFloat() < 0.06f) drift += (rnd_.nextBool() ? 1.f : -1.f) * 0.5f;   // occasional kink
            a = ang + drift;
            from = {from.x + std::cos(a) * step, from.y + std::sin(a) * step};
            travelled += step;
            shine += (rnd_.nextFloat() - 0.5f) * 0.5f; shine = juce::jlimit(0.25f, 1.f, shine);
            const float t = travelled / len;
            L.pts.push_back(from); L.widths.push_back(w0 * (1.f - 0.8f * t)); L.shine.push_back(shine);
            if (from.x < -20 || from.y < -20 || from.x > w + 20 || from.y > h + 20) break;
        }
        return L;
    };
    std::function<void(const Line&, float, float, int)> branchOff = [&](const Line& parent, float ang, float len, int depth) {
        const int branches = (depth == 0 ? 1 : 0) + rnd_.nextInt(depth == 0 ? 3 : 2);
        for (int b = 0; b < branches && parent.pts.size() > 5; ++b) {
            const int at = 2 + rnd_.nextInt((int)parent.pts.size() - 4);
            const float ba = ang + (rnd_.nextBool() ? 1.f : -1.f) * (0.3f + 0.7f * rnd_.nextFloat());
            const float blen = len * (0.12f + 0.3f * rnd_.nextFloat());
            Line B = walk(parent.pts[(size_t)at], ba, blen, parent.widths[(size_t)at] * 0.7f, parent.delay + 0.25f * (float)at / (float)parent.pts.size() + 0.1f, 0.7f);
            if (depth < 1 && rnd_.nextFloat() < 0.5f) branchOff(B, ba, blen, depth + 1);
            im.lines.push_back(std::move(B));
        }
    };
    for (int i = 0; i < rays; ++i) {
        const float ang = 2.f * kPi * (i + 0.6f * (rnd_.nextFloat() - 0.5f)) / rays;
        const float len = maxLen * (rnd_.nextFloat() < 0.3f ? 0.9f + 0.6f * rnd_.nextFloat() : 0.15f + 0.5f * rnd_.nextFloat());
        Line L = walk(im.at, ang, len, 1.1f + 0.6f * rnd_.nextFloat(), 0.f, 0.5f);
        branchOff(L, ang, len, 0);
        if (L.pts.size() > 3) rayMid.push_back(L.pts[L.pts.size() * 2 / 3]);
        im.lines.push_back(std::move(L));
    }
    // Spider web near the impact: short tangential cracks linking the rays.
    const int webs = 10 + rnd_.nextInt(10);
    for (int k = 0; k < webs; ++k) {
        const float rad = 8.f + 55.f * rnd_.nextFloat() * rnd_.nextFloat(), a0 = rnd_.nextFloat() * 2.f * kPi;
        const juce::Point<float> p0{im.at.x + std::cos(a0) * rad, im.at.y + std::sin(a0) * rad};
        const float tang = a0 + kPi * 0.5f * (rnd_.nextBool() ? 1.f : -1.f) + (rnd_.nextFloat() - 0.5f) * 0.5f;
        im.lines.push_back(walk(p0, tang, 6.f + 22.f * rnd_.nextFloat(), 0.6f, 0.25f + 0.3f * rnd_.nextFloat(), 0.8f));
    }
    // Shards: faint sheen on the wedges between neighbouring rays.
    for (size_t i = 0; i + 1 < rayMid.size(); ++i) {
        if (rnd_.nextFloat() > 0.5f) continue;
        juce::Path p; p.startNewSubPath(im.at); p.lineTo(rayMid[i]); p.lineTo(rayMid[i + 1]); p.closeSubPath();
        im.shards.push_back(std::move(p));
    }
    impacts_.push_back(std::move(im));
}

void Pane::draw(juce::Graphics& g) const {
    if (impacts_.empty() || strength_ <= 0.f) return;
    const float s = strength_;
    const juce::Colour edge(0xffe6eeff);
    for (const auto& im : impacts_) {
        for (const auto& sh : im.shards) {
            g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.07f * s * im.reveal), im.at.x, im.at.y, juce::Colours::white.withAlpha(0.f), im.at.x + 90.f, im.at.y + 60.f, true));
            g.fillPath(sh);
        }
        for (const auto& L : im.lines) {
            const float rv = juce::jlimit(0.f, 1.f, (im.reveal - L.delay) / std::max(0.05f, 1.f - L.delay));
            const int n = (int)std::ceil(rv * (float)(L.pts.size() - 1));
            if (n < 1) continue;
            // Segment by segment: width tapers and the highlight catches the light unevenly.
            for (int i = 1; i <= n && i < (int)L.pts.size(); ++i) {
                const juce::Line<float> seg(L.pts[(size_t)i - 1], L.pts[(size_t)i]);
                const float w = std::max(0.35f, 0.5f * (L.widths[(size_t)i - 1] + L.widths[(size_t)i]));
                const float sh = 0.5f * (L.shine[(size_t)i - 1] + L.shine[(size_t)i]);
                g.setColour(juce::Colours::black.withAlpha(0.5f * s * sh)); g.drawLine(juce::Line<float>(seg.getStart().translated(0.7f, 0.9f), seg.getEnd().translated(0.7f, 0.9f)).withShortenedStart(-0.5f).withShortenedEnd(-0.5f), w + 1.0f);
                g.setColour(edge.withAlpha(0.16f * s * sh)); g.drawLine(seg, w * 2.4f);
                g.setColour(edge.withAlpha((0.45f + 0.5f * sh) * s)); g.drawLine(seg.withShortenedStart(-0.3f).withShortenedEnd(-0.3f), w);
            }
        }
        const float R = 6.f * im.reveal;
        g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.75f * s), im.at.x, im.at.y, juce::Colours::white.withAlpha(0.f), im.at.x + R, im.at.y, true));
        g.fillEllipse(im.at.x - R, im.at.y - R, 2 * R, 2 * R);
    }
}

}  // namespace glass
