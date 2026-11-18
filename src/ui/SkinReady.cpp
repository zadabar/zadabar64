// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Skin A: "READY." - the C64 start-up screen.

#include "Skins.h"

namespace zadabar
{

namespace
{
    const juce::Colour border   { 0xff6c5eb5 };   // C64 light blue
    const juce::Colour screen   { 0xff352879 };   // C64 blue
    const juce::Colour pale     { 0xffa59ee8 };
    const juce::Colour white    { 0xffffffff };
    const juce::Colour deep     { 0xff2b2063 };
}

SkinReady::SkinReady()
{
    setColour (juce::ComboBox::textColourId,        screen);
    setColour (juce::ComboBox::backgroundColourId,  white);
    setColour (juce::ComboBox::arrowColourId,       screen);
    setColour (juce::ComboBox::outlineColourId,     white);
    setColour (juce::PopupMenu::backgroundColourId, screen);
    setColour (juce::PopupMenu::textColourId,       white);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, pale);
    setColour (juce::PopupMenu::highlightedTextColourId,       screen);
    setColour (juce::Label::textColourId,           pale);
    setColour (juce::TextButton::textColourOffId,   pale);
    setColour (juce::TextButton::textColourOnId,    screen);
}

juce::Colour SkinReady::textColour() const  { return pale; }
juce::Colour SkinReady::valueColour() const { return white; }

void SkinReady::paintBackground (juce::Graphics& g, juce::Rectangle<int> b)
{
    g.fillAll (border);
    g.setColour (screen);
    g.fillRect (b.reduced (14));
}

void SkinReady::paintHeader (juce::Graphics& g, juce::Rectangle<int>, juce::Rectangle<int> logoArea,
                             juce::Rectangle<int> info, const HeaderState& s)
{
    auto la = logoArea;
    if (auto* logo = cachedLogo (pale))
        logo->drawWithin (g, la.removeFromLeft (76).reduced (0, 12).toFloat(), juce::RectanglePlacement::centred, 1.0f);
    la.removeFromLeft (12);

    g.setColour (white);
    g.setFont (font (32.0f));
    g.drawText ("ZADABAR64", la.withTrimmedBottom (18), juce::Justification::bottomLeft, false);
    g.setColour (pale);
    g.setFont (font (8.0f));
    g.drawText ("SID INSTRUMENT BY ZADABAR", la.withTrimmedTop (la.getHeight() - 14), juce::Justification::topLeft, false);

    // info: banner + voice activity
    auto top = info.removeFromTop (info.getHeight() / 2);
    g.setFont (font (16.0f));
    g.drawFittedText ("**** RESIDFP SID " + shortVersion() + " ****", top, juce::Justification::centredBottom, 1, 0.5f);

    g.setFont (font (8.0f));
    auto row = info.withSizeKeepingCentre (130, 16);
    g.drawText ("VOICES", row.removeFromLeft (56), juce::Justification::centredLeft, false);
    for (int v = 0; v < 3; ++v)
    {
        auto box = row.removeFromLeft (24).withTrimmedRight (4);
        if (s.voiceActive[(size_t) v]) { g.setColour (white); g.fillRect (box); g.setColour (screen); }
        else                          { g.setColour (pale);  g.drawRect (box, 2); }
        g.drawText (juce::String (v + 1), box, juce::Justification::centred, false);
        g.setColour (pale);
    }
}

void SkinReady::paintStatusStrip (juce::Graphics& g, juce::Rectangle<int> r, const HeaderState& s)
{
    g.setColour (pale);
    g.setFont (font (16.0f));
    auto left = r.removeFromLeft (120);
    g.drawText ("READY.", left, juce::Justification::centredLeft, false);
    if (s.cursorOn)
        g.fillRect (left.getX() + 104, r.getCentreY() - 8, 16, 16);

    g.setFont (font (8.0f));
    g.drawText ("64K RAM SYSTEM  38911 BASIC BYTES FREE   " + s.chipText, r, juce::Justification::centredRight, false);
}

