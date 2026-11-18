// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Widgets.h"

#include <algorithm>
#include <cmath>

#include "PluginProcessor.h"
#include "sid/SidChip.h"

namespace zadabar
{

// ------------------------------------------------------------------ SegmentedChoice

SegmentedChoice::SegmentedChoice (juce::AudioProcessorValueTreeState& state, const juce::String& paramID,
                                  const juce::StringArray& labels, Section section, float fontSize)
{
    setSection (*this, section);
    auto* param = state.getParameter (paramID);
    jassert (param != nullptr);
    numOptions = labels.size();

    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = buttons.add (new juce::TextButton (labels[i]));
        b->setClickingTogglesState (false);
        b->getProperties().set ("fontSize", fontSize);
        b->onClick = [this, i] { if (attachment != nullptr) attachment->setValueAsCompleteGesture (static_cast<float> (i)); };
        addAndMakeVisible (b);
    }

    attachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
    {
        const int sel = juce::roundToInt (v);
        for (int i = 0; i < buttons.size(); ++i)
            buttons[i]->setToggleState (i == sel, juce::dontSendNotification);
    });
    attachment->sendInitialUpdate();
}

void SegmentedChoice::resized()
{
    auto r = getLocalBounds();
    constexpr int gap = 4;
    const int w = (r.getWidth() - gap * (numOptions - 1)) / juce::jmax (1, numOptions);
    for (auto* b : buttons)
    {
        b->setBounds (r.removeFromLeft (w));
        r.removeFromLeft (gap);
    }
}

// ------------------------------------------------------------------ LevelMeter

LevelMeter::LevelMeter (Zadabar64Processor& p) : processor (p)
{
    startTimerHz (30);
}

void LevelMeter::timerCallback()
{
    // dB scale: -48 dB .. 0 dB fills the meter
    const float peakLin = processor.takePeakLevel();
    const float db = juce::Decibels::gainToDecibels (peakLin, -60.0f);
    const float target = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);

    level = target > level ? target : juce::jmax (target, level - 0.03f);   // fast up, slow fall

    if (level >= peak) { peak = level; peakHoldTicks = 30; }
    else if (--peakHoldTicks < 0) peak = juce::jmax (level, peak - 0.02f);

    if (std::abs (level - drawnLevel) > 1.0e-4f || std::abs (peak - drawnPeak) > 1.0e-4f)
    {
        drawnLevel = level;
        drawnPeak  = peak;
        repaint();
    }
}

void LevelMeter::paint (juce::Graphics& g)
{
    if (auto* skin = skinOf (*this))
        skin->paintMeter (g, getLocalBounds(), level, peak);
}

// ------------------------------------------------------------------ WaveScope

namespace
{
    constexpr double scopeRate  = 96000.0;
    constexpr double scopeHz    = 187.5;      // 512 samples per period at 96 kHz
    constexpr int    scopeCycle = 512;
    constexpr int    carrierV   = 0;
    constexpr int    modV       = 2 * Reg::voiceStride;
}

WaveScope::WaveScope (Zadabar64Processor& p) : processor (p), chip (std::make_unique<SidChip>())
{
    samples.resize ((size_t) (2 * scopeCycle));
    startTimerHz (30);
}

WaveScope::~WaveScope() = default;

void WaveScope::timerCallback()
{
    if (isShowing() && capture())
        repaint();
}

