// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include <juce_audio_processors/juce_audio_processors.h>

namespace zadabar
{

namespace ParamID
{
    inline constexpr auto mode        = "mode";
    inline constexpr auto chipModel   = "chipModel";
    inline constexpr auto video       = "video";
    inline constexpr auto analogNoise = "analogNoise";
    inline constexpr auto waveform    = "waveform";
    inline constexpr auto pulseWidth  = "pulseWidth";
    inline constexpr auto pwmSpeed    = "pwmSpeed";
    inline constexpr auto attack      = "attack";
    inline constexpr auto decay       = "decay";
    inline constexpr auto sustain     = "sustain";
    inline constexpr auto release     = "release";
    inline constexpr auto filterOn    = "filterOn";
    inline constexpr auto filterMode  = "filterMode";
    inline constexpr auto cutoff      = "cutoff";
    inline constexpr auto resonance   = "resonance";
    inline constexpr auto volume      = "volume";
    inline constexpr auto outputGain  = "outputGain";

    // modulation (instrument mode)
    inline constexpr auto ring        = "ring";
    inline constexpr auto sync        = "sync";
    inline constexpr auto modPitch    = "modPitch";
    inline constexpr auto modFine     = "modFine";
    inline constexpr auto vibDepth    = "vibDepth";
    inline constexpr auto vibSpeed    = "vibSpeed";
    inline constexpr auto vibDelay    = "vibDelay";
    inline constexpr auto playMode    = "playMode";
    inline constexpr auto glideMode   = "glideMode";
    inline constexpr auto glideTime   = "glideTime";
    inline constexpr auto arpMode     = "arpMode";
    inline constexpr auto arpPattern  = "arpPattern";
    inline constexpr auto arpSpeed    = "arpSpeed";

