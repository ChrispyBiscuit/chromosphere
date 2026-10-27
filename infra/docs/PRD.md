# PRD: Chromosphere | INFRA (polyphonic synthesizer, VST3 / AU)

Status: Draft 1, implemented (M1 to M5 done; M6 CI set up, first Windows and macOS runs pending). Personal project, same scope rules as the Chromosphere reverb PRD.
Product name: **Chromosphere | INFRA** (bundle and file name `ChromosphereInfra`, since `|` is not allowed in file names)
Date: 2026-10-26

---

## 1. Summary

INFRA is the instrument in the Chromosphere family. EXO is the reverb, the sun seen in visible light. INFRA is the
same star seen in infrared: deeper, redder, hotter underneath. It is a polyphonic wavetable and virtual-analog
synthesizer with a step arpeggiator, a modulation matrix, four universal macros, and an effects rack that ends in
the Chromosphere reverb itself, the same FDN engine EXO uses.

It ships as VST3 (Windows, macOS, Linux) and AU (macOS). It shares EXO's design language: a near-black matte
faceplate, raised instrument bays with bolts, recessed scope displays, Barlow small capitals, knurled dials with lit
arcs, and a live, shaded sun that reacts to the signal. The accent moves from solar orange to **infrared red**.

## 2. Goals and non-goals

### Goals
- G1. A distinctive, deep sound: a rich oscillator section (wavetables with space-themed spectra, unison, phase warps,
  FM between oscillators), a characterful filter section, and effects good enough to finish a patch inside the synth.
- G2. At least **100 factory presets**, designed one by one (not parameter jitter), covering bass, lead, pad, keys,
  pluck, bell, brass/poly, strings, arp, sequence, drone, texture and FX. Every preset uses the four macros
  consistently. (Shipped: 297, including a 25-preset Strings category.)
- G3. A modern arpeggiator: tempo-synced, 11 modes, up to 4 octaves, swing, latch, and a 16-step lane for
  velocity (rests), transpose and ratchets, with a step probability control.
- G4. Real-time safe: no allocation, locks, or file access on the audio thread. Sample-accurate MIDI.
- G5. Reasonable CPU: 16 voices with 2 oscillators x 4 unison voices, all effects on, under 15% of one core at
  48 kHz on a current desktop CPU (measured by `infra_bench`).
- G6. The EXO look, recoloured, at any window size from 1000 x 680 up to 1900 x 1300.
- G7. Everything checkable headless: DSP tests, a render of every factory preset (finite, not silent, no clipping),
  and editor snapshots as PNG.

### Non-goals (v1)
- MPE, microtuning, and MIDI learn (the MIDI path is written so these can be added).
- User-importable wavetables (the tables are generated; import is a v1.1 item).
- Oversampling (the drive stage uses a gentle curve and the oscillators are band-limited instead).
- AAX (same constraints as EXO: SDK and signing, see `docs/AAX.md`).

## 3. Users

| User | Needs |
|---|---|
| Producer | Big, finished sounds from presets; macros that do something useful on every patch |
| Sound designer | Wavetable morphing, a deep mod matrix, FM, noise colours, extreme effects |
| Live player | Arp with latch, mod wheel and aftertouch routed in every preset, low CPU |

## 4. Sound engine

### 4.1 Signal flow (per voice, stereo)
```
OSC A (wavetable, unison) ─┐
OSC B (wavetable, unison) ─┤                 ┌────────── AMP ENV ──────────┐
SUB (sine/tri/square) ─────┼─► mix ─► FILTER ─► VCA ─► pan ─► voice sum ─► FX RACK ─► out
NOISE (4 colours) ─────────┘     ▲ drive, key track, env, velocity
```
Modulation runs at control rate (every 16 samples); oscillators, filter and amp run per sample.

### 4.2 Oscillators A and B
Each oscillator plays one of ten generated **spectral tables**. A table has 16 frames; the **Morph** control scans
through them (smoothly interpolated). Every frame is stored band-limited at 11 octave levels (mipmaps), with a
crossfade between levels, so the oscillators do not alias at any pitch.

| # | Table | What the 16 frames do |
|---|---|---|
| 1 | Classic | sine, triangle, saw, square, narrowing pulse, the analog shapes in one sweep |
| 2 | Fraunhofer | a bright saw spectrum crossed by dark absorption lines that drift through the harmonics |
| 3 | Pulsar | pulsar synthesis: a short burst in a period of silence, the duty cycle shrinking frame by frame |
| 4 | Nebula | vocal formants sweeping A, E, I, O, U, over a buzzy source |
| 5 | Ion Drive | drawbar organ sets, from flute to full registration |
| 6 | Magnetar | two-operator FM, modulation index rising from 0 to 8 (ratio 1:2, then 1:3) |
| 7 | Plasma | hard-sync sweep: a slave saw sweeping from 1x to 8x the master, captured as clean spectra |
| 8 | Event Horizon | a sine folded by a wavefolder with increasing gain |
| 9 | Dark Matter | seeded random spectra with a soft tilt, each frame a different "planet" |
| 10 | Quasar | sparse, high, bell-like partial sets (1, 3, 7, 11 ...) that gain energy as they move |

