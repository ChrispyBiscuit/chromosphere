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
        {'v', "....." "....." "#####" ".###." "..#.." "....." "....."},     // combo chevron
        {'%', "##..#" "##.#." "...#." "..#.." ".#..." "#.###" "#..##"},
        {'=', "....." "....." "#####" "....." "#####" "....." "....."},
        {'*', "....." ".#.#." "..#.." "#####" "..#.." ".#.#." "....."},
    };
    return m;
}
const char* glyphFor(juce::juce_wchar c) {
    if (c == 1) c = 'v';                                          // chevron: not reachable from text
    else c = (juce::juce_wchar)juce::CharacterFunctions::toUpperCase(c);
    auto it = glyphs().find(c);
    return it == glyphs().end() ? nullptr : it->second;
}

int textCols(const juce::String& s) { return s.isEmpty() ? 0 : s.length() * 6 - 1; }
// Writes a string's glyph columns into the mask starting at column c0 (columns outside are dropped).
void stamp(std::vector<float>& mask, int cols, const juce::String& s, int c0, float level) {
    int c = c0;
    for (auto t = s.getCharPointer(); !t.isEmpty(); ++t, c += 6) {
        const char* gph = glyphFor(*t); if (!gph) continue;
        for (int r = 0; r < 7; ++r) for (int k = 0; k < 5; ++k) {
            const int cc = c + k; if (cc < 0 || cc >= cols || gph[r * 5 + k] != '#') continue;
            mask[(size_t)(r * cols + cc)] = std::max(mask[(size_t)(r * cols + cc)], level);
        }
    }
}
// Phosphor colour by state: a cooling dot goes from white-hot through the blue-green to a dim teal.
juce::Colour phosphorAt(float s) {
    const juce::Colour cool(0xff1d5a66), mid = kPhosphor, hot(0xffeefdff);
    if (s < 0.6f) return cool.interpolatedWith(mid, juce::jlimit(0.f, 1.f, s / 0.6f));
    return mid.interpolatedWith(hot, juce::jlimit(0.f, 1.f, (s - 0.6f) / 0.4f) * 0.45f);
}
}  // namespace

Display::Display() { setInterceptsMouseClicks(false, false); startTimerHz(30); }

void Display::setContent(const Content& c) {
    if (c.left == content_.left && c.centre == content_.centre && c.glyph == content_.glyph && c.right == content_.right && c.alignLeft == content_.alignLeft) return;
    content_ = c; scroll_ = 0; hold_ = 1.5; scrollAcc_ = 0.0;
    under_ = juce::Image();            // the pitch depends on the text: relayout the matrix on the next step
    rebuildMask();
}

void Display::resized() { under_ = juce::Image(); over_ = juce::Image(); sprites_.clear(); }

