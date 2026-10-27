# PRD: Chromosphere | ULTRA (drum machine, VST3 / AU / standalone)

Status: Draft 1, implemented (M1 to M5 done on Linux; M6 CI set up, first Windows and macOS runs pending). Personal project, same scope rules as the Chromosphere reverb PRD and the INFRA PRD.
Product name: **Chromosphere | ULTRA** (bundle and file name `ChromosphereUltra`, since `|` is not allowed in file names)
Date: 2026-10-27

---

## 1. Summary

ULTRA is the drum machine in the Chromosphere family. EXO is the reverb, the sun in visible light. INFRA is the
synthesizer, the same star in infrared. ULTRA is the star in **ultraviolet**: the short wavelengths, where the sun
is all flares and sharp bursts of energy. A drum hit is a flare: a burst of energy and then a decay.

ULTRA is an eight-track synthesized drum machine. Each track plays one of thirteen drum **models** (synthesis
engines: kicks, snares, claps, hats, cymbals, toms, FM, metal, rim, noise, zap, modal resonator), shaped by six
dials that every model interprets in its own way. A per-track step sequencer drives the tracks. It has polymetric
lengths, per-track rates, swing, and lanes for velocity, chance, ratchet, pitch, micro-timing, trig conditions and
one parameter lock. A mixer with choke groups and multi-out routing feeds a master bus: drive, a glue
compressor, echo and the Chromosphere reverb as send effects, EQ, width and a limiter. The reverb is the same FDN
engine EXO uses, through the same thread-safe engine host.

It ships as VST3 (Windows, macOS, Linux), AU (macOS) and a standalone app. It shares the family design language:
a near-black matte faceplate, raised instrument bays with bolts, an amber CRT scope over a live sun, Barlow small
capitals, knurled dials with lit arcs, seven-segment readouts, lever switches and a cyan VFD preset selector. The
accent moves from INFRA's infrared red to **ultraviolet violet**, and the sun is drawn as it looks in the
extreme-ultraviolet images from solar observatories, where the corona glows violet.

## 2. What is borrowed, and from where

ULTRA is built from the two existing products. It reuses their code where possible rather than writing new code.

