#include "RetroLook.h"
#include <cmath>
#include <map>

namespace retro {

const juce::Colour kPanel(0xff24272b), kPanelLight(0xff34383d), kPanelDark(0xff15171a),
    kSilk(0xffd8d1bf), kSilkDim(0xff8f8a7c), kEngrave(0xff0c0d0f),
    kVfd(0xff7df9ff), kVfdDim(0xff1a4a52), kVfdGlass(0xff040d12), kVfdGlassTop(0xff0b1c24), kVfdBezel(0xff0a0b0d),
    kAmber(0xffffb340), kRed(0xffff4d4d), kIvory(0xfff1e9d6), kChrome(0xffd6d9dd), kChromeDark(0xff43474d);

namespace {
constexpr float kPi = juce::MathConstants<float>::pi;

juce::Font silkFont(float h, bool bold) {
    return juce::Font(juce::FontOptions(h, bold ? juce::Font::bold : juce::Font::plain)).withExtraKerningFactor(0.10f);
}

// 14-segment masks. Bits: a b c d e f g1 g2 h i j k l m (see drawSegText for the geometry).
enum : unsigned { A = 1, B = 2, C = 4, D = 8, E = 16, F = 32, G1 = 64, G2 = 128, H = 256, I = 512, J = 1024, K = 2048, L = 4096, M = 8192 };
unsigned segMask(juce::juce_wchar ch) {
    static const std::map<juce::juce_wchar, unsigned> table = {
        {'0', A | B | C | D | E | F | J | K}, {'1', B | C}, {'2', A | B | G1 | G2 | E | D}, {'3', A | B | C | D | G2},
        {'4', B | C | F | G1 | G2}, {'5', A | F | G1 | G2 | C | D}, {'6', A | F | G1 | G2 | C | D | E}, {'7', A | B | C},
        {'8', A | B | C | D | E | F | G1 | G2}, {'9', A | B | C | D | F | G1 | G2},
        {'A', A | B | C | E | F | G1 | G2}, {'B', A | B | C | D | I | L | G2}, {'C', A | D | E | F}, {'D', A | B | C | D | I | L},
        {'E', A | D | E | F | G1 | G2}, {'F', A | E | F | G1}, {'G', A | C | D | E | F | G2}, {'H', B | C | E | F | G1 | G2},
        {'I', A | D | I | L}, {'J', B | C | D | E}, {'K', E | F | G1 | J | M}, {'L', D | E | F}, {'M', B | C | E | F | H | J},
        {'N', B | C | E | F | H | M}, {'O', A | B | C | D | E | F}, {'P', A | B | E | F | G1 | G2}, {'Q', A | B | C | D | E | F | M},
        {'R', A | B | E | F | G1 | G2 | M}, {'S', A | F | G1 | G2 | C | D}, {'T', A | I | L}, {'U', B | C | D | E | F},
        {'V', E | F | K | J}, {'W', B | C | E | F | K | M}, {'X', H | J | K | M}, {'Y', H | J | L}, {'Z', A | D | J | K},
        {'-', G1 | G2}, {'/', J | K}, {'\\', H | M}, {'(', J | M}, {')', H | K}, {'+', G1 | G2 | I | L}, {'*', H | I | J | K | L | M | G1 | G2},
        {'%', F | C | J | K}, {'\'', I}, {',', K}, {'_', D}, {'=', D | G1 | G2}, {'>', H | K}, {'<', J | M}, {'x', H | J | K | M},
        {' ', 0}};
    auto it = table.find(ch);
    if (it == table.end()) it = table.find(juce::CharacterFunctions::toUpperCase(ch));
    return it == table.end() ? 0u : it->second;
}

const juce::Image& dotGridTile() {
    static juce::Image tile = [] {
        juce::Image im(juce::Image::ARGB, 24, 24, true);
        juce::Graphics g(im);
        for (int y = 1; y < 24; y += 3) for (int x = 1; x < 24; x += 3) {
            g.setColour(juce::Colours::white.withAlpha(0.045f));
            g.fillRect(x, y, 1, 1);
        }
        return im;
    }();
    return tile;
}
}  // namespace

// ---- Panel -------------------------------------------------------------------------------------
const juce::Image& panelTexture(int w, int h) {
    static std::map<std::pair<int, int>, juce::Image> cache;
    auto key = std::make_pair(w, h);
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    if (cache.size() > 6) cache.clear();
    juce::Image im(juce::Image::RGB, std::max(1, w), std::max(1, h), false);
    juce::Graphics g(im);
    g.setGradientFill(juce::ColourGradient(kPanelLight, 0.f, 0.f, kPanel, 0.f, (float)h, false));
    g.fillAll();
    juce::Random rnd(1234);
    for (int y = 0; y < h; ++y) {                    // horizontal brushing
        const float a = 0.02f + rnd.nextFloat() * 0.07f;
        g.setColour((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha(a));
        g.drawHorizontalLine(y, 0.f, (float)w);
    }
    for (int i = 0; i < w * h / 900; ++i) {         // sparse grain
        g.setColour(juce::Colours::white.withAlpha(rnd.nextFloat() * 0.05f));
        g.fillRect(rnd.nextInt(w), rnd.nextInt(h), 1 + rnd.nextInt(3), 1);
    }
    juce::ColourGradient vig(juce::Colours::transparentBlack, w * 0.5f, h * 0.45f, juce::Colours::black.withAlpha(0.55f), 0.f, 0.f, true);
    g.setGradientFill(vig); g.fillAll();
    return cache[key] = im;
}

void drawScrew(juce::Graphics& g, float cx, float cy, float r, float slotAngle) {
    juce::Path shadow; shadow.addEllipse(cx - r, cy - r, 2 * r, 2 * r);
    juce::DropShadow(juce::Colours::black.withAlpha(0.6f), (int)(r * 0.6f), {0, (int)(r * 0.3f)}).drawForPath(g, shadow);
    g.setGradientFill(juce::ColourGradient(kChrome, cx - r * 0.6f, cy - r * 0.7f, kChromeDark, cx + r * 0.6f, cy + r * 0.8f, false));
    g.fillEllipse(cx - r, cy - r, 2 * r, 2 * r);
    g.setColour(kPanelDark.withAlpha(0.8f)); g.drawEllipse(cx - r, cy - r, 2 * r, 2 * r, 1.f);
    juce::Path slot; slot.addRoundedRectangle(-r * 0.78f, -r * 0.13f, r * 1.56f, r * 0.26f, r * 0.1f);
    g.setColour(kPanelDark); g.fillPath(slot, juce::AffineTransform::rotation(slotAngle).translated(cx, cy));
    g.setColour(juce::Colours::white.withAlpha(0.25f));
    g.fillPath(slot, juce::AffineTransform::rotation(slotAngle).translated(cx + 0.6f, cy + 0.6f));
}

void drawSilk(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just, float fontHeight,
              juce::Colour colour, bool bold) {
    g.setFont(silkFont(fontHeight, bold));
    g.setColour(kEngrave.withAlpha(0.7f)); g.drawText(text, area.translated(0.f, 1.f), just);
    g.setColour(colour); g.drawText(text, area, just);
}

// ---- VFD ---------------------------------------------------------------------------------------
juce::Rectangle<float> drawVfdGlass(juce::Graphics& g, juce::Rectangle<float> r, float corner, float bezel, float pad) {
    juce::Path outer; outer.addRoundedRectangle(r, corner);
    juce::DropShadow(juce::Colours::black.withAlpha(0.5f), 6, {0, 2}).drawForPath(g, outer);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1a1c20), r.getX(), r.getY(), kVfdBezel, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, corner);
    const auto inner = r.reduced(bezel);
    g.setGradientFill(juce::ColourGradient(kVfdGlassTop, inner.getX(), inner.getY(), kVfdGlass, inner.getX(), inner.getBottom(), false));
    g.fillRoundedRectangle(inner, corner - 1.5f);
    g.setTiledImageFill(dotGridTile(), (int)inner.getX(), (int)inner.getY(), 1.f);
    g.fillRoundedRectangle(inner, corner - 1.5f);
    // Inner shadow at the top edge and a soft reflection band.
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.5f), inner.getX(), inner.getY(), juce::Colours::transparentBlack, inner.getX(), inner.getY() + 10.f, false));
    g.fillRect(inner.withHeight(std::min(10.f, inner.getHeight() * 0.3f)));
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.05f), inner.getX(), inner.getY(), juce::Colours::transparentWhite, inner.getX(), inner.getY() + inner.getHeight() * 0.45f, false));
    g.fillRoundedRectangle(inner.withHeight(inner.getHeight() * 0.45f), corner - 1.5f);
    g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawRoundedRectangle(inner, corner - 1.5f, 1.f);
    return inner.reduced(pad);
}

