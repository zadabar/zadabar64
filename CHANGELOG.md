# Changelog

All notable changes to Zadabar64. Versions follow `project(Zadabar64 VERSION …)` in `CMakeLists.txt`.

## 0.4.0 – play it live

### Mod wheel and pitch bend
- New **WHEEL** panel: the mod wheel (CC 1) moves vibrato, cutoff, resonance, pulse width or
  MOD PITCH (a hard-sync sweep), with an amount from −100 to +100 %. Wheel vibrato starts right
  away, without the vibrato delay. The knobs themselves don't move.
- New **BEND** panel: separate up and down ranges (0–24 semitones, default 2), on the note's
  pitch or only on the ring/sync modulator (a sync sweep on the bender).
- Both are part of instrument patches. SYNC SCREAMER and SYNC BASS sweep the sync, ACID SID BASS
  and PULSE BASS open the filter, THIN PULSE LEAD fattens the pulse. Older patches load the
  defaults (wheel → vibrato +50 %, bend ±2).
- Reset All Controllers (CC 121) recentres the wheel and the bender. Drum mode ignores both.

### MIDI learn
- Right-click any knob, switch, list or drum level fader → **MIDI LEARN**, then move a control on
  your MIDI controller. **FORGET** removes it. Hovering a control shows its CC.
- Any MIDI channel, CC 0–119 (120–127 are channel commands). One CC per parameter; learning a CC
  that is already used moves it. A learned CC 1 replaces the mod wheel routing.
- The map is saved with the project. **SETUP → MIDI** lists it, with FORGET ALL and SET AS
  DEFAULT (the map new instances start with).

### Other
- A right-click no longer toggles buttons or moves knobs.
- Release ZIP: paths inside the archive now use "/", so it unpacks correctly with 7-Zip, macOS
  and Linux tools too.
- The version shown in the plugin comes from the build.

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
