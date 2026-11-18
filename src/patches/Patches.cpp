// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Patches.h"

#include "Parameters.h"

namespace zadabar
{

namespace
{
    const char* const typeTag[2] = { "instrument", "drumkit" };
    constexpr auto rootTag = "ZADABAR64PATCH";
}

PatchManager::PatchManager (juce::AudioProcessorValueTreeState& s) : state (s)
{
    current[0].name = "INIT";
    current[1].name = "INIT KIT";
}

juce::StringArray PatchManager::paramIDs (PatchType t)
{
    // the chip model and the filter are part of both sounds
    juce::StringArray ids { ParamID::chipModel, ParamID::filterOn, ParamID::filterMode,
                            ParamID::cutoff, ParamID::resonance, ParamID::volume };
    if (t == PatchType::Instrument)
    {
        ids.addArray ({ ParamID::waveform, ParamID::pulseWidth, ParamID::pwmSpeed,
                        ParamID::attack, ParamID::decay, ParamID::sustain, ParamID::release,
                        ParamID::ring, ParamID::sync, ParamID::modPitch, ParamID::modFine,
                        ParamID::vibDepth, ParamID::vibSpeed, ParamID::vibDelay,
                        ParamID::playMode, ParamID::glideMode, ParamID::glideTime,
                        ParamID::arpMode, ParamID::arpPattern, ParamID::arpSpeed,
                        ParamID::wheelDest, ParamID::wheelAmount,
                        ParamID::bendUp, ParamID::bendDown, ParamID::bendTarget });
    }
    else
    {
        for (int d = 0; d < numDrums; ++d)
            for (auto* what : { "tune", "decay", "sweep", "noise", "wave", "level" })
                ids.add (drumParam (d, what));
    }
    return ids;
}

juce::File PatchManager::userFolder (PatchType t)
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("Zadabar64")
        .getChildFile (t == PatchType::Instrument ? "Instrument Patches" : "Drum Kits");
}

juce::String PatchManager::safeFileName (const juce::String& patchName)
{
    auto n = juce::File::createLegalFileName (patchName.trim());
    return n.isEmpty() ? juce::String ("UNTITLED") : n;
}

std::vector<PatchManager::Entry> PatchManager::list (PatchType t) const
{
    std::vector<Entry> out;
    const auto& f = factoryPatches();
    for (int i = 0; i < static_cast<int> (f.size()); ++i)
        if (f[(size_t) i].type == t)
            out.push_back ({ f[(size_t) i].name, true, i, {} });

    auto files = userFolder (t).findChildFiles (juce::File::findFiles, false, "*" + fileExtension (t));
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileNameWithoutExtension().compareIgnoreCase (b.getFileNameWithoutExtension()) < 0; });
    for (const auto& file : files)
    {
        auto name = file.getFileNameWithoutExtension();
        if (auto xml = juce::XmlDocument::parse (file))
            name = xml->getStringAttribute ("name", name);
        out.push_back ({ name, false, -1, file });
    }
    return out;
}

float PatchManager::plainValue (const juce::String& id) const
{
    if (auto* p = state.getParameter (id))
        return p->convertFrom0to1 (p->getValue());
    return 0.0f;
}

void PatchManager::apply (PatchType t, const std::map<juce::String, float>& values)
{
    ++loadCounter;
    for (const auto& id : paramIDs (t))
        if (auto* p = state.getParameter (id))
        {
            const auto it = values.find (id);
            const float norm = it != values.end() ? p->convertTo0to1 (it->second) : p->getDefaultValue();
            p->beginChangeGesture();
            p->setValueNotifyingHost (norm);
            p->endChangeGesture();
        }
}

void PatchManager::takeSnapshot (PatchType t)
{
    current[(size_t) t].modifiedInProject = false;
    auto& snap = current[(size_t) t].snapshot;
    snap.clear();
    for (const auto& id : paramIDs (t))
        snap[id] = plainValue (id);
}

bool PatchManager::isModified (PatchType t) const
{
    if (current[(size_t) t].modifiedInProject)
        return true;
    const auto& snap = current[(size_t) t].snapshot;
    if (snap.empty())
        return false;
    for (const auto& [id, v] : snap)
        if (auto* p = state.getParameter (id))
        {
            const auto range = p->getNormalisableRange();
            if (std::abs (plainValue (id) - v) > 0.001f * (range.end - range.start))
                return true;
        }
    return false;
}