void Display::rebuildStatic() {
    const auto r = getLocalBounds().toFloat();
    const int W = std::max(1, getWidth()), H = std::max(1, getHeight());
    const float margin = juce::jlimit(2.5f, 7.f, r.getHeight() * 0.2f), rows = 7.f;
    pitch_ = std::min((r.getHeight() - 2.f * margin) / (rows + 0.5f), 3.2f);
    // A name that nearly fits gets a finer pitch rather than a scroll.
    if (content_.glyph.isEmpty() && content_.centre.isNotEmpty()) {
        const int need = textCols(content_.centre) + (content_.right.isEmpty() ? 0 : textCols(content_.right) + 4);
        const float needed = (r.getWidth() - 2.f * margin) / (float)(need + 1);   // one spare column so flooring never loses the last one
        if (needed < pitch_ && needed >= minPitch_) pitch_ = needed;
    }
    dot_ = pitch_ * 0.66f;
    cols_ = std::max(1, (int)((r.getWidth() - 2.f * margin) / pitch_));
    x0_ = r.getCentreX() - cols_ * pitch_ * 0.5f;
    y0_ = r.getCentreY() - rows * pitch_ * 0.5f;
    mask_.assign((size_t)(cols_ * 7), 0.f); state_.assign((size_t)(cols_ * 7), 0.f);
    // Under: glass substrate and the ghost matrix.
    under_ = juce::Image(juce::Image::ARGB, W, H, true);
    {
        juce::Graphics g(under_);
        juce::Path clip; clip.addRoundedRectangle(r, 5.f); g.reduceClipRegion(clip);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff0b1216), r.getX(), r.getY(), juce::Colour(0xff04080a), r.getX(), r.getBottom(), false));
        g.fillRect(r);
        g.setColour(kPhosphor.withAlpha(0.05f));
        for (int rr = 0; rr < 7; ++rr) for (int c = 0; c < cols_; ++c) {
            if (c % 6 == 5) continue;                         // the gap column between characters has no dots
            const float cx = x0_ + c * pitch_ + pitch_ * 0.5f, cy = y0_ + rr * pitch_ + pitch_ * 0.5f;
            g.fillEllipse(cx - dot_ * 0.5f, cy - dot_ * 0.5f, dot_, dot_);
        }
    }
    // Over: the anode mesh in front of the phosphor, the glass reflection, edge darkening, bevel.
    over_ = juce::Image(juce::Image::ARGB, W, H, true);
    {
        juce::Graphics g(over_);
        juce::Path clip; clip.addRoundedRectangle(r, 5.f); g.reduceClipRegion(clip);
        g.setColour(juce::Colours::black.withAlpha(0.16f));
        for (float y = r.getY() + 1.f; y < r.getBottom(); y += 2.f) g.fillRect(r.getX(), y, r.getWidth(), 0.6f);
        g.setColour(juce::Colours::black.withAlpha(0.08f));
        for (float x = r.getX() + 1.f; x < r.getRight(); x += 2.f) g.fillRect(x, r.getY(), 0.6f, r.getHeight());
        g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.07f), r.getX(), r.getY(), juce::Colours::white.withAlpha(0.f), r.getX(), r.getY() + r.getHeight() * 0.45f, false));
        g.fillRect(r.withHeight(r.getHeight() * 0.45f));
        const float edge = std::min(24.f, r.getWidth() * 0.12f);
        g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.35f), r.getX(), 0.f, juce::Colours::transparentBlack, r.getX() + edge, 0.f, false)); g.fillRect(r.withWidth(edge));
        g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.35f), r.getRight(), 0.f, juce::Colours::transparentBlack, r.getRight() - edge, 0.f, false)); g.fillRect(r.withLeft(r.getRight() - edge));
        g.setColour(juce::Colours::black.withAlpha(0.8f)); g.drawRoundedRectangle(r.reduced(0.5f), 5.f, 1.f);
        g.setColour(juce::Colours::white.withAlpha(0.06f)); g.drawRoundedRectangle(r.reduced(1.5f), 4.f, 1.f);
    }
    // Sprites: one dot with its bloom at each of 32 brightness levels, colour shifting with heat.
    sprites_.clear();
    const int S = std::max(4, (int)std::ceil(dot_ * 4.f)) | 1;
    for (int i = 0; i < 32; ++i) {
        const float sLev = (i + 1) / 32.f;
        juce::Image im(juce::Image::ARGB, S, S, true);
        juce::Graphics g(im);
        const float cx = S * 0.5f, cy = S * 0.5f;
        const juce::Colour col = phosphorAt(sLev);
        g.setGradientFill(juce::ColourGradient(col.withAlpha(0.30f * sLev * sLev), cx, cy, col.withAlpha(0.f), cx + dot_ * 1.9f, cy, true));
        g.fillEllipse(cx - dot_ * 1.9f, cy - dot_ * 1.9f, dot_ * 3.8f, dot_ * 3.8f);
        g.setColour(col.withAlpha(std::pow(sLev, 0.8f))); g.fillEllipse(cx - dot_ * 0.5f, cy - dot_ * 0.5f, dot_, dot_);
        if (sLev > 0.6f) { g.setColour(juce::Colour(0xffd8fbff).withAlpha(0.45f * (sLev - 0.6f) / 0.4f)); g.fillEllipse(cx - dot_ * 0.26f, cy - dot_ * 0.26f, dot_ * 0.52f, dot_ * 0.52f); }
        sprites_.push_back(im);
    }
    rebuildMask();
}

