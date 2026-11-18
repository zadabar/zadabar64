// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MidiLearn.h"

namespace zadabar
{

MidiLearn::MidiLearn (juce::AudioProcessor& processor)
{
    for (auto* p : processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            params.add (ranged);
    for (auto& m : map)
        m = -1;
}

bool MidiLearn::handle (const juce::MidiMessage& m) noexcept
{
    if (! m.isController())
        return false;
    const int cc = m.getControllerNumber();
    if (cc < 0 || cc >= numCCs)
        return false;

    if (int target = learning.load(); target >= 0 && learning.compare_exchange_strong (target, -1))
    {
        for (auto& slot : map)                 // a parameter has only one CC
            if (slot.load() == target)
                slot = -1;
        map[(size_t) cc] = target;
        lastLearned = cc;
        ++learned;
        ++changes;
    }

    const int index = map[(size_t) cc].load();
    if (index < 0 || index >= params.size())
        return false;

    auto* p = params.getUnchecked (index);
    const float value = static_cast<float> (m.getControllerValue()) / 127.0f;
    if (std::abs (p->getValue() - value) > 1.0e-6f)
        p->setValueNotifyingHost (value);
    return true;
}

int MidiLearn::indexOf (const juce::String& paramID) const
{
    for (int i = 0; i < params.size(); ++i)
        if (params.getUnchecked (i)->getParameterID() == paramID)
            return i;
    return -1;
}

void MidiLearn::startLearning (const juce::String& paramID)
{
    learning = indexOf (paramID);
    ++changes;
}

juce::String MidiLearn::learningID() const
{
    const int i = learning.load();
    return i >= 0 && i < params.size() ? params.getUnchecked (i)->getParameterID() : juce::String();
}

int MidiLearn::ccFor (const juce::String& paramID) const
{
    const int index = indexOf (paramID);
    if (index >= 0)
        for (int cc = 0; cc < numCCs; ++cc)
            if (map[(size_t) cc].load() == index)
                return cc;
    return -1;
}

void MidiLearn::forget (const juce::String& paramID)
{
    const int index = indexOf (paramID);
    for (auto& slot : map)
        if (index >= 0 && slot.load() == index)
            slot = -1;
    ++changes;
}

void MidiLearn::forgetAll()
{
    for (auto& slot : map)
        slot = -1;
    learning = -1;
    ++changes;
}

std::vector<std::pair<int, juce::String>> MidiLearn::mappings() const
{
    std::vector<std::pair<int, juce::String>> out;
    for (int cc = 0; cc < numCCs; ++cc)
        if (const int i = map[(size_t) cc].load(); i >= 0 && i < params.size())
            out.emplace_back (cc, params.getUnchecked (i)->getParameterID());
    return out;
}

juce::String MidiLearn::nameOf (const juce::String& paramID) const
{
    const int i = indexOf (paramID);
    return i >= 0 ? params.getUnchecked (i)->getName (64) : paramID;
}

juce::String MidiLearn::toString() const
{
    juce::StringArray parts;
    for (const auto& [cc, id] : mappings())
        parts.add (juce::String (cc) + "=" + id);
    return parts.joinIntoString (";");
}

void MidiLearn::fromString (const juce::String& text)
{
    for (auto& slot : map)
        slot = -1;
    for (const auto& part : juce::StringArray::fromTokens (text, ";", {}))
    {
        const int cc = part.upToFirstOccurrenceOf ("=", false, false).trim().getIntValue();
        const int index = indexOf (part.fromFirstOccurrenceOf ("=", false, false).trim());
        if (index < 0 || cc < 0 || cc >= numCCs || ! part.containsChar ('='))
            continue;
        for (auto& slot : map)
            if (slot.load() == index)
                slot = -1;
        map[(size_t) cc] = index;
    }
    ++changes;
}

void MidiLearn::writeState (juce::ValueTree& state) const
{
    if (auto old = state.getChildWithName (stateTag); old.isValid())
        state.removeChild (old, nullptr);
    juce::ValueTree t (stateTag);
    for (const auto& [cc, id] : mappings())
        t.appendChild (juce::ValueTree ("CC", { { "cc", cc }, { "param", id } }), nullptr);
    state.appendChild (t, nullptr);
}

bool MidiLearn::readState (const juce::ValueTree& state)
{
    const auto t = state.getChildWithName (stateTag);
    if (! t.isValid())
        return false;
    juce::StringArray parts;
    for (const auto& c : t)
        parts.add (c.getProperty ("cc").toString() + "=" + c.getProperty ("param").toString());
    fromString (parts.joinIntoString (";"));
    return true;
}

} // namespace zadabar
