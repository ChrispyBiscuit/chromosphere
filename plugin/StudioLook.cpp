#include "StudioLook.h"
#include <BinaryData.h>
#include <cmath>
#include <map>

namespace studio {

const juce::Colour kBg(0xff121315), kPlate(0xff1b1c1f), kPlateEdge(0xff26282c), kLine(0xff2a2c30),
    kText(0xffe8e6e1), kTextDim(0xff9a9da3), kTextFaint(0xff5c5f66), kAccent(0xffd9a05b), kAccentDim(0xff6b5636),
    kDisplayBg(0xff0d0e10), kGrid(0xff1f2124), kCurve(0xffe6b36d), kCurveFill(0x33d9a05b), kMeter(0xffb9bcc3), kMeterHot(0xffd9a05b), kDanger(0xffd96b5b);

namespace {
juce::Typeface::Ptr loadFace(const char* name) {
    int size = 0;
    if (const char* data = BinaryData::getNamedResource(name, size)) return juce::Typeface::createSystemTypefaceFor(data, (size_t)size);
    return nullptr;
}
juce::Typeface::Ptr face(int weight) {
    static juce::Typeface::Ptr regular = loadFace("BarlowRegular_ttf"), medium = loadFace("BarlowMedium_ttf"), semibold = loadFace("BarlowSemiBold_ttf");
    if (weight >= 600 && semibold) return semibold;
    if (weight >= 500 && medium) return medium;
    return regular;
}
}  // namespace

juce::Font font(float h, int weight) {
    if (auto f = face(weight)) return juce::Font(juce::FontOptions(f).withHeight(h));
    return juce::Font(juce::FontOptions(h, weight >= 500 ? juce::Font::bold : juce::Font::plain));
}
juce::Font caps(float h) { return font(h, 500).withExtraKerningFactor(0.12f); }

void drawCaps(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification just, float h, juce::Colour c) {
    g.setFont(caps(h)); g.setColour(c); g.drawText(text.toUpperCase(), area, just, false);
}

const juce::Image& backgroundTexture(int w, int h) {
    static std::map<std::pair<int, int>, juce::Image> cache;
    auto key = std::make_pair(w, h);
    if (auto it = cache.find(key); it != cache.end()) return it->second;
    if (cache.size() > 6) cache.clear();
    juce::Image im(juce::Image::RGB, std::max(1, w), std::max(1, h), false);
    juce::Graphics g(im);
    g.setGradientFill(juce::ColourGradient(kBg.brighter(0.06f), 0.f, 0.f, kBg, 0.f, (float)h, false));
    g.fillAll();
    juce::Random rnd(7);
    for (int i = 0; i < w * h / 14; ++i) {           // fine matte grain, barely there
        g.setColour((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha(0.012f + rnd.nextFloat() * 0.02f));
        g.fillRect(rnd.nextInt(w), rnd.nextInt(h), 1, 1);
    }
    return cache[key] = im;
}

void drawPlate(juce::Graphics& g, juce::Rectangle<float> r, float corner) {
    juce::Path p; p.addRoundedRectangle(r, corner);
    juce::DropShadow(juce::Colours::black.withAlpha(0.45f), 18, {0, 6}).drawForPath(g, p);
    g.setGradientFill(juce::ColourGradient(kPlate.brighter(0.05f), r.getX(), r.getY(), kPlate, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, corner);
    g.setColour(juce::Colours::white.withAlpha(0.05f)); g.drawRoundedRectangle(r.reduced(0.5f), corner, 1.f);
    g.setColour(juce::Colours::white.withAlpha(0.06f)); g.drawLine(r.getX() + corner, r.getY() + 1.f, r.getRight() - corner, r.getY() + 1.f, 1.f);
}

void drawDisplay(juce::Graphics& g, juce::Rectangle<float> r, float corner) {
    g.setColour(kDisplayBg); g.fillRoundedRectangle(r, corner);
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.5f), r.getX(), r.getY(), juce::Colours::transparentBlack, r.getX(), r.getY() + 14.f, false));
    g.fillRoundedRectangle(r.withHeight(std::min(14.f, r.getHeight())), corner);
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.drawRoundedRectangle(r.reduced(0.5f), corner, 1.f);
    g.setColour(juce::Colours::white.withAlpha(0.035f)); g.drawRoundedRectangle(r.reduced(1.5f), corner - 1.f, 1.f);
}

void drawDivider(juce::Graphics& g, float x0, float y, float x1) {
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.drawLine(x0, y, x1, y, 1.f);
    g.setColour(juce::Colours::white.withAlpha(0.05f)); g.drawLine(x0, y + 1.f, x1, y + 1.f, 1.f);
}

