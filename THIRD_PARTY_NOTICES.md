# Third-party notices

Zadabar64 itself is licensed under the GNU General Public License v3.0 or later
(see `LICENSE`). It is built with the following third-party software, which is
downloaded at build time and not stored in this repository.

## libresidfp (reSIDfp)

- Source: https://github.com/libsidplayfp/libresidfp (version 1.3.0)
- License: GNU General Public License, version 2 or (at your option) any later version
- Copyright (c) 2000-2011 Dag Lem, (c) 2007-2010 Antti Lankila,
  (c) 2010-2026 Leandro Nini

Zadabar64 uses it under the "or any later version" option, as GPL-3.0.

## JUCE

- Source: https://github.com/juce-framework/JUCE (version 9.0.5)
- License: GNU Affero General Public License v3 (open-source option of JUCE's dual licence)
- Copyright (c) Raw Material Software Limited

## Press Start 2P font (resources/fonts)

- Source: https://github.com/google/fonts/tree/main/ofl/pressstart2p
- Copyright 2012 The Press Start 2P Project Authors (cody@zone38.net),
  with Reserved Font Name "Press Start 2P"
- License: SIL Open Font License 1.1, text in `resources/fonts/PressStart2P-OFL.txt`

## Zadabar logo (resources/images/zadabar_z.svg, zadabar64_icon_*, docs/images/zadabar64-icon.png)

- Copyright (c) Zadabar. The logo is not covered by the GPL; please don't reuse it
  for other projects.

## VST3 SDK 3.8 (bundled with JUCE)

- Copyright (c) 2025 Steinberg Media Technologies GmbH
- License: MIT
- VST is a registered trademark of Steinberg Media Technologies GmbH.

## CLAP and clap-juce-extensions

- Source: https://github.com/free-audio/clap-juce-extensions (downloaded at build time,
  with the CLAP SDK and clap-helpers as submodules)
- License: MIT. CLAP and clap-helpers: Copyright (c) 2021 Alexandre Bique;
  clap-juce-extensions: Copyright (c) 2019-2020 Paul Walker

## Libraries bundled with JUCE that end up in the binaries

zlib (Zlib), HarfBuzz (MIT), SheenBidi (Apache-2.0), LunaSVG and PlutoVG (MIT),
libpng (libpng-2.0) and the IJG JPEG library (IJG licence: *this software is based in
part on the work of the Independent JPEG Group*). JUCE's audio file codecs (FLAC, Ogg
Vorbis, Opus, MP3) and WebP are switched off in `CMakeLists.txt`, so they are not included.

## Licence texts for binary releases

`release/licenses/` holds the full licence text of every component above; the release
ZIP ships them. `release/collect_licenses.py` refreshes them from the downloaded sources
after a JUCE / libresidfp / clap-juce-extensions update.
