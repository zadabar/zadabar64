// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace zadabar
{

namespace
{
    std::array<float*, Player::numChips> offset (float* const* bufs, int n)
    {
        std::array<float*, Player::numChips> r {};
        for (size_t i = 0; i < r.size(); ++i)
            r[i] = bufs[i] + n;
        return r;
    }
}

Zadabar64Processor::Zadabar64Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ZADABAR64", createParameterLayout()),
      params (apvts)
{
    settings->addChangeListener (this);
    midiLearn.fromString (settings->getMidiMap());   // the user's default map
}

Zadabar64Processor::~Zadabar64Processor()
{
    settings->removeChangeListener (this);
}

void Zadabar64Processor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    updateLatency();
}

// Tight timing = 40 ms lookahead: more than one full wrap of the SID's 15-bit
// envelope rate counter (32768 cycles = 33.3 ms PAL, 32.0 ms NTSC).
void Zadabar64Processor::updateLatency()
{
    const int samples = settings->getTiming() == 1 ? juce::roundToInt (sampleRate * 0.040) : 0;
    if (samples == lookahead.load())
        return;
    lookahead = samples;
    setLatencySamples (samples);
    updateHostDisplay (ChangeDetails().withLatencyChanged (true));
}

void Zadabar64Processor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    queueHead = queueCount = 0;
    lookahead = -1;
    updateLatency();
    mono.assign (static_cast<size_t> (juce::jmax (samplesPerBlock, 512)), 0.0f);
    for (auto& b : chipBuffers)
        b.assign (mono.size(), 0.0f);
    for (auto& gn : chipGain)
    {
        gn.reset (sampleRate, 0.003);
        gn.setCurrentAndTargetValue (1.0f);
    }
    keyboardState.reset();

    currentChipModel = -1;
    currentVideo     = -1;
    reconfigureChip();
    for (auto& c : chips)
        c.setAnalogNoise (Params::asInt (params.analogNoise));

    // Let the emulated chips settle before anyone hears them: the first volume
    // register write makes a DC step ("click") on the 6581, which the chip's
    // output filter takes a few hundred ms to remove.
    player.applyParams (params);
    prerollChips (0.5);

    // the chips have settled on these settings: a change before the first block must still be hidden
    structureKey = -1;
    settle = Settle::None;
    settleGain = 1.0f;
    beginStructureChange (0);

    dcX1 = dcY1 = 0.0f;
    dcCoeff = static_cast<float> (1.0 - juce::MathConstants<double>::twoPi * 5.0 / sampleRate);

    outputGain.reset (sampleRate, 0.05);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (params.outputGain.load()));
}

void Zadabar64Processor::prerollChips (double seconds)
{
    // Each chip runs a slightly different length (+7 ms per chip): reSIDfp's
    // dither generator starts from the same seed in every chip, and identical
    // noise in three chips would add up 9.5 dB instead of 4.8 dB.
    for (size_t k = 0; k < chips.size(); ++k)
        for (int done = 0, total = static_cast<int> (sampleRate * (seconds + 0.007 * static_cast<double> (k))); done < total; )
        {
            const int n = juce::jmin (static_cast<int> (mono.size()), total - done);
            chips[k].render (mono.data(), n);
            done += n;
        }
}

// (Re)initialises the emulated chip when the model or clock (PAL/NTSC) changes.
void Zadabar64Processor::reconfigureChip()
{
    const int model = Params::asInt (params.chipModel);
    const int video = Params::asInt (params.video);

    if (video != currentVideo)
    {
        // A new clock needs a new resampler setup, which also resets the chip.
        const auto standard = video == 1 ? VideoStandard::NTSC : VideoStandard::PAL;
        for (auto& c : chips)
            c.prepare (sampleRate, model == 1 ? ChipModel::CSG8580 : ChipModel::MOS6581, standard);
        player.prepare (sampleRate, standard);
        currentVideo     = video;
        currentChipModel = model;
    }
    else if (model != currentChipModel)
    {
        for (auto& c : chips)
            c.setChipModel (model == 1 ? ChipModel::CSG8580 : ChipModel::MOS6581);
        currentChipModel = model;
    }
}

