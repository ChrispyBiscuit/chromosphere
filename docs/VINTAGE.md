# Chromosphere vintage suite

Eight reverbs in the manner of hardware from 1970 to 1990. Each is its own topology with the
converter character of its era and three macro controls, on top of the shared Size, Decay,
Pre-delay and Mix. Names are evocative, not trademarks; "in the manner of" is the brief.

| # | Name | After | What it is | Topology | A / B / C |
|---|---|---|---|---|---|
| 1 | Steel Plate | EMT 140 (1957, still on every 70s record) | Dense at once, bright, metallic | Dispersion all-pass chain into a short, dense FDN; no early reflections; damper low-pass on the tail; transducer drive | Damper / Stiffness / Drive |
| 2 | Spring Tank | Accutronics tanks in amps and consoles | The boing | Up to three springs per channel: delay loops holding 24–120 dispersion stages and a low-pass, feedback from the decay time | Springs / Boing / Tension |
| 3 | Digital '76 | EMT 250 | The first digital reverb: soft, lush, slightly grainy | FDN with 12-bit at 24 kHz converters, 6–12 kHz band limit, gentle chorus on the tail | Bandwidth / Bits / Chorus |
| 4 | Concert '78 | Lexicon 224 | The chorused hall of the late 70s | Large FDN with strong slow modulation, bass shelf, 7–18 kHz band limit | Chorus / Bass / Bandwidth |
| 5 | Random Hall | Lexicon 480L | Spin and wander | FDN with random delay modulation whose rate and depth are the controls; shape brings the early build-up forward | Spin / Wander / Shape |
| 6 | Nonlinear '81 | AMS RMX16 | Gated and reverse programs | Four diffusers into a dense multitap burst with a fixed envelope: flat and cut, or rising for reverse | Duration / Density / Reverse |
| 7 | Space Station | Ursa Major SST-282 | The multitap cloud | 32 taps over a span, recirculated through a narrow band; 12-bit grit at the top of Grain | Span / Recirc / Grain |
| 8 | Hall '85 | Yamaha SPX90 | The affordable digital hall | FDN with sparse early pattern, no modulation, 12-bit at 31 kHz down to 9-bit at 15 kHz, coarse input band | Bits / Rate / Grit |

Where they sit: Mode entries 14 to 21, after the cosmic ten. The macro dials appear as a
"Vintage" group whose names change with the unit. Presets: a "Vintage" family, three per unit.

Modules (`dsp/vintage/modules.*`): `Bandwidth` (second-order Butterworth low-pass),
`Quantizer` (zero-order hold at a target rate, rounding to a word length with one LSB of TPDF
dither, a one-pole at half the rate), `DispersionChain` (first-order all-passes in cascade),
`SpringTank`, `Burst` (diffusers, multitap with envelope and optional recirculation).

What the units ignore: Spring Tank, Nonlinear '81 and Space Station have no FDN, so Freeze and
the three-band decay shape do not apply (the spring reads the mid decay time; the burst units
run on Duration and Span). Units whose modulation is part of the character (Digital '76,
Concert '78, Random Hall) do not follow the Mod Depth and Rate dials.

Verification (`tests/test_vintage_engine.cpp`): every unit is block-size independent, finite
and bounded under hostile macro settings, produces a tail that decays; the nonlinear program
holds its window and stops; the quantised units carry a converter noise floor the clean setting
does not; the spring is finite with dispersion active.