void glowPath(juce::Graphics& g, const juce::Path& p, juce::Colour c, float width, float glowScale) {
    const juce::PathStrokeType::JointStyle j = juce::PathStrokeType::curved;
    const juce::PathStrokeType::EndCapStyle e = juce::PathStrokeType::rounded;
    g.setColour(c.withAlpha(0.10f)); g.strokePath(p, juce::PathStrokeType(width * 4.5f * glowScale, j, e));
    g.setColour(c.withAlpha(0.25f)); g.strokePath(p, juce::PathStrokeType(width * 2.2f * glowScale, j, e));
    g.setColour(c); g.strokePath(p, juce::PathStrokeType(width, j, e));
    g.setColour(juce::Colours::white.withAlpha(0.35f)); g.strokePath(p, juce::PathStrokeType(width * 0.35f, j, e));
}

void glowDot(juce::Graphics& g, float x, float y, float r, juce::Colour c) {
    g.setColour(c.withAlpha(0.15f)); g.fillEllipse(x - r * 3, y - r * 3, r * 6, r * 6);
    g.setColour(c.withAlpha(0.4f)); g.fillEllipse(x - r * 1.8f, y - r * 1.8f, r * 3.6f, r * 3.6f);
    g.setColour(c); g.fillEllipse(x - r, y - r, 2 * r, 2 * r);
}