void PatchManager::loadFactory (int index)
{
    const auto& f = factoryPatches();
    if (index < 0 || index >= static_cast<int> (f.size()))
        return;
    const auto& patch = f[(size_t) index];
    std::map<juce::String, float> values;
    for (const auto& v : patch.values)
        values[v.id] = v.value;
    apply (patch.type, values);
    current[(size_t) patch.type].name = patch.name;
    current[(size_t) patch.type].file = juce::File();
    takeSnapshot (patch.type);
}

bool PatchManager::loadFile (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (rootTag))
        return false;

    const auto t = xml->getStringAttribute ("type") == typeTag[1] ? PatchType::DrumKit : PatchType::Instrument;
    std::map<juce::String, float> values;
    for (auto* e : xml->getChildWithTagNameIterator ("PARAM"))
        values[e->getStringAttribute ("id")] = static_cast<float> (e->getDoubleAttribute ("value"));
    apply (t, values);

    current[(size_t) t].name = xml->getStringAttribute ("name", file.getFileNameWithoutExtension());
    current[(size_t) t].file = file.isAChildOf (userFolder (t)) ? file : juce::File();
    takeSnapshot (t);
    return true;
}

void PatchManager::load (PatchType t, const Entry& e)
{
    if (e.factory)
        loadFactory (e.factoryIndex);
    else
        loadFile (e.file);
    juce::ignoreUnused (t);
}

int PatchManager::currentIndex (PatchType t) const
{
    const auto entries = list (t);
    const auto& cur = current[(size_t) t];
    for (int i = 0; i < static_cast<int> (entries.size()); ++i)
    {
        const auto& e = entries[(size_t) i];
        if (cur.file != juce::File() ? e.file == cur.file : (e.factory && e.name == cur.name))
            return i;
    }
    return -1;
}

void PatchManager::step (PatchType t, int delta)
{
    const auto entries = list (t);
    if (entries.empty())
        return;
    const int n = static_cast<int> (entries.size());
    const int i = currentIndex (t);
    const int next = i < 0 ? 0 : ((i + delta) % n + n) % n;
    load (t, entries[(size_t) next]);
}

bool PatchManager::save (PatchType t, const juce::String& name)
{
    const auto clean = name.trim().toUpperCase();
    if (clean.isEmpty())
        return false;

    juce::XmlElement xml (rootTag);
    xml.setAttribute ("version", 1);
    xml.setAttribute ("type", typeTag[(size_t) t]);
    xml.setAttribute ("name", clean);
    for (const auto& id : paramIDs (t))
    {
        auto* e = xml.createNewChildElement ("PARAM");
        e->setAttribute ("id", id);
        e->setAttribute ("value", plainValue (id));
    }

    const auto folder = userFolder (t);
    if (! folder.createDirectory())
        return false;
    const auto file = folder.getChildFile (safeFileName (clean) + fileExtension (t));
    if (! xml.writeTo (file))
        return false;

    current[(size_t) t].name = clean;
    current[(size_t) t].file = file;
    takeSnapshot (t);
    return true;
}

bool PatchManager::remove (PatchType t, const Entry& e)
{
    if (e.factory || ! e.file.existsAsFile())
        return false;
    const bool wasCurrent = current[(size_t) t].file == e.file;
    if (! e.file.moveToTrash() && ! e.file.deleteFile())
        return false;
    if (wasCurrent)
        current[(size_t) t].file = juce::File();   // the sound stays; it is just not a saved patch any more
    return true;
}

void PatchManager::writeState (juce::ValueTree& s) const
{
    for (int t = 0; t < 2; ++t)
    {
        s.setProperty (juce::String ("patchName") + juce::String (t), current[(size_t) t].name, nullptr);
        s.setProperty (juce::String ("patchFile") + juce::String (t), current[(size_t) t].file.getFullPathName(), nullptr);
        s.setProperty (juce::String ("patchModified") + juce::String (t), isModified (static_cast<PatchType> (t)), nullptr);
    }
}

void PatchManager::readState (const juce::ValueTree& s)
{
    for (int t = 0; t < 2; ++t)
    {
        const auto name = s.getProperty (juce::String ("patchName") + juce::String (t)).toString();
        if (name.isNotEmpty())
            current[(size_t) t].name = name;
        const auto path = s.getProperty (juce::String ("patchFile") + juce::String (t)).toString();
        current[(size_t) t].file = juce::File::isAbsolutePath (path) ? juce::File (path) : juce::File();
        takeSnapshot (static_cast<PatchType> (t));   // the project's sound is the reference
        current[(size_t) t].modifiedInProject = static_cast<bool> (s.getProperty (juce::String ("patchModified") + juce::String (t), false));
    }
}

} // namespace zadabar
