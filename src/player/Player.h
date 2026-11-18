// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <cstdint>

#include <juce_audio_basics/juce_audio_basics.h>

#include "Parameters.h"
#include "sid/SidChip.h"

namespace zadabar
{

// What the UI may look at (written by the audio thread, read by the editor).
struct PlayerDisplay
{
    struct Voice
    {
        std::atomic<bool>    gate      { false };
        std::atomic<int64_t> changedAt { 0 };     // sample clock of the last gate change
        std::atomic<int>     drum      { -1 };    // drum mode: a drum is sounding on this voice slot
        std::atomic<int>     pulseWidth { 2048 }; // instrument: current pulse width (moves with PWM)
    };

    std::array<Voice, 3>             voices;
    std::array<std::atomic<int64_t>, numDrums> drumTriggeredAt {};
    std::atomic<int64_t>             clock      { 0 };
    std::atomic<double>              sampleRate { 44100.0 };
};

// The "player routine": turns MIDI into SID register writes, the way a C64
// music driver does. Per-frame work happens in frameTick() at the C64 frame
// rate (PAL ~50 Hz / NTSC ~60 Hz).
//
// Instrument mode: up to 3 notes, each on its own chip (note v on chips[v]):
// SID voice 1 plays the note, SID voice 3 is a silent modulator (muted with
// the 3OFF bit) for ring modulation and hard sync at the note's pitch + MOD
// PITCH. Vibrato, portamento and PWM are updated once per frame, like a real
// C64 player routine. Poly or mono (last-note priority, legato).
//
// Drum mode: every drum has its own chip (drum d on chips[d]), so each drum has
// its own level in the mix and drums never cut each other off. Inside its chip
// a drum rotates over the 3 voices, and the voice it will use next is "armed"
// (envelope rates 0, gate off) as soon as the current hit starts. By the next
// hit its envelope rate counter has settled, so the SID's ADSR bug can't delay
// or swallow the hit (it would otherwise make most hits ~33 ms late).
class Player
{
public:
    static constexpr int numVoices = 3;
    static constexpr int numChips  = numDrums;
    enum class Mode { Instrument = 0, Drums = 1 };

    explicit Player (std::array<SidChip*, numChips> chipsToDrive) : chips (chipsToDrive) {}

    void prepare (double hostSampleRate, VideoStandard);

    // Called at the start of every audio block: pushes knob values to the chips.
    void applyParams (const Params&);

    // reservedVoice: the voice prepared by prepareNote() for this note-on, or -1.
    void handleMidi (const juce::MidiMessage&, int reservedVoice = -1);
    void allNotesOff();

    // "Hard restart" for Tight timing (instrument mode), called ~40 ms before a
    // note-on is played. Returns the voice to pass to handleMidi() later, or -1.
    // Drum mode needs no preparation (voices are pre-armed) and returns -1.
    int prepareNote (int note);

    // Renders numSamples, running frame ticks at the right sample positions.
    // outputs[c] receives chip c; chips that are idle get silence and are not
    // clocked at all (their state is frozen, which is harmless while silent).
    void render (float* const* outputs, int numSamples);

    bool    isDrumMode() const noexcept { return mode == Mode::Drums; }
    int64_t getClock() const noexcept   { return clock; }

    // Clock every chip for a while, idle or not (lets them settle after a switch).
    void keepChipsAwake (int64_t samples) noexcept
    {
        for (size_t c = 0; c < chipBusyUntil.size(); ++c)
        {
            awakeUntil[c]    = std::max (awakeUntil[c], clock + samples);
            chipBusyUntil[c] = std::max (chipBusyUntil[c], awakeUntil[c]);
        }
    }

    // Instrument layout. Ring mod and sync need a free modulator voice per note, so
    // then every note gets its own chip (voice 1 = note, voice 3 = muted modulator).
    // Without them the three notes share one chip, like on a real C64: a third of the CPU.
    bool isSharedLayout() const noexcept { return sharedChip; }

    PlayerDisplay& getDisplay() noexcept             { return display; }
    const PlayerDisplay& getDisplay() const noexcept { return display; }

private:
    // ---------------------------------------------------------------- instrument
    struct Voice
    {
        int      note         = -1;     // MIDI note, -1 = free
        int      lastNote     = 60;     // keeps the pitch while the release rings out
        bool     gate         = false;
        double   pitch        = 60.0;   // current pitch in semitones (moves while gliding)
        double   glideFrom    = 60.0;
        int      glideFrames  = 0;      // 0 = no glide running
        int      glideFrame   = 0;
        int      framesOn     = 0;      // frames since the note started (vibrato delay)
        double   vibPhase     = 0.0;    // 0..1
        uint32_t startedAt    = 0;      // for "steal the oldest"
        uint32_t releasedAt   = 0;      // for "reuse the longest-released"
        int      pulseWidth   = 2048;   // current 12-bit pulse width (moves with PWM)
        int      pwmDirection = 1;
        bool     parked       = true;   // oscillator stopped (see park())
        bool     reserved     = false;  // hard-restarting, waiting for its note (Tight timing)
        int64_t  parkAt       = -1;     // sample clock at which the release is surely over
        int64_t  busyUntil    = 0;      // this voice needs its chip clocked until then
        int      arpStep      = 0;      // arpeggio: current step, counted from the note start
        double   arpNextAt    = 0.0;    // sample clock of the next arpeggio step
    };

