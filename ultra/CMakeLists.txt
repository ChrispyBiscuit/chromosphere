# Chromosphere | ULTRA: drum machine. Included from the top-level CMakeLists (needs frdsp, frhost, pffft).

add_library(ultradsp STATIC
  dsp/ultra/voice.cpp
  dsp/ultra/sequencer.cpp
  dsp/ultra/fx.cpp
  dsp/ultra/machine.cpp)
target_include_directories(ultradsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dsp)
target_link_libraries(ultradsp PUBLIC frhost)
if(MSVC)
  # AVX2 with precise float semantics plus FMA contraction (EXO optimization guide): fast, and identical results at
  # every block size.
  target_compile_options(ultradsp PRIVATE /W3 /O2 /arch:AVX2 /fp:precise /fp:contract)
else()
  target_compile_options(ultradsp PRIVATE -Wall -Wextra -O3 -fno-math-errno -ffp-contract=fast)
endif()
if(FR_NATIVE AND NOT MSVC)
  target_compile_options(ultradsp PRIVATE -march=native)
endif()

add_executable(ultra_tests tests/test_ultra.cpp)
target_link_libraries(ultra_tests ultradsp)
add_test(NAME ultra_dsp COMMAND ultra_tests)

add_executable(ultra_bench tools/ultra_bench.cpp)
target_link_libraries(ultra_bench ultradsp)

# ---- Plugin ----
if(FR_BUILD_PLUGIN)
  # Product version: CI passes 0.1.<run number>; the updater compares it with the release manifest.
  set(ULTRA_VERSION "0.1.0" CACHE STRING "ULTRA product version (x.y.z)")
  if(NOT ULTRA_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(WARNING "ULTRA_VERSION \"${ULTRA_VERSION}\" is not x.y.z; using 0.1.0")
    set(ULTRA_VERSION "0.1.0" CACHE STRING "ULTRA product version (x.y.z)" FORCE)
  endif()
  # The GitHub repository whose "ultra-nightly" release holds the installer and latest.json.
  set(ULTRA_UPDATE_REPO "ChrispyBiscuit/chromosphere" CACHE STRING "owner/repo for ULTRA updates")
  set(ULTRA_FORMATS VST3 Standalone)
  if(APPLE)
    list(APPEND ULTRA_FORMATS AU)
  endif()
  juce_add_plugin(ChromosphereUltra
    COMPANY_NAME "Chromosphere"
    PLUGIN_MANUFACTURER_CODE Chrm
    PLUGIN_CODE Cult
    FORMATS ${ULTRA_FORMATS}
    PRODUCT_NAME "ChromosphereUltra"            # bundle and file name (no "|" allowed in file names)
    PLUGIN_NAME "Chromosphere | ULTRA"          # the name hosts show
    VERSION ${ULTRA_VERSION}
    ICON_BIG ${CMAKE_CURRENT_SOURCE_DIR}/installer/icon_1024.png
    ICON_SMALL ${CMAKE_CURRENT_SOURCE_DIR}/installer/icon_256.png
    DESCRIPTION "Synthesized drum machine"
    VST3_CATEGORIES Instrument Drum
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
  file(GLOB ULTRA_FONTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/*.ttf)
  file(GLOB ULTRA_IMAGES ${CMAKE_CURRENT_SOURCE_DIR}/assets/img/*.png ${CMAKE_CURRENT_SOURCE_DIR}/assets/img/*.jpg)
  juce_add_binary_data(UltraBinaryData NAMESPACE UltraData HEADER_NAME UltraData.h SOURCES data/presets.json ${ULTRA_FONTS} ${ULTRA_IMAGES})
  target_sources(ChromosphereUltra PRIVATE
    plugin/PluginProcessor.cpp plugin/PluginEditor.cpp plugin/UltraLook.cpp plugin/Sun.cpp plugin/Wordmark.cpp plugin/PatternStore.cpp
    plugin/Presets.cpp plugin/Ignite.cpp plugin/Panels.cpp plugin/Vfd.cpp plugin/SevenSeg.cpp plugin/Updater.cpp)
  target_include_directories(ChromosphereUltra PRIVATE plugin)
  target_compile_definitions(ChromosphereUltra PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    ULTRA_UPDATE_REPO="${ULTRA_UPDATE_REPO}")
  target_link_libraries(ChromosphereUltra
    PRIVATE ultradsp UltraBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_opengl juce::juce_cryptography
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

  set(ULTRA_TEST_LIBS ChromosphereUltra ultradsp UltraBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_opengl juce::juce_cryptography)
  add_executable(ultra_plugin_smoke tests/plugin_smoke.cpp)
  target_include_directories(ultra_plugin_smoke PRIVATE plugin)
  target_link_libraries(ultra_plugin_smoke PRIVATE ${ULTRA_TEST_LIBS})
  target_compile_definitions(ultra_plugin_smoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  add_test(NAME ultra_plugin_smoke COMMAND ultra_plugin_smoke)
  add_executable(ultra_preset_render tests/preset_render.cpp)
  target_include_directories(ultra_preset_render PRIVATE plugin)
  target_link_libraries(ultra_preset_render PRIVATE ${ULTRA_TEST_LIBS})
  target_compile_definitions(ultra_preset_render PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  add_test(NAME ultra_presets COMMAND ultra_preset_render)
  add_executable(ultra_updater_test tests/updater_test.cpp)
  target_include_directories(ultra_updater_test PRIVATE plugin)
  target_link_libraries(ultra_updater_test PRIVATE ${ULTRA_TEST_LIBS})
  target_compile_definitions(ultra_updater_test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
  add_test(NAME ultra_updater COMMAND ultra_updater_test)
  add_executable(ultra_plugin_bench tools/ultra_plugin_bench.cpp)
  target_include_directories(ultra_plugin_bench PRIVATE plugin)
  target_link_libraries(ultra_plugin_bench PRIVATE ${ULTRA_TEST_LIBS})
  target_compile_definitions(ultra_plugin_bench PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  add_executable(ultra_editor_snapshot tests/editor_snapshot.cpp)
  target_include_directories(ultra_editor_snapshot PRIVATE plugin)
  target_link_libraries(ultra_editor_snapshot PRIVATE ${ULTRA_TEST_LIBS})
  target_compile_definitions(ultra_editor_snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
endif()
