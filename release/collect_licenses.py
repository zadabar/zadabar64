"""Copies the licence texts of everything inside the Zadabar64 binaries into release/licenses/.

Run it after a build (the sources it reads are downloaded by CMake into build/_deps):
    python release/collect_licenses.py                 # uses build/_deps
    python release/collect_licenses.py --juce PATH ... # or point at the sources yourself

The results are committed, so making a release zip does not need this script.
Re-run it when JUCE, libresidfp or clap-juce-extensions is updated.
"""
import argparse
import pathlib
import re

here = pathlib.Path(__file__).resolve().parent
root = here.parent
deps = root / "build" / "_deps"

ap = argparse.ArgumentParser()
ap.add_argument("--juce", default=str(deps / "juce-src"))
ap.add_argument("--residfp", default=str(deps / "libresidfp-src"))
ap.add_argument("--clap", default=str(deps / "clap_juce_extensions-src"))
args = ap.parse_args()
juce = pathlib.Path(args.juce) / "modules"
residfp = pathlib.Path(args.residfp)
clap = pathlib.Path(args.clap)

def ijg_legal(readme: str) -> str:
    """The LEGAL ISSUES section of the IJG README (what its licence asks to ship)."""
    m = re.search(r"\nLEGAL ISSUES\n=+\n(.*?)\n\n\n[A-Z][A-Z ]+\n=+", readme, re.S)
    return "Independent JPEG Group's JPEG software - LEGAL ISSUES\n(from the README of the IJG library bundled with JUCE)\n\n" + (m.group(1) if m else readme)

files = {
    "reSIDfp - GPL-2.0-or-later.txt":        residfp / "COPYING",
    "JUCE - AGPL-3.0.txt":                   juce / "juce_gui_extra/native/typescript/webview-interop/LICENSE-AGPL.md",
    "VST3 SDK - MIT.txt":                    juce / "juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt",
    "CLAP - MIT.txt":                        clap / "clap-libs/clap/LICENSE",
    "clap-helpers - MIT.txt":                clap / "clap-libs/clap-helpers/LICENSE",
    "clap-juce-extensions - MIT.txt":        clap / "LICENSE.md",
    "zlib - Zlib.txt":                       juce / "juce_core/zip/zlib/LICENSE",
    "HarfBuzz - MIT.txt":                    juce / "juce_graphics/fonts/harfbuzz/COPYING",
    "SheenBidi - Apache-2.0.txt":            juce / "juce_graphics/unicode/sheenbidi/LICENSE",
    "LunaSVG - MIT.txt":                     juce / "juce_graphics/drawables/lunasvg/LICENSE",
    "PlutoVG - MIT.txt":                     juce / "juce_graphics/drawables/lunasvg/plutovg/LICENSE",
    "libpng - libpng-2.0.txt":               juce / "juce_graphics/image_formats/pnglib/LICENSE",
    "IJG libjpeg - IJG.txt":                 juce / "juce_graphics/image_formats/jpglib/README",
    "Press Start 2P font - OFL-1.1.txt":     root / "resources/fonts/PressStart2P-OFL.txt",
}

out = here / "licenses"
out.mkdir(exist_ok=True)
for name, src in files.items():
    text = src.read_text(encoding="utf-8", errors="replace")
    if name.startswith("IJG"):
        text = ijg_legal(text)
    # Windows line endings, so Notepad shows them right
    (out / name).write_bytes(text.replace("\r\n", "\n").replace("\n", "\r\n").encode("utf-8"))
    print("  ", name)
print(f"{len(files)} licence files in {out}")
