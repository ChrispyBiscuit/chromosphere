# Chromosphere | EXO — front-end design specification

Audit copy. Describes the plug-in editor exactly as implemented on branch `prd-draft-2`
(JUCE 8, C++20). Pixel values are at 1x; the editor is resizable and scales its layout, not its
assets, except where noted. Source of truth: `plugin/StudioLook.*` (palette, type, primitives),
`plugin/PluginEditor.*` (layout, controls), `plugin/Sun.*` (live sun), `plugin/Wordmark.*`,
`plugin/Vfd.*`.

## 1. Product and intent

- A reverb plug-in (VST3 for FL Studio, AAX for Pro Tools) with 13 engines: Algorithmic,
  Convolution, Hybrid, and a "cosmic" suite of ten (Shimmer, Redshift, Corona, Wormhole, Event
  Horizon, Nebula, Pulsar, Solar Wind, Aurora, Supernova).
- Visual brief, in the owner's words: top-tier restraint (FabFilter, Neural DSP Archetype), a
  "space-ship instrument panel" feel, solar colours and imagery. Rejected directions: retro VFD
  skeuomorphism on the whole UI ("cheeseball", "flash game from 2005"), textured scratched or
  pitted metal ("old video game"), cracked-glass overlays. Survived: one VFD element (the preset
  selector), a live sun, restrained matte panel.
- The UI must never load the host's UI thread. All live rendering runs on background threads;
  the message thread only blits.

## 2. Window and scale

- Default size 1080 x 700. Resizable by corner handle; minimum 900 x 600, maximum 1500 x 980.
- Outer margin 22 px all round. Nothing touches the window edge.
- Background: an embedded corona photograph (`corona_bg.jpg`) darkened to near black, cached at
  window size. Reads as a dark field with a warm glow low in the frame; faint star speckle.

## 3. Colour

All colours are opaque sRGB unless noted.

| Token | Hex | Use |
|---|---|---|
| kBg | #121315 | base behind everything |
| kPlate | #1b1c1f | instrument bay surface |
| kPlateEdge | #26282c | plate frame highlight |
| kLine | #2a2c30 | value-arc track, rules |
| kText | #ece6dc | primary text, dial pointer, readouts |
| kTextDim | #9c9893 | section titles, major ticks, caps labels |
| kTextFaint | #5e5a56 | tertiary caps labels, minor ticks, status line |
| kAccent | #e8873a | the one accent: wordmark, value arcs, lamps, active toggles, bass glow |
| kAccentDim | #6e4426 | accent at rest |
| kDisplayBg | #0c0b0d | scope base |
| kGrid | #201d1c | grid (outside the scope) |
| kScopeText | #bdb6ad | scope labels and axis values (local to the scope) |
| kScopeGrid | #3c3733 | scope grid lines |
| kCurve | #f2a655 | decay curve |
| kCurveFill | #e8873a @ 25% | area under the curve |
| kMeter | #c9bfb2 | meter fill |
| kMeterHot | #e8873a | meter top |
| kDanger | #e0523a | errors; right end of the decay-curve gradient |
| VFD phosphor | #8ef3ff | preset selector dots (blue-green ZnO:Zn) |

Rule: orange is the only hue on the panel. Everything else is warm grey. The VFD cyan is the
single deliberate exception, confined to the preset selector so the panel reads as one warm
system with one cold instrument in it.

## 4. Typography

- Face: Barlow (OFL), embedded. Weights used: 500, 600. Fallback: system sans.
- `font(h, weight)`: body text. `caps(h)`: Barlow 500, upper-cased, +12% tracking. All labels on
  the panel are caps().
- Sizes: wordmark 17/600 with +6% tracking; section titles 9.5 caps; dial titles 9.5 caps
  (kTextDim); selector labels 9.5 caps (kTextFaint); readout values 11–14/500 (scales with box
  height); buttons 10–11.5 caps; status line 8.5 caps (kTextFaint); scope labels 9.5 caps; small
  axis values 8 caps.
- Numbers in readouts are centred in a dark window (see 7.3).

## 5. Layout (top to bottom)

