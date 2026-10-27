#include "UltraLook.h"
#include "SevenSeg.h"
#include "Vfd.h"
#include <UltraData.h>
#include <cmath>
#include <map>
#include <vector>

namespace look {

// Ultraviolet: the EXO palette with the accent moved from solar orange to UV violet, and the greys cooled a touch.
const juce::Colour kBg(0xff111014), kPlate(0xff1a191e), kPlateEdge(0xff27262c), kLine(0xff2b2930),
    kText(0xffece8f0), kTextDim(0xff9a95a3), kTextFaint(0xff5e5966), kAccent(0xff9b6bff), kAccentDim(0xff3f2d6e),
    kDisplayBg(0xff0b0a0e), kGrid(0xff1d1a24), kCurve(0xffb08cff), kCurveFill(0x409b6bff), kMeter(0xffc4bcd2), kMeterHot(0xff9b6bff), kDanger(0xffff3d6e);

namespace {
juce::Typeface::Ptr loadFace(const char* name) {
    int size = 0;
    if (const char* data = UltraData::getNamedResource(name, size)) return juce::Typeface::createSystemTypefaceFor(data, (size_t)size);
    return nullptr;
}
}  // namespace

// Typefaces and image caches shared by every open editor in the process. They live in one object held through
// juce::SharedResourcePointer by each editor's ResourceScope, so they are released when the last editor closes. No
// JUCE object is a static: statics are destroyed while the host unloads the DLL, after JUCE has shut down and too
// late to release Direct2D images safely (EXO's optimization guide, 4; INFRA's RCA).
struct Resources {
    juce::Typeface::Ptr regular = loadFace("BarlowRegular_ttf"), medium = loadFace("BarlowMedium_ttf"), semibold = loadFace("BarlowSemiBold_ttf");
    std::map<juce::String, juce::Image> assetCache;
    std::map<std::pair<int, int>, juce::Image> backgroundCache;
    juce::Image plateGrain;
    std::map<int, juce::Image> dialCache;
    Resources();
    ~Resources();
};
namespace {
Resources* current = nullptr;    // message thread: the set the open editors share
}
Resources::Resources() { current = this; }
Resources::~Resources() { if (current == this) current = nullptr; }
ResourceScope::ResourceScope() : res_(std::make_unique<juce::SharedResourcePointer<Resources>>()) {}
ResourceScope::~ResourceScope() = default;

namespace {
// Drawing outside any editor (should not happen) gets a set that is deliberately never destroyed.
Resources& res() {
    if (current) return *current;
    jassertfalse;
    static Resources* orphan = new Resources();
    return *orphan;
}
juce::Typeface::Ptr face(int weight) {
    auto& r = res();
    if (weight >= 600 && r.semibold) return r.semibold;
    if (weight >= 500 && r.medium) return r.medium;
    return r.regular;
}
}  // namespace

juce::Image asset(const char* name) {
    auto& cache = res().assetCache;
    auto it = cache.find(name);
    if (it != cache.end()) return it->second;
    int size = 0;
    const char* data = UltraData::getNamedResource(name, size);
    juce::Image im = data ? juce::ImageFileFormat::loadFrom(data, (size_t)size) : juce::Image();
    cache[name] = im;
    return im;
}

juce::Font font(float h, int weight) {
    if (auto f = face(weight)) return juce::Font(juce::FontOptions(f).withHeight(h));
    return juce::Font(juce::FontOptions(h, weight >= 500 ? juce::Font::bold : juce::Font::plain));
}
juce::Font caps(float h) { return font(h, 500).withExtraKerningFactor(0.12f); }

void drawCaps(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just, float h, juce::Colour c) {
    g.setFont(caps(h)); g.setColour(c); g.drawText(text.toUpperCase(), area, just, false);
}

const juce::Image& backgroundTexture(int w, int h) {
    auto& cache = res().backgroundCache;
    auto key = std::make_pair(w, h);
    if (auto it = cache.find(key); it != cache.end()) return it->second;
    if (cache.size() > 6) cache.clear();
    juce::Image im(juce::Image::RGB, std::max(1, w), std::max(1, h), false);
    juce::Graphics g(im);
    g.setGradientFill(juce::ColourGradient(kBg.brighter(0.06f), 0.f, 0.f, kBg, 0.f, (float)h, false));
    g.fillAll();
    // Corona streamers (generated image), scaled to cover, kept faint.
    if (auto cor = asset("corona_bg_jpg"); cor.isValid()) {
        const float s = std::max((float)w / cor.getWidth(), (float)h / cor.getHeight());
        g.setOpacity(1.0f);
        g.drawImageTransformed(cor, juce::AffineTransform::scale(s).translated((w - cor.getWidth() * s) * 0.5f, (h - cor.getHeight() * s) * 0.5f), false);
        g.setOpacity(1.f);
    }
    // Warm cast at the top edge, like light from above the panel.
    g.setGradientFill(juce::ColourGradient(kAccent.withAlpha(0.05f), 0.f, 0.f, kAccent.withAlpha(0.f), 0.f, h * 0.35f, false));
    g.fillAll();
    juce::Random rnd(7);
    for (int i = 0; i < w * h / 14; ++i) {           // fine matte grain, barely there
        g.setColour((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha(0.012f + rnd.nextFloat() * 0.02f));
        g.fillRect(rnd.nextInt(w), rnd.nextInt(h), 1, 1);
    }
    return cache[key] = im;
}

void drawBolt(juce::Graphics& g, float cx, float cy, float r) {
    g.setColour(juce::Colours::black.withAlpha(0.55f)); g.fillEllipse(cx - r - 1.f, cy - r, 2 * r + 2.f, 2 * r + 2.f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff5a5d63), cx - r * 0.6f, cy - r * 0.7f, juce::Colour(0xff26282c), cx + r * 0.6f, cy + r * 0.8f, false));
    g.fillEllipse(cx - r, cy - r, 2 * r, 2 * r);
    // Hex socket.
    juce::Path hex; hex.addPolygon({cx, cy}, 6, r * 0.52f, 0.f);
    g.setColour(juce::Colour(0xff0e0f11)); g.fillPath(hex);
    g.setColour(juce::Colours::white.withAlpha(0.10f)); g.drawEllipse(cx - r + 0.5f, cy - r + 0.5f, 2 * r - 1.f, 2 * r - 1.f, 1.f);
}


