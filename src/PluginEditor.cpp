// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginEditor.h"

namespace zadabar
{

namespace Layout
{
    constexpr int width    = 1040;
    constexpr int height   = 720;
    constexpr int margin   = 14;    // skin A draws its C64 border here
    constexpr int header   = 76;
    constexpr int strip    = 28;
    constexpr int keyboard = 62;
    constexpr int gap      = 10;
    constexpr int knobCell = 104;   // label + knob + value
}

static juce::StringArray upper (const juce::StringArray& in)
{
    juce::StringArray out;
    for (auto& s : in)
        out.add (s.toUpperCase());
    return out;
}

Zadabar64Editor::Zadabar64Editor (Zadabar64Processor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      modeSwitch (p.apvts, ParamID::mode, { "INSTRUMENT", "DRUMS" }, Section::Chip),
      chipModel  (p.apvts, ParamID::chipModel, { "6581", "8580" }, Section::Chip),
      video      (p.apvts, ParamID::video, { "PAL", "NTSC" }, Section::Chip),
      envelopeView (p),
      waveScope (p),
      playMode   (p.apvts, ParamID::playMode, { "POLY", "MONO" }, Section::Glide),
      glideMode  (p.apvts, ParamID::glideMode, { "ALWAYS", "LEGATO" }, Section::Glide),
      arpMode    (p.apvts, ParamID::arpMode, { "OFF", "CHORD", "FIXED" }, Section::Arp, 7.0f),
      filterMode (p.apvts, ParamID::filterMode, { "LP", "BP", "HP", "NOTCH" }, Section::Filter),
      meter (p),
      drumPads (p, [this] (int i) { selectDrum (i); }),
      drumView (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      patchBar (p),
      saveDialog (p)
{
    skinIndex = settings->getSkin();
    skin = Skin::create (skinIndex);
    setLookAndFeel (skin.get());
    settings->addChangeListener (this);

    // header
    addAndMakeVisible (modeSwitch);
    setSection (setupButton, Section::Setup);
    setupButton.onClick = [this] { showSetup (! setupOpen); };
    addAndMakeVisible (setupButton);

    // chip
    setupCaption (modelCaption, "MODEL", Section::Chip);
    setupCaption (clockCaption, "CLOCK", Section::Chip);
    addAndMakeVisible (chipModel);
    addAndMakeVisible (video);
    setupKnob (analogNoise, "ANALOG NOISE", Section::Chip, ParamID::analogNoise);

    // oscillator
    setupCaption (waveCaption, "WAVEFORM", Section::Osc);
    setSection (waveform, Section::Osc);
    waveform.addItemList (upper (waveformNames), 1);
    addAndMakeVisible (waveform);
    waveformAttachment = std::make_unique<ComboAttachment> (p.apvts, ParamID::waveform, waveform);
    setupKnob (pulseWidth, "PULSE W", Section::Osc, ParamID::pulseWidth);
    setupKnob (pwmSpeed,   "PWM",     Section::Osc, ParamID::pwmSpeed);
    setSection (waveScope, Section::Osc);
    addAndMakeVisible (waveScope);

    // envelope
    setSection (envelopeView, Section::Env);
    addAndMakeVisible (envelopeView);
    setupKnob (attack,  "ATTACK",  Section::Env, ParamID::attack);
    setupKnob (decay,   "DECAY",   Section::Env, ParamID::decay);
    setupKnob (sustain, "SUSTAIN", Section::Env, ParamID::sustain);
    setupKnob (release, "RELEASE", Section::Env, ParamID::release);

    // ring mod / sync
    for (auto* b : { &ringButton, &syncButton })
    {
        setSection (*b, Section::Mod);
        b->setClickingTogglesState (true);
        addAndMakeVisible (b);
    }
    ringAttachment = std::make_unique<ButtonAttachment> (p.apvts, ParamID::ring, ringButton);
    syncAttachment = std::make_unique<ButtonAttachment> (p.apvts, ParamID::sync, syncButton);
    ringButton.setTooltip ("Ring modulation: the note is multiplied with the modulator (MOD PITCH). Works on the TRIANGLE waveform.");
    syncButton.setTooltip ("Hard sync: the modulator restarts the oscillator; MOD PITCH then shapes the tone.");
    setupKnob (modPitch, "PITCH", Section::Mod, ParamID::modPitch);
    setupKnob (modFine,  "FINE",  Section::Mod, ParamID::modFine);
    setupCaption (ringHint, {}, Section::Mod);
    ringHint.setJustificationType (juce::Justification::centred);

    // vibrato
    setupKnob (vibDepth, "DEPTH", Section::Vibrato, ParamID::vibDepth);
    setupKnob (vibSpeed, "SPEED", Section::Vibrato, ParamID::vibSpeed);
    setupKnob (vibDelay, "DELAY", Section::Vibrato, ParamID::vibDelay);

    // glide
    addAndMakeVisible (playMode);
    addAndMakeVisible (glideMode);
    setupKnob (glideTime, "TIME", Section::Glide, ParamID::glideTime);

    // arpeggio
    addAndMakeVisible (arpMode);
    setSection (arpPattern, Section::Arp);
    for (int i = 0; i < numArpPatterns; ++i)
        arpPattern.addItem (arpPatterns[i].name, i + 1);
    addAndMakeVisible (arpPattern);
    arpPatternAttachment = std::make_unique<ComboAttachment> (p.apvts, ParamID::arpPattern, arpPattern);
    setupKnob (arpSpeed, "SPEED", Section::Arp, ParamID::arpSpeed);
    arpSpeed.slider.setTooltip ("Frames per step. 1 frame (1/50 s on PAL) is the classic C64 chord buzz.");

    // filter
    addAndMakeVisible (filterMode);
    setSection (filterOn, Section::Filter);
    filterOn.setClickingTogglesState (true);
    addAndMakeVisible (filterOn);
    filterOnAttachment = std::make_unique<ButtonAttachment> (p.apvts, ParamID::filterOn, filterOn);
    setupKnob (cutoff,    "CUTOFF", Section::Filter, ParamID::cutoff);
    setupKnob (resonance, "RESO",   Section::Filter, ParamID::resonance);

    // output
    setupKnob (volume,     "SID VOL", Section::Out, ParamID::volume);
    setupKnob (outputGain, "GAIN",    Section::Out, ParamID::outputGain);
    setSection (meter, Section::Out);
    addAndMakeVisible (meter);

    // drums
    addAndMakeVisible (drumPads);
    setSection (drumView, Section::Drums);
    addAndMakeVisible (drumView);
    setupCaption (drumName, {}, Section::Drums);
    drumName.getProperties().set ("fontSize", 16.0f);
    setupCaption (drumWaveCaption, "WAVE", Section::Drums);
    setSection (drumWave, Section::Drums);
    drumWave.addItemList (upper (drumWaveNames), 1);
    addAndMakeVisible (drumWave);
    setupKnob (drumTune,  "TUNE",  Section::Drums);
    setupKnob (drumDecay, "DECAY", Section::Drums);
    setupKnob (drumSweep, "SWEEP", Section::Drums);
    setupKnob (drumNoise, "NOISE", Section::Drums);
    selectDrum (processor.selectedDrum.load());

    // keyboard
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyWidth (19.0f);
    keyboard.setScrollButtonsVisible (false);
    addAndMakeVisible (keyboard);
    skin->styleKeyboard (keyboard);

    // setup page
    setupPage.onSkinChosen = [this] (int i) { settings->setSkin (i); };
    setupPage.onClose      = [this] { showSetup (false); };
    setupPage.onTimingChosen = [this] (int t) { settings->setTiming (t); };
    setupPage.setCurrentSkin (skinIndex);
    setupPage.setCurrentTiming (settings->getTiming());
    addChildComponent (setupPage);

    // patches
    addAndMakeVisible (patchBar);
    patchBar.onSaveAs = [this] (PatchType t) { saveDialog.show (t); };
    addChildComponent (saveDialog);
    saveDialog.onSaved = [this] { patchBar.refresh(); };

    setSize (Layout::width, Layout::height);
    updateVisibility();
    timerCallback();   // fill the header status right away
    startTimerHz (30);
}

Zadabar64Editor::~Zadabar64Editor()
{
    settings->removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void Zadabar64Editor::setupKnob (Knob& k, const juce::String& name, Section section, const juce::String& paramID)
{
    setSection (k.slider, section);
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 92, 14);
    k.slider.setMouseDragSensitivity (180);
    addAndMakeVisible (k.slider);

    setupCaption (k.label, name, section);
    k.label.setJustificationType (juce::Justification::centred);

    if (paramID.isNotEmpty())
        k.attachment = std::make_unique<SliderAttachment> (processor.apvts, paramID, k.slider);
}

void Zadabar64Editor::setupCaption (juce::Label& l, const juce::String& text, Section section)
{
    setSection (l, section);
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centredLeft);
    l.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (l);
}

void Zadabar64Editor::selectDrum (int index)
{
    index = juce::jlimit (0, numDrums - 1, index);
    processor.selectedDrum = index;
    drumName.setText (drumNames[index], juce::dontSendNotification);

    // Rebind the drum editor's controls to the chosen drum's parameters.
    drumTune.attachment.reset();
    drumDecay.attachment.reset();
    drumSweep.attachment.reset();
    drumNoise.attachment.reset();
    drumWaveAttachment.reset();

    auto& st = processor.apvts;
    drumTune.attachment  = std::make_unique<SliderAttachment> (st, drumParam (index, "tune"),  drumTune.slider);
    drumDecay.attachment = std::make_unique<SliderAttachment> (st, drumParam (index, "decay"), drumDecay.slider);
    drumSweep.attachment = std::make_unique<SliderAttachment> (st, drumParam (index, "sweep"), drumSweep.slider);
    drumNoise.attachment = std::make_unique<SliderAttachment> (st, drumParam (index, "noise"), drumNoise.slider);
    drumWaveAttachment   = std::make_unique<ComboAttachment>  (st, drumParam (index, "wave"),  drumWave);
    drumPads.repaint();
}

void Zadabar64Editor::applySkin (int index)
{
    auto newSkin = Skin::create (index);
    setLookAndFeel (newSkin.get());
    std::swap (skin, newSkin);     // old skin is destroyed only after nothing uses it
    skinIndex = index;
    skin->styleKeyboard (keyboard);
    setupPage.setCurrentSkin (index);
    resized();
    repaint();
}

void Zadabar64Editor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    setupPage.setCurrentTiming (settings->getTiming());
    if (settings->getSkin() != skinIndex)
        applySkin (settings->getSkin());
}

void Zadabar64Editor::showSetup (bool shouldShow)
{
    setupOpen = shouldShow;
    setupButton.setToggleState (shouldShow, juce::dontSendNotification);
    updateVisibility();
    repaint();
}

void Zadabar64Editor::updateVisibility()
{
    const bool drums = Params::asInt (*processor.apvts.getRawParameterValue (ParamID::mode)) == 1;
    shownMode = drums ? 1 : 0;
    const bool panels = ! setupOpen;

    for (auto* c : std::initializer_list<juce::Component*> { &modelCaption, &clockCaption, &chipModel, &video, &analogNoise.slider, &analogNoise.label,
                                                             &filterMode, &filterOn, &cutoff.slider, &cutoff.label,
                                                             &resonance.slider, &resonance.label, &volume.slider, &volume.label,
                                                             &outputGain.slider, &outputGain.label, &meter })
        c->setVisible (panels);

    for (auto* c : std::initializer_list<juce::Component*> { &waveCaption, &waveform, &pulseWidth.slider, &pulseWidth.label,
                                                             &pwmSpeed.slider, &pwmSpeed.label, &envelopeView, &waveScope,
                                                             &attack.slider, &attack.label, &decay.slider, &decay.label,
                                                             &sustain.slider, &sustain.label, &release.slider, &release.label,
                                                             &ringButton, &syncButton, &modPitch.slider, &modPitch.label,
                                                             &modFine.slider, &modFine.label, &ringHint,
                                                             &vibDepth.slider, &vibDepth.label, &vibSpeed.slider, &vibSpeed.label,
                                                             &vibDelay.slider, &vibDelay.label, &playMode, &glideMode,
                                                             &glideTime.slider, &glideTime.label,
                                                             &arpMode, &arpPattern, &arpSpeed.slider, &arpSpeed.label })
        c->setVisible (panels && ! drums);

    for (auto* c : std::initializer_list<juce::Component*> { &drumPads, &drumView, &drumName, &drumWaveCaption, &drumWave,
                                                             &drumTune.slider, &drumTune.label, &drumDecay.slider, &drumDecay.label,
                                                             &drumSweep.slider, &drumSweep.label, &drumNoise.slider, &drumNoise.label })
        c->setVisible (panels && drums);

    setupPage.setVisible (setupOpen);
    headerState.drumMode = drums;
    resized();
}

void Zadabar64Editor::timerCallback()
{
    ++ticks;
    if (ticks % 6 == 0)
        patchBar.refresh();
    if (Params::asInt (*processor.apvts.getRawParameterValue (ParamID::mode)) != shownMode)
    {
        updateVisibility();
        repaint();
    }

    const auto& disp = processor.getDisplay();
    for (size_t v = 0; v < 3; ++v)
        headerState.voiceActive[v] = disp.voices[v].gate.load() || disp.voices[v].drum.load() >= 0;
    headerState.cursorOn  = (ticks / 15) % 2 == 0;
    headerState.scrollPos += 2.0f;
    // Ring modulation only affects the triangle waveform on a real SID: say so.
    const bool ringOn = processor.apvts.getRawParameterValue (ParamID::ring)->load() > 0.5f;
    const int  wave   = Params::asInt (*processor.apvts.getRawParameterValue (ParamID::waveform));
    const bool hasTri = (waveformBits[juce::jlimit (0, 7, wave)] & Ctrl::triangle) != 0;
    ringHint.setText (ringOn && ! hasTri ? "RING NEEDS\nTRIANGLE WAVE" : "", juce::dontSendNotification);

    // Knobs that can't change the sound right now are dimmed and locked.
    const bool hasPulse = (waveformBits[juce::jlimit (0, 7, wave)] & Ctrl::pulse) != 0;
    const bool modUsed  = ringOn || processor.apvts.getRawParameterValue (ParamID::sync)->load() > 0.5f;
    const int  arp      = Params::asInt (*processor.apvts.getRawParameterValue (ParamID::arpMode));
    const bool arpChord = arp == static_cast<int> (ArpMode::Chord);
    for (auto [k, on] : { std::pair<Knob*, bool> { &pulseWidth, hasPulse }, { &pwmSpeed, hasPulse },
                          { &modPitch, modUsed }, { &modFine, modUsed },
                          { &arpSpeed, arp != 0 }, { &glideTime, ! arpChord } })
        if (k->slider.isEnabled() != on)
        {
            k->slider.setEnabled (on);
            k->slider.setAlpha (on ? 1.0f : 0.3f);
            k->label.setAlpha (on ? 1.0f : 0.3f);
        }
    // CHORD arpeggio is always mono and never glides; the pattern list is for FIXED only
    for (auto [c, on] : { std::pair<juce::Component*, bool> { &arpPattern, arp == static_cast<int> (ArpMode::Fixed) },
                          { &playMode, ! arpChord }, { &glideMode, ! arpChord } })
        if (c->isEnabled() != on)
        {
            c->setEnabled (on);
            c->setAlpha (on ? 1.0f : 0.3f);
        }

    const bool is8580 = Params::asInt (*processor.apvts.getRawParameterValue (ParamID::chipModel)) == 1;
    const bool isNtsc = Params::asInt (*processor.apvts.getRawParameterValue (ParamID::video)) == 1;
    headerState.chipText = juce::String (is8580 ? "8580" : "6581") + (isNtsc ? " NTSC" : " PAL");

    // Repaint the header and strip only when something on them changed (voice lights,
    // cursor, chip), except a strip that animates by itself (the Demoscene scroller).
    const auto signature = juce::String (headerState.voiceActive[0] ? "1" : "0") + (headerState.voiceActive[1] ? "1" : "0")
                         + (headerState.voiceActive[2] ? "1" : "0") + (headerState.cursorOn ? "C" : "c")
                         + (headerState.drumMode ? "D" : "I") + headerState.chipText;
    if (signature != headerSignature)
    {
        headerSignature = signature;
        repaint (headerArea);
        repaint (stripTextArea);
    }
    else if (skin->animatesStatusStrip())
    {
        repaint (stripTextArea);
    }
}

juce::Rectangle<int> Zadabar64Editor::contentOf (juce::Rectangle<int> panel) const
{
    return panel.reduced (10, 8).withTrimmedTop (skin->panelTitleHeight());
}

void Zadabar64Editor::layoutKnobRow (juce::Rectangle<int> r, std::initializer_list<Knob*> knobs)
{
    const int w = r.getWidth() / static_cast<int> (knobs.size());
    for (auto* k : knobs)
    {
        auto cell = r.removeFromLeft (w);
        k->label.setBounds (cell.removeFromTop (12));
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, cell.getWidth(), 14);
        k->slider.setBounds (cell);
    }
}

