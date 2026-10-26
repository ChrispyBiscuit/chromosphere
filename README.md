# Chromosphere

Personal reverb plugin project. See `docs/PRD.md`.

## Build

DSP library, renderer, tests, benchmark (no JUCE needed):

    cmake -S . -B build && cmake --build build -j
    ctest --test-dir build --output-on-failure
    build/cs_bench                      # add -DCS_NATIVE=ON for local CPU numbers
    python3 tools/run_m1.py             # PRD Section 7 metrics on the C++ output

Plugin (JUCE 8, VST3; AU on macOS):

    cmake -S . -B build-plugin -DCS_BUILD_PLUGIN=ON [-DCS_JUCE_DIR=/path/to/JUCE]
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

## Cosmic suite

Ten reverbs beyond the stock set, each its own algorithm: Shimmer, Redshift, Corona, Wormhole, Event Horizon,
Nebula, Pulsar, Solar Wind, Aurora, Supernova. Pick one from the Mode selector; three macro dials appear with
names specific to the design. `docs/COSMIC.md` describes each one. `tools/cosmic_check.py` verifies their behaviour
on renders (for example that Shimmer adds energy an octave up), and `tests/test_cosmic_engine.cpp` checks that every
design is block-size independent, bounded under hostile settings, decays, and freezes.

## Live sun

The emblem top left and the limb behind the scope are a rendered sun (`plugin/Sun.cpp`): a rotating
noise-textured sphere with limb darkening, a turbulent corona, prominence arcs and ejected particles.
Both read the reverb level: a hot signal brightens the surface, speeds the rotation, throws more
particles and raises the prominences; silence calms it over about a second and a half. The renderer
is an OpenGL fragment shader (`juce_opengl`), with a CPU path at lower resolution that takes over when
no GL context can be made (that path also produces the headless snapshots). The wordmark
(`plugin/Wordmark.cpp`) passes through a software lens with chromatic and spherical aberration, a
rare short glitch, and a breathing underscore cursor. Above the scope sun sits a radiation layer:
per-frame sensor grain, flickering hot pixels and short ray tracks that streak and fade, all denser
with signal.

Hot signal also brings sunspots (from the 15–200 Hz band), flares and coronal mass ejections; the
same band makes the dial rings glow and pulse. When the bass gets very elevated, a pane of glass in
front of the scope cracks from an impact point (`plugin/Glass.cpp`) and the cracks stay until the
signal has been gone for a while. The instrument bays are matte anodised: a fine grain and a soft vignette, nothing more.

`cs_sun_gl_check` opens the editor in a real window and reports whether both suns run on the GL
path; under a bare Xvfb it needs Mesa (llvmpipe) and passes there.

## Updates

The plug-in can check for new versions, but only if you turn it on: Updates button (top right) > "Check for updates
when the plug-in opens". It reads `latest.json` from the nightly release once a day, compares versions, and offers to
download the installer to your Downloads folder (checksum verified) and run it. It never replaces its own files and
sends nothing but that one request. While the repository is private the check fails with "Could not reach the update
server", because GitHub release files need a login; it works once the repository is public.

## Using the plugin

- **Modes:** Algorithmic (early reflections + feedback delay network), Convolution (an impulse response through a zero-latency convolver),
  Hybrid (the early part of an IR, then a matched algorithmic tail). Load an IR with "Load IR..." or pick a factory one.
- **Presets:** 123 factory presets (`data/presets/factory.json`, made by `tools/make_presets.py`) in six families plus impulse-response presets.
  They are parameter sets built from families with small variations, checked to load and render, but not auditioned by ear.
  User presets are saved as `.cspreset` files in `~/.config/Chromosphere/Presets` (Linux), `~/Library/Application Support/Chromosphere/Presets` (macOS)
  or `%APPDATA%\Chromosphere\Presets` (Windows). A/B compares two settings; Undo and Redo cover parameter changes.
- **Freeze** holds the tail and mutes the input.
- `cs_editor_snapshot` (plugin build) writes PNGs of the editor at several sizes for layout checks without a screen.