void SkinReady::paintPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, Section)
{
    g.setColour (pale);
    g.drawRect (r, 4);

    g.setFont (font (16.0f));
    const int w = juce::GlyphArrangement::getStringWidthInt (font (16.0f), title) + 16;
    auto tab = juce::Rectangle<int> (r.getX(), r.getY(), w, 24);
    g.fillRect (tab);
    g.setColour (screen);
    g.drawText (title, tab, juce::Justification::centred, false);
}

void SkinReady::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                  float startAngle, float endAngle, juce::Slider&)
{
    // 16 x 16 "C64 pixels": a stepped LED ring of 16 blocks, a round body and a pointer.
    const int px   = juce::jmax (2, juce::jmin (w, h, 72) / 16);
    const int size = px * 16;
    const int ox   = x + (w - size) / 2;
    const int oy   = y + (h - size) / 2;
    const float c  = 7.5f;

    auto pixel = [&] (int gx, int gy, juce::Colour col)
    {
        g.setColour (col);
        g.fillRect (ox + gx * px, oy + gy * px, px, px);
    };

    const int lit = juce::roundToInt (pos * 15.0f);
    for (int i = 0; i < 16; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * static_cast<float> (i) / 15.0f;
        pixel (juce::roundToInt (c + 7.0f * std::sin (a)), juce::roundToInt (c - 7.0f * std::cos (a)), i <= lit ? white : border);
    }

    for (int gy = 0; gy < 16; ++gy)
        for (int gx = 0; gx < 16; ++gx)
        {
            const float d = std::hypot (static_cast<float> (gx) - c, static_cast<float> (gy) - c);
            if (d <= 4.7f)
                pixel (gx, gy, d > 3.9f ? border : pale);
        }

    const float a = startAngle + pos * (endAngle - startAngle);
    for (int t = 1; t <= 4; ++t)
        pixel (juce::roundToInt (c + static_cast<float> (t) * 0.95f * std::sin (a)),
               juce::roundToInt (c - static_cast<float> (t) * 0.95f * std::cos (a)), screen);
}

void SkinReady::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds();
    if (b.getToggleState() || down)
    {
        g.setColour (white);
        g.fillRect (r);
    }
    else
    {
        if (over) { g.setColour (pale.withAlpha (0.18f)); g.fillRect (r); }
        g.setColour (pale);
        g.drawRect (r, 2);
    }
}

void SkinReady::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    g.setColour ((b.getToggleState() || down) ? screen : pale);
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (2), juce::Justification::centred, 1, 0.5f);
}

void SkinReady::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    g.setColour (white);
    g.fillRect (0, 0, w, h);
    g.setColour (screen);
    g.setFont (font (8.0f));
    g.drawText ("<", 4, 0, 10, h, juce::Justification::centredLeft, false);
    g.drawText (">", w - 14, 0, 10, h, juce::Justification::centredRight, false);
}

void SkinReady::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                  juce::Slider::SliderStyle, juce::Slider&)
{
    // a row of 4-pixel blocks, lit up to the value
    auto r = juce::Rectangle<int> (x, y, w, h).withSizeKeepingCentre (w, 12);
    g.setColour (pale);
    g.drawRect (r, 2);
    auto inner = r.reduced (4, 4);
    const int blocks = inner.getWidth() / 6;
    const int lit = juce::roundToInt ((pos - static_cast<float> (x)) / static_cast<float> (w) * static_cast<float> (blocks));
    for (int i = 0; i < blocks; ++i)
    {
        g.setColour (i < lit ? white : deep);
        g.fillRect (inner.getX() + i * 6, inner.getY(), 4, inner.getHeight());
    }
}

void SkinReady::paintMeter (juce::Graphics& g, juce::Rectangle<int> r, float level, float peak)
{
    g.setColour (pale);
    g.drawRect (r, 2);
    auto inner = r.reduced (4);
    constexpr int segs = 16;
    const float segH = static_cast<float> (inner.getHeight()) / segs;
    const int lit  = juce::roundToInt (level * segs);
    const int peakSeg = juce::jlimit (0, segs - 1, juce::roundToInt (peak * segs) - 1);

    for (int i = 0; i < segs; ++i)
    {
        auto seg = juce::Rectangle<float> (static_cast<float> (inner.getX()), static_cast<float> (inner.getBottom()) - (i + 1) * segH,
                                           static_cast<float> (inner.getWidth()), segH).reduced (0.0f, 1.0f);
        if (i < lit)
            g.setColour (i >= segs - 2 ? white : pale);
        else if (i == peakSeg && peak > 0.01f)
            g.setColour (white);
        else
            g.setColour (deep);
        g.fillRect (seg);
    }
}

