# Applies cmake/juce-vst3-input-events.patch to a JUCE checkout once (it is skipped when already applied).
#   cmake -DJUCE_DIR=<checkout> -DPATCH=<patch> -P ApplyJucePatch.cmake
# The patch adds VST3ClientExtensions::processInputEvents, which hands a plugin the host's raw VST3 events (stock
# JUCE drops note expression before processBlock). Chromosphere | INFRA uses it for per-note tuning (FL Studio slides).
find_package(Git REQUIRED)
execute_process(COMMAND ${GIT_EXECUTABLE} apply --ignore-whitespace --reverse --check ${PATCH}
                WORKING_DIRECTORY ${JUCE_DIR} RESULT_VARIABLE already OUTPUT_QUIET ERROR_QUIET)
if(already EQUAL 0)
  message(STATUS "JUCE patch already applied: ${PATCH}")
  return()
endif()
execute_process(COMMAND ${GIT_EXECUTABLE} apply --ignore-whitespace ${PATCH}
                WORKING_DIRECTORY ${JUCE_DIR} RESULT_VARIABLE failed)
if(NOT failed EQUAL 0)
  message(FATAL_ERROR "Could not apply ${PATCH} to ${JUCE_DIR}")
endif()
message(STATUS "Applied JUCE patch: ${PATCH}")
