// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Skin B: "Breadbin" - the C64 computer case.

#include "Skins.h"

namespace zadabar
{

namespace
{
    const juce::Colour caseTop     { 0xffe3d9bf };
    const juce::Colour caseBottom  { 0xffd3c7a8 };
    const juce::Colour groove      { 0xffb9ac8b };
    const juce::Colour tray        { 0xffcdc1a1 };
    const juce::Colour trayShadow  { 0xffa79973 };
    const juce::Colour brown       { 0xff3d2f24 };
    const juce::Colour brownLight  { 0xff5a4836 };
    const juce::Colour keyShadow   { 0xff241b14 };
    const juce::Colour cream       { 0xffece5d3 };
    const juce::Colour beigeText   { 0xffc9bc9a };
    const juce::Colour amber       { 0xffe8b84a };
    const juce::Colour rust        { 0xff7a2a1e };
    const juce::Colour bezel       { 0xff2b2723 };
    const juce::Colour c64Blue     { 0xff352879 };
    const juce::Colour c64Light    { 0xff6c5eb5 };
    const juce::Colour c64Pale     { 0xffa59ee8 };

    void keycap (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour top, juce::Colour bottom, juce::Colour shadow)
    {
        g.setColour (shadow);
        g.fillRoundedRectangle (r.translated (0.0f, 3.0f), 4.0f);
        g.setGradientFill (juce::ColourGradient (top, r.getX(), r.getY(), bottom, r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.0f);
    }
}

SkinBreadbin::SkinBreadbin()
{
    setColour (juce::ComboBox::textColourId,        brown);
    setColour (juce::ComboBox::backgroundColourId,  caseTop);
    setColour (juce::ComboBox::arrowColourId,       brown);
    setColour (juce::PopupMenu::backgroundColourId, caseTop);
    setColour (juce::PopupMenu::textColourId,       brown);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, amber);
    setColour (juce::PopupMenu::highlightedTextColourId,       keyShadow);
    setColour (juce::Label::textColourId,           brown);
}

juce::Colour SkinBreadbin::textColour() const  { return brown; }
juce::Colour SkinBreadbin::valueColour() const { return rust; }

void SkinBreadbin::paintBackground (juce::Graphics& g, juce::Rectangle<int> b)
{
    auto r = b.toFloat();
    g.fillAll (juce::Colour (0xff1b1712));
    g.setGradientFill (juce::ColourGradient (caseTop, 0.0f, r.getY(), caseBottom, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 14.0f);
    g.setColour (trayShadow);
    g.fillRect (r.removeFromBottom (3.0f).reduced (10.0f, 0.0f));
}

void SkinBreadbin::paintHeader (juce::Graphics& g, juce::Rectangle<int>, juce::Rectangle<int> logoArea,
                                juce::Rectangle<int> info, const HeaderState& s)
{
    // --- name badge
    auto badge = logoArea.reduced (0, 4).toFloat();
    g.setColour (juce::Colour (0xff9c8e6e));
    g.fillRoundedRectangle (badge.translated (0.0f, 2.0f), 6.0f);
    g.setGradientFill (juce::ColourGradient (brownLight, 0.0f, badge.getY(), brown, 0.0f, badge.getBottom(), false));
    g.fillRoundedRectangle (badge, 6.0f);

    auto inner = badge.reduced (12.0f, 8.0f).toNearestInt();
    if (auto* logo = cachedLogo (juce::Colours::white))
        logo->drawWithin (g, inner.removeFromLeft (66).toFloat(), juce::RectanglePlacement::centred, 1.0f);
    inner.removeFromLeft (10);

    auto stripes = inner.removeFromRight (48).withSizeKeepingCentre (48, 29);
    const juce::Colour rainbow[] = { juce::Colour (0xffc8402f), juce::Colour (0xffe5862b), juce::Colour (0xffeec23a),
                                     juce::Colour (0xff5aa04a), juce::Colour (0xff3a78bf) };
    for (int i = 0; i < 5; ++i)
    {
        g.setColour (rainbow[i]);
        g.fillRect (stripes.getX(), stripes.getY() + i * 6, stripes.getWidth(), 5);
    }

    g.setColour (cream);
    g.setFont (font (24.0f));
    g.drawText ("ZADABAR64", inner.withTrimmedBottom (16), juce::Justification::bottomLeft, false);
    g.setColour (beigeText);
    g.setFont (font (8.0f));
    g.drawText ("SID SOUND SYNTHESIZER", inner.withTrimmedTop (inner.getHeight() - 12), juce::Justification::topLeft, false);

    // --- monitor with status
    auto led = info.removeFromRight (54);
    auto mon = info.reduced (8, 2).toFloat();
    g.setColour (bezel);
    g.fillRoundedRectangle (mon, 9.0f);
    auto scr = mon.reduced (6.0f);
    g.setColour (c64Light);
    g.fillRoundedRectangle (scr, 5.0f);
    scr = scr.reduced (5.0f);
    g.setColour (c64Blue);
    g.fillRect (scr);

    auto text = scr.reduced (6.0f, 3.0f).toNearestInt();
    const int lineH = text.getHeight() / 3;
    g.setFont (font (8.0f));
    g.setColour (c64Pale);
    auto line1 = text.removeFromTop (lineH);
    g.drawText (s.chipText, line1, juce::Justification::centredLeft, false);
    for (int v = 0; v < 3; ++v)
    {
        auto box = juce::Rectangle<int> (line1.getRight() - (3 - v) * 14, line1.getCentreY() - 4, 10, 8);
        if (s.voiceActive[(size_t) v]) g.fillRect (box); else g.drawRect (box, 1);
    }
    g.drawText (s.drumMode ? "MODE DRUMS" : "MODE INSTRUMENT", text.removeFromTop (lineH), juce::Justification::centredLeft, false);
    g.setColour (juce::Colours::white);
    g.drawText ("READY.", text, juce::Justification::centredLeft, false);
    if (s.cursorOn)
        g.fillRect (text.getX() + juce::GlyphArrangement::getStringWidthInt (font (8.0f), "READY. "), text.getCentreY() - 4, 8, 8);

    // --- power LED
    auto ledDot = led.withSizeKeepingCentre (14, 14).translated (0, -6).toFloat();
    g.setColour (juce::Colour (0x66ff4b33));
    g.fillEllipse (ledDot.expanded (5.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff8a73), ledDot.getX() + 4, ledDot.getY() + 4,
                                             juce::Colour (0xff7a140c), ledDot.getRight(), ledDot.getBottom(), true));
    g.fillEllipse (ledDot);
    g.setColour (brown);
    g.setFont (font (8.0f));
    g.drawText ("POWER", led.withTrimmedTop (led.getHeight() / 2 + 6), juce::Justification::centredTop, false);
}

void SkinBreadbin::paintStatusStrip (juce::Graphics& g, juce::Rectangle<int> r, const HeaderState&)
{
    // the vent grooves on top of the case
    auto vents = r.reduced (0, 4);
    for (int x = vents.getX(); x < vents.getRight(); x += 14)
    {
        g.setColour (groove);
        g.fillRect (x + 10, vents.getY(), 4, vents.getHeight());
    }
    g.setColour (trayShadow);
    g.fillRect (vents.getX(), vents.getBottom(), vents.getWidth(), 1);
}

void SkinBreadbin::paintPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, Section)
{
    const auto rf = r.toFloat();
    g.setColour (trayShadow);
    g.fillRoundedRectangle (rf, 6.0f);
    g.setColour (tray);
    g.fillRoundedRectangle (rf.withTrimmedTop (2.0f), 6.0f);

    g.setFont (font (8.0f));
    const float w = static_cast<float> (juce::GlyphArrangement::getStringWidthInt (font (8.0f), title)) + 20.0f;
    auto cap = juce::Rectangle<float> (rf.getX() + 8.0f, rf.getY() + 7.0f, w, 18.0f);
    keycap (g, cap, brownLight, brown, keyShadow);
    g.setColour (cream);
    g.drawText (title, cap.toNearestInt(), juce::Justification::centred, false);
}

