# Chromosphere cosmic suite

Ten reverbs beyond the stock rooms, halls and plates. Each is its own algorithm, built from the stock
engine (early reflections, diffusion, FDN tail, convolver) plus the modules in `dsp/cosmic/`:
grain pitch shifter, swept state-variable filter, saturator, reverse buffer, granulator, pulse gate.
Each design has three macro controls (A, B, C) with names specific to that design, on top of the
shared Size, Decay, Pre-delay and Mix. Names and descriptions are working titles.

| # | Name | What it is | Topology | A / B / C |
|---|---|---|---|---|
| 1 | Shimmer | The classic octave-up bloom | FDN with a pitch shifter (+12) in the feedback loop, low-pass in loop | Shimmer amount / Interval (+5, +7, +12, +19) / Tone |
| 2 | Redshift | The opposite: a tail that sinks | FDN with -12 (and a little -7) in the loop, darkening low-pass | Shift amount / Depth (-5, -7, -12) / Darkness |
| 3 | Corona | Harmonic halo: fifth + octave | Two shifters (+7, +12) blended in loop, bright high shelf, slow wide modulation | Halo / Fifth-octave balance / Brightness |
| 4 | Wormhole | Thick, moving, detuned | Detune shifters (+/-0.3 st) in loop, post chorus (two LFO delays) | Detune / Movement / Width |
| 5 | Event Horizon | Heavy, dense, pulled down | Saturation in the loop, slow downward low-pass sweep, small -12 | Gravity (drive) / Collapse (sweep rate) / Sub |
| 6 | Nebula | Granular cloud into a vast tail | Granulator (random position, octaves up/down) feeds a large FDN | Cloud density / Scatter / Octaves |
| 7 | Pulsar | Rhythmic, breathing | Stock tail into a pulse gate (rate, shape) with input ducking | Rate / Shape / Duck |
| 8 | Solar Wind | Filtered, drifting, airy | Swept band-pass in the loop with random-walk cutoff, wide stereo | Wind (depth) / Drift (rate) / Resonance |
| 9 | Aurora | Curtains of light | Post-tail band-pass sweeps with 90 degrees between L and R, a touch of +12 | Sweep / Colour (centre) / Shimmer |
| 10 | Supernova | Swelling reverse blooms | Reverse buffer pre-stage into a huge tail, bloom envelope | Chunk / Bloom / Afterglow |

Where they sit in the plugin: the Mode selector grows from 3 to 13 entries, grouped
(Stock: Algorithmic, Convolution, Hybrid; Cosmic: the ten). The macro controls appear as a
"Cosmic" group of three dials whose names change with the mode. Presets get a "Cosmic" family.

Status: all ten are built (`dsp/cosmic/cosmic_engine.*`), wired into the plug-in (Mode entries 4 to 13,
parameters `cosmicA/B/C`), with three factory presets each in the "Cosmic" family.

How the engine works: the stock tail runs inside a feedback loop that is processed in fixed 32-sample
frames (so the result does not depend on the host block size; the wet path is delayed by those 32 samples,
0.7 ms, and nothing is reported to the host). The fed-back signal passes through the design's inserts
(pitch shifters, saturator, swept filter), is scaled so the loop gain stays below one at any decay time
(the tail's gain for sustained input grows with the square root of the decay time), and is soft-clipped.
A watchdog resets the engine if the output ever stops being finite.

Verification (`tools/cosmic_check.py`, 220 Hz burst, late tail 1.5 to 3 s against the stock algorithm):
Shimmer +34 dB an octave up; Redshift +27 dB an octave down; Corona +28 dB a fifth up; Event Horizon +24 dB
third harmonic; Pulsar gates by more than 100 dB; Supernova shows the reversed swells; all ten stay finite,
bounded (peak below 4 with everything at maximum) and decay.

Not done yet: listening. The macro ranges were set by reasoning and by these measurements, not by ear.

Rules that stay: zero latency reported (the pitch shifter and reverse buffer add internal delay
inside the wet path only), no allocation on the audio thread, stable for every setting (fuzz test),
freeze still works, CPU within the High-mode budget.