    // MIDI performance controls (instrument mode)
    inline constexpr auto wheelDest   = "wheelDest";
    inline constexpr auto wheelAmount = "wheelAmount";
    inline constexpr auto bendUp      = "bendUp";
    inline constexpr auto bendDown    = "bendDown";
    inline constexpr auto bendTarget  = "bendTarget";
}

// ---------------------------------------------------------------- Mod wheel
// What the mod wheel (CC 1) moves, and how far at full wheel (WHEEL AMOUNT 100 %).
enum class WheelDest { Vibrato = 0, Cutoff, Resonance, PulseWidth, ModPitch };
inline const juce::StringArray wheelDestNames { "Vibrato", "Cutoff", "Resonance", "Pulse Width", "Mod Pitch" };
inline constexpr double wheelVibratoCents = 100.0;   // VIBRATO: wheel vibrato depth at 100 %
inline constexpr double wheelModSemis     = 24.0;    // MOD PITCH: semitones at 100 % (sync sweeps)

// Pitch bend moves the note, or only the ring/sync modulator (MOD PITCH).
enum class BendTarget { Pitch = 0, ModPitch = 1 };

// ---------------------------------------------------------------- Arpeggio
// The C64 "chord": one voice steps through the notes of a chord, one step every
// 1-4 frames, as fast as the player routine runs (50 Hz PAL / 60 Hz NTSC).
inline constexpr int maxArpSteps = 4;
struct ArpPattern { const char* name; int length; int steps[maxArpSteps]; };
inline constexpr ArpPattern arpPatterns[] = {
    { "MAJOR 0-4-7",    3, { 0, 4, 7 } },
    { "MINOR 0-3-7",    3, { 0, 3, 7 } },
    { "OCTAVE 0-12",    2, { 0, 12 } },
    { "FIFTH 0-7",      2, { 0, 7 } },
    { "POWER 0-7-12",   3, { 0, 7, 12 } },
    { "SUS4 0-5-7",     3, { 0, 5, 7 } },
    { "MAJ7 0-4-7-11",  4, { 0, 4, 7, 11 } },
    { "MIN7 0-3-7-10",  4, { 0, 3, 7, 10 } },
    { "DOM7 0-4-7-10",  4, { 0, 4, 7, 10 } },
    { "DIM 0-3-6",      3, { 0, 3, 6 } },
    { "2 OCT 0-12-24",  3, { 0, 12, 24 } },
};
inline constexpr int numArpPatterns = static_cast<int> (sizeof (arpPatterns) / sizeof (arpPatterns[0]));
enum class ArpMode { Off = 0, Chord = 1, Fixed = 2 };

// ---------------------------------------------------------------- Drum kit
// Six classic C64 drums. Each is a tiny per-frame SID program, shaped by
// five parameters (see Player::writeDrumFrame).
inline constexpr int numDrums = 6;
inline const char* const drumNames[numDrums]      = { "KICK", "SNARE", "CL HAT", "OP HAT", "LO TOM", "HI TOM" };
inline const char* const drumParamIDs[numDrums]   = { "kick", "snare", "chat", "ohat", "ltom", "htom" };

// General MIDI drum notes -> drum slot (-1 = not used)
inline int drumForNote (int note) noexcept
{
    switch (note)
    {
        case 35: case 36:                   return 0;  // kick
        case 37: case 38: case 39: case 40: return 1;  // snare, rim, clap
        case 42: case 44:                   return 2;  // closed / pedal hat
        case 46: case 49: case 51:          return 3;  // open hat, cymbals
        case 41: case 43: case 45:          return 4;  // low toms
        case 47: case 48: case 50:          return 5;  // high toms
        default:                            return -1;
    }
}
inline constexpr int drumPadNotes[numDrums] = { 36, 38, 42, 46, 45, 48 };

// Drum body waveforms. "Hubbard" alternates noise and pulse every frame,
// the trick Rob Hubbard used for his famous snares.
inline const juce::StringArray drumWaveNames { "Triangle", "Saw", "Pulse", "Noise", "Hubbard" };

struct DrumDefaults { int tune, decay, sweep, noise, wave; };
inline constexpr DrumDefaults drumDefaults[numDrums] = {
    { 31, 7, 24, 1, 0 },   // kick:   low triangle, big pitch drop, 1 frame noise click
    { 50, 7, 12, 1, 4 },   // snare:  Hubbard noise/pulse
    { 96, 3,  0, 0, 3 },   // closed hat: short high noise
    { 96, 9,  0, 0, 3 },   // open hat:   long high noise
    { 43, 8, 14, 1, 0 },   // low tom
    { 50, 7, 14, 1, 0 },   // high tom
};

inline juce::String drumParam (int drum, const char* what)
{
    return juce::String (drumParamIDs[drum]) + "_" + what;
}

inline juce::String noteName (int note)
{
    return juce::MidiMessage::getMidiNoteName (note, true, true, 3);
}

// The reverse of noteName() ("C3" = 60, "D#-1" = 15); a plain number is taken as a MIDI note.
inline int parseNoteName (const juce::String& text)
{
    const auto t = text.trim().toUpperCase();
    if (t.isEmpty())
        return 0;
    const int letter = juce::String ("C D EF G A B").indexOfChar (t[0]);
    if (letter < 0 || t[0] == ' ')
        return t.getIntValue();
    int semi = letter, i = 1;
    if (t[1] == '#') { ++semi; ++i; }
    else if (t[1] == 'B' && t.length() > 2) { --semi; ++i; }
    return (t.substring (i).getIntValue() + 2) * 12 + semi;
}

// Waveform choices and the SID control-register bits they select.
inline const juce::StringArray waveformNames { "Triangle", "Saw", "Pulse", "Noise",
                                               "Saw+Tri", "Pulse+Tri", "Pulse+Saw", "Pulse+Saw+Tri" };
inline constexpr uint8_t waveformBits[] = { 0x10, 0x20, 0x40, 0x80, 0x30, 0x50, 0x60, 0x70 };

inline const juce::StringArray filterModeNames { "Low-pass", "Band-pass", "High-pass", "Notch (LP+HP)" };
inline constexpr uint8_t filterModeBits[] = { 0x10, 0x20, 0x40, 0x50 };

// Real SID envelope timings in ms (C64 Programmer's Reference Guide).
inline constexpr float sidAttackMs[16] = { 2, 8, 16, 24, 38, 56, 68, 80, 100, 250, 500, 800, 1000, 3000, 5000, 8000 };
inline constexpr float sidDecayMs[16]  = { 6, 24, 48, 72, 114, 168, 204, 240, 300, 750, 1500, 2400, 3000, 9000, 15000, 24000 };

// The same timings as display text.
inline const char* const attackTimes[16]  = { "2MS", "8MS", "16MS", "24MS", "38MS", "56MS", "68MS", "80MS",
                                              "100MS", "250MS", "500MS", "800MS", "1S", "3S", "5S", "8S" };
inline const char* const decayTimes[16]   = { "6MS", "24MS", "48MS", "72MS", "114MS", "168MS", "204MS", "240MS",
                                              "300MS", "750MS", "1.5S", "2.4S", "3S", "9S", "15S", "24S" };

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto pid = [] (const char* id) { return ParameterID { id, 1 }; };

