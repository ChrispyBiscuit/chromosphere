# Acoustic engine

The Acoustic mode models a real room. The other modes shape a reverb with dials; Acoustic computes the reverb from a room's
dimensions, its surface materials, the positions of the sound sources and the microphones, and the air.
Code: `dsp/space/room_model.*` (the physics, run when a room is built) and `dsp/space/space_engine.*` (the audio).
Tests: `tests/test_space_engine.cpp`.

## What it models

| Effect | How |
|---|---|
| Room shape and size | Shoebox, length x width x height (Size scales all three). |
| Echoes and reflections | Image-source method (Allen & Berkley 1979): every specular reflection path to every order, up to the early-part length. |
| Delay | Each path arrives at distance / c. c = 331.3 sqrt(1 + T/273.15) m/s, so it follows the air temperature. Time 0 is the direct sound. |
| Inverse-square law | Each path's amplitude is 1/r (energy 1/r^2). The direct sound uses the same law, so the direct-to-reverberant ratio follows the distance and the critical distance r_c = 0.057 sqrt(V/T). |
| Materials | One material each for the walls, the floor and the ceiling: octave-band absorption (63 Hz to 16 kHz). Each bounce multiplies a path by sqrt(1 - alpha) in each band. |
| Diffusion (scattering) | Each material also has a scattering coefficient. The scattered share of each reflection leaves the specular path and returns as "diffuse rain": impulses whose density grows as 4 pi c^3 t^2 / V, carrying the scattered share of the statistical energy. |
| Frequency attenuation by air | ISO 9613-1 air absorption per band from temperature and humidity, on every path (by its length) and in the decay time (the 4mV term). |
| Frequency gain (resonance) | In small rooms, below the Schroeder frequency 2000 sqrt(T/V), the room's modes are modelled: f = c/2 sqrt((nx/L)^2 + (ny/W)^2 + (nz/H)^2). Each mode gets its decay from the walls it meets (Kuttruff's modal damping) and its gain from the mode shape at the sources and microphones. Bass is boosted or cancelled depending on where the listener sits. |
| Decay time | Eyring per band: T = 24 ln10 V / (c (-S ln(1 - a) + 4mV)), with a the area-weighted absorption. |
| Stereo | The input's left and right channels are two sources across the stage (Spread). The listener has a microphone pair: ORTF, XY, AB, Wide AB, Blumlein or Mono. Every path is computed for each source and each microphone (true stereo, four channels), with the time difference between the microphones and each microphone's polar pattern (first order: g = (1 - p) + p cos theta). The late field uses the pair's diffuse-field coherence. |
| Contents | Furnishing (0 to 1) stands for what a shoebox does not model: furniture, people, doors, windows, irregular surfaces. It adds a broadband absorption (0.20 at 63 Hz rising to 0.30 above 1 kHz at full furnishing) and scattering. |

## How it is rendered

1. **Early part** (true stereo, through the zero-latency convolver). Its length is at least the conservative perceptual mixing
   time t_mp95 = 0.0117 V + 50.1 ms and twice the mean one, t_mp50 = 20 V/S + 12 ms (Lindau, Kosanke & Weinzierl). It is
   50 to 220 ms at Normal quality, x0.6 at Eco and x1.4 at High. The image sources and the diffuse rain are accumulated per
   octave band (8-tap windowed-sinc fractional delays). Each band is weighted by a smooth octave window in the frequency
   domain, so every reflection carries its own spectrum.
2. **Late field.** A feedback delay network whose delays are spread around the room's mean free path (0.5x to 1.6x of
   4V/S / c). Its per-line decay filters are fitted to the room's T60 curve; the fit is weighted so that it is accurate in decay
   time at every frequency. Before use, a probe of the network is measured. Its gain and two shelves are set so that its level
   just after the early part equals the statistical late energy, (4 pi c / V) exp(-13.8 t / T) per band, relative to the direct
   sound at 1 m. The early part fades out over its last quarter while the network fades in.
3. **Modes** (small rooms only). The network is high-passed (4th order) at the crossover, and a bank of resonators supplies the
   bass. The bank's total energy is scaled to the statistical late energy below the crossover.
4. **Direct sound** (Direct dial, off by default). The four source-to-microphone paths, with delay, 1/r, directivity and air.

