# PRD: Chromosphere | MUON (transient and harmonics processor, VST3 / AU)

Status: Draft 2, in development (decisions of Section 12 taken 2026-10-27). Personal project, same scope rules as the Chromosphere reverb PRD.
Product name: **Chromosphere | MUON** (bundle and file name `ChromosphereMuon`, since `|` is not allowed in file names)
Date: 2026-10-27

---

## 1. Summary

MUON is the processor in the Chromosphere family that works on the *shape* and *colour* of a sound rather than its
space. EXO is the reverb, the sun in visible light. INFRA is the synth, the same star in infrared. MUON is what the
sun's cosmic rays make when they strike the upper atmosphere: a heavy, fast, penetrating particle that arrives first
and leaves a bright trail. In audio terms, the arrival is the **transient** and the trail is the **harmonics and
resonance** that follow it.

It is an insert effect with four stages:
1. **Split**: the signal is divided into up to three frequency bands, and each band into a **transient** stream and a
   **body** (sustain) stream, level-independently.
2. **Shape**: attack and sustain gain per band, applied to those streams (a transient designer).
3. **Harmonics**: generated upper harmonics with control of order (2nd to 8th, even/odd balance) and generated
   lower harmonics (sub-octaves and psychoacoustic bass), assignable to the transient stream, the body stream or both.
4. **Resonance**: a tuned bank of resonators (modal, comb and body) that the transients *strike*, so a hit can ring
   at a pitch, a chord or a material.

It ships as VST3 (Windows, macOS, Linux) and AU (macOS). There is no standalone app: MUON is an insert effect. It
shares the family's design language, with a new accent: **Cherenkov blue**, the light muons give off in a detector tank.
A fifth, optional stage, **Afterglow**, sends the resonance and transients into the Chromosphere reverb (Section 4.6).

## 2. Goals and non-goals

### Goals
- G1. A transient shaper that is **level-independent** (no threshold, like the SPL differential-envelope designs),
  works per band, and stays clean at extreme settings (no clicks, pumping or zipper noise).
- G2. Harmonic generation where the user chooses **which orders** are added (2nd, 3rd, 4th ... 8th, and sub-octaves),
  not only "more drive". Low aliasing at every setting (Section 8).
- G3. Resonators that turn transients into pitched or material-like rings (kick tuned to the song's key, snare body,
  metallic hits), with a key/note selector and optional MIDI control.
- G4. Zero reported latency in the default mode. Optional lookahead and linear-phase modes report their latency
  exactly.
- G5. Real-time safe: no allocation, locks or file access on the audio thread. Every parameter smoothed and
  automatable.
- G6. Low CPU: one stereo instance, three bands, all stages on, 4x oversampling, under 3% of one core at 48 kHz on
  a current desktop CPU (measured by `muon_bench`). Many instances on a drum bus must be practical.
- G7. **At least 120 factory presets**, designed individually, in the categories of Section 6. The four macros mean
  the same thing in every preset.
- G8. The family look, recoloured blue, at any window size from 1000 x 680 to 1900 x 1300.
- G9. Everything checkable headless: DSP tests with objective gates, a render of every factory preset against test
  material, and editor snapshots as PNG.

### Non-goals (v1)
- Spectral (STFT) transient/tonal separation like Eventide's Structural Split. It costs 20-50 ms of latency; v1 uses
  time-domain envelopes. Planned as an optional "HQ split" in v1.1 (Section 4.2.3).
- Polyphonic pitch tracking. The resonators follow a chosen key or MIDI notes, or a monophonic pitch tracker on the
  low band (Section 4.5).
- Sidechain input (keyed transient shaping). The bus layout reserves it for v1.1.
- Surround formats; v1 is mono and stereo (with mid/side).
- AAX (same constraints as EXO: SDK and signing, see `docs/AAX.md`).

## 3. Users

| User | Needs |
|---|---|
| Mixing engineer | More or less snap and sustain on drums, bass and guitars without setting a threshold; drum bus glue |
| Producer / beatmaker | Kicks tuned to the key, sub-octaves under 808s, harmonics that let bass translate on phones |
| Mastering (light use) | Gentle even-order warmth and a high-band exciter, linear-phase bands, exact latency |
| Sound designer | Resonators struck by transients, extreme harmonic orders, body-only or transient-only processing |

