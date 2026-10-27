#include "Wordmark.h"
#include "Look.h"
#include <cmath>

namespace wordmark {
using namespace look;

namespace {
struct Px { float r, g, b, a; };
// Bilinear sample of a premultiplied ARGB image; outside the image is transparent.
inline Px sample(const juce::Image::BitmapData& bd, float x, float y) {
    const int x0 = (int)std::floor(x), y0 = (int)std::floor(y);
    const float fx = x - x0, fy = y - y0;
    auto get = [&](int xi, int yi) -> Px {
        if (xi < 0 || yi < 0 || xi >= bd.width || yi >= bd.height) return {0, 0, 0, 0};
        const auto* p = (const juce::PixelARGB*)bd.getPixelPointer(xi, yi);
        return {p->getRed() / 255.f, p->getGreen() / 255.f, p->getBlue() / 255.f, p->getAlpha() / 255.f};
    };
    const Px a = get(x0, y0), b = get(x0 + 1, y0), c = get(x0, y0 + 1), d = get(x0 + 1, y0 + 1);
    auto lerp = [](float p, float q, float t) { return p + (q - p) * t; };
    return {lerp(lerp(a.r, b.r, fx), lerp(c.r, d.r, fx), fy), lerp(lerp(a.g, b.g, fx), lerp(c.g, d.g, fx), fy),
            lerp(lerp(a.b, b.b, fx), lerp(c.b, d.b, fx), fy), lerp(lerp(a.a, b.a, fx), lerp(c.a, d.a, fx), fy)};
}
inline float clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
}  // namespace

Wordmark::Wordmark(std::function<float()> level) : level_(std::move(level)) {
    setInterceptsMouseClicks(false, false);
    setOpaque(false);
    start_ = juce::Time::getMillisecondCounterHiRes() * 0.001;
    startTimerHz(15);
}
Wordmark::~Wordmark() { stopTimer(); }

void Wordmark::resized() { source_ = juce::Image(); out_ = juce::Image(); }

// The clean text (and the cursor's cell), drawn once at 2x.
void Wordmark::buildSource() {
    const int w = std::max(2, (int)(getWidth() * kScale)), h = std::max(2, (int)(getHeight() * kScale));
    // Software images: both are read or written pixel by pixel every frame, which on a native (GPU) image
    // would mean a read-back and an upload each time.
    source_ = juce::Image(juce::Image::ARGB, w, h, true, juce::SoftwareImageType());
    out_ = juce::Image(juce::Image::ARGB, w, h, true, juce::SoftwareImageType());
    fullPass_ = true;
}

void Wordmark::render(double t) {
    if (!source_.isValid()) buildSource();
    const int w = source_.getWidth(), h = source_.getHeight();
    // --- Glitch schedule: short, rare, a little more frequent with signal. ---
    const float lvl = level_ ? clamp01(level_()) : 0.f;
    if (t >= nextGlitch_ && glitchEnd_ < t) {
        glitchStart_ = t; glitchEnd_ = t + 0.06 + 0.12 * rnd_.nextDouble();
        nextGlitch_ = glitchEnd_ + 2.5 + 7.0 * rnd_.nextDouble() * (1.f - 0.6f * lvl);
        for (int i = 0; i < 3; ++i) { bandH_[i] = 2 + rnd_.nextInt(4); bandY_[i] = rnd_.nextInt(std::max(1, h / 2 - bandH_[i])); bandDx_[i] = (rnd_.nextFloat() * 2.f - 1.f) * 3.5f; }
    }
    const bool glitching = t >= glitchStart_ && t < glitchEnd_;
    const float gph = glitching ? (float)((t - glitchStart_) / (glitchEnd_ - glitchStart_)) : 0.f;
    const float gAmt = glitching ? std::sin(gph * 3.14159f) : 0.f;        // eases in and out

    // --- Source: text plus the breathing cursor. ---
    source_.clear(source_.getBounds(), juce::Colours::transparentBlack);
    {
        juce::Graphics g(source_);
        g.addTransform(juce::AffineTransform::scale(kScale));
        auto r = juce::Rectangle<float>(0, 0, (float)getWidth(), (float)getHeight()).reduced((float)kPad, 0.f);
        const auto f = font(17.f, 600).withExtraKerningFactor(0.06f);
        g.setFont(f);
        const juce::String word = look::theme().wordmark;
        const float tw = juce::GlyphArrangement::getStringWidth(f, word);
        textL_ = r.getX() * kScale; textR_ = (r.getX() + tw + 3.f + juce::GlyphArrangement::getStringWidth(f, "_")) * kScale;
        g.setColour(kAccent);
        g.drawText(word, r, juce::Justification::centredLeft);
        // Cursor: soft fade in and out, about one breath per 1.2 s.
        const float breathe = 0.5f + 0.5f * std::sin((float)(t * 2.0 * 3.14159265 / 1.2));
        const float ca = breathe * breathe * (3.f - 2.f * breathe);
        g.setColour(kAccent.withAlpha(0.15f + 0.85f * ca));
        g.drawText("_", r.withX(r.getX() + tw + 3.f), juce::Justification::centredLeft);
        cursorL_ = (r.getX() + tw + 3.f) * kScale;
    }
    // The lens maps each output pixel from a few pixels around it. Between glitches the word is still and only
    // the breathing cursor changes, so only the columns from just before the cursor are recomputed (the margin
    // covers the chromatic shift and the softening taps); a glitch, and the frame after it, redo the whole word.
    const bool full = fullPass_ || glitching || wasGlitching_;
    wasGlitching_ = glitching; fullPass_ = false;
    const int xFrom = full ? 0 : juce::jlimit(0, w, (int)cursorL_ - 24);

    // --- Lens pass. ---
    const float cx = 0.5f * (textL_ + textR_), cy = h * 0.5f, hw = std::max(1.f, 0.5f * (textR_ - textL_));
    const float kBarrel = 0.006f;                                  // spherical: slight barrel, grows with r^2
    const float kSoft = 0.9f;                                      // spherical: edge softening weight
    const float kChroma = 0.0045f + 0.012f * gAmt;                 // lateral chromatic aberration: ~1 px fringe at the word ends
    juce::Image::BitmapData src(source_, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dst(out_, juce::Image::BitmapData::readWrite);   // columns left of xFrom keep the last frame
    for (int y = 0; y < h; ++y) {
        float bandDx = 0.f;
        if (glitching) for (int i = 0; i < 3; ++i) if (y >= bandY_[i] * 2 && y < (bandY_[i] + bandH_[i]) * 2) bandDx = bandDx_[i] * kScale * gAmt;
        const float v = (y - cy) / cy;
        auto* row = dst.getLinePointer(y);
        for (int x = xFrom; x < w; ++x) {
            const float u = (x - cx) / hw;
            const float r2 = u * u + v * v * 0.15f;                 // the word is wide; weight the horizontal
            const float barrel = 1.f + kBarrel * r2;
            auto pos = [&](float chroma) { return std::pair<float, float>{cx + u * hw * barrel * (1.f + chroma * r2) - bandDx, cy + v * cy * barrel}; };
            const auto pr = pos(+kChroma), pg = pos(0.f), pb = pos(-kChroma);
            Px R = sample(src, pr.first, pr.second), G = sample(src, pg.first, pg.second), B = sample(src, pb.first, pb.second);
            // Spherical softening: blend the green (sharp) channel with a wider tap away from the centre.
            const float soft = clamp01(r2 * kSoft) * 0.5f;
            if (soft > 0.f) {
                const Px s1 = sample(src, pg.first - 1.6f, pg.second), s2 = sample(src, pg.first + 1.6f, pg.second);
                G.g = G.g * (1.f - soft) + 0.5f * (s1.g + s2.g) * soft; G.a = G.a * (1.f - soft) + 0.5f * (s1.a + s2.a) * soft;
            }
            const float a = std::max({R.a, G.a, B.a});
            auto* p = (juce::PixelARGB*)(row + x * 4);
            p->setARGB((juce::uint8)(a * 255.f), (juce::uint8)(clamp01(R.r) * 255.f), (juce::uint8)(clamp01(G.g) * 255.f), (juce::uint8)(clamp01(B.b) * 255.f));
        }
    }
}

void Wordmark::renderNow() {
    const double t = fixedTime_ >= 0 ? fixedTime_ : juce::Time::getMillisecondCounterHiRes() * 0.001 - start_;
    render(t);
}

void Wordmark::timerCallback() { if (!isShowing()) return; renderNow(); repaint(); }

void Wordmark::paint(juce::Graphics& g) {
    if (!out_.isValid()) renderNow();
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImageTransformed(out_, juce::AffineTransform::scale(1.f / kScale), false);
}

}  // namespace wordmark
