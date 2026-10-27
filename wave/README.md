# Chromosphere | WAVE

Sing, hum or whistle, and play the Chromosphere instruments with your voice. WAVE tracks one voice in real time
and turns it into notes: the melody you sing, snapped to a key if you like, or a chord in the key for every note.
Loudness becomes velocity and aftertouch, and **Follow** carries your vibrato and slides onto the notes.

WAVE is built into **Chromosphere | INFRA** as its fifth page. The engine here (`dsp/wave`) has no JUCE
dependency, so other family instruments can use it later. Design and decisions: `docs/PRD.md`.

![WAVE page](../infra/docs/images/wave.jpg)

## Playing it

**Standalone app (the simplest and lowest-latency way):**
1. Open the INFRA app.
2. In Options > Audio/MIDI Settings, choose your microphone as the input device.
3. Go to the WAVE page and switch **WAVE on**. The app's "Mute audio input" setting is turned off for you;
   INFRA never sends the microphone to the speakers unless **Monitor** is up.
4. Sing. Use headphones: a voice tracker hears speakers as well as you.

**To play another instrument or DAW from your voice:** in the app, switch **MIDI out** on and choose a MIDI output
in Options > Audio/MIDI Settings. On Windows 11, Windows MIDI Services provides loopback ports (MIDI Settings app)
with no extra driver. Select the same port as a MIDI input in your DAW. MIDI out carries the notes, velocity and
the loudness/brightness controllers. Per-note pitch (Follow) stays inside INFRA, because MIDI 1.0 cannot bend
the notes of a chord separately.

**Inside a DAW:** INFRA now has an audio input bus, "WAVE In". How a host feeds audio into an instrument varies,
and none of the following has been tried yet (the build was tested headlessly only):
- Reaper: put the microphone on INFRA's track input, then record-arm it and turn monitoring on.
- Ableton Live, Bitwig, Cubase, Studio One: route the microphone track to INFRA's sidechain/audio input.
- FL Studio: generators do not normally take audio. Patcher may be able to route audio into INFRA's input.
  Otherwise use the standalone app with MIDI out, as above.

## Controls

| Section | Control | What it does |
|---|---|---|
| Input | WAVE on | Listens to the input. Off: the input is ignored. |
| | Range | Your lowest note: Low (E2), Mid (A2), High (E3), Very high (A3). Higher ranges respond faster. |
| | Gain, Gate | Input gain; the level below which nothing plays (red dashes on the trace). |
| | Monitor | Mixes your voice into INFRA's output (for the app without direct monitoring). |
| Notes | Key, Scale | 13 scales; Chromatic plays exactly what you sing. **Find key**: sing a few phrases, press again, and Key and Scale are set to the key you sang in. |
| | Follow | 0 plays locked notes; 100% carries the voice's exact pitch (vibrato, scoops, slides). |
| | Stability | How long a new pitch must hold before the note changes (shorter is quicker, longer is steadier). |
| | Octave, Dynamics | Output octave; how much loudness sets velocity. |
| Chords | Mode | Melody (one note) or Chords. |
| | Chord | Triad, Seventh, Sus2, Sus4, Add9, Ninth, Power, Octaves. In seven-note scales chords come from the scale: singing D in C major gives D minor. |
| | Sung note is | Root (chord built up from it) or Top (your melody stays on top; the chord is chosen under it, primary chords first). |
| | Voicing, Bass | Close, Open (drop-2) or Smooth (nearest inversion to the last chord); a bass note one or two octaves down. |
| | Strum, Hold | Delay between chord tones; Hold keeps the chord until you sing the next note (try it with the arpeggiator). |
| Expression | Loudness to, Brightness to | Aftertouch or mod wheel. Every INFRA preset routes aftertouch to something. |
| | MIDI out | Sends WAVE's notes to INFRA's MIDI output. |

WAVE settings are a performance setup: loading presets, Ignite and A/B leave them alone, and user presets do not
store them. They are saved with the project.

## Measured (`wave_tests`, synthetic voices)

- Note-on 25 ms after a sung A3 starts (Mid range), 17 ms for E4 (High), 31 ms for E2 (Low); audio-interface
  latency comes on top.
- Pitch within 3.5 cents from E2 to C6, and no octave errors on weak-fundamental voices or on sopranos whose 2nd or
  3rd harmonic dominates.
- Vibrato of +/-50 cents holds one note. A 40-note random phrase plays 40 of 40 notes with none extra.
  Fricatives, noise and silence play nothing.
- Same events for every block size; 44.1 to 192 kHz; 0.5% of one core at 48 kHz.

On real recordings (Essentia's test audio: a Carnatic vocal and a soprano), WAVE follows the line closely,
including a chromatic descent from C6. These recordings are what found the soprano harmonic problem.

## wave_analyze

Runs WAVE over a recording and prints the notes, the key it hears, and optionally writes a MIDI file:

    build/wave/wave_analyze voice.wav out.mid --range 1 --key 9 --scale 2 --chords --strum 20

Options: `--range 0..3`, `--gate dB`, `--key 0..11`, `--scale 0..12` (order of the Scale menu), `--chords`,
`--chord 0..7`, `--top`, `--bass 0..2`, `--strum ms`, `--stability ms`, `--octave n`, `--hold`, `--trace`.

## Not yet

Beatbox to drums (ULTRA), a separate WAVE plugin with MIDI output for the rest of the family, and MPE output.