## 4. Processing

### 4.1 Signal flow (stereo, or mid/side)
```
            ┌─► BAND LOW ──► SPLIT ─┬─ transient ─► SHAPE(attack) ─┬─► HARMONICS(T) ─┐
in ─► trim ─┼─► BAND MID ──► SPLIT ─┤                               │                  ├─► band sum ─┐
            └─► BAND HIGH ─► SPLIT ─┴─ body ──────► SHAPE(sustain) ─┴─► HARMONICS(B) ─┘              │
                                                                                                       │
     transient sum (all bands) ──────────────────► RESONANCE (struck) ──────────────────────► + ◄─────┘
     low band body ──────────────────────────────► SUB (octave down / psychoacoustic) ──────► +
                                                                                                       │
                                                         clip guard ◄─ output / mix ◄─ auto-gain ◄────┘
```
- Band count 1, 2 or 3. With one band the crossover is skipped and the path is bit-identical to a direct path.
- Stereo modes: **Stereo** (linked detection, the default), **Dual** (independent L/R), **Mid/Side** (M and S get
  their own shape amounts; harmonics on M only by default, so the sub and low harmonics stay mono).
- Parameters are smoothed per sample (gains) or per 16 samples (filter coefficients), as in INFRA.

### 4.2 Split

#### 4.2.1 Crossover
- Default: **Linkwitz-Riley 4th order** (24 dB/oct), two crossover points (default 120 Hz and 2.5 kHz), zero latency.
  The bands sum to an all-pass, flat in magnitude.
- **Linear phase** option: FIR crossover built with pffft (already in the repo), latency reported to the host
  (about 10 ms at 48 kHz; Section 7.3). For mastering.

#### 4.2.2 Transient / body separation (time domain, default)
Per band, a **differential envelope** detector in the manner of SPL's Differential Envelope Technology:
- A **fast** envelope follower (attack about 0.1 ms) and a **slow** one (attack 5-50 ms, set by the *Speed* control)
  run on the band's rectified level, in the log domain. Their difference is the *attack signal*: large at an onset,
  zero on steady material, independent of the absolute level.
- The same pair with the roles of release swapped gives the *sustain signal* (positive while the sound rings longer
  than a fast release would allow).
- A **transient mask** m(t) in 0..1 is derived from the attack signal with a smooth curve (*Sensitivity* sets the
  knee). The transient stream is `m · x`, the body stream is `(1 - m) · x`, so they always sum back to the band.
- Time constants adapt to the band (the low band's are about four times slower than the high band's) so a kick's
  attack is not cut at its first cycle.
- **Lookahead** (0, 1, 2 or 5 ms, reported as latency): the mask is computed on the undelayed signal and applied to
  the delayed one, so boosted attacks reach full gain before the onset instead of during it.

