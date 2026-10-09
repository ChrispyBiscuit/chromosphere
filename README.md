# FancyReverb

Personal reverb plugin project. See `docs/PRD.md`.

## Build

DSP library, renderer, tests, benchmark (no JUCE needed):

    cmake -S . -B build && cmake --build build -j
    ctest --test-dir build --output-on-failure
    build/fr_bench                      # add -DFR_NATIVE=ON for local CPU numbers
    python3 tools/run_m1.py             # PRD Section 7 metrics on the C++ output

Plugin (JUCE 8, VST3; AU on macOS):

    cmake -S . -B build-plugin -DFR_BUILD_PLUGIN=ON [-DFR_JUCE_DIR=/path/to/JUCE]
    cmake --build build-plugin --target FancyReverb_VST3 -j
    pluginval --strictness-level 10 build-plugin/FancyReverb_artefacts/Release/VST3/FancyReverb.vst3

Python tools need numpy, scipy, soundfile, pyroomacoustics.