### 5.1 Header, 68 px tall (18 px above, 16 px below the row)

Left to right:
1. Brand area, 300 px: live sun emblem (square, row height + 8, expanded 4 px) then the wordmark
   (row height x 0.55, padded 8 px each side for the lens displacement), then the status line
   beneath it: `ENGINE <name> · <sr> kHz · v<version>` in 8.5 caps kTextFaint.
2. 18 px gap.
3. Preset bar fills the middle: `[<] [VFD name field] [>]  Save Delete  [A] [A>B]  Undo Redo`.
   Arrows are square (row height). Buttons: Save 56, Delete 64, A/B square, copy 56, Undo 56,
   Redo 56; 6 px between siblings, 14 px between groups.
4. 14 px gap, then Updates button, 84 px (130 px when a version badge is shown), right-aligned.

### 5.2 Scope (main display)

- Height: 30% of the remaining height (26% in cosmic modes, 24% with Advanced open), clamped to
  140–300 px. Corner radius 8. Base kDisplayBg.
- Three panes separated by 1 px vertical rules: Decay-time graph (left ~52%), Early reflections
  or impulse response (middle), Reverb level history under it, and three vertical meters at
  the right (IN, OUT, TAIL) with caps labels under them.
- Scope brackets: four corner brackets in kTextFaint, 1 px, 10 px legs, inset 3 px.
- Live sun behind the graphs (see 8). Labels and grid use kScopeText / kScopeGrid so they stay
  legible over the sun at any level.
- Decay curve: 2 px, gradient kCurve to kDanger left to right; three draggable handles, 6 px
  rings in kCurve on kDisplayBg fills. Area under the curve: kCurveFill gradient to transparent.

### 5.3 Main plate (instrument bay)

- 16 px below the scope; fills to the bottom margin. Corner radius 10.
- Plate: drop shadow (black 50%, radius 16, offset 0,6), outer frame gradient #2a2c31 to
  #1c1e22, 1 px white 7% highlight, four bolts (r 3.2) inset 5 px in the corners. Inner bay
  inset 10 px, kPlate with a 12% darker top gradient, 10 px dark fall-off along the top edge,
  1 px black 50% outline. Surface: fine procedural grain at 7% opacity, radial vignette to black
  22% in the corners. No other texture.
- Inner content inset 24 px horizontally, 18 px vertically.
- Row 1 (44 px): caps labels `MODE`, `QUALITY` (kTextFaint) over two combo boxes (130–180 px and
  that minus 40), 14 px gaps; on the right, `FREEZE` and `DIFFUSION` toggles (112 / 132 px) with
  lamps.
- Section rule `01  REVERB`: code in 9.5/600 kAccent 85%, title in 9.5 caps kTextDim, then a
  1 px kLine rule to the right edge.
- Row 2: four large dials (Size, Decay, Pre-delay, Mix), equal columns. Rows are sized from
  the bottom up: small-dial row 84–150 px (74–104 in cosmic modes), macro row 82–132 px, the
  big dials take the rest (minimum 96 px). 6 px air above each section rule.
- Dividers: a 1 px black 35% line with a 1 px white 4.5% edge between adjacent dial cells,
  inset 10 px from the top and 6 px from the bottom of the row.
- Section rule `02  LEVELS` (or `02  COSMIC` in cosmic modes).
- Row 3: five small dials (Low decay, High decay, Early/IR, Tail, Output). In cosmic modes this
  row takes 25% and a further row of three larger macro dials (named per engine) sits beneath,
  centred, max 260 px per column.
- Footer (30 px): `ADVANCED` toggle 126 px, `LOAD IR` 80 px, factory-IR combo (≤150 px), IR
  name in the middle, `REVERSE IR` toggle 126 px right-aligned.
- Advanced plate (optional, below, 150–230 px): a second plate, section `03  ADVANCED`, dials in
  rows of six.

## 6. Primitives

### 6.1 Dial (rotary, `drawRotarySlider`)

- Well: circle of diameter D = min(w, h); vertical gradient #0f1012 to #1a1b1e; 1 px black 60%
  edge, 1 px white 5% inner highlight.
