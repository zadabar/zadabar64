# Changelog

All notable changes to Zadabar64. Versions follow `project(Zadabar64 VERSION …)` in `CMakeLists.txt`.

## 0.3.0 – first public release

### Drum machine
- Six SID drums (kick, snare, closed hat, open hat, low tom, high tom) on General MIDI notes,
  each on its own SID chip
- Per drum: tune, decay, pitch sweep, noise frames, waveform (incl. the noise/pulse "Hubbard"
  snare) and a level fader
- Hits rotate over the three voices of their chip and are armed ahead of time: no late or
  swallowed hits (the SID's ADSR bug), no latency
- 8 factory kits

### Instrument
- All SID waveforms incl. the combined ones, pulse width + PWM, the SID's own ADSR,
  LP/BP/HP/notch filter with 11-bit cutoff
- Ring modulation and hard sync with MOD PITCH / FINE, working on every note of a chord
- Vibrato (depth, speed, delay), glide (always / legato), poly (3 notes) or mono
- Arpeggio: CHORD (held keys on one voice) or FIXED (11 patterns), 1–4 frames per step,
  frame-exact on PAL and NTSC
- 21 factory patches

### Everywhere
- Cycle-accurate reSIDfp emulation, MOS 6581 or CSG 8580, PAL or NTSC clock
- Three skins: READY., Breadbin, Demoscene
- Oscilloscope drawn by a hidden SID chip, live envelope view, level meter, drum program view
- Patch browser: save, load, share user patches (`Documents\Zadabar64`)
- Live / Tight timing (SETUP page), optional 6581 analog noise
- Formats: VST3, CLAP, standalone (Windows 10/11 x64)
