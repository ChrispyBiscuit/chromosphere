#include "SevenSeg.h"
#include "StudioLook.h"
#include <cmath>

namespace seg {
const juce::Colour kLed(0xffff3a22);     // GaAsP red, ~650 nm, as seen through the filter

namespace {
// Segment bits: a b c d e f g  (top, top-right, bottom-right, bottom, bottom-left, top-left, middle)
int bitsFor(juce::juce_wchar c) {
    switch (juce::CharacterFunctions::toUpperCase(c)) {
        case '0': return 0b1111110; case '1': return 0b0110000; case '2': return 0b1101101; case '3': return 0b1111001;
        case '4': return 0b0110011; case '5': return 0b1011011; case '6': return 0b1011111; case '7': return 0b1110000;
        case '8': return 0b1111111; case '9': return 0b1111011; case '-': return 0b0000001;
        case 'A': return 0b1110111; case 'B': return 0b0011111; case 'C': return 0b1001110; case 'D': return 0b0111101;
        case 'E': return 0b1001111; case 'F': return 0b1000111; case 'H': return 0b0110111; case 'L': return 0b0001110;
        case 'N': return 0b0010101; case 'O': return 0b0011101; case 'P': return 0b1100111; case 'R': return 0b0000101;
        case 'S': return 0b1011011; case 'T': return 0b0001111; case 'U': return 0b0111110;
        default: return 0;
    }
}
// Hexagonal bar for one segment, in a digit box of width w and height h with segment thickness t.
juce::Path segmentPath(int i, float w, float h, float t) {
    const float gap = t * 0.35f;                       // the dark gap at the corners
    auto hbar = [&](float y) {        // horizontal, centred at y
        juce::Path p; const float x0 = t * 0.5f + gap, x1 = w - t * 0.5f - gap, ht = t * 0.5f;
        p.startNewSubPath(x0, y); p.lineTo(x0 + ht, y - ht); p.lineTo(x1 - ht, y - ht); p.lineTo(x1, y); p.lineTo(x1 - ht, y + ht); p.lineTo(x0 + ht, y + ht); p.closeSubPath(); return p;
    };
    auto vbar = [&](float x, float y0, float y1) {   // vertical, centred at x
        juce::Path p; const float ht = t * 0.5f; y0 += gap; y1 -= gap;
        p.startNewSubPath(x, y0); p.lineTo(x + ht, y0 + ht); p.lineTo(x + ht, y1 - ht); p.lineTo(x, y1); p.lineTo(x - ht, y1 - ht); p.lineTo(x - ht, y0 + ht); p.closeSubPath(); return p;
    };
    const float mid = h * 0.5f;
    switch (i) {
        case 0: return hbar(t * 0.5f);
        case 1: return vbar(w - t * 0.5f, t * 0.5f, mid);
        case 2: return vbar(w - t * 0.5f, mid, h - t * 0.5f);
        case 3: return hbar(h - t * 0.5f);
        case 4: return vbar(t * 0.5f, mid, h - t * 0.5f);
        case 5: return vbar(t * 0.5f, t * 0.5f, mid);
        default: return hbar(mid);
    }
}
}  // namespace

void draw(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, bool enabled) {
    using namespace studio;
    // Split "-12.0 dB" into the number for the display and the unit for the silkscreen.
    juce::String num, unit;
    { int i = 0; const auto s = text.trim(); while (i < s.length() && (juce::CharacterFunctions::isDigit(s[i]) || s[i] == '-' || s[i] == '.' || s[i] == '+')) ++i; num = s.substring(0, i); unit = s.substring(i).trim(); if (num.isEmpty()) { num = s; unit.clear(); } }
    // The window is centred under the dial; the unit is silkscreened just outside it on the right.
    const float unitW = 24.f;
    auto win = r.reduced(unitW, 0.f).reduced(0.5f);
    if (unit.isNotEmpty()) drawCaps(g, unit, juce::Rectangle<float>(win.getRight() + 3.f, r.getY(), unitW - 3.f, r.getHeight()), juce::Justification::centredLeft, std::clamp(r.getHeight() * 0.42f, 8.f, 9.5f), enabled ? kTextFaint : kTextFaint.withAlpha(0.5f));
    juce::Graphics::ScopedSaveState ss(g);
    juce::Path clip; clip.addRoundedRectangle(win, 3.f); g.reduceClipRegion(clip);
    // The filter window: near-black with a deep red cast, a touch lighter at the top.
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1a0604), win.getX(), win.getY(), juce::Colour(0xff0d0302), win.getX(), win.getBottom(), false));
    g.fillRect(win);
    // Digit geometry: height from the window, slanted 8 degrees, right-aligned.
    const float dh = win.getHeight() - 5.f, dw = dh * 0.58f, t = std::max(1.6f, dh * 0.17f), adv = dw + dh * 0.22f, dotW = t * 1.1f;
    const float skew = -std::tan(8.f * 3.14159265f / 180.f);
    int cells = 0; for (auto c = num.getCharPointer(); !c.isEmpty(); ++c) if (*c != '.') ++cells;
    const int dots = num.length() - cells;
    const float total = cells * adv + dots * dotW;
    float x = win.getRight() - 5.f - total, y = win.getY() + 2.5f;
    const int W = (int)std::ceil(r.getWidth()), H = (int)std::ceil(r.getHeight());
    juce::Image lit(juce::Image::ARGB, std::max(1, W), std::max(1, H), true);
    juce::Graphics lg(lit); lg.addTransform(juce::AffineTransform::translation(-r.getX(), -r.getY()));
    const juce::Colour ghost = juce::Colour(0xff5a1a12).withAlpha(enabled ? 0.5f : 0.28f);
    const juce::Colour tipCol = kLed.darker(0.35f), midCol = kLed, hotCol = juce::Colour(0xffffb8a0);
    for (auto c = num.getCharPointer(); !c.isEmpty(); ++c) {
        if (*c == '.') {
            // Decimal point: a small square-ish dot at the baseline, lit red.
            const float px = x - dotW * 0.15f, py = y + dh - t * 0.5f;
            const auto tr = juce::AffineTransform::shear(skew, 0.f).translated(px - skew * py, 0.f);
            juce::Path d; d.addRoundedRectangle(px - t * 0.45f, py - t * 0.45f, t * 0.9f, t * 0.9f, t * 0.2f);
            g.setColour(ghost); g.fillPath(d, juce::AffineTransform::shear(skew, 0.f).translated(-skew * py, 0.f));
            if (enabled) { lg.setColour(midCol); lg.fillPath(d, juce::AffineTransform::shear(skew, 0.f).translated(-skew * py, 0.f)); }
            juce::ignoreUnused(tr);
            x += dotW; continue;
        }
        const int bits = bitsFor(*c);
        const auto cellT = juce::AffineTransform::translation(x, y);
        // Italic: shear about the digit's baseline so the bottom stays put.
        const auto italic = juce::AffineTransform::shear(skew, 0.f).translated(-skew * (y + dh), 0.f);
        for (int i = 0; i < 7; ++i) {
            juce::Path p = segmentPath(i, dw, dh, t);
            p.applyTransform(cellT.followedBy(italic));
            g.setColour(ghost); g.fillPath(p);                   // every segment is there, unlit, behind the filter
            if (!enabled || !(bits & (1 << (6 - i)))) continue;
            // Lit: the light pipe is brightest in its middle and falls off toward the tips.
            const auto b = p.getBounds();
            const bool horiz = i == 0 || i == 3 || i == 6;
            juce::ColourGradient grad = horiz ? juce::ColourGradient(tipCol, b.getX(), 0.f, tipCol, b.getRight(), 0.f, false)
                                              : juce::ColourGradient(tipCol, 0.f, b.getY(), tipCol, 0.f, b.getBottom(), false);
            grad.addColour(0.5, midCol.interpolatedWith(hotCol, 0.35f)); grad.addColour(0.3, midCol); grad.addColour(0.7, midCol);
            lg.setGradientFill(grad); lg.fillPath(p);
        }
        x += adv;
    }
    // Bloom: the epoxy scatters some light around each lit segment.
    juce::Image glow = lit.createCopy();
    { juce::ImageConvolutionKernel k(7); k.createGaussianBlur(2.2f); k.applyToImage(glow, glow, glow.getBounds()); }
    g.setOpacity(0.55f); g.drawImageAt(glow, (int)r.getX(), (int)r.getY());
    g.setOpacity(1.f);   g.drawImageAt(lit, (int)r.getX(), (int)r.getY());
    // The glossy face: a reflection band and a dark lower edge, then the bezel.
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.07f), win.getX(), win.getY(), juce::Colours::white.withAlpha(0.f), win.getX(), win.getY() + win.getHeight() * 0.42f, false));
    g.fillRect(win.withHeight(win.getHeight() * 0.42f));
    g.setColour(juce::Colours::black.withAlpha(0.8f)); g.drawRoundedRectangle(win, 3.f, 1.f);
    g.setColour(juce::Colours::white.withAlpha(0.05f)); g.drawRoundedRectangle(win.reduced(1.f), 2.5f, 1.f);
}

ValueLabel::ValueLabel() : juce::Label({}, {}) {
    setJustificationType(juce::Justification::centred);
    setKeyboardType(juce::TextInputTarget::decimalKeyboard);
    setColour(juce::Label::textColourId, juce::Colours::transparentBlack);
}
void ValueLabel::paint(juce::Graphics& g) {
    if (isBeingEdited()) return;
    if (!cache_.isValid() && getWidth() > 0 && getHeight() > 0) {
        cache_ = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics cg(cache_);
        draw(cg, getLocalBounds().toFloat(), getText(), isEnabled());
    }
    if (cache_.isValid()) g.drawImageAt(cache_, 0, 0);
}
void ValueLabel::paintOverChildren(juce::Graphics& g) {
    if (isBeingEdited()) { g.setColour(studio::kAccent.withAlpha(0.8f)); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.f, 1.f); }
}

}  // namespace seg