#### 4.2.3 HQ split (v1.1, spectral)
An STFT harmonic/percussive separation (median filtering across time and frequency, after FitzGerald's method)
with soft masks, about 21-43 ms latency. It gives a cleaner split on dense mixes (a snare inside a sustained pad).
The engine's split interface is written so this drops in as a second implementation.

### 4.3 Shape (transient designer)
Per band:
- **Attack** -100..+100% (about -24..+15 dB at full scale): gain on the transient stream, driven by the attack
  signal.
- **Sustain** -100..+100% (about -24..+24 dB): gain on the body stream, driven by the sustain signal.
- **Speed**: the slow follower's time constants (how long a "transient" lasts: 5 ms snap to 50 ms thump).
- **Clip**: a soft clip on the transient stream after the attack gain (ADAA, Section 4.4.4), for the hard,
  shaved transient of a clipped drum bus. Amount 0..100%.
- Gain changes are applied through smoothed, bounded gain curves (no step faster than the fast follower), so
  extreme settings do not click.
- Global **Punch** (a single dial that scales all bands' attack together) and **Tail** (all bands' sustain), so the
  plugin is usable without opening the band view.

### 4.4 Harmonics

Each band has a harmonics section, fed by its **transient** stream, **body** stream, or **both** (selector, default
both). All nonlinear stages run oversampled (Section 4.4.4).

#### 4.4.1 Upper harmonics: the **order bank**
The distinctive control: eight sliders, **H2 to H8** plus **H1** (the dry fundamental level), each adding that order
of harmonic.
- Method: **envelope-normalised Chebyshev waveshaping**. A Chebyshev polynomial T_n turns a unit-amplitude sine into
  exactly its nth harmonic. Program material is not a unit sine, so each band is (a) divided by its own smoothed
  envelope to bring it near unit amplitude, (b) shaped by the weighted sum of T_2..T_8, (c) multiplied back by the
  envelope. On narrow-band material (a bass note in the low band, a vocal in the mid band) the added harmonics land
  at the chosen orders; on dense material the intermodulation is real but stays proportional, which is the musical
  result of any saturator. Bands keep the intermodulation local.
- **Phase** per order: 0 or 180 degrees (sign of the term), which changes the waveform's asymmetry and how it adds to
  the dry signal.
- **Presets for the bank** (one click): *Tube* (falling even orders), *Tape* (mostly 3rd, gentle 5th), *Transformer*
  (2nd and 3rd at low frequency only), *Square* (odd orders 1/n), *Saw* (all orders 1/n), *Octaver* (H2 only),
  *Bell* (H3, H5, H7 with H1 reduced).
- Output follows a **Harmonics** amount (0..100%, wet only), so the bank's shape and its quantity are separate.

#### 4.4.2 Character saturators
Alongside the order bank, a **Drive** with curve choices for users who want "a sound", not a recipe: **Soft**
(tanh), **Tube** (asymmetric, even-rich, with a slow bias shift that follows the envelope), **Tape** (soft
symmetric plus a gentle high-frequency loss with drive), **Diode** (hard knee), **Fold** (wavefolder, from INFRA's
drive) and **Rectify** (half and full wave, octave-up grit). Drive, Bias (asymmetry), Tone (tilt after the curve),
Mix.

#### 4.4.3 Lower harmonics: **SUB**
On the low band's body stream (or a dedicated band, 30-150 Hz, set by its own filter):
- **Octave down** and **two octaves down**: a tracked sub generator. A zero-crossing flip-flop on the band-limited
  signal (the classic octave-divider method, as in hardware sub-harmonic synths) drives a square, which is low-pass
  filtered to a near-sine and multiplied by the band's envelope. A **Purity** control blends from the square (gritty)
  to the filtered sine (clean). A hysteresis and a minimum period guard keep the divider from chattering on noise.
- **Psychoacoustic bass** ("Phantom"): generates H2-H4 of the low band and lets the user cut the fundamental, so bass
  is heard on small speakers that cannot reproduce it (the missing-fundamental effect, as in MaxxBass-style
  processors). Uses the order bank engine with a fixed recipe.
- Sub level, Phantom level, low-pass for the generated signal, and **Mono below** (default 120 Hz).

#### 4.4.4 Anti-aliasing
- **Oversampling**: 1x, 2x, 4x (default), 8x, with polyphase half-band IIR filters (minimum phase, so zero added
  latency beyond a few samples of group delay, which is reported) in the default mode and linear-phase FIR half-bands
  in the linear-phase mode. Only the nonlinear stages run at the high rate; the split and the gains run at 1x.
- **Antiderivative anti-aliasing (ADAA)**, first order, on every memoryless curve (tanh, the clip, the diode, the
  folder), after Parker et al. (DAFx 2016) and Bilbao, Esqueda, Parker and Välimäki (IEEE SPL 2017). ADAA plus 2x
  costs less than 8x alone for a comparable alias level.
- Chebyshev polynomials of order n produce content up to n times the input bandwidth, so the order bank's highest
  order is limited per band by the oversampling factor and the band's upper edge (the UI greys out orders that would
  alias, with a tooltip).
- A DC blocker after every asymmetric stage.

