// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The factory patches. Values are in the units shown on the knobs:
//   waveform   0 TRI 1 SAW 2 PULSE 3 NOISE 4 SAW+TRI 5 PULSE+TRI 6 PULSE+SAW 7 ALL
//   pulseWidth 0..4095 (2048 = square), pwmSpeed 0..255 per frame
//   attack/decay/sustain/release 0..15 (SID steps), cutoff 0..2047, resonance 0..15
//   filterMode 0 LP 1 BP 2 HP 3 NOTCH, chipModel 0 = 6581, 1 = 8580
//   modPitch semitones, modFine cents, vibDepth cents, vibSpeed Hz, vibDelay ms
//   playMode 0 POLY 1 MONO, glideMode 0 ALWAYS 1 LEGATO, glideTime ms
//   arpMode 0 OFF 1 CHORD 2 FIXED, arpPattern = index into arpPatterns, arpSpeed frames
//   drums: <drum>_tune (MIDI note), _decay 0..15, _sweep semitones, _noise frames,
//          _wave 0 TRI 1 SAW 2 PULSE 3 NOISE 4 HUBBARD, _level dB

#include "Patches.h"

#include <deque>

namespace zadabar
{

namespace
{
    using V = std::vector<PatchValue>;
    constexpr auto I = PatchType::Instrument;
    constexpr auto K = PatchType::DrumKit;

    // one drum, all six values
    void drum (V& v, const char* id, float tune, float decay, float sweep, float noise, float wave, float level = 0.0f)
    {
        static std::deque<juce::String> keep;   // stable storage for the generated IDs
        for (auto [what, value] : { std::pair<const char*, float> { "tune", tune }, { "decay", decay }, { "sweep", sweep },
                                    { "noise", noise }, { "wave", wave }, { "level", level } })
        {
            keep.push_back (juce::String (id) + "_" + what);
            v.push_back ({ keep.back().toRawUTF8(), value });
        }
    }

    V kit (V base, std::initializer_list<std::array<float, 6>> drums)
    {
        const char* ids[] = { "kick", "snare", "chat", "ohat", "ltom", "htom" };
        int i = 0;
        for (const auto& d : drums)
            drum (base, ids[i++], d[0], d[1], d[2], d[3], d[4], d[5]);
        return base;
    }

