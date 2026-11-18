// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace reSIDfp { class residfp; }

namespace zadabar
{

// SID register offsets ($D400 + offset on a real C64).
namespace Reg
{
    // Per voice: base = voice * 7
    constexpr int freqLo = 0, freqHi = 1, pwLo = 2, pwHi = 3, control = 4, attackDecay = 5, sustainRelease = 6;
    constexpr int voiceStride = 7;

    constexpr int fcLo       = 0x15;  // filter cutoff bits 0-2
    constexpr int fcHi       = 0x16;  // filter cutoff bits 3-10
    constexpr int resFilt    = 0x17;  // resonance (bits 4-7) | voice->filter routing (bits 0-2)
    constexpr int modeVolume = 0x18;  // LP/BP/HP/3OFF (bits 4-7) | master volume (bits 0-3)
}

// Control register bits
namespace Ctrl
{
    constexpr uint8_t gate     = 0x01;
    constexpr uint8_t sync     = 0x02;
    constexpr uint8_t ring     = 0x04;
    constexpr uint8_t test     = 0x08;
    constexpr uint8_t triangle = 0x10;
    constexpr uint8_t saw      = 0x20;
    constexpr uint8_t pulse    = 0x40;
    constexpr uint8_t noise    = 0x80;
}

enum class ChipModel { MOS6581 = 0, CSG8580 = 1 };
enum class VideoStandard { PAL = 0, NTSC = 1 };

// Thin wrapper around one reSIDfp chip:
//  - keeps a shadow copy of the write-only registers (so callers can read back
//    what they wrote and so unchanged values aren't rewritten)
//  - renders float audio at the host sample rate
class SidChip
{
public:
    SidChip();
    ~SidChip();

    void prepare (double hostSampleRate, ChipModel, VideoStandard);
    void setChipModel (ChipModel);
    ChipModel getChipModel() const noexcept     { return model; }
    double getClockHz() const noexcept          { return clockHz; }

    void reset();

    // Background hiss of a real C64: 0 = off .. 8. (reSIDfp level 1 is a
    // barely-there dither, which avoids quantisation noise; we use it for "off".)
    void setAnalogNoise (int level);

    void write (int reg, uint8_t value);
    void writeIfChanged (int reg, uint8_t value);
    uint8_t shadow (int reg) const noexcept     { return regs[static_cast<size_t> (reg & 0x1f)]; }

    // Renders exactly numSamples mono samples (range about -1..+1).
    void render (float* out, int numSamples);

    // SID frequency register value for a frequency in Hz.
    uint16_t frequencyToRegister (double hz) const noexcept;

    static constexpr double palClockHz  = 985248.0;
    static constexpr double ntscClockHz = 1022730.0;

private:
    std::unique_ptr<reSIDfp::residfp> sid;
    std::array<uint8_t, 32> regs {};
    std::vector<short> scratch;

    ChipModel model     = ChipModel::MOS6581;
    double    clockHz   = palClockHz;
    double    sampleRate = 44100.0;
    int       noiseLevel = -1;
};

} // namespace zadabar