Per-oscillator controls: Table, Morph, Octave (-3..+3), Semitone (-12..+12), Fine (-100..+100 cents), Level, Pan,
Unison (1..8 voices), Detune (0..100 cents spread), Width (stereo spread of the unison voices),
Warp mode and Warp amount, and Phase (0..360; random below 0 means free running).

Warp modes: **Off**; **Bend** (phase distortion, pushes energy up the spectrum); **Mirror** (reflects the cycle
around its centre); **Quantize** (phase stepping, a crystalline lo-fi edge); **FM from B** (OSC A only: OSC B
phase-modulates OSC A); **Ring by B** (OSC A only: ring modulation).

### 4.3 Sub and noise
- Sub: sine, triangle or square, one or two octaves below OSC A, level.
- Noise: White, Pink, **Solar Wind** (brown noise through a slow band-pass sweep), **Crackle** (sparse impulses
  like radio static), level. Noise passes through the filter like the oscillators.

### 4.4 Filter
Types: **LP 24** (ZDF ladder with saturation in the loop), **LP 12**, **BP 12**, **HP 12**, **HP 24**, **Notch**,
**Comb** (tuned, resonant, the frequency follows cutoff), **Formant** (vowel morph controlled by cutoff).
Controls: Cutoff (20 Hz .. 20 kHz), Resonance, Drive (pre-filter saturation), Envelope amount (-72..+72 semitones),
Key track (0..100%), Velocity amount.

### 4.5 Envelopes and LFOs
- Amp envelope, Filter envelope, Mod envelope: ADSR, attack 0..20 s, decay 1 ms..20 s, release 1 ms..20 s,
  exponential segments, plus a Curve control on the amp envelope.
- LFO 1 and LFO 2: Sine, Triangle, Saw up, Saw down, Square, Sample & Hold, **Drift** (smoothed random walk).
  Rate in Hz or tempo-synced (8 bars .. 1/32, dotted and triplet). Per-voice retrigger or free running. Fade-in.

### 4.6 Modulation matrix and macros
(Added during the build: LFO 1 Amount and LFO 2 Amount destinations, so the mod wheel can bring in vibrato.)

Eight slots: Source, Destination, Amount (bipolar).

