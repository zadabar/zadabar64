// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#include "SetupPage.h"

#include "Widgets.h"

namespace zadabar
{

SetupPage::SetupPage()
{
    setSection (*this, Section::Setup);

    skinHeading.setText ("SKIN", juce::dontSendNotification);
    skinHeading.getProperties().set ("fontSize", 16.0f);
    addAndMakeVisible (skinHeading);

    for (int i = 0; i < Skin::numSkins; ++i)
    {
        auto* b = skinButtons.add (new juce::TextButton (Skin::nameOf (i)));
        b->getProperties().set ("fontSize", 16.0f);
        b->onClick = [this, i] { if (onSkinChosen) onSkinChosen (i); };
        addAndMakeVisible (b);

        auto* d = skinDescriptions.add (new juce::Label ({}, Skin::descriptionOf (i)));
        d->setJustificationType (juce::Justification::centredTop);
        addAndMakeVisible (d);
    }

    timingHeading.setText ("TIMING", juce::dontSendNotification);
    timingHeading.getProperties().set ("fontSize", 16.0f);
    addAndMakeVisible (timingHeading);



    int t = 0;
    for (auto* b : { &timingLive, &timingTight })
    {
        b->getProperties().set ("fontSize", 16.0f);
        b->onClick = [this, t] { if (onTimingChosen) onTimingChosen (t); };
        addAndMakeVisible (b);
        ++t;
    }

    midiHeading.setText ("MIDI", juce::dontSendNotification);
    midiHeading.getProperties().set ("fontSize", 16.0f);
    addAndMakeVisible (midiHeading);
    midiForgetAll.onClick   = [this] { if (onForgetAllMidi) onForgetAllMidi(); };
    midiSaveDefault.onClick = [this] { if (onSaveMidiDefault) onSaveMidiDefault(); };
    midiSaveDefault.setTooltip ("New Zadabar64 instances start with this MIDI map. Each project keeps its own map.");
    for (auto* b : { &midiForgetAll, &midiSaveDefault })
        addAndMakeVisible (b);

    closeButton.getProperties().set ("fontSize", 16.0f);
    closeButton.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (closeButton);
}

void SetupPage::setCurrentSkin (int index)
{
    for (int i = 0; i < skinButtons.size(); ++i)
        skinButtons[i]->setToggleState (i == index, juce::dontSendNotification);
}

void SetupPage::setCurrentTiming (int timing)
{
    timingLive.setToggleState  (timing == 0, juce::dontSendNotification);
    timingTight.setToggleState (timing == 1, juce::dontSendNotification);
}

void SetupPage::setMidiMappings (const juce::StringArray& lines)
{
    if (lines == midiLines)
        return;
    midiLines = lines;
    midiForgetAll.setEnabled (! lines.isEmpty());
    repaint();
}

void SetupPage::paint (juce::Graphics& g)
{
    if (auto* skin = skinOf (*this))
    {
        skin->paintPanel (g, getLocalBounds(), "SETUP", Section::Setup);

        g.setColour (skin->textColour());
        g.setFont (skin->font (8.0f));
        auto drawLines = [&g] (juce::Rectangle<int> area, std::initializer_list<const char*> lines)
        {
            for (auto* line : lines)
                g.drawText (line, area.removeFromTop (14), juce::Justification::centredLeft, false);
        };

        drawLines (timingHelpArea, { "LIVE:  NO LATENCY. DRUMS ARE ALWAYS ON TIME; INSTRUMENT NOTES CAN BE DELAYED",
                                     "       UP TO 33 MS BY THE SID ADSR BUG.  TIGHT: HARD RESTART BEFORE EVERY NOTE,",
                                     "       ALWAYS ON TIME, ADDS 40 MS LATENCY (YOUR DAW COMPENSATES DURING PLAYBACK)." });

        drawLines (midiHelpArea, { "MOD WHEEL AND PITCH BEND: SEE THE WHEEL AND BEND PANELS.  MIDI LEARN: RIGHT-CLICK",
                                   "A KNOB, SWITCH OR LIST, CHOOSE MIDI LEARN AND MOVE A CONTROL ON YOUR CONTROLLER." });

        // the map, in columns
        constexpr int rows = 4, columns = 3;
        const int colWidth = midiListArea.getWidth() / columns;
        g.setColour (skin->valueColour());
        if (midiLines.isEmpty())
            g.drawText ("NO CONTROLLERS LEARNED YET.", midiListArea.withHeight (14), juce::Justification::centredLeft, false);
        for (int i = 0; i < midiLines.size() && i < rows * columns; ++i)
        {
            auto text = midiLines[i];
            if (i == rows * columns - 1 && midiLines.size() > rows * columns)
                text = "+" + juce::String (midiLines.size() - i) + " MORE";
            g.drawText (text, midiListArea.getX() + (i / rows) * colWidth, midiListArea.getY() + (i % rows) * 14,
                        colWidth - 8, 14, juce::Justification::centredLeft, true);
        }
        g.setColour (skin->textColour());

        const auto credits = "ZADABAR64 " + Skin::shortVersion() + " BY ZADABAR  -  FREE SOFTWARE, GPL-3.0";
        drawLines (getLocalBounds().reduced (24).removeFromBottom (28),
                   { credits.toRawUTF8(),
                     "SID EMULATION: RESIDFP  -  FONT: PRESS START 2P (SIL OFL 1.1)" });
    }
}

void SetupPage::resized()
{
    auto r = getLocalBounds().reduced (24);
    r.removeFromTop (28);

    closeButton.setBounds (getLocalBounds().reduced (16).removeFromTop (28).removeFromRight (110));

    skinHeading.setBounds (r.removeFromTop (24));
    r.removeFromTop (8);

    auto cards = r.removeFromTop (84);
    const int gap = 16;
    const int w = (cards.getWidth() - gap * (Skin::numSkins - 1)) / Skin::numSkins;
    for (int i = 0; i < Skin::numSkins; ++i)
    {
        auto c = cards.removeFromLeft (w);
        cards.removeFromLeft (gap);
        skinButtons[i]->setBounds (c.removeFromTop (48));
        c.removeFromTop (8);
        skinDescriptions[i]->setBounds (c);
    }

    r.removeFromTop (12);
    timingHeading.setBounds (r.removeFromTop (24));
    r.removeFromTop (8);
    auto row = r.removeFromTop (40);
    timingLive.setBounds (row.removeFromLeft (160));
    row.removeFromLeft (16);
    timingTight.setBounds (row.removeFromLeft (160));
    r.removeFromTop (10);
    timingHelpArea = r.removeFromTop (48);

    r.removeFromTop (6);
    auto midiRow = r.removeFromTop (24);
    midiHeading.setBounds (midiRow.removeFromLeft (120));
    midiSaveDefault.setBounds (midiRow.removeFromRight (150));
    midiRow.removeFromRight (10);
    midiForgetAll.setBounds (midiRow.removeFromRight (120));
    r.removeFromTop (6);
    midiHelpArea = r.removeFromTop (28);
    r.removeFromTop (4);
    midiListArea = r.removeFromTop (56);
}

} // namespace zadabar
