# Chromosphere

Personal reverb plugin project. See `docs/PRD.md`.

## Build

DSP library, renderer, tests, benchmark (no JUCE needed):

    cmake -S . -B build && cmake --build build -j
    ctest --test-dir build --output-on-failure
    build/cs_bench                      # add -DFR_NATIVE=ON for local CPU numbers
    python3 tools/run_m1.py             # PRD Section 7 metrics on the C++ output

Plugin (JUCE 8, VST3; AU on macOS):

    cmake -S . -B build-plugin -DFR_BUILD_PLUGIN=ON [-DFR_JUCE_DIR=/path/to/JUCE]
    cmake --build build-plugin --target Chromosphere_VST3 -j
    pluginval --strictness-level 10 build-plugin/Chromosphere_artefacts/Release/VST3/Chromosphere.vst3

Python tools need numpy, scipy, soundfile, pyroomacoustics.

## Windows: FL Studio and Pro Tools

Every push builds a Windows installer. Download it here (no login needed):

    https://github.com/ChrispyBiscuit/Chromosphere/releases/download/nightly/Chromosphere-Setup.exe

Run it (Windows SmartScreen warns because it is not code-signed: "More info" > "Run anyway"). It installs
- the VST3 to `C:\Program Files\Common Files\VST3\Chromosphere.vst3`, the folder FL Studio and all other VST3 hosts scan;
- the AAX to `C:\Program Files\Common Files\Avid\Audio\Plug-Ins`, when the installer was built with the Avid AAX SDK (not yet: see `docs/AAX.md`).

Plug-ins do not go into the FL Studio program folder (`C:\Program Files\Image-Line\FL Studio 2024`).

Then in FL Studio: Options > Manage plugins > "Find plugins" (tick "Verify plugins" the first time) > filter for "Fancy" >
tick the star in the F column. Pick it from a mixer slot or from Browser > Plugin database > Installed > Effects > VST3.
FL Studio 20.8 or newer is needed for VST3. The installer script is `installer/Chromosphere.iss` (Inno Setup);
`Chromosphere-windows-vst3.zip` on the same release page has the bare VST3 and `install_windows.ps1` for a manual install.

## Pro Tools

See `docs/AAX.md`. It needs the Avid AAX SDK and signing, and has never been built here.

## Using the plugin

- **Modes:** Algorithmic (early reflections + feedback delay network), Convolution (an impulse response through a zero-latency convolver),
  Hybrid (the early part of an IR, then a matched algorithmic tail). Load an IR with "Load IR..." or pick a factory one.
- **Presets:** 123 factory presets (`data/presets/factory.json`, made by `tools/make_presets.py`) in six families plus impulse-response presets.
  They are parameter sets built from families with small variations, checked to load and render, but not auditioned by ear.
  User presets are saved as `.cspreset` files in `~/.config/Chromosphere/Presets` (Linux), `~/Library/Application Support/Chromosphere/Presets` (macOS)
  or `%APPDATA%\Chromosphere\Presets` (Windows). A/B compares two settings; Undo and Redo cover parameter changes.
- **Freeze** holds the tail and mutes the input.
- `cs_editor_snapshot` (plugin build) writes PNGs of the editor at several sizes for layout checks without a screen.