Sources: LFO 1, LFO 2, Mod Env, Filter Env, Amp Env, Velocity, Key, Mod Wheel, Aftertouch, Pitch Bend,
Macro 1 to 4, Random (per note), Arp Step (the current step's level), Drift (slow per-voice random).

Destinations: Pitch (both), Pitch A, Pitch B, Morph A, Morph B, Warp A, Warp B, Level A, Level B, Sub level,
Noise level, Detune (both), Cutoff, Resonance, Filter drive, Amp, Pan, LFO 1 rate, LFO 2 rate.

**The four macros** have fixed names and a fixed meaning in every factory preset, so a player can reach for them
blind:
| Macro | Meaning |
|---|---|
| **HEAT** | brightness and energy: cutoff, drive, warp, FM |
| **GRAVITY** | weight and pull: sub, envelope depth, low end, glide |
| **ORBIT** | motion: LFO depth and rate, morph sweeps, unison detune |
| **FLUX** | space and aftermath: effect sends, delay feedback, reverb size and mix |
Macros are mod sources (so they can drive any destination in the matrix) and also drive the effect rack directly
through the FLUX and HEAT effect amounts (Section 4.9).

### 4.7 Voices
- Poly (1..16 voices), Mono, Legato. Voice stealing: released voices first, then the oldest.
- Glide (0..2 s), always or legato only. Pitch bend range (0..24 semitones). Velocity sensitivity.
- **Analog drift**: a per-voice slow random offset on pitch and cutoff, 0..100%.
- Sustain pedal (CC 64), mod wheel (CC 1), channel aftertouch, pitch bend.

### 4.8 Arpeggiator ("ORBIT ARP")
- Modes: Up, Down, Up/Down, Down/Up, As Played, Random, Converge, Diverge, Chord, **Orbit** (alternates the outer
  notes, closing in then opening out), **Pulsar** (the lowest note as a pedal between every other note).
- Rate: 1/1 .. 1/32 with dotted and triplet values, synced to the host tempo (or an internal tempo when the host
  has no transport). Octaves 1..4. Gate 5..200% (above 100% notes overlap, for legato patches). Swing 0..75%.
- Latch (hold the chord after the keys are released).
- 16-step lane, length 1..16. Per step: **Velocity** (0 = rest), **Transpose** (-12..+12 semitones),
  **Ratchet** (1..4 repeats within the step). **Probability** (0..100%) decides whether each step plays.
- The current step's velocity is the **Arp Step** mod source, so steps can open the filter or move the wavetable.

### 4.9 Effects rack (fixed order, all stereo)
1. **Drive**: Soft, Tube (asymmetric), Fold, **Cosmic Ray** (bit and rate reduction). Amount, tone, mix.
2. **Aurora** chorus / ensemble: three modulated delays with 120 degrees between them. Rate, depth, mix.
3. **Echo** delay: tempo-synced or free, ping-pong, feedback, damping (low-pass in the loop), mix.
4. **Chromosphere reverb**: the FDN engine from EXO (algorithmic mode) through the same thread-safe engine host.
   Size, decay, damping (high-frequency decay), pre-delay, mix.
5. **Output**: stereo width, level, and a transparent peak limiter so no preset can clip the host.

## 5. Presets
- At least 100 factory presets in 12 categories, stored as JSON (`infra/data/presets.json`) generated by
  `infra/tools/make_presets.py`. Each preset has a name, category, tags, and a one-line description.
- Every preset sets the macro assignments (Section 4.6) and routes the mod wheel and aftertouch to something
  musical.
- User presets: one XML file each in the user application-data folder (`ChromosphereInfra/Presets`).
- Browser: search by name, category or tag. Previous / next, A/B compare, undo / redo.
- **Ignite**: a randomizer that builds a new patch from musical rules (it picks a category archetype, then
  varies tables, filter, envelopes and modulation within ranges that suit it), never a plain random fill.

## 6. Interface

Same construction as EXO, in infrared red. It follows EXO's later design language: seven-segment LED readouts,
lever switches, a VFD preset selector and VFD combos, an amber CRT scope with halation and scanlines, and scored
dividers between dials.
- Header: the live sun emblem, the wordmark **CHROMOSPHERE | INFRA** (lens, aberration and the breathing cursor),
  a status line (voices in use, tempo, sample rate), the preset bar, and the Ignite button.
- Main display (recessed scope over the rising red limb of the sun): the OSC A and OSC B wavetables as stacked
  frames with the current morph frame lit, the filter response, the output oscilloscope and level meters.
  The sun reacts to output level (energy) and bass (sunspots), as in EXO.
- Macro bay: the four macros as large dials beside the display, always visible.
- Pages in the main plate: **SYNTH** (oscillators, sub and noise, filter, amp and filter envelopes), **MOD**
  (LFOs, mod envelope, matrix, voice settings), **ARP** (controls and the 16-step lane editor with live step
  lights), **FX** (the rack).
- Section codes (01, 02 ...), small capitals, lamps on toggles, readout windows under every dial.

## 7. Architecture

```
infra/
  dsp/       JUCE-free engine: tables, oscillator, filters, envelopes, LFOs, voice, arp, effects, synth
  plugin/    JUCE processor (parameters -> engine), editor, look and feel, sun, wordmark, presets
  tools/     make_presets.py (factory presets), infra_render (headless render), infra_bench
  tests/     DSP tests, preset render check, editor snapshot
  data/      presets.json
  assets/    fonts (Barlow, OFL), red solar images
```
- The engine (`infra::Synth`) takes a block of audio and a list of timestamped MIDI events. It never allocates
  after `prepare()`. Parameters arrive as a plain struct, copied once per block.
- The reverb stage uses `fr::EngineHost` from the reverb project unchanged (worker-thread rebuilds, cross-fades).
- Wavetables are generated once per process (pffft inverse FFTs), shared read-only by all instances.

## 8. Quality and test gates
- Oscillators: aliasing products of a saw at 5 kHz (48 kHz) at least 60 dB below the fundamental.
- Every factory preset, played with a fixed phrase (chord, single notes, held arp): output finite, peak below
  -0.1 dBFS (after the limiter), RMS above -60 dBFS, no DC offset above 1%.
- Arp: note timing exact to the sample at 120 BPM for all rates; ratchets and swing land where computed.
- Voice stealing never leaves stuck notes (all notes released, after release time, output decays to silence).
- Engine handles any block size from 1 to 4096 and sample rates from 44.1 to 192 kHz.

## 9. Milestones
1. M1: PRD (this document).
2. M2: DSP engine and tests.
3. M3: Plugin processor and parameters, smoke test.
4. M4: Editor in the EXO language, recoloured red, with the live displays.
5. M5: 100+ factory presets, render check of every preset, Ignite.
6. M6: CI (Windows, macOS, Linux), README.

## 10. Risks
| Risk | Mitigation |
|---|---|
| CPU with high unison and many voices | control-rate modulation, mipmapped tables, voice count limit, bench gate |
| Presets designed without listening | objective render checks (level, spectrum centroid, decay) and conservative gain staging; limiter on the output |
| Reverb engine thread per instance | `EngineHost` already used in EXO; the worker sleeps between polls |
| Wavetable memory | 10 tables x 16 frames x 11 levels x 2048 floats is about 14 MB, built once per process |