    auto tableAttr = [] (const char* const* table)
    {
        return AudioParameterIntAttributes().withStringFromValueFunction (
            [table] (int v, int) { return String (v) + " " + table[jlimit (0, 15, v)]; });
    };

    // --- Mode
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamID::mode), "Mode",
                                                        StringArray { "Instrument", "Drums" }, 0));

    // --- Chip
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamID::chipModel), "Chip Model",
                                                        StringArray { "MOS 6581", "CSG 8580" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamID::video), "Clock",
                                                        StringArray { "PAL", "NTSC" }, 0));
    // reSIDfp can add the background hiss of a real C64 (0 = off, 8 = a lot).
    layout.add (std::make_unique<AudioParameterInt> (pid (ParamID::analogNoise), "Analog Noise", 0, 8, 0,
        AudioParameterIntAttributes().withStringFromValueFunction (
            [] (int v, int) { return v == 0 ? String ("OFF") : String (v); })));

    // --- Oscillator
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamID::waveform), "Waveform", waveformNames, 2));
    layout.add (std::make_unique<AudioParameterInt> (pid (ParamID::pulseWidth), "Pulse Width", 0, 4095, 2048,
        AudioParameterIntAttributes().withStringFromValueFunction (
            [] (int v, int) { return String (roundToInt (v * 100.0f / 4095.0f)) + "%"; })
        .withValueFromStringFunction ([] (const String& t) { return roundToInt (t.getFloatValue() * 4095.0f / 100.0f); })));
    layout.add (std::make_unique<AudioParameterInt> (pid (ParamID::pwmSpeed), "PWM Speed", 0, 255, 0,
        AudioParameterIntAttributes().withStringFromValueFunction (
            [] (int v, int) { return v == 0 ? String ("OFF") : String (v); })));

    // --- Envelope (SID native 0..15 steps)
    layout.add (std::make_unique<AudioParameterInt> (pid (ParamID::attack),  "Attack",  0, 15, 0,  tableAttr (attackTimes)));
    layout.add (std::make_unique<AudioParameterInt> (pid (ParamID::decay),   "Decay",   0, 15, 9,  tableAttr (decayTimes)));
    layout.add (std::make_unique<AudioParameterInt> (pid (ParamID::sustain), "Sustain", 0, 15, 10));
    layout.add (std::make_unique<AudioParameterInt> (pid (ParamID::release), "Release", 0, 15, 9,  tableAttr (decayTimes)));

    // --- Filter
    layout.add (std::make_unique<AudioParameterBool>   (pid (ParamID::filterOn),   "Filter On", true));
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamID::filterMode), "Filter Mode", filterModeNames, 0));
    layout.add (std::make_unique<AudioParameterInt>    (pid (ParamID::cutoff),     "Cutoff", 0, 2047, 1024));
    layout.add (std::make_unique<AudioParameterInt>    (pid (ParamID::resonance),  "Resonance", 0, 15, 8));

    // --- Output
    layout.add (std::make_unique<AudioParameterInt> (pid (ParamID::volume), "SID Volume", 0, 15, 15));
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamID::outputGain), "Output Gain",
                                                       NormalisableRange<float> { -24.0f, 12.0f }, 6.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return String (std::abs (v) < 0.05f ? 0.0f : v, 1) + " DB"; })));

    // --- Ring modulation / hard sync. Every note has a silent modulator oscillator
    //     (SID voice 3, muted with 3OFF) at the note's pitch + MOD PITCH.
    auto msAttr = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return v < 0.5f ? String ("OFF") : v < 1000.0f ? String (roundToInt (v)) + "MS" : String (v / 1000.0f, 2) + "S"; })
        .withValueFromStringFunction ([] (const String& text)
        {
            const auto t = text.trim().toUpperCase();
            if (t.startsWith ("OFF")) return 0.0f;
            const float x = t.getFloatValue();
            return t.endsWith ("S") && ! t.endsWith ("MS") ? x * 1000.0f : x;   // "1.50S" = 1500 ms
        });

    layout.add (std::make_unique<AudioParameterBool> (pid (ParamID::ring), "Ring Mod", false));
    layout.add (std::make_unique<AudioParameterBool> (pid (ParamID::sync), "Hard Sync", false));
    layout.add (std::make_unique<AudioParameterInt>  (pid (ParamID::modPitch), "Mod Pitch", -24, 24, 7,
        AudioParameterIntAttributes().withStringFromValueFunction (
            [] (int v, int) { return (v > 0 ? "+" : "") + String (v) + " ST"; })));
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamID::modFine), "Mod Fine",
                                                       NormalisableRange<float> { -50.0f, 50.0f }, 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return (v > 0.0f ? "+" : "") + String (roundToInt (v)) + " CT"; })));

    // --- Vibrato (updated once per C64 frame, like a real player routine)
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamID::vibDepth), "Vibrato Depth",
                                                       NormalisableRange<float> { 0.0f, 100.0f }, 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return v < 0.5f ? String ("OFF") : String (roundToInt (v)) + " CT"; })));
    NormalisableRange<float> speedRange { 0.5f, 12.0f };
    speedRange.setSkewForCentre (5.0f);
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamID::vibSpeed), "Vibrato Speed", speedRange, 5.5f,
        AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return String (v, 1) + " HZ"; })));
    NormalisableRange<float> delayRange { 0.0f, 2000.0f };
    delayRange.setSkewForCentre (400.0f);
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamID::vibDelay), "Vibrato Delay", delayRange, 250.0f, msAttr));

    // --- Portamento / play mode
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamID::playMode),  "Play Mode",  StringArray { "Poly", "Mono" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamID::glideMode), "Glide Mode", StringArray { "Always", "Legato" }, 0));
    NormalisableRange<float> glideRange { 0.0f, 2000.0f };
    glideRange.setSkewForCentre (250.0f);
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamID::glideTime), "Glide Time", glideRange, 0.0f, msAttr));

    // --- Arpeggio
    StringArray patternNames;
    for (const auto& a : arpPatterns)
        patternNames.add (a.name);
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamID::arpMode), "Arp Mode", StringArray { "Off", "Chord", "Fixed" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamID::arpPattern), "Arp Pattern", patternNames, 0));
    layout.add (std::make_unique<AudioParameterInt> (pid (ParamID::arpSpeed), "Arp Speed", 1, 4, 1,
        AudioParameterIntAttributes().withStringFromValueFunction (
            [] (int v, int) { return String (v) + (v == 1 ? " FRAME" : " FRAMES"); })));

    // --- Mod wheel and pitch bend
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamID::wheelDest), "Wheel Destination", wheelDestNames, 0));
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamID::wheelAmount), "Wheel Amount",
                                                       NormalisableRange<float> { -100.0f, 100.0f }, 50.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { const int i = roundToInt (v); return (i > 0 ? "+" : "") + String (i) + "%"; })
        .withValueFromStringFunction ([] (const String& t) { return t.retainCharacters ("-0123456789.").getFloatValue(); })));
    auto semisAttr = AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return String (v) + " ST"; });
    layout.add (std::make_unique<AudioParameterInt> (pid (ParamID::bendUp),   "Bend Up",   0, 24, 2, semisAttr));
    layout.add (std::make_unique<AudioParameterInt> (pid (ParamID::bendDown), "Bend Down", 0, 24, 2, semisAttr));
    layout.add (std::make_unique<AudioParameterChoice> (pid (ParamID::bendTarget), "Bend Target", StringArray { "Pitch", "Mod Pitch" }, 0));

    // --- Drum kit
    for (int d = 0; d < numDrums; ++d)
    {
        const auto& def = drumDefaults[d];
        const String name = String (drumNames[d]).toLowerCase();
        auto dp = [d] (const char* what) { return ParameterID { drumParam (d, what), 1 }; };

        layout.add (std::make_unique<AudioParameterInt> (dp ("tune"), name + " tune", 12, 108, def.tune,
            AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return noteName (v); })
                                         .withValueFromStringFunction ([] (const String& t) { return parseNoteName (t); })));
        layout.add (std::make_unique<AudioParameterInt> (dp ("decay"), name + " decay", 0, 15, def.decay, tableAttr (decayTimes)));
        layout.add (std::make_unique<AudioParameterInt> (dp ("sweep"), name + " sweep", 0, 48, def.sweep,
            AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return String (v) + " ST"; })));
        layout.add (std::make_unique<AudioParameterInt> (dp ("noise"), name + " noise", 0, 8, def.noise,
            AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return String (v) + " FR"; })));
        layout.add (std::make_unique<AudioParameterChoice> (dp ("wave"), name + " wave", drumWaveNames, def.wave));

        // Per-drum level. Not a SID feature: in drum mode every voice runs on its own
        // emulated chip, and this is that chip's gain in the mix.
        NormalisableRange<float> lvl { -48.0f, 6.0f };
        lvl.setSkewForCentre (-12.0f);
        layout.add (std::make_unique<AudioParameterFloat> (dp ("level"), name + " level", lvl, 0.0f,
            AudioParameterFloatAttributes().withStringFromValueFunction (
                [] (float v, int) { return v <= -47.9f ? String ("OFF") : String (std::abs (v) < 0.05f ? 0.0f : v, 1) + " DB"; })
            .withValueFromStringFunction ([] (const String& t) { return t.trim().toUpperCase().startsWith ("OFF") ? -48.0f : t.getFloatValue(); })));
    }

    return layout;
}

