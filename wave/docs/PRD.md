# PRD: Chromosphere | WAVE (voice to notes, built into INFRA)

Status: Draft 1. Personal project, same scope rules as the Chromosphere reverb PRD.
Product name: **Chromosphere | WAVE**. It ships inside **Chromosphere | INFRA** (a fifth page, WAVE) and as a
JUCE-free library (`wave/`) that other family instruments can use later.
Date: 2026-10-27

---

## 1. Summary

The sun rings. Helioseismology listens to the acoustic waves that travel through the star and read its interior
from them. WAVE is that idea pointed the other way: the user hums, sings or whistles, and the voice plays the
Chromosphere instruments.

WAVE tracks the pitch, loudness and brightness of a single voice in real time and turns them into notes:
- **Melody**: the sung note, optionally snapped to a key and scale, with the voice's own vibrato and slides carried
  as per-note tuning (the voice's exact pitch, or a locked note, or anything between).
- **Chords**: each sung note becomes a chord built in the chosen key (diatonic triads, sevenths, sus, add9, ninths,
  power chords), with the sung note as the root or as the top voice, smooth voice leading, an optional bass note,
  strum and hold.
- **Expression**: loudness sets velocity and drives aftertouch or the mod wheel, which every INFRA preset routes to
  something musical; vowel brightness can drive the other.
- **Key listening**: sing for a few seconds and WAVE finds the key.

## 2. Research and the implementation decision

### 2.1 Prior art
Vochlea's **Dubler 2** is the closest product: voice to MIDI with key locking, automatic key detection, a chord
mode with 14 fixed chord sets, and beatbox triggers. Reviews (MusicTech, MusicRadar, Sound On Sound, Bonedo) agree
that it is better for beats than for pitch, that almost all of its pitch MIDI needs editing, that its chord mode
cannot be customised, and that its in-DAW capture route does not support chords. It runs as a separate app that
sends MIDI to the DAW. Offline melody extraction (Melodyne, DAW audio-to-MIDI) is accurate but not live.

What this tells us: pitch detection on a voice is not the hard part; note segmentation is (when a new note starts,
what is vibrato, what is a scoop). Key locking and chords hide pitch errors, and they are where a product can beat
Dubler (custom chord types, voicing, a melody-on-top mode).

### 2.2 Pitch tracking
- YIN and McLeod (MPM) are the standard time-domain trackers for monophonic voice. Both need the analysis window to
  hold at least one period of the lowest note, plus the lag range: the window length, not the arithmetic, sets the
  latency floor (about 10 ms per period at 100 Hz).
- Small neural trackers (SwiftF0, about 96k parameters, 42x faster than CREPE) are accurate in noise, but the PRD of
  the family excludes machine-learning inference in the plugins and they do not lower the window-length floor.
- Decision: **YIN** (difference function, cumulative mean normalisation, absolute threshold, parabolic interpolation)
  on a decimated signal (about 11 to 12 kHz), with the window sized from a **Range** control so a high voice gets a
  shorter window and lower latency. Note segmentation is a state machine on pitch stability, voicing, level and
  level onsets (after Brossier's real-time note segmentation: onset function plus silence gate).

### 2.3 Where WAVE runs: inside INFRA
Three routes were considered:

| Route | For | Against |
|---|---|---|
| A. Separate plugin with MIDI output | Drives any instrument | VST3 has no MIDI-effect type; FL Studio does not pass MIDI between plugins without manual port routing, and users report the VST3 port setting missing (Scaler forum); most DAWs need manual routing |
| B. Standalone app with a virtual MIDI port | Works with every DAW (Windows 11 now ships loopback MIDI ports, Windows MIDI Services, 2026) | An extra app and a MIDI hop; continuous pitch needs channel pitch bend, which cannot bend a chord's notes separately |
| C. **Built into INFRA** | No MIDI routing at all; lowest latency; the voice's pitch goes straight to INFRA's per-note **NoteTuning** path (already used for VST3 note expression), so vibrato and slides bend every chord note exactly; the arp, macros and presets work unchanged | Plays INFRA only (unless MIDI out is used) |

Decision: **C, with B's benefits included**. WAVE is built into INFRA, and INFRA gets:
- an audio **input bus** ("WAVE In", mono or stereo). In the INFRA standalone app it is the microphone (the
  lowest-latency way to play: one audio-interface round trip plus analysis). In a DAW it takes the track input or a
  sidechain, depending on the host.
- **MIDI output** of WAVE's notes (a switch). In the standalone app, choose a MIDI output in Settings (on Windows,
  a Windows MIDI Services loopback port) and WAVE plays any instrument in any DAW; in a DAW, hosts that route
  plugin MIDI output can send WAVE to ULTRA or anything else.

The engine lives in `wave/dsp` with no JUCE dependency, so a separate WAVE plugin or ULTRA integration later reuses
it unchanged.

## 3. Goals and non-goals

### Goals
- G1. Playable latency: from the start of a sung note to the note-on, at most **30 ms** in the Mid range and
  **40 ms** in the Low range, measured on synthetic voice in the test suite (Section 8). Audio-interface latency is
  extra.
- G2. Accurate notes: steady tones within 10 cents; no octave errors and no note changes from vibrato of up to
  +/-50 cents; no notes from unvoiced noise at the default gate.
- G3. Chords in a key that sound like a player chose them: diatonic quality from the scale, sung note as root or as
  the top voice, smooth voice leading, bass, strum and hold.
- G4. Expression: velocity from loudness, continuous loudness and brightness as aftertouch or mod wheel, and pitch
  follow from locked notes to the exact voice pitch.
- G5. Real-time safe and block-size independent: no allocation, locks or file access on the audio thread; the same
  events at the same sample positions for any host block size, at 44.1 to 192 kHz.
- G6. Cheap: the whole WAVE engine under 1% of one core at 48 kHz.
- G7. Everything checkable headless: DSP tests with objective gates, a command-line analyser that turns a WAV file
  into notes and a MIDI file, plugin tests and editor snapshots.

### Non-goals (v1)
- Polyphonic input (several voices, guitar chords).
- Beatbox to drums (onset classification to ULTRA). Planned for v1.1; the onset detector is already part of the
  engine.
- Machine-learning pitch tracking (family rule; YIN is enough for one voice).
- MPE output. MIDI output carries notes, velocity and the expression controllers; continuous pitch stays inside
  INFRA (NoteTuning).
- Feedback suppression: use headphones when singing near speakers.

## 4. Engine

### 4.1 Signal flow
```
input (mono sum) -> gain -> high-pass 60 Hz -> anti-alias low-pass -> decimate to ~11-12 kHz -> ring buffer
   every hop (32 decimated samples, ~2.7 ms):
      YIN pitch + confidence, RMS level (fast and slow), brightness (native-rate high-band ratio)
      -> median-of-3 pitch -> note segmentation -> quantizer (key, scale, hysteresis)
      -> melody or chord builder -> event queue (with strum delays) -> events with sample offsets
```

### 4.2 Pitch tracker
- Decimation factor D = round(fs / 11025): 4 at 44.1/48 kHz, 8 at 88.2 kHz, 9 at 96 kHz, 17 at 192 kHz. A
  4th-order Butterworth low-pass at 0.4 x the decimated rate before decimation.
- Range control sets the lowest pitch (and so the window): Low (from E2, 75 Hz), Mid (from A2, 100 Hz), High (from
  E3, 150 Hz), Very high (from A3, 200 Hz). The highest pitch is 1600 Hz in every range.
- YIN: integration window = maximum lag = fa / fmin; threshold 0.2 on the normalised difference; the first dip
  below it, refined to its local minimum and by parabolic interpolation. Confidence = 1 - normalised difference.
- Voiced = confidence above 0.75 and the gate open.

### 4.3 Level, gate and brightness
- Fast level: RMS of the last 5 ms. Slow level: a 60 ms smoother of the fast level.
- Gate (dB control) with 6 dB hysteresis.
- Brightness: the energy ratio of a 1.5 kHz high-passed signal to the full signal, smoothed to about 30 ms,
  mapped to 0..1.

### 4.4 Note segmentation
States: Idle, Active.
- **Note on** (Idle): voiced for 2 consecutive hops with the pitch spread within 1 semitone. The note is the
  quantized median pitch. Velocity from the peak fast level (Dynamics control: 0 = fixed 100, 1 = full range).
- **Note change** (Active): the quantizer gives a different note for the whole **Stability** time (10..200 ms,
  default 50 ms). The new note starts before the old one ends (legato, so INFRA's legato and glide work). Common
  chord tones are kept.
- **Re-articulation**: a level onset (fast level 6 dB over slow, and over the gate by 6 dB, at least 80 ms after
  the last onset) restarts the note. "Da da da" on one pitch plays three notes.
- **Note off**: unvoiced or gated for longer than 45 ms. With **Hold** on, notes stay until the next sung note.
- Median-of-3 on the pitch track removes single-hop octave glitches.

### 4.5 Quantizer, keys and scales
- Key: C..B. Scales: Chromatic, Major, Natural minor, Harmonic minor, Melodic minor, Dorian, Phrygian, Lydian,
  Mixolydian, Locrian, Major pentatonic, Minor pentatonic, Blues.
- The sung pitch goes to the nearest scale tone. Hysteresis: the current note holds until the pitch passes the
  midpoint to a neighbouring scale tone by 0.2 semitones (vibrato does not flip notes).
- **Follow** (0..100%): the per-note tuning sent with each note is Follow x (voice pitch - note), smoothed over
  10 ms. 0 = locked notes; 100% = the voice's exact pitch, vibrato and slides included (applied to every chord
  tone, so chords bend together).
- **Octave**: -3..+3 octaves on the output.
- **Key listening**: while Listen is on, WAVE builds a duration-weighted histogram of sung pitch classes; turning it
  off picks the best of the 24 major and minor keys by correlation with the Krumhansl-Kessler profiles and sets
  Key and Scale.

### 4.6 Chord builder
- Chord types: Triad, Seventh, Sus2, Sus4, Add9, Ninth, Power, Octaves.
- In seven-note scales chords are stacked from the scale (singing D in C major gives D minor; the seventh on G gives
  G7). In other scales the chord quality is the one whose tones fit the scale best (major, then minor, then power).
- **Sung note is**: Root (the chord is built up from it), or Top (the melody stays on top and the chord is chosen
  under it: among the chords of the scale that contain the note, the primary ones first: I, IV, V, vi, ii, iii,
  vii).
- **Voicing**: Close (root position), Open (drop-2), Smooth (the inversion nearest the previous chord, kept near the
  sung register).
- **Bass**: Off, one octave or two octaves below the chord root.
- **Strum**: 0..120 ms between successive chord tones, low to high.
- Up to 8 notes per chord (including bass).

### 4.7 Output events
Note on (with velocity), note off, per-note tuning (semitones), and one expression controller per source:
- **Level to**: Off, Aftertouch, Mod wheel (default Aftertouch: every INFRA preset routes it).
- **Brightness to**: Off, Aftertouch, Mod wheel (default Off).
Controller events are sent at most every 4 hops (~11 ms) and only when the value changes.

## 5. INFRA integration
- New parameters (all prefixed `wv`, automatable): On, Range, Input gain, Gate, Monitor, Key, Scale, Follow,
  Stability, Octave, Dynamics, Mode (Melody / Chords), Chord, Sung note is, Voicing, Bass, Strum, Hold,
  Level to, Brightness to, MIDI out.
- Presets do not touch WAVE settings: loading a preset, Ignite, and A/B keep them (they are a performance setup, not
  part of a sound). User presets do not save them.
- Bus layout: stereo or mono output (as before), plus an input bus "WAVE In" (disabled, mono or stereo). WAVE reads
  the mono sum. With WAVE off the input is ignored and nothing changes for existing projects.
- WAVE's events merge with the host's MIDI (same block, by sample offset), so the arpeggiator, sustain pedal,
  voice modes, glide and macros work with the voice as with a keyboard.
- **Monitor** (0..100%) mixes the input into INFRA's output (for the standalone app without direct monitoring).
- **MIDI out**: WAVE's notes, velocities and expression controllers on channel 1. INFRA declares MIDI output.
- Reported latency stays zero (WAVE is a live input; compensating it would delay the synth).

## 6. Interface
A fifth page, **WAVE**, in INFRA's tabs:
- **Voice trace** (section 19): a recessed display with the last 4 seconds as a piano roll: the detected pitch as a
  bright trace, the played notes as bars, the scale's tones as lit rows (the key's root brighter), and the input
  level. Readouts: detected note and cents, current chord name.
