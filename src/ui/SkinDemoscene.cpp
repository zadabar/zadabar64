// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Skin C: "Demoscene" - a cracktro with raster bars and LED-ring knobs.

#include "Skins.h"

namespace zadabar
{

namespace
{
    const juce::Colour black     { 0xff000000 };
    const juce::Colour panelBg   { 0xff0a0a0a };
    const juce::Colour grey      { 0xffb0b0b0 };
    const juce::Colour dimLed    { 0xff222222 };
    const juce::Colour c64Blue   { 0xff352879 };
    const juce::Colour c64Light  { 0xff6c5eb5 };
    const juce::Colour c64Pale   { 0xffa59ee8 };
    const juce::Colour cyan      { 0xff70a4b2 };
    const juce::Colour yellow    { 0xffb8c76f };

    const char* const scrollText =
        "*** ZADABAR PRESENTS ZADABAR64 *** THE RESIDFP POWERED SID INSTRUMENT *** 6581 + 8580 *** "
        "DRUM MODE INSIDE *** GREETINGS TO ALL SID FREAKS AROUND THE WORLD ***      ";

    void glowStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c)
    {
        g.setColour (c.withAlpha (0.18f)); g.strokePath (p, juce::PathStrokeType (7.0f));
        g.setColour (c.withAlpha (0.35f)); g.strokePath (p, juce::PathStrokeType (4.0f));
        g.setColour (c);                   g.strokePath (p, juce::PathStrokeType (1.8f));
    }
}

juce::Colour SkinDemoscene::accent (Section s)
{
    switch (s)
    {
        case Section::Chip:   return cyan;
        case Section::Osc:    return juce::Colour (0xff9ad284);
        case Section::Env:    return yellow;
        case Section::Filter: return juce::Colour (0xffa77bcf);
        case Section::Out:    return juce::Colour (0xffc18a7c);
        case Section::Drums:  return juce::Colour (0xffd9a066);
        case Section::Setup:  return c64Pale;
        case Section::Mod:    return juce::Colour (0xffe07b9b);
        case Section::Vibrato:return juce::Colour (0xff7fd1c7);
        case Section::Glide:  return juce::Colour (0xffc9b0f0);
        case Section::Arp:    return juce::Colour (0xfff0a35e);
        case Section::Midi:   return juce::Colour (0xff8fb8f0);
    }
    return cyan;
}

SkinDemoscene::SkinDemoscene()
{
    setColour (juce::ComboBox::textColourId,        juce::Colours::white);
    setColour (juce::ComboBox::backgroundColourId,  black);
    setColour (juce::PopupMenu::backgroundColourId, black);
    setColour (juce::PopupMenu::textColourId,       juce::Colours::white);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, c64Light);
    setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);
    setColour (juce::Label::textColourId,           grey);
}

juce::Colour SkinDemoscene::textColour() const  { return grey; }
juce::Colour SkinDemoscene::valueColour() const { return juce::Colours::white; }

void SkinDemoscene::paintBackground (juce::Graphics& g, juce::Rectangle<int>)
{
    g.fillAll (black);
}