| From | What | How |
|---|---|---|
| EXO / reverb (`dsp/`, `plugin/EngineHost.*`, branch `main`) | FDN reverb engine, `fr::EngineHost` (worker-thread rebuilds, cross-fades), denormal guard | linked unchanged (`frdsp`, `frhost`) |
| INFRA (`infra/`, branch `claude/awesome-bell-rehevx`) | DSP blocks: `Rng`, `fastTanh`, `Svf`, `VoiceFilter` (8 filter types), `Noise`; effects: `Drive` (Soft/Tube/Fold/Cosmic Ray), `Echo`, `ReverbStage`, `Limiter` | copied into `ultra/dsp/ultra/` (namespace `ultra`); Echo and Reverb changed to send/return use |
| INFRA (ported from EXO, with INFRA's UI performance fixes, see `infra/docs/RCA-ui-performance.md`) | Look and feel, live sun, wordmark lens, VFD display and combos, seven-segment readouts, knob/choice/toggle/page/section widgets, preset bar and browser, update panel, updater, cached chrome | copied into `ultra/plugin/`, recoloured, brand strings changed |
| INFRA | Preset manager (embedded JSON factory presets, XML user presets, A/B, undo), Ignite idea, preset render check, editor snapshot tool, updater test, Inno Setup installer, CI release job | copied and extended for patterns |
| EXO / INFRA | Fonts (Barlow, OFL), corona and limb images | fonts copied; images derived by `tools/make_uv_assets.py` (as INFRA's `make_red_assets.py`) |

The UI rules from INFRA's root-cause analysis apply from the start:
- Images whose pixels are read or written, or that are touched off the message thread, are `SoftwareImageType`.
- Static chrome (background, plates and their shadows) is drawn once per window size.
- Each animated area has one repaint source, and nothing animates while it is hidden.
- Shared image caches are released when the last editor closes, never by static destruction.

## 3. Goals and non-goals

### Goals
- G1. **Sound.** Thirteen drum models, each good across its range, from classic analog circuits (808/909 style
  kicks, snares, hats, claps, cowbells) to modern FM, modal and noise designs. No samples: everything is
  synthesized, so tune and decay are continuous and nothing aliases audibly.
- G2. **Groove.** A modern step sequencer: 8 tracks, 8 patterns, up to 32 steps per track, each track with its
  own length and rate (polymeter and polyrhythm). Swing, humanize, and per-step velocity, chance, ratchet,
  pitch, micro-timing (nudge), trig condition and a parameter lock. Sample-accurate and locked to the host
  transport.
- G3. **Mix.** Per-track level, pan, two sends, mute, solo, choke groups (open/closed hats) and output routing
  to 8 auxiliary stereo outputs. The master bus can finish a beat inside the plugin.
- G4. **Four macros with a fixed meaning in every kit**, the same names as INFRA: **HEAT**, **GRAVITY**,
  **ORBIT**, **FLUX** (Section 7).
- G5. **At least 100 factory presets** (kit plus patterns), designed one by one, across 12 genre categories.
  Each has a name, category, tags and a one-line description.
- G6. **Ignite**: builds a new kit and patterns from genre rules, never a plain random fill.
- G7. Real-time safe: no allocation, locks or file access on the audio thread. Pattern edits reach the audio
  thread through a lock-free triple buffer.
- G8. Reasonable CPU: all 8 tracks hitting 1/32 ratchets with long tails and every effect on, under 10% of one
  core at 48 kHz on a current desktop CPU (measured by `ultra_bench`).
- G9. Light on a session (EXO's optimization guide): one playing instance under 1.5% of a core, a stopped, silent
  instance under 0.1%, the open editor at idle under 10%; one updater, cache and factory bank per process; nothing
  slow while the plug-in loads (measured by `ultra_plugin_bench`).
- G9. The family look, at any window size from 1180 x 820 up to 1900 x 1300.
- G10. Everything checkable headless: DSP and sequencer tests, a render of every factory preset (finite, not
  silent, no clipping), plugin smoke tests and editor snapshots as PNG.

### Non-goals (v1)
- Sample playback and user sample import (a "Sample" model is a v1.1 item; the model list is built to grow).
- MIDI output and dragging patterns into the host as MIDI (v1.1).
- Song mode / pattern chains (patterns switch live, by parameter, by MIDI note or by the host's automation).
- Per-step locks of more than one parameter (one lock lane per track, with a choosable target).
- AAX (same constraints as EXO: SDK and signing, see `docs/AAX.md`).
- Oversampling (the oscillators use band-limited shapes; the drive stage uses gentle curves).

## 4. Users

| User | Needs |
|---|---|
| Producer | A finished, mixed beat from a preset in seconds; kits that sit in a track; macros that do something on every kit |
| Beat maker / live player | Fast step entry, pattern switching on the bar, fills, mutes, MIDI pads, low CPU, nothing that stops the groove |
| Sound designer | Deep models, parameter locks, conditions, polymeter, modal and FM percussion, extreme effects |
| Mix engineer | Multi-out routing, choke groups, clean gain staging, a limiter that never lets the host clip |

## 5. Sound engine

### 5.1 Signal flow
```
               ┌─────────────── per track (x8) ───────────────┐
sequencer ─┐   │ MODEL ─► DRIVE ─► FILTER ─► AMP ─► LEVEL/PAN │──► Main ──┐
MIDI notes ┴─► │  (trigger params captured per hit)           │──► Aux n  │ (dry, pre master bus)
               └───────────┬───────────────┬──────────────────┘           │
                           │ send A        │ send B                       │
                           ▼               ▼                              ▼
                         ECHO ──(to rev)─► REVERB                MASTER DRIVE ─► GLUE
                           │               │                              │
                           └──── returns ──┴──────────────► + ◄───────────┘
                                                            │
                                               EQ ─► WIDTH ─► OUTPUT ─► LIMITER ─► out
```
Each track has two voice slots. A new hit takes the free slot; if both ring, the older one fades out in 3 ms.
A track is effectively monophonic with click-free retriggering, and a long cymbal can still ring under a fast
ratchet.

### 5.2 Trigger-time parameters
The sound of a hit is fixed when it is triggered, as on hardware drum machines. At trigger time the voice
captures its model parameters: the six dials, with that step's pitch and lock offsets and the macro offsets
added. So a parameter lock changes only its own step, and turning a dial never warps a ringing tail. Level,
pan, sends, filter cutoff and resonance stay live. They follow the dials and the host automation while a hit
rings.

### 5.3 The six dials
Every model reads the same six controls. The labels on the VOICE page change with the model, so the dial says
what it does.

| Dial | Range | Generic meaning |
|---|---|---|
| **Tune** | -24..+24 semitones | pitch relative to the model's base frequency |
| **Decay** | 0..1 | length; mapped to each model's useful range (e.g. kick 60 ms..4 s, closed hat 10..300 ms) |
| **Tone** | 0..1 | brightness |
| **Snap** | 0..1 | transient: click, attack, noise burst |
| **Color** | 0..1 | the model's timbre axis (Section 5.4) |
| **Sweep** | 0..1 | pitch envelope depth, or the model's motion axis |

Plus per track: **Drive** (pre-filter saturation), **Filter** (Off, LP 24, LP 12, BP 12, HP 12, HP 24, Notch,
Comb, Formant; INFRA's `VoiceFilter`), **Cutoff**, **Resonance**, **Velocity** sensitivity.

### 5.4 Models

| # | Model | Synthesis | Color | Sweep | Base |
|---|---|---|---|---|---|
| 1 | **Core Kick** | sine body with exponential pitch envelope, click (filtered impulse and noise), waveshaper | clean sine → driven, square-ish body | pitch drop 0..48 st | 50 Hz |
| 2 | **Gravity Kick** | FM kick: sine carrier, modulator at 1:1 with a decaying index, long sub tail | FM index | pitch drop | 45 Hz |
| 3 | **Flare Snare** | two tuned body modes (1 : 1.58) with pitch drop, noise through band-pass and high-pass with its own "snappy" envelope | body ↔ noise balance | body pitch drop | 185 Hz |
| 4 | **Burst Clap** | band-passed noise in 1..6 bursts about 9 ms apart, then a diffuse tail | burst count and spread | tail ("room") level | 1.1 kHz |
| 5 | **Ion Hat** | six PolyBLEP squares at the 808's inharmonic ratios, band-pass and high-pass, plus noise; closed and open by Decay | metal ↔ noise | band-pass sweep | 7.5 kHz |
| 6 | **Corona Cymbal** | the metallic bank, ring-modulated pairs, two bands with separate decays, noise wash | shimmer (ring mod) | stick-to-wash balance | 6 kHz |
| 7 | **Orbit Tom** | sine-to-triangle body with pitch drop, second mode at 1.5x, noise skin | body shape and second mode | pitch drop | 110 Hz |
| 8 | **Pulsar FM** | two-operator FM percussion with its own index envelope | ratio (0.5..8, with stops at musical ratios) | pitch drop | 220 Hz |
| 9 | **Quasar Metal** | cowbell (two squares at 540:800) crossfading to bell (inharmonic sine partials 1, 2.76, 5.40, 8.93) | cowbell → bell | partial detune | 560 Hz |
| 10 | **Spark Rim** | rimshot and clave: a resonant band-pass "ping" excited by a click, a short tonal burst | rim → clave | pitch drop | 1.7 kHz |
| 11 | **Solar Wind** | filtered noise instrument (shaker, brush, riser, crash noise) with an attack | white → pink → crackle | filter sweep up or down (0.5 = none) | 3 kHz |
| 12 | **Plasma Zap** | laser / zap: a fast exponential sweep from far above the pitch, phase-distorted sine to saw | waveform | sweep depth | 300 Hz |
| 13 | **Nebula Modal** | modal resonator: an impulse or noise excitation into six resonant modes | material: membrane (Bessel ratios) → bar → glass | strike position | 180 Hz |

All models: amplitude envelopes are exponential (with a short linear attack for Snap where useful), oscillators
use a polynomial sine and PolyBLEP squares and saws, and every model is level-matched so that the default dials
peak near -6 dBFS at full velocity.

### 5.5 Velocity, accent, choke
- Velocity scales level by `1 - vs + vs * v` (vs = the track's velocity sensitivity), and also opens Tone a
  little (up to +0.15 at full velocity) so hard hits are brighter.
- **Choke groups** 1..4. A hit on a track in group g fades every other track in g in 5 ms (an open hat cut by
  the closed hat).

### 5.6 Master bus
1. **Drive**: Soft, Tube, Fold, Cosmic Ray (INFRA's drive). Amount, tone, mix.
2. **Glue**: a feed-forward RMS bus compressor with a soft knee. Threshold, ratio (1..10), attack (0.1..30 ms),
   release (50..1200 ms), makeup, mix (parallel compression). Gain reduction is shown in the scope.
3. **Echo** (send A): tempo-synced or free, ping-pong, feedback, damping, return level, and **Echo > Reverb**
   (how much echo feeds the reverb).
4. **Chromosphere reverb** (send B): the EXO FDN engine through `fr::EngineHost`. Size, decay, damping,
   pre-delay, return level.
5. **Output**: low shelf (100 Hz) and high shelf (8 kHz) at ±12 dB, stereo width, output level, and a peak
   limiter at -1 dBFS, so no kit can clip the host.

### 5.7 Outputs
The main stereo output, plus 8 optional stereo auxiliary outputs (`Aux 1` .. `Aux 8`). A track's **Output**
selects Main or an aux. A track sent to an aux plays there dry (no master bus); its sends still feed the main
bus effects. If the host has not enabled that aux, the track falls back to Main. A mono main output is
supported (sum to mono).

## 6. Sequencer

### 6.1 Structure
- 8 **patterns** (A..H) per preset. A pattern holds one track pattern for each of the 8 tracks.
- A track pattern has a **length** (1..32 steps) and a **rate** (1/32, 1/16T, 1/16, 1/8T, 1/8, 1/4, plus 1/16
  dotted for 3-against-4 work). Tracks with different lengths or rates drift against each other and meet again
  later (polymeter).
- Per step, seven lanes:

| Lane | Range | Meaning |
|---|---|---|
| **Velocity** | 0..127 | 0 = no hit |
| **Chance** | 0..100% | probability the step plays |
| **Ratchet** | 1..4 | repeats inside the step, evenly spaced, each a little softer |
| **Pitch** | -24..+24 st | added to Tune for this step |
| **Nudge** | -50..+50% of a step | micro-timing, early or late |
| **Condition** | Always, Fill, Not fill, 1st, Not 1st, 1:2, 2:2, 1:3, 2:3, 3:3, 1:4, 2:4, 3:4, 4:4 | when the step plays (A:B = on loop A of every B loops of this track) |
| **Lock** | -100..+100% | an offset to the track's **lock target** (Decay, Tone, Snap, Color, Sweep, Drive, Cutoff, Pan, Send A or Send B) for this step only |

### 6.2 Timing
- **Locked to the host's song position** (PPQ) while the host plays: a step's time is a pure function of the
  host position, so looping, scrubbing and bouncing give the same result every time. While the host is stopped
  (or in the standalone app) the internal clock runs from the **Play** button at the host tempo, or at the
  **Tempo** parameter when the host gives none.
- Swing (50..75%) delays every second 1/16 step (on tracks with a 1/16 or finer rate it applies to the off-step
  of each pair).
- **Humanize** (from the ORBIT macro) adds up to ±8 ms of timing jitter and ±15% of velocity jitter. It is
  seeded from the step's absolute index, so a given bar always plays the same way.
- Chance is decided from a hash of (seed, track, absolute step index), so it is deterministic per position,
  ratchet repeats of one step share one decision, and a loop of the same bar repeats its result.
- Events are sample-accurate inside the block at any block size.

### 6.3 Patterns, fills, live use
- **Pattern** is a host parameter (automatable). A change is queued and happens on the next bar line (4 beats).
  It is immediate while stopped.
- **Fill** (host parameter, momentary in the UI) makes Fill steps play and Not-fill steps rest.
- MIDI notes 24..31 (C0..G0) select patterns A..H. Each track also responds to its own MIDI note (default GM
  map: 36 kick, 38 snare, 39 clap, 42 closed hat, 46 open hat, 45 low tom, 50 high tom, 56 cowbell), with the
  note's velocity, at sample-accurate offsets.
- Editing: click toggles a step (velocity 100), drag in a lane paints values, double-click resets a value. Copy,
  paste and clear for a track or a whole pattern; **Shift** rotates a track left or right; **Mutate** varies
  the selected track (moves, adds and removes hits by the rules of its role).
- Every edit is undoable (the pattern data lives in the plugin's value tree under the undo manager).

## 7. Macros

The four macros have the same names as INFRA's and a fixed meaning in every kit, so a player can reach for them
blind. They are hard-wired (not a matrix) so they work the same on every factory kit, generated kit and user kit.

| Macro | Meaning | What it moves |
|---|---|---|
| **HEAT** | brightness and energy | master drive +0.5, every voice's Tone +0.35 and Snap +0.25, glue makeup +3 dB |
| **GRAVITY** | weight and pull | kick and tom models: Decay x(1 + 1.5 g), Tune -5 g st; low shelf +6 g dB |
| **ORBIT** | motion and groove | swing +20 points (capped at 75%), humanize 0 → full, lock lanes x(1 + o) |
| **FLUX** | space and aftermath | send A and B levels x(1 + 2 f) plus 0.2 f, reverb decay x(1 + f), echo feedback +0.25 f |

## 8. Presets

- At least 100 factory presets in 12 categories: House, Techno, Electro, Hip-Hop, Trap, Drum & Bass,
  Breakbeat, UK Garage, Lo-Fi, Synthwave, Industrial, World & Percussion, plus Experimental if needed. Stored as
  JSON (`ultra/data/presets.json`) generated by `ultra/tools/make_presets.py`.
- A preset holds parameter values (only those that differ from the defaults), the 8 track names, and 2..8
  patterns written in a compact step notation in the script.
- Every preset sets its tempo hint, its choke groups (hats), and patterns that use the lanes musically (ghost
  notes at low velocity, ratchets on hats, conditions for variation, a fill pattern).
- User presets: one XML file each in the user application-data folder (`ChromosphereUltra/Presets`), holding
  parameters and patterns.
- Browser: search by name, category, tag or description. Previous / next, A/B compare, undo / redo.
- **Keep patterns**: a preset-bar switch that loads only the kit (sounds, mixer, effects) and keeps the
  current patterns, so kits can be auditioned under one groove.
- **Ignite** (genre menu: House, Techno, Hip-Hop, Trap, Drum & Bass, Breakbeat, Electro, Experimental): picks
  models for each role, designs the sounds within ranges that suit the genre, writes 4 patterns (main, variation,
  break, fill) from the genre's rhythmic rules, and sets the macros' starting points.

## 9. Interface

The family construction in ultraviolet. Default 1280 x 900, resizable 1180 x 820 .. 1900 x 1300.

- **Header**: the live UV sun emblem, the wordmark **CHROMOSPHERE | ULTRA** (lens, aberration, glitch,
  breathing cursor), a status line (pattern, step, tempo, sample rate, version), the preset bar (VFD name,
  arrows, Save, Delete, A/B, Undo, Redo, Keep patterns), Ignite and Updates.
- **Main display** (recessed amber CRT over the rising violet limb), three panes:
  1. **Orbit radar**: the 8 tracks as concentric orbits around a small sun. Each orbit has one tick per step;
     active steps are dots sized by velocity; each track's playhead sweeps its own orbit (so polymeter is
     visible as orbits turning at different speeds), and a hit flashes its dot.
  2. **Voice**: a rendered hit of the selected track (waveform and envelope), with its model, tune and decay.
  3. **Output**: scope, level meters, glue gain-reduction meter and a limiter lamp.
- **Macro bay**: the four macros as large dials beside the display, and the transport: Play, Pattern A..H
  buttons (the queued one blinks), Fill, Swing.
- **Pages** in the main plate:
  - **SEQ**: the 8 x 16 step grid (steps 17..32 on a second page), each row with the track name, mute and solo
    lamps, length and rate; playhead column; below it, the lane editor of the selected track (velocity, chance,
    ratchet, pitch, nudge, condition, lock) and the pattern tools (copy, paste, clear, shift, mutate).
  - **VOICE**: the selected track: model, the six dials with model-specific labels, drive, filter, velocity,
    choke, MIDI note, output, lock target.
  - **MIX**: 8 channel strips (level, pan, send A, send B, mute, solo, choke, output) and the returns.
  - **FX**: drive, glue, echo, reverb, output.
- **Pads** across the bottom (where INFRA has its keyboard): 8 pads with the track number, name and model. They
  light with each hit (brightness by velocity). Clicking a pad selects its track and plays it; double-clicking
  renames it.
- Section codes (01, 02 ...), caps labels, seven-segment readouts under every dial, scored dividers, lamps on
  toggles, a kAccent focus ring, accessibility titles on every control, as in INFRA.

## 10. Architecture

```
ultra/
  dsp/ultra/  JUCE-free engine: params, blocks (from INFRA), models, voice, sequencer, fx, drum machine
  plugin/     processor (parameters -> engine), pattern store, editor, look, sun, VFD, readouts, presets, Ignite
  tools/      make_presets.py, make_uv_assets.py, ultra_bench
  tests/      DSP and sequencer tests, plugin smoke, preset render check, editor snapshot, updater test
  data/       presets.json
  assets/     fonts (Barlow, OFL), ultraviolet solar images
  installer/  Inno Setup script and icons
```
- `ultra::DrumMachine::process(outputs, n, midi events, transport)` renders a block; it never allocates after
  `prepare()`. Parameters arrive as a plain `KitParams` struct copied once per block.
- **Pattern data path**: the patterns live in a `PATTERNS` child of the parameter value tree, as one compact
  string per lane. That gives undo, host state and presets for free. A listener on the message thread turns the
  tree into a plain `PatternBank` struct and publishes it through a lock-free triple buffer. The audio thread
  takes the newest bank at the start of each block. It never waits, and it never sees a half-written pattern.
- UI taps (per-track hit flashes, step positions, scope, meters) are atomics written once per block.
- The reverb stage is `fr::EngineHost` from the reverb project, unchanged.

## 11. Quality and test gates

- Every model at its default dials peaks at -6 dBFS (±0.5 dB) at full velocity.
- Every model, at every corner of Color, Snap, Sweep, Tone and Decay, at Tune -24 and +24: output finite, peak
  under 1.5 (+3.5 dBFS, before the master limiter), and every hit ends (falls 80 dB) within twice its longest
  decay time.
- The hat and cymbal squares (2-point PolyBLEP): at 3130 Hz, the top of the metal bank at Tune +24, the worst
  alias stays at least 28 dB below the fundamental (measured: about 30 dB there, 38 dB at 810 Hz). The band-pass
  and the dense metallic spectrum mask the rest; oversampling the bank is a v1.1 option if it is ever audible.
- Sequencer: at 120 BPM and 48 kHz, 1/16 steps land exactly every 6000 samples, at any block size from 1 to
  4096. Swing, nudge, ratchets and triplet rates land where computed. Polymeter lengths repeat at the LCM. A:B
  conditions, Fill and 1st behave as specified. Chance is deterministic per position and close to its
  percentage over 10,000 steps. Pattern switches land on the bar line. A host loop jump replays its steps.
- MIDI note triggers are sample-accurate; choke groups cut within 5 ms.
- Every factory preset, played for 4 bars at its tempo: output finite, peak below -0.1 dBFS (after the
  limiter), loud enough (90th-percentile RMS above -30 dBFS), no DC above 1%.
- Engine handles any block size from 1 to 4096 and sample rates from 44.1 to 192 kHz.
- State round trip restores parameters and patterns exactly; undo reverts a pattern edit.
- CPU gate as in G8.

## 12. Milestones

1. M1: PRD (this document).
2. M2: DSP: models, voices, sequencer, mixer, master bus; tests and benchmark.
3. M3: Plugin processor, parameters, pattern store, multi-out, smoke test.
4. M4: Editor in the family language, in ultraviolet, with the live displays and the pads.
5. M5: 100+ factory presets, render check of every preset, Ignite.
6. M6: CI (Windows, macOS, Linux), installer, updater, release to `ultra-nightly`, README.

## 13. Risks

| Risk | Mitigation |
|---|---|
| Presets designed without listening | objective render checks (level, peak, DC, decay), conservative gain staging, the limiter, honest README |
| Pattern state size in the host project | compact lane strings (about 25 KB of XML for 8 full patterns) |
| Audio thread reading half-edited patterns | triple buffer, written only on the message thread under a mutex the audio thread never takes |
| Multi-out hosts that do not enable aux buses | tracks fall back to Main; the main mix is always complete |
| CPU of six-square hats and modal voices at 1/32 ratchets | two voice slots per track, voices sleep when silent, polynomial sines, bench gate |
| Merge with the INFRA branch | ULTRA lives in its own folder; only the top-level CMakeLists (one line) and the CI workflow are shared |
| Windows GPU image pitfalls (INFRA's RCA) | the RCA's rules are applied from the start (Section 2) |
