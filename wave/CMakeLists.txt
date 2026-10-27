# Chromosphere | WAVE: voice to notes and chords. A JUCE-free engine; INFRA builds it in (see docs/PRD.md).
# Included from the top-level CMakeLists (wave_analyze uses the reverb project's WAV reader, frdsp).

add_library(wavedsp STATIC
  dsp/wave/tracker.cpp
  dsp/wave/harmony.cpp
  dsp/wave/engine.cpp)
target_include_directories(wavedsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dsp)
if(MSVC)
  target_compile_options(wavedsp PRIVATE /W3 /O2 /arch:AVX2 /fp:precise /fp:contract)
else()
  target_compile_options(wavedsp PRIVATE -Wall -Wextra -O3 -fno-math-errno)
endif()

add_executable(wave_tests tests/test_wave.cpp)
target_link_libraries(wave_tests wavedsp)
add_test(NAME wave_dsp COMMAND wave_tests)

add_executable(wave_analyze tools/wave_analyze.cpp)
target_link_libraries(wave_analyze wavedsp frdsp)
