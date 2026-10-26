# Solar imagery: what to generate and how it is used

The interface is procedural (vector, any size). Images are optional layers on top of it; the plug-in
runs without them. If you generate them, put the files in `assets/img/` with these names and they are
embedded at the next build.

Licence: anything you generate yourself is fine. NASA imagery (SDO/AIA, SOHO) is public domain and is the
reference look: the AIA 304 angstrom channel shows the chromosphere itself, orange-red with prominences.

| File | Size (px) | Used for | Content |
|---|---|---|---|
| `solar_limb.png` | 2400 x 800, PNG with alpha | Bottom of the main display, 20-30% opacity, blurred further in code | The edge of the Sun seen from above: a dark field, the limb as a soft arc of orange-red light rising from below the frame, a few faint prominences. No stars, no text. Must fade to fully transparent at the top and sides. |
| `corona_bg.png` | 2400 x 1600, PNG | Panel background texture replacement, 6-10% opacity over near-black | Very dark, soft coronal streamers radiating from the lower centre; think long exposure, almost monochrome with a warm tint. Low contrast is essential: it sits behind dials. |
| `dial_cap.png` | 512 x 512, PNG with alpha | Optional: replaces the procedural dial cap | A matte black knob cap seen from above, lit from the top-left, with a faint brushed ring at the edge. No pointer (it is drawn). Transparent corners. |
| `dial_cap@2x.png` | 1024 x 1024 | Same, for HiDPI | Same image at twice the size. |
| `icon_1024.png` | 1024 x 1024 | Plug-in icon / installer | A solar disc with the chromosphere edge visible, on black. Readable at 64 px. |

Colour: background near-black with a warm cast (#0c0b0d to #141315), accent solar orange (#e8873a),
highlights amber (#f2a655) to red-orange (#e0523a), text warm off-white (#ece6dc). Avoid pure yellow and
saturated blue: the palette is warm throughout; the only cool note is the meter grey.

Rules for any image: no text, no logos, no hard edges that would fight the vector layout, and nothing that
reads as "wallpaper": the images are atmosphere, not the subject.

Prompts that worked as a starting point for image generators:
- limb: "edge of the sun viewed from space in extreme ultraviolet, orange-red chromosphere glowing along a soft arc at the bottom of the frame, dark empty space above, faint solar prominences, no stars, cinematic, high resolution"
- corona: "solar corona streamers in very low contrast, nearly black, faint warm highlights, abstract background texture, no sun disc visible"
- icon: "minimal solar disc with a glowing orange-red rim on pure black, centred, high contrast, flat lighting"