float segCellWidth(float h) { return h * 0.62f; }

float drawSegText(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just,
                  juce::Colour c, bool ghost, float maxCellHeight) {
    // Count cells: '.' and ',' after a character ride on that cell.
    juce::Array<std::pair<unsigned, bool>> cells;        // mask, decimal point
    for (int i = 0; i < text.length(); ++i) {
        const auto ch = text[i];
        if ((ch == '.' || ch == ',') && !cells.isEmpty() && !cells.getReference(cells.size() - 1).second) { cells.getReference(cells.size() - 1).second = true; continue; }
        cells.add({segMask(ch), false});
    }
    if (cells.isEmpty()) return 0.f;
    const int n = cells.size();
    float cellH = std::min({area.getHeight(), maxCellHeight, area.getWidth() / (n * 0.62f * 1.42f)});
    const float cellW = segCellWidth(cellH), pitch = cellW * 1.42f, total = pitch * n - (pitch - cellW);
    float x = area.getX();
    if (just.testFlags(juce::Justification::horizontallyCentred)) x = area.getCentreX() - total * 0.5f;
    else if (just.testFlags(juce::Justification::right)) x = area.getRight() - total;
    const float y = area.getCentreY() - cellH * 0.5f;
    const float t = std::max(1.2f, cellH * 0.09f), gap = t * 0.55f, pad = t * 0.7f;
    // Segment endpoints in cell space.
    auto seg = [&](int which, float cx) -> juce::Line<float> {
        const float x0 = cx + pad, x1 = cx + cellW - pad, xm = cx + cellW * 0.5f, y0 = y + pad, y1 = y + cellH - pad, ym = y + cellH * 0.5f;
        switch (which) {
            case 0: return {x0, y0, x1, y0}; case 1: return {x1, y0, x1, ym}; case 2: return {x1, ym, x1, y1}; case 3: return {x0, y1, x1, y1};
            case 4: return {x0, ym, x0, y1}; case 5: return {x0, y0, x0, ym}; case 6: return {x0, ym, xm, ym}; case 7: return {xm, ym, x1, ym};
            case 8: return {x0, y0, xm, ym}; case 9: return {xm, y0, xm, ym}; case 10: return {x1, y0, xm, ym}; case 11: return {x0, y1, xm, ym};
            case 12: return {xm, ym, xm, y1}; default: return {x1, y1, xm, ym};
        }
    };
    auto drawSet = [&](bool lit, float alpha, float width) {
        juce::Path p;
        float cx = x;
        for (int i = 0; i < n; ++i, cx += pitch) {
            const unsigned m = cells[i].first;
            for (int s = 0; s < 14; ++s) {
                const bool on = (m >> s) & 1u;
                if (on != lit) continue;
                auto l = seg(s, cx);
                const auto d = (l.getEnd() - l.getStart()); const float len = d.getDistanceFromOrigin();
                if (len <= 2 * gap) continue;
                const auto u = d / len;
                p.addLineSegment({l.getStart() + u * gap, l.getEnd() - u * gap}, width);
            }
            if (cells[i].second == lit || (lit && cells[i].second)) {
                if (cells[i].second && lit) { const float d = t * 2.1f; p.addEllipse(cx + cellW + (pitch - cellW - d) * 0.5f, y + cellH - pad - d * 0.55f, d, d); }
                else if (!lit && !cells[i].second) {}
            }
        }
        g.setColour(c.withAlpha(alpha)); g.fillPath(p);
    };
    if (ghost) drawSet(false, 0.07f, t);
    drawSet(true, 0.12f, t * 3.2f);
    drawSet(true, 0.30f, t * 1.9f);
    drawSet(true, 1.0f, t);
    drawSet(true, 0.45f, t * 0.45f);   // hot core: slightly whiter centre
    return total;
}