void SkinBreadbin::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                     float startAngle, float endAngle, juce::Slider& s)
{
    const float size = static_cast<float> (juce::jmin (w, h, 76));
    const auto  area = juce::Rectangle<float> (size, size).withCentre (juce::Rectangle<int> (x, y, w, h).toFloat().getCentre());
    const auto  c    = area.getCentre();
    const float rOut = size * 0.5f;

    // printed scale
    const int ticks = juce::roundToInt (s.getMaximum() - s.getMinimum()) == 15 ? 16 : 11;
    g.setColour (brown);
    for (int i = 0; i < ticks; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * static_cast<float> (i) / static_cast<float> (ticks - 1);
        juce::Path t;
        t.addRectangle (-1.0f, -rOut, 2.0f, rOut * 0.16f);
        g.fillPath (t, juce::AffineTransform::rotation (a).translated (c));
    }

    // fluted black body
    const float rBody = rOut * 0.74f;
    g.setColour (juce::Colour (0x8c281c0f));
    g.fillEllipse (juce::Rectangle<float> (rBody * 2, rBody * 2).withCentre (c.translated (0.0f, 3.0f)));
    constexpr int flutes = 28;
    for (int i = 0; i < flutes; ++i)
    {
        juce::Path wedge;
        const float a0 = juce::MathConstants<float>::twoPi * static_cast<float> (i) / flutes;
        wedge.addPieSegment (juce::Rectangle<float> (rBody * 2, rBody * 2).withCentre (c), a0,
                             a0 + juce::MathConstants<float>::twoPi / flutes, 0.0f);
        g.setColour ((i % 2) == 0 ? juce::Colour (0xff1c1a18) : juce::Colour (0xff34302b));
        g.fillPath (wedge);
    }

    // aluminium cap with indicator
    const float rCap = rOut * 0.46f;
    const auto cap = juce::Rectangle<float> (rCap * 2, rCap * 2).withCentre (c);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, cap.getX() + rCap * 0.7f, cap.getY() + rCap * 0.6f,
                                             juce::Colour (0xff8d8d8d), cap.getRight(), cap.getBottom(), true));
    g.fillEllipse (cap);

    const float a = startAngle + pos * (endAngle - startAngle);
    juce::Path ind;
    ind.addRoundedRectangle (-1.2f, -rCap + 2.0f, 2.4f, rCap * 0.7f, 1.0f);
    g.setColour (juce::Colour (0xff1c1a18));
    g.fillPath (ind, juce::AffineTransform::rotation (a).translated (c));
}

