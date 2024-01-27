cmake_minimum_required(VERSION 3.10)
project(alsa-jamesdsp VERSION 0.1
        DESCRIPTION "ALSA JamesDSPFX Plugin"
        LANGUAGES C CXX)

include_directories(./src)

set(JDSP_SRC
    src/pcm_jdspfx.cpp
    src/jdspfx.h
    src/EffectDSPMain.h
    src/Effect.h
    src/EffectDSPMain.cpp
    src/Effect.cpp
    src/kissfft/kiss_fft.c
    src/kissfft/kiss_fftr.c
    src/dspinterface.h
    src/JLimiter.c
    src/reverb.c
    src/compressor.c
    src/AutoConvolver.c
    src/mnspline.c
    src/ArbFIRGen.c
    src/vdc.c
    src/bs2b.c
    src/iirfilters.c
    src/valve/12ax7amp/Tube.c
    src/valve/12ax7amp/wdfcircuits_triode.c
)

find_package(PkgConfig REQUIRED)

pkg_check_modules(ALSA REQUIRED
        alsa)
pkg_check_modules(SNDFILE REQUIRED
        samplerate
        sndfile)

set(CMAKE_SHARED_LINKER_FLAGS "-rdynamic -ldl")
add_definitions("-O3 -fPIC -DPIC -Wall -Wextra -funroll-loops -ffast-math")

add_library(alsa-jamesdsp SHARED ${JDSP_SRC})

target_include_directories(alsa-jamesdsp PUBLIC ${ALSA_INCLUDE_DIRS} ${SNDFILE_INCLUDE_DIRS})
target_compile_options(alsa-jamesdsp PUBLIC ${ALSA_CFLAGS} ${SNDFILE_CFLAGS})
target_link_libraries(alsa-jamesdsp ${ALSA_LIBRARIES} ${SNDFILE_LIBRARIES})

set_target_properties(alsa-jamesdsp PROPERTIES OUTPUT_NAME "asound_module_pcm_jdspfx")