void SkinDemoscene::paintHeader (juce::Graphics& g, juce::Rectangle<int> header, juce::Rectangle<int> logoArea,
                                 juce::Rectangle<int> info, const HeaderState& s)
{
    // raster bars behind everything
    const auto hf = header.toFloat();
    juce::ColourGradient bars (black, 0.0f, hf.getY(), black, 0.0f, hf.getBottom(), false);
    const std::pair<double, juce::Colour> stops[] = {
        { 0.08, c64Blue }, { 0.16, c64Light }, { 0.22, c64Pale }, { 0.25, juce::Colours::white }, { 0.28, c64Pale },
        { 0.34, c64Light }, { 0.42, c64Blue }, { 0.50, black }, { 0.52, black }, { 0.58, juce::Colour (0xff6f3d86) },
        { 0.66, juce::Colour (0xff9a6759) }, { 0.73, yellow }, { 0.75, juce::Colours::white }, { 0.77, yellow },
        { 0.84, juce::Colour (0xff9a6759) }, { 0.92, juce::Colour (0xff6f3d86) } };
    for (auto& st : stops)
        bars.addColour (st.first, st.second);
    g.setGradientFill (bars);
    g.fillRect (header.withX (0).withWidth (header.getRight() + header.getX()));

    // logo box
    auto box = logoArea.reduced (0, 6);
    g.setColour (black);
    g.fillRect (box);
    auto inner = box.reduced (12, 6);
    if (auto* logo = cachedLogo (juce::Colours::white))
        logo->drawWithin (g, inner.removeFromLeft (66).toFloat(), juce::RectanglePlacement::centred, 1.0f);
    inner.removeFromLeft (10);
    g.setFont (font (32.0f));
    g.setColour (c64Light);
    g.drawText ("ZADABAR64", inner.withTrimmedBottom (12).translated (3, 3), juce::Justification::bottomLeft, false);
    g.setColour (juce::Colours::white);
    g.drawText ("ZADABAR64", inner.withTrimmedBottom (12), juce::Justification::bottomLeft, false);
    g.setColour (cyan);
    g.setFont (font (8.0f));
    g.drawText ("A ZADABAR PRODUCTION", inner.withTrimmedTop (inner.getHeight() - 10), juce::Justification::topLeft, false);

    // voice activity box
    auto vb = info.withSizeKeepingCentre (juce::jmin (info.getWidth(), 210), info.getHeight() - 16);
    g.setColour (black);
    g.fillRect (vb);
    auto cols = vb.reduced (8, 6);
    const int cw = cols.getWidth() / 3;
    for (int v = 0; v < 3; ++v)
    {
        auto col = cols.removeFromLeft (cw).reduced (4, 0);
        auto label = col.removeFromBottom (10);
        g.setColour (juce::Colour (0xff9ad284));
        g.setFont (font (8.0f));
        g.drawText ("V" + juce::String (v + 1), label, juce::Justification::centred, false);
        col.removeFromBottom (3);
        const bool on = s.voiceActive[(size_t) v];
        for (int i = 0; i < 5; ++i)
        {
            auto seg = col.removeFromBottom (col.getHeight() / (5 - i)).reduced (0, 1);
            g.setColour (on ? juce::Colour (0xff9ad284).withAlpha (1.0f - 0.12f * static_cast<float> (i)) : dimLed);
            g.fillRect (seg);
        }
    }
}

void SkinDemoscene::paintStatusStrip (juce::Graphics& g, juce::Rectangle<int> r, const HeaderState& s)
{
    g.setColour (c64Blue);
    g.fillRect (r.removeFromTop (2));
    g.fillRect (r.removeFromBottom (2));

    g.saveState();
    g.reduceClipRegion (r);
    g.setFont (font (16.0f));
    g.setColour (yellow);
    const juce::String text (scrollText);
    const float w = juce::GlyphArrangement::getStringWidth (font (16.0f), text);
    float x = static_cast<float> (r.getX()) - std::fmod (s.scrollPos, w);
    for (; x < static_cast<float> (r.getRight()); x += w)
        g.drawText (text, juce::Rectangle<float> (x, static_cast<float> (r.getY()), w + 4.0f, static_cast<float> (r.getHeight())),
                    juce::Justification::centredLeft, false);
    g.restoreState();
}

void SkinDemoscene::paintPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, Section s)
{
    const auto a = accent (s);
    g.setColour (panelBg);
    g.fillRect (r);
    g.setColour (a);
    g.drawRect (r, 2);
    g.fillRect (r.getX() + 8, r.getY() + 9, 8, 8);
    g.setColour (juce::Colours::white);
    // shrink long titles ("RING/SYNC" in a narrow panel) instead of cutting them off
    float size = 16.0f;
    while (size > 9.0f && juce::GlyphArrangement::getStringWidthInt (font (size), title) > r.getWidth() - 28)
        size -= 1.0f;
    g.setFont (font (size));
    g.drawText (title, r.getX() + 22, r.getY() + 4, r.getWidth() - 26, 18, juce::Justification::centredLeft, false);
}