void SkinBreadbin::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat();
    g.setColour (brown);
    g.fillRoundedRectangle (r, 4.0f);
    if (b.getToggleState() || down)
    {
        g.setColour (amber);
        g.fillRoundedRectangle (r.reduced (2.0f), 3.0f);
    }
    else if (over)
    {
        g.setColour (amber.withAlpha (0.2f));
        g.fillRoundedRectangle (r.reduced (2.0f), 3.0f);
    }
}

void SkinBreadbin::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    g.setColour ((b.getToggleState() || down) ? keyShadow : beigeText);
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (3), juce::Justification::centred, 1, 0.5f);
}

void SkinBreadbin::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    keycap (g, juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (w), static_cast<float> (h) - 3.0f),
            cream, caseBottom, juce::Colour (0xff8f8060));
    juce::Path arrow;
    const float ax = static_cast<float> (w) - 14.0f, ay = static_cast<float> (h) * 0.5f - 3.0f;
    arrow.addTriangle (ax, ay, ax + 8.0f, ay, ax + 4.0f, ay + 5.0f);
    g.setColour (brown);
    g.fillPath (arrow);
}

void SkinBreadbin::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                     juce::Slider::SliderStyle, juce::Slider&)
{
    // recessed slot with an amber fill and a hardware fader cap
    const auto slot = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y) + static_cast<float> (h) * 0.5f - 3.0f,
                                              static_cast<float> (w), 6.0f);
    g.setColour (bezel);
    g.fillRoundedRectangle (slot, 3.0f);
    g.setColour (amber);
    g.fillRoundedRectangle (slot.withRight (pos).reduced (1.0f), 2.0f);

    auto cap = juce::Rectangle<float> (10.0f, static_cast<float> (h) - 2.0f).withCentre ({ pos, static_cast<float> (y) + static_cast<float> (h) * 0.5f });
    keycap (g, cap.withTrimmedBottom (2.0f), juce::Colour (0xff5a5651), juce::Colour (0xff2a2724), keyShadow);
    g.setColour (cream);
    g.fillRect (cap.withSizeKeepingCentre (6.0f, 1.5f));
}

