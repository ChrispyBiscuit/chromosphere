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
  target_compile_options(infradsp PRIVATE /W3 /O2 /fp:fast)
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
  set(INFRA_FORMATS VST3)
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
    plugin/Presets.cpp plugin/Ignite.cpp plugin/Panels.cpp)
  target_include_directories(ChromosphereInfra PRIVATE plugin)
  target_compile_definitions(ChromosphereInfra PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_link_libraries(ChromosphereInfra
    PRIVATE infradsp InfraBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_opengl
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

  set(INFRA_TEST_LIBS ChromosphereInfra infradsp InfraBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_opengl)
  add_executable(infra_plugin_smoke tests/plugin_smoke.cpp)
  target_include_directories(infra_plugin_smoke PRIVATE plugin)
  target_link_libraries(infra_plugin_smoke PRIVATE ${INFRA_TEST_LIBS})
  target_compile_definitions(infra_plugin_smoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  add_test(NAME infra_plugin_smoke COMMAND infra_plugin_smoke)
  add_executable(infra_preset_render tests/preset_render.cpp)
  target_include_directories(infra_preset_render PRIVATE plugin)
  target_link_libraries(infra_preset_render PRIVATE ${INFRA_TEST_LIBS})
  target_compile_definitions(infra_preset_render PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  add_test(NAME infra_presets COMMAND infra_preset_render)
  add_executable(infra_editor_snapshot tests/editor_snapshot.cpp)
  target_include_directories(infra_editor_snapshot PRIVATE plugin)
  target_link_libraries(infra_editor_snapshot PRIVATE ${INFRA_TEST_LIBS})
  target_compile_definitions(infra_editor_snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
endif()
