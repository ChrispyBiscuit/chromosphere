#include "Vfd.h"
#include <map>
#include <cmath>

namespace vfd {
const juce::Colour kPhosphor(0xff8ef3ff);     // ZnO:Zn blue-green, slightly toward cyan under the filter

namespace {
// 5x7 glyphs, row-major, '#' lit. Upper case only; lower case maps up.
const std::map<juce::juce_wchar, const char*>& glyphs() {
    static const std::map<juce::juce_wchar, const char*> m = {
        {' ', "....." "....." "....." "....." "....." "....." "....."},
        {'A', ".###." "#...#" "#...#" "#####" "#...#" "#...#" "#...#"},
        {'B', "####." "#...#" "#...#" "####." "#...#" "#...#" "####."},
        {'C', ".###." "#...#" "#...." "#...." "#...." "#...#" ".###."},
        {'D', "###.." "#..#." "#...#" "#...#" "#...#" "#..#." "###.."},
        {'E', "#####" "#...." "#...." "####." "#...." "#...." "#####"},
        {'F', "#####" "#...." "#...." "####." "#...." "#...." "#...."},
        {'G', ".###." "#...#" "#...." "#.###" "#...#" "#...#" ".####"},
        {'H', "#...#" "#...#" "#...#" "#####" "#...#" "#...#" "#...#"},
        {'I', ".###." "..#.." "..#.." "..#.." "..#.." "..#.." ".###."},
        {'J', "..###" "...#." "...#." "...#." "...#." "#..#." ".##.."},
        {'K', "#...#" "#..#." "#.#.." "##..." "#.#.." "#..#." "#...#"},
        {'L', "#...." "#...." "#...." "#...." "#...." "#...." "#####"},
        {'M', "#...#" "##.##" "#.#.#" "#.#.#" "#...#" "#...#" "#...#"},
        {'N', "#...#" "#...#" "##..#" "#.#.#" "#..##" "#...#" "#...#"},
        {'O', ".###." "#...#" "#...#" "#...#" "#...#" "#...#" ".###."},
        {'P', "####." "#...#" "#...#" "####." "#...." "#...." "#...."},
        {'Q', ".###." "#...#" "#...#" "#...#" "#.#.#" "#..#." ".##.#"},
        {'R', "####." "#...#" "#...#" "####." "#.#.." "#..#." "#...#"},
        {'S', ".####" "#...." "#...." ".###." "....#" "....#" "####."},
        {'T', "#####" "..#.." "..#.." "..#.." "..#.." "..#.." "..#.."},
        {'U', "#...#" "#...#" "#...#" "#...#" "#...#" "#...#" ".###."},
        {'V', "#...#" "#...#" "#...#" "#...#" "#...#" ".#.#." "..#.."},
        {'W', "#...#" "#...#" "#...#" "#.#.#" "#.#.#" "##.##" "#...#"},
        {'X', "#...#" "#...#" ".#.#." "..#.." ".#.#." "#...#" "#...#"},
        {'Y', "#...#" "#...#" ".#.#." "..#.." "..#.." "..#.." "..#.."},
        {'Z', "#####" "....#" "...#." "..#.." ".#..." "#...." "#####"},
        {'0', ".###." "#...#" "#..##" "#.#.#" "##..#" "#...#" ".###."},
        {'1', "..#.." ".##.." "..#.." "..#.." "..#.." "..#.." ".###."},
        {'2', ".###." "#...#" "....#" "...#." "..#.." ".#..." "#####"},
        {'3', "#####" "...#." "..#.." "...#." "....#" "#...#" ".###."},
        {'4', "...#." "..##." ".#.#." "#..#." "#####" "...#." "...#."},
        {'5', "#####" "#...." "####." "....#" "....#" "#...#" ".###."},
        {'6', "..##." ".#..." "#...." "####." "#...#" "#...#" ".###."},
        {'7', "#####" "....#" "...#." "..#.." ".#..." ".#..." ".#..."},
        {'8', ".###." "#...#" "#...#" ".###." "#...#" "#...#" ".###."},
        {'9', ".###." "#...#" "#...#" ".####" "....#" "...#." ".##.."},
        {'-', "....." "....." "....." "#####" "....." "....." "....."},
        {'.', "....." "....." "....." "....." "....." ".##.." ".##.."},
        {',', "....." "....." "....." "....." ".##.." "..#.." ".#..."},
        {':', "....." ".##.." ".##.." "....." ".##.." ".##.." "....."},
        {'/', "....." "....#" "...#." "..#.." ".#..." "#...." "....."},
        {'(', "..#.." ".#..." "#...." "#...." "#...." ".#..." "..#.."},
        {')', "..#.." "...#." "....#" "....#" "....#" "...#." "..#.."},
        {'+', "....." "..#.." "..#.." "#####" "..#.." "..#.." "....."},
        {'\'', ".##.." "..#.." ".#..." "....." "....." "....." "....."},
        {'&', ".##.." "#..#." "#..#." ".##.." "#.#.#" "#..#." ".##.#"},
        {'!', "..#.." "..#.." "..#.." "..#.." "..#.." "....." "..#.."},
        {'?', ".###." "#...#" "....#" "...#." "..#.." "....." "..#.."},
        {'|', "..#.." "..#.." "..#.." "..#.." "..#.." "..#.." "..#.."},
        {'_', "....." "....." "....." "....." "....." "....." "#####"},
        {'#', ".#.#." ".#.#." "#####" ".#.#." "#####" ".#.#." ".#.#."},
        {'<', "....#" "...##" "..###" ".####" "..###" "...##" "....#"},     // arrow glyphs for the selector
        {'>', "#...." "##..." "###.." "####." "###.." "##..." "#...."},
    };
    return m;
}
const char* glyphFor(juce::juce_wchar c) {
    c = (juce::juce_wchar)juce::CharacterFunctions::toUpperCase(c);
    auto it = glyphs().find(c);
    return it == glyphs().end() ? nullptr : it->second;
}

// Lays out a string on the dot matrix and lights its dots. pitch: px per dot; cell: 6 columns x 8 rows.
void lightText(juce::Graphics& lit, const juce::String& s, float x0, float y0, float pitch, float dot, float bright) {
    const juce::Colour core = kPhosphor.withMultipliedBrightness(1.f).withAlpha(bright);
    const juce::Colour centre = juce::Colour(0xffeefdff).withAlpha(0.75f * bright);
    float x = x0;
    for (auto t = s.getCharPointer(); !t.isEmpty(); ++t) {
        if (const char* gph = glyphFor(*t)) {
            for (int r = 0; r < 7; ++r) for (int c = 0; c < 5; ++c) {
                if (gph[r * 5 + c] != '#') continue;
                const float cx = x + c * pitch + pitch * 0.5f, cy = y0 + r * pitch + pitch * 0.5f;
                // Bloom first (wide, faint), then the dot, then its hot centre.
                lit.setGradientFill(juce::ColourGradient(kPhosphor.withAlpha(0.28f * bright), cx, cy, kPhosphor.withAlpha(0.f), cx + dot * 1.9f, cy, true));
                lit.fillEllipse(cx - dot * 1.9f, cy - dot * 1.9f, dot * 3.8f, dot * 3.8f);
                lit.setColour(core); lit.fillEllipse(cx - dot * 0.5f, cy - dot * 0.5f, dot, dot);
                lit.setColour(centre); lit.fillEllipse(cx - dot * 0.26f, cy - dot * 0.26f, dot * 0.52f, dot * 0.52f);
            }
        }
        x += 6 * pitch;
    }
}
float textWidth(const juce::String& s, float pitch) { return s.length() * 6 * pitch - pitch; }
}  // namespace

float draw(juce::Graphics& g, juce::Rectangle<float> r, const Content& c, float brightness, float scroll) {
    const int W = std::max(1, (int)std::ceil(r.getWidth())), H = std::max(1, (int)std::ceil(r.getHeight()));
    juce::Graphics::ScopedSaveState ss(g);
    juce::Path clip; clip.addRoundedRectangle(r, 5.f); g.reduceClipRegion(clip);
    // Glass over the dark phosphor substrate: near-black with a cool cast, slightly lighter at the top.
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff0b1216), r.getX(), r.getY(), juce::Colour(0xff04080a), r.getX(), r.getBottom(), false));
    g.fillRect(r);
    // Dot pitch: fit the longest line, 7 rows high, with margins.
    const float rows = 7.f, margin = 7.f;
    float pitch = std::min((r.getHeight() - 2.f * margin) / (rows + 0.5f), 3.2f);
    const juce::String main = c.glyph.isNotEmpty() ? c.glyph : c.centre;
    // A name that nearly fits gets a slightly finer pitch rather than a marquee.
    if (main.isNotEmpty() && c.glyph.isEmpty()) {
        const float needed = (r.getWidth() - 2.f * margin) / (6.f * main.length() - 1.f);
        if (needed < pitch && needed >= 1.9f) pitch = needed;
    }
    // The dim category label is shown only when the name still fits beside it.
    const float inner = r.getWidth() - 2.f * margin;
    const bool showLeft = c.left.isNotEmpty() && textWidth(c.left, pitch) + 6.f * pitch + textWidth(main, pitch) <= inner;
    const float leftW = showLeft ? textWidth(c.left, pitch) + 6.f * pitch : 0.f;
    const float avail = inner - leftW;
    const float overflow = std::max(0.f, textWidth(main, pitch) - avail);
    const float dot = pitch * 0.66f;
    const float y0 = r.getCentreY() - rows * pitch * 0.5f;
    // The unlit matrix: every dot the display could light, faintly there behind the glass.
    {
        juce::Graphics::ScopedSaveState s2(g);
        g.setColour(kPhosphor.withAlpha(0.05f));
        const float x0 = r.getX() + margin;
        for (float x = x0; x < r.getRight() - margin; x += 6 * pitch) {
            for (int rr = 0; rr < 7; ++rr) for (int cc = 0; cc < 5; ++cc) {
                const float cx = x + cc * pitch + pitch * 0.5f, cy = y0 + rr * pitch + pitch * 0.5f;
                if (cx > r.getRight() - margin) break;
                g.fillEllipse(cx - dot * 0.5f, cy - dot * 0.5f, dot, dot);
            }
        }
    }
    // Lit layer, drawn into its own image so a blurred copy can be laid under it as the wide glow.
    juce::Image litImg(juce::Image::ARGB, W, H, true);
    {
        juce::Graphics lit(litImg);
        lit.addTransform(juce::AffineTransform::translation(-r.getX(), -r.getY()));
        if (showLeft) lightText(lit, c.left, r.getX() + margin, y0, pitch, dot, 0.42f * brightness);
        if (main.isNotEmpty()) {
            const float tw = textWidth(main, pitch);
            const float x0 = c.glyph.isNotEmpty() ? r.getCentreX() - tw * 0.5f
                           : overflow > 0.f ? r.getX() + margin + leftW - scroll : r.getX() + margin + leftW + (avail - tw) * 0.5f;
            // Clip the text to its window so a scrolling name does not run under the label or the edge.
            juce::Graphics::ScopedSaveState s3(lit);
            lit.reduceClipRegion(juce::Rectangle<float>(r.getX() + margin + leftW, r.getY(), avail, r.getHeight()).toNearestInt());
            lightText(lit, main, x0, y0, pitch, dot, brightness);
        }
    }
    juce::Image glow = litImg.createCopy();
    { juce::ImageConvolutionKernel k(9); k.createGaussianBlur(3.2f); k.applyToImage(glow, glow, glow.getBounds()); }
    g.setOpacity(0.7f); g.drawImageAt(glow, (int)r.getX(), (int)r.getY());
    g.setOpacity(1.f);  g.drawImageAt(litImg, (int)r.getX(), (int)r.getY());
    // Anode mesh in front of the phosphor: a fine grid, darker than the glass, catches the eye up close.
    g.setColour(juce::Colours::black.withAlpha(0.16f));
    for (float y = r.getY() + 1.f; y < r.getBottom(); y += 2.f) g.fillRect(r.getX(), y, r.getWidth(), 0.6f);
    g.setColour(juce::Colours::black.withAlpha(0.08f));
    for (float x = r.getX() + 1.f; x < r.getRight(); x += 2.f) g.fillRect(x, r.getY(), 0.6f, r.getHeight());
    // Glass: a soft reflection across the upper part, darker edges, and a bevel.
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.07f), r.getX(), r.getY(), juce::Colours::white.withAlpha(0.f), r.getX(), r.getY() + r.getHeight() * 0.45f, false));
    g.fillRect(r.withHeight(r.getHeight() * 0.45f));
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.35f), r.getX(), 0.f, juce::Colours::transparentBlack, r.getX() + 24.f, 0.f, false)); g.fillRect(r.withWidth(24.f));
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.35f), r.getRight(), 0.f, juce::Colours::transparentBlack, r.getRight() - 24.f, 0.f, false)); g.fillRect(r.withLeft(r.getRight() - 24.f));
    g.setColour(juce::Colours::black.withAlpha(0.8f)); g.drawRoundedRectangle(r.reduced(0.5f), 5.f, 1.f);
    g.setColour(juce::Colours::white.withAlpha(0.06f)); g.drawRoundedRectangle(r.reduced(1.5f), 4.f, 1.f);
    return overflow;
}

