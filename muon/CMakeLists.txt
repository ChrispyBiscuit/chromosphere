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
  target_compile_options(muondsp PRIVATE /W3 /O2 /fp:fast)
else()
  target_compile_options(muondsp PRIVATE -Wall -Wextra -O3 -fno-math-errno)
endif()
if(FR_NATIVE AND NOT MSVC)
  target_compile_options(muondsp PRIVATE -march=native)
endif()

add_executable(muon_tests tests/test_muon.cpp)
target_link_libraries(muon_tests muondsp)
add_test(NAME muon_dsp COMMAND muon_tests)

add_executable(muon_bench tools/muon_bench.cpp)
target_link_libraries(muon_bench muondsp)
