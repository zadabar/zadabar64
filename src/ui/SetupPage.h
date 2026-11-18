// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Skin.h"

namespace zadabar
{

// The SETUP screen, shown over the panels. Settings here are user preferences
// (shared by all instances), not part of the DAW project.
class SetupPage : public juce::Component
{
public:
    SetupPage();

    void setCurrentSkin (int index);
    void setCurrentTiming (int timing);
    std::function<void (int)> onSkinChosen;
    std::function<void (int)> onTimingChosen;
    std::function<void()>     onClose;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::OwnedArray<juce::TextButton> skinButtons;
    juce::OwnedArray<juce::Label>      skinDescriptions;
    juce::Label      skinHeading;
    juce::Label      timingHeading;
    juce::Rectangle<int> timingHelpArea;
    juce::TextButton timingLive { "LIVE" }, timingTight { "TIGHT" };
    juce::TextButton closeButton { "CLOSE" };
};

} // namespace zadabar