- **Input** (20): On, Range, Gain, Gate, Monitor.
- **Notes** (21): Key, Scale, Listen, Follow, Stability, Octave, Dynamics.
- **Chords** (22): Mode, Chord, Sung note is, Voicing, Bass, Strum, Hold.
- **Expression** (23): Level to, Brightness to, MIDI out.
The status line shows "WAVE" with the detected note while WAVE is on.

## 7. Architecture
```
wave/
  dsp/wave/   tracker (decimator, YIN, level, brightness), harmony (scales, quantizer, chords, key finder),
              engine (segmentation, events)
  tests/      DSP tests (synthetic voices: harmonics, formants, vibrato, jitter, breath noise)
  tools/      wave_analyze: a WAV file in, the notes out (text and a standard MIDI file)
  docs/       this PRD
```
- `wave::Engine::process(const float* in, int n, Event* out, int maxOut)` is allocation-free after `prepare()`.
- INFRA owns one engine; its processor converts events to `infra::MidiEvent` and merges them.
- UI data: a ring of per-hop frames (pitch, level, note) and the current chord, in atomics.

## 8. Quality and test gates
- Pitch: synthetic voices (harmonic source with formants, jitter and breath noise) from 80 Hz to 1000 Hz, error
  below 10 cents on the steady part.
