// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PatchBar.h"

#include "PluginProcessor.h"
#include "Widgets.h"

namespace zadabar
{

// ------------------------------------------------------------------ PatchBar

PatchBar::PatchBar (Zadabar64Processor& p) : processor (p)
{
    for (auto* b : { &prev, &next, &name, &save })
    {
        setSection (*b, Section::Setup);
        b->getProperties().set ("fontSize", 8.0f);
        addAndMakeVisible (b);
    }
    name.getProperties().set ("fontSize", 8.0f);
    prev.setTooltip ("Previous patch");
    next.setTooltip ("Next patch");
    name.setTooltip ("All patches, load from file, open the patch folder");
    save.setTooltip ("Save the current sound as a user patch");

    prev.onClick = [this] { processor.patches.step (type(), -1); refresh(); };
    next.onClick = [this] { processor.patches.step (type(), +1); refresh(); };
    name.onClick = [this] { showMenu(); };
    save.onClick = [this] { if (onSaveAs) onSaveAs (type()); };
    refresh();
}

PatchType PatchBar::type() const
{
    return Params::asInt (*processor.apvts.getRawParameterValue (ParamID::mode)) == 1 ? PatchType::DrumKit
                                                                                       : PatchType::Instrument;
}

void PatchBar::resized()
{
    auto r = getLocalBounds();
    save.setBounds (r.removeFromRight (64));
    r.removeFromRight (6);
    next.setBounds (r.removeFromRight (28));
    r.removeFromRight (4);
    prev.setBounds (r.removeFromLeft (28));
    r.removeFromLeft (4);
    name.setBounds (r);
}

void PatchBar::refresh()
{
    auto& pm = processor.patches;
    const auto t = type();
    const auto key = juce::String ((int) t) + pm.currentName (t) + (pm.currentIsUser (t) ? "u" : "f");
    if (key != cachedKey)
    {
        cachedKey = key;
        cachedIndex = pm.currentIndex (t);
    }
    const int index = cachedIndex;
    const auto number = index >= 0 ? juce::String (index + 1).paddedLeft ('0', 3) + " " : juce::String();
    const auto text = number + pm.currentName (t) + (pm.isModified (t) ? " *" : "");
    if (name.getButtonText() != text)
        name.setButtonText (text);
}

void PatchBar::showMenu()
{
    auto& pm = processor.patches;
    const auto t = type();
    const auto entries = pm.list (t);
    const int currentIndex = pm.currentIndex (t);

    juce::PopupMenu menu;
    menu.setLookAndFeel (&getLookAndFeel());
    menu.addSectionHeader (t == PatchType::Instrument ? "FACTORY PATCHES" : "FACTORY KITS");
    bool userHeader = false;
    for (int i = 0; i < static_cast<int> (entries.size()); ++i)
    {
        const auto& e = entries[(size_t) i];
        if (! e.factory && ! userHeader)
        {
            menu.addSectionHeader ("YOUR PATCHES");
            userHeader = true;
        }
        menu.addItem (i + 1, juce::String (i + 1).paddedLeft ('0', 3) + " " + e.name, true, i == currentIndex);
    }
    if (! userHeader)
    {
        menu.addSectionHeader ("YOUR PATCHES");
        menu.addItem (-1, "(NONE YET - USE SAVE)", false);
    }

    constexpr int cmdSave = 10001, cmdLoad = 10002, cmdFolder = 10003, cmdDelete = 10004;
    menu.addSeparator();
    menu.addItem (cmdSave, "SAVE AS...");
    menu.addItem (cmdLoad, "LOAD FROM FILE...");
    menu.addItem (cmdFolder, "OPEN PATCH FOLDER");
    if (pm.currentIsUser (t) && currentIndex >= 0)
        menu.addItem (cmdDelete, "DELETE " + pm.currentName (t));

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&name),
        [this, t, entries] (int result)
        {
            auto& patches = processor.patches;
            if (result >= 1 && result <= static_cast<int> (entries.size()))
                patches.load (t, entries[(size_t) (result - 1)]);
            else if (result == cmdSave && onSaveAs)
                onSaveAs (t);
            else if (result == cmdLoad)
                loadFromFile();
            else if (result == cmdFolder)
            {
                const auto folder = PatchManager::userFolder (t);
                folder.createDirectory();
                folder.startAsProcess();
            }
            else if (result == cmdDelete)
            {
                const int i = patches.currentIndex (t);
                if (i >= 0)
                    patches.remove (t, patches.list (t)[(size_t) i]);
            }
            cachedKey.clear();   // the list may have changed
            refresh();
        });
}

