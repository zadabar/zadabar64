// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace zadabar
{

// Tags a control with the parameter it edits (for MIDI learn).
inline void setParamID (juce::Component& c, const juce::String& id) { c.getProperties().set ("paramID", id); }

// The parameter of a control: its own tag or its parent's (a SegmentedChoice's buttons).
inline juce::String findParamID (juce::Component& c)
{
    for (auto* p = &c; p != nullptr; p = p->getParentComponent())
        if (p->getProperties().contains ("paramID"))
            return p->getProperties()["paramID"].toString();
    return {};
}

// Implemented by the editor: shows the MIDI learn menu for a control.
struct LearnHost
{
    virtual ~LearnHost() = default;
    virtual void showLearnMenu (juce::Component& control, const juce::String& paramID) = 0;
};

// A control that opens the MIDI learn menu on right-click (ctrl-click on a Mac)
// instead of reacting to it: a right-click must not toggle a button or move a knob.
template <typename Base>
class Learnable : public Base
{
public:
    using Base::Base;

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            if (auto* host = this->template findParentComponentOfClass<LearnHost>())
                if (const auto id = findParamID (*this); id.isNotEmpty())
                {
                    rightClick = true;
                    host->showLearnMenu (*this, id);
                    return;
                }
        }
        rightClick = false;
        Base::mouseDown (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! rightClick)
            Base::mouseDrag (e);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (rightClick)
        {
            rightClick = false;
            return;
        }
        Base::mouseUp (e);
    }

private:
    bool rightClick = false;
};

using LearnSlider = Learnable<juce::Slider>;
using LearnCombo  = Learnable<juce::ComboBox>;
using LearnButton = Learnable<juce::TextButton>;

} // namespace zadabar