bool Zadabar64Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void Zadabar64Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    if (static_cast<int> (mono.size()) < numSamples)
    {
        mono.resize (static_cast<size_t> (numSamples));   // host sent a bigger block than announced
        for (auto& b : chipBuffers)
            b.resize (mono.size());
    }
    float* chipOut[Player::numChips] {};
    for (size_t c = 0; c < chips.size(); ++c)
        chipOut[c] = chipBuffers[c].data();

    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    // MIDI learn: learned controllers move their parameters (before the block's
    // parameters are read, so they count from this block on). They are not
    // passed on to the player: a learned CC 1 replaces the mod wheel routing.
    for (const auto meta : midi)
        midiLearn.handle (meta.getMessage());

    if (! beginStructureChange (numSamples))
    {
        reconfigureChip();
        for (auto& c : chips)
            c.setAnalogNoise (Params::asInt (params.analogNoise));
        player.applyParams (params);
    }

    const int64_t blockStart = player.getClock();
    const int     la         = lookahead.load();

    if (la <= 0)
    {
        // Live timing: play queued leftovers (if Tight was just switched off), then
        // render sample-accurately: audio up to each MIDI event, then the event.
        while (queueCount > 0)
        {
            auto& e = queue[(size_t) queueHead];
            player.handleMidi (juce::MidiMessage (e.bytes, e.size), e.voice);
            queueHead = (queueHead + 1) % queueSize;
            --queueCount;
        }

        int pos = 0;
        for (const auto meta : midi)
        {
            const int eventPos = juce::jlimit (0, numSamples, meta.samplePosition);
            if (eventPos > pos)
            {
                player.render (offset (chipOut, pos).data(), eventPos - pos);
                pos = eventPos;
            }
            const auto msg = meta.getMessage();
            if (! midiLearn.isMapped (msg))
                player.handleMidi (msg);
        }
        if (pos < numSamples)
            player.render (offset (chipOut, pos).data(), numSamples - pos);
    }
    else
    {
        // Tight timing: queue every event `la` samples later; hard-restart note-ons now.
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            if (msg.getRawDataSize() > 3 || queueCount >= queueSize || midiLearn.isMapped (msg))
                continue;   // sysex etc. is not used; queue full = drop (never happens in practice)

            auto& e = queue[(size_t) ((queueHead + queueCount) % queueSize)];
            e.due  = blockStart + juce::jlimit (0, numSamples, meta.samplePosition) + la;
            e.size = msg.getRawDataSize();
            std::memcpy (e.bytes, msg.getRawData(), static_cast<size_t> (e.size));
            e.voice = msg.isNoteOn() ? player.prepareNote (msg.getNoteNumber()) : -1;
            ++queueCount;
        }

        renderUntil (blockStart + numSamples, chipOut, blockStart);
    }

    // Mix the chips into one mono signal. Drum mode: chip d is drum d, at its level.
    // Instrument mode: chips 0-2 each play one note.
    if (player.isDrumMode())
    {
        std::fill (mono.begin(), mono.begin() + numSamples, 0.0f);
        for (size_t c = 0; c < chips.size(); ++c)
        {
            const float db = params.drums[c].level->load();
            auto& gain = chipGain[c];
            gain.setTargetValue (db <= -47.9f ? 0.0f : juce::Decibels::decibelsToGain (db));
            const float* src = chipOut[c];
            for (int i = 0; i < numSamples; ++i)
                mono[(size_t) i] += src[i] * gain.getNextValue();
        }
    }
    else
    {
        // instrument: all notes on chip 0 (classic SID), or one chip per note (chips 0-2)
        // when ring mod / sync need a modulator voice per note
        if (player.isSharedLayout())
            std::copy (chipOut[0], chipOut[0] + numSamples, mono.begin());
        else
            for (int i = 0; i < numSamples; ++i)
                mono[(size_t) i] = chipOut[0][i] + chipOut[1][i] + chipOut[2][i];
    }

    // The SID is mono: copy to all output channels with the output gain.
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (params.outputGain.load()));
    auto* left = buffer.getWritePointer (0);
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float x = mono[static_cast<size_t> (i)];
        dcY1 = x - dcX1 + dcCoeff * dcY1;
        dcX1 = x;

        switch (settle)
        {
            case Settle::None:
                break;
            case Settle::FadeOut:   // reaches 0 at the end of this block
                settleGain = fadeOutFrom * static_cast<float> (juce::jmax (0, --settleCount)) / static_cast<float> (settleLength);
                break;
            case Settle::Mute:
                settleGain = 0.0f;
                if (--settleCount <= 0)
                {
                    dcX1 = x;       // the new resting level becomes the zero line
                    dcY1 = 0.0f;
                    settle = Settle::FadeIn;
                    settleLength = settleCount = juce::jmax (1, static_cast<int> (sampleRate * 0.005));
                }
                break;
            case Settle::FadeIn:
                settleGain = 1.0f - static_cast<float> (--settleCount) / static_cast<float> (settleLength);
                if (settleCount <= 0) { settle = Settle::None; settleGain = 1.0f; }
                break;
        }

        left[i] = dcY1 * settleGain * outputGain.getNextValue();
        peak = juce::jmax (peak, std::abs (left[i]));
    }

    // Keep the highest peak until the UI collects it.
    float prev = peakLevel.load();
    while (peak > prev && ! peakLevel.compare_exchange_weak (prev, peak)) {}

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