    std::vector<FactoryPatch> build()
    {
        std::vector<FactoryPatch> p;

        // ------------------------------------------------------------ instrument
        p.push_back ({ "INIT", I, {} });

        // leads
        p.push_back ({ "HUBBARD LEAD", I, {   // narrow PWM pulse, delayed vibrato, legato slides
            { "waveform", 2 }, { "pulseWidth", 1100 }, { "pwmSpeed", 28 },
            { "attack", 0 }, { "decay", 9 }, { "sustain", 11 }, { "release", 7 },
            { "filterOn", 0 },
            { "vibDepth", 35 }, { "vibSpeed", 6.0f }, { "vibDelay", 300 },
            { "playMode", 1 }, { "glideMode", 1 }, { "glideTime", 60 } } });
        p.push_back ({ "SYNC SCREAMER", I, {   // hard sync a twelfth above, through a bright LP
            { "waveform", 1 }, { "sync", 1 }, { "modPitch", 19 }, { "modFine", 20 },
            { "attack", 0 }, { "decay", 10 }, { "sustain", 12 }, { "release", 7 },
            { "filterOn", 1 }, { "cutoff", 1800 }, { "resonance", 6 },
            { "vibDepth", 25 }, { "vibSpeed", 5.5f }, { "vibDelay", 400 },
            { "playMode", 1 }, { "glideMode", 1 }, { "glideTime", 80 } } });
        p.push_back ({ "GALWAY FLUTE", I, {    // soft triangle with a slow, wide vibrato
            { "waveform", 0 },
            { "attack", 3 }, { "decay", 9 }, { "sustain", 11 }, { "release", 8 },
            { "filterOn", 0 },
            { "vibDepth", 40 }, { "vibSpeed", 5.0f }, { "vibDelay", 350 },
            { "playMode", 1 }, { "glideMode", 1 }, { "glideTime", 40 } } });
        p.push_back ({ "8580 BRASS", I, {      // the 8580's loud combined pulse+saw
            { "chipModel", 1 }, { "waveform", 6 }, { "pulseWidth", 2048 },
            { "attack", 2 }, { "decay", 10 }, { "sustain", 12 }, { "release", 7 },
            { "filterOn", 1 }, { "cutoff", 1300 }, { "resonance", 10 },
            { "vibDepth", 20 }, { "vibSpeed", 6.0f }, { "vibDelay", 300 },
            { "playMode", 1 } } });
        p.push_back ({ "THIN PULSE LEAD", I, {  // needle-thin pulse, fast vibrato
            { "waveform", 2 }, { "pulseWidth", 420 },
            { "attack", 0 }, { "decay", 9 }, { "sustain", 12 }, { "release", 6 },
            { "filterOn", 0 },
            { "vibDepth", 30 }, { "vibSpeed", 6.5f }, { "vibDelay", 200 },
            { "playMode", 1 }, { "glideMode", 1 }, { "glideTime", 30 } } });

        // basses
        p.push_back ({ "ACID SID BASS", I, {    // resonant 6581 low-pass, legato slides
            { "waveform", 1 },
            { "attack", 0 }, { "decay", 6 }, { "sustain", 5 }, { "release", 3 },
            { "filterOn", 1 }, { "cutoff", 420 }, { "resonance", 14 },
            { "playMode", 1 }, { "glideMode", 1 }, { "glideTime", 90 } } });
        p.push_back ({ "OCTAVE BASS", I, {      // the 1-frame octave jump bass
            { "waveform", 1 },
            { "attack", 0 }, { "decay", 5 }, { "sustain", 8 }, { "release", 3 },
            { "filterOn", 1 }, { "cutoff", 1100 }, { "resonance", 9 },
            { "arpMode", 2 }, { "arpPattern", 2 }, { "arpSpeed", 1 },
            { "playMode", 1 } } });
        p.push_back ({ "PULSE BASS", I, {
            { "waveform", 2 }, { "pulseWidth", 1024 }, { "pwmSpeed", 12 },
            { "attack", 0 }, { "decay", 7 }, { "sustain", 9 }, { "release", 4 },
            { "filterOn", 1 }, { "cutoff", 700 }, { "resonance", 6 },
            { "playMode", 1 }, { "glideMode", 1 }, { "glideTime", 50 } } });
        p.push_back ({ "SYNC BASS", I, {
            { "waveform", 1 }, { "sync", 1 }, { "modPitch", 7 },
            { "attack", 0 }, { "decay", 6 }, { "sustain", 6 }, { "release", 3 },
            { "filterOn", 1 }, { "cutoff", 1350 }, { "resonance", 5 },
            { "playMode", 1 } } });

        // chords and arpeggios
        p.push_back ({ "CHORD BUZZ", I, {       // hold a chord: the classic 1-frame C64 arpeggio
            { "waveform", 2 }, { "pulseWidth", 2048 }, { "pwmSpeed", 20 },
            { "attack", 0 }, { "decay", 8 }, { "sustain", 11 }, { "release", 7 },
            { "filterOn", 1 }, { "cutoff", 1500 }, { "resonance", 4 },
            { "arpMode", 1 }, { "arpSpeed", 1 } } });
        p.push_back ({ "SLOW CHORD ARP", I, {   // CHORD mode at 3 frames per note
            { "waveform", 0 },
            { "attack", 0 }, { "decay", 9 }, { "sustain", 10 }, { "release", 9 },
            { "filterOn", 0 },
            { "arpMode", 1 }, { "arpSpeed", 3 } } });
        p.push_back ({ "MINOR ARP KEYS", I, {
            { "waveform", 2 }, { "pulseWidth", 1600 },
            { "attack", 0 }, { "decay", 7 }, { "sustain", 8 }, { "release", 7 },
            { "filterOn", 1 }, { "cutoff", 1400 }, { "resonance", 3 },
            { "arpMode", 2 }, { "arpPattern", 1 }, { "arpSpeed", 2 } } });
        p.push_back ({ "DOM7 STAB", I, {        // short saw stab, four-note arpeggio
            { "waveform", 1 },
            { "attack", 0 }, { "decay", 7 }, { "sustain", 0 }, { "release", 7 },
            { "filterOn", 1 }, { "cutoff", 1600 }, { "resonance", 5 },
            { "arpMode", 2 }, { "arpPattern", 8 }, { "arpSpeed", 1 } } });
        p.push_back ({ "2 OCTAVE SPARKLE", I, {
            { "waveform", 0 },
            { "attack", 0 }, { "decay", 9 }, { "sustain", 6 }, { "release", 10 },
            { "filterOn", 0 },
            { "arpMode", 2 }, { "arpPattern", 10 }, { "arpSpeed", 1 } } });

        // bells, pads, effects
        p.push_back ({ "RING BELLS", I, {       // inharmonic ring mod, long decay
            { "waveform", 0 }, { "ring", 1 }, { "modPitch", 19 }, { "modFine", -14 },
            { "attack", 0 }, { "decay", 11 }, { "sustain", 0 }, { "release", 11 },
            { "filterOn", 0 } } });
        p.push_back ({ "METAL GONG", I, {
            { "waveform", 0 }, { "ring", 1 }, { "modPitch", -5 }, { "modFine", 33 },
            { "attack", 0 }, { "decay", 12 }, { "sustain", 0 }, { "release", 12 },
            { "filterOn", 1 }, { "cutoff", 1600 }, { "resonance", 10 } } });
        p.push_back ({ "SID PAD", I, {          // slow attack, slow PWM, gentle vibrato
            { "waveform", 2 }, { "pulseWidth", 1800 }, { "pwmSpeed", 6 },
            { "attack", 10 }, { "decay", 11 }, { "sustain", 12 }, { "release", 11 },
            { "filterOn", 1 }, { "cutoff", 800 }, { "resonance", 6 }, { "volume", 12 },
            { "vibDepth", 15 }, { "vibSpeed", 4.5f }, { "vibDelay", 600 } } });
        p.push_back ({ "SAW+TRI ORGAN", I, {    // combined wave, loud on the 8580
            { "chipModel", 1 }, { "waveform", 4 },
            { "attack", 0 }, { "decay", 0 }, { "sustain", 15 }, { "release", 5 },
            { "filterOn", 0 }, { "volume", 11 },
            { "vibDepth", 10 }, { "vibSpeed", 6.0f }, { "vibDelay", 0 } } });
        p.push_back ({ "NOISE WIND", I, {       // noise through a screaming band-pass
            { "waveform", 3 },
            { "attack", 11 }, { "decay", 11 }, { "sustain", 10 }, { "release", 12 },
            { "filterOn", 1 }, { "filterMode", 1 }, { "cutoff", 900 }, { "resonance", 15 }, { "volume", 11 } } });
        p.push_back ({ "SIREN", I, {
            { "waveform", 0 },
            { "attack", 0 }, { "decay", 0 }, { "sustain", 15 }, { "release", 6 },
            { "filterOn", 0 }, { "volume", 12 },
            { "vibDepth", 100 }, { "vibSpeed", 3.0f }, { "vibDelay", 0 } } });

        // ------------------------------------------------------------ drum kits
        // tune, decay, sweep, noise frames, wave, level dB
        p.push_back ({ "INIT KIT", K, {} });
        p.push_back ({ "CLASSIC 64", K, kit ({ { "filterOn", 0 } }, {
            { 31, 7, 24, 1, 0,  0 },     // kick
            { 50, 7, 12, 1, 4, -2 },     // snare (Hubbard)
            { 96, 3,  0, 0, 3, -6 },     // closed hat
            { 96, 9,  0, 0, 3, -7 },     // open hat
            { 43, 8, 14, 1, 0, -1 },     // low tom
            { 50, 7, 14, 1, 0, -1 } }) });
        p.push_back ({ "HUBBARD KIT", K, kit ({ { "filterOn", 0 } }, {
            { 29, 6, 30, 1, 0,  0 },
            { 52, 6, 14, 1, 4, -1 },
            { 100, 2, 0, 0, 3, -7 },
            { 98, 8,  0, 0, 3, -8 },
            { 41, 8, 18, 1, 0, -1 },
            { 48, 7, 18, 1, 0, -1 } }) });
        p.push_back ({ "GALWAY KIT", K, kit ({ { "filterOn", 1 }, { "cutoff", 1700 }, { "resonance", 5 } }, {
            { 33, 7, 20, 2, 0,  0 },
            { 55, 7, 20, 2, 4, -1 },
            { 92, 3,  0, 0, 3, -5 },
            { 92, 9,  0, 0, 3, -6 },
            { 45, 9, 12, 2, 2, -3 },     // pulse toms
            { 52, 8, 12, 2, 2, -3 } }) });
        p.push_back ({ "8580 TIGHT", K, kit ({ { "chipModel", 1 }, { "filterOn", 1 }, { "cutoff", 1800 }, { "resonance", 4 } }, {
            { 30, 5, 28, 1, 0,  1 },
            { 54, 5, 10, 1, 4, -1 },
            { 104, 1, 0, 0, 3, -6 },
            { 100, 7, 0, 0, 3, -7 },
            { 45, 6, 16, 1, 0, -1 },
            { 52, 6, 16, 1, 0, -1 } }) });
        p.push_back ({ "BOOMY 6581", K, kit ({ { "filterOn", 1 }, { "cutoff", 1300 }, { "resonance", 12 } }, {
            { 26, 9, 30, 2, 0,  0 },     // long resonant boom
            { 48, 8, 16, 2, 4, -1 },
            { 92, 3,  0, 0, 3, -4 },
            { 92, 9,  0, 0, 3, -5 },
            { 38, 9, 20, 1, 0, -1 },
            { 45, 9, 20, 1, 0, -1 } }) });
        p.push_back ({ "ELECTRO 64", K, kit ({ { "filterOn", 0 } }, {
            { 28, 8, 36, 0, 0,  0 },     // deep sweep, no click
            { 84, 6,  0, 0, 3, -4 },     // noise clap
            { 108, 1, 0, 0, 3, -8 },
            { 104, 6, 0, 0, 3, -9 },
            { 40, 7, 24, 0, 2, -5 },     // "pew" pulse toms
            { 47, 7, 24, 0, 2, -5 } }) });
        p.push_back ({ "LOFI SAW KIT", K, kit ({ { "filterOn", 1 }, { "cutoff", 900 }, { "resonance", 9 } }, {
            { 30, 6, 24, 1, 1,  0 },
            { 47, 7, 18, 2, 4,  0 },
            { 88, 2,  0, 0, 3, -2 },
            { 88, 8,  0, 0, 3, -3 },
            { 40, 7, 12, 1, 1, -2 },
            { 47, 7, 12, 1, 1, -2 } }) });

        return p;
    }
}

const std::vector<FactoryPatch>& factoryPatches()
{
    static const std::vector<FactoryPatch> patches = build();
    return patches;
}

} // namespace zadabar