bool WaveScope::capture()
{
    auto& st = processor.apvts;
    auto get = [&st] (const char* id) { return st.getRawParameterValue (id)->load(); };

    const int model = juce::roundToInt (get (ParamID::chipModel));
    if (model != chipModel)
    {
        // New chip: let it settle (6581 DC step), then measure a full-scale saw for the scale.
        chipModel = model;
        chip->prepare (scopeRate, model == 1 ? ChipModel::CSG8580 : ChipModel::MOS6581, VideoStandard::PAL);
        chip->write (Reg::modeVolume, 0x0f);
        chip->write (Reg::resFilt, 0x00);
        chip->write (carrierV + Reg::attackDecay, 0x00);
        chip->write (carrierV + Reg::sustainRelease, 0xf0);
        const uint16_t f = chip->frequencyToRegister (scopeHz);
        chip->write (carrierV + Reg::freqLo, (uint8_t) (f & 0xff));
        chip->write (carrierV + Reg::freqHi, (uint8_t) (f >> 8));
        chip->write (carrierV + Reg::control, Ctrl::saw | Ctrl::gate);
        std::vector<float> warm ((size_t) scopeRate / 2);
        chip->render (warm.data(), (int) warm.size());
        chip->render (samples.data(), scopeCycle);
        const auto [lo, hi] = std::minmax_element (samples.begin(), samples.begin() + scopeCycle);
        calMid   = 0.5f * (*lo + *hi);
        calRange = std::max (1.0e-4f, *hi - *lo);
    }

    const int  wave     = juce::jlimit (0, 7, juce::roundToInt (get (ParamID::waveform)));
    const auto bits     = waveformBits[wave];
    const bool ring     = get (ParamID::ring) > 0.5f;
    const bool sync     = get (ParamID::sync) > 0.5f;
    const double offset = get (ParamID::modPitch) + get (ParamID::modFine) / 100.0;

    // Pulse width: the one a playing voice has right now (PWM moves it), else the knob.
    int pw = juce::roundToInt (get (ParamID::pulseWidth));
    const auto& disp = processor.getDisplay();
    int64_t newest = -1;
    for (const auto& v : disp.voices)
        if (v.gate.load() && v.drum.load() < 0 && v.changedAt.load() > newest)
        {
            newest = v.changedAt.load();
            pw = v.pulseWidth.load();
        }

    // Only run the hidden chip when the picture would change (new settings, or PWM moving).
    const auto signature = juce::String (model) + "/" + juce::String (wave) + "/" + juce::String ((int) ring) + juce::String ((int) sync)
                         + "/" + juce::String (offset, 3) + "/" + juce::String ((bits & Ctrl::pulse) != 0 ? pw : -1);
    if (signature == traceSignature && ! trace.empty())
        return false;
    traceSignature = signature;

    // Same layout as a sounding note: with sync the modulator plays the note, the oscillator is offset.
    const double ratio  = std::pow (2.0, offset / 12.0);
    const double carHz  = sync ? scopeHz * ratio : scopeHz;
    const double modHz  = sync ? scopeHz : scopeHz * ratio;
    const uint16_t cf = chip->frequencyToRegister (carHz), mf = chip->frequencyToRegister (modHz);
    chip->write (carrierV + Reg::freqLo, (uint8_t) (cf & 0xff));
    chip->write (carrierV + Reg::freqHi, (uint8_t) (cf >> 8));
    chip->write (modV + Reg::freqLo, (uint8_t) (mf & 0xff));
    chip->write (modV + Reg::freqHi, (uint8_t) (mf >> 8));
    chip->write (carrierV + Reg::pwLo, (uint8_t) (pw & 0xff));
    chip->write (carrierV + Reg::pwHi, (uint8_t) ((pw >> 8) & 0x0f));

    const uint8_t mods = (uint8_t) ((ring ? Ctrl::ring : 0) | (sync ? Ctrl::sync : 0));
    const uint8_t ctrl = (uint8_t) (bits | mods | Ctrl::gate);
    if ((bits & Ctrl::noise) == 0)
    {
        // TEST restarts both oscillators, so the trace always starts at the same phase.
        chip->write (carrierV + Reg::control, (uint8_t) (ctrl | Ctrl::test));
        chip->write (modV + Reg::control, (uint8_t) (Ctrl::triangle | Ctrl::test));
        float dummy[8];
        chip->render (dummy, 8);
        chip->write (modV + Reg::control, Ctrl::triangle);
    }
    chip->write (carrierV + Reg::control, ctrl);
    chip->render (samples.data(), (int) samples.size());

    // centre the trace, keep the scale fixed: quiet combined waveforms look quiet
    const auto [lo, hi] = std::minmax_element (samples.begin(), samples.end());
    const float mid = 0.5f * (*lo + *hi);
    trace.resize (samples.size());
    for (size_t i = 0; i < samples.size(); ++i)
        trace[i] = { static_cast<float> (i) / static_cast<float> (samples.size() - 1),
                     juce::jlimit (0.0f, 1.0f, 0.5f + 0.85f * (samples[i] - mid) / calRange) };
    return true;
}

void WaveScope::paint (juce::Graphics& g)
{
    if (auto* skin = skinOf (*this))
    {
        if (trace.empty())
            capture();
        skin->paintScope (g, getLocalBounds(), trace);
    }
}

// ------------------------------------------------------------------ EnvelopeView

namespace
{
    constexpr const float* attackMs = sidAttackMs;
    constexpr const float* decayMs  = sidDecayMs;

    struct EnvLayout { float a, d, s, r, xA, xD, xS, xR, sus; };

