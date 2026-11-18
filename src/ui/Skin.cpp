// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Skin.h"
#include "Skins.h"

#include "BinaryData.h"

namespace zadabar
{

Skin::Skin()
{
    static juce::Typeface::Ptr shared = juce::Typeface::createSystemTypefaceFor (
        BinaryData::PressStart2PRegular_ttf, static_cast<size_t> (BinaryData::PressStart2PRegular_ttfSize));
    c64 = shared;
    setDefaultSansSerifTypeface (c64);
}

std::unique_ptr<Skin> Skin::create (int index)
{
    switch (index)
    {
        case 1:  return std::make_unique<SkinBreadbin>();
        case 2:  return std::make_unique<SkinDemoscene>();
        default: return std::make_unique<SkinReady>();
    }
}

juce::String Skin::nameOf (int index)
{
    static const char* names[] = { "READY.", "BREADBIN", "DEMOSCENE" };
    return names[juce::jlimit (0, numSkins - 1, index)];
}

juce::String Skin::descriptionOf (int index)
{
    static const char* text[] = { "C64 START-UP SCREEN",
                                  "THE C64 CASE, 80S KNOBS",
                                  "CRACKTRO, LED RINGS" };
    return text[juce::jlimit (0, numSkins - 1, index)];
}

juce::Font Skin::font (float height) const
{
    return juce::Font (juce::FontOptions().withTypeface (c64).withHeight (height));
}

juce::Font Skin::fittingFont (const juce::String& text, int width) const
{
    return juce::GlyphArrangement::getStringWidthInt (font (16.0f), text) <= width - 6 ? font (16.0f) : font (8.0f);
}

std::unique_ptr<juce::Drawable> Skin::createLogo (juce::Colour colour) const
{
    auto logo = juce::Drawable::createFromImageData (BinaryData::zadabar_z_svg, static_cast<size_t> (BinaryData::zadabar_z_svgSize));
    if (logo != nullptr)
        logo->replaceColour (juce::Colours::white, colour);
    return logo;
}

const juce::Drawable* Skin::cachedLogo (juce::Colour colour) const
{
    auto& slot = logoCache[colour.getARGB()];
    if (slot == nullptr)
        slot = createLogo (colour);
    return slot.get();
}

float Skin::sampleCurve (const std::vector<juce::Point<float>>& c, float x)
{
    if (c.empty())         return 0.0f;
    if (x <= c.front().x)  return c.front().y;
    for (size_t i = 1; i < c.size(); ++i)
        if (x <= c[i].x)
        {
            const float span = c[i].x - c[i - 1].x;
            const float t = span > 0.0f ? (x - c[i - 1].x) / span : 0.0f;
            return c[i - 1].y + t * (c[i].y - c[i - 1].y);
        }
    return c.back().y;
}

std::vector<juce::Point<float>> Skin::toArea (const std::vector<juce::Point<float>>& pts, juce::Rectangle<float> a)
{
    std::vector<juce::Point<float>> out;
    out.reserve (pts.size());
    for (auto p : pts)
        out.emplace_back (a.getX() + p.x * a.getWidth(), a.getBottom() - p.y * a.getHeight());
    return out;
}

// ------------------------------------------------------------------ common LookAndFeel

juce::Font Skin::getLabelFont (juce::Label& l)
{
    const float size = l.getProperties().getWithDefault ("fontSize", 8.0f);
    return font (size);
}

juce::Font Skin::getTextButtonFont (juce::TextButton& b, int)
{
    const float size = b.getProperties().getWithDefault ("fontSize", 8.0f);
    return font (size);
}

juce::Font Skin::getComboBoxFont (juce::ComboBox&) { return font (8.0f); }
juce::Font Skin::getPopupMenuFont()                { return font (8.0f); }

juce::Label* Skin::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setColour (juce::Label::textColourId, valueColour());
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::TextEditor::textColourId, valueColour());
    l->setColour (juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha (0.4f));
    l->setColour (juce::TextEditor::highlightColourId, valueColour().withAlpha (0.3f));
    l->setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::TextEditor::focusedOutlineColourId, valueColour());
    l->setColour (juce::CaretComponent::caretColourId, valueColour());
    return l;
}

void Skin::drawLabel (juce::Graphics& g, juce::Label& label)
{
    if (label.isBeingEdited())
        return;

    g.fillAll (label.findColour (juce::Label::backgroundColourId));
    g.setColour (label.findColour (juce::Label::textColourId));
    const auto f = getLabelFont (label);
    g.setFont (f);
    const int lines = juce::jmax (1, static_cast<int> (static_cast<float> (label.getHeight()) / (f.getHeight() * 1.5f)));
    g.drawFittedText (label.getText(), label.getLocalBounds(), label.getJustificationType(), lines, 1.0f);
}

void Skin::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (1, 1, box.getWidth() - 20, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centred);
}

} // namespace zadabar
