# Chromosphere | INFRA: polyphonic synthesizer. Included from the top-level CMakeLists (needs frdsp, frhost, pffft).

add_library(infradsp STATIC
  dsp/infra/tables.cpp
  dsp/infra/voice.cpp
  dsp/infra/arp.cpp
  dsp/infra/fx.cpp
  dsp/infra/synth.cpp)
target_include_directories(infradsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dsp)
target_link_libraries(infradsp PUBLIC frhost pffft)
if(MSVC)
  # AVX2 (optimisation guide 5.5): every Intel and AMD desktop CPU since about 2013 has it.
  target_compile_options(infradsp PRIVATE /W3 /O2 /fp:fast /arch:AVX2)
else()
  target_compile_options(infradsp PRIVATE -Wall -Wextra -O3 -fno-math-errno)
endif()
if(FR_NATIVE AND NOT MSVC)
  target_compile_options(infradsp PRIVATE -march=native)
endif()

add_executable(infra_tests tests/test_infra.cpp)
target_link_libraries(infra_tests infradsp)
add_test(NAME infra_dsp COMMAND infra_tests)

add_executable(infra_bench tools/infra_bench.cpp)
target_link_libraries(infra_bench infradsp)

# ---- Plugin ----
if(FR_BUILD_PLUGIN)
  # Product version: CI passes 0.1.<run number>; the updater compares it with the release manifest.
  set(INFRA_VERSION "0.1.0" CACHE STRING "INFRA product version (x.y.z)")
  if(NOT INFRA_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(WARNING "INFRA_VERSION \"${INFRA_VERSION}\" is not x.y.z; using 0.1.0")
    set(INFRA_VERSION "0.1.0" CACHE STRING "INFRA product version (x.y.z)" FORCE)
  endif()
  # The GitHub repository whose "infra-nightly" release holds the installer and latest.json.
  set(INFRA_UPDATE_REPO "ChrispyBiscuit/chromosphere" CACHE STRING "owner/repo for INFRA updates")
  set(INFRA_FORMATS VST3 Standalone)
  if(APPLE)
    list(APPEND INFRA_FORMATS AU)
  endif()
  juce_add_plugin(ChromosphereInfra
    COMPANY_NAME "Chromosphere"
    PLUGIN_MANUFACTURER_CODE Chrm
    PLUGIN_CODE Cinf
    FORMATS ${INFRA_FORMATS}
    PRODUCT_NAME "ChromosphereInfra"            # bundle and file name (no "|" allowed in file names)
    PLUGIN_NAME "Chromosphere | INFRA"          # the name hosts show
    VERSION ${INFRA_VERSION}
    ICON_BIG ${CMAKE_CURRENT_SOURCE_DIR}/installer/icon_1024.png
    ICON_SMALL ${CMAKE_CURRENT_SOURCE_DIR}/installer/icon_256.png
    DESCRIPTION "Polyphonic wavetable synthesizer"
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
  file(GLOB INFRA_FONTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/*.ttf)
  file(GLOB INFRA_IMAGES ${CMAKE_CURRENT_SOURCE_DIR}/assets/img/*.png ${CMAKE_CURRENT_SOURCE_DIR}/assets/img/*.jpg)
  juce_add_binary_data(InfraBinaryData NAMESPACE InfraData HEADER_NAME InfraData.h SOURCES data/presets.json ${INFRA_FONTS} ${INFRA_IMAGES})
  target_sources(ChromosphereInfra PRIVATE
    plugin/PluginProcessor.cpp plugin/PluginEditor.cpp plugin/InfraLook.cpp plugin/Sun.cpp plugin/Wordmark.cpp
    plugin/Presets.cpp plugin/Ignite.cpp plugin/Panels.cpp plugin/Vfd.cpp plugin/SevenSeg.cpp plugin/Updater.cpp
    plugin/NoteExpression.cpp)
  # NoteExpression.cpp reads VST3 events and implements INoteExpressionController: it needs the SDK's pluginterfaces.
  target_include_directories(ChromosphereInfra PRIVATE plugin ${FR_JUCE_SOURCE_DIR}/modules/juce_audio_processors/format_types/VST3_SDK)
  target_compile_definitions(ChromosphereInfra PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    INFRA_UPDATE_REPO="${INFRA_UPDATE_REPO}")
  target_link_libraries(ChromosphereInfra
    PRIVATE infradsp InfraBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_opengl juce::juce_cryptography
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

  set(INFRA_TEST_LIBS ChromosphereInfra infradsp InfraBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_opengl juce::juce_cryptography)
  add_executable(infra_plugin_smoke tests/plugin_smoke.cpp)
  target_include_directories(infra_plugin_smoke PRIVATE plugin ${FR_JUCE_SOURCE_DIR}/modules/juce_audio_processors/format_types/VST3_SDK)
  target_link_libraries(infra_plugin_smoke PRIVATE ${INFRA_TEST_LIBS})
  target_compile_definitions(infra_plugin_smoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  add_test(NAME infra_plugin_smoke COMMAND infra_plugin_smoke)
  add_executable(infra_preset_render tests/preset_render.cpp)
  target_include_directories(infra_preset_render PRIVATE plugin)
  target_link_libraries(infra_preset_render PRIVATE ${INFRA_TEST_LIBS})
  target_compile_definitions(infra_preset_render PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  add_test(NAME infra_presets COMMAND infra_preset_render)
  add_executable(infra_updater_test tests/updater_test.cpp)
  target_include_directories(infra_updater_test PRIVATE plugin)
  target_link_libraries(infra_updater_test PRIVATE ${INFRA_TEST_LIBS})
  target_compile_definitions(infra_updater_test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
  add_test(NAME infra_updater COMMAND infra_updater_test)
  add_executable(infra_plugin_bench tools/infra_plugin_bench.cpp)
  target_include_directories(infra_plugin_bench PRIVATE plugin)
  target_link_libraries(infra_plugin_bench PRIVATE ${INFRA_TEST_LIBS})
  target_compile_definitions(infra_plugin_bench PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  add_executable(infra_editor_snapshot tests/editor_snapshot.cpp)
  target_include_directories(infra_editor_snapshot PRIVATE plugin)
  target_link_libraries(infra_editor_snapshot PRIVATE ${INFRA_TEST_LIBS})
  target_compile_definitions(infra_editor_snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
endif()