### 4.5 Resonance
A bank of resonators fed by the **transient sum** (or the full signal, selector), mixed into the output. The
transients "strike" it.
- **Modes**:
  - **Tuned**: up to 8 two-pole resonators (bandpass, constant peak gain) at a **Root** note and a **Chord** shape
    (Unison, Octaves, Fifths, Major, Minor, Sus, Harmonic series), with Spread (detune) and Stereo (alternating
    pan).
  - **Body**: a modal bank of 16 modes with the frequency ratios of a material (**Wood**, **Skin** (membrane, Bessel
    ratios), **Metal** (plate), **Glass**, **Tube** (cylinder, odd harmonics)), scaled by **Size**.
  - **Comb**: a feedback comb (flanged, metallic, or Karplus-Strong-like with a damping filter in the loop).
- Controls: **Decay** (10 ms .. 4 s, T60), **Damping** (high-frequency decay faster than low), **Tone**,
  **Strike** (how much of the transient excites the bank), **Mix**.
- **Pitch source**: Fixed (Root and Chord), **Key** (the song's key and scale; resonators snap to scale degrees),
  **MIDI** (incoming notes retune the bank; the plugin accepts MIDI input for this only), or **Track** (a monophonic
  YIN pitch tracker on the low band: the bank follows a bass or a tuned kick).
- Every resonator has a soft limiter on its state so feedback-like settings cannot run away, and the bank is gated
  so it decays to true silence (denormal-safe, using `dsp/common/denormal.h`).

### 4.6 Afterglow (reverb send)
A send into the Chromosphere FDN reverb (EXO's algorithmic engine, through `fr::EngineHost`, as INFRA's rack does),
so a struck resonance can bloom into a space.
- **Source**: Resonance (default), Transients, or Full (the processed signal).
- Controls: **Send**, **Size**, **Decay**, **Damping**, **Pre-delay**, **Mix** (the return level).
- The return is added after auto-gain, before the clip guard. With Send at zero the reverb host is not run, so the
  stage costs nothing.
- The reverb runs at 1x and adds no reported latency (EXO's algorithmic mode is zero-latency).

### 4.7 Output
- **Auto-gain** (on by default): a slow loudness match (momentary-loudness style, about 400 ms window, gain change
  limited to ±12 dB) so louder never wins a comparison by accident. The applied gain is shown.
- **Mix** (dry/wet, latency-compensated dry), **Output** level, **Clip guard** (a transparent peak limiter, as in
  INFRA, so no setting can clip the host; off by default for mastering, on in the drum presets).
- **Delta** listen: outputs only what MUON adds or removes (wet minus dry), per stage or overall.
- **Solo** buttons per band and per stream (transient / body / harmonics / sub / resonance).

### 4.8 Macros
Four macros with fixed names and a fixed meaning in every factory preset, named for a muon's properties:

| Macro | Meaning |
|---|---|
| **VELOCITY** | the arrival: attack amounts, transient clip, lookahead feel (Punch) |
| **MASS** | weight: low-band sustain, sub and Phantom levels, low-order (H2, H3) harmonics |
| **CHARGE** | energy and brightness: high orders (H4-H8), drive, high-band attack, exciter tone |
| **DECAY** | what is left behind: sustain amounts, resonator decay and mix, Afterglow send and decay |

Each macro drives up to eight targets with a range per target (an INFRA-style assignment list per preset), so the
same macro can mean "a little more" on a vocal and "much more" on a kick.

### 4.9 Parameter design rules (from EXO's PRD, Section 5.5)
- Every control is usable from 0 to full scale; no dead zones. Neutral values sit at a detent (12 o'clock for
  bipolar controls) and are exactly transparent.
- With all shapes at 0, harmonics at 0, sub, resonance, Afterglow and auto-gain off, the output equals the input (to within
  -120 dB for the minimum-phase crossover summed to an all-pass; bit-exact with one band).
- Units shown in readouts: dB, %, ms, Hz, note names for the resonance root.

## 5. Interface

Same construction as EXO and INFRA (see EXO's `docs/DESIGN_SPEC.md` and INFRA's `plugin/InfraLook.*`), in
**Cherenkov blue** (accent about `#3fa9ff`, at rest `#1d4f7a`; the amber CRT, the red seven-segment readouts and the
cyan VFD stay as in the family). Proposed default size 1180 x 760.
- Header: the live sun emblem, the wordmark **CHROMOSPHERE | MUON**, a status line (bands, oversampling, latency in
  ms, sample rate), the preset bar (VFD), A/B, undo/redo, Updates.
- **Main display** (the amber CRT scope over the sun's limb), three panes:
  1. **Track view**: a scrolling waveform (about 3 s) with the transient stream drawn bright and the body stream dim,
     and the applied attack/sustain gain as a curve over it. Each detected onset leaves a short blue streak like a
     particle track in a cloud chamber, fading over half a second.
  2. **Spectrum**: the output harmonic content of the selected band as bars at H1..H8 for the tracked pitch (when the
     tracker has a pitch), otherwise an FFT analyser with the crossover points as draggable handles.
  3. **Meters**: In, Out, auto-gain, and resonator energy.
  The sun reacts to transients (flares on onsets) and to the low band (sunspots), as in EXO.
- **Macro bay**: the four macros, always visible, with Punch and Tail beside them.
- Pages in the main plate: **SHAPE** (bands side by side: attack, sustain, speed, clip; solo; split mode),
  **HARMONICS** (per band: the order bank as eight vertical faders, the drive section, the source selector; the SUB
  section), **RESONANCE** (mode, root and chord or material, pitch source, decay, damping, strike, mix, with a
  keyboard strip showing the tuned notes; the AFTERGLOW send beside it), **OUTPUT** (mix, auto-gain, clip guard, stereo mode, oversampling,
  lookahead, crossover type, delta).
- Section codes (01, 02 ...), small capitals, lamps on toggles, seven-segment readouts under every dial, lever
  switches.
- Rendering rules from INFRA's `docs/RCA-ui-performance.md` apply from the first commit: static plates cached per
  size, one repaint source per animated area, no pixel access to GPU images, no GPU images created off the UI thread
  or held in statics, the sun on its own low-priority thread. The track view is drawn from a lock-free FIFO of
  per-block summaries (min/max, mask, gain), never from raw audio.

## 6. Presets
- At least 120 factory presets, JSON (`muon/data/presets.json`) generated by `muon/tools/make_presets.py`, with name,
  category, tags, one-line description, and the macro assignments. Validated by a `check_presets.py` as INFRA does.
- Categories: **Kick**, **Snare & Clap**, **Toms & Perc**, **Hats & Cymbals**, **Drum Bus**, **Room & Overheads**,
  **Bass & 808**, **Guitar**, **Vocal**, **Keys & Synth**, **Mix Bus / Master**, **Sound Design** (resonator-led:
  tuned hits, metallic, glass, struck bodies).
- Levels: every preset is rendered on matching test material and calibrated so the auto-gain-off output is within
  1 dB of the input loudness (INFRA's measure-and-calibrate loop).
- User presets: one XML file each in `ChromosphereMuon/Presets` in the user application-data folder. Browser with
  search, previous/next, A/B, undo/redo.
- **Ignite** (as in INFRA): builds a setting from rules for a chosen source (Kick, Snare, Bass, Vocal, Bus, Weird),
  never a plain random fill.

## 7. Architecture

### 7.1 Layout
```
muon/
  dsp/muon/   JUCE-free engine: crossover, envelope/split, shaper, order bank, saturators, oversampler, sub,
              resonators, pitch tracker, auto-gain, limiter, engine
  plugin/     JUCE processor (parameters -> engine), editor, look (shared with INFRA where possible), presets
  tools/      make_presets.py, check_presets.py, muon_render (headless), muon_bench, muon_metrics.py
  tests/      DSP tests, preset render check, plugin smoke, editor snapshot
  data/       presets.json, test material (short drum, bass and vocal loops, generated or licensed CC0)
  assets/     fonts (Barlow, OFL), blue solar images
  installer/  Inno Setup script and icons (as INFRA)
```
- Built from the top-level `CMakeLists.txt` via `add_subdirectory(muon)`, like INFRA. Library `muondsp`, links
  `frhost` (the reverb engine host, for Afterglow, and through it `frdsp` and `pffft`).
- The engine (`muon::Engine`) takes a block of audio (and, for the resonator, a list of timestamped MIDI notes). It
  never allocates after `prepare()`. Parameters arrive as a plain struct, copied once per block, as in INFRA.
- Shared UI code: the look-and-feel, sun, wordmark, VFD and seven-segment readouts currently live in `infra/plugin/`
  and EXO's `plugin/`. MUON is the third copy, so v1 starts by moving them into a shared `ui/` library
  (`chromosphere_ui`) parameterised by the accent colour. This also lands the RCA fixes once for all three products.
- JUCE `AudioProcessorValueTreeState` for parameters; plugin code `Cmuo`, manufacturer `Chrm`, VST3 category
  `Fx|Dynamics|Distortion`, AU type `aufx`. MIDI input enabled (resonator pitch only); `IS_SYNTH FALSE`.

### 7.2 Real-time rules
Same as EXO's PRD Section 8.2: no allocation, locks, file or network access, logging or system calls on the audio
thread; FTZ/DAZ on; every buffer sized in `prepare()` for the maximum block size and oversampling factor.

### 7.3 Latency
| Mode | Reported latency at 48 kHz |
|---|---|
| Default (min-phase crossover, IIR oversampling, no lookahead) | 0 samples (group delay of the half-bands is left unreported, < 0.2 ms, as is usual for IIR oversamplers) |
| Lookahead 1 / 2 / 5 ms | 48 / 96 / 240 samples |
| Linear-phase crossover and oversampling | about 480 samples (exact value reported) |
Latency changes only on a parameter that the host is told needs a restart notification; the dry path in Mix is
always delay-matched.

### 7.4 Engine stage order and CPU plan
- Split, shape and gains at 1x, SIMD across the two channels (and across bands where possible).
- Only harmonics run oversampled, and only when their amount is above zero (stages with zero mix are skipped, with a
  short cross-fade when they turn on or off).
- Resonators run at 1x; 8 or 16 two-pole sections are cheap. The pitch tracker runs on the low band decimated by 8.

## 8. Quality and test gates (objective, run headless)
- **Transparency**: neutral settings, one band: bit-exact. Three bands, minimum phase: magnitude flat within
  ±0.01 dB 20 Hz-20 kHz. Linear phase: null against the delayed input below -100 dB.
- **Level independence**: a test drum loop at -6 dBFS and at -36 dBFS, Attack +50%: the change in crest factor
  matches within 0.5 dB.
- **Shaper behaviour**: on a synthetic decaying-tone burst, Attack +100% raises the first 5 ms by at least 9 dB and
  leaves the level after 100 ms within 0.5 dB; Sustain +100% raises the level at 200 ms by at least 9 dB. No sample
  step larger than the input's own maximum step plus 1 dB at any setting (click test).
- **Order bank**: a 200 Hz sine at -12 dBFS with only H_n at full: H_n within ±1 dB of its target level; all other
  harmonics at least 40 dB below it (n = 2..8).
- **Aliasing**: a 5 kHz sine at 0 dBFS through Drive Soft at full, 4x oversampling: every non-harmonic component
  below -90 dB relative to the fundamental; at 1x with ADAA, below -60 dB. Order bank at 4x: same -90 dB gate for
  every order the UI allows.
- **Sub**: a 60 Hz to 120 Hz bass line: octave-down output at the right pitch (within 1 cent) after 2 periods, no
  octave errors over the test file, decays to silence (-120 dB) within 50 ms after the input stops.
- **Resonance**: Tuned mode at A2, struck by an impulse: the strongest peak within 1 cent of 110 Hz, T60 within 10%
  of the Decay setting; any setting decays to silence; no NaN or Inf under hostile input (full-scale noise, DC,
  impulses at Nyquist).
- **Auto-gain**: on pink noise and the test loops, output loudness within 1 LU of the input for every factory preset.
- **Engine**: any block size 1..4096, sample rates 44.1..192 kHz, results independent of block size (to -120 dB).
- **Presets**: every factory preset rendered on its category's test material: finite, peak below -0.1 dBFS with clip
  guard on, within 1 dB of calibrated loudness.
- **Plugin**: pluginval strictness 10 on Windows, macOS and Linux in CI; state save/restore round-trip; latency
  reported equals measured latency (impulse test) in every mode.

## 9. Research notes (how this gets done)
- **Transient shaping without a threshold**: SPL's Differential Envelope Technology subtracts a slow-attack envelope
  from a fast one, which is zero on steady material at any level; the same idea in the sustain path. MUON's split
  (4.2.2) is this, in the log domain, per band, with the mask making the two streams sum back to the input.
- **Transient/tonal split as a routing tool**: Eventide's Physion popularised separate effect chains on the
  transient and tonal parts of a signal (its Structural Split is spectral and higher-latency). MUON gives that
  routing at zero latency in v1, and the spectral version as an option in v1.1.
- **Targeted harmonic orders**: Chebyshev polynomials give exact orders for a unit sine only; for program material
  the input needs amplitude normalisation and band limiting, which MUON's bands and envelopes provide. Prior art:
  HoRNet Harmonics (ten Chebyshev generators with level and phase).
- **Aliasing**: first-order ADAA (Parker et al. 2016; Bilbao et al. 2017, higher orders) plus modest oversampling.
- **Sub-octaves**: flip-flop octave division on a low-passed band (the dbx Boom Box / sub-harmonic synthesizer
  method), envelope-following, with a purity filter.
- **Psychoacoustic bass**: generate harmonics 2-4 of the bass band; the ear infers the fundamental.
- **Resonators**: two-pole resonators and modal banks with material ratios (as in physical-modelling percussion),
  excited by the transient stream rather than by MIDI.
- Reused from the repo: pffft (linear-phase FIRs, analyser), `dsp/common` (denormals, SIMD, RNG), INFRA's drive
  curves, limiter and DC blocker (`infra/dsp/infra/fx.*`), the preset tooling pattern, the updater, installer and CI
  jobs.

## 10. Milestones
1. M1: PRD (this document).
2. M2: Shared `chromosphere_ui` library extracted from INFRA and EXO; both still build and their snapshots match.
3. M3: DSP engine (split, shape, order bank, saturators, oversampling with ADAA, sub, resonators, auto-gain) and the
   gates of Section 8; `muon_bench`.
4. M4: Plugin processor, parameters, latency reporting, MIDI for the resonator, smoke test, pluginval.
5. M5: Editor in the family language, blue, with the track view, spectrum and meters.
6. M6: 120+ factory presets, calibration loop, render check, Ignite.
7. M7: CI (Windows installer with the VST3, macOS AU, Linux), `muon-nightly` release with updater manifest, README.

## 11. Risks
| Risk | Mitigation |
|---|---|
| Afterglow adds a reverb thread per instance (many instances on a drum kit) | the host is only prepared while Send is above zero; the worker sleeps between polls |
| Time-domain split leaks body into the transient stream on dense material | per-band detection, adaptive time constants, lookahead option; spectral HQ split in v1.1 |
| Chebyshev bank sounds harsh or intermodulates on full mixes | band-limited per band, envelope-normalised, order limits per band; bank presets voiced conservatively |
| Sub generator octave errors or chatter | hysteresis, minimum period, band-limited input, purity filter; octave-error gate in tests |
| Resonators run away or ring forever | bounded state, decay gate, hostile-input tests |
| Presets designed without listening (as INFRA and EXO so far) | objective gates and loudness calibration; mark presets "not auditioned" until reviewed by ear |
| UI cost in hosts (INFRA's RCA) | shared UI library with the RCA fixes, idle-CPU check in CI |
| A third copy of the look | extract `chromosphere_ui` first (M2) |

## 12. Decisions
Taken 2026-10-27:
1. Accent colour: **Cherenkov blue**. The VFD keeps its cyan; the accent is a deeper, more saturated blue so the two
   still read as different instruments.
2. Reverb send: **yes**, as the Afterglow stage (Section 4.6).
3. Standalone app: **no**. VST3 and AU only; the Windows installer installs the VST3.
4. Sidechain: v1.1, as planned in Section 2.

## 13. Success criteria
- All Section 8 gates pass in CI on three platforms.
- On a real drum bus, MUON replaces a transient designer plus a saturator plus a tuned resonator with one instance,
  at under 3% CPU.
- The four macros give a useful result on every factory preset without opening a page.
