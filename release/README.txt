
      _____   ___    ____  ___    ____  ___    ____  _____ __ __
     /__  /  /   |  / __ \/   |  / __ )/   |  / __ \/ ___// // /
       / /  / /| | / / / / /| | / __  / /| | / /_/ / __ \/ // /_
      / /__/ ___ |/ /_/ / ___ |/ /_/ / ___ |/ _, _/ /_/ /__  __/
     /____/_/  |_/_____/_/  |_/_____/_/  |_/_/ |_|\____/  /_/

             A SID SYNTH + THE FIRST FREE, OPEN-SOURCE
                    SID DRUM MACHINE PLUGIN

                      version {VERSION}  -  by Zadabar


                       ||||||||||||||||||||||||
                     .-''''''''''''''''''''''''-.
                     |o                         |
                     |    MOS 6581  /  8580     |
                     |    SOUND INTERFACE       |
                     |    DEVICE  x6            |
                     |                          |
                     '-........................-'
                       ||||||||||||||||||||||||


  ======================================================================
   WHAT IS IT?
  ======================================================================

     Zadabar64 is a free VST3 / CLAP instrument and standalone app that
     plays through a cycle-accurate emulation of the Commodore 64's SID
     chip. Not samples, not an imitation: the real chip, simulated one
     clock cycle at a time (reSIDfp, the engine inside sidplayfp).

     Two instruments in one plugin, switched in the header:

       INSTRUMENT   a SID synthesizer with the tricks C64 musicians
                    used: PWM, ring mod, hard sync, vibrato, glide and
                    the 50 Hz arpeggio chord.

       DRUMS        six SID drums, each on its own SID chip, designed
                    frame by frame the way C64 drivers did it, with a
                    level fader per drum. No hit is ever late or choked.

     Pick the MOS 6581 (1982, warm and dirty filter) or the CSG 8580
     (1987, clean and bright), PAL or NTSC, and one of three skins.


  ======================================================================
   FEATURES
  ======================================================================

   INSTRUMENT MODE                       DRUM MODE
   ---------------                       ---------
   * Triangle, saw, pulse, noise and     * Kick, snare, closed hat,
     the combined waveforms                open hat, low tom, high tom
   * Pulse width + PWM sweep             * Each drum on its own SID chip
   * Native SID ADSR, real timings       * Tune, decay, sweep, noise
   * LP / BP / HP / notch filter,          frames, waveform, level
     11-bit cutoff, 4-bit resonance      * "Hubbard" noise/pulse snare
   * Ring mod + hard sync with MOD       * General MIDI drum notes,
     PITCH and FINE                        clickable pads
   * Vibrato: depth, speed, delay        * Zero latency, no stolen hits
   * Glide: always or legato only
   * Poly (3 notes) or mono legato       EVERYWHERE
   * Arpeggio: CHORD (hold a chord)      ----------
     or FIXED patterns, 1-4 frames       * 6581 or 8580, PAL or NTSC
     per step, frame-exact               * Oscilloscope drawn by a real
                                           (emulated) SID
                                         * Live envelope view, level meter
                                         * 3 skins: READY. / Breadbin /
                                           Demoscene
                                         * 21 factory patches, 8 kits,
                                           save your own


  ======================================================================
   QUICK START
  ======================================================================

     1.  Install it: see INSTALL.txt.
     2.  Load Zadabar64 on an instrument track and play.
     3.  Click the patch name (top right) to browse the factory patches.
         Try HUBBARD LEAD, SYNC SCREAMER, CHORD BUZZ (hold a chord!)
         and ACID SID BASS.
     4.  Switch the header to DRUMS for the drum machine. Click the pads,
         pick a kit (HUBBARD KIT, ELECTRO 64 ...), shape each drum.
     5.  SAVE your own sounds. SETUP (top right) changes the skin.


  ======================================================================
   GOOD TO KNOW
  ======================================================================

     * RING mod only works on the TRIANGLE wave, like on the real chip.
       MOD PITCH and FINE only matter with RING or SYNC on.
     * Knobs that can't change the sound right now are dimmed.
     * CHORD arpeggio: hold the keys, one SID voice plays them all.
       SPEED 1 = one note per frame, the classic 50 Hz buzz.
     * Without ring/sync your notes share one SID, like on a C64.
       With ring/sync each note gets its own chip.
     * SETUP -> TIMING: LIVE (no latency, authentic) or TIGHT (a hard
       restart before every note, 40 ms latency your DAW compensates).
     * ANALOG NOISE brings back the 6581's hiss, if you miss it.


  ======================================================================
   FACTORY SOUNDS
  ======================================================================

     PATCHES   Init, Hubbard Lead, Sync Screamer, Galway Flute,
               8580 Brass, Thin Pulse Lead, Acid SID Bass, Octave Bass,
               Pulse Bass, Sync Bass, Chord Buzz, Slow Chord Arp,
               Minor Arp Keys, Dom7 Stab, 2 Octave Sparkle, Ring Bells,
               Metal Gong, SID Pad, Saw+Tri Organ, Noise Wind, Siren

     KITS      Init Kit, Classic 64, Hubbard Kit, Galway Kit,
               8580 Tight, Boomy 6581, Electro 64, Lofi Saw Kit


  ======================================================================
   FREE SOFTWARE
  ======================================================================

     Zadabar64 is free software under the GNU General Public License,
     version 3 or later (LICENSE.txt). Use it for anything, including
     commercial music: the music you make is yours. You may share and
     change the program under the GPL; the full source code is at

         {GITHUB}

     Zadabar64 is built with other free software. THIRD_PARTY_NOTICES.txt
     lists it, the licences are in the licenses\ folder.


  ======================================================================
   CREDITS
  ======================================================================

     Zadabar64 ........................ Zadabar of Crisis Productions
     SID emulation (reSIDfp) .......... Dag Lem, Antti Lankila,
                                        Leandro Nini
     Framework ........................ JUCE
     CLAP support ..................... free-audio clap-juce-extensions
     Font ............................. Press Start 2P (CodeMan38)
     The SID .......................... Bob Yannes, MOS Technology, 1981

     Greetings to t0m4t0 and csega of Crisis Productions, and to every
     scener who kept the SID alive.


  ======================================================================

     Website ........ {WEBSITE}
     Source code .... {GITHUB}
     Bugs, ideas .... {GITHUB}/issues

     Commodore 64 is a trademark of its respective owner. Zadabar64 is
     not affiliated with or endorsed by it. VST is a registered
     trademark of Steinberg Media Technologies GmbH.

                                READY.
                                _