bool Zadabar64Processor::beginStructureChange (int numSamples)
{
    const int64_t key = static_cast<int64_t> (Params::asInt (params.mode))
                      | static_cast<int64_t> (params.filterOn.load() > 0.5f) << 1
                      | static_cast<int64_t> (Params::asInt (params.filterMode)) << 2
                      | static_cast<int64_t> (Params::asInt (params.chipModel)) << 4
                      | static_cast<int64_t> (params.ring->load() > 0.5f || params.sync->load() > 0.5f) << 5
                      | static_cast<int64_t> (patches.loadCounter.load()) << 8;
    const int muteSamples = static_cast<int> (sampleRate * 0.040);

    if (structureKey < 0)
    {
        structureKey = key;
        return false;
    }
    if (settle == Settle::FadeOut)
    {
        // faded out last block: switch now, unheard, and let every chip settle
        structureKey = key;
        settle = Settle::Mute;
        settleCount = muteSamples;
        player.keepChipsAwake (muteSamples + numSamples + static_cast<int> (sampleRate * 0.01));
        return false;
    }
    if (key == structureKey)
        return false;

    if (settle == Settle::Mute)
    {
        structureKey = key;             // already silent: switch right away, mute a bit longer
        settleCount = muteSamples;
        player.keepChipsAwake (muteSamples + numSamples + static_cast<int> (sampleRate * 0.01));
        return false;
    }
    settle = Settle::FadeOut;           // this block still plays the old settings, fading to silence
    fadeOutFrom = settleGain;
    settleLength = settleCount = juce::jmax (1, numSamples);
    return true;
}

// Renders up to endClock, playing queued events at their due sample.
void Zadabar64Processor::renderUntil (int64_t endClock, float* const* out, int64_t blockStartClock)
{
    while (player.getClock() < endClock)
    {
        int64_t next = endClock;
        if (queueCount > 0)
            next = juce::jmin (endClock, juce::jmax (player.getClock(), queue[(size_t) queueHead].due));

        if (next > player.getClock())
        {
            const auto from = player.getClock() - blockStartClock;
            player.render (offset (out, static_cast<int> (from)).data(), static_cast<int> (next - player.getClock()));
        }

        while (queueCount > 0 && queue[(size_t) queueHead].due <= player.getClock())
        {
            auto& e = queue[(size_t) queueHead];
            player.handleMidi (juce::MidiMessage (e.bytes, e.size), e.voice);
            queueHead = (queueHead + 1) % queueSize;
            --queueCount;
        }
    }
}

juce::AudioProcessorEditor* Zadabar64Processor::createEditor()
{
    return new Zadabar64Editor (*this);
}

void Zadabar64Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("selectedDrum", selectedDrum.load(), nullptr);
    patches.writeState (state);
    midiLearn.writeState (state);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Zadabar64Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            midiLearn.readState (state);   // older projects have no map: keep the default one
            if (auto learnTree = state.getChildWithName (MidiLearn::stateTag); learnTree.isValid())
                state.removeChild (learnTree, nullptr);
            selectedDrum = juce::jlimit (0, numDrums - 1, static_cast<int> (state.getProperty ("selectedDrum", 0)));
            apvts.replaceState (state);
            patches.readState (state);
        }
}

void Zadabar64Processor::loadFactoryPatch (int index)
{
    const auto& f = factoryPatches();
    if (index < 0 || index >= static_cast<int> (f.size()))
        return;
    // a drum kit from the host's list also switches to drum mode (and back)
    if (auto* m = apvts.getParameter (ParamID::mode))
    {
        m->beginChangeGesture();
        m->setValueNotifyingHost (f[(size_t) index].type == PatchType::DrumKit ? 1.0f : 0.0f);
        m->endChangeGesture();
    }
    patches.loadFactory (index);
}

} // namespace zadabar

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new zadabar::Zadabar64Processor();
}