// ---- Dial body, cached per size ------------------------------------------------------------------
namespace {
const juce::Image& dialBody(int px) {
    static std::map<int, juce::Image> cache;
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

StudioLookAndFeel::StudioLookAndFeel() {
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

void StudioLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s) {
    const auto area = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h);
    const float D = std::min(area.getWidth(), area.getHeight());
    const float cx = area.getCentreX(), cy = area.getCentreY();
    const bool on = s.isEnabled();
    const float a = start + pos * (end - start);
    // Track and value arc just outside the dial.
    const float rArc = D * 0.5f - D * 0.04f, tw = std::max(1.5f, D * 0.022f);
    juce::Path track; track.addCentredArc(cx, cy, rArc, rArc, 0.f, start, end, true);
    g.setColour(kLine); g.strokePath(track, juce::PathStrokeType(tw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (on) {
        juce::Path val; val.addCentredArc(cx, cy, rArc, rArc, 0.f, start, a, true);
        g.setColour(kAccent); g.strokePath(val, juce::PathStrokeType(tw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    // Body from the cache, scaled so the dial (not the shadow pad) fits inside the arc.
    const int px = std::max(16, (int)std::round(D * 0.86f / 0.64f));           // body radius is 0.32 * px
    const auto& img = dialBody(px);
    const float scale = D * 0.86f / (px * 0.64f);
    g.setOpacity(on ? 1.f : 0.5f);
    g.drawImageTransformed(img, juce::AffineTransform::scale(scale).translated(cx - px * 0.5f * scale, cy - px * 0.5f * scale), false);
    g.setOpacity(1.f);
    // Pointer: short, bright, on the cap.
    const float R = D * 0.43f, p0 = R * 0.52f, p1 = R * 0.78f, pw = std::max(2.f, D * 0.028f);
    const juce::Point<float> q0(cx + std::sin(a) * p0, cy - std::cos(a) * p0), q1(cx + std::sin(a) * p1, cy - std::cos(a) * p1);
    g.setColour(juce::Colours::black.withAlpha(0.5f)); g.drawLine({q0.translated(0, 1.f), q1.translated(0, 1.f)}, pw);
    g.setColour(on ? kText : kTextFaint); g.drawLine({q0, q1}, pw);
    if (s.hasKeyboardFocus(false)) { g.setColour(kAccent.withAlpha(0.8f)); g.drawEllipse(cx - R - 3, cy - R - 3, 2 * R + 6, 2 * R + 6, 1.f); }
}

juce::Label* StudioLookAndFeel::createSliderTextBox(juce::Slider& s) {
    auto* l = LookAndFeel_V4::createSliderTextBox(s);
    l->setJustificationType(juce::Justification::centred);
    l->setFont(font(13.f, 500));
    l->setColour(juce::Label::outlineWhenEditingColourId, kAccent);
    return l;
}

void StudioLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& l) {
    if (l.isBeingEdited()) return;
    const bool sliderBox = dynamic_cast<juce::Slider*>(l.getParentComponent()) != nullptr;
    g.setFont(sliderBox ? font(std::clamp(l.getHeight() * 0.62f, 11.f, 14.f), 500) : l.getFont());
    g.setColour(l.findColour(juce::Label::textColourId).withMultipliedAlpha(l.isEnabled() ? 1.f : 0.4f));
    g.drawText(l.getText(), l.getLocalBounds().reduced(2, 0), l.getJustificationType(), true);
}

void StudioLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool over, bool) {
    // Pill switch on the left, label on the right.
    const auto r = b.getLocalBounds().toFloat();
    const float h = std::min(18.f, r.getHeight() * 0.7f), w = h * 1.8f;
    const auto pill = juce::Rectangle<float>(r.getX() + 1.f, r.getCentreY() - h * 0.5f, w, h);
    const bool on = b.getToggleState();
    g.setColour(juce::Colours::black.withAlpha(0.45f)); g.fillRoundedRectangle(pill.translated(0, 1.f), h * 0.5f);
    g.setColour(on ? kAccent : juce::Colour(0xff2a2c31)); g.fillRoundedRectangle(pill, h * 0.5f);
    g.setColour(juce::Colours::white.withAlpha(over ? 0.12f : 0.06f)); g.drawRoundedRectangle(pill.reduced(0.5f), h * 0.5f, 1.f);
    const float kr = h * 0.5f - 2.f, kx = on ? pill.getRight() - 2.f - kr : pill.getX() + 2.f + kr;
    g.setColour(juce::Colours::black.withAlpha(0.35f)); g.fillEllipse(kx - kr, pill.getCentreY() - kr + 1.f, 2 * kr, 2 * kr);
    g.setColour(on ? juce::Colour(0xfffaf6ee) : juce::Colour(0xff8d9199)); g.fillEllipse(kx - kr, pill.getCentreY() - kr, 2 * kr, 2 * kr);
    drawCaps(g, b.getButtonText(), r.withTrimmedLeft(w + 10.f), juce::Justification::centredLeft, std::clamp(r.getHeight() * 0.42f, 10.f, 11.5f), b.isEnabled() ? kTextDim : kTextFaint);
    if (b.hasKeyboardFocus(false)) { g.setColour(kAccent.withAlpha(0.7f)); g.drawRoundedRectangle(pill.expanded(2.f), h * 0.5f + 2.f, 1.f); }
}

void StudioLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) {
    const auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(down ? juce::Colour(0xff26282d) : over ? juce::Colour(0xff222429) : juce::Colour(0xff1d1f23));
    g.fillRoundedRectangle(r, 6.f);
    g.setColour(b.hasKeyboardFocus(false) ? kAccent.withAlpha(0.8f) : juce::Colours::white.withAlpha(over ? 0.14f : 0.09f));
    g.drawRoundedRectangle(r, 6.f, 1.f);
}

void StudioLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) {
    drawCaps(g, b.getButtonText(), b.getLocalBounds().toFloat().reduced(4.f, 0.f), juce::Justification::centred,
             std::clamp(b.getHeight() * 0.40f, 10.f, 11.5f), b.isEnabled() ? kText : kTextFaint);
}

juce::Font StudioLookAndFeel::getTextButtonFont(juce::TextButton&, int h) { return caps(std::clamp(h * 0.4f, 10.f, 11.5f)); }

void StudioLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) {
    const auto r = juce::Rectangle<float>(0, 0, (float)w, (float)h).reduced(0.5f);
    g.setColour(kDisplayBg); g.fillRoundedRectangle(r, 6.f);
    g.setColour(box.hasKeyboardFocus(false) ? kAccent.withAlpha(0.8f) : juce::Colours::white.withAlpha(0.09f)); g.drawRoundedRectangle(r, 6.f, 1.f);
    const float ch = 4.f, cxp = r.getRight() - 14.f, cyp = r.getCentreY();
    juce::Path chev; chev.startNewSubPath(cxp - ch, cyp - ch * 0.5f); chev.lineTo(cxp, cyp + ch * 0.5f); chev.lineTo(cxp + ch, cyp - ch * 0.5f);
    g.setColour(box.isEnabled() ? kTextDim : kTextFaint); g.strokePath(chev, juce::PathStrokeType(1.5f));
    g.setFont(font(13.f, 500)); g.setColour(box.isEnabled() ? kText : kTextFaint);
    g.drawText(box.getText(), r.reduced(12.f, 0.f).withTrimmedRight(16.f), juce::Justification::centredLeft, true);
}

void StudioLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& l) { l.setBounds(0, 0, 0, 0); l.setFont(getComboBoxFont(box)); }
juce::Font StudioLookAndFeel::getComboBoxFont(juce::ComboBox&) { return font(13.f, 500); }

void StudioLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int w, int h) {
    g.fillAll(kPlate);
    g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawRect(0, 0, w, h, 1);
}

void StudioLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area, bool sep, bool active, bool highlighted, bool ticked, bool,
                                          const juce::String& text, const juce::String&, const juce::Drawable*, const juce::Colour*) {
    if (sep) { g.setColour(kLine); g.fillRect(area.reduced(8, 0).withHeight(1).withY(area.getCentreY())); return; }
    if (highlighted && active) { g.setColour(juce::Colours::white.withAlpha(0.06f)); g.fillRect(area.reduced(4, 1)); }
    g.setFont(getPopupMenuFont());
    g.setColour(active ? (ticked ? kAccent : kText) : kTextFaint);
    g.drawText(text, area.reduced(14, 0), juce::Justification::centredLeft, true);
}

juce::Font StudioLookAndFeel::getPopupMenuFont() { return font(13.f, 400); }

void StudioLookAndFeel::drawCallOutBoxBackground(juce::CallOutBox&, juce::Graphics& g, const juce::Path& path, juce::Image&) {
    juce::DropShadow(juce::Colours::black.withAlpha(0.6f), 16, {0, 6}).drawForPath(g, path);
    g.setColour(kPlate); g.fillPath(path);
    g.setColour(juce::Colours::white.withAlpha(0.08f)); g.strokePath(path, juce::PathStrokeType(1.f));
}

juce::Font StudioLookAndFeel::getAlertWindowMessageFont() { return font(14.f); }
juce::Font StudioLookAndFeel::getAlertWindowFont() { return font(13.f); }
juce::Font StudioLookAndFeel::getAlertWindowTitleFont() { return font(16.f, 600); }

}  // namespace studio
