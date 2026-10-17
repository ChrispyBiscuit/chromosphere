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

## Using the plugin

- **Modes:** Algorithmic (early reflections + feedback delay network), Convolution (an impulse response through a zero-latency convolver),
  Hybrid (the early part of an IR, then a matched algorithmic tail). Load an IR with "Load IR..." or pick a factory one.
- **Presets:** 123 factory presets (`data/presets/factory.json`, made by `tools/make_presets.py`) in six families plus impulse-response presets.
  They are parameter sets built from families with small variations, checked to load and render, but not auditioned by ear.
  User presets are saved as `.frpreset` files in `~/.config/FancyReverb/Presets` (Linux), `~/Library/Application Support/FancyReverb/Presets` (macOS)
  or `%APPDATA%\FancyReverb\Presets` (Windows). A/B compares two settings; Undo and Redo cover parameter changes.
- **Freeze** holds the tail and mutes the input.
- `fr_editor_snapshot` (plugin build) writes PNGs of the editor at several sizes for layout checks without a screen.
