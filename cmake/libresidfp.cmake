# Builds libresidfp (the reSIDfp SID chip emulation, GPL-2.0-or-later) as a
# static library target called `residfp`.
#
# libresidfp normally uses autotools, which generate two small headers.
# We generate the same two headers here with CMake, so the library builds
# with MSVC on Windows as well as with GCC/Clang.

include(FetchContent)

set(ZADABAR_RESIDFP_VERSION "1.3.0")

FetchContent_Declare(
    libresidfp
    GIT_REPOSITORY https://github.com/libsidplayfp/libresidfp.git
    GIT_TAG        v${ZADABAR_RESIDFP_VERSION}
    GIT_SHALLOW    ON
)
# libresidfp has no CMakeLists.txt, so this only downloads the sources.
FetchContent_MakeAvailable(libresidfp)

set(RESIDFP_SRC "${libresidfp_SOURCE_DIR}/src")
set(RESIDFP_GEN "${CMAKE_CURRENT_BINARY_DIR}/residfp_generated")

# --- generated headers (values autotools would normally fill in)
string(REPLACE "." ";" _rv "${ZADABAR_RESIDFP_VERSION}")
list(GET _rv 0 LIB_MAJOR)
list(GET _rv 1 LIB_MINOR)
list(GET _rv 2 LIB_LEVEL)

set(PACKAGE_VERSION "${ZADABAR_RESIDFP_VERSION}")
set(RESIDFP_BRANCH_HINTS 1)
set(RESIDFP_INLINING 1)
set(RESIDFP_INLINE inline)
if(MSVC)
    set(HAVE_BUILTIN_EXPECT 0)   # MSVC has no __builtin_expect
else()
    set(HAVE_BUILTIN_EXPECT 1)
endif()

configure_file("${RESIDFP_SRC}/siddefs-fp.h.in"         "${RESIDFP_GEN}/siddefs-fp.h"         @ONLY)
configure_file("${RESIDFP_SRC}/residfp/sidversion.h.in" "${RESIDFP_GEN}/residfp/sidversion.h" @ONLY)

add_library(residfp STATIC
    ${RESIDFP_SRC}/Dac.cpp
    ${RESIDFP_SRC}/EnvelopeGenerator.cpp
    ${RESIDFP_SRC}/ExternalFilter.cpp
    ${RESIDFP_SRC}/Filter.cpp
    ${RESIDFP_SRC}/Filter6581.cpp
    ${RESIDFP_SRC}/Filter8580.cpp
    ${RESIDFP_SRC}/FilterModelConfig.cpp
    ${RESIDFP_SRC}/FilterModelConfig6581.cpp
    ${RESIDFP_SRC}/FilterModelConfig8580.cpp
    ${RESIDFP_SRC}/Integrator6581.cpp
    ${RESIDFP_SRC}/Integrator8580.cpp
    ${RESIDFP_SRC}/OpAmp.cpp
    ${RESIDFP_SRC}/SID.cpp
    ${RESIDFP_SRC}/Spline.cpp
    ${RESIDFP_SRC}/State.cpp
    ${RESIDFP_SRC}/WaveformCalculator.cpp
    ${RESIDFP_SRC}/WaveformGenerator.cpp
    ${RESIDFP_SRC}/resample/SincResampler.cpp
    ${RESIDFP_SRC}/version.cc
    ${RESIDFP_SRC}/residfp/residfp.cpp
)

target_include_directories(residfp
    PUBLIC  "${RESIDFP_SRC}" "${RESIDFP_GEN}"
)

target_compile_features(residfp PUBLIC cxx_std_17)
set_target_properties(residfp PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    target_compile_definitions(residfp PRIVATE WIN32_LEAN_AND_MEAN _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS)
    target_compile_options(residfp PRIVATE /W0 /O2)
    # siddefs-fp.h checks __cplusplus >= 201103L. MSVC reports 199711L unless
    # told otherwise. PUBLIC because our own sources include that header too.
    target_compile_options(residfp PUBLIC /Zc:__cplusplus)
else()
    target_compile_options(residfp PRIVATE -w -O3)
    find_package(Threads REQUIRED)
    target_link_libraries(residfp PUBLIC Threads::Threads)
endif()