void Zadabar64Editor::resized()
{
    auto area = getLocalBounds().reduced (Layout::margin + 6, Layout::margin);

    headerArea = area.removeFromTop (Layout::header);
    {
        auto h = headerArea;
        logoArea = h.removeFromLeft (390);
        auto setupB = h.removeFromRight (96);
        setupButton.setBounds (setupB.withSizeKeepingCentre (96, 30));
        h.removeFromRight (12);
        auto modeB = h.removeFromRight (250);
        modeSwitch.setBounds (modeB.withSizeKeepingCentre (250, 30));
        h.removeFromRight (12);
        h.removeFromLeft (12);
        infoArea = h;
    }
    backgroundDirty = true;   // size, skin, mode or setup changed: redraw the static layer
    stripArea = area.removeFromTop (Layout::strip);
    stripTextArea = stripArea;
    patchBar.setBounds (stripTextArea.removeFromRight (440).reduced (0, 1));
    stripTextArea.removeFromRight (12);
    saveDialog.setBounds (getLocalBounds());
    area.removeFromTop (6);

    keyboard.setBounds (area.removeFromBottom (Layout::keyboard).reduced (0, 4));
    area.removeFromBottom (6);

    setupPage.setBounds (area);

    // Columns: CHIP and OUT run the full height. In between, instrument mode has
    // two rows (OSC / ENVELOPE / FILTER, then RING-SYNC / VIBRATO / GLIDE);
    // drum mode has DRUM KIT and FILTER at full height.
    const bool drums = shownMode == 1;
    const float unit = static_cast<float> (area.getWidth() - 4 * Layout::gap) / (2.0f + 10.0f + 2.2f);
    chipPanel = area.removeFromLeft (juce::roundToInt (2.0f * unit));
    area.removeFromLeft (Layout::gap);
    outPanel = area.removeFromRight (juce::roundToInt (2.2f * unit));
    area.removeFromRight (Layout::gap);

    auto middle = area;
    auto take = [&] (juce::Rectangle<int>& row, float u)
    {
        auto r = row.removeFromLeft (juce::roundToInt (u * unit));
        row.removeFromLeft (Layout::gap);
        return r;
    };

    if (drums)
    {
        drumPanel   = take (middle, 7.0f);
        filterPanel = middle;
        oscPanel = envPanel = modPanel = vibPanel = arpPanel = glidePanel = {};
    }
    else
    {
        auto rowA = middle.removeFromTop (middle.getHeight() * 57 / 100);
        middle.removeFromTop (Layout::gap);
        auto rowB = middle;

        oscPanel    = take (rowA, 2.4f);
        envPanel    = take (rowA, 4.6f);
        filterPanel = rowA;

        // second row: RING/SYNC under OSC, then VIBRATO, ARP and GLIDE
        modPanel   = rowB.removeFromLeft (oscPanel.getWidth());
        rowB.removeFromLeft (Layout::gap);
        vibPanel   = take (rowB, 3.3f);
        arpPanel   = take (rowB, 2.5f);
        glidePanel = rowB;
        drumPanel  = {};
    }

    // --- chip
    {
        auto c = contentOf (chipPanel);
        modelCaption.setBounds (c.removeFromTop (14));
        chipModel.setBounds (c.removeFromTop (28));
        c.removeFromTop (12);
        clockCaption.setBounds (c.removeFromTop (14));
        video.setBounds (c.removeFromTop (28));
        layoutKnobRow (c.removeFromBottom (Layout::knobCell), { &analogNoise });
    }

    // --- oscillator + envelope
    if (! drums)
    {
        auto c = contentOf (oscPanel);
        waveCaption.setBounds (c.removeFromTop (14));
        waveform.setBounds (c.removeFromTop (28));
        layoutKnobRow (c.removeFromBottom (Layout::knobCell), { &pulseWidth, &pwmSpeed });
        c.removeFromTop (8);
        c.removeFromBottom (6);
        waveScope.setBounds (c);

        auto e = contentOf (envPanel);
        layoutKnobRow (e.removeFromBottom (Layout::knobCell), { &attack, &decay, &sustain, &release });
        e.removeFromBottom (8);
        envelopeView.setBounds (e);
    }
    else
    {
        auto c = contentOf (drumPanel);
        drumPads.setBounds (c.removeFromTop (102));
        c.removeFromTop (12);
        auto top = c.removeFromTop (30);
        drumName.setBounds (top.removeFromLeft (top.getWidth() / 2));
        drumWaveCaption.setBounds (top.removeFromLeft (50));
        drumWave.setBounds (top.reduced (0, 1));
        layoutKnobRow (c.removeFromBottom (Layout::knobCell), { &drumTune, &drumDecay, &drumSweep, &drumNoise });
        c.removeFromBottom (8);
        c.removeFromTop (8);
        drumView.setBounds (c);
    }

    // --- ring mod / vibrato / glide
    if (! drums)
    {
        auto m = contentOf (modPanel);
        auto buttons = m.removeFromTop (28);
        ringButton.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2 - 3));
        buttons.removeFromLeft (6);
        syncButton.setBounds (buttons);
        m.removeFromTop (4);
        ringHint.setBounds (m.removeFromTop (24));
        layoutKnobRow (m.removeFromBottom (Layout::knobCell), { &modPitch, &modFine });

        layoutKnobRow (contentOf (vibPanel).removeFromBottom (Layout::knobCell), { &vibDepth, &vibSpeed, &vibDelay });

        auto a = contentOf (arpPanel);
        arpMode.setBounds (a.removeFromTop (28));
        a.removeFromTop (6);
        arpPattern.setBounds (a.removeFromTop (28));
        layoutKnobRow (a.removeFromBottom (Layout::knobCell), { &arpSpeed });

        auto g = contentOf (glidePanel);
        playMode.setBounds (g.removeFromTop (28));
        g.removeFromTop (6);
        glideMode.setBounds (g.removeFromTop (28));
        layoutKnobRow (g.removeFromBottom (Layout::knobCell), { &glideTime });
    }

    // --- filter
    {
        auto c = contentOf (filterPanel);
        filterMode.setBounds (c.removeFromTop (28));
        c.removeFromTop (8);
        filterOn.setBounds (c.removeFromTop (28));
        layoutKnobRow (c.removeFromBottom (Layout::knobCell), { &cutoff, &resonance });
    }

    // --- output: two knobs stacked + meter
    {
        auto c = contentOf (outPanel);
        meter.setBounds (c.removeFromRight (26));
        c.removeFromRight (6);
        auto knobs = c.removeFromBottom (juce::jmin (c.getHeight(), Layout::knobCell * 2 + 12));
        layoutKnobRow (knobs.removeFromTop (Layout::knobCell), { &volume });
        knobs.removeFromTop (12);
        layoutKnobRow (knobs, { &outputGain });
    }
}