// Matte anodised surface for the instrument bays: a fine, low-contrast grain and nothing else.
// Restraint is the point; scratch or wear overlays read as game art.
namespace {
float vnoise(float x, float y) {
    const int xi = (int)std::floor(x), yi = (int)std::floor(y);
    const float fx = x - xi, fy = y - yi;
    auto h = [](int a, int b) { unsigned n = (unsigned)(a * 374761393 + b * 668265263); n = (n ^ (n >> 13)) * 1274126177u; return (float)((n ^ (n >> 16)) & 0xffff) / 65535.f; };
    auto sm = [](float t) { return t * t * (3.f - 2.f * t); };
    const float ux = sm(fx), uy = sm(fy);
    return (h(xi, yi) * (1 - ux) + h(xi + 1, yi) * ux) * (1 - uy) + (h(xi, yi + 1) * (1 - ux) + h(xi + 1, yi + 1) * ux) * uy;
}
}
const juce::Image& plateTexture() {
    auto& img = res().plateGrain;
    if (img.isValid()) return img;
    const int N = 512;
    juce::Image sw(juce::Image::ARGB, N, N, true, juce::SoftwareImageType());
    {
    juce::Image::BitmapData bd(sw, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < N; ++y) for (int x = 0; x < N; ++x) {
        const float g1 = vnoise(x * 1.9f, y * 1.9f), g2 = vnoise(x * 0.45f + 7.f, y * 0.45f + 3.f);
        const float v = (g1 - 0.5f) * 0.75f + (g2 - 0.5f) * 0.25f;        // -0.5..0.5, mostly fine
        const float a = std::min(1.f, std::abs(v) * 2.f);
        const juce::uint8 c = v > 0 ? 255 : 0;
        auto* p = (juce::PixelARGB*)bd.getPixelPointer(x, y);
        p->setARGB((juce::uint8)(a * 255.f), (juce::uint8)(c * a), (juce::uint8)(c * a), (juce::uint8)(c * a));   // premultiplied light/dark speckle
    }
    }
    img = juce::NativeImageType().convert(sw);
    return img;
}

void gaussianBlur(juce::Image& img, int radius, float sigma) {
    if (!img.isValid() || img.getFormat() != juce::Image::ARGB || radius < 1) return;
    const int W = img.getWidth(), H = img.getHeight(), taps = 2 * radius + 1;
    std::vector<int> wt((size_t)taps);
    { float sum = 0.f; std::vector<float> f((size_t)taps);
      for (int i = 0; i < taps; ++i) { const float d = (float)(i - radius); f[(size_t)i] = std::exp(-d * d / (2.f * sigma * sigma)); sum += f[(size_t)i]; }
      for (int i = 0; i < taps; ++i) wt[(size_t)i] = (int)std::lround(f[(size_t)i] / sum * 4096.f); }
    juce::Image::BitmapData bd(img, juce::Image::BitmapData::readWrite);
    std::vector<juce::uint32> line((size_t)std::max(W, H));   // one row or column of packed pixels
    auto pass = [&](int count, int len, auto pixelAt) {
        for (int k = 0; k < count; ++k) {
            for (int i = 0; i < len; ++i) line[(size_t)i] = pixelAt(k, i)->getNativeARGB();
            for (int i = 0; i < len; ++i) {
                int a = 0, r = 0, g = 0, b = 0;
                for (int t = 0; t < taps; ++t) {
                    const int j = i + t - radius;
                    if (j < 0 || j >= len) continue;                   // outside is transparent
                    const juce::uint32 p = line[(size_t)j]; const int w = wt[(size_t)t];
                    a += (int)(p >> 24) * w; r += (int)((p >> 16) & 0xff) * w; g += (int)((p >> 8) & 0xff) * w; b += (int)(p & 0xff) * w;
                }
                pixelAt(k, i)->setARGB((juce::uint8)std::min(255, a >> 12), (juce::uint8)std::min(255, r >> 12), (juce::uint8)std::min(255, g >> 12), (juce::uint8)std::min(255, b >> 12));
            }
        }
    };
    pass(H, W, [&](int y, int x) { return (juce::PixelARGB*)bd.getPixelPointer(x, y); });
    pass(W, H, [&](int x, int y) { return (juce::PixelARGB*)bd.getPixelPointer(x, y); });
}

