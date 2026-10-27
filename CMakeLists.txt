cmake_minimum_required(VERSION 3.20)
project(FancyReverb VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # the static libraries end up inside the plugin shared object
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
option(FR_NATIVE "Compile with -march=native (local benchmarks only)" OFF)

add_library(pffft STATIC third_party/pffft/pffft.c third_party/pffft/pffft_common.c)
target_include_directories(pffft PUBLIC third_party/pffft)
target_compile_definitions(pffft PUBLIC PFFFT_STATIC_DEFINE)   # static library: no dllimport on Windows
if(MSVC)
  target_compile_options(pffft PRIVATE /O2 /w)
else()
  target_compile_options(pffft PRIVATE -O3 -w)
  target_link_libraries(pffft PRIVATE m)
endif()
if(MSVC)
  add_compile_definitions(_USE_MATH_DEFINES NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

add_library(frdsp STATIC
  dsp/conv/partitioned_convolver.cpp
  dsp/conv/ir_tools.cpp
  dsp/fdn/decay_design.cpp
  dsp/fdn/fdn.cpp
  dsp/er/early_reflections.cpp
  dsp/reverb.cpp
  dsp/engines.cpp
  dsp/io/wav_io.cpp)
target_include_directories(frdsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dsp)
target_link_libraries(frdsp PUBLIC pffft)
if(MSVC)
  # /openmp:experimental enables "#pragma omp simd" without linking the OpenMP runtime.
  target_compile_options(frdsp PRIVATE /W3 /O2 /fp:precise /openmp:experimental)
else()
  target_compile_options(frdsp PRIVATE -Wall -Wextra -O3 -fno-math-errno -fopenmp-simd)
endif()
if(FR_NATIVE AND NOT MSVC)
  target_compile_options(frdsp PRIVATE -march=native)
endif()

add_library(frhost STATIC plugin/EngineHost.cpp)
target_include_directories(frhost PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/plugin)
find_package(Threads REQUIRED)
target_link_libraries(frhost PUBLIC frdsp Threads::Threads)

add_executable(fr_render tools/fr_render.cpp)
target_link_libraries(fr_render frdsp)

add_executable(fr_bench bench/bench_reverb.cpp)
target_link_libraries(fr_bench frdsp)

enable_testing()
add_executable(fr_tests tests/test_dsp.cpp)
target_link_libraries(fr_tests frdsp)
add_test(NAME dsp COMMAND fr_tests)
add_executable(fr_conv_tests tests/test_convolver.cpp)
target_link_libraries(fr_conv_tests frdsp)
add_test(NAME convolver COMMAND fr_conv_tests)
add_executable(fr_ir_tests tests/test_ir_tools.cpp)
target_link_libraries(fr_ir_tests frdsp)
add_test(NAME ir_tools COMMAND fr_ir_tests)
add_executable(fr_host_tests tests/test_engine_host.cpp)
target_link_libraries(fr_host_tests frhost)
add_test(NAME engine_host COMMAND fr_host_tests)

# ---- Plugin (JUCE 8). Off by default so DSP-only builds need no JUCE. ----
option(FR_BUILD_PLUGIN "Build the JUCE plugin" OFF)
if(FR_BUILD_PLUGIN)
  set(FR_JUCE_DIR "" CACHE PATH "Path to a JUCE 8 checkout (empty = download)")
  if(FR_JUCE_DIR)
    add_subdirectory(${FR_JUCE_DIR} juce EXCLUDE_FROM_ALL)
  else()
    include(FetchContent)
    FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.9 GIT_SHALLOW ON)
    FetchContent_MakeAvailable(juce)
  endif()
  set(FR_FORMATS VST3)
  if(APPLE)
    list(APPEND FR_FORMATS AU)
  endif()
  # AAX (Pro Tools): needs the Avid AAX SDK, which cannot be redistributed. See docs/AAX.md.
  set(FR_AAX_SDK_PATH "" CACHE PATH "Path to the Avid AAX SDK (empty = no AAX build)")
  if(FR_AAX_SDK_PATH)
    juce_set_aax_sdk_path(${FR_AAX_SDK_PATH})
    list(APPEND FR_FORMATS AAX)
  endif()
  juce_add_plugin(FancyReverb
    COMPANY_NAME "FancyReverb"
    PLUGIN_MANUFACTURER_CODE Frvb
    PLUGIN_CODE Fnrv
    FORMATS ${FR_FORMATS}
    PRODUCT_NAME "FancyReverb"
    AAX_CATEGORY AAX_ePlugInCategory_Reverb
    VST3_CATEGORIES Fx Reverb
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
  juce_add_binary_data(FRBinaryData SOURCES
    data/irs/small_room.wav data/irs/studio_hall.wav data/irs/large_hall.wav data/presets/factory.json)
  target_sources(FancyReverb PRIVATE plugin/PluginProcessor.cpp plugin/PluginEditor.cpp plugin/Presets.cpp)
  target_compile_definitions(FancyReverb PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_link_libraries(FancyReverb
    PRIVATE frhost FRBinaryData juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

  add_executable(fr_plugin_smoke tests/plugin_smoke.cpp)
  target_include_directories(fr_plugin_smoke PRIVATE plugin)
  target_link_libraries(fr_plugin_smoke PRIVATE FancyReverb frhost FRBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats)
  target_compile_definitions(fr_plugin_smoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  add_executable(fr_editor_snapshot tests/editor_snapshot.cpp)
  target_include_directories(fr_editor_snapshot PRIVATE plugin)
  target_link_libraries(fr_editor_snapshot PRIVATE FancyReverb frhost FRBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats)
  target_compile_definitions(fr_editor_snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  add_test(NAME plugin_smoke COMMAND fr_plugin_smoke ${CMAKE_CURRENT_SOURCE_DIR}/data/irs/studio_hall.wav)
endif()

add_executable(fr_profile_run bench/profile_run.cpp)
target_link_libraries(fr_profile_run frdsp)

# ---- Chromosphere | INFRA (synthesizer), sharing the reverb engine above ----
add_subdirectory(infra)
add_subdirectory(muon)