void Display::rebuildMask() {
    if (cols_ <= 0) return;
    std::fill(mask_.begin(), mask_.end(), 0.f);
    const juce::String main = content_.glyph.isNotEmpty() ? content_.glyph : content_.centre;
    const int mainCols = textCols(main);
    if (content_.glyph.isNotEmpty()) { stamp(mask_, cols_, main, (cols_ - mainCols) / 2, 1.f); textCols_ = 0; availCols_ = cols_; dirty_ = true; return; }
    const int leftCols = textCols(content_.left);
    const int rightCols = content_.right.isEmpty() ? 0 : textCols(content_.right) + 4;
    if (rightCols > 0) stamp(mask_, cols_, content_.right, cols_ - textCols(content_.right), 0.55f);
    const bool showLeft = leftCols > 0 && leftCols + 6 + mainCols <= cols_ - rightCols;
    const int c0 = showLeft ? leftCols + 6 : 0;
    availCols_ = cols_ - c0 - rightCols;
    if (showLeft) stamp(mask_, cols_, content_.left, 0, 0.42f);
    textCols_ = mainCols;
    if (mainCols <= availCols_) stamp(mask_, cols_, main, c0 + (content_.alignLeft ? 0 : (availCols_ - mainCols) / 2), 1.f);
    else {
        // Ticker: the text passes through the window, wrapping with a gap; only its window columns are driven.
        const int period = mainCols + 8;
        std::vector<float> strip((size_t)(period * 7), 0.f);
        stamp(strip, period, main, 0, 1.f);
        for (int r = 0; r < 7; ++r) for (int c = 0; c < availCols_; ++c) {
            const int src = ((c + scroll_) % period + period) % period;
            mask_[(size_t)(r * cols_ + c0 + c)] = strip[(size_t)(r * period + src)];
        }
    }
    dirty_ = true;
}

void Display::step(double dt) {
    if (!under_.isValid() && getWidth() > 0 && getHeight() > 0) rebuildStatic();
    if (cols_ <= 0) return;
    // Ticker timing: a hold, then one column every 60 ms.
    if (textCols_ > availCols_) {
        if (hold_ > 0.0) hold_ -= dt;
        else { scrollAcc_ += dt; while (scrollAcc_ >= 0.06) { scrollAcc_ -= 0.06; ++scroll_; } rebuildMask(); }
    }
    // Phosphor: driven dots rise in ~25 ms; undriven ones cool. The cooling is fast while hot and
    // slows as the dot dims, so a switched-off character flashes down then lingers as an ember.
    const float rise = 1.f - std::exp(-(float)dt / 0.025f);
    bool changed = false;
    for (size_t i = 0; i < state_.size(); ++i) {
        const float t = mask_[i], s = state_[i];
        float n = s;
        if (t > s) n = s + (t - s) * rise;
        else if (s > 0.002f) { const float tau = 0.08f + 0.45f * (1.f - s); n = s * std::exp(-(float)dt / tau); if (n < 0.002f) n = 0.f; }
        if (n != s) { state_[i] = n; changed = true; }
    }
    if (changed) dirty_ = true;
}

void Display::timerCallback() {
    // A real tube is driven far above the flicker threshold: what remains is a slow drift of a
    // percent or two, not a visible flash.
    if (rnd_.nextFloat() < 0.02f) target_ = 0.985f + 0.015f * rnd_.nextFloat();
    const float f0 = flicker_; flicker_ += (target_ - flicker_) * 0.06f;
    step(1.0 / 30.0);
    if (dirty_ || std::abs(flicker_ - f0) > 0.002f) repaint();
}