void Zadabar64Editor::paint (juce::Graphics& g)
{
    // The background and the panel frames are static: draw them into an image once, at the
    // screen's pixel density, and only paint the moving header and strip on top.
    const float scale = juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (backgroundDirty || std::abs (scale - backgroundScale) > 0.001f || ! backgroundCache.isValid())
    {
        backgroundScale = scale;
        backgroundDirty = false;
        backgroundCache = juce::Image (juce::Image::RGB, juce::roundToInt ((float) getWidth() * scale),
                                       juce::roundToInt ((float) getHeight() * scale), false);
        juce::Graphics bg (backgroundCache);
        bg.addTransform (juce::AffineTransform::scale (scale));
        paintStatic (bg);
    }
    g.drawImage (backgroundCache, getLocalBounds().toFloat());

    skin->paintHeader (g, headerArea, logoArea, infoArea, headerState);
    skin->paintStatusStrip (g, stripTextArea, headerState);
}

void Zadabar64Editor::paintStatic (juce::Graphics& g)
{
    skin->paintBackground (g, getLocalBounds());
    if (setupOpen)
        return;

    skin->paintPanel (g, chipPanel, "CHIP", Section::Chip);
    if (shownMode == 1)
        skin->paintPanel (g, drumPanel, "DRUM KIT", Section::Drums);
    else
    {
        skin->paintPanel (g, oscPanel, "OSC", Section::Osc);
        skin->paintPanel (g, envPanel, "ENVELOPE", Section::Env);
        skin->paintPanel (g, modPanel, "RING/SYNC", Section::Mod);
        skin->paintPanel (g, vibPanel, "VIBRATO", Section::Vibrato);
        skin->paintPanel (g, arpPanel, "ARP", Section::Arp);
        skin->paintPanel (g, glidePanel, "GLIDE", Section::Glide);
    }
    skin->paintPanel (g, filterPanel, "FILTER", Section::Filter);
    skin->paintPanel (g, outPanel, "OUT", Section::Out);
}

} // namespace zadabar
