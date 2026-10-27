# Chromosphere

Personal reverb plugin project. See `docs/PRD.md`.

**Chromosphere | ULTRA**, the drum machine in the family, lives in `ultra/` (see `ultra/README.md` and `ultra/docs/PRD.md`).

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

## Windows and FL Studio

CI (`.github/workflows/build.yml`) builds a Windows VST3 on every push. Download the `FancyReverb-windows-vst3` artifact from the Actions tab,
run `install_windows.ps1` as administrator (or copy `FancyReverb.vst3` to `C:\Program Files\Common Files\VST3`), then in FL Studio
use Options > Manage plugins > Find installed plugins. FL Studio needs version 20.8 or newer for VST3.
Status: the workflow has not run yet. The DSP code was cross-compiled with MinGW and its tests pass as Windows programs (under Wine);
the JUCE plugin has not been built with MSVC.

## Pro Tools

See `docs/AAX.md`. It needs the Avid AAX SDK and signing, and has never been built here.

## Using the plugin

- **Modes:** Algorithmic (early reflections + feedback delay network), Convolution (an impulse response through a zero-latency convolver),
  Hybrid (the early part of an IR, then a matched algorithmic tail). Load an IR with "Load IR..." or pick a factory one.
- **Presets:** 123 factory presets (`data/presets/factory.json`, made by `tools/make_presets.py`) in six families plus impulse-response presets.
  They are parameter sets built from families with small variations, checked to load and render, but not auditioned by ear.
  User presets are saved as `.frpreset` files in `~/.config/FancyReverb/Presets` (Linux), `~/Library/Application Support/FancyReverb/Presets` (macOS)
  or `%APPDATA%\FancyReverb\Presets` (Windows). A/B compares two settings; Undo and Redo cover parameter changes.
- **Freeze** holds the tail and mutes the input.
- `fr_editor_snapshot` (plugin build) writes PNGs of the editor at several sizes for layout checks without a screen.
