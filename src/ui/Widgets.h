// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Learnable.h"
#include "Skin.h"

namespace zadabar
{

class Zadabar64Processor;

inline Skin* skinOf (juce::Component& c) { return dynamic_cast<Skin*> (&c.getLookAndFeel()); }

// A row of buttons bound to a choice / bool parameter (one button per option).
class SegmentedChoice : public juce::Component
{
public:
    SegmentedChoice (juce::AudioProcessorValueTreeState&, const juce::String& paramID,
                     const juce::StringArray& labels, Section, float fontSize = 8.0f);
    void resized() override;
    void setTooltip (const juce::String& text);   // on every button

private:
    juce::OwnedArray<LearnButton> buttons;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    int numOptions = 0;
};

// Output level meter (peak, falling, with peak hold).
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter (Zadabar64Processor&);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    Zadabar64Processor& processor;
    float level = 0.0f, peak = 0.0f;
    float drawnLevel = -1.0f, drawnPeak = -1.0f;   // repaint only when these change
    int   peakHoldTicks = 0;
};

// The ADSR shape drawn from the real SID timings, with a dot per sounding voice.
class EnvelopeView : public juce::Component, private juce::Timer
{
public:
    explicit EnvelopeView (Zadabar64Processor&);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    Zadabar64Processor& processor;
    juce::String drawnSignature;   // ADSR values last drawn
    bool hadDots = false;          // voices were moving on the curve at the last paint
};

// The oscillator, drawn from a real (hidden) SID chip: same waveform, chip model,
// pulse width (live while PWM sweeps), ring mod and sync as the sound.
class WaveScope : public juce::Component, private juce::Timer
{
public:
    explicit WaveScope (Zadabar64Processor&);
    ~WaveScope() override;
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    bool capture();   // false: nothing changed since the last trace

    Zadabar64Processor& processor;
    std::unique_ptr<class SidChip> chip;
    int   chipModel = -1;
    float calMid = 0.0f, calRange = 1.0f;
    std::vector<float> samples;
    std::vector<juce::Point<float>> trace;
    juce::String traceSignature;   // the settings the trace was captured with
};

// The selected drum's frame program: pitch per C64 frame, noise frames marked.
class DrumView : public juce::Component, private juce::Timer
{
public:
    explicit DrumView (Zadabar64Processor&);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    Zadabar64Processor& processor;
    juce::String drawnSignature;   // selected drum + its settings, last drawn
};

// Six drum pads: click to select (for editing) and to hear the drum.
// Under each pad a level fader (the drum's level in the mix).
class DrumPads : public juce::Component, private juce::Timer
{
public:
    DrumPads (Zadabar64Processor&, std::function<void (int)> onSelect);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<int> padBounds (int index) const;

    Zadabar64Processor& processor;
    std::function<void (int)> onSelect;
    int heldNote = -1;
    bool flashing = false;         // a pad was lit at the last tick

    juce::OwnedArray<LearnSlider> levels;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttachments;
    static constexpr int faderHeight = 18;
};

} // namespace zadabar
