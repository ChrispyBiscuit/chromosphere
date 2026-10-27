cmake_minimum_required(VERSION 3.20)
set(CS_VERSION "0.1.0" CACHE STRING "Product version (CI passes 0.1.<run number>)")
if(NOT CS_VERSION MATCHES "^[0-9]+\.[0-9]+\.[0-9]+$")
  message(WARNING "CS_VERSION \"${CS_VERSION}\" is not x.y.z; using 0.1.0")
  set(CS_VERSION "0.1.0" CACHE STRING "Product version (CI passes 0.1.<run number>)" FORCE)
endif()
project(Chromosphere VERSION ${CS_VERSION} LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # the static libraries end up inside the plugin shared object
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
option(CS_NATIVE "Compile with -march=native (local benchmarks only)" OFF)

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

add_library(csdsp STATIC
  dsp/conv/partitioned_convolver.cpp
  dsp/conv/ir_tools.cpp
  dsp/cosmic/modules.cpp
  dsp/cosmic/cosmic_engine.cpp
  dsp/fdn/decay_design.cpp
  dsp/fdn/fdn.cpp
  dsp/er/early_reflections.cpp
  dsp/reverb.cpp
  dsp/engines.cpp
  dsp/io/wav_io.cpp)
target_include_directories(csdsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dsp)
target_link_libraries(csdsp PUBLIC pffft)
if(MSVC)
  # /openmp:experimental enables "#pragma omp simd" without linking the OpenMP runtime.
  target_compile_options(csdsp PRIVATE /W3 /O2 /fp:precise /openmp:experimental)
else()
  target_compile_options(csdsp PRIVATE -Wall -Wextra -O3 -fno-math-errno -fopenmp-simd)
endif()
if(CS_NATIVE AND NOT MSVC)
  target_compile_options(csdsp PRIVATE -march=native)
endif()

add_library(cshost STATIC plugin/EngineHost.cpp)
target_include_directories(cshost PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/plugin)
find_package(Threads REQUIRED)
target_link_libraries(cshost PUBLIC csdsp Threads::Threads)

add_executable(cs_render tools/cs_render.cpp)
target_link_libraries(cs_render csdsp)

add_executable(cs_bench bench/bench_reverb.cpp)
target_link_libraries(cs_bench csdsp)

enable_testing()
add_executable(cs_tests tests/test_dsp.cpp)
target_link_libraries(cs_tests csdsp)
add_test(NAME dsp COMMAND cs_tests)
add_executable(cs_conv_tests tests/test_convolver.cpp)
target_link_libraries(cs_conv_tests csdsp)
add_test(NAME convolver COMMAND cs_conv_tests)
add_executable(cs_ir_tests tests/test_ir_tools.cpp)
target_link_libraries(cs_ir_tests csdsp)
add_test(NAME ir_tools COMMAND cs_ir_tests)
add_executable(cs_cosmic_tests tests/test_cosmic_modules.cpp)
target_link_libraries(cs_cosmic_tests csdsp)
add_test(NAME cosmic_modules COMMAND cs_cosmic_tests)
add_executable(cs_cosmic_engine_tests tests/test_cosmic_engine.cpp)
target_link_libraries(cs_cosmic_engine_tests csdsp)
add_test(NAME cosmic_engine COMMAND cs_cosmic_engine_tests)
add_executable(cs_host_tests tests/test_engine_host.cpp)
target_link_libraries(cs_host_tests cshost)
add_test(NAME engine_host COMMAND cs_host_tests)

# ---- Plugin (JUCE 8). Off by default so DSP-only builds need no JUCE. ----
option(CS_BUILD_PLUGIN "Build the JUCE plugin" OFF)
if(CS_BUILD_PLUGIN)
  set(CS_JUCE_DIR "" CACHE PATH "Path to a JUCE 8 checkout (empty = download)")
  if(CS_JUCE_DIR)
    add_subdirectory(${CS_JUCE_DIR} juce EXCLUDE_FROM_ALL)
  else()
    include(FetchContent)
    FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.9 GIT_SHALLOW ON)
    FetchContent_MakeAvailable(juce)
  endif()
  set(CS_FORMATS VST3)
  if(APPLE)
    list(APPEND CS_FORMATS AU)
  endif()
  # AAX (Pro Tools): needs the Avid AAX SDK, which cannot be redistributed. See docs/AAX.md.
  set(CS_AAX_SDK_PATH "" CACHE PATH "Path to the Avid AAX SDK (empty = no AAX build)")
  if(CS_AAX_SDK_PATH)
    juce_set_aax_sdk_path(${CS_AAX_SDK_PATH})
    list(APPEND CS_FORMATS AAX)
  endif()
  juce_add_plugin(Chromosphere
    COMPANY_NAME "Chromosphere"
    PLUGIN_MANUFACTURER_CODE Chrm
    PLUGIN_CODE Csph
    FORMATS ${CS_FORMATS}
    PRODUCT_NAME "Chromosphere"                 # bundle and file name (no "|" allowed in file names)
    PLUGIN_NAME "Chromosphere | EXO"            # the name hosts show
    AAX_CATEGORY AAX_ePlugInCategory_Reverb
    VST3_CATEGORIES Fx Reverb
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
  file(GLOB CS_FONTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/*.ttf)
  file(GLOB CS_IMAGES ${CMAKE_CURRENT_SOURCE_DIR}/assets/img/*.png ${CMAKE_CURRENT_SOURCE_DIR}/assets/img/*.jpg)
  juce_add_binary_data(CSBinaryData SOURCES
    data/irs/small_room.wav data/irs/studio_hall.wav data/irs/large_hall.wav data/presets/factory.json ${CS_FONTS} ${CS_IMAGES})
  target_sources(Chromosphere PRIVATE plugin/PluginProcessor.cpp plugin/PluginEditor.cpp plugin/StudioLook.cpp plugin/Presets.cpp plugin/Updater.cpp plugin/Sun.cpp plugin/Wordmark.cpp plugin/Vfd.cpp plugin/SevenSeg.cpp)
  target_compile_definitions(Chromosphere PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_link_libraries(Chromosphere
    PRIVATE cshost CSBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_cryptography juce::juce_graphics juce::juce_opengl
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

  add_executable(cs_plugin_smoke tests/plugin_smoke.cpp)
  target_include_directories(cs_plugin_smoke PRIVATE plugin)
  target_link_libraries(cs_plugin_smoke PRIVATE Chromosphere cshost CSBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_opengl)
  target_compile_definitions(cs_plugin_smoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  add_executable(cs_editor_snapshot tests/editor_snapshot.cpp)
  target_include_directories(cs_editor_snapshot PRIVATE plugin)
  target_link_libraries(cs_editor_snapshot PRIVATE Chromosphere cshost CSBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_opengl)
  target_compile_definitions(cs_editor_snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
  find_package(X11)
  add_executable(cs_sun_gl_check tests/sun_gl_check.cpp)
  target_include_directories(cs_sun_gl_check PRIVATE plugin)
  target_link_libraries(cs_sun_gl_check PRIVATE Chromosphere cshost CSBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_cryptography juce::juce_opengl ${X11_X11_LIB})
  target_compile_definitions(cs_sun_gl_check PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
  add_test(NAME plugin_smoke COMMAND cs_plugin_smoke ${CMAKE_CURRENT_SOURCE_DIR}/data/irs/studio_hall.wav)
endif()

add_executable(cs_profile_run bench/profile_run.cpp)
target_link_libraries(cs_profile_run csdsp)