void Display::paint(juce::Graphics& g) {
    if (getWidth() <= 0 || getHeight() <= 0) return;
    if (!under_.isValid()) rebuildStatic();
    g.drawImageAt(under_, 0, 0);
    if (!sprites_.empty()) {
        const int S = sprites_[0].getWidth();
        g.setOpacity(flicker_);
        for (int r = 0; r < 7; ++r) for (int c = 0; c < cols_; ++c) {
            const float s = state_[(size_t)(r * cols_ + c)];
            if (s <= 0.002f) continue;
            const int lev = juce::jlimit(0, 31, (int)(s * 32.f));
            const float cx = x0_ + c * pitch_ + pitch_ * 0.5f, cy = y0_ + r * pitch_ + pitch_ * 0.5f;
            g.drawImageAt(sprites_[(size_t)lev], (int)std::lround(cx - S * 0.5f), (int)std::lround(cy - S * 0.5f));
        }
        g.setOpacity(1.f);
    }
    g.drawImageAt(over_, 0, 0);
    dirty_ = false;
}

// ---- ValueLabel ----
ValueLabel::ValueLabel() : juce::Label({}, {}) {
    addAndMakeVisible(vfd_); vfd_.toBack();
    vfd_.setMinPitch(1.2f);
    setJustificationType(juce::Justification::centred);
    setKeyboardType(juce::TextInputTarget::decimalKeyboard);
    setColour(juce::Label::textColourId, juce::Colours::transparentBlack);
    sync();
}
void ValueLabel::resized() { juce::Label::resized(); vfd_.setBounds(getLocalBounds()); }
void ValueLabel::sync() { Content c; c.centre = getText(); vfd_.setContent(c); vfd_.setAlpha(isEnabled() ? 1.f : 0.4f); }
void ValueLabel::paintOverChildren(juce::Graphics& g) {
    if (isBeingEdited()) { g.setColour(juce::Colour(0xffe8873a).withAlpha(0.8f)); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.f, 1.f); }
}

void stepAll(juce::Component& root, double dt) {
    if (auto* d = dynamic_cast<Display*>(&root)) { d->renderNow(dt); return; }
    for (auto* c : root.getChildren()) stepAll(*c, dt);
}

// ---- ComboBox ----
ComboBox::ComboBox() {
    addAndMakeVisible(vfd_);
    vfd_.toBack();
    vfd_.setMinPitch(1.5f);
    setColour(juce::ComboBox::textColourId, juce::Colours::transparentBlack);   // the internal label stays empty
    sync();
}
void ComboBox::resized() { juce::ComboBox::resized(); vfd_.setBounds(getLocalBounds()); }
void ComboBox::valueChanged(juce::Value& v) { juce::ComboBox::valueChanged(v); sync(); }
void ComboBox::sync() {
    Content c; c.centre = getText(); c.alignLeft = true; c.right = juce::String::charToString((juce::juce_wchar)1);
    vfd_.setContent(c);
    vfd_.setAlpha(isEnabled() ? 1.f : 0.45f);
}
void ComboBox::paint(juce::Graphics&) { sync(); }   // the Value notification is asynchronous; catch up here
void ComboBox::paintOverChildren(juce::Graphics& g) {
    const auto r = getLocalBounds().toFloat().reduced(0.5f);
    if (hasKeyboardFocus(true)) { g.setColour(juce::Colour(0xffe8873a).withAlpha(0.8f)); g.drawRoundedRectangle(r, 5.f, 1.f); }
    else if (isMouseOver(true)) { g.setColour(juce::Colours::white.withAlpha(0.10f)); g.drawRoundedRectangle(r, 5.f, 1.f); }
}

}  // namespace vfd
