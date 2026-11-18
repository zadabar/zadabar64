// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "patches/Patches.h"

namespace zadabar
{

class Zadabar64Processor;

// Patch browser for the current mode: < NAME > SAVE.
// Clicking the name opens the list (factory + user patches) and the file commands.
class PatchBar : public juce::Component
{
public:
    explicit PatchBar (Zadabar64Processor&);
    void resized() override;
    void refresh();                         // name, number and "modified" mark

    std::function<void (PatchType)> onSaveAs;   // the editor shows the save dialog

private:
    PatchType type() const;
    void showMenu();
    void loadFromFile();

    Zadabar64Processor& processor;
    juce::TextButton prev { "<" }, next { ">" }, name, save { "SAVE" };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String cachedKey;      // type + name + user flag, to avoid re-reading the folder every tick
    int cachedIndex = -1;
};

// "Save patch" dialog, drawn by the skin on top of the editor.
class SavePatchDialog : public juce::Component
{
public:
    explicit SavePatchDialog (Zadabar64Processor&);
    void show (PatchType);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    std::function<void()> onSaved;

private:
    void commit();
    void close();
    void updateHint();
    juce::Rectangle<int> box() const;

    Zadabar64Processor& processor;
    PatchType type = PatchType::Instrument;
    juce::TextEditor nameEditor;
    juce::TextButton ok { "SAVE" }, cancel { "CANCEL" };
    juce::String hint;
};

} // namespace zadabar
