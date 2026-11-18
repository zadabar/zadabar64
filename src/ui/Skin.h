// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <map>

#include <array>
#include <memory>
#include <vector>

#include <juce_audio_utils/juce_audio_utils.h>

namespace zadabar
{

// Which part of the plugin a control belongs to. Some skins colour-code sections.
enum class Section { Chip = 0, Osc, Env, Filter, Out, Drums, Setup, Mod, Vibrato, Glide, Arp };

// Tags a component with its section (read by the skins when drawing it).
inline void setSection (juce::Component& c, Section s)  { c.getProperties().set ("section", static_cast<int> (s)); }
inline Section getSection (const juce::Component& c)
{
    // look up the parent chain, so a slider's text box inherits its slider's section
    for (auto* p = &c; p != nullptr; p = p->getParentComponent())
        if (p->getProperties().contains ("section"))
            return static_cast<Section> (static_cast<int> (p->getProperties()["section"]));
    return Section::Chip;
}

// Things the header shows that change while playing.
struct HeaderState
{
    std::array<bool, 3> voiceActive {};
    bool  cursorOn   = true;    // blinking cursor
    float scrollPos  = 0.0f;    // demoscene scroller, in pixels
    juce::String chipText;      // e.g. "6581 PAL"
    bool  drumMode   = false;
};

// A skin = a LookAndFeel (knobs, buttons, combo boxes) plus painters for the
// plugin's own surfaces (background, header, panels, meter, envelope, pads).
// All skins share one layout; only the drawing differs.
class Skin : public juce::LookAndFeel_V4
{
public:
    Skin();
    ~Skin() override = default;

    static constexpr int numSkins = 3;
    static std::unique_ptr<Skin> create (int index);
    static juce::String nameOf (int index);
    static juce::String descriptionOf (int index);

    // --- shared helpers
    juce::Font font (float height) const;                      // the C64 font
    juce::Font fittingFont (const juce::String& text, int width) const;   // 16 px if it fits, else 8 px
    std::unique_ptr<juce::Drawable> createLogo (juce::Colour) const;
    const juce::Drawable* cachedLogo (juce::Colour) const;     // parsed once per colour, not on every paint

    // true if the status strip moves on its own (the Demoscene scroller): repainted every frame
    virtual bool animatesStatusStrip() const { return false; }

    // --- surfaces, implemented by each skin
    virtual void paintBackground (juce::Graphics&, juce::Rectangle<int> bounds) = 0;
    virtual void paintHeader (juce::Graphics&, juce::Rectangle<int> header, juce::Rectangle<int> logoArea,
                              juce::Rectangle<int> infoArea, const HeaderState&) = 0;
    virtual void paintStatusStrip (juce::Graphics&, juce::Rectangle<int>, const HeaderState&) = 0;
    virtual void paintPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String& title, Section) = 0;
    virtual void paintMeter (juce::Graphics&, juce::Rectangle<int>, float level01, float peak01) = 0;
    virtual void paintEnvelope (juce::Graphics&, juce::Rectangle<int>, const std::vector<juce::Point<float>>& curve01,
                                const std::vector<juce::Point<float>>& dots01) = 0;
    // Oscilloscope trace, points in 0..1 (y 0.5 = centre line). Defaults to the envelope screen.
    virtual void paintScope (juce::Graphics& g, juce::Rectangle<int> r, const std::vector<juce::Point<float>>& trace01)
    {
        paintEnvelope (g, r, trace01, {});
    }
    virtual void paintPad (juce::Graphics&, juce::Rectangle<int>, const juce::String& name, const juce::String& note,
                           bool selected, float flash01) = 0;
    virtual void styleKeyboard (juce::MidiKeyboardComponent&) = 0;

    virtual int panelTitleHeight() const { return 26; }
    virtual juce::Colour textColour() const = 0;       // labels
    virtual juce::Colour valueColour() const = 0;      // knob values

    // --- common LookAndFeel overrides (fonts everywhere)
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    // Height of a 0..1 polyline at x (linear interpolation).
    static float sampleCurve (const std::vector<juce::Point<float>>& curve01, float x);

    // Map the curve's 0..1 coordinates into a rectangle (y up).
    static std::vector<juce::Point<float>> toArea (const std::vector<juce::Point<float>>& pts01, juce::Rectangle<float> area);

protected:
    juce::Typeface::Ptr c64;
    mutable std::map<juce::uint32, std::unique_ptr<juce::Drawable>> logoCache;
};

} // namespace zadabar