void PatchBar::loadFromFile()
{
    const auto t = type();
    chooser = std::make_unique<juce::FileChooser> ("Load a Zadabar64 patch", PatchManager::userFolder (t),
                                                   "*.z64patch;*.z64kit");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (file.existsAsFile())
            {
                // a kit file switches to drum mode, a patch file to instrument mode
                const bool kit = file.hasFileExtension ("z64kit");
                if (auto* m = processor.apvts.getParameter (ParamID::mode))
                    m->setValueNotifyingHost (kit ? 1.0f : 0.0f);
                processor.patches.loadFile (file);
            }
            refresh();
        });
}

// ------------------------------------------------------------------ SavePatchDialog

SavePatchDialog::SavePatchDialog (Zadabar64Processor& p) : processor (p)
{
    setWantsKeyboardFocus (true);
    nameEditor.setInputRestrictions (24, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 -+.!#&()'");
    nameEditor.onTextChange = [this] { updateHint(); repaint(); };
    nameEditor.onReturnKey  = [this] { commit(); };
    nameEditor.onEscapeKey  = [this] { close(); };
    addAndMakeVisible (nameEditor);
    for (auto* b : { &ok, &cancel })
    {
        setSection (*b, Section::Setup);
        b->getProperties().set ("fontSize", 8.0f);
        addAndMakeVisible (b);
    }
    ok.onClick = [this] { commit(); };
    cancel.onClick = [this] { close(); };
}

juce::Rectangle<int> SavePatchDialog::box() const
{
    return getLocalBounds().withSizeKeepingCentre (440, 190);
}

void SavePatchDialog::show (PatchType t)
{
    type = t;
    if (auto* skin = skinOf (*this))
    {
        nameEditor.setFont (skin->font (16.0f));
        nameEditor.setColour (juce::TextEditor::textColourId, skin->valueColour());
        nameEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha (0.25f));
        nameEditor.setColour (juce::TextEditor::outlineColourId, skin->textColour());
        nameEditor.setColour (juce::TextEditor::focusedOutlineColourId, skin->valueColour());
        nameEditor.setColour (juce::TextEditor::highlightColourId, skin->valueColour().withAlpha (0.3f));
        nameEditor.setColour (juce::CaretComponent::caretColourId, skin->valueColour());
    }
    auto current = processor.patches.currentName (t);
    if (current.startsWith ("INIT"))
        current = {};
    nameEditor.setText (current, false);
    nameEditor.selectAll();
    updateHint();
    setVisible (true);
    toFront (true);
    nameEditor.grabKeyboardFocus();
}

void SavePatchDialog::updateHint()
{
    const auto n = nameEditor.getText().trim();
    const auto file = PatchManager::userFolder (type).getChildFile (PatchManager::safeFileName (n.toUpperCase())
                                                                    + PatchManager::fileExtension (type));
    if (n.isEmpty())
        hint = "TYPE A NAME";
    else if (file.existsAsFile())
        hint = "REPLACES YOUR PATCH WITH THIS NAME";
    else
        hint = type == PatchType::Instrument ? "SAVED TO DOCUMENTS/ZADABAR64/INSTRUMENT PATCHES"
                                             : "SAVED TO DOCUMENTS/ZADABAR64/DRUM KITS";
    ok.setEnabled (n.isNotEmpty());
}

void SavePatchDialog::resized()
{
    auto b = box().reduced (20, 16);
    b.removeFromTop (38);
    nameEditor.setBounds (b.removeFromTop (34));
    b.removeFromTop (30);
    auto buttons = b.removeFromTop (30);
    cancel.setBounds (buttons.removeFromRight (110));
    buttons.removeFromRight (10);
    ok.setBounds (buttons.removeFromRight (110));
}

void SavePatchDialog::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.55f));
    if (auto* skin = skinOf (*this))
    {
        const auto b = box();
        {
            juce::Graphics::ScopedSaveState keep (g);
            g.reduceClipRegion (b);
            skin->paintBackground (g, b);
        }
        skin->paintPanel (g, b, type == PatchType::Instrument ? "SAVE PATCH" : "SAVE DRUM KIT", Section::Setup);
        g.setColour (skin->textColour());
        g.setFont (skin->font (8.0f));
        auto r = b.reduced (20, 16);
        r.removeFromTop (38 + 34 + 8);
        g.drawText (hint, r.removeFromTop (14), juce::Justification::centredLeft, false);
    }
}

bool SavePatchDialog::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { close(); return true; }
    return false;
}

void SavePatchDialog::commit()
{
    const auto n = nameEditor.getText().trim();
    if (n.isEmpty())
        return;
    if (processor.patches.save (type, n))
    {
        close();
        if (onSaved)
            onSaved();
    }
    else
    {
        hint = "COULD NOT WRITE THE FILE";
        repaint();
    }
}

void SavePatchDialog::close()
{
    setVisible (false);
}

} // namespace zadabar