void SkinReady::paintEnvelope (juce::Graphics& g, juce::Rectangle<int> r, const std::vector<juce::Point<float>>& curve,
                               const std::vector<juce::Point<float>>& dots)
{
    // Block graphics: 4-pixel columns, heights snapped to 4 pixels.
    const auto area = r.reduced (2);
    for (int x = 0; x < area.getWidth(); x += 4)
    {
        const float v = sampleCurve (curve, (static_cast<float> (x) + 2.0f) / static_cast<float> (area.getWidth()));
        const int hh = (juce::roundToInt (v * static_cast<float> (area.getHeight())) / 4) * 4;
        g.setColour (pale);
        g.fillRect (area.getX() + x, area.getBottom() - hh, 3, hh);
    }
    g.setColour (border);
    g.fillRect (area.getX(), area.getBottom(), area.getWidth(), 2);

    g.setColour (white);
    for (auto d : toArea (dots, area.toFloat()))
        g.fillRect (juce::Rectangle<float> (8.0f, 8.0f).withCentre (d));
}

void SkinReady::paintScope (juce::Graphics& g, juce::Rectangle<int> r, const std::vector<juce::Point<float>>& trace)
{
    // Block graphics again: one 4x4 "pixel" per column, gaps filled so edges stay solid.
    const auto area = r.reduced (2);
    g.setColour (border);
    g.drawRect (area, 2);
    const auto in = area.reduced (6);
    for (int x = 0; x < in.getWidth(); x += 8)
        g.fillRect (in.getX() + x, in.getCentreY() - 1, 4, 2);

    const int cols = in.getWidth() / 4;
    int prev = -1;
    g.setColour (pale);
    for (int i = 0; i < cols; ++i)
    {
        const float v = sampleCurve (trace, (static_cast<float> (i) + 0.5f) / static_cast<float> (cols));
        const int y = (juce::roundToInt ((1.0f - v) * static_cast<float> (in.getHeight() - 4)) / 4) * 4;
        const int top = prev < 0 ? y : std::min (y, prev), bottom = prev < 0 ? y : std::max (y, prev);
        g.fillRect (in.getX() + i * 4, in.getY() + top, 4, bottom - top + 4);
        prev = y;
    }
}

void SkinReady::paintPad (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& name, const juce::String& note,
                          bool selected, float flash)
{
    if (selected) { g.setColour (white); g.fillRect (r); }
    else          { g.setColour (pale);  g.drawRect (r, 2); }

    if (flash > 0.0f)
    {
        g.setColour (pale.withAlpha (0.7f * flash));
        g.fillRect (r.reduced (4));
    }

    g.setColour (selected ? screen : pale);
    g.setFont (fittingFont (name, r.getWidth()));
    g.drawText (name, r.withTrimmedBottom (r.getHeight() / 3), juce::Justification::centredBottom, false);
    g.setFont (font (8.0f));
    g.drawText (note, r.withTrimmedTop (r.getHeight() * 2 / 3), juce::Justification::centredTop, false);
}

void SkinReady::styleKeyboard (juce::MidiKeyboardComponent& k)
{
    k.setColour (juce::MidiKeyboardComponent::whiteNoteColourId,              pale);
    k.setColour (juce::MidiKeyboardComponent::blackNoteColourId,              screen);
    k.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId,       screen);
    k.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,    white.withAlpha (0.35f));
    k.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,         white);
    k.setColour (juce::MidiKeyboardComponent::textLabelColourId,              screen);
    k.setColour (juce::MidiKeyboardComponent::shadowColourId,                 juce::Colours::transparentBlack);
    k.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, screen);
    k.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId,      pale);
}

} // namespace zadabar
