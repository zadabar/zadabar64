// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#include "SidChip.h"

#include <algorithm>
#include <cmath>

#include "residfp/residfp.h"

namespace zadabar
{

SidChip::SidChip()
    : sid (std::make_unique<reSIDfp::residfp>())
{
}

SidChip::~SidChip() = default;

void SidChip::prepare (double hostSampleRate, ChipModel newModel, VideoStandard standard)
{
    sampleRate = hostSampleRate;
    clockHz    = standard == VideoStandard::NTSC ? ntscClockHz : palClockHz;

    setChipModel (newModel);

    // High-quality sinc resampling from the ~1 MHz chip clock down to the host rate.
    // reSIDfp's two-pass sinc resampler crashes above ~200 kHz (measured: 200 kHz fine,
    // 250 kHz and up segfault). Up there everything audible is far below Nyquist, so the
    // plain decimator is used instead.
    const auto method = sampleRate <= 200000.0 ? reSIDfp::RESAMPLE : reSIDfp::DECIMATE;
    sid->setSamplingParameters (clockHz, method, sampleRate);

    scratch.assign (4096, 0);
    reset();
}

void SidChip::setChipModel (ChipModel newModel)
{
    model = newModel;
    sid->setChipModel (newModel == ChipModel::CSG8580 ? reSIDfp::CSG8580 : reSIDfp::MOS6581);
}

void SidChip::setAnalogNoise (int level)
{
    level = std::clamp (level, 0, 8);
    if (level == noiseLevel)
        return;
    noiseLevel = level;
    sid->noiseLevel (std::max (1, level));   // never 0: reSIDfp needs a little dither
}

void SidChip::reset()
{
    sid->reset();
    regs.fill (0);
}

void SidChip::write (int reg, uint8_t value)
{
    reg &= 0x1f;
    regs[static_cast<size_t> (reg)] = value;
    sid->write (reg, value);
}

void SidChip::writeIfChanged (int reg, uint8_t value)
{
    if (regs[static_cast<size_t> (reg & 0x1f)] != value)
        write (reg, value);
}

void SidChip::render (float* out, int numSamples)
{
    constexpr float scale = 1.0f / 32768.0f;

    while (numSamples > 0)
    {
        const int n = std::min (numSamples, static_cast<int> (scratch.size()));
        sid->clock (scratch.data(), n);   // produces exactly n samples

        for (int i = 0; i < n; ++i)
            out[i] = static_cast<float> (scratch[static_cast<size_t> (i)]) * scale;

        out        += n;
        numSamples -= n;
    }
}

uint16_t SidChip::frequencyToRegister (double hz) const noexcept
{
    // SID oscillator: f_out = reg * clock / 2^24
    const double reg = hz * 16777216.0 / clockHz;
    return static_cast<uint16_t> (std::clamp (std::lround (reg), 0L, 65535L));
}

} // namespace zadabar