- Ring scale: 21 ticks over the 270° sweep; every 5th is major (6 px, kTextDim, 1.4 px), others
  3.5 px, kTextFaint.
- Value arc at radius well − 8.5: track kLine; value kAccent, width max(1.5, D x 0.022), round
  caps; a halo of the same path at 3x width, kAccent 25%.
- Cap: cached rendered image (`dial_cap.png` / 2x), diameter (arc radius − 3.5) x 2.
- Pointer: kText line from 45% to 92% of cap radius, width max(2, cap x 0.045), with a 1 px
  black 50% shadow offset 1 px down.
- Bass glow (15–200 Hz band, 0..1, fast attack, ~250 ms release): ring edge brightens to kAccent
  up to 75%, an inner glow swells inward 2–8 px, major/minor ticks warm toward kAccent (70% at
  full), value-arc halo widens to 5x and rises to 70%.
- Title: 9.5 caps kTextDim above (15 px), 2 px, dial, 6 px, readout (see 6.3, 20 px tall).
- Focus ring: 1 px kAccent 80% circle 2 px outside the well.
- Disabled: everything at 50%.

### 6.2 Toggle (`drawToggleButton`)

A two-way lever switch: round bezel (radius ≤ 10, gradient #45484f to #1c1e22, 1 px white 8%
rim, dark throat at 62%), a tapered chrome bat handle thrown 54° left (off) or right (on) with a
ball tip, 1 px shadow; caps label to the right; a 7 px lamp at the far right, kAccent lit when
on. Focus: kAccent 70% ring around the bezel.

### 6.3 Readout window

A VFD (see 7.3) 76–112 x 20 px showing the value centred, dot pitch down to 1.2 px so values
never scroll. Double-click opens the plain text editor over it with a kAccent outline.

### 6.4 Buttons and combos

Buttons: filled #232529 with 1 px white 8% edge, radius 5, caps label kTextDim; hover kText and
white 14% edge; down #0a0b0c. Primary (Save) same but label kText. Combo boxes (Mode, Quality,
Factory IR) are VFDs: the chosen entry left-aligned, a dim down-arrow glyph at the right end,
pitch down to 1.6 px before scrolling; popup menu kPlate with kAccent highlight.

### 6.5 Section header

`drawSection`: code (two digits) in kAccent, title in caps kTextDim, hairline rule in kLine.

## 7. Header elements

### 7.1 Sun emblem (left)

A live rendered sun, Disc mode (see 8), square. Background behind it is the window texture.

### 7.2 Wordmark

`CHROMOSPHERE | EXO` in Barlow 17/600, +6% tracking, kAccent, one colour, one word-group.
Rendered through a software lens at 2x and drawn down:
- Lateral chromatic aberration: red/blue channel separation growing with distance from the
  word's centre, ≈1 px at the ends.
- Spherical aberration: barrel 0.006 r², horizontal softening from a ±1.6 px tap mixed up to
  50% at the edges.
- Glitch: every 2.5–9.5 s (sooner with signal), for 60–180 ms, 2–3 horizontal bands shift
  ±3.5 px and the fringe triples, eased in and out.
- Cursor: `_` after the word, kAccent, alpha breathing 0.15–1.0 on a 1.2 s smooth-step cycle.
- 30 fps timer, CPU, ~1 ms per frame.

### 7.3 Preset selector (VFD)

The one skeuomorphic element. Realistic vacuum fluorescent display:
- Glass: #0b1216 to #04080a vertical gradient, radius 5, 1 px black 80% outer, 1 px white 6%
  inner bevel; reflection band white 7% to 0 over the top 45%; 24 px edge darkening to black
  35% left and right.
- Matrix: 5x7 dots per character on a 6x8 cell, pitch ≤ 3.2 px (shrinks to 1.9 px to fit, then
  marquees: 1.8 s hold, crawl 24 px/s, 1.5 s hold, snap back). Every dot the display could light
  is drawn unlit at 5% phosphor.