- Latency: note-on within 30 ms of the tone's start in the Mid range (A3 and above), within 40 ms in the Low range.
- Vibrato +/-50 cents at 5.5 Hz on a held note: one note on, no note changes.
- Sung scale with gaps and glides: the right note sequence; re-articulated notes on one pitch play separately.
- Octave errors: none on the synthetic set, including a voice with a weak fundamental.
- White and pink noise at -20 dBFS, and silence: no notes.
- Chords: diatonic qualities right for every degree of every seven-note scale; Top mode keeps the sung note highest;
  Smooth voicing moves less than Close over a I-vi-IV-V progression; bass and strum offsets exact.
- Every note-on has a note-off after silence (no stuck notes), with and without Hold (Hold releases on reset).
- Block-size independence: blocks of 1, 7, 64, 512 and 4096 samples give identical events; 44.1 to 192 kHz work.
- Key finder: a sung major or minor scale fragment finds its key.
- CPU: measured by the DSP test (whole engine, 48 kHz) under 1% of one core.
- Plugin: WAVE on with a sung tone on the input makes sound; WAVE off, the input is ignored; presets keep WAVE
  settings; pluginval at strictness 10 (CI).

## 9. Milestones
1. M1: PRD (this document).
2. M2: WAVE engine and DSP tests, `wave_analyze`.
3. M3: INFRA integration (bus, parameters, events, MIDI out, preset exclusion), plugin tests.
4. M4: WAVE page and voice trace, snapshots.
5. M5: CI and README.
6. v1.1: beatbox to ULTRA, a stand-alone WAVE plugin with MIDI output for the rest of the family.

