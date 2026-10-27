# Chromosphere | INFRA

The instrument in the Chromosphere family. EXO is the reverb; INFRA is the same star seen in infrared. It's a
polyphonic wavetable synthesizer with a step arpeggiator, a modulation matrix, four universal macros and an
effects rack that ends in the Chromosphere reverb engine itself. See `docs/PRD.md`.

![Synth page](docs/images/synth.jpg)

| | |
|---|---|
| ![Mod page](docs/images/mod.jpg) | ![Arp page](docs/images/arp.jpg) |
| ![FX page](docs/images/fx.jpg) | |

## What is in it

- **Oscillators A and B**: ten generated spectral tables (Classic, Fraunhofer, Pulsar, Nebula, Ion Drive, Magnetar,
  Plasma, Event Horizon, Dark Matter, Quasar), 16 frames each, band-limited at 11 levels with no aliasing (a
  5 kHz saw is 66 dB clean). Each has Morph, unison up to 8 with detune and width, and the warps Bend, Mirror,
  Quantize, FM from B and Ring by B. A sub oscillator and four noise colours (White, Pink, Solar Wind, Crackle) sit
  alongside.
- **Filter**: LP 24 (ZDF ladder), LP 12, BP 12, HP 12, HP 24, Notch, Comb, Formant, with drive, envelope, key and
  velocity amounts.
- **Modulation**: amp, filter and mod envelopes; two LFOs with tempo sync, fade-in and the Drift and S&H shapes;
  an 8-slot matrix with 18 sources and 26 destinations. Those include LFO Amount (mod-wheel vibrato) and the
  effect sends.
- **Four macros with a fixed meaning in every preset**: HEAT (brightness and energy), GRAVITY (weight and pull),
  ORBIT (motion) and FLUX (space and aftermath).
- **Orbit arpeggiator**: Up, Down, Up/Down, Down/Up, As Played, Random, Converge, Diverge, Chord, Orbit and Pulsar
  modes. It has a 16-step lane of velocity (0 is a rest), transpose and ratchet, plus swing, latch and step
  probability, and syncs to the host transport.
- **Effects**: Drive (Soft, Tube, Fold, Cosmic Ray), the Aurora chorus, the Echo ping-pong delay, the Chromosphere
  reverb (the EXO FDN engine through its thread-safe host), then width, output and a peak limiter.
- **107 factory presets** in 12 categories, each designed individually (`tools/make_presets.py`). Every preset uses
  the four macros, the mod wheel and aftertouch. Levels are calibrated by measurement to within about 0.6 dB. None
  has been auditioned by ear yet.
- **Ignite** builds a new patch from rules for a chosen sound type (bass, lead, pad, pluck, keys, arp, drone or FX).
- **Interface** in the EXO design language, in red: matte instrument bays with bolts, knurled dials with lit arcs
  that pulse with the bass, red seven-segment readouts, lever switches, and a cyan VFD preset selector and combos.
  An amber CRT scope (wavetables, filter response, envelope, output) sits over the rising limb of a live red sun.
- An on-screen keyboard, user presets, A/B, undo and redo. Any window size from 1180 x 820 to 1900 x 1300.

## Build

INFRA builds with the rest of the repository (it reuses the reverb's `frdsp` and `frhost`):

    cmake -S . -B build && cmake --build build -j --target infra_tests infra_bench
    build/infra/infra_tests            # DSP: tables, aliasing, voices, sustain, arp timing, block sizes, rates
    build/infra/infra_bench            # CPU: 16 voices x 2 osc x 4 unison with all effects

    cmake -S . -B build-plugin -DFR_BUILD_PLUGIN=ON [-DFR_JUCE_DIR=/path/to/JUCE]
    cmake --build build-plugin -j --target ChromosphereInfra_VST3 infra_plugin_smoke infra_preset_render infra_editor_snapshot
    build-plugin/infra/infra_plugin_smoke        # parameters, state, Ignite, mono bus, fuzzing, user presets
    build-plugin/infra/infra_preset_render       # renders every factory preset and checks it
    xvfb-run -a build-plugin/infra/infra_editor_snapshot out/   # PNGs of the editor

Presets: edit `tools/make_presets.py` and run it to rewrite `data/presets.json`. To recalibrate levels, run
`infra_preset_render --measure measured.json`, then `python3 tools/make_presets.py --calibrate measured.json`, and
repeat two or three times.

Windows: CI uploads `ChromosphereInfra-windows-vst3` with `install_windows.ps1`. FL Studio lists it as
"Chromosphere | INFRA" under generators.

## Status

- Linux: everything builds; all tests pass; the editor was checked from snapshots.
- Windows (MSVC) and macOS (AU): set up in CI but not yet built there.
- CPU: the worst case above uses about 15-16% of one core on a 2.8 GHz cloud VM, near the PRD's 15% target
  (which assumes a desktop CPU).
