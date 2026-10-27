# Chromosphere shared editor code: the family look and feel (themed per product), the live sun, the wordmark,
# the VFD and seven-segment readouts, parameter-bound controls and the updater. An INTERFACE library, so the
# sources compile inside each plug-in target with that target's JUCE modules and binary data.
add_library(chromosphere_ui INTERFACE)
target_sources(chromosphere_ui INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/Look.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Sun.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Wordmark.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SevenSeg.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Vfd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Controls.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Updater.cpp)
target_include_directories(chromosphere_ui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