    // ---------------------------------------------------------------- drums
    struct DrumVoice
    {
        bool    active = false;          // drum program running
        int     frame  = 0;
        bool    gate   = false;
        bool    parked = true;
        int64_t parkAt = -1;
    };

    struct DrumSettings { int tune = 48, decay = 8, sweep = 0, noise = 0, wave = 0; };

    void noteOn (int note, int reservedVoice);
    void noteOff (int note);
    void frameTick();

    int  chooseVoice() const;
    void startVoice (int v, int note, bool glide, double glideFromPitch, int reservedVoice);
    void legatoTo (int v, int note, bool glide);
    void releaseVoice (int v);
    void writeFrequency (int v);
    void writeControl (int v);
    void writePulseWidth (int v);
    void setGate (int v, bool on);
    void scheduleParking (int v, float releaseMs);
    void park (int v);
    bool arpActive (int v) const noexcept;
    double arpOffset (int v) const noexcept;
    void updateChord();
    int  lowestHeld() const noexcept;
    void advanceArpeggios();

    void triggerDrum (int drum);
    void writeDrumFrame (int drum, int v);
    void armDrumVoice (int drum, int v);
    void parkDrumVoice (int drum, int v);
    void updateDrumDisplay();

    // Register writes that must happen a little later (see deferDecay()).
    struct PendingWrite { int chip = 0; int reg = 0; uint8_t value = 0; int64_t due = 0; };
    static constexpr int maxPending = 32;
    std::array<PendingWrite, maxPending> pending {};
    int numPending = 0;
    void deferDecay (int chipIndex, int reg, uint8_t attackDecay);
    void applyPendingWrites();

    static int base (int v) noexcept { return v * Reg::voiceStride; }

    // where instrument voice v lives (see isSharedLayout())
    int      chipIndexOf (int v) const noexcept { return sharedChip ? 0 : v; }
    SidChip& chipOf (int v) noexcept            { return chip (chipIndexOf (v)); }
    int      noteReg (int v) const noexcept     { return sharedChip ? base (v) : carrier; }
    void     setVoiceBusy (int v, int64_t until) noexcept;
    static constexpr int carrier   = 0;                    // instrument: SID voice 1 plays the note
    static constexpr int modulator = 2 * Reg::voiceStride; // instrument: SID voice 3, muted (3OFF)
    Voice& voice (int v) noexcept             { return voices[static_cast<size_t> (v)]; }
    const Voice& voice (int v) const noexcept { return voices[static_cast<size_t> (v)]; }
    SidChip& chip (int c) noexcept            { return *chips[static_cast<size_t> (c)]; }
    int64_t msToSamples (double ms) const noexcept { return static_cast<int64_t> (ms * sampleRate / 1000.0); }

    std::array<SidChip*, numChips> chips;
    std::array<Voice, numVoices> voices {};
    std::array<std::array<DrumVoice, numVoices>, numDrums> drumVoices {};
    std::array<int, numDrums>     nextDrumVoice {};
    std::array<int64_t, numChips> chipBusyUntil {};   // clock until which a chip must be clocked
    std::array<int64_t, numChips> awakeUntil {};      // keepChipsAwake()
    std::array<int, numVoices>    pendingRestart { -1, -1, -1 };   // notes to restart after a layout switch
    bool sharedChip = true;
    std::array<float, numChips>   lastOutput {};      // held while a chip is idle
    std::array<DrumSettings, numDrums> drums {};

    Mode     mode               = Mode::Instrument;
    double   sampleRate         = 44100.0;
    double   samplesPerFrame    = 44100.0 / 50.0;
    double   samplesToNextFrame = 0.0;
    uint32_t eventCounter       = 0;
    int64_t  clock              = 0;
    float    pitchBendSemis     = 0.0f;

    // Latest instrument parameter values, cached for the frame tick.
    uint8_t  waveBits     = Ctrl::pulse;
    int      basePulse    = 2048;
    int      pwmSpeed     = 0;
    int      releaseIndex = 9;
    uint8_t  adValue      = 0x09;
    uint8_t  srValue      = 0xa9;

    // modulation
    bool     ringOn = false, syncOn = false;
    double   modOffset    = 7.0;     // semitones (MOD PITCH + MOD FINE)
    double   vibDepth     = 0.0;     // semitones
    double   vibSpeedHz   = 5.5;
    double   vibDelayMs   = 250.0;
    bool     mono         = false;
    bool     hissOn       = false;   // ANALOG NOISE > 0: keep chip 0 running so the hiss never stops
    bool     legatoGlide  = false;   // glide only between overlapping notes
    double   glideMs      = 0.0;
    double   lastPitch    = -1.0;    // last note played, for portamento
    std::array<int, 16> keyStack {}; // held keys, mono mode (oldest first)
    int      numKeys      = 0;
    std::array<bool, 128> held {};
    int      numHeld      = 0;

    // arpeggio
    ArpMode  arpMode      = ArpMode::Off;
    int      arpPattern   = 0;
    int      arpSpeed     = 1;       // frames per step
    std::array<int, 16> chordSteps {}; // CHORD mode: held keys above the lowest one, in semitones
    int      chordLength  = 0;

    PlayerDisplay display;
};

} // namespace zadabar