// Lock-free access to current parameter values from the audio thread.
struct Params
{
    explicit Params (juce::AudioProcessorValueTreeState& s)
        : mode       (*s.getRawParameterValue (ParamID::mode)),
          chipModel  (*s.getRawParameterValue (ParamID::chipModel)),
          video      (*s.getRawParameterValue (ParamID::video)),
          analogNoise (*s.getRawParameterValue (ParamID::analogNoise)),
          waveform   (*s.getRawParameterValue (ParamID::waveform)),
          pulseWidth (*s.getRawParameterValue (ParamID::pulseWidth)),
          pwmSpeed   (*s.getRawParameterValue (ParamID::pwmSpeed)),
          attack     (*s.getRawParameterValue (ParamID::attack)),
          decay      (*s.getRawParameterValue (ParamID::decay)),
          sustain    (*s.getRawParameterValue (ParamID::sustain)),
          release    (*s.getRawParameterValue (ParamID::release)),
          filterOn   (*s.getRawParameterValue (ParamID::filterOn)),
          filterMode (*s.getRawParameterValue (ParamID::filterMode)),
          cutoff     (*s.getRawParameterValue (ParamID::cutoff)),
          resonance  (*s.getRawParameterValue (ParamID::resonance)),
          volume     (*s.getRawParameterValue (ParamID::volume)),
          outputGain (*s.getRawParameterValue (ParamID::outputGain))
    {
        ring      = s.getRawParameterValue (ParamID::ring);
        sync      = s.getRawParameterValue (ParamID::sync);
        modPitch  = s.getRawParameterValue (ParamID::modPitch);
        modFine   = s.getRawParameterValue (ParamID::modFine);
        vibDepth  = s.getRawParameterValue (ParamID::vibDepth);
        vibSpeed  = s.getRawParameterValue (ParamID::vibSpeed);
        vibDelay  = s.getRawParameterValue (ParamID::vibDelay);
        playMode  = s.getRawParameterValue (ParamID::playMode);
        glideMode = s.getRawParameterValue (ParamID::glideMode);
        glideTime = s.getRawParameterValue (ParamID::glideTime);
        arpMode    = s.getRawParameterValue (ParamID::arpMode);
        arpPattern = s.getRawParameterValue (ParamID::arpPattern);
        arpSpeed   = s.getRawParameterValue (ParamID::arpSpeed);
        wheelDest   = s.getRawParameterValue (ParamID::wheelDest);
        wheelAmount = s.getRawParameterValue (ParamID::wheelAmount);
        bendUp      = s.getRawParameterValue (ParamID::bendUp);
        bendDown    = s.getRawParameterValue (ParamID::bendDown);
        bendTarget  = s.getRawParameterValue (ParamID::bendTarget);

        for (int d = 0; d < numDrums; ++d)
        {
            drums[(size_t) d].tune  = s.getRawParameterValue (drumParam (d, "tune"));
            drums[(size_t) d].decay = s.getRawParameterValue (drumParam (d, "decay"));
            drums[(size_t) d].sweep = s.getRawParameterValue (drumParam (d, "sweep"));
            drums[(size_t) d].noise = s.getRawParameterValue (drumParam (d, "noise"));
            drums[(size_t) d].wave  = s.getRawParameterValue (drumParam (d, "wave"));
            drums[(size_t) d].level = s.getRawParameterValue (drumParam (d, "level"));
        }
    }