void drawVfdBar(juce::Graphics& g, juce::Rectangle<float> r, float level01, float peak01, int segments) {
    const bool vertical = r.getHeight() > r.getWidth();
    const float len = vertical ? r.getHeight() : r.getWidth();
    const float pitch = len / segments, segLen = pitch * 0.68f;
    const int lit = (int)std::round(juce::jlimit(0.f, 1.f, level01) * segments);
    const int peakSeg = peak01 < 0 ? -1 : (int)std::round(juce::jlimit(0.f, 1.f, peak01) * segments) - 1;
    for (int i = 0; i < segments; ++i) {
        const float frac = (i + 1) / (float)segments;
        const juce::Colour col = frac > 0.92f ? kRed : frac > 0.75f ? kAmber : kVfd;
        juce::Rectangle<float> s = vertical ? juce::Rectangle<float>(r.getX(), r.getBottom() - pitch * (i + 1) + (pitch - segLen) * 0.5f, r.getWidth(), segLen)
                                            : juce::Rectangle<float>(r.getX() + pitch * i + (pitch - segLen) * 0.5f, r.getY(), segLen, r.getHeight());
        const bool on = i < lit || i == peakSeg;
        if (on) { g.setColour(col.withAlpha(0.22f)); g.fillRoundedRectangle(s.expanded(2.5f), 2.f); }
        g.setColour(on ? col : col.withAlpha(0.08f));
        g.fillRoundedRectangle(s, 1.f);
    }
}