void drawPlate(juce::Graphics& g, juce::Rectangle<float> r, float corner) {
    // Outer machined frame.
    juce::Path p; p.addRoundedRectangle(r, corner);
    juce::DropShadow(juce::Colours::black.withAlpha(0.5f), 16, {0, 6}).drawForPath(g, p);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2a2c31), r.getX(), r.getY(), juce::Colour(0xff1c1e22), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, corner);
    g.setColour(juce::Colours::white.withAlpha(0.07f)); g.drawRoundedRectangle(r.reduced(0.5f), corner, 1.f);
    // Recessed instrument bay inside the frame.
    const auto bay = r.reduced(10.f);
    g.setGradientFill(juce::ColourGradient(kPlate.darker(0.12f), bay.getX(), bay.getY(), kPlate, bay.getX(), bay.getBottom(), false));
    g.fillRoundedRectangle(bay, corner - 3.f);
    {   // fine anodised grain, barely there, and a soft vignette
        juce::Graphics::ScopedSaveState ss(g);
        juce::Path clip; clip.addRoundedRectangle(bay, corner - 3.f); g.reduceClipRegion(clip);
        const auto& tex = plateTexture();
        g.setOpacity(0.07f);
        const int W = tex.getWidth(), H = tex.getHeight();
        for (int j = 0; bay.getY() + j * H < bay.getBottom(); ++j)
            for (int i = 0; bay.getX() + i * W < bay.getRight(); ++i) {   // mirrored tiles: no seams
                const float tx = bay.getX() + i * W, ty = bay.getY() + j * H;
                auto t = juce::AffineTransform::scale(i % 2 ? -1.f : 1.f, j % 2 ? -1.f : 1.f).translated(tx + (i % 2 ? W : 0), ty + (j % 2 ? H : 0));
                g.drawImageTransformed(tex, t, false);
            }
        g.setOpacity(1.f);
        g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack, bay.getCentreX(), bay.getCentreY(), juce::Colours::black.withAlpha(0.22f), bay.getX(), bay.getY(), true));
        g.fillRoundedRectangle(bay, corner - 3.f);     // vignette toward the corners
    }
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.45f), bay.getX(), bay.getY(), juce::Colours::transparentBlack, bay.getX(), bay.getY() + 10.f, false));
    g.fillRoundedRectangle(bay.withHeight(10.f), corner - 3.f);
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.drawRoundedRectangle(bay.reduced(0.5f), corner - 3.f, 1.f);
    // Mounting bolts in the frame corners.
    const float br = 3.2f, inset = 5.f;
    drawBolt(g, r.getX() + inset, r.getY() + inset, br); drawBolt(g, r.getRight() - inset, r.getY() + inset, br);
    drawBolt(g, r.getX() + inset, r.getBottom() - inset, br); drawBolt(g, r.getRight() - inset, r.getBottom() - inset, br);
}

