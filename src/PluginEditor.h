// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"
#include "Settings.h"
#include "ui/SetupPage.h"
#include "ui/Skin.h"
#include "ui/PatchBar.h"
#include "ui/Widgets.h"

namespace zadabar
{

class Zadabar64Editor : public juce::AudioProcessorEditor,
                        private juce::Timer,
                        private juce::ChangeListener
{
public:
    explicit Zadabar64Editor (Zadabar64Processor&);
    ~Zadabar64Editor() override;

    void paint (juce::Graphics&) override;
    void paintStatic (juce::Graphics&);   // background + panel frames (cached)
    void resized() override;

    // for tests / screenshots
    void showSetup (bool shouldShow);
    void applySkin (int index);

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void setupKnob (Knob&, const juce::String& name, Section, const juce::String& paramID = {});
    void setupCaption (juce::Label&, const juce::String& text, Section);
    void selectDrum (int index);
    void updateVisibility();
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void layoutKnobRow (juce::Rectangle<int>, std::initializer_list<Knob*>);
    juce::Rectangle<int> contentOf (juce::Rectangle<int> panel) const;

    Zadabar64Processor& processor;
    juce::SharedResourcePointer<Settings> settings;
    std::unique_ptr<Skin> skin;       // declared first: outlives every component below
    int skinIndex = 0;

    // header
    SegmentedChoice  modeSwitch;
    juce::TextButton setupButton { "SETUP" };
    HeaderState      headerState;

    // chip
    juce::Label     modelCaption, clockCaption;
    SegmentedChoice chipModel, video;
    Knob analogNoise;

    // oscillator
    juce::Label    waveCaption;
    juce::ComboBox waveform;
    std::unique_ptr<ComboAttachment> waveformAttachment;
    Knob pulseWidth, pwmSpeed;

    // envelope
    EnvelopeView envelopeView;
    WaveScope waveScope;
    Knob attack, decay, sustain, release;

    // ring mod / sync
    juce::TextButton ringButton { "RING" }, syncButton { "SYNC" };
    std::unique_ptr<ButtonAttachment> ringAttachment, syncAttachment;
    Knob modPitch, modFine;
    juce::Label ringHint;

    // vibrato
    Knob vibDepth, vibSpeed, vibDelay;

    // glide
    SegmentedChoice playMode, glideMode;
    Knob glideTime;

    // arpeggio
    SegmentedChoice arpMode;
    juce::ComboBox arpPattern;
    std::unique_ptr<ComboAttachment> arpPatternAttachment;
    Knob arpSpeed;

    // filter
    SegmentedChoice  filterMode;
    juce::TextButton filterOn { "FILTER ON" };
    std::unique_ptr<ButtonAttachment> filterOnAttachment;
    Knob cutoff, resonance;

    // output
    Knob volume, outputGain;
    LevelMeter meter;

    // drums
    DrumPads       drumPads;
    DrumView       drumView;
    juce::Label    drumName, drumWaveCaption;
    juce::ComboBox drumWave;
    std::unique_ptr<ComboAttachment> drumWaveAttachment;
    Knob drumTune, drumDecay, drumSweep, drumNoise;

    juce::MidiKeyboardComponent keyboard;
    SetupPage setupPage;
    juce::TooltipWindow tooltips { this, 700 };

    juce::Rectangle<int> headerArea, logoArea, infoArea, stripArea, stripTextArea;
    PatchBar patchBar;

    // background + panel frames, drawn once (they only change with size, skin, mode or setup)
    juce::Image backgroundCache;
    float backgroundScale = 0.0f;
    bool  backgroundDirty = true;
    juce::String headerSignature;
    SavePatchDialog saveDialog;
    juce::Rectangle<int> chipPanel, oscPanel, envPanel, filterPanel, outPanel, drumPanel;
    juce::Rectangle<int> modPanel, vibPanel, arpPanel, glidePanel;

    bool setupOpen = false;
    int  shownMode = -1;
    int  ticks     = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Zadabar64Editor)
};

} // namespace zadabar
