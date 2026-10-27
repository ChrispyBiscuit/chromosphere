# Chromosphere | ULTRA: drum machine. Included from the top-level CMakeLists (needs frdsp, frhost, pffft).

add_library(ultradsp STATIC
  dsp/ultra/voice.cpp
  dsp/ultra/sequencer.cpp
  dsp/ultra/fx.cpp
  dsp/ultra/machine.cpp)
target_include_directories(ultradsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dsp)
target_link_libraries(ultradsp PUBLIC frhost)
if(MSVC)
  target_compile_options(ultradsp PRIVATE /W3 /O2 /fp:fast)
else()
  target_compile_options(ultradsp PRIVATE -Wall -Wextra -O3 -fno-math-errno)
endif()
if(FR_NATIVE AND NOT MSVC)
  target_compile_options(ultradsp PRIVATE -march=native)
endif()

add_executable(ultra_tests tests/test_ultra.cpp)
target_link_libraries(ultra_tests ultradsp)
add_test(NAME ultra_dsp COMMAND ultra_tests)

add_executable(ultra_bench tools/ultra_bench.cpp)
target_link_libraries(ultra_bench ultradsp)