void drawDisplay(juce::Graphics& g, juce::Rectangle<float> r, float corner, bool opaque) {
    if (opaque) g.setColour(kDisplayBg), g.fillRoundedRectangle(r, corner);
    // The solar limb rising from below the bottom edge (static image), unless a live sun is drawn underneath.
    if (opaque) {
        juce::Graphics::ScopedSaveState ss(g);
        juce::Path clip; clip.addRoundedRectangle(r, corner); g.reduceClipRegion(clip);
        if (auto limb = asset("solar_limb_png"); limb.isValid()) {
            const float s = r.getWidth() / limb.getWidth();
            const float imgH = limb.getHeight() * s;
            g.setOpacity(0.75f);
            g.drawImageTransformed(limb, juce::AffineTransform::scale(s).translated(r.getX(), r.getBottom() - imgH * 0.86f), false);
            g.setOpacity(1.f);
        } else {
            const float cx = r.getX() + r.getWidth() * 0.5f, cy = r.getBottom() + r.getWidth() * 0.55f, rad = r.getWidth() * 0.75f;
            g.setGradientFill(juce::ColourGradient(kAccent.withAlpha(0.16f), cx, cy, kAccent.withAlpha(0.f), cx, cy - rad, true));
            g.fillRect(r);
        }
    }
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.5f), r.getX(), r.getY(), juce::Colours::transparentBlack, r.getX(), r.getY() + 14.f, false));
    g.fillRoundedRectangle(r.withHeight(std::min(14.f, r.getHeight())), corner);
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawRoundedRectangle(r.reduced(0.5f), corner, 1.f);
    g.setColour(juce::Colours::white.withAlpha(0.035f)); g.drawRoundedRectangle(r.reduced(1.5f), corner - 1.f, 1.f);
    // Scope frame: bracket marks in the corners.
    const float L = 10.f, o = 7.f;
    g.setColour(kTextFaint.withAlpha(0.9f));
    for (int cx = 0; cx < 2; ++cx) for (int cy = 0; cy < 2; ++cy) {
        const float x = cx ? r.getRight() - o : r.getX() + o, y = cy ? r.getBottom() - o : r.getY() + o;
        g.drawLine(x, y, x + (cx ? -L : L), y, 1.f); g.drawLine(x, y, x, y + (cy ? -L : L), 1.f);
    }
}

void drawDivider(juce::Graphics& g, float x0, float y, float x1) {
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.drawLine(x0, y, x1, y, 1.f);
    g.setColour(juce::Colours::white.withAlpha(0.05f)); g.drawLine(x0, y + 1.f, x1, y + 1.f, 1.f);
}

void drawSection(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& code, const juce::String& title) {
    auto left = r.removeFromLeft(28.f);
    g.setFont(font(9.5f, 600)); g.setColour(kAccent.withAlpha(0.85f)); g.drawText(code, left, juce::Justification::centredLeft);
    const float tw = juce::GlyphArrangement::getStringWidth(caps(9.5f), title.toUpperCase()) + 10.f;
    drawCaps(g, title, r.removeFromLeft(tw), juce::Justification::centredLeft, 9.5f, kTextDim);
    g.setColour(kLine); g.drawLine(r.getX() + 4.f, r.getCentreY(), r.getRight(), r.getCentreY(), 1.f);
}

// ---- Dial body, cached per size ------------------------------------------------------------------
namespace {
const juce::Image& dialBody(int px) {
    auto& cache = res().dialCache;
    if (auto it = cache.find(px); it != cache.end()) return it->second;
    if (cache.size() > 12) cache.clear();
    const float pad = px * 0.18f, R = px * 0.5f - pad;              // pad leaves room for the shadow
    juce::Image im(juce::Image::ARGB, px, px, true);
    juce::Graphics g(im);
    const float cx = px * 0.5f, cy = px * 0.5f;
    juce::Path body; body.addEllipse(cx - R, cy - R, 2 * R, 2 * R);
    juce::DropShadow(juce::Colours::black.withAlpha(0.6f), (int)std::max(4.f, R * 0.45f), {0, (int)std::max(2.f, R * 0.18f)}).drawForPath(g, body);
    juce::DropShadow(juce::Colours::black.withAlpha(0.35f), (int)std::max(2.f, R * 0.12f), {0, (int)std::max(1.f, R * 0.05f)}).drawForPath(g, body);
    // Outer skirt: dark with a faint rim light at the top.
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2c2e33), cx, cy - R, juce::Colour(0xff141517), cx, cy + R, false));
    g.fillEllipse(cx - R, cy - R, 2 * R, 2 * R);
    g.setColour(juce::Colours::white.withAlpha(0.10f)); g.drawEllipse(cx - R + 0.75f, cy - R + 0.75f, 2 * R - 1.5f, 2 * R - 1.5f, 1.f);
    // Very fine knurl on the skirt: low-contrast radial lines.
    const int n = (int)std::clamp(R * 1.6f, 36.f, 96.f);
    for (int i = 0; i < n; ++i) {
        const float a = i * juce::MathConstants<float>::twoPi / n;
        g.setColour((i & 1) ? juce::Colours::white.withAlpha(0.035f) : juce::Colours::black.withAlpha(0.22f));
        g.drawLine(cx + std::sin(a) * R * 0.86f, cy - std::cos(a) * R * 0.86f, cx + std::sin(a) * R * 0.98f, cy - std::cos(a) * R * 0.98f, std::max(1.f, R * 0.035f));
    }
    // Cap: matte, lit from the top-left.
    const float rc = R * 0.80f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a3d43), cx - rc * 0.55f, cy - rc * 0.75f, juce::Colour(0xff1a1b1e), cx + rc * 0.4f, cy + rc * 0.9f, true));
    g.fillEllipse(cx - rc, cy - rc, 2 * rc, 2 * rc);
    g.setColour(juce::Colours::black.withAlpha(0.55f)); g.drawEllipse(cx - rc, cy - rc, 2 * rc, 2 * rc, std::max(1.f, R * 0.04f));
    g.setColour(juce::Colours::white.withAlpha(0.07f)); g.drawEllipse(cx - rc + 1.f, cy - rc + 1.f, 2 * rc - 2.f, 2 * rc - 2.f, 1.f);
    return cache[px] = im;
}
}  // namespace