The output is normalised so that a room's reverb carries unit energy. Rooms of different size are therefore at similar
loudness, while the ratio of direct to reverberant sound, and of early to late sound, stays physical.

## Controls

- **Size**: scales length, width and height.
- **Distance**: from the sources to the listener, along the room. The sources stand about a fifth of the way from the front wall.
- **Furnishing**: how full the room is.
- **Pre-delay**: an extra delay of the reverb only (not the direct sound).
- **Length, Width, Height**: the room, in metres.
- **Spread**: the distance between the left and right sources.
- **Direct**: the level of the direct sound.
- **Early / IR, Tail**: trims of the early part and the late field (0 dB = physical).
- **Walls, Floor, Ceiling, Microphones**: the materials and the microphone pair.
- **Advanced**: modulation (tail only; off in a real room, so small by default), seat offset (left to right), humidity, air temperature.
- **Quality**: early-part length, network size (16, 32, 64 lines) and mode count.
- **Diffusion** off: every reflection is specular (no scattering).
- **Decay dials**: not used. The room sets the decay, and the display shows it.

## Presets

The room presets use published or typical dimensions and surfaces. Each preset's Furnishing is solved by `tools/make_presets.py`
so that its mid-band decay matches the measured or typical reverberation time of that kind of space. The script prints each
room's decay at 125 Hz, 1 kHz, 4 kHz and 8 kHz. Examples:

| Preset | Model | Typical / measured RT (mid) |
|---|---|---|
| Grand Hall | 48.8 x 19.8 x 17.8 m, plaster on lath, seats | 2.0 s (a famous Viennese shoebox hall, occupied) |
| Symphony Hall | 38 x 23 x 18.6 m | 1.9 s |
| Big Studio Live Room | 18.35 x 11.65 x 7.31 m, wood | about 1.2 s |
| Stairwell | 3 x 3 x 14 m, painted concrete | 3.6 to 4.8 s measured |
| Parking Garage | 60 x 40 x 2.6 m, concrete | 2.6 s measured |
| Tiled Bathroom | 2.6 x 2.0 x 2.5 m, glazed tile | 0.6 to 1.0 s |
| Mausoleum | 18 x 18 x 30 m, polished stone | about 15 s |

The Plates, Special, Impulse Response, Cosmic and Vintage presets keep their engines. Sessions saved before this version keep
the mode they were saved with.

## Data and sources

Absorption, 125 Hz to 4 kHz: the long-circulated compilation in Harris (*Handbook of Acoustical Measurements*), Everest &
Pohlmann (*Master Handbook of Acoustics*) and Egan (*Architectural Acoustics*). Checked against published copies for painted
concrete, marble and glazed tile, unglazed brick, plate glass, wood floor, heavy carpet, velour, occupied audience and water;
the other rows are textbook values. The 63 Hz, 8 kHz and 16 kHz values are extrapolated. Sheet metal, natural rock and
bookshelves are estimates (no tabulated source was found).

Scattering: Odeon's mid-frequency guidance (Zeng, Lynge & Rindel, *Applied Acoustics* 2006): smooth surfaces 0.02 to 0.05,
brick 0.1 to 0.2, audience 0.6 to 0.7, bookshelves 0.3. These values are spread over the bands by surface group, rising toward
high frequencies; the per-band values are estimates.

Formulas: ISO 9613-1:1993 (air absorption); Eyring (decay); Kuttruff, *Room Acoustics* (image sources, echo density, modal
damping, mean free path, critical distance); Schroeder (crossover frequency); Lindau, Kosanke & Weinzierl, JAES 60(11), 2012,
and DAGA 2010 (mixing time); Allen & Berkley, JASA 1979 (image method); Jot & Chaigne (FDN decay design).

Hall data: Beranek, *Concert Halls and Opera Houses* (2004). Garage and stairwell: Pomona College sound map measurements.
Mausoleum: published reverberation measurements.

## Limits

- Shoebox rooms only. Fan-shaped halls, domes, balconies and coupled rooms (two spaces with different decays) are not modelled.
- One material for all four walls.
- The sources are omnidirectional.
- The late field is statistical: correct in level, spectrum and decay, not in the exact time pattern of a real room.
- Absorption data at 63 Hz and above 4 kHz is extrapolated.