Display::Display() { setInterceptsMouseClicks(false, false); startTimerHz(30); }
void Display::setContent(const Content& c) {
    if (c.left == content_.left && c.centre == content_.centre && c.glyph == content_.glyph) return;
    content_ = c; cache_ = juce::Image(); scroll_ = 0.f; hold_ = 1.8; repaint();
}
void Display::timerCallback() {
    // Phosphor flicker: a slow wander of a few percent with the odd dip, like a real tube.
    if (rnd_.nextFloat() < 0.08f) target_ = 0.93f + 0.07f * rnd_.nextFloat();
    flicker_ += (target_ - flicker_) * 0.3f;
    // Marquee: hold at the start, crawl to the end, hold, snap back.
    if (overflow_ > 0.f) {
        if (hold_ > 0.0) hold_ -= 1.0 / 30.0;
        else {
            scroll_ += 0.8f;
            if (scroll_ >= overflow_) { scroll_ = overflow_; hold_ = 1.5; }
            if (scroll_ >= overflow_ && hold_ <= 0.0) { scroll_ = 0.f; hold_ = 1.8; }
        }
        cache_ = juce::Image();
    }
    repaint();
}
void Display::paint(juce::Graphics& g) {
    if (!cache_.isValid() && getWidth() > 0 && getHeight() > 0) {
        cache_ = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics cg(cache_);
        overflow_ = draw(cg, getLocalBounds().toFloat(), content_, 1.f, scroll_);
    }
    if (cache_.isValid()) { g.setOpacity(0.9f + 0.1f * flicker_); g.drawImageAt(cache_, 0, 0); }
}

}  // namespace vfd