- Lit dot: bloom radial gradient 28% out to 1.9x dot, dot core #8ef3ff, hot centre #eefdff 75%
  at half size; plus a 9-tap Gaussian blurred copy of the lit layer under it at 70%.
- Anode mesh in front: horizontal 0.6 px black 16% every 2 px, vertical 0.6 px black 8% every
  2 px.
- Flicker: brightness wanders 93–100% with occasional dips, 30 fps.
- Content: category (dim, 42%) left-aligned when it fits; preset name bright, centred.
- Arrow buttons: same glass with a single 5x7 triangle glyph. Hover: white 10% ring; down: black
  25% wash. Name field: focus kAccent 80% ring.

### 7.4 Updates button

Standard button; a version badge widens it to 130 px when a newer version is known. Opens a
call-out panel (470 x 254, kPlate) with: opt-in check toggle, automatic-install toggle (Windows),
GPU rendering toggle (off by default), status line, `Check now / Download / Run installer /
Skip this version`.

## 8. The live sun (`plugin/Sun.*`)

Shared model, two views:
- Disc (emblem): full sphere, radius 40% of the square.
- Limb (scope): a sphere of radius 0.95 x width whose top rises 30% into the scope from below;
  atmosphere distances measured against the scope height (falloff = R / 0.45 h); intensity 65%
  so readings stay legible; radiation layer on.

Surface: rotating 3-D value-noise granulation (4+3 octaves on GPU, 3+2 on CPU), limb darkening
z^0.45, colour ramp deep #6b1405 → orange #ff6b17 → hot #ffd98c. Corona: exp fall-off with
turbulence, prominence arcs. Particles (≤48) ejected from the rim, pulled back. Sunspots (≤12,
from the bass band, active latitudes, grow 1.2 s / hold / fade 2.5 s), flares (≤6, mostly from
spots, from level), coronal mass ejections (≤6, lobes with a bright front, from level).
Radiation layer (scope only): per-frame grain, single-pixel hot pixels (density 0.04%–0.34% with
level), up to seven ray tracks per third of a second.

Drive: energy follows level (attack 40 ms, release 1.6 s); bass follows the 15–200 Hz band
(attack 120 ms, release 2.5 s). Level is −54..−6 dBFS mapped to 0..1 from the larger of tail
peak and 70% of output peak.

Rendering: default CPU on a low-priority thread per view (scope at 0.35x, emblem at 0.75x, 20/24
fps), blitted with high-quality resampling; OpenGL fragment shader at full resolution is opt-in
(timer-driven 30 fps, swap interval 0, component painting only on the scope) because OpenGL
inside some hosts has hung whole systems.

## 9. Motion and timing summary

| Element | Rate | Thread |
|---|---|---|
| Scope graphs, meters | 30 fps repaint | message |
| Sun (CPU) | 20 fps scope, 24 fps emblem | background |
| Sun (GPU, opt-in) | 30 fps | GL thread |
| Wordmark lens | 30 fps | message |
| VFD flicker / marquee | 30 fps | message (cached image, re-rendered only while scrolling) |
| Dial bass glow | 30 fps, repaint only on change > 0.4% | message |
| Engine crossfade on mode change | 150 ms | audio |

## 10. Accessibility and states

- Every control has a title and description for screen readers (JUCE accessibility).
- Keyboard focus: kAccent 80% ring. Tab order follows reading order.
- Disabled controls at 50%; IR controls disabled outside convolution/hybrid; cosmic macros hidden
  outside cosmic modes; Advanced plate hidden unless toggled.
- Hover is subtle (white 10–14% edges); nothing moves on hover.
- No text below 8 px caps; axis values are the smallest text.

## 11. Known open questions for the audit

- Header density at the 900 px minimum: the preset field drops to ~140 px and marquees
  12-character names.
- The VFD cyan against the orange system: intentional single cold instrument; confirm it does
  not read as a second accent.
- Scope at full signal: confirm the sun plus radiation never competes with the decay curve.
- Wordmark glitch frequency: confirm "subtle" at 2.5–9.5 s.
- Emblem GL/CPU tint: the corona reaches the emblem's bounds and tints the texture by 1–3
  levels, visible only at 600% zoom.
