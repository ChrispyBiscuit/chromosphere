# Chromosphere | MUON: transient and harmonics processor. Included from the top-level CMakeLists (needs frhost, pffft).

add_library(muondsp STATIC
  dsp/muon/filters.cpp
  dsp/muon/crossover.cpp
  dsp/muon/harmonics.cpp
  dsp/muon/sub.cpp
  dsp/muon/resonance.cpp
  dsp/muon/engine.cpp)
target_include_directories(muondsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dsp)
target_link_libraries(muondsp PUBLIC frhost pffft)
if(MSVC)
  # Optimization guide 5.5: AVX2 and contraction, precise otherwise.
  target_compile_options(muondsp PRIVATE /W3 /O2 /arch:AVX2 /fp:precise /fp:contract)
else()
  target_compile_options(muondsp PRIVATE -Wall -Wextra -O3 -fno-math-errno -fno-trapping-math)
endif()
if(FR_NATIVE AND NOT MSVC)
  target_compile_options(muondsp PRIVATE -march=native)
endif()

add_executable(muon_tests tests/test_muon.cpp)
target_link_libraries(muon_tests muondsp)
add_test(NAME muon_dsp COMMAND muon_tests)

add_executable(muon_bench tools/muon_bench.cpp)
target_link_libraries(muon_bench muondsp)

# ---- Plugin ----
if(FR_BUILD_PLUGIN)
  # Product version: CI passes 0.1.<run number>; the updater compares it with the release manifest.
  set(MUON_VERSION "0.1.0" CACHE STRING "MUON product version (x.y.z)")
  if(NOT MUON_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(WARNING "MUON_VERSION \"${MUON_VERSION}\" is not x.y.z; using 0.1.0")
    set(MUON_VERSION "0.1.0" CACHE STRING "MUON product version (x.y.z)" FORCE)
  endif()
  set(MUON_UPDATE_REPO "ChrispyBiscuit/chromosphere" CACHE STRING "owner/repo for MUON updates")
  set(MUON_FORMATS VST3)
  if(APPLE)
    list(APPEND MUON_FORMATS AU)
  endif()
  juce_add_plugin(ChromosphereMuon
    COMPANY_NAME "Chromosphere"
    PLUGIN_MANUFACTURER_CODE Chrm
    PLUGIN_CODE Cmuo
    FORMATS ${MUON_FORMATS}
    PRODUCT_NAME "ChromosphereMuon"             # bundle and file name (no "|" allowed in file names)
    PLUGIN_NAME "Chromosphere | MUON"           # the name hosts show
    VERSION ${MUON_VERSION}
    ICON_BIG ${CMAKE_CURRENT_SOURCE_DIR}/installer/icon_1024.png
    ICON_SMALL ${CMAKE_CURRENT_SOURCE_DIR}/installer/icon_256.png
    DESCRIPTION "Transient and harmonics processor"
    VST3_CATEGORIES Fx Dynamics Distortion
    AU_MAIN_TYPE kAudioUnitType_MusicEffect
    IS_SYNTH FALSE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
  # Fonts are shared with INFRA (same files); the solar images are MUON's blue ones.
  file(GLOB MUON_FONTS ${CMAKE_SOURCE_DIR}/infra/assets/fonts/*.ttf)
  file(GLOB MUON_IMAGES ${CMAKE_CURRENT_SOURCE_DIR}/assets/img/*.png ${CMAKE_CURRENT_SOURCE_DIR}/assets/img/*.jpg)
  juce_add_binary_data(MuonBinaryData NAMESPACE MuonData HEADER_NAME MuonData.h SOURCES data/presets.json ${MUON_FONTS} ${MUON_IMAGES})
  target_sources(ChromosphereMuon PRIVATE
    plugin/PluginProcessor.cpp plugin/PluginEditor.cpp plugin/Presets.cpp plugin/Ignite.cpp plugin/Panels.cpp)
  target_include_directories(ChromosphereMuon PRIVATE plugin)
  target_compile_definitions(ChromosphereMuon PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    MUON_UPDATE_REPO="${MUON_UPDATE_REPO}")
  target_link_libraries(ChromosphereMuon
    PRIVATE muondsp MuonBinaryData chromosphere_ui juce::juce_audio_utils juce::juce_dsp juce::juce_opengl juce::juce_cryptography
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

  set(MUON_TEST_LIBS ChromosphereMuon muondsp MuonBinaryData juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats juce::juce_opengl juce::juce_cryptography)
  set(MUON_PLUGIN_TESTS plugin_smoke preset_render editor_snapshot)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND MUON_PLUGIN_TESTS editor_cpu)   # reads /proc
  endif()
  foreach(t ${MUON_PLUGIN_TESTS})
    add_executable(muon_${t} tests/${t}.cpp)
    target_include_directories(muon_${t} PRIVATE plugin ${CMAKE_SOURCE_DIR}/ui)
    target_link_libraries(muon_${t} PRIVATE ${MUON_TEST_LIBS})
    target_compile_definitions(muon_${t} PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  endforeach()
  if(TARGET muon_editor_cpu)
    target_compile_definitions(muon_editor_cpu PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1)
  endif()
  add_test(NAME muon_plugin_smoke COMMAND muon_plugin_smoke)
  add_test(NAME muon_presets COMMAND muon_preset_render)
endif()