void SkinDemoscene::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                      float startAngle, float endAngle, juce::Slider& s)
{
    const auto a = accent (getSection (s));
    const float size = static_cast<float> (juce::jmin (w, h, 76));
    const auto  c    = juce::Rectangle<int> (x, y, w, h).toFloat().getCentre();
    const float r    = size * 0.5f;

    // LED ring
    const int lit = juce::roundToInt (pos * 15.0f);
    for (int i = 0; i < 16; ++i)
    {
        const float ang = startAngle + (endAngle - startAngle) * static_cast<float> (i) / 15.0f;
        const auto p = c + juce::Point<float> (std::sin (ang), -std::cos (ang)) * (r - 4.0f);
        if (i <= lit)
        {
            g.setColour (a.withAlpha (0.3f));
            g.fillEllipse (juce::Rectangle<float> (11.0f, 11.0f).withCentre (p));
            g.setColour (a);
        }
        else
        {
            g.setColour (dimLed);
        }
        g.fillEllipse (juce::Rectangle<float> (5.5f, 5.5f).withCentre (p));
    }

    // body
    const float rb = r * 0.64f;
    g.setColour (juce::Colour (0xff444444));
    g.fillEllipse (juce::Rectangle<float> (rb * 2 + 4, rb * 2 + 4).withCentre (c));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3a), c.x, c.y - rb * 0.3f, juce::Colour (0xff121212), c.x, c.y + rb, true));
    g.fillEllipse (juce::Rectangle<float> (rb * 2, rb * 2).withCentre (c));

    // coloured cap + pointer
    const float rc = rb * 0.56f;
    g.setColour (a);
    g.fillEllipse (juce::Rectangle<float> (rc * 2, rc * 2).withCentre (c));
    const float ang = startAngle + pos * (endAngle - startAngle);
    juce::Path ptr;
    ptr.addRectangle (-1.2f, -rc + 1.5f, 2.4f, rc * 0.75f);
    g.setColour (black);
    g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (c));
}

void SkinDemoscene::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    const auto a = accent (getSection (b));
    auto r = b.getLocalBounds();
    if (b.getToggleState() || down) { g.setColour (a); g.fillRect (r); }
    else
    {
        g.setColour (over ? a.withAlpha (0.18f) : black);
        g.fillRect (r);
        g.setColour (a);
        g.drawRect (r, 2);
    }
}

void SkinDemoscene::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    const auto a = accent (getSection (b));
    g.setColour ((b.getToggleState() || down) ? black : a);
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (2), juce::Justification::centred, 1, 0.5f);
}

void SkinDemoscene::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto a = accent (getSection (box));
    g.setColour (black);
    g.fillRect (0, 0, w, h);
    g.setColour (a);
    g.drawRect (0, 0, w, h, 2);
    juce::Path arrow;
    const float ax = static_cast<float> (w) - 14.0f, ay = static_cast<float> (h) * 0.5f - 3.0f;
    arrow.addTriangle (ax, ay, ax + 8.0f, ay, ax + 4.0f, ay + 5.0f);
    g.fillPath (arrow);
}

void SkinDemoscene::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                      juce::Slider::SliderStyle, juce::Slider& s)
{
    const auto a = accent (getSection (s));
    auto r = juce::Rectangle<int> (x, y, w, h).withSizeKeepingCentre (w, 10);
    const int segs = juce::jmax (4, r.getWidth() / 7);
    const float segW = static_cast<float> (r.getWidth()) / static_cast<float> (segs);
    const int lit = juce::roundToInt ((pos - static_cast<float> (x)) / static_cast<float> (w) * static_cast<float> (segs));
    for (int i = 0; i < segs; ++i)
    {
        auto seg = juce::Rectangle<float> (static_cast<float> (r.getX()) + static_cast<float> (i) * segW, static_cast<float> (r.getY()),
                                           segW - 2.0f, static_cast<float> (r.getHeight()));
        if (i < lit)
        {
            g.setColour (a.withAlpha (0.3f));
            g.fillRect (seg.expanded (1.0f, 2.0f));
            g.setColour (a);
        }
        else
        {
            g.setColour (dimLed);
        }
        g.fillRect (seg);
    }
}

