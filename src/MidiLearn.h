// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <utility>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

namespace zadabar
{

// MIDI learn: MIDI controllers (CC 0-119) mapped to plugin parameters.
// One parameter per CC and one CC per parameter. Learning: the UI picks a
// parameter, the audio thread takes the next CC that arrives.
// The map is saved with the project; a default map for new instances is kept
// in the user settings (see Settings).
class MidiLearn
{
public:
    explicit MidiLearn (juce::AudioProcessor&);

    // --- audio thread
    // A learned CC moves its parameter (and a pending learn takes this CC).
    // Returns true if the message was a CC that is now mapped (don't pass it on).
    bool handle (const juce::MidiMessage&) noexcept;
    bool isMapped (const juce::MidiMessage& m) const noexcept
    {
        return m.isController() && m.getControllerNumber() < numCCs
            && map[(size_t) m.getControllerNumber()].load (std::memory_order_relaxed) >= 0;
    }

    // --- message thread
    void startLearning (const juce::String& paramID);
    void cancelLearning() noexcept                       { learning = -1; ++changes; }
    juce::String learningID() const;                      // empty if not learning
    int  ccFor (const juce::String& paramID) const;       // -1 if not mapped
    void forget (const juce::String& paramID);
    void forgetAll();
    std::vector<std::pair<int, juce::String>> mappings() const;   // (cc, paramID), by CC
    juce::String nameOf (const juce::String& paramID) const;     // the parameter's name

    // "21=cutoff;74=resonance"
    juce::String toString() const;
    void fromString (const juce::String&);

    // Saved with the project. readState returns false if the state has no map (older projects).
    void writeState (juce::ValueTree&) const;
    bool readState (const juce::ValueTree&);
    static constexpr auto stateTag = "MIDI_LEARN";

    // bumps on every change of the map or of the learning state (the UI polls it)
    int version() const noexcept { return changes.load(); }
    int lastLearnedCC() const noexcept { return lastLearned.load(); }
    int learnCount() const noexcept    { return learned.load(); }   // bumps when a learn completes

    static constexpr int numCCs = 120;   // 120-127 are channel mode messages

private:
    int indexOf (const juce::String& paramID) const;

    juce::Array<juce::RangedAudioParameter*> params;
    std::array<std::atomic<int>, numCCs> map;     // parameter index, -1 = none
    std::atomic<int> learning { -1 };             // parameter index waiting for a CC
    std::atomic<int> changes { 0 };
    std::atomic<int> lastLearned { -1 };
    std::atomic<int> learned { 0 };
};

} // namespace zadabar
