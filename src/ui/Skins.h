// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Skin.h"

namespace zadabar
{

#define ZADABAR_SKIN_OVERRIDES                                                                                   \
    void paintBackground (juce::Graphics&, juce::Rectangle<int>) override;                                       \
    void paintHeader (juce::Graphics&, juce::Rectangle<int>, juce::Rectangle<int>, juce::Rectangle<int>,         \
                      const HeaderState&) override;                                                              \
    void paintStatusStrip (juce::Graphics&, juce::Rectangle<int>, const HeaderState&) override;                  \
    void paintPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String&, Section) override;              \
    void paintMeter (juce::Graphics&, juce::Rectangle<int>, float, float) override;                              \
    void paintEnvelope (juce::Graphics&, juce::Rectangle<int>, const std::vector<juce::Point<float>>&,            \
                        const std::vector<juce::Point<float>>&) override;                                        \
    void paintPad (juce::Graphics&, juce::Rectangle<int>, const juce::String&, const juce::String&, bool, float) override; \
    void styleKeyboard (juce::MidiKeyboardComponent&) override;                                                  \
    juce::Colour textColour() const override;                                                                    \
    juce::Colour valueColour() const override;                                                                   \
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;     \
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;        \
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;                               \
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;          \
    void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float,                             \
                           juce::Slider::SliderStyle, juce::Slider&) override;

// A: the C64 start-up screen. Pixel knobs, inverse-video tabs.
class SkinReady : public Skin
{
public:
    SkinReady();
    void paintScope (juce::Graphics&, juce::Rectangle<int>, const std::vector<juce::Point<float>>&) override;
    ZADABAR_SKIN_OVERRIDES
};

// B: the C64 "breadbin" computer case. Ribbed 80s knobs with aluminium caps.
class SkinBreadbin : public Skin
{
public:
    SkinBreadbin();
    int panelTitleHeight() const override { return 30; }
    ZADABAR_SKIN_OVERRIDES
};

// C: a demoscene cracktro. Raster bars, scroller, LED-ring knobs.
class SkinDemoscene : public Skin
{
public:
    SkinDemoscene();
    void paintScope (juce::Graphics&, juce::Rectangle<int>, const std::vector<juce::Point<float>>&) override;
    bool animatesStatusStrip() const override { return true; }   // the scroller
    ZADABAR_SKIN_OVERRIDES

    static juce::Colour accent (Section);
};

#undef ZADABAR_SKIN_OVERRIDES

} // namespace zadabar
