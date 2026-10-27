# INFRA against the optimization guide

How Chromosphere | INFRA stands against the suite's optimization guide (`docs/OPTIMIZATION_GUIDE.md` on the EXO
branch, `prd-draft-2`). Every number was measured on Linux with the software renderer, as the guide does, at
48 kHz with 256-sample blocks. One core = 100%.

## Numbers

| Item | Before | After | Guide target |
|---|---|---|---|
| Editor open, idle | 43% | **13%** | < 10% |
| One silent instance | 1.2–1.9% | **0.016–0.038%** | < 0.1% |
| One instance playing a four-note chord, mean of 10 presets | 2.2% | 2.3% | < 1.5% "for a typical mode" |
| Editor closed | adds nothing | adds nothing | adds nothing |
| Load an instance into a running session | ~25 ms | ~20 ms (construct 9–12, prepare 11–14) | no network or disk storms |

The guide's targets are for an effect. A synth playing a four-note chord, with unison stacks and all effects, does
more work by nature, so the "playing" target does not compare directly.

Most of the remaining editor cost is the Linux software renderer scaling and blending images onto the window.
Direct2D does that on the GPU on Windows.

How to reproduce:
- **Instance cost and load time:** `build/infra/infra_plugin_bench [seconds]`.
- **Editor cost:** run the standalone app under Xvfb and read its CPU time from `/proc/<pid>/stat` (fields 14 and
  15) over 20 s, after a few seconds' warm-up.

## Checklist

**Done:**
- [x] **Benchmarks:** DSP cost playing and silent (`infra_plugin_bench`), editor cost idle.
- [x] **Software images:** every per-pixel image is a software image, and none is made off the message thread. The
  sun frames, scope layers, wordmark, LED bloom and plate grain are software. Images that are only drawn are
  converted to native once.
- [x] **Static drawing is cached:**
  - the window background and plates;
  - each page's section headers and dividers;
  - the scope overlay (traces, halation, frame, scanlines), rebuilt only when what it shows changes;
  - the wordmark lens, rerun only for a glitch;
  - dial bodies and seven-segment readouts.
- [x] **No display-rate repaints:**
  - suns: 6 fps when quiet, 15–20 with signal;
  - wordmark cursor: 15 Hz, a small area;
  - editor timer: 15 Hz;
  - dot-matrix displays stop their timers when settled;
  - mod matrix rows and the header repaint only on change.
- [x] **Hidden or minimised windows:** all animation and render threads stop.
- [x] **OpenGL** is off; the CPU path is the default.
- [x] **Shared caches and updater:**
  - fonts and image caches are a `SharedResourcePointer<look::Assets>`, gone when the last editor closes;
  - there are no JUCE objects in function statics;
  - the parsed factory presets are shared;
  - the updater is one per process, and editors and the Updates panel listen with `ChangeListener`.
- [x] **Cross-process lock:** network checks, downloads and settings reads are guarded by
  `InterProcessLock("ChromosphereInfraUpdater")`, and the settings are reloaded before they are read.
- [x] **No heavy work at load:** the update check starts 6 s after load, once per process.
- [x] **Worker threads** wait on events or sleep.
- [x] **Sleep on silence,** with tests: it sleeps, a note wakes it on the same block, and the output is the same
  across a sleep for every block size.
- [x] **Feedback paths:** DC blockers per voice, no dither, `ScopedNoDenormals`.
- [x] **Control rate:** free-running LFOs and effect modulation run on the absolute timeline. The output is
  identical for block sizes 1, 7, 64, 256, 480 and 4096 (difference 0). Across a sleep it is within 3e-6.
- [x] **AVX2:** the DSP library is built with `/arch:AVX2` on Windows.

**Still to do:**
- [ ] **pluginval strictness 10:** runs in CI on every push; check the next run.
- [ ] **20-instance session in FL Studio:** needs a Windows machine.

## Bugs found on the way

While making the test suite strict, three delay lines turned out to read one sample past the end of their buffer:
the Aurora chorus, the echo and the voice comb filter. When a fractional read position fell a hair below zero,
wrapping it by the buffer length rounded to exactly the length in floating point.

The value read past the end was whatever memory followed the buffer, sometimes an enormous number. The output
limiter hid it as -1 dBFS of noise, but it filled the reverb, which carried it into the next sounds. These were
presets using chorus or the comb filter: the plucks, string ensembles and organs.

All three now keep the index inside the buffer. The render test now also fails any preset that needs more than
20 dB of limiting, so a runaway signal can no longer pass as "finite". Preset levels were recalibrated afterwards:
median -16.0 dB, spread 0.5 dB.
