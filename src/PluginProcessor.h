// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "MidiLearn.h"
#include "Parameters.h"
#include "Settings.h"
#include "patches/Patches.h"
#include "player/Player.h"
#include "sid/SidChip.h"

namespace zadabar
{

class Zadabar64Processor : public juce::AudioProcessor,
                           private juce::ChangeListener
{
public:
    Zadabar64Processor();
    ~Zadabar64Processor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // No host program list: in VST3 it becomes a "program" parameter, and a host
    // restoring that parameter would load a factory patch over the restored sound.
    // Patches are browsed in the plugin's patch bar instead.
    int getNumPrograms() override    { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    // Loads factory patch `index` and switches to its mode (instrument / drums).
    void loadFactoryPatch (int index);

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    PatchManager patches { apvts };
    MidiLearn midiLearn { *this };                   // CC -> parameter (saved with the project)
    juce::MidiKeyboardState keyboardState;

    // For the UI
    const PlayerDisplay& getDisplay() const noexcept { return player.getDisplay(); }
    float takePeakLevel() noexcept                   { return peakLevel.exchange (0.0f); }
    std::atomic<int> selectedDrum { 0 };             // drum shown in the drum editor (saved with the state)

private:

    // Switching the mode, the filter routing or the chip model (or loading a patch)
    // makes the 6581's output jump to a new resting level: summed over six chips,
    // a loud thump. We fade out, let the chips settle unheard, re-centre the DC
    // blocker on the new level and fade back in (about 50 ms in all).
    enum class Settle { None, FadeOut, Mute, FadeIn };
    Settle  settle = Settle::None;
    int64_t structureKey = -1;
    int     settleCount = 0, settleLength = 1;
    float   settleGain = 1.0f, fadeOutFrom = 1.0f;
    bool    beginStructureChange (int numSamples);   // true: fade out this block on the old settings
    void reconfigureChip();
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void updateLatency();
    void renderUntil (int64_t endClock, float* const* out, int64_t blockStartClock);

    // Tight timing: every MIDI event is played `lookahead` samples after it
    // arrives (the host compensates); note-ons are hard-restarted on arrival.
    struct Pending
    {
        int64_t due = 0;
        uint8_t bytes[3] {};
        int     size = 0;
        int     voice = -1;
    };
    static constexpr int queueSize = 1024;
    std::array<Pending, queueSize> queue {};
    int queueHead = 0, queueCount = 0;
    std::atomic<int> lookahead { 0 };
    juce::SharedResourcePointer<Settings> settings;

    Params  params;
    // Instrument mode plays on chips[0]; drum mode gives every drum its own chip.
    std::array<SidChip, Player::numChips> chips;
    Player  player { { &chips[0], &chips[1], &chips[2], &chips[3], &chips[4], &chips[5] } };

    double sampleRate = 44100.0;
    int    currentChipModel = -1;
    int    currentVideo     = -1;

    std::vector<float> mono;
    std::array<std::vector<float>, Player::numChips> chipBuffers;
    std::array<juce::SmoothedValue<float>, Player::numChips> chipGain;
    void prerollChips (double seconds);
    std::atomic<float> peakLevel { 0.0f };

    // DC blocker (one-pole high-pass, ~5 Hz): parked 6581 voices leave a small DC offset.
    float dcX1 = 0.0f, dcY1 = 0.0f, dcCoeff = 0.9993f;
    juce::SmoothedValue<float> outputGain { 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Zadabar64Processor)
};

} // namespace zadabar
