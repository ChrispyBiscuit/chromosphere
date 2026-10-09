cmake_minimum_required(VERSION 3.20)
project(FancyReverb VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
option(FR_NATIVE "Compile with -march=native (local benchmarks only)" OFF)

add_library(frdsp STATIC
  dsp/fdn/decay_design.cpp
  dsp/fdn/fdn.cpp
  dsp/er/early_reflections.cpp
  dsp/reverb.cpp)
target_include_directories(frdsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dsp)
target_compile_options(frdsp PRIVATE -Wall -Wextra -O3 -fno-math-errno)
if(FR_NATIVE)
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
  juce_add_plugin(FancyReverb
    COMPANY_NAME "FancyReverb"
    PLUGIN_MANUFACTURER_CODE Frvb
    PLUGIN_CODE Fnrv
    FORMATS ${FR_FORMATS}
    PRODUCT_NAME "FancyReverb"
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
  target_sources(FancyReverb PRIVATE plugin/PluginProcessor.cpp)
  target_compile_definitions(FancyReverb PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_link_libraries(FancyReverb
    PRIVATE frhost juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
