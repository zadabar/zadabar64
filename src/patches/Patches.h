// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <map>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

namespace zadabar
{

// Two banks: instrument patches (INSTRUMENT mode) and drum kits (DRUMS mode).
enum class PatchType { Instrument = 0, DrumKit = 1 };

struct PatchValue { const char* id; float value; };   // plain (displayed) units

struct FactoryPatch
{
    const char* name;
    PatchType   type;
    std::vector<PatchValue> values;   // everything not listed is the parameter's default
};

const std::vector<FactoryPatch>& factoryPatches();

// Loads and saves patches. A patch covers the sound of one mode: its parameters
// are set, every other parameter of that mode goes back to its default.
// Global settings (mode, PAL/NTSC, analog noise, output gain) are never touched.
//
// User patches are XML files, one per patch:
//   Documents/Zadabar64/Instrument Patches/<name>.z64patch
//   Documents/Zadabar64/Drum Kits/<name>.z64kit
// Must be used on the message thread.
class PatchManager
{
public:
    explicit PatchManager (juce::AudioProcessorValueTreeState&);

    struct Entry
    {
        juce::String name;
        bool         factory = true;
        int          factoryIndex = -1;   // into factoryPatches()
        juce::File   file;                // user patches
    };

    // Factory patches of this type first, then the user's, sorted by name.
    std::vector<Entry> list (PatchType) const;

    void load (PatchType, const Entry&);
    void loadFactory (int factoryIndex);
    bool loadFile (const juce::File&);          // any .z64patch / .z64kit; false if unreadable
    void step (PatchType, int delta);           // previous / next in list()
    bool save (PatchType, const juce::String& name);   // to the user folder (replaces a same-named one)
    bool remove (PatchType, const Entry&);      // user patches only (moved to the trash)

    juce::String currentName (PatchType t) const { return current[(size_t) t].name; }
    bool currentIsUser (PatchType t) const       { return current[(size_t) t].file != juce::File(); }
    int  currentIndex (PatchType) const;        // position in list(), -1 if not there
    bool isModified (PatchType) const;          // changed since it was loaded or saved

    static juce::StringArray paramIDs (PatchType);
    static juce::File userFolder (PatchType);
    static juce::String fileExtension (PatchType t) { return t == PatchType::Instrument ? ".z64patch" : ".z64kit"; }
    static juce::String safeFileName (const juce::String& patchName);

    // Kept in the plugin state, so a project reopens with the right names.
    void writeState (juce::ValueTree&) const;
    void readState (const juce::ValueTree&);

    // Counts patch loads, so the audio thread can hide the switching thump.
    std::atomic<int> loadCounter { 0 };

private:
    void apply (PatchType, const std::map<juce::String, float>& values);
    void takeSnapshot (PatchType);
    float plainValue (const juce::String& id) const;

    struct Current
    {
        juce::String name;
        juce::File   file;                       // empty for factory patches
        std::map<juce::String, float> snapshot;  // values right after load / save
        bool modifiedInProject = false;          // it was already modified when the project was saved
    };

    juce::AudioProcessorValueTreeState& state;
    std::array<Current, 2> current;
};

} // namespace zadabar
