cmake_minimum_required(VERSION 3.20)
project(FancyReverb LANGUAGES CXX)
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

add_executable(fr_render tools/fr_render.cpp)
target_link_libraries(fr_render frdsp)

add_executable(fr_bench bench/bench_reverb.cpp)
target_link_libraries(fr_bench frdsp)

enable_testing()
add_executable(fr_tests tests/test_dsp.cpp)
target_link_libraries(fr_tests frdsp)
add_test(NAME dsp COMMAND fr_tests)
