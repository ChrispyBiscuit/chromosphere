# Chromosphere | MUON

The transient and harmonics processor in the Chromosphere family. EXO is the reverb, INFRA the synth; MUON is what
the sun's cosmic rays make when they hit the atmosphere: a fast, heavy particle that arrives first and leaves a
bright trail. Here the arrival is the transient and the trail is the harmonics and resonance that follow it.
See `docs/PRD.md`.

## What is in it

- **Split**: one to three bands (Linkwitz-Riley minimum phase, or linear phase), each separated into a transient
  stream and a body stream by a level-independent differential-envelope detector (no threshold). Lookahead 0 to
  5 ms; stereo-linked, dual or mid/side.
- **Shape**: Attack and Sustain per band (about -24 to +15 dB and -24 to +24 dB), Speed, and a transient Clip;
  global Punch and Tail.
- **Harmonics**: an **order bank** per band, eight faders H1 to H8 that add exactly those harmonic orders. It works on
  the source's analytic signal (a Hilbert pair), so the orders are exact for a tone at any level with no envelope
  ripple. Each order can be phase-inverted, and there are seven one-click shapes (Tube, Tape, Transformer, Square,
  Saw, Octaver, Bell). Six drive curves (Soft, Tube, Tape, Diode, Fold, Rectify) with bias, tone and mix. Everything
  non-linear runs oversampled (1x to 8x, IIR or linear-phase FIR half-bands) with antiderivative anti-aliasing.
  Harmonics can work on the transients, the body, or both.
- **Sub**: one and two octaves down from a tracked flip-flop divider (clean to gritty), psychoacoustic "Phantom"
  bass (harmonics 2 to 4) with a fundamental cut, and mono below a set frequency.
- **Resonance**: resonators struck by the transients. Tuned (chords), Body (wood, skin, metal, glass and tube
  materials) or Comb, with the pitch fixed, on the song's key, from MIDI notes, or tracked from the bass.
- **Afterglow**: a send into the Chromosphere reverb (EXO's FDN engine) from the resonance, the transients or the
  whole signal.
- **Output**: auto-gain (K-weighted loudness match), mix, output level, clip guard, and listen modes (delta,
  transients, body, each band, sub, resonance, Afterglow).
- **Four macros with one meaning in every preset**: VELOCITY (the arrival), MASS (weight), CHARGE (energy) and DECAY
  (what is left). Right-click any dial to assign it to a macro.
- **122 factory presets** in 12 categories (Kick, Snare & Clap, Toms & Perc, Hats & Cymbals, Drum Bus, Room &
  Overheads, Bass & 808, Guitar, Vocal, Keys & Synth, Mix Bus / Master, Sound Design), each written individually in
  `tools/make_presets.py`. Each was rendered on matching test material and its level calibrated to within 1 dB of the
  input. None has been auditioned by ear yet.
- **Ignite** builds a setting from rules for a source: kick, snare, bass, vocal, bus or weird.
- **Interface**: the family console in Cherenkov blue, with a live blue sun and an amber CRT scope. The scope shows a
  track view of input and output, with the transients burning hotter and a streak for each onset, an analyser with
  draggable crossover points, and meters. User presets, A/B, undo and redo. Any window size from 1000 x 680 to
  1900 x 1300.

## Build

MUON builds with the rest of the repository; the DSP needs no JUCE:

    cmake -S . -B build && cmake --build build -j --target muon_tests muon_bench
    build/muon/muon_tests              # the PRD Section 8 gates (39 checks)
    build/muon/muon_bench              # CPU, including the silent (asleep) case

    cmake -S . -B build-plugin -DFR_BUILD_PLUGIN=ON [-DFR_JUCE_DIR=/path/to/JUCE]
    cmake --build build-plugin -j --target ChromosphereMuon_VST3 muon_plugin_smoke muon_preset_render muon_editor_snapshot
    build-plugin/muon/muon_plugin_smoke             # parameters, latency, macros, state, Ignite, buses, MIDI, fuzzing
    build-plugin/muon/muon_preset_render            # every factory preset on its material: ids, peaks, level
    xvfb-run -a build-plugin/muon/muon_editor_snapshot out/    # PNGs of the editor
    build-plugin/muon/muon_editor_cpu 15            # editor CPU, idle and playing (Linux; see below)

Presets: edit `tools/make_presets.py` and run it to rewrite `data/presets.json`. To recalibrate levels, run
`muon_preset_render --measure measured.json`, then `python3 tools/make_presets.py --calibrate measured.json`, rebuild,
and repeat two or three times. `tools/make_blue_assets.py` derives the blue images and icons from EXO's originals.

The editor code shared with INFRA (look and feel, sun, wordmark, readouts, VFD, controls, updater) lives in
`../ui/`. Each product installs its own theme (`plugin/MuonTheme.h`).

## Performance

Measured on a 2.3 GHz cloud Xeon VM; a desktop CPU is roughly twice as fast:

| Case | CPU (one core) |
|---|---|
| Three bands, every stage on, 4x | 4.4% |
| Same at 2x | 3.3% |
| Shaping only, three bands | 0.7% |
| Silent input (the engine sleeps once its tails end) | 0.07% |
| Editor open, idle (Xvfb, software rendering) | 16% total, 7.7% on the message thread |
| Editor open, audio playing | 38% total, 17% on the message thread |

The PRD's target for the first line is 3% on a desktop CPU. The oversampled harmonics are most of the cost.
`muon_editor_cpu` opens a real window, which needs the EWMH window-manager atoms that bare Xvfb lacks: run it under a
window manager, or start `tools/x11_atoms.py` on the display first (as the CI job does).

## Windows: install, update, release

- **Download:** `ChromosphereMuon-Setup.exe` from the repository's **muon-nightly** release. It installs the VST3
  plug-in to `C:\Program Files\Common Files\VST3`. The zip next to it holds the same files for manual installation.
  There is no standalone app: MUON is an insert effect.
- **Updates:** the same mechanism as INFRA: one daily check of `latest.json` per process, under a cross-process lock.
  The Updates button lights up when a new version is out, then Install now. The SHA-256 is verified, and the
  installer runs silently once the DAW has closed.
- **Releasing:** every push runs CI. The Windows job builds and tests MUON, runs pluginval at strictness 10 and builds
  the Inno Setup installer (`installer/ChromosphereMuon.iss`). The `release-muon` job then republishes muon-nightly
  with a fresh `latest.json`.
- **Not code-signed:** Windows SmartScreen warns the first time (More info > Run anyway).

## Status

- Linux: the VST3 builds, all tests pass, and pluginval passes at strictness 10; the editor was checked from
  snapshots.
- Windows (MSVC, installer) and macOS (AU): built by CI, not yet run on a machine here.