    // Segment widths grow with sqrt(time) so both 2 ms and 8 s stay readable.
    EnvLayout layoutFor (int A, int D, int S, int R)
    {
        EnvLayout e {};
        e.a = attackMs[A]; e.d = decayMs[D]; e.r = decayMs[R];
        e.sus = static_cast<float> (S) / 15.0f;
        const float wa = std::sqrt (e.a), wd = std::sqrt (e.d), wr = std::sqrt (e.r);
        const float hold = (wa + wd + wr) * 0.35f + 4.0f;
        const float total = wa + wd + hold + wr;
        e.xA = wa / total;
        e.xD = e.xA + wd / total;
        e.xS = e.xD + hold / total;
        e.xR = 1.0f;
        return e;
    }

    float expFall (float t) { return std::exp (-4.0f * t); }   // 0..1 -> 1..~0
}

EnvelopeView::EnvelopeView (Zadabar64Processor& p) : processor (p)
{
    startTimerHz (30);
}

void EnvelopeView::timerCallback()
{
    // Redraw while voices move along the curve, or when the envelope settings change.
    auto& st = processor.apvts;
    juce::String sig;
    for (auto* id : { ParamID::attack, ParamID::decay, ParamID::sustain, ParamID::release })
        sig << juce::roundToInt (st.getRawParameterValue (id)->load()) << "/";
    bool anyGate = false;
    for (const auto& v : processor.getDisplay().voices)
        anyGate = anyGate || v.gate.load();
    if (anyGate || hadDots || sig != drawnSignature)
    {
        drawnSignature = sig;
        repaint();
    }
}

void EnvelopeView::paint (juce::Graphics& g)
{
    auto* skin = skinOf (*this);
    if (skin == nullptr)
        return;

    auto& st = processor.apvts;
    auto get = [&st] (const char* id) { return juce::jlimit (0, 15, juce::roundToInt (st.getRawParameterValue (id)->load())); };
    const auto e = layoutFor (get (ParamID::attack), get (ParamID::decay), get (ParamID::sustain), get (ParamID::release));

    std::vector<juce::Point<float>> curve;
    curve.emplace_back (0.0f, 0.0f);
    curve.emplace_back (e.xA, 1.0f);
    constexpr int steps = 24;
    for (int i = 1; i <= steps; ++i)
    {
        const float t = static_cast<float> (i) / steps;
        curve.emplace_back (e.xA + (e.xD - e.xA) * t, e.sus + (1.0f - e.sus) * expFall (t));
    }
    curve.emplace_back (e.xS, e.sus);
    for (int i = 1; i <= steps; ++i)
    {
        const float t = static_cast<float> (i) / steps;
        curve.emplace_back (e.xS + (e.xR - e.xS) * t, e.sus * (i == steps ? 0.0f : expFall (t)));
    }

    // one dot per sounding voice, placed by how long its gate has been on / off
    std::vector<juce::Point<float>> dots;
    const auto& disp = processor.getDisplay();
    const double sr = disp.sampleRate.load();
    const int64_t now = disp.clock.load();
    for (const auto& v : disp.voices)
    {
        if (v.drum.load() >= 0)
            continue;
        const float ms = static_cast<float> (static_cast<double> (now - v.changedAt.load()) * 1000.0 / sr);
        if (v.gate.load())
        {
            if (ms < e.a)
                dots.emplace_back (e.xA * ms / e.a, ms / e.a);
            else if (ms < e.a + e.d)
            {
                const float t = (ms - e.a) / e.d;
                dots.emplace_back (e.xA + (e.xD - e.xA) * t, e.sus + (1.0f - e.sus) * expFall (t));
            }
            else
            {
                const float t = juce::jmin (1.0f, (ms - e.a - e.d) / 2000.0f);
                dots.emplace_back (e.xD + (e.xS - e.xD) * t, e.sus);
            }
        }
        else if (v.changedAt.load() > 0 && ms < e.r && e.sus > 0.0f)
        {
            const float t = ms / e.r;
            dots.emplace_back (e.xS + (e.xR - e.xS) * t, e.sus * expFall (t));
        }
    }

    hadDots = ! dots.empty();
    skin->paintEnvelope (g, getLocalBounds(), curve, dots);
}

// ------------------------------------------------------------------ DrumView

DrumView::DrumView (Zadabar64Processor& p) : processor (p)
{
    startTimerHz (15);
}