    struct Drum
    {
        std::atomic<float>* tune  = nullptr;
        std::atomic<float>* decay = nullptr;
        std::atomic<float>* sweep = nullptr;
        std::atomic<float>* noise = nullptr;
        std::atomic<float>* wave  = nullptr;
        std::atomic<float>* level = nullptr;   // dB
    };
    std::array<Drum, numDrums> drums {};

    std::atomic<float>* ring      = nullptr;
    std::atomic<float>* sync      = nullptr;
    std::atomic<float>* modPitch  = nullptr;   // semitones
    std::atomic<float>* modFine   = nullptr;   // cents
    std::atomic<float>* vibDepth  = nullptr;   // cents
    std::atomic<float>* vibSpeed  = nullptr;   // Hz
    std::atomic<float>* vibDelay  = nullptr;   // ms
    std::atomic<float>* playMode  = nullptr;   // 0 poly, 1 mono
    std::atomic<float>* glideMode = nullptr;   // 0 always, 1 legato
    std::atomic<float>* glideTime = nullptr;   // ms
    std::atomic<float>* arpMode    = nullptr;  // ArpMode
    std::atomic<float>* arpPattern = nullptr;  // index into arpPatterns
    std::atomic<float>* arpSpeed   = nullptr;  // frames per step
    std::atomic<float>* wheelDest   = nullptr; // WheelDest
    std::atomic<float>* wheelAmount = nullptr; // -100..100 %
    std::atomic<float>* bendUp      = nullptr; // semitones
    std::atomic<float>* bendDown    = nullptr; // semitones
    std::atomic<float>* bendTarget  = nullptr; // BendTarget

    std::atomic<float>& mode;

    static int asInt (const std::atomic<float>& a) noexcept { return juce::roundToInt (a.load()); }

    std::atomic<float>& chipModel;
    std::atomic<float>& video;
    std::atomic<float>& analogNoise;
    std::atomic<float>& waveform;
    std::atomic<float>& pulseWidth;
    std::atomic<float>& pwmSpeed;
    std::atomic<float>& attack;
    std::atomic<float>& decay;
    std::atomic<float>& sustain;
    std::atomic<float>& release;
    std::atomic<float>& filterOn;
    std::atomic<float>& filterMode;
    std::atomic<float>& cutoff;
    std::atomic<float>& resonance;
    std::atomic<float>& volume;
    std::atomic<float>& outputGain;
};

} // namespace zadabar