## 10. Risks
| Risk | Mitigation |
|---|---|
| Latency feels late on percussive patches | Range control shortens the window; documented; pads and leads suit voice best |
| Real voices differ from the synthetic test set | `wave_analyze` on the user's own recordings; parameters (gate, stability, range) exposed |
| Hosts differ in how they feed audio into an instrument | The standalone app always works (microphone in, optional MIDI out to a loopback port); host notes in the README |
| Speaker bleed into the microphone retriggers notes | Gate, headphones advised |
| WAVE settings lost on preset change | Excluded from presets, Ignite and A/B by parameter prefix (tested) |

## 11. Sources
- Dubler 2 reviews: MusicTech, MusicRadar, Sound On Sound, Bonedo; Thomann and Andertons product pages.
- de Cheveigne and Kawahara, "YIN, a fundamental frequency estimator for speech and music" (2002).
- McLeod and Wyvill, "A smarter way to find pitch" (MPM, 2005); JUCE forum thread on lowest-latency pitch detection.
- SwiftF0 (arXiv 2508.18440).
- Brossier, Bello, Plumbley, "Real-time temporal segmentation of note objects in music signals" (2004).
- Krumhansl and Kessler key profiles (1982).
- Scaler and JUCE forums on MIDI routing from VST3 plugins in FL Studio; Windows MIDI Services loopback
  documentation (Microsoft, 2026).