void DrumView::paint (juce::Graphics& g)
{
    auto* skin = skinOf (*this);
    if (skin == nullptr)
        return;

    // Mirrors Player::writeDrumFrame: noise frames first, then the body sweeping down.
    const int d = processor.selectedDrum.load();
    auto& st = processor.apvts;
    auto get = [&] (const char* what) { return juce::roundToInt (st.getRawParameterValue (drumParam (d, what))->load()); };
    const int tune = get ("tune"), sweep = get ("sweep"), noise = get ("noise"), wave = get ("wave");

    constexpr int frames = 16;
    auto y = [] (double note) { return static_cast<float> (juce::jlimit (0.0, 1.0, (note - 12.0) / 108.0)); };

    std::vector<juce::Point<float>> curve, dots;
    for (int n = 0; n < frames; ++n)
    {
        bool noiseFrame = n < noise || (wave == 4 && n % 2 == 0);
        const double note = noiseFrame ? (wave == 3 ? tune : juce::jmin (tune + 36, 120))
                                       : tune + sweep * std::pow (0.55, n);
        const float x0 = static_cast<float> (n) / frames, x1 = static_cast<float> (n + 1) / frames;
        curve.emplace_back (x0, y (note));
        curve.emplace_back (x1, y (note));
        if (noiseFrame)
            dots.emplace_back ((x0 + x1) * 0.5f, y (note));
    }
    skin->paintEnvelope (g, getLocalBounds(), curve, dots);
}

// ------------------------------------------------------------------ DrumPads

DrumPads::DrumPads (Zadabar64Processor& p, std::function<void (int)> cb)
    : processor (p), onSelect (std::move (cb))
{
    setSection (*this, Section::Drums);

    for (int i = 0; i < numDrums; ++i)
    {
        auto* sl = levels.add (new juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox));
        sl->setPopupDisplayEnabled (true, true, this);
        sl->setDoubleClickReturnValue (true, 0.0);
        sl->setTitle (juce::String (drumNames[i]) + " level");
        addAndMakeVisible (sl);
        levelAttachments.add (new juce::AudioProcessorValueTreeState::SliderAttachment (
            processor.apvts, drumParam (i, "level"), *sl));
    }
    startTimerHz (30);
}

void DrumPads::resized()
{
    for (int i = 0; i < numDrums; ++i)
    {
        auto pad = padBounds (i);
        levels[i]->setBounds (pad.getX(), pad.getBottom() + 6, pad.getWidth(), faderHeight);
    }
}

juce::Rectangle<int> DrumPads::padBounds (int i) const
{
    constexpr int cols = numDrums, rows = 1, gap = 8;
    const int w = (getWidth()  - gap * (cols - 1)) / cols;
    const int h = (getHeight() - faderHeight - 6 - gap * (rows - 1)) / rows;
    return { (i % cols) * (w + gap), (i / cols) * (h + gap), w, h };
}

void DrumPads::timerCallback()
{
    // Pads light up for 180 ms after a hit: repaint only while one is lit (and once after).
    const auto& disp = processor.getDisplay();
    const double flashSamples = disp.sampleRate.load() * 0.18;
    const int64_t now = disp.clock.load();
    bool lit = false;
    for (int i = 0; i < numDrums; ++i)
    {
        const int64_t trig = disp.drumTriggeredAt[(size_t) i].load();
        lit = lit || (trig > 0 && static_cast<double> (now - trig) < flashSamples);
    }
    if (lit || flashing)
        repaint();
    flashing = lit;
}

void DrumView::timerCallback()
{
    const int d = processor.selectedDrum.load();
    juce::String sig (d);
    for (auto* what : { "tune", "decay", "sweep", "noise", "wave" })
        sig << "/" << juce::roundToInt (processor.apvts.getRawParameterValue (drumParam (d, what))->load());
    if (sig != drawnSignature)
    {
        drawnSignature = sig;
        repaint();
    }
}

void DrumPads::paint (juce::Graphics& g)
{
    auto* skin = skinOf (*this);
    if (skin == nullptr)
        return;

    const auto& disp = processor.getDisplay();
    const double flashSamples = disp.sampleRate.load() * 0.18;
    const int64_t now = disp.clock.load();

    for (int i = 0; i < numDrums; ++i)
    {
        const int64_t trig = disp.drumTriggeredAt[(size_t) i].load();
        const float flash = trig > 0 ? juce::jlimit (0.0f, 1.0f, 1.0f - static_cast<float> (static_cast<double> (now - trig) / flashSamples)) : 0.0f;
        skin->paintPad (g, padBounds (i), drumNames[i], noteName (drumPadNotes[i]),
                        processor.selectedDrum.load() == i, flash);
    }
}

void DrumPads::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < numDrums; ++i)
        if (padBounds (i).contains (e.getPosition()))
        {
            if (onSelect)
                onSelect (i);
            heldNote = drumPadNotes[i];
            processor.keyboardState.noteOn (10, heldNote, 0.9f);   // audition
            repaint();
            return;
        }
}

void DrumPads::mouseUp (const juce::MouseEvent&)
{
    if (heldNote >= 0)
        processor.keyboardState.noteOff (10, heldNote, 0.0f);
    heldNote = -1;
}

} // namespace zadabar