// ---- Buttons -----------------------------------------------------------------------------------
void drawMetalButton(juce::Graphics& g, juce::Rectangle<float> r, bool down, bool over, bool focused) {
    juce::Path p; p.addRoundedRectangle(r, 4.f);
    if (!down) juce::DropShadow(juce::Colours::black.withAlpha(0.55f), 5, {0, 2}).drawForPath(g, p);
    const juce::Colour top = down ? juce::Colour(0xff3a3e44) : over ? juce::Colour(0xff7a7f87) : juce::Colour(0xff6a6f77);
    const juce::Colour bottom = down ? juce::Colour(0xff2a2d31) : juce::Colour(0xff3d4147);
    g.setGradientFill(juce::ColourGradient(top, r.getX(), r.getY(), bottom, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 4.f);
    g.setColour(juce::Colours::white.withAlpha(down ? 0.05f : 0.18f)); g.drawLine(r.getX() + 3, r.getY() + 1.f, r.getRight() - 3, r.getY() + 1.f, 1.f);
    g.setColour(focused ? kVfd.withAlpha(0.9f) : kPanelDark); g.drawRoundedRectangle(r, 4.f, 1.f);
}

void drawLed(juce::Graphics& g, float cx, float cy, float r, juce::Colour c, bool on) {
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.fillEllipse(cx - r - 1.5f, cy - r - 1.5f, 2 * r + 3, 2 * r + 3);
    if (on) { g.setColour(c.withAlpha(0.35f)); g.fillEllipse(cx - r * 2.4f, cy - r * 2.4f, r * 4.8f, r * 4.8f); }
    g.setGradientFill(juce::ColourGradient(on ? c.brighter(0.6f) : c.darker(1.2f), cx - r * 0.4f, cy - r * 0.4f, on ? c : c.darker(2.0f), cx + r, cy + r, true));
    g.fillEllipse(cx - r, cy - r, 2 * r, 2 * r);
    g.setColour(juce::Colours::white.withAlpha(on ? 0.7f : 0.25f)); g.fillEllipse(cx - r * 0.45f, cy - r * 0.55f, r * 0.5f, r * 0.4f);
}

// ---- LookAndFeel -------------------------------------------------------------------------------
RetroLookAndFeel::RetroLookAndFeel() {
    setColour(juce::ResizableWindow::backgroundColourId, kPanel);
    setColour(juce::Label::textColourId, kSilk);
    setColour(juce::Slider::textBoxTextColourId, kVfd);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId, kVfd.withAlpha(0.3f));
    setColour(juce::ComboBox::textColourId, kVfd);
    setColour(juce::PopupMenu::backgroundColourId, kVfdGlass);
    setColour(juce::PopupMenu::textColourId, kVfd);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, kVfd.withAlpha(0.18f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::TextButton::textColourOffId, kIvory);
    setColour(juce::TextButton::textColourOnId, kIvory);
    setColour(juce::ToggleButton::textColourId, kSilk);
    setColour(juce::TextEditor::backgroundColourId, kVfdGlass);
    setColour(juce::TextEditor::textColourId, kVfd);
    setColour(juce::TextEditor::highlightColourId, kVfd.withAlpha(0.3f));
    setColour(juce::TextEditor::outlineColourId, kVfdDim);
    setColour(juce::TextEditor::focusedOutlineColourId, kVfd);
    setColour(juce::CaretComponent::caretColourId, kVfd);
    setColour(juce::ListBox::backgroundColourId, kVfdGlass);
    setColour(juce::AlertWindow::backgroundColourId, kPanel);
    setColour(juce::AlertWindow::textColourId, kSilk);
    setColour(juce::AlertWindow::outlineColourId, kPanelDark);
    setColour(juce::ScrollBar::thumbColourId, kVfdDim);
}

void RetroLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s) {
    const auto area = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h);
    const float R = std::min(area.getWidth(), area.getHeight()) * 0.5f;      // outer radius incl. scale
    const float cx = area.getCentreX(), cy = area.getCentreY();
    const float rk = R * 0.70f;                                                // knob radius
    const bool on = s.isEnabled();
    const float a = start + pos * (end - start);
    // Engraved tick scale.
    const int ticks = 11;
    for (int i = 0; i < ticks; ++i) {
        const float ta = start + (end - start) * i / (ticks - 1);
        const bool major = i == 0 || i == ticks - 1 || i == ticks / 2;
        const float r0 = rk + R * 0.08f, r1 = r0 + R * (major ? 0.13f : 0.08f);
        const juce::Point<float> p0(cx + std::sin(ta) * r0, cy - std::cos(ta) * r0), p1(cx + std::sin(ta) * r1, cy - std::cos(ta) * r1);
        g.setColour(kEngrave.withAlpha(0.8f)); g.drawLine({p0.translated(0, 1), p1.translated(0, 1)}, major ? 2.f : 1.2f);
        g.setColour(on ? kSilk.withAlpha(0.85f) : kSilkDim.withAlpha(0.5f)); g.drawLine({p0, p1}, major ? 2.f : 1.2f);
    }
    // Value arc in VFD light.
    if (on) {
        juce::Path arc; const float ra = rk + R * 0.055f;
        arc.addCentredArc(cx, cy, ra, ra, 0.f, start, a, true);
        glowPath(g, arc, kVfd, std::max(1.5f, R * 0.035f), 0.8f);
    }
    // Shadow and chrome bezel.
    juce::Path body; body.addEllipse(cx - rk, cy - rk, 2 * rk, 2 * rk);
    juce::DropShadow(juce::Colours::black.withAlpha(0.65f), (int)std::max(3.f, rk * 0.35f), {0, (int)std::max(1.f, rk * 0.15f)}).drawForPath(g, body);
    g.setGradientFill(juce::ColourGradient(kChrome, cx - rk, cy - rk, kChromeDark, cx + rk, cy + rk, false));
    g.fillEllipse(cx - rk, cy - rk, 2 * rk, 2 * rk);
    const float rb = rk * 0.86f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2b2e33), cx - rb, cy + rb, juce::Colour(0xff0f1012), cx + rb, cy - rb, false));
    g.fillEllipse(cx - rb, cy - rb, 2 * rb, 2 * rb);
    // Knurled grip: alternating light/dark radial marks around the rim.
    const int knurl = 40;
    for (int i = 0; i < knurl; ++i) {
        const float ka = i * 2 * kPi / knurl;
        const float r0 = rb * 0.80f, r1 = rb * 0.97f;
        g.setColour((i % 2 ? juce::Colours::white.withAlpha(0.10f) : juce::Colours::black.withAlpha(0.45f)));
        g.drawLine(cx + std::sin(ka) * r0, cy - std::cos(ka) * r0, cx + std::sin(ka) * r1, cy - std::cos(ka) * r1, std::max(1.f, rb * 0.05f));
    }
    // Cap with a soft highlight.
    const float rc = rb * 0.78f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff4a4e55), cx - rc * 0.5f, cy - rc * 0.6f, juce::Colour(0xff1b1d21), cx + rc * 0.3f, cy + rc * 0.8f, true));
    g.fillEllipse(cx - rc, cy - rc, 2 * rc, 2 * rc);
    g.setColour(juce::Colours::white.withAlpha(0.07f)); g.fillEllipse(cx - rc * 0.7f, cy - rc * 0.85f, rc * 1.4f, rc * 0.7f);
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.drawEllipse(cx - rc, cy - rc, 2 * rc, 2 * rc, 1.f);
    // Pointer line.
    const float p0 = rc * 0.25f, p1 = rb * 0.98f, pw = std::max(2.f, rc * 0.13f);
    const juce::Point<float> q0(cx + std::sin(a) * p0, cy - std::cos(a) * p0), q1(cx + std::sin(a) * p1, cy - std::cos(a) * p1);
    g.setColour(juce::Colours::black.withAlpha(0.7f)); g.drawLine({q0, q1}, pw + 2.f);
    g.setColour(on ? kIvory : kSilkDim); g.drawLine({q0, q1}, pw);
    if (s.hasKeyboardFocus(false)) { g.setColour(kVfd); g.drawEllipse(cx - rk - 2, cy - rk - 2, 2 * rk + 4, 2 * rk + 4, 1.5f); }
}

juce::Label* RetroLookAndFeel::createSliderTextBox(juce::Slider& s) {
    auto* l = LookAndFeel_V4::createSliderTextBox(s);
    l->setJustificationType(juce::Justification::centred);
    l->setColour(juce::Label::outlineWhenEditingColourId, kVfd);
    return l;
}

void RetroLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& l) {
    const bool sliderBox = dynamic_cast<juce::Slider*>(l.getParentComponent()) != nullptr;
    if (!sliderBox) {
        g.setFont(l.getFont()); g.setColour(l.findColour(juce::Label::textColourId).withMultipliedAlpha(l.isEnabled() ? 1.f : 0.5f));
        g.drawText(l.getText(), l.getLocalBounds().reduced(2, 0), l.getJustificationType(), true);
        return;
    }
    const auto glass = drawVfdGlass(g, l.getLocalBounds().toFloat().reduced(0.5f), 3.f, 1.5f, 1.5f);
    if (l.isBeingEdited()) return;   // the TextEditor draws the text
    auto text = l.getText().trim();
    text = text.replace("Hz", "HZ").replace("ms", "MS").replace(" s", " S").replace("dB", "DB").replace("x", "X");
    drawSegText(g, text, glass.reduced(3.f, 0.f), juce::Justification::centred, l.isEnabled() ? kVfd : kVfdDim, true, 20.f);
}

void RetroLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool over, bool down) {
    const auto r = b.getLocalBounds().toFloat().reduced(1.f);
    drawMetalButton(g, r, down || b.getToggleState(), over, b.hasKeyboardFocus(false));
    const float ledR = std::min(5.f, r.getHeight() * 0.2f);
    const juce::Colour led = b.getName() == "freeze" ? kRed : kVfd;
    drawLed(g, r.getX() + ledR + 7.f, r.getCentreY(), ledR, led, b.getToggleState());
    g.setFont(silkFont(std::max(10.f, std::min(13.f, r.getHeight() * 0.42f)), true));
    g.setColour(b.isEnabled() ? kIvory : kSilkDim);
    g.drawText(b.getButtonText().toUpperCase(), r.withTrimmedLeft(ledR * 2 + 12.f).reduced(2.f, 0.f), juce::Justification::centredLeft, true);
}

void RetroLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) {
    drawMetalButton(g, b.getLocalBounds().toFloat().reduced(1.f), down, over, b.hasKeyboardFocus(false));
}

void RetroLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) {
    g.setFont(getTextButtonFont(b, b.getHeight()));
    g.setColour(b.isEnabled() ? kIvory : kSilkDim);
    g.drawText(b.getButtonText().toUpperCase(), b.getLocalBounds().reduced(4, 0), juce::Justification::centred, true);
}

juce::Font RetroLookAndFeel::getTextButtonFont(juce::TextButton&, int h) { return silkFont(std::max(10.f, std::min(13.f, h * 0.42f)), true); }

void RetroLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) {
    const auto glass = drawVfdGlass(g, juce::Rectangle<float>(0, 0, (float)w, (float)h).reduced(0.5f), 4.f, 2.f, 2.f);
    const float ah = std::min(6.f, h * 0.22f);
    juce::Path tri; tri.addTriangle(glass.getRight() - ah * 2, glass.getCentreY() - ah * 0.6f, glass.getRight(), glass.getCentreY() - ah * 0.6f, glass.getRight() - ah, glass.getCentreY() + ah * 0.6f);
    g.setColour(box.isEnabled() ? kVfd : kVfdDim); g.fillPath(tri);
    if (box.hasKeyboardFocus(false)) { g.setColour(kVfd); g.drawRoundedRectangle(juce::Rectangle<float>(0, 0, (float)w, (float)h).reduced(1.f), 4.f, 1.5f); }
    // Text drawn here in 14-segment style; the box's own label is made invisible by positionComboBoxText.
    drawSegText(g, box.getText(), glass.withTrimmedRight(ah * 2.5f).reduced(4.f, 1.f), juce::Justification::centredLeft, box.isEnabled() ? kVfd : kVfdDim, true, 18.f);
}

void RetroLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& l) {
    l.setBounds(0, 0, 0, 0);    // hidden: drawComboBox renders the text
    l.setFont(getComboBoxFont(box));
}

juce::Font RetroLookAndFeel::getComboBoxFont(juce::ComboBox&) { return juce::Font(juce::FontOptions(13.f)); }

void RetroLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int w, int h) {
    g.fillAll(kVfdGlass);
    g.setTiledImageFill(dotGridTile(), 0, 0, 1.f); g.fillRect(0, 0, w, h);
    g.setColour(kVfdDim); g.drawRect(0, 0, w, h, 1);
}

void RetroLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool sep, bool active, bool highlighted, bool ticked, bool,
                                         const juce::String& text, const juce::String&, const juce::Drawable*, const juce::Colour*) {
    if (sep) { g.setColour(kVfdDim); g.fillRect(area.reduced(6, 0).withHeight(1).withY(area.getCentreY())); return; }
    if (highlighted && active) { g.setColour(kVfd.withAlpha(0.15f)); g.fillRect(area); }
    g.setFont(getPopupMenuFont());
    g.setColour(active ? (highlighted ? juce::Colours::white : kVfd) : kVfdDim);
    g.drawText((ticked ? juce::String(juce::CharPointer_UTF8("\xe2\x97\x8f ")) : juce::String("   ")) + text, area.reduced(8, 0), juce::Justification::centredLeft, true);
}

juce::Font RetroLookAndFeel::getPopupMenuFont() { return juce::Font(juce::FontOptions(14.f)); }

void RetroLookAndFeel::drawCallOutBoxBackground(juce::CallOutBox&, juce::Graphics& g, const juce::Path& path, juce::Image&) {
    juce::DropShadow(juce::Colours::black.withAlpha(0.7f), 10, {0, 3}).drawForPath(g, path);
    g.setColour(kPanel); g.fillPath(path);
    g.setColour(kPanelDark); g.strokePath(path, juce::PathStrokeType(1.5f));
}

}  // namespace retro