void SkinDemoscene::paintMeter (juce::Graphics& g, juce::Rectangle<int> r, float level, float peak)
{
    g.setColour (black);
    g.fillRect (r);
    g.setColour (c64Blue);
    g.drawRect (r, 2);

    const juce::Colour ramp[] = { c64Blue, c64Light, cyan, juce::Colour (0xff9ad284), yellow,
                                  juce::Colour (0xffd9a066), juce::Colour (0xffc18a7c), juce::Colour (0xffff5544) };
    auto inner = r.reduced (4).toFloat();
    constexpr int segs = 16;
    const float segH = inner.getHeight() / segs;
    const int lit = juce::roundToInt (level * segs);
    const int peakSeg = juce::roundToInt (peak * segs) - 1;

    for (int i = 0; i < segs; ++i)
    {
        auto seg = juce::Rectangle<float> (inner.getX(), inner.getBottom() - (static_cast<float> (i) + 1.0f) * segH,
                                           inner.getWidth(), segH).reduced (0.0f, 1.0f);
        const auto col = ramp[i / 2];
        if (i < lit || (i == peakSeg && peak > 0.01f))
        {
            g.setColour (col.withAlpha (0.35f));
            g.fillRect (seg.expanded (2.0f, 0.5f));
            g.setColour (col);
        }
        else
        {
            g.setColour (dimLed);
        }
        g.fillRect (seg);
    }
}

void SkinDemoscene::paintEnvelope (juce::Graphics& g, juce::Rectangle<int> r, const std::vector<juce::Point<float>>& curve,
                                   const std::vector<juce::Point<float>>& dots)
{
    const auto a = accent (Section::Env);
    g.setColour (black);
    g.fillRect (r);
    g.setColour (a.withAlpha (0.12f));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine (r.getY() + r.getHeight() * i / 4, static_cast<float> (r.getX()), static_cast<float> (r.getRight()));

    const auto area = r.toFloat().reduced (6.0f, 6.0f);
    const auto pts = toArea (curve, area);
    juce::Path p;
    for (size_t i = 0; i < pts.size(); ++i)
        i == 0 ? p.startNewSubPath (pts[i]) : p.lineTo (pts[i]);
    glowStroke (g, p, a);

    for (auto d : toArea (dots, area))
    {
        g.setColour (juce::Colours::white.withAlpha (0.3f));
        g.fillEllipse (juce::Rectangle<float> (16.0f, 16.0f).withCentre (d));
        g.setColour (juce::Colours::white);
        g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (d));
    }
}

void SkinDemoscene::paintScope (juce::Graphics& g, juce::Rectangle<int> r, const std::vector<juce::Point<float>>& trace)
{
    const auto a = accent (Section::Osc);
    g.setColour (black);
    g.fillRect (r);
    g.setColour (a.withAlpha (0.12f));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine (r.getY() + r.getHeight() * i / 4, static_cast<float> (r.getX()), static_cast<float> (r.getRight()));

    const auto pts = toArea (trace, r.toFloat().reduced (6.0f, 6.0f));
    juce::Path p;
    for (size_t i = 0; i < pts.size(); ++i)
        i == 0 ? p.startNewSubPath (pts[i]) : p.lineTo (pts[i]);
    glowStroke (g, p, a);
}

void SkinDemoscene::paintPad (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& name, const juce::String& note,
                              bool selected, float flash)
{
    const auto a = accent (Section::Drums);
    if (selected) { g.setColour (a); g.fillRect (r); }
    else          { g.setColour (black); g.fillRect (r); g.setColour (a); g.drawRect (r, 2); }

    if (flash > 0.0f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.5f * flash));
        g.fillRect (r.reduced (3));
    }

    g.setColour (selected ? black : juce::Colours::white);
    g.setFont (fittingFont (name, r.getWidth()));
    g.drawText (name, r.withTrimmedBottom (r.getHeight() / 3), juce::Justification::centredBottom, false);
    g.setColour (selected ? black : a);
    g.setFont (font (8.0f));
    g.drawText (note, r.withTrimmedTop (r.getHeight() * 2 / 3), juce::Justification::centredTop, false);
}

void SkinDemoscene::styleKeyboard (juce::MidiKeyboardComponent& k)
{
    k.setColour (juce::MidiKeyboardComponent::whiteNoteColourId,              juce::Colour (0xff1d1d1d));
    k.setColour (juce::MidiKeyboardComponent::blackNoteColourId,              black);
    k.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId,       c64Blue);
    k.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,    cyan.withAlpha (0.35f));
    k.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,         cyan);
    k.setColour (juce::MidiKeyboardComponent::textLabelColourId,              grey);
    k.setColour (juce::MidiKeyboardComponent::shadowColourId,                 juce::Colours::transparentBlack);
    k.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, black);
    k.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId,      cyan);
}

} // namespace zadabar