UltraLookAndFeel::UltraLookAndFeel() {
    if (auto f = face(400)) setDefaultSansSerifTypeface(f);
    setColour(juce::ResizableWindow::backgroundColourId, kBg);
    setColour(juce::Label::textColourId, kText);
    setColour(juce::Slider::textBoxTextColourId, kText);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId, kAccent.withAlpha(0.35f));
    setColour(juce::ComboBox::textColourId, kText);
    setColour(juce::PopupMenu::backgroundColourId, kPlate);
    setColour(juce::PopupMenu::textColourId, kText);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, kAccent.withAlpha(0.18f));
    setColour(juce::PopupMenu::highlightedTextColourId, kText);
    setColour(juce::TextButton::textColourOffId, kText);
    setColour(juce::TextButton::textColourOnId, kText);
    setColour(juce::ToggleButton::textColourId, kText);
    setColour(juce::TextEditor::backgroundColourId, kDisplayBg);
    setColour(juce::TextEditor::textColourId, kText);
    setColour(juce::TextEditor::highlightColourId, kAccent.withAlpha(0.35f));
    setColour(juce::TextEditor::outlineColourId, kLine);
    setColour(juce::TextEditor::focusedOutlineColourId, kAccent);
    setColour(juce::CaretComponent::caretColourId, kAccent);
    setColour(juce::ListBox::backgroundColourId, kPlate);
    setColour(juce::AlertWindow::backgroundColourId, kPlate);
    setColour(juce::AlertWindow::textColourId, kText);
    setColour(juce::AlertWindow::outlineColourId, kLine);
    setColour(juce::ScrollBar::thumbColourId, kTextFaint);
}

void UltraLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s) {
    // The slider hands over everything above its readout; keep a clear band between dial and readout.
    const auto area = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).withTrimmedBottom((float)dialReadoutGap(s));
    const float D = std::min(area.getWidth(), area.getHeight());
    const float cx = area.getCentreX(), cy = area.getCentreY();
    const bool on = s.isEnabled();
    const float a = start + pos * (end - start);
    const float bass = on ? bassGlow.load(std::memory_order_relaxed) : 0.f;
    // Recessed well.
    const float rw = D * 0.5f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff0f1012), cx, cy - rw, juce::Colour(0xff1a1b1e), cx, cy + rw, false));
    g.fillEllipse(cx - rw, cy - rw, 2 * rw, 2 * rw);
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawEllipse(cx - rw, cy - rw, 2 * rw, 2 * rw, 1.f);
    g.setColour(juce::Colours::white.withAlpha(0.05f)); g.drawEllipse(cx - rw + 1.f, cy - rw + 1.f, 2 * rw - 2.f, 2 * rw - 2.f, 1.f);
    // Bass glow: the ring around the well lights up and swells inward with low-frequency energy.
    if (bass > 0.01f) {
        const float gw = 2.f + 6.f * bass;
        g.setColour(kAccent.withAlpha(0.12f * bass)); g.drawEllipse(cx - rw + gw, cy - rw + gw, 2 * (rw - gw), 2 * (rw - gw), gw * 2.f);
        g.setColour(kAccent.withAlpha(0.30f * bass)); g.drawEllipse(cx - rw + gw * 0.5f, cy - rw + gw * 0.5f, 2 * (rw - gw * 0.5f), 2 * (rw - gw * 0.5f), gw);
        g.setColour(kAccent.withAlpha(0.75f * bass)); g.drawEllipse(cx - rw + 0.6f, cy - rw + 0.6f, 2 * rw - 1.2f, 2 * rw - 1.2f, 1.2f);
    }
    // Ring scale: tick marks around the well edge.
    const int ticks = 21;
    for (int i = 0; i < ticks; ++i) {
        const float ta = start + (end - start) * i / (ticks - 1);
        const bool major = (i % 5) == 0;
        const float r0 = rw - (major ? 6.f : 3.5f), r1 = rw - 1.f;
        g.setColour(on ? (major ? kTextDim : kTextFaint).interpolatedWith(kAccent, 0.7f * bass) : kTextFaint.withAlpha(0.5f));
        g.drawLine(cx + std::sin(ta) * r0, cy - std::cos(ta) * r0, cx + std::sin(ta) * r1, cy - std::cos(ta) * r1, major ? 1.4f : 1.f);
    }
    // Value arc just inside the ticks.
    const float rArc = rw - 8.5f, tw = std::max(1.5f, D * 0.022f);
    juce::Path track; track.addCentredArc(cx, cy, rArc, rArc, 0.f, start, end, true);
    g.setColour(kLine); g.strokePath(track, juce::PathStrokeType(tw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (on) {
        // Bipolar controls (pan, envelope amount, mod amounts) light the arc from the centre outward.
        const bool bipolar = (bool)s.getProperties().getWithDefault("bipolar", false);
        const float from = bipolar ? 0.5f * (start + end) : start;
        juce::Path val; val.addCentredArc(cx, cy, rArc, rArc, 0.f, std::min(from, a), std::max(from, a), true);
        g.setColour(kAccent); g.strokePath(val, juce::PathStrokeType(tw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(kAccent.withAlpha(0.25f + 0.45f * bass)); g.strokePath(val, juce::PathStrokeType(tw * (3.f + 2.f * bass), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    // Dial body (cached), sized to sit inside the arc.
    const float Dk = (rArc - 3.5f) * 2.f;
    const int px = std::max(16, (int)std::round(Dk / 0.64f));
    const auto& img = dialBody(px);
    const float scale = Dk / (px * 0.64f);
    g.setOpacity(on ? 1.f : 0.5f);
    g.drawImageTransformed(img, juce::AffineTransform::scale(scale).translated(cx - px * 0.5f * scale, cy - px * 0.5f * scale), false);
    g.setOpacity(1.f);
    // Pointer.
    const float R = Dk * 0.5f, p0 = R * 0.45f, p1 = R * 0.92f, pw = std::max(2.f, Dk * 0.045f);
    const juce::Point<float> q0(cx + std::sin(a) * p0, cy - std::cos(a) * p0), q1(cx + std::sin(a) * p1, cy - std::cos(a) * p1);
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.drawLine({q0.translated(0, 1.f), q1.translated(0, 1.f)}, pw);
    g.setColour(on ? kText : kTextFaint); g.drawLine({q0, q1}, pw);
    if (s.hasKeyboardFocus(false)) { g.setColour(kAccent.withAlpha(0.8f)); g.drawEllipse(cx - rw - 2, cy - rw - 2, 2 * rw + 4, 2 * rw + 4, 1.f); }
}

void UltraLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle style, juce::Slider& s) {
    if (style != juce::Slider::LinearHorizontal) { LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, pos, 0, 0, style, s); return; }
    // A recessed slot with a lit bar; bipolar sliders fill from the centre.
    const auto r = juce::Rectangle<float>((float)x, (float)y + h * 0.5f - 3.f, (float)w, 6.f);
    g.setColour(kDisplayBg); g.fillRoundedRectangle(r, 3.f);
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawRoundedRectangle(r, 3.f, 1.f);
    const bool bipolar = (bool)s.getProperties().getWithDefault("bipolar", false);
    const float mid = bipolar ? r.getCentreX() : r.getX();
    const float px = juce::jlimit(r.getX(), r.getRight(), pos);
    if (s.isEnabled() && std::abs(px - mid) > 0.5f) {
        const auto fill = juce::Rectangle<float>(std::min(mid, px), r.getY() + 1.f, std::abs(px - mid), r.getHeight() - 2.f);
        g.setColour(kAccent.withAlpha(0.3f)); g.fillRoundedRectangle(fill.expanded(0.f, 2.f), 3.f);
        g.setColour(kAccent); g.fillRoundedRectangle(fill, 2.f);
    }
    if (bipolar) { g.setColour(kTextFaint); g.fillRect(r.getCentreX() - 0.5f, r.getY() - 3.f, 1.f, r.getHeight() + 6.f); }
    g.setColour(s.isEnabled() ? kText : kTextFaint); g.fillRoundedRectangle(px - 2.f, r.getY() - 4.f, 4.f, r.getHeight() + 8.f, 2.f);
}

juce::Label* UltraLookAndFeel::createSliderTextBox(juce::Slider& s) {
    // Rotary dials read out on a red seven-segment LED module; linear sliders keep a plain readout.
    juce::Label* l = s.isRotary() ? (juce::Label*)new seg::ValueLabel() : LookAndFeel_V4::createSliderTextBox(s);
    l->setJustificationType(juce::Justification::centred);
    l->setFont(font(13.f, 500));
    l->setColour(juce::Label::outlineWhenEditingColourId, kAccent);
    return l;
}

void UltraLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& l) {
    const bool sliderBox = dynamic_cast<juce::Slider*>(l.getParentComponent()) != nullptr;
    if (sliderBox) {
        // Inset readout window.
        const auto r = l.getLocalBounds().toFloat().reduced(1.f);
        g.setColour(kDisplayBg); g.fillRoundedRectangle(r, 3.f);
        g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.5f), r.getX(), r.getY(), juce::Colours::transparentBlack, r.getX(), r.getY() + 6.f, false));
        g.fillRoundedRectangle(r.withHeight(6.f), 3.f);
        g.setColour(juce::Colours::white.withAlpha(0.07f)); g.drawRoundedRectangle(r, 3.f, 1.f);
    }
    if (l.isBeingEdited()) return;
    g.setFont(sliderBox ? font(std::clamp(l.getHeight() * 0.62f, 11.f, 14.f), 500) : l.getFont());
    g.setColour((sliderBox ? kAccent.brighter(0.25f) : l.findColour(juce::Label::textColourId)).withMultipliedAlpha(l.isEnabled() ? 1.f : 0.35f));
    g.drawText(l.getText(), l.getLocalBounds().reduced(2, 0), l.getJustificationType(), true);
}

void UltraLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool over, bool) {
    // A two-way lever switch (as on EXO): threaded bezel, chrome bat handle thrown left (off) or right (on), label, lamp.
    const auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();
    const float R = std::min(10.f, r.getHeight() * 0.36f);
    const float cx = r.getX() + R + 3.f, cy = r.getCentreY() + 1.f;
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillEllipse(cx - R - 1.f, cy - R + 0.5f, 2 * R + 2.f, 2 * R + 2.f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff45484f), cx, cy - R, juce::Colour(0xff1c1e22), cx, cy + R, false));
    g.fillEllipse(cx - R, cy - R, 2 * R, 2 * R);
    g.setColour(juce::Colours::white.withAlpha(over ? 0.14f : 0.08f)); g.drawEllipse(cx - R + 0.5f, cy - R + 0.5f, 2 * R - 1.f, 2 * R - 1.f, 1.f);
    const float Ri = R * 0.62f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff050506), cx, cy - Ri, juce::Colour(0xff17181b), cx, cy + Ri, false));
    g.fillEllipse(cx - Ri, cy - Ri, 2 * Ri, 2 * Ri);
    const float lean = on ? 1.f : -1.f;
    const float len = R * 1.55f, ang = lean * 0.95f;
    const juce::Point<float> base(cx, cy), tip(cx + std::sin(ang) * len, cy - std::cos(ang) * len);
    const juce::Point<float> nrm(std::cos(ang), std::sin(ang));
    const float wb = R * 0.42f, wt = R * 0.30f;
    juce::Path rod;
    rod.startNewSubPath(base + nrm * wb); rod.lineTo(tip + nrm * wt); rod.lineTo(tip - nrm * wt); rod.lineTo(base - nrm * wb); rod.closeSubPath();
    g.setColour(juce::Colours::black.withAlpha(0.45f)); g.fillPath(rod, juce::AffineTransform::translation(1.f, 1.5f));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe2e3e6), base.x - nrm.x * wb, base.y - nrm.y * wb, juce::Colour(0xff5a5d64), base.x + nrm.x * wb, base.y + nrm.y * wb, false));
    g.fillPath(rod);
    const float ball = R * 0.40f;
    g.setColour(juce::Colours::black.withAlpha(0.45f)); g.fillEllipse(tip.x - ball + 1.f, tip.y - ball + 1.5f, 2 * ball, 2 * ball);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff2f2f4), tip.x - ball * 0.4f, tip.y - ball * 0.5f, juce::Colour(0xff6a6d74), tip.x + ball * 0.6f, tip.y + ball * 0.7f, true));
    g.fillEllipse(tip.x - ball, tip.y - ball, 2 * ball, 2 * ball);
    drawCaps(g, b.getButtonText(), r.withTrimmedLeft(2 * R + 16.f).withTrimmedRight(16.f), juce::Justification::centredLeft, std::clamp(r.getHeight() * 0.42f, 10.f, 11.5f), b.isEnabled() ? kTextDim : kTextFaint);
    const float lr = 3.5f, lx = r.getRight() - lr - 2.f, ly = r.getCentreY();
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.fillEllipse(lx - lr - 1.f, ly - lr - 1.f, 2 * lr + 2.f, 2 * lr + 2.f);
    if (on) { g.setColour(kAccent.withAlpha(0.35f)); g.fillEllipse(lx - lr * 2.2f, ly - lr * 2.2f, lr * 4.4f, lr * 4.4f); }
    g.setColour(on ? kAccent.brighter(0.3f) : juce::Colour(0xff3a3c41)); g.fillEllipse(lx - lr, ly - lr, 2 * lr, 2 * lr);
    g.setColour(juce::Colours::white.withAlpha(on ? 0.6f : 0.15f)); g.fillEllipse(lx - lr * 0.5f, ly - lr * 0.6f, lr * 0.6f, lr * 0.5f);
    if (b.hasKeyboardFocus(false)) { g.setColour(kAccent.withAlpha(0.7f)); g.drawEllipse(cx - R - 3.f, cy - R - 3.f, 2 * R + 6.f, 2 * R + 6.f, 1.f); }
}

void UltraLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) {
    const auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    const bool lit = b.getClickingTogglesState() && b.getToggleState();      // a latching switch that is on
    g.setColour(lit ? juce::Colour(0xff2a2140) : down ? juce::Colour(0xff26282d) : over ? juce::Colour(0xff222429) : juce::Colour(0xff1d1f23));
    g.fillRoundedRectangle(r, 6.f);
    if (lit) { g.setColour(kAccent.withAlpha(0.55f)); g.drawRoundedRectangle(r, 6.f, 1.f); return; }
    g.setColour(b.hasKeyboardFocus(false) ? kAccent.withAlpha(0.8f) : juce::Colours::white.withAlpha(over ? 0.14f : 0.09f));
    g.drawRoundedRectangle(r, 6.f, 1.f);
}

void UltraLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) {
    const bool accent = (bool)b.getProperties().getWithDefault("accent", false) || (b.getClickingTogglesState() && b.getToggleState());   // e.g. an update is waiting
    drawCaps(g, b.getButtonText(), b.getLocalBounds().toFloat().reduced(4.f, 0.f), juce::Justification::centred,
             std::clamp(b.getHeight() * 0.40f, 10.f, 11.5f), !b.isEnabled() ? kTextFaint : accent ? kAccent.brighter(0.2f) : kText);
}

juce::Font UltraLookAndFeel::getTextButtonFont(juce::TextButton&, int h) { return caps(std::clamp(h * 0.4f, 10.f, 11.5f)); }

void UltraLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) {
    const auto r = juce::Rectangle<float>(0, 0, (float)w, (float)h).reduced(0.5f);
    g.setColour(kDisplayBg); g.fillRoundedRectangle(r, 6.f);
    g.setColour(box.hasKeyboardFocus(false) ? kAccent.withAlpha(0.8f) : juce::Colours::white.withAlpha(0.09f)); g.drawRoundedRectangle(r, 6.f, 1.f);
    const float ch = 4.f, cxp = r.getRight() - 14.f, cyp = r.getCentreY();
    juce::Path chev; chev.startNewSubPath(cxp - ch, cyp - ch * 0.5f); chev.lineTo(cxp, cyp + ch * 0.5f); chev.lineTo(cxp + ch, cyp - ch * 0.5f);
    g.setColour(box.isEnabled() ? kTextDim : kTextFaint); g.strokePath(chev, juce::PathStrokeType(1.5f));
    g.setFont(font(13.f, 500)); g.setColour(box.isEnabled() ? kText : kTextFaint);
    g.drawText(box.getText(), r.reduced(12.f, 0.f).withTrimmedRight(16.f), juce::Justification::centredLeft, true);
}

void UltraLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& l) { l.setBounds(0, 0, 0, 0); l.setFont(getComboBoxFont(box)); }
juce::Font UltraLookAndFeel::getComboBoxFont(juce::ComboBox&) { return font(13.f, 500); }

void UltraLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int w, int h) {
    g.fillAll(kPlate);
    g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawRect(0, 0, w, h, 1);
}

void UltraLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool sep, bool active, bool highlighted, bool ticked, bool,
                                          const juce::String& text, const juce::String&, const juce::Drawable*, const juce::Colour*) {
    if (sep) { g.setColour(kLine); g.fillRect(area.reduced(8, 0).withHeight(1).withY(area.getCentreY())); return; }
    if (highlighted && active) { g.setColour(juce::Colours::white.withAlpha(0.06f)); g.fillRect(area.reduced(4, 1)); }
    g.setFont(getPopupMenuFont());
    g.setColour(active ? (ticked ? kAccent : kText) : kTextFaint);
    g.drawText(text, area.reduced(14, 0), juce::Justification::centredLeft, true);
}

juce::Font UltraLookAndFeel::getPopupMenuFont() { return font(13.f, 400); }

void UltraLookAndFeel::drawCallOutBoxBackground(juce::CallOutBox&, juce::Graphics& g, const juce::Path& path, juce::Image&) {
    juce::DropShadow(juce::Colours::black.withAlpha(0.6f), 16, {0, 6}).drawForPath(g, path);
    g.setColour(kPlate); g.fillPath(path);
    g.setColour(juce::Colours::white.withAlpha(0.08f)); g.strokePath(path, juce::PathStrokeType(1.f));
}

juce::Font UltraLookAndFeel::getAlertWindowMessageFont() { return font(14.f); }
juce::Font UltraLookAndFeel::getAlertWindowFont() { return font(13.f); }
juce::Font UltraLookAndFeel::getAlertWindowTitleFont() { return font(16.f, 600); }

}  // namespace look