void SkinBreadbin::paintMeter (juce::Graphics& g, juce::Rectangle<int> r, float level, float peak)
{
    g.setColour (bezel);
    g.fillRoundedRectangle (r.toFloat(), 5.0f);
    auto inner = r.reduced (5, 7).toFloat();
    constexpr int leds = 16;
    const float step = inner.getHeight() / leds;
    const int lit = juce::roundToInt (level * leds);
    const int peakLed = juce::roundToInt (peak * leds) - 1;

    for (int i = 0; i < leds; ++i)
    {
        juce::Colour on  = i >= 14 ? juce::Colour (0xffff4b33) : i >= 10 ? amber : juce::Colour (0xff6fd05a);
        const bool isOn = i < lit || (i == peakLed && peak > 0.01f);
        const float d = juce::jmin (inner.getWidth(), step) - 3.0f;
        auto dot = juce::Rectangle<float> (d, d).withCentre ({ inner.getCentreX(), inner.getBottom() - (static_cast<float> (i) + 0.5f) * step });
        if (isOn)
        {
            g.setColour (on.withAlpha (0.35f));
            g.fillEllipse (dot.expanded (2.0f));
        }
        g.setColour (isOn ? on : on.darker (2.2f));
        g.fillEllipse (dot);
    }
}

void SkinBreadbin::paintEnvelope (juce::Graphics& g, juce::Rectangle<int> r, const std::vector<juce::Point<float>>& curve,
                                  const std::vector<juce::Point<float>>& dots)
{
    const auto rf = r.toFloat();
    g.setColour (bezel);
    g.fillRoundedRectangle (rf, 6.0f);
    const auto scr = rf.reduced (6.0f);
    g.setColour (juce::Colour (0xff1d1a14));
    g.fillRoundedRectangle (scr, 3.0f);

    g.setColour (amber.withAlpha (0.12f));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine (juce::roundToInt (scr.getY() + scr.getHeight() * static_cast<float> (i) / 4.0f), scr.getX(), scr.getRight());

    const auto area = scr.reduced (4.0f, 6.0f);
    const auto pts = toArea (curve, area);
    juce::Path p;
    for (size_t i = 0; i < pts.size(); ++i)
        i == 0 ? p.startNewSubPath (pts[i]) : p.lineTo (pts[i]);
    g.setColour (amber.withAlpha (0.25f));
    g.strokePath (p, juce::PathStrokeType (5.0f));
    g.setColour (amber);
    g.strokePath (p, juce::PathStrokeType (1.8f));

    for (auto d : toArea (dots, area))
    {
        g.setColour (juce::Colour (0x66ff4b33));
        g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre (d));
        g.setColour (juce::Colour (0xffff6a50));
        g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (d));
    }
}

void SkinBreadbin::paintPad (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& name, const juce::String& note,
                             bool selected, float flash)
{
    auto rf = r.toFloat().withTrimmedBottom (3.0f);
    if (selected) keycap (g, rf, juce::Colour (0xfff0c862), juce::Colour (0xffd9a43a), juce::Colour (0xff8a6a28));
    else          keycap (g, rf, juce::Colour (0xffd6c39c), juce::Colour (0xffbfa97f), juce::Colour (0xff8f8060));

    if (flash > 0.0f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.55f * flash));
        g.fillRoundedRectangle (rf.reduced (3.0f), 3.0f);
    }

    g.setColour (keyShadow);
    g.setFont (font (8.0f));
    g.drawText (name, rf.withTrimmedBottom (rf.getHeight() * 0.45f).toNearestInt(), juce::Justification::centredBottom, false);
    g.setColour (rust);
    g.drawText (note, rf.withTrimmedTop (rf.getHeight() * 0.6f).toNearestInt(), juce::Justification::centredTop, false);
}

void SkinBreadbin::styleKeyboard (juce::MidiKeyboardComponent& k)
{
    k.setColour (juce::MidiKeyboardComponent::whiteNoteColourId,              juce::Colour (0xfff1ead8));
    k.setColour (juce::MidiKeyboardComponent::blackNoteColourId,              brown);
    k.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId,       brown);
    k.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,    amber.withAlpha (0.35f));
    k.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,         amber);
    k.setColour (juce::MidiKeyboardComponent::textLabelColourId,              brownLight);
    k.setColour (juce::MidiKeyboardComponent::shadowColourId,                 juce::Colour (0x40000000));
    k.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, brown);
    k.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId,      beigeText);
}

} // namespace zadabar
