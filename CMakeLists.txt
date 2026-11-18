cmake_minimum_required(VERSION 3.22)

project(Zadabar64 VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static MSVC runtime: the plugin runs without the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

# --- JUCE (AGPLv3 open-source licence) ---------------------------------
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        9.0.5
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(JUCE)

# --- reSIDfp SID emulation (GPL-2.0-or-later) ---------------------------
include(cmake/libresidfp.cmake)

# --- The plugin ---------------------------------------------------------
juce_add_plugin(Zadabar64
    COMPANY_NAME                "Zadabar"
    PRODUCT_NAME                "Zadabar64"
    VERSION                     ${PROJECT_VERSION}
    PLUGIN_MANUFACTURER_CODE    Zdbr
    PLUGIN_CODE                 Zd64
    BUNDLE_ID                   com.zadabar.zadabar64
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Instrument Synth
    ICON_BIG                    resources/images/zadabar64_icon_1024.png
    ICON_SMALL                  resources/images/zadabar64_icon_256.png
    FORMATS                     VST3 Standalone
)

target_sources(Zadabar64 PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/Settings.cpp
    src/sid/SidChip.cpp
    src/player/Player.cpp
    src/ui/Skin.cpp
    src/ui/SkinReady.cpp
    src/ui/SkinBreadbin.cpp
    src/ui/SkinDemoscene.cpp
    src/ui/Widgets.cpp
    src/ui/SetupPage.cpp
    src/ui/PatchBar.cpp
    src/patches/Patches.cpp
    src/patches/FactoryPatches.cpp
)

# Fonts and images compiled into the plugin.
# The font is "Press Start 2P" (SIL Open Font License 1.1). See resources/fonts/.
juce_add_binary_data(Zadabar64Assets SOURCES
    resources/fonts/PressStart2P-Regular.ttf
    resources/images/zadabar_z.svg
)
set_target_properties(Zadabar64Assets PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(Zadabar64 PRIVATE src)

target_compile_definitions(Zadabar64 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    # Zadabar64 reads no audio files and no WebP images: leave those codecs out
    # (smaller binaries, fewer third-party licences to ship)
    JUCE_USE_FLAC=0
    JUCE_USE_OGGVORBIS=0
    JUCE_USE_OPUS=0
    JUCE_USE_MP3AUDIOFORMAT=0
    JUCE_USE_WINDOWS_MEDIA_FORMAT=0
    JUCE_USE_WEBP=0
)

# --- CLAP (via free-audio/clap-juce-extensions, MIT) ----------------------
# JUCE 9 cannot build CLAP plug-ins yet; this community wrapper adds the format.
option(ZADABAR_CLAP "Also build the CLAP plug-in" ON)
if(ZADABAR_CLAP)
    FetchContent_Declare(
        clap_juce_extensions
        GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
        # pinned: the commit Zadabar64 was tested with (CLAP 1.2.10), passes clap-validator
        GIT_TAG        3837f4a6ed2ab0ba46aba69ae5b7041c7c64ce74
        GIT_SUBMODULES_RECURSE ON
    )
    FetchContent_MakeAvailable(clap_juce_extensions)
    clap_juce_extensions_plugin(TARGET Zadabar64
        CLAP_ID       "com.zadabar.zadabar64"
        CLAP_FEATURES instrument synthesizer drum-machine stereo)
endif()

target_link_libraries(Zadabar64
    PRIVATE
        residfp
        Zadabar64Assets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
