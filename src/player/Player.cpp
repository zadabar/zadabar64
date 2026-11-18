// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Player.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace zadabar
{

namespace
{
    constexpr uint8_t drumWaveBits[] = { Ctrl::triangle, Ctrl::saw, Ctrl::pulse, Ctrl::noise, Ctrl::pulse };
    constexpr int hubbardWave    = 4;
    constexpr int noiseWave      = 3;
    constexpr int drumGateFrames = 2;      // gate held for 2 frames, then the release finishes the sound
    constexpr double armSettleMs = 40.0;   // > one 15-bit rate-counter wrap (33.3 ms PAL, 32 ms NTSC)
    constexpr uint8_t voice3Off  = 0x80;   // $D418 bit 7: mute voice 3 (when it is not filtered)
    constexpr int64_t forever    = INT64_MAX / 2;

    double noteToHz (double note) { return 440.0 * std::pow (2.0, (note - 69.0) / 12.0); }

    // triangle LFO: phase 0..1 -> 0, +1, 0, -1, 0
    double triangle (double ph)
    {
        ph -= std::floor (ph);
        if (ph < 0.25) return 4.0 * ph;
        if (ph < 0.75) return 2.0 - 4.0 * ph;
        return 4.0 * ph - 4.0;
    }
}

void Player::prepare (double hostSampleRate, VideoStandard standard)
{
    // Real C64 frame rates: PAL 985248 / (312 * 63) ~ 50.12 Hz, NTSC 1022730 / (263 * 65) ~ 59.83 Hz
    const double frameHz = standard == VideoStandard::NTSC ? 1022730.0 / (263.0 * 65.0)
                                                           : 985248.0  / (312.0 * 63.0);
    sampleRate         = hostSampleRate;
    samplesPerFrame    = hostSampleRate / frameHz;
    samplesToNextFrame = samplesPerFrame;
    pitchBendSemis     = 0.0f;
    display.sampleRate = hostSampleRate;

    // The chips were just reset: every register is 0, i.e. every voice is
    // parked and armed (rates 0, gate off, frequency 0).
    voices = {};
    numPending = 0;
    drumVoices = {};
    nextDrumVoice = {};
    chipBusyUntil = {};
    awakeUntil = {};
    held = {};
    numHeld = numKeys = 0;
    lastPitch = -1.0;
    for (auto& v : display.voices) { v.gate = false; v.drum = -1; }
}

// ======================================================================== params

void Player::applyParams (const Params& p)
{
    const auto newMode = Params::asInt (p.mode) == 1 ? Mode::Drums : Mode::Instrument;
    const bool newShared = ! (p.ring->load() > 0.5f || p.sync->load() > 0.5f);
    if (newShared != sharedChip)
    {
        // Ring / sync switched: the notes move between one shared chip and a chip each.
        // Silence them in the old layout, then restart the held ones in the new one
        // (the processor mutes the switch for ~50 ms, so this is not heard as a click).
        std::array<int, numVoices> sounding {};
        for (int v = 0; v < numVoices; ++v)
        {
            sounding[(size_t) v] = voice (v).gate && voice (v).note >= 0 ? voice (v).note : -1;
            if (voice (v).gate) { setGate (v, false); writeControl (v); }
            park (v);
            voice (v).note = -1;
            voice (v).reserved = false;
        }
        sharedChip = newShared;
        pendingRestart = sounding;
    }
    const auto newArp  = static_cast<ArpMode> (std::clamp (Params::asInt (*p.arpMode), 0, 2));
    // CHORD arpeggio plays all held keys on one voice, so it is a mono mode too
    const bool newMono = Params::asInt (*p.playMode) == 1 || newArp == ArpMode::Chord;
    if (newMode != mode || newMono != mono || (newArp == ArpMode::Chord) != (arpMode == ArpMode::Chord))
    {
        // Silence and park everything, in both modes' layouts.
        for (int v = 0; v < numVoices; ++v) { setGate (v, false); park (v); voice (v).note = -1; voice (v).reserved = false; }
        for (int d = 0; d < numDrums; ++d)
            for (int v = 0; v < numVoices; ++v)
                parkDrumVoice (d, v);
        mode = newMode;
        mono = newMono;
        arpMode = newArp;
        chordLength = 0;
        held = {};
        numHeld = numKeys = 0;
        updateDrumDisplay();
    }

    hissOn = Params::asInt (p.analogNoise) > 0;

    // --- shared: filter and volume (drums go through the filter too)
    const bool isDrums   = mode == Mode::Drums;
    const int  cutoff    = std::clamp (Params::asInt (p.cutoff), 0, 2047);
    const bool filterOn  = p.filterOn.load() > 0.5f;
    const int  resonance = std::clamp (Params::asInt (p.resonance), 0, 15);
    const int  fmode     = std::clamp (Params::asInt (p.filterMode), 0, 3);
    const int  volume    = std::clamp (Params::asInt (p.volume), 0, 15);

    // Instrument: only voice 1 (the note) goes through the filter; voice 3 is the
    // muted modulator (3OFF only mutes a voice that is not routed to the filter).
    const bool    modLayout = ! isDrums && ! sharedChip;
    const uint8_t routing = filterOn ? (modLayout ? 0x01 : 0x07) : 0x00;
    const uint8_t modeVol = static_cast<uint8_t> (filterModeBits[fmode] | volume | (modLayout ? voice3Off : 0));
    for (auto* c : chips)
    {
        c->writeIfChanged (Reg::fcLo, static_cast<uint8_t> (cutoff & 0x07));
        c->writeIfChanged (Reg::fcHi, static_cast<uint8_t> (cutoff >> 3));
        c->writeIfChanged (Reg::resFilt, static_cast<uint8_t> ((resonance << 4) | routing));
        c->writeIfChanged (Reg::modeVolume, modeVol);
    }

    if (isDrums)
    {
        for (int d = 0; d < numDrums; ++d)
        {
            const auto& src = p.drums[(size_t) d];
            auto& dst = drums[(size_t) d];
            dst.tune  = Params::asInt (*src.tune);
            dst.decay = std::clamp (Params::asInt (*src.decay), 0, 15);
            dst.sweep = Params::asInt (*src.sweep);
            dst.noise = Params::asInt (*src.noise);
            dst.wave  = std::clamp (Params::asInt (*src.wave), 0, 4);
        }
        pendingRestart.fill (-1);   // no instrument notes in drum mode
        return;   // the drum programs own the voice registers
    }

    // --- instrument patch
    const int wave = std::clamp (Params::asInt (p.waveform), 0, 7);
    waveBits     = waveformBits[wave];
    basePulse    = Params::asInt (p.pulseWidth);
    pwmSpeed     = Params::asInt (p.pwmSpeed);
    releaseIndex = std::clamp (Params::asInt (p.release), 0, 15);
    adValue = static_cast<uint8_t> ((Params::asInt (p.attack)  << 4) | Params::asInt (p.decay));
    srValue = static_cast<uint8_t> ((Params::asInt (p.sustain) << 4) | Params::asInt (p.release));

    ringOn      = p.ring->load() > 0.5f;
    syncOn      = p.sync->load() > 0.5f;
    modOffset   = p.modPitch->load() + p.modFine->load() / 100.0;
    vibDepth    = p.vibDepth->load() / 100.0;
    vibSpeedHz  = p.vibSpeed->load();
    vibDelayMs  = p.vibDelay->load();
    legatoGlide = Params::asInt (*p.glideMode) == 1;
    glideMs     = p.glideTime->load();
    arpMode     = newArp;
    arpPattern  = std::clamp (Params::asInt (*p.arpPattern), 0, numArpPatterns - 1);
    arpSpeed    = std::clamp (Params::asInt (*p.arpSpeed), 1, 4);

    for (int v = 0; v < numVoices; ++v)
    {
        auto& c = chipOf (v);
        if (! voice (v).reserved)   // mid hard restart: leave its envelope alone
        {
            c.writeIfChanged (noteReg (v) + Reg::attackDecay,    adValue);
            c.writeIfChanged (noteReg (v) + Reg::sustainRelease, srValue);
        }
        writeControl (v);

        if (pwmSpeed == 0)
        {
            voice (v).pulseWidth = basePulse;
            writePulseWidth (v);
        }
        if (! voice (v).parked)
            writeFrequency (v);   // MOD PITCH / ring / sync changes take effect right away
    }

    for (int v = 0; v < numVoices; ++v)
        if (const int note = pendingRestart[(size_t) v]; note >= 0)
        {
            pendingRestart[(size_t) v] = -1;
            startVoice (v, note, false, note, -1);
        }
}

// ======================================================================== MIDI

void Player::handleMidi (const juce::MidiMessage& m, int reservedVoice)
{
    if (m.isNoteOn())
        noteOn (m.getNoteNumber(), reservedVoice);
    else if (m.isNoteOff())
        noteOff (m.getNoteNumber());
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        allNotesOff();
    else if (m.isPitchWheel() && mode == Mode::Instrument)
    {
        pitchBendSemis = 2.0f * (static_cast<float> (m.getPitchWheelValue()) - 8192.0f) / 8192.0f;
        for (int v = 0; v < numVoices; ++v)
            if (! voice (v).parked)
                writeFrequency (v);
    }
}

void Player::allNotesOff()
{
    if (mode == Mode::Drums)
        return;   // drums are one-shots and ring out on their own

    held = {};
    numHeld = numKeys = 0;
    for (int v = 0; v < numVoices; ++v)
        if (voice (v).gate)
            releaseVoice (v);
}

// ======================================================================== instrument

void Player::setGate (int v, bool on)
{
    auto& vc = voice (v);
    if (vc.gate == on)
        return;

    vc.gate = on;
    display.voices[(size_t) v].gate      = on;
    display.voices[(size_t) v].changedAt = clock;
}

int Player::chooseVoice() const
{
    if (mono)
        return 0;

    // 1) a free voice: take the one that was released longest ago
    int best = -1;
    for (int v = 0; v < numVoices; ++v)
        if (! voice (v).gate && ! voice (v).reserved && (best < 0 || voice (v).releasedAt < voice (best).releasedAt))
            best = v;
    if (best >= 0)
        return best;

    // 2) all three busy: steal the oldest note
    best = 0;
    for (int v = 1; v < numVoices; ++v)
        if (voice (v).startedAt < voice (best).startedAt)
            best = v;
    return best;
}

int Player::prepareNote (int)
{
    if (mode == Mode::Drums || mono)
        return -1;   // drums are always pre-armed; mono keeps legato notes untouched

    const int v = chooseVoice();
    auto& vc = voice (v);
    auto& c = chipOf (v);
    vc.reserved  = true;
    vc.startedAt = ++eventCounter;   // newest: won't be stolen by the next notes
    vc.note      = -1;
    vc.parkAt    = -1;
    setGate (v, false);
    setVoiceBusy (v, forever);

    // Gate off with all rates at their fastest: the envelope falls to zero in
    // a few ms, and the rate counter settles within one 15-bit wrap (33 ms).
    c.write (noteReg (v) + Reg::attackDecay, 0x00);
    c.write (noteReg (v) + Reg::sustainRelease, 0x00);
    c.write (noteReg (v) + Reg::control, static_cast<uint8_t> (c.shadow (noteReg (v) + Reg::control) & ~Ctrl::gate));
    return v;
}

void Player::noteOn (int note, int reservedVoice)
{
    if (mode == Mode::Drums)
    {
        if (const int d = drumForNote (note); d >= 0)
            triggerDrum (d);
        return;
    }

    const bool otherKeysHeld = numHeld > 0;
    if (! held[(size_t) note]) { held[(size_t) note] = true; ++numHeld; }

    if (arpMode == ArpMode::Chord)
    {
        // The held keys are the chord. The first key starts the note; keys that join
        // while it sounds only change the chord (no retrigger), like a tracker.
        auto& vc = voice (0);
        updateChord();
        if (otherKeysHeld && vc.gate)
        {
            vc.note = vc.lastNote = lowestHeld();
            vc.pitch = vc.note;
            writeFrequency (0);
            return;
        }
        startVoice (0, lowestHeld(), false, lastPitch, -1);
        return;
    }

    if (mono)
    {
        // last-note priority: remember the key order
        if (numKeys == static_cast<int> (keyStack.size()))
        {
            std::copy (keyStack.begin() + 1, keyStack.end(), keyStack.begin());
            --numKeys;
        }
        keyStack[(size_t) numKeys++] = note;

        auto& vc = voice (0);
        if (otherKeysHeld && vc.gate)
        {
            legatoTo (0, note, glideMs > 0.5);   // overlapping notes: no retrigger
            return;
        }
        const bool glide = glideMs > 0.5 && ! legatoGlide && lastPitch >= 0.0;
        startVoice (0, note, glide, vc.gate || ! vc.parked ? vc.pitch : lastPitch, -1);
        return;
    }

    const bool glide = glideMs > 0.5 && lastPitch >= 0.0 && (! legatoGlide || otherKeysHeld);
    const int v = reservedVoice >= 0 ? reservedVoice : chooseVoice();
    startVoice (v, note, glide, lastPitch, reservedVoice);
}

void Player::startVoice (int v, int note, bool glide, double glideFromPitch, int reservedVoice)
{
    auto& vc = voice (v);
    auto& c = chipOf (v);

    if (reservedVoice >= 0)
        vc.reserved = false;

    // Gate off first, so a voice that is still held (stolen) restarts its attack.
    if (vc.gate)
    {
        setGate (v, false);
        writeControl (v);
    }

    // A parked (silent) voice gets its oscillators reset, so every fresh note
    // starts from the same phase. The tiny step this makes is masked by the note.
    if (vc.parked)
    {
        c.write (noteReg (v) + Reg::control, Ctrl::test);
        if (! sharedChip)
            c.write (modulator + Reg::control, Ctrl::test);
    }

    // Open the gate with decay 0 and set the real decay one sample later (see deferDecay()).
    c.write (noteReg (v) + Reg::attackDecay,    static_cast<uint8_t> (adValue & 0xf0));
    c.write (noteReg (v) + Reg::sustainRelease, srValue);
    deferDecay (chipIndexOf (v), noteReg (v) + Reg::attackDecay, adValue);

    vc.note      = note;
    vc.lastNote  = note;
    vc.startedAt = ++eventCounter;
    vc.parked    = false;
    vc.parkAt    = -1;
    vc.framesOn  = 0;
    vc.vibPhase  = 0.0;
    vc.glideFrames = 0;
    vc.pitch     = note;
    vc.arpStep   = 0;   // every note starts on the root, the next step comes after exactly N frames
    vc.arpNextAt = static_cast<double> (clock) + arpSpeed * samplesPerFrame;
    if (glide)
    {
        vc.pitch       = glideFromPitch;
        vc.glideFrom   = glideFromPitch;
        vc.glideFrames = std::max (1, static_cast<int> (std::lround (glideMs * sampleRate / 1000.0 / samplesPerFrame)));
        vc.glideFrame  = 0;
    }
    lastPitch = note;
    setVoiceBusy (v, forever);

    if (pwmSpeed > 0)
    {
        vc.pulseWidth   = basePulse;
        vc.pwmDirection = 1;
        writePulseWidth (v);
    }

    writeFrequency (v);
    setGate (v, true);
    writeControl (v);
}

// Mono: move a sounding voice to a new note without retriggering the envelope.
void Player::legatoTo (int v, int note, bool glide)
{
    auto& vc = voice (v);
    vc.note     = note;
    vc.lastNote = note;
    if (glide)
    {
        vc.glideFrom   = vc.pitch;
        vc.glideFrames = std::max (1, static_cast<int> (std::lround (glideMs * sampleRate / 1000.0 / samplesPerFrame)));
        vc.glideFrame  = 0;
    }
    else
    {
        vc.pitch = note;
        vc.glideFrames = 0;
    }
    lastPitch = note;
    writeFrequency (v);
}

void Player::noteOff (int note)
{
    if (mode == Mode::Drums)
        return;   // drums are one-shots

    if (held[(size_t) note]) { held[(size_t) note] = false; --numHeld; }

    if (arpMode == ArpMode::Chord)
    {
        auto& vc = voice (0);
        if (! vc.gate)
            return;
        if (numHeld == 0)
        {
            releaseVoice (0);   // the last chord keeps cycling through the release, as on a C64
            return;
        }
        updateChord();
        vc.note = vc.lastNote = lowestHeld();
        vc.pitch = vc.note;
        writeFrequency (0);
        return;
    }

    if (mono)
    {
        // drop the key from the stack
        int w = 0;
        for (int i = 0; i < numKeys; ++i)
            if (keyStack[(size_t) i] != note)
                keyStack[(size_t) w++] = keyStack[(size_t) i];
        numKeys = w;

        auto& vc = voice (0);
        if (vc.gate && vc.note == note)
        {
            if (numKeys > 0)
                legatoTo (0, keyStack[(size_t) (numKeys - 1)], glideMs > 0.5);   // back to the previous held key
            else
                releaseVoice (0);
        }
        return;
    }

    for (int v = 0; v < numVoices; ++v)
        if (voice (v).note == note && voice (v).gate)
            releaseVoice (v);
}

void Player::releaseVoice (int v)
{
    auto& vc = voice (v);
    setGate (v, false);
    vc.note = -1;
    vc.releasedAt = ++eventCounter;
    writeControl (v);   // release phase starts on the real chip
    scheduleParking (v, sidDecayMs[releaseIndex]);
}

void Player::scheduleParking (int v, float releaseMs)
{
    // The SID's release is exponential and its timing table is approximate:
    // allow 20 % plus two frames before stopping the oscillator.
    voice (v).parkAt = clock + msToSamples (releaseMs * 1.2 + 40.0);
    setVoiceBusy (v, voice (v).parkAt + msToSamples (armSettleMs));
}

// A released voice on a real SID never goes fully silent: the oscillator keeps
// running and leaks through the envelope DAC (6581 especially). So once the
// release is over we stop the oscillators (frequency 0). The waveform stays
// selected: switching it off would make the 6581's output jump and click.
void Player::park (int v)
{
    auto& vc = voice (v);
    auto& c = chipOf (v);
    vc.parked = true;
    vc.parkAt = -1;
    c.writeIfChanged (noteReg (v) + Reg::freqLo, 0);
    c.writeIfChanged (noteReg (v) + Reg::freqHi, 0);
    if (! sharedChip)
    {
        c.writeIfChanged (modulator + Reg::freqLo, 0);
        c.writeIfChanged (modulator + Reg::freqHi, 0);
    }
    setVoiceBusy (v, std::max (std::min (voice (v).busyUntil, clock + msToSamples (armSettleMs)), clock));
}

// Pitch = note (+ glide) + pitch bend + vibrato. With hard sync the heard pitch is
// the modulator's (it restarts the carrier), so then the modulator plays the note
// and MOD PITCH moves the carrier instead.
void Player::writeFrequency (int v)
{
    const auto& vc = voice (v);
    double p = vc.pitch + arpOffset (v) + static_cast<double> (pitchBendSemis);

    const double vibOnMs = vc.framesOn * samplesPerFrame * 1000.0 / sampleRate;
    if (vibDepth > 0.0 && vibOnMs >= vibDelayMs)
        p += vibDepth * triangle (vc.vibPhase);

    const double carrierNote   = syncOn ? p + modOffset : p;
    const double modulatorNote = syncOn ? p : p + modOffset;

    auto& c = chipOf (v);
    const uint16_t cr = c.frequencyToRegister (noteToHz (carrierNote));
    const uint16_t mr = c.frequencyToRegister (noteToHz (modulatorNote));
    c.writeIfChanged (noteReg (v) + Reg::freqLo,   static_cast<uint8_t> (cr & 0xff));
    c.writeIfChanged (noteReg (v) + Reg::freqHi,   static_cast<uint8_t> (cr >> 8));
    if (! sharedChip)
    {
        c.writeIfChanged (modulator + Reg::freqLo, static_cast<uint8_t> (mr & 0xff));
        c.writeIfChanged (modulator + Reg::freqHi, static_cast<uint8_t> (mr >> 8));
    }
}

void Player::writeControl (int v)
{
    auto& c = chipOf (v);
    const uint8_t mods = static_cast<uint8_t> ((ringOn ? Ctrl::ring : 0) | (syncOn ? Ctrl::sync : 0));
    c.writeIfChanged (noteReg (v) + Reg::control, static_cast<uint8_t> (waveBits | mods | (voice (v).gate ? Ctrl::gate : 0)));
    if (! sharedChip)
        c.writeIfChanged (modulator + Reg::control, Ctrl::triangle);   // never gated: it is only a clock source
}

void Player::writePulseWidth (int v)
{
    const int pw = std::clamp (voice (v).pulseWidth, 0, 4095);
    display.voices[(size_t) v].pulseWidth = pw;
    chipOf (v).writeIfChanged (noteReg (v) + Reg::pwLo, static_cast<uint8_t> (pw & 0xff));
    chipOf (v).writeIfChanged (noteReg (v) + Reg::pwHi, static_cast<uint8_t> (pw >> 8));
}

// ======================================================================== drums

void Player::triggerDrum (int d)
{
    const int v = nextDrumVoice[(size_t) d];
    nextDrumVoice[(size_t) d] = (v + 1) % numVoices;

    auto& dv = drumVoices[(size_t) d][(size_t) v];
    const auto& ds = drums[(size_t) d];
    auto& c = chip (d);

    // This voice was armed (rates 0, gate off) at least one hit ago, so its
    // envelope rate counter is settled and the attack starts immediately.
    // TEST bit: reset the oscillator (and noise generator) for a consistent attack.
    c.write (base (v) + Reg::control, Ctrl::test);
    const auto decay = static_cast<uint8_t> (ds.decay);
    c.write (base (v) + Reg::attackDecay,    0x00);    // decay set right after the gate opens (deferDecay)
    c.write (base (v) + Reg::sustainRelease, decay);   // sustain 0, release = decay
    deferDecay (d, base (v) + Reg::attackDecay, decay);   // attack 0, decay
    c.writeIfChanged (base (v) + Reg::pwLo, 0x00);
    c.writeIfChanged (base (v) + Reg::pwHi, 0x08);     // 50 % pulse

    dv.active = true;
    dv.parked = false;
    dv.frame  = 0;
    dv.gate   = false;
    dv.parkAt = clock + msToSamples ((drumGateFrames * 20.0 + sidDecayMs[decay]) * 1.2 + 40.0);
    chipBusyUntil[(size_t) d] = std::max (chipBusyUntil[(size_t) d], dv.parkAt + msToSamples (armSettleMs));

    display.drumTriggeredAt[(size_t) d] = clock;
    writeDrumFrame (d, v);

    // Arm the voice this drum will use next (its oldest hit is cut short).
    armDrumVoice (d, nextDrumVoice[(size_t) d]);
    updateDrumDisplay();
}

void Player::armDrumVoice (int d, int v)
{
    auto& dv = drumVoices[(size_t) d][(size_t) v];
    auto& c = chip (d);

    c.write (base (v) + Reg::attackDecay,    0x00);
    c.write (base (v) + Reg::sustainRelease, 0x00);   // a ringing tail fades within ~6 ms
    c.write (base (v) + Reg::control, static_cast<uint8_t> (c.shadow (base (v) + Reg::control) & ~Ctrl::gate));
    dv.active = false;
    dv.gate   = false;
    if (! dv.parked)
        dv.parkAt = std::min (dv.parkAt, clock + msToSamples (armSettleMs));
    chipBusyUntil[(size_t) d] = std::max (chipBusyUntil[(size_t) d], clock + msToSamples (armSettleMs * 2.0));
}

void Player::parkDrumVoice (int d, int v)
{
    auto& dv = drumVoices[(size_t) d][(size_t) v];
    auto& c = chip (d);

    c.writeIfChanged (base (v) + Reg::freqLo, 0);
    c.writeIfChanged (base (v) + Reg::freqHi, 0);
    c.writeIfChanged (base (v) + Reg::attackDecay,    0x00);
    c.writeIfChanged (base (v) + Reg::sustainRelease, 0x00);
    c.writeIfChanged (base (v) + Reg::control, static_cast<uint8_t> (c.shadow (base (v) + Reg::control) & ~Ctrl::gate));
    dv.active = false;
    dv.gate   = false;
    dv.parked = true;
    dv.parkAt = -1;
    chipBusyUntil[(size_t) d] = std::max (chipBusyUntil[(size_t) d], clock + msToSamples (armSettleMs));
}

// One frame of a drum program:
//   frames [0, noise)   noise burst (the "click" / "snap")
//   then                body waveform, pitch = tune + sweep * 0.55^frame
//   Hubbard wave        alternates noise / pulse every frame
//   gate on for the first frames, then the release rings out
void Player::writeDrumFrame (int d, int v)
{
    auto& dv = drumVoices[(size_t) d][(size_t) v];
    const auto& ds = drums[(size_t) d];
    const int n = dv.frame;

    const bool attackNoise = n < ds.noise;
    const bool hubbardNoise = ds.wave == hubbardWave && (n % 2) == 0;

    double note;
    uint8_t wave;
    if (ds.wave == noiseWave)
    {
        wave = Ctrl::noise;
        note = ds.tune;                          // pure noise drums (hats) use their tune directly
    }
    else if (attackNoise)
    {
        wave = Ctrl::noise;
        note = std::min (ds.tune + 24, 108);     // a dull "thud" rather than a bright tick
    }
    else if (hubbardNoise)
    {
        wave = Ctrl::noise;
        note = std::min (ds.tune + 36, 120);     // bright snare noise
    }
    else
    {
        wave = drumWaveBits[ds.wave];
        note = ds.tune + ds.sweep * std::pow (0.55, n);
    }

    auto& c = chip (d);
    const uint16_t reg = c.frequencyToRegister (noteToHz (note));
    c.writeIfChanged (base (v) + Reg::freqLo, static_cast<uint8_t> (reg & 0xff));
    c.writeIfChanged (base (v) + Reg::freqHi, static_cast<uint8_t> (reg >> 8));

    dv.gate = n < drumGateFrames;
    c.writeIfChanged (base (v) + Reg::control, static_cast<uint8_t> (wave | (dv.gate ? Ctrl::gate : 0)));
}

void Player::updateDrumDisplay()
{
    for (int v = 0; v < numVoices; ++v)
    {
        int playing = -1;
        if (mode == Mode::Drums)
            for (int d = 0; d < numDrums && playing < 0; ++d)
                if (drumVoices[(size_t) d][(size_t) v].active)
                    playing = d;
        display.voices[(size_t) v].drum = playing;
    }
}

// ======================================================================== frame tick

void Player::frameTick()
{
    if (mode == Mode::Drums)
    {
        bool changed = false;
        for (int d = 0; d < numDrums; ++d)
            for (int v = 0; v < numVoices; ++v)
            {
                auto& dv = drumVoices[(size_t) d][(size_t) v];
                if (dv.parked)
                    continue;

                if (dv.parkAt >= 0 && clock >= dv.parkAt)
                {
                    parkDrumVoice (d, v);
                    changed = true;
                }
                else if (dv.active)
                {
                    ++dv.frame;
                    writeDrumFrame (d, v);
                }
            }
        if (changed)
            updateDrumDisplay();
        return;
    }

    const double frameSeconds = samplesPerFrame / sampleRate;
    for (int v = 0; v < numVoices; ++v)
    {
        auto& vc = voice (v);
        if (vc.parked)
            continue;

        if (! vc.gate && ! vc.reserved && vc.parkAt >= 0 && clock >= vc.parkAt)
        {
            park (v);
            continue;
        }

        // portamento: a straight line in semitones, one step per frame
        if (vc.glideFrames > 0)
        {
            ++vc.glideFrame;
            const double t = std::min (1.0, static_cast<double> (vc.glideFrame) / vc.glideFrames);
            vc.pitch = vc.glideFrom + (vc.lastNote - vc.glideFrom) * t;
            if (vc.glideFrame >= vc.glideFrames)
                vc.glideFrames = 0;
        }

        // vibrato: triangle LFO, starts after the delay
        ++vc.framesOn;
        if (vibDepth > 0.0 && vc.framesOn * frameSeconds * 1000.0 >= vibDelayMs)
            vc.vibPhase = std::fmod (vc.vibPhase + vibSpeedHz * frameSeconds, 1.0);

        // PWM sweep
        if (pwmSpeed > 0)
        {
            vc.pulseWidth += vc.pwmDirection * pwmSpeed;
            if (vc.pulseWidth >= 3968) { vc.pulseWidth = 3968; vc.pwmDirection = -1; }
            if (vc.pulseWidth <= 128)  { vc.pulseWidth = 128;  vc.pwmDirection =  1; }
            writePulseWidth (v);
        }

        writeFrequency (v);
    }
}

// ======================================================================== deferred writes

// The SID "ADSR bug", part 2: in the very first cycle after the gate opens, the
// chip compares its rate counter against the DECAY rate instead of the attack
// rate. With attack 0 and a different decay, the counter can slip past its
// reset point in exactly that cycle and must then run a full 15-bit lap
// (~33 ms) before the attack starts: about 1 note in 9.
// So we open the gate with decay 0 (same rate as attack 0) and write the real
// decay one sample later, long before the 2 ms attack is over.
void Player::deferDecay (int chipIndex, int reg, uint8_t attackDecay)
{
    for (int i = 0; i < numPending; ++i)   // replace a pending write for the same register
        if (pending[(size_t) i].chip == chipIndex && pending[(size_t) i].reg == reg)
        {
            pending[(size_t) i].value = attackDecay;
            pending[(size_t) i].due   = clock + 1;
            return;
        }

    if (numPending < maxPending)
        pending[(size_t) numPending++] = { chipIndex, reg, attackDecay, clock + 1 };
    else
        chip (chipIndex).write (reg, attackDecay);   // never happens; write now
}

void Player::applyPendingWrites()
{
    for (int i = 0; i < numPending; )
    {
        auto& w = pending[(size_t) i];
        if (w.due <= clock)
        {
            chip (w.chip).write (w.reg, w.value);
            w = pending[(size_t) --numPending];
        }
        else
        {
            ++i;
        }
    }
}

// ======================================================================== arpeggio

// The classic C64 chord: the voice's frequency jumps between the chord notes,
// one step every arpSpeed frames, counted from the moment the note started
// (on a C64 a note always starts on a frame, so every step is exactly N frames
// long). The gate stays open: the envelope runs on through the whole chord.
bool Player::arpActive (int v) const noexcept
{
    if (mode != Mode::Instrument || voice (v).parked)
        return false;
    if (arpMode == ArpMode::Fixed)
        return arpPatterns[arpPattern].length > 1;
    return arpMode == ArpMode::Chord && v == 0 && chordLength > 1;
}

double Player::arpOffset (int v) const noexcept
{
    if (mode != Mode::Instrument)
        return 0.0;
    const int step = voice (v).arpStep;
    if (arpMode == ArpMode::Fixed)
    {
        const auto& a = arpPatterns[arpPattern];
        return a.steps[step % a.length];
    }
    if (arpMode == ArpMode::Chord && v == 0 && chordLength > 0)
        return chordSteps[(size_t) (step % chordLength)];
    return 0.0;
}

int Player::lowestHeld() const noexcept
{
    for (int n = 0; n < 128; ++n)
        if (held[(size_t) n])
            return n;
    return voice (0).lastNote;
}

// CHORD mode: the held keys, lowest first, as steps above the lowest key.
void Player::updateChord()
{
    chordLength = 0;
    int root = -1;
    for (int n = 0; n < 128 && chordLength < static_cast<int> (chordSteps.size()); ++n)
        if (held[(size_t) n])
        {
            if (root < 0)
                root = n;
            chordSteps[(size_t) chordLength++] = n - root;
        }
}

void Player::advanceArpeggios()
{
    const double now = static_cast<double> (clock);
    for (int v = 0; v < numVoices; ++v)
    {
        if (! arpActive (v))
            continue;
        auto& vc = voice (v);
        if (now < vc.arpNextAt)
            continue;

        const double stepLength = arpSpeed * samplesPerFrame;
        ++vc.arpStep;
        vc.arpNextAt += stepLength;
        if (vc.arpNextAt <= now)                 // arpeggio just switched on mid-note
            vc.arpNextAt = now + stepLength;
        writeFrequency (v);
    }
}

// ======================================================================== chip clocking

// A chip is clocked while any of its voices needs it (or keepChipsAwake() asks).
void Player::setVoiceBusy (int v, int64_t until) noexcept
{
    voice (v).busyUntil = until;
    const int c = chipIndexOf (v);
    int64_t busy = awakeUntil[(size_t) c];
    for (int w = 0; w < numVoices; ++w)
        if (chipIndexOf (w) == c)
            busy = std::max (busy, voice (w).busyUntil);
    chipBusyUntil[(size_t) c] = busy;
}

// ======================================================================== render

void Player::render (float* const* outputs, int numSamples)
{
    std::array<float*, numChips> outs {};
    for (size_t c = 0; c < numChips; ++c)
        outs[c] = outputs[c];

    while (numSamples > 0)
    {
        applyPendingWrites();

        const int untilFrame = std::max (1, static_cast<int> (std::ceil (samplesToNextFrame)));
        int n = std::min (numSamples, untilFrame);
        for (int i = 0; i < numPending; ++i)   // stop exactly where a deferred write is due
            n = std::min (n, std::max (1, static_cast<int> (pending[(size_t) i].due - clock)));
        for (int v = 0; v < numVoices; ++v)    // ... and exactly where an arpeggio step is due
            if (arpActive (v))
                n = std::min (n, std::max (1, static_cast<int> (std::ceil (voice (v).arpNextAt - static_cast<double> (clock)))));

        for (size_t c = 0; c < numChips; ++c)
        {
            // a chip with nothing to play is not clocked; it holds its last (resting) output
            if (clock < chipBusyUntil[c] || (c == 0 && hissOn))
            {
                chips[c]->render (outs[c], n);
                lastOutput[c] = outs[c][n - 1];
            }
            else
            {
                std::fill (outs[c], outs[c] + n, lastOutput[c]);
            }
            outs[c] += n;
        }

        numSamples -= n;
        clock      += n;
        samplesToNextFrame -= n;
        advanceArpeggios();

        if (samplesToNextFrame <= 0.0)
        {
            frameTick();
            samplesToNextFrame += samplesPerFrame;
        }
    }
    display.clock = clock;
}

} // namespace zadabar
