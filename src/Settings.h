// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_data_structures/juce_data_structures.h>

namespace zadabar
{

// User preferences shared by every Zadabar64 instance on this computer
// (not saved with DAW projects). Stored in:
//   Windows: %APPDATA%\Zadabar\Zadabar64.settings
//   macOS:   ~/Library/Application Support/Zadabar/Zadabar64.settings
//   Linux:   ~/.config/Zadabar/Zadabar64.settings
//
// Use through juce::SharedResourcePointer<Settings> so all instances share one copy.
class Settings : public juce::ChangeBroadcaster
{
public:
    Settings();

    int  getSkin() const;
    void setSkin (int skinIndex);

    // 0 = Live: no latency, authentic SID (the ADSR bug can delay a note by up to 33 ms)
    // 1 = Tight: "hard restart" 40 ms ahead of every note, reported to the host as latency
    int  getTiming() const;
    void setTiming (int timing);

private:
    juce::ApplicationProperties props;
};

} // namespace zadabar
