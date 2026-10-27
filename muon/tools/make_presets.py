"""Factory presets for Chromosphere | MUON (writes ../data/presets.json).

Every preset is written out below, one by one: a name, a category, a one-line description, tags, the parameters that
differ from the defaults, and the test material it is checked on (muon_preset_render). The four macros keep one meaning
in every preset (PRD 4.8):
    VELOCITY  the arrival: attack amounts, transient clip, punch
    MASS      weight: low-band sustain, sub and phantom, low orders (H2, H3) on the low band
    CHARGE    energy and brightness: drive, high orders, high-band attack, tone
    DECAY     what is left: sustain amounts, resonator decay and mix, Afterglow send and decay
Assignments are derived from what each preset uses (std_macros), with per-preset overrides where a preset needs a
macro to do something particular.

Levels: `--calibrate measured.json` reads the loudness change muon_preset_render measured for each preset (auto-gain
off) and folds a correction into its Output so the preset sits within 1 dB of the input. Run render, calibrate, and
repeat two or three times.
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "data", "presets.json")
LEVELS = os.path.join(HERE, "preset_levels.json")

PRESETS = []


def band_count(p):
    return int(p.get("bands", "3"))


def std_macros(p, scale=1.0):
    """The fixed macro meanings, applied to the bands and stages the preset uses."""
    n = band_count(p)
    m = []
    def add(macro, pid, amt):
        m.append({"macro": macro, "id": pid, "amount": round(max(-1.0, min(1.0, amt * scale)), 3)})
    for b in range(1, n + 1):
        x = f"b{b}"
        add(0, x + "Atk", 0.3 if b < n else 0.4)                     # Velocity
        add(3, x + "Sus", 0.3 if b > 1 else 0.2)                     # Decay
        add(2, x + "Drv", 0.25 if p.get(x + "Drv", 0) else 0.15)     # Charge
    add(0, f"b{n}Clip", 0.3)
    add(1, "b1Sus", 0.25)                                            # Mass
    add(1, "sub", 0.35)
    add(1, "phantom", 0.25)
    add(1, "b1Harm", 0.3)
    add(2, f"b{n}Harm", 0.3)
    add(2, f"b{n}Tone", 0.2)
    add(3, "resDecay", 0.3)
    add(3, "resMix", 0.3)
    add(3, "glowSend", 0.3)
    add(3, "glowDecay", 0.25)
    return m


def P(name, cat, desc, tags, material, params, macros=None, scale=1.0):
    p = dict(params)
    p.setdefault("clipGuard", True)
    PRESETS.append({"name": name, "category": cat, "description": desc, "tags": tags, "material": material,
                    "author": "Chromosphere", "params": p, "macros": macros if macros is not None else std_macros(p, scale)})


# ---------------------------------------------------------------------------------------------------------------
# Kick
K = "Kick"
P("Muon Strike", K, "Tight front, sustain held back, a little sub for weight: the default kick fixer.", ["punch", "tight"], "kick",
  {"bands": "2", "xLow": 120, "b1Atk": 0.45, "b1Sus": -0.2, "b2Atk": 0.6, "b2Sus": -0.35, "sub": 0.15, "subRange": 120})
P("Air Shower Kick", K, "Long boom: low sustain up, the click softened, tuned ring an octave below the root.", ["boom", "808"], "kick",
  {"bands": "2", "xLow": 140, "b1Sus": 0.55, "b2Atk": -0.2, "resMode": "Tuned", "chord": "Octaves", "root": 33, "resDecay": 0.8, "resMix": 0.2, "strike": 0.7})
P("Pion Click", K, "All beater: the high band's attack pushed and clipped for a hard click that cuts on phones.", ["click", "beater"], "kick",
  {"bands": "2", "xLow": 160, "b2Atk": 0.9, "b2Clip": 0.45, "b2Spd": 0.2, "b1Sus": -0.15, "b2Curve": "Diode", "b2Drv": 0.2})
P("Cherenkov Thump", K, "Low-band punch with tube warmth on the attack only: round, dense, forward.", ["warm", "round"], "kick",
  {"bands": "2", "xLow": 110, "b1Atk": 0.55, "b1Src": "Transient", "b1Curve": "Tube", "b1Drv": 0.35, "b1Sus": -0.1})
P("Sub Lepton", K, "An octave-down generator under the kick, clean purity, mono below 120 Hz.", ["sub", "weight"], "kick",
  {"bands": "2", "xLow": 120, "sub": 0.45, "subPur": 0.95, "subRange": 110, "mono": 120, "b1Atk": 0.25})
P("Tuned Impact", K, "The kick rings at the song's root: tuned resonators on octaves, struck by the transient.", ["tuned", "ring"], "kick",
  {"bands": "2", "xLow": 130, "b1Atk": 0.35, "resMode": "Tuned", "chord": "Octaves", "root": 36, "resDecay": 0.45, "resMix": 0.35, "strike": 0.8})
P("Phantom Kick", K, "Harmonics 2 to 4 of the low end so the kick is heard on small speakers; the fundamental trimmed.", ["phones", "translation"], "kick",
  {"bands": "2", "xLow": 120, "phantom": 0.55, "phCut": 0.25, "b1Atk": 0.2})
P("Gated Boom", K, "Big attack, sustain cut hard: a short, gated thud that leaves room for the bass.", ["short", "gated"], "kick",
  {"bands": "2", "xLow": 140, "b1Atk": 0.5, "b1Sus": -0.8, "b2Sus": -0.7, "b2Atk": 0.4})
P("Skin Kick", K, "A membrane body under the kick: the Skin material struck a fifth above the root.", ["body", "acoustic"], "kick",
  {"bands": "2", "xLow": 140, "b1Atk": 0.3, "resMode": "Body", "material": "Skin", "root": 40, "size": 0.45, "resDecay": 0.35, "resMix": 0.3})
P("Distant Muon", K, "Softened attack, a lifted tail and a short Afterglow: the kick from the back of the room.", ["room", "soft"], "kick",
  {"bands": "2", "xLow": 120, "b1Atk": -0.3, "b2Atk": -0.4, "b1Sus": 0.3, "glowSrc": "Full", "glowSend": 0.3, "glowDecay": 0.9, "glowSize": 0.7, "glowMix": 0.4})

# Snare and clap
S = "Snare & Clap"
P("Crack Front", S, "The top band's snap pushed and slightly clipped; the body left alone.", ["crack", "snap"], "snare",
  {"xLow": 200, "xHigh": 3500, "b3Atk": 0.7, "b3Clip": 0.3, "b2Atk": 0.25})
P("Fat Body", S, "Mid-band sustain up with tube warmth: the snare's body and wires grow.", ["body", "fat"], "snare",
  {"xLow": 180, "xHigh": 2500, "b2Sus": 0.5, "b2Curve": "Tube", "b2Drv": 0.3, "b1Sus": 0.2})
P("Skin Ring", S, "A struck membrane tuned near the drum: adds a tone to a dead snare.", ["tone", "ring"], "snare",
  {"xLow": 200, "xHigh": 3000, "resMode": "Body", "material": "Skin", "root": 55, "size": 0.5, "resDecay": 0.3, "resMix": 0.35, "strike": 0.7})
P("Clap Spread", S, "The clap's transients widened by the resonance's stereo and a short room.", ["clap", "wide"], "snare",
  {"xLow": 250, "xHigh": 4000, "b3Atk": 0.4, "resMode": "Comb", "root": 60, "resDecay": 0.15, "resMix": 0.2, "spread": 0.6, "resStereo": 1.0,
   "glowSrc": "Transients", "glowSend": 0.3, "glowDecay": 0.8, "glowSize": 0.6})
P("Tight Rim", S, "Short and dry: sustain down on every band, attack up on the top.", ["dry", "tight"], "snare",
  {"xLow": 200, "xHigh": 3000, "b1Sus": -0.5, "b2Sus": -0.6, "b3Sus": -0.5, "b3Atk": 0.4})
P("Brushed Air", S, "Attack softened, the top's sustain and air lifted: a brush-like snare.", ["soft", "air"], "snare",
  {"xLow": 250, "xHigh": 5000, "b2Atk": -0.4, "b3Atk": -0.3, "b3Sus": 0.4, "b3Curve": "Tape", "b3Drv": 0.2, "b3Tone": 0.7})
P("Metal Snare", S, "A plate of metal struck by the snare: an industrial clang under the hit.", ["metal", "industrial"], "snare",
  {"xLow": 200, "xHigh": 3000, "resMode": "Body", "material": "Metal", "root": 62, "size": 0.4, "resDecay": 0.5, "resMix": 0.35, "strike": 0.8})
P("Room Burst", S, "Transients sent into a short Afterglow and the tail lifted: a snare in a live room.", ["room", "live"], "snare",
  {"xLow": 200, "xHigh": 3000, "b2Sus": 0.3, "glowSrc": "Transients", "glowSend": 0.45, "glowDecay": 1.1, "glowSize": 0.8, "glowMix": 0.5})
P("Crushed Snare", S, "Diode drive and clip on the body and top for a trashy, compressed crack.", ["crushed", "lofi"], "snare",
  {"xLow": 200, "xHigh": 3000, "b2Curve": "Diode", "b2Drv": 0.5, "b3Clip": 0.6, "b3Atk": 0.5, "b2Sus": 0.3})
P("Octave Snap", S, "The top band's transients an octave up (H2 only): a brighter, synthetic snap.", ["bright", "synthetic"], "snare",
  {"xLow": 200, "xHigh": 3000, "b3Src": "Transient", "b3H2": 0.7, "b3Harm": 0.5, "b3Atk": 0.3})

# Toms and percussion
T = "Toms & Perc"
P("Tom Bloom", T, "Tom sustain grows and rings at its pitch, tracked from the low band.", ["toms", "ring"], "toms",
  {"xLow": 150, "xHigh": 2500, "b1Sus": 0.4, "b2Sus": 0.3, "resMode": "Tuned", "pitch": "Track", "chord": "Unison", "resDecay": 0.6, "resMix": 0.3})
P("Tom Gate", T, "Short toms: sustain cut so fills stay clear.", ["toms", "short"], "toms",
  {"xLow": 150, "xHigh": 2500, "b1Sus": -0.6, "b2Sus": -0.6, "b3Sus": -0.4, "b2Atk": 0.3})
P("Wood Block", T, "Hits strike a wooden bar: marimba-like body under any percussion.", ["wood", "tuned"], "perc",
  {"resMode": "Body", "material": "Wood", "root": 60, "size": 0.5, "resDecay": 0.4, "resMix": 0.45, "strike": 0.8, "b3Atk": 0.3})
P("Glass Ping", T, "Struck glass, slowly beating partials: delicate rings from hand percussion.", ["glass", "bell"], "perc",
  {"resMode": "Body", "material": "Glass", "root": 72, "size": 0.5, "resDecay": 1.4, "resMix": 0.35, "strike": 0.7, "resStereo": 0.8})
P("Conga Snap", T, "The slap of hand drums brought forward, the body tightened.", ["hand drums", "snap"], "perc",
  {"xLow": 200, "xHigh": 2500, "b2Atk": 0.5, "b3Atk": 0.6, "b1Sus": -0.3, "b2Sus": -0.2})
P("Tube Toms", T, "Toms through a tube curve on the body: fuller, rounder, a little louder in the mix.", ["warm", "toms"], "toms",
  {"xLow": 150, "xHigh": 2500, "b1Curve": "Tube", "b1Drv": 0.3, "b2Curve": "Tube", "b2Drv": 0.35, "b1Sus": 0.2})
P("Steel Pan Hit", T, "A metal plate tuned to a major chord: hits become steel-drum notes.", ["metal", "tuned"], "perc",
  {"resMode": "Tuned", "chord": "Major", "root": 62, "resDecay": 0.7, "resMix": 0.45, "strike": 0.9, "spread": 0.2})
P("Shaker Shape", T, "Shakers and tambourines: the attack lifted on the top band only, sustain trimmed.", ["shaker", "top"], "perc",
  {"bands": "2", "xLow": 2000, "b2Atk": 0.5, "b2Sus": -0.3})
P("Tabla Tone", T, "Tracked pitch with a fifth above: percussion that sings its own note.", ["tuned", "tabla"], "perc",
  {"resMode": "Tuned", "pitch": "Track", "chord": "Fifths", "resDecay": 0.5, "resMix": 0.35, "strike": 0.7})

P("Cowbell Clank", T, "Metal body tuned high with a short decay: any hit becomes a clank.", ["metal", "cowbell"], "perc",
  {"resMode": "Body", "material": "Metal", "root": 76, "size": 0.35, "resDecay": 0.25, "resMix": 0.4, "strike": 0.9})

# Hats and cymbals
H = "Hats & Cymbals"
P("Hat Tick", H, "Hi-hat transients sharpened, the wash pulled down.", ["hats", "tight"], "hats",
  {"bands": "2", "xLow": 4000, "b2Atk": 0.5, "b2Sus": -0.5})
P("Cymbal Wash", H, "Ride and crash sustain lifted, attack softened: a longer, smoother wash.", ["cymbals", "wash"], "hats",
  {"bands": "2", "xLow": 3000, "b2Atk": -0.3, "b2Sus": 0.5})
P("Tape Hats", H, "Tape curve on the top band: hats lose the fizz and gain density.", ["tape", "smooth"], "hats",
  {"bands": "2", "xLow": 3000, "b2Curve": "Tape", "b2Drv": 0.4, "b2Tone": 0.4})
P("Sizzle", H, "High-order harmonics on the transients only: air and sparkle on dull hats.", ["air", "exciter"], "hats",
  {"bands": "2", "xLow": 5000, "b2Src": "Transient", "b2H3": 0.4, "b2H5": 0.2, "b2Harm": 0.4, "os": "8x"})
P("Short Hats", H, "Closed hats made shorter still: sustain cut and lookahead on the attack.", ["short", "closed"], "hats",
  {"bands": "2", "xLow": 5000, "b2Sus": -0.8, "b2Atk": 0.3, "look": "1 ms"})
P("Metal Shimmer", H, "Metal resonators high in the range ring under the cymbals.", ["metal", "shimmer"], "hats",
  {"resMode": "Body", "material": "Metal", "root": 84, "size": 0.6, "resDecay": 1.5, "resMix": 0.25, "resStereo": 1.0})
P("Soft Hats", H, "Spiky programmed hats tamed: attack down, a little drive for body.", ["soft", "tame"], "hats",
  {"bands": "2", "xLow": 4000, "b2Atk": -0.5, "b2Curve": "Soft", "b2Drv": 0.2})
P("Rectified Hats", H, "Full-wave rectified top: hats an octave of grit higher.", ["grit", "fx"], "hats",
  {"bands": "2", "xLow": 4000, "b2Curve": "Rectify", "b2Bias": 1.0, "b2Drv": 0.3, "b2DMix": 0.4})

P("Open Hat Tame", H, "Open hats shorter and softer: attack down, sustain down, a darker top.", ["open", "tame"], "hats",
  {"bands": "2", "xLow": 4000, "b2Atk": -0.3, "b2Sus": -0.4, "b2Curve": "Tape", "b2Drv": 0.15, "b2Tone": 0.35})
P("Ride Ping", H, "The ride's ping forward and a metal ring under it.", ["ride", "ping"], "hats",
  {"bands": "2", "xLow": 3000, "b2Atk": 0.45, "resMode": "Body", "material": "Metal", "root": 88, "resDecay": 0.8, "resMix": 0.2})

# Drum bus
D = "Drum Bus"
P("Bus Glue", D, "Gentle attack lift on all bands with tube warmth: the starting point for a drum bus.", ["glue", "bus"], "drums",
  {"b1Atk": 0.2, "b2Atk": 0.25, "b3Atk": 0.2, "b1Curve": "Tube", "b1Drv": 0.15, "b2Curve": "Tube", "b2Drv": 0.15, "b3Curve": "Tube", "b3Drv": 0.1})
P("Punch Bus", D, "More attack, less sustain, lookahead: drums jump out.", ["punch", "bus"], "drums",
  {"b1Atk": 0.45, "b2Atk": 0.5, "b3Atk": 0.4, "b1Sus": -0.2, "b2Sus": -0.3, "look": "2 ms"})
P("Smash Bus", D, "Clipped transients and sustain up on the mid band: room-mic smash in one instance.", ["smash", "aggressive"], "drums",
  {"b2Atk": 0.5, "b2Clip": 0.6, "b2Sus": 0.5, "b3Clip": 0.5, "b2Curve": "Diode", "b2Drv": 0.35, "mix": 0.7})
P("Parallel Crush", D, "Heavy drive and sustain on the wet signal, mixed under the dry.", ["parallel", "crush"], "drums",
  {"b1Drv": 0.6, "b2Drv": 0.7, "b3Drv": 0.5, "b2Sus": 0.6, "b3Sus": 0.4, "mix": 0.4})
P("Clipper Bus", D, "Transient clip on every band: peaks shaved for level without dulling the hits.", ["clip", "loud"], "drums",
  {"b1Clip": 0.4, "b2Clip": 0.5, "b3Clip": 0.5, "b1Atk": 0.2, "b2Atk": 0.2})
P("Tape Bus", D, "Tape curve on every band, a softer top: drums from a cassette deck.", ["tape", "warm"], "drums",
  {"b1Curve": "Tape", "b1Drv": 0.3, "b2Curve": "Tape", "b2Drv": 0.35, "b3Curve": "Tape", "b3Drv": 0.3, "b3Tone": 0.35, "b3Atk": -0.15})
P("Breakbeat Snap", D, "For sampled breaks: top-band snap, low-band weight, a little phantom bass.", ["breaks", "sample"], "drums",
  {"b3Atk": 0.55, "b1Sus": 0.2, "phantom": 0.25, "b2Atk": 0.3})
P("Dry Bus", D, "Sustain trimmed on every band: drums as if the room were smaller.", ["dry", "tight"], "drums",
  {"b1Sus": -0.3, "b2Sus": -0.45, "b3Sus": -0.35})
P("Room Bloom", D, "Sustain up and a short Afterglow from the transients: drums in a bigger room.", ["room", "big"], "drums",
  {"b2Sus": 0.4, "b3Sus": 0.3, "glowSrc": "Transients", "glowSend": 0.3, "glowDecay": 1.4, "glowSize": 1.0, "glowMix": 0.5})
P("Mid/Side Snap", D, "Mid/side mode: the centre (kick, snare) snaps, the sides stay as they are.", ["mid/side", "punch"], "drums",
  {"stereo": "Mid/Side", "b1Atk": 0.4, "b2Atk": 0.5, "b3Atk": 0.3})
P("Lo-Fi Bus", D, "Rectified and folded drive, the top darkened: a dusty sampler bus.", ["lofi", "dusty"], "drums",
  {"b2Curve": "Fold", "b2Drv": 0.3, "b3Curve": "Rectify", "b3Drv": 0.2, "b3DMix": 0.4, "b3Tone": 0.25, "mix": 0.8})

# Room and overheads
R = "Room & Overheads"
P("Room Lift", R, "Room mics: sustain up on the low and mid bands for more space.", ["room", "space"], "drums",
  {"b1Sus": 0.4, "b2Sus": 0.5, "b3Sus": 0.2})
P("Overhead Focus", R, "Overheads: snare and cymbal transients forward, the wash down.", ["overheads", "focus"], "drums",
  {"b2Atk": 0.4, "b3Atk": 0.3, "b3Sus": -0.3})
P("Ambience Pull", R, "Spill and ambience reduced: sustain cut, transients kept.", ["dry", "spill"], "drums",
  {"b1Sus": -0.4, "b2Sus": -0.5, "b3Sus": -0.5, "sens": 0.7})
P("Crunchy Room", R, "Room mics driven into a tube curve and pulled back in parallel.", ["crunch", "room"], "drums",
  {"b1Curve": "Tube", "b1Drv": 0.45, "b2Curve": "Tube", "b2Drv": 0.55, "b2Sus": 0.4, "mix": 0.6})
P("Glass Room", R, "A long Afterglow fed only by the transients: a reverb that follows the hits.", ["reverb", "transient"], "drums",
  {"glowSrc": "Transients", "glowSend": 0.5, "glowDecay": 2.6, "glowSize": 1.6, "glowDamp": 0.5, "glowMix": 0.5})
P("Mono Room", R, "Low end made mono below 150 Hz, room sustain up.", ["mono", "room"], "drums",
  {"mono": 150, "b1Sus": 0.3, "b2Sus": 0.3})
P("Close Mic", R, "Overheads made closer: attack up, sustain down, lookahead.", ["close", "dry"], "drums",
  {"b2Atk": 0.35, "b3Atk": 0.35, "b2Sus": -0.35, "b3Sus": -0.3, "look": "1 ms"})
P("Dark Ambience", R, "Room sustain up with a darker top: a softer, warmer space.", ["dark", "warm"], "drums",
  {"b1Sus": 0.35, "b2Sus": 0.35, "b3Atk": -0.2, "b3Curve": "Tape", "b3Drv": 0.2, "b3Tone": 0.25})

P("Snare Mic Spill", R, "Snare bottom and spill cleaned: sustain down on the low and mid bands, lookahead.", ["spill", "clean"], "snare",
  {"b1Sus": -0.5, "b2Sus": -0.4, "look": "1 ms"})

# Bass and 808
B = "Bass & 808"
P("Bass Weight", B, "Low-band sustain and H2 on the low end: a fuller bass that holds its level.", ["weight", "warm"], "bass",
  {"bands": "2", "xLow": 200, "b1Sus": 0.3, "b1H2": 0.35, "b1Harm": 0.4})
P("Phone Bass", B, "Phantom harmonics with the fundamental trimmed: the bass line reads on small speakers.", ["phones", "translation"], "bass",
  {"bands": "2", "xLow": 200, "phantom": 0.6, "phCut": 0.3, "mono": 120})
P("Pick Attack", B, "Plucks and picks brought forward on the upper band.", ["pick", "attack"], "bass",
  {"bands": "2", "xLow": 250, "b2Atk": 0.6, "b2Spd": 0.25})
P("Octave Under", B, "A clean octave down under the bass line, mono.", ["sub", "octave"], "bass",
  {"bands": "2", "xLow": 200, "sub": 0.5, "subPur": 0.95, "subRange": 160, "mono": 120})
P("Growl", B, "Upper-band fold drive and H3 on the low band: an aggressive, harmonic bass.", ["growl", "aggressive"], "bass",
  {"bands": "2", "xLow": 220, "b2Curve": "Fold", "b2Drv": 0.45, "b1H3": 0.4, "b1Harm": 0.4, "b2Atk": 0.3})
P("808 Glide", B, "808s: sustain up, H2 for audibility, a deep octave below for the tail.", ["808", "long"], "bass",
  {"bands": "2", "xLow": 180, "b1Sus": 0.45, "b1H2": 0.4, "b1Harm": 0.45, "sub": 0.25, "subRange": 120})
P("808 Punch", B, "808s with a stronger front: the click and first cycle pushed, the tail held.", ["808", "punch"], "bass",
  {"bands": "2", "xLow": 160, "b1Atk": 0.5, "b2Atk": 0.6, "b1Sus": -0.1, "b2Clip": 0.3})
P("Transformer Low", B, "Transformer recipe on the low band: H2 and H3 for iron-like weight.", ["iron", "warm"], "bass",
  {"bands": "2", "xLow": 200, "b1H2": 0.3, "b1H3": 0.4, "b1Harm": 0.5, "b1Curve": "Tube", "b1Drv": 0.2})
P("Synth Bass Edge", B, "High band tape drive and attack: synth basses with more edge.", ["synth", "edge"], "bass",
  {"bands": "2", "xLow": 300, "b2Curve": "Tape", "b2Drv": 0.4, "b2Atk": 0.3, "b2Tone": 0.65})
P("Upright Body", B, "Wood resonance tracked from the bass: an acoustic body under DI bass.", ["upright", "wood"], "bass",
  {"bands": "2", "xLow": 250, "resMode": "Body", "material": "Wood", "pitch": "Track", "size": 0.5, "resDecay": 0.4, "resMix": 0.25, "resIn": "Full", "strike": 0.4})
P("Two Octaves Down", B, "The deepest sub: two octaves below, clean, mono.", ["sub", "deep"], "bass",
  {"bands": "2", "xLow": 200, "sub2": 0.45, "subPur": 1.0, "subRange": 160, "mono": 120})

# Guitar
G = "Guitar"
P("Pick Snap", G, "Pick attack forward on the mid and top bands.", ["pick", "attack"], "guitar",
  {"xLow": 200, "xHigh": 3000, "b2Atk": 0.4, "b3Atk": 0.5})
P("Sustainer", G, "Sustain up on every band: notes ring longer, like a sustainer pickup.", ["sustain", "long"], "guitar",
  {"xLow": 200, "xHigh": 3000, "b1Sus": 0.4, "b2Sus": 0.6, "b3Sus": 0.4})
P("Tube Strum", G, "Tube curve on the mid band, a little on the top: warmer strummed acoustics.", ["warm", "acoustic"], "guitar",
  {"xLow": 200, "xHigh": 3500, "b2Curve": "Tube", "b2Drv": 0.35, "b3Curve": "Tube", "b3Drv": 0.2})
P("Fuzz Face", G, "Asymmetric soft drive with bias on the mid band: a classic fuzz colour.", ["fuzz", "drive"], "guitar",
  {"xLow": 200, "xHigh": 3000, "b2Curve": "Soft", "b2Drv": 0.7, "b2Bias": 0.5, "b2Tone": 0.4})
P("Octave Fuzz", G, "Full-wave rectified mid band: an octave-up fuzz.", ["octave", "fuzz"], "guitar",
  {"xLow": 200, "xHigh": 3000, "b2Curve": "Rectify", "b2Bias": 1.0, "b2Drv": 0.5, "b2DMix": 0.6})
P("Clean Clarity", G, "Body sustain trimmed, presence harmonics on the top: clearer clean guitars.", ["clean", "clarity"], "guitar",
  {"xLow": 250, "xHigh": 3000, "b1Sus": -0.2, "b3H2": 0.3, "b3Harm": 0.3})
P("Sympathetic Strings", G, "Resonators on the song's scale ring with the playing, like sympathetic strings.", ["sitar", "strings"], "guitar",
  {"resMode": "Tuned", "pitch": "Key", "scale": "Minor", "root": 45, "resDecay": 1.6, "resMix": 0.3, "resIn": "Full", "strike": 0.35, "resStereo": 0.8})
P("Palm Mute Tight", G, "Palm mutes tightened: sustain cut on the low and mid bands.", ["metal", "tight"], "guitar",
  {"xLow": 200, "xHigh": 3000, "b1Sus": -0.5, "b2Sus": -0.4, "b2Atk": 0.3})
P("Shimmer Guitar", G, "An octave of harmonics and a long Afterglow from the full signal.", ["ambient", "shimmer"], "guitar",
  {"xLow": 250, "xHigh": 3000, "b3H2": 0.5, "b3Harm": 0.4, "glowSrc": "Full", "glowSend": 0.4, "glowDecay": 4.0, "glowSize": 2.0, "glowMix": 0.4})

P("Slide Resonance", G, "Resonators follow the tracked pitch an octave apart: slide guitar with a glowing halo.", ["slide", "tracked"], "guitar",
  {"resMode": "Tuned", "pitch": "Track", "chord": "Octaves", "resDecay": 1.2, "resMix": 0.3, "resIn": "Full", "strike": 0.4})

# Vocal
V = "Vocal"
P("Vocal Presence", V, "H2 on the mid band, a gentle tape top: a vocal that sits forward.", ["presence", "forward"], "vocal",
  {"xLow": 250, "xHigh": 4000, "b2H2": 0.3, "b2Harm": 0.35, "b3Curve": "Tape", "b3Drv": 0.2})
P("De-Breath", V, "Body sustain down a little: breaths and room between words recede.", ["clean", "breath"], "vocal",
  {"xLow": 250, "xHigh": 4000, "b2Sus": -0.3, "b3Sus": -0.35, "sens": 0.6})
P("Consonant Lift", V, "Consonants and plosive fronts brought up on the top band.", ["diction", "clarity"], "vocal",
  {"xLow": 250, "xHigh": 3500, "b3Atk": 0.45, "b3Spd": 0.3})
P("Soft Esses", V, "The top band's transients softened: a gentle de-esser by shape.", ["de-ess", "soft"], "vocal",
  {"xLow": 250, "xHigh": 5000, "b3Atk": -0.45})
P("Tube Vocal", V, "Tube curve on the mid and low bands: warmth and density.", ["warm", "tube"], "vocal",
  {"xLow": 250, "xHigh": 4000, "b1Curve": "Tube", "b1Drv": 0.3, "b2Curve": "Tube", "b2Drv": 0.3})
P("Radio Voice", V, "Lows muted, diode drive in the middle: a small-speaker, telephone voice.", ["radio", "lofi"], "vocal",
  {"xLow": 400, "xHigh": 3000, "b1Mute": True, "b3Mute": True, "b2Curve": "Diode", "b2Drv": 0.45})
P("Vocal Air", V, "High-order harmonics and sustain on the top band: breathy air.", ["air", "bright"], "vocal",
  {"xLow": 250, "xHigh": 6000, "b3H3": 0.3, "b3H2": 0.3, "b3Harm": 0.3, "b3Sus": 0.2, "os": "8x"})
P("Choir Ring", V, "Resonators on the scale, struck softly by the full signal: a halo of pitches.", ["choir", "halo"], "vocal",
  {"resMode": "Tuned", "pitch": "Key", "scale": "Major", "root": 48, "resDecay": 2.0, "resMix": 0.25, "resIn": "Full", "strike": 0.3, "resStereo": 1.0,
   "glowSrc": "Resonance", "glowSend": 0.4, "glowDecay": 3.0})
P("Rap Punch", V, "Front of each word pushed, a little clip: rap and spoken vocals that cut.", ["rap", "punch"], "vocal",
  {"xLow": 250, "xHigh": 4000, "b2Atk": 0.4, "b3Atk": 0.3, "b2Clip": 0.3})
P("Distant Vocal", V, "Softened fronts and a long Afterglow: the vocal steps back.", ["distant", "ambient"], "vocal",
  {"xLow": 250, "xHigh": 4000, "b2Atk": -0.3, "b3Atk": -0.4, "glowSrc": "Full", "glowSend": 0.45, "glowDecay": 2.4, "glowSize": 1.5, "glowMix": 0.45})

# Keys and synth
KS = "Keys & Synth"
P("Piano Hammer", KS, "Hammer attack forward on the top band, the low end held.", ["piano", "attack"], "keys",
  {"xLow": 200, "xHigh": 3000, "b3Atk": 0.45, "b2Atk": 0.25})
P("Piano Bloom", KS, "Sustain up and soft drive: a piano that blooms after each note.", ["piano", "sustain"], "keys",
  {"xLow": 200, "xHigh": 3000, "b1Sus": 0.3, "b2Sus": 0.45, "b2Curve": "Soft", "b2Drv": 0.2})
P("Rhodes Bark", KS, "Electric piano bark: tube drive on the mid band's transients only.", ["rhodes", "bark"], "keys",
  {"xLow": 200, "xHigh": 3000, "b2Src": "Transient", "b2Curve": "Tube", "b2Drv": 0.6})
P("Pluck Tighten", KS, "Synth plucks tighter: sustain cut, lookahead, attack up.", ["pluck", "tight"], "keys",
  {"b1Sus": -0.4, "b2Sus": -0.5, "b3Sus": -0.4, "b2Atk": 0.3, "look": "1 ms"})
P("Pad Swell", KS, "Pads: attack softened on every band, sustain lifted.", ["pad", "soft"], "keys",
  {"b1Atk": -0.5, "b2Atk": -0.6, "b3Atk": -0.6, "b2Sus": 0.3})
P("Organ Grit", KS, "Saw recipe on the mid band, a touch of drive: an organ with more bite.", ["organ", "grit"], "keys",
  {"xLow": 200, "xHigh": 3000, "b2H2": 0.5, "b2H3": 0.33, "b2H4": 0.25, "b2Harm": 0.35, "b2Drv": 0.2})
P("Square Lead", KS, "Odd orders on the mid band: a hollow, square-wave colour on any lead.", ["lead", "hollow"], "keys",
  {"xLow": 200, "xHigh": 4000, "b2H3": 0.333, "b2H5": 0.2, "b2H7": 0.143, "b2Harm": 0.5})
P("Bell Keys", KS, "Bell recipe on the top band: odd high orders over a reduced fundamental.", ["bell", "bright"], "keys",
  {"xLow": 250, "xHigh": 2500, "b3H1": 0.6, "b3H3": 0.6, "b3H5": 0.4, "b3H7": 0.3, "b3Harm": 0.4})
P("Harp Strings", KS, "Notes strike tuned resonators on the scale: a harp-like halo.", ["harp", "halo"], "keys",
  {"resMode": "Tuned", "pitch": "Key", "scale": "Major", "root": 60, "resDecay": 1.5, "resMix": 0.3, "strike": 0.6, "resStereo": 0.9})
P("MIDI Resonator", KS, "The resonators follow MIDI notes: play chords into the plug-in's MIDI input.", ["midi", "chords"], "keys",
  {"resMode": "Tuned", "pitch": "MIDI", "resDecay": 1.2, "resMix": 0.45, "strike": 0.7, "resStereo": 0.8})

# Mix bus and master
M = "Mix Bus / Master"
P("Master Warmth", M, "Gentle tube on every band and linear-phase bands: a little density for the mix.", ["warm", "subtle"], "mix",
  {"xType": "Linear phase", "b1Curve": "Tube", "b1Drv": 0.1, "b2Curve": "Tube", "b2Drv": 0.1, "b3Curve": "Tube", "b3Drv": 0.08, "clipGuard": False})
P("Mix Punch", M, "A light attack lift on every band with lookahead: a mix that moves more.", ["punch", "subtle"], "mix",
  {"b1Atk": 0.15, "b2Atk": 0.2, "b3Atk": 0.15, "look": "2 ms", "clipGuard": False})
P("Air Exciter", M, "H2 and H3 on the top band only: an exciter for the mix's air.", ["air", "exciter"], "mix",
  {"xType": "Linear phase", "xHigh": 6000, "b3H2": 0.3, "b3H3": 0.2, "b3Harm": 0.25, "os": "8x", "clipGuard": False})
P("Bass Translation", M, "Phantom harmonics for the low end and mono below 100 Hz: the mix on small speakers.", ["translation", "mono"], "mix",
  {"phantom": 0.25, "mono": 100, "clipGuard": False})
P("Tape Master", M, "Tape curve on every band, the top darkened a touch.", ["tape", "glue"], "mix",
  {"b1Curve": "Tape", "b1Drv": 0.15, "b2Curve": "Tape", "b2Drv": 0.15, "b3Curve": "Tape", "b3Drv": 0.15, "b3Tone": 0.42, "clipGuard": False})
P("Loud Clip", M, "Transient clip on every band and the clip guard: level without a limiter's pumping.", ["loud", "clip"], "mix",
  {"b1Clip": 0.35, "b2Clip": 0.45, "b3Clip": 0.45, "clipGuard": True})
P("Tame Transients", M, "Attack softened a little on every band: a smoother, less spiky mix.", ["smooth", "tame"], "mix",
  {"b1Atk": -0.15, "b2Atk": -0.25, "b3Atk": -0.3, "clipGuard": False})
P("Mid/Side Body", M, "Mid/side: the centre's body lifted a little, the sides passed untouched.", ["mid/side", "centre"], "mix",
  {"stereo": "Mid/Side", "b2Sus": 0.2, "b3Sus": 0.25, "clipGuard": False})
P("Transformer Master", M, "Transformer recipe on the low band, linear phase: iron colour in the bass only.", ["iron", "low"], "mix",
  {"xType": "Linear phase", "b1H2": 0.15, "b1H3": 0.2, "b1Harm": 0.3, "clipGuard": False})

# Sound design
SD = "Sound Design"
P("Cosmic Ray", SD, "Folded drive, struck metal and a long Afterglow: every hit becomes a strange event.", ["fx", "metal"], "drums",
  {"b2Curve": "Fold", "b2Drv": 0.6, "resMode": "Body", "material": "Metal", "root": 50, "resDecay": 2.5, "resMix": 0.5, "strike": 0.9,
   "glowSrc": "Resonance", "glowSend": 0.5, "glowDecay": 6.0, "glowSize": 2.5})
P("Glass Rain", SD, "Hats and percussion strike high glass: a shower of tuned pings.", ["glass", "pings"], "hats",
  {"resMode": "Body", "material": "Glass", "root": 79, "size": 0.7, "resDecay": 2.5, "resMix": 0.55, "strike": 1.0, "resStereo": 1.0})
P("Tube Organ Hit", SD, "Closed-tube resonance (odd harmonics) tuned low: a pipe-organ thump.", ["pipe", "organ"], "kick",
  {"resMode": "Body", "material": "Tube", "root": 36, "size": 0.5, "resDecay": 1.2, "resMix": 0.5, "strike": 0.8})
P("Comb Machine", SD, "A comb resonator, wide and long: every transient becomes a metallic flange.", ["comb", "metallic"], "drums",
  {"resMode": "Comb", "root": 57, "resDecay": 1.5, "resDamp": 0.2, "resMix": 0.5, "spread": 0.8, "resStereo": 1.0})
P("Transient Only", SD, "Listen to the transients alone: the attack layer, everything else removed.", ["listen", "transient"], "drums",
  {"listen": "Transients", "b2Atk": 0.3, "clipGuard": False})
P("Body Only", SD, "Listen to the body alone: drums without their attacks, a ghostly tail.", ["listen", "body"], "drums",
  {"listen": "Body", "b2Sus": 0.3, "clipGuard": False})
P("Muon Decay", SD, "Sustain inverted (cut) and a long Afterglow from the transients: hits become swells.", ["reverse", "swell"], "drums",
  {"b1Sus": -1.0, "b2Sus": -1.0, "b3Sus": -1.0, "glowSrc": "Transients", "glowSend": 0.7, "glowDecay": 4.0, "glowSize": 2.2, "glowMix": 0.7})
P("Harmonic Storm", SD, "Every order at full on every band: a wall of harmonics from any source.", ["extreme", "harmonics"], "keys",
  {"b1H2": 1.0, "b1H3": 1.0, "b1Harm": 0.6, "b2H3": 1.0, "b2H5": 1.0, "b2H7": 1.0, "b2Harm": 0.7, "b3H2": 1.0, "b3H4": 1.0, "b3Harm": 0.5, "os": "8x"})
P("Sub Bomb", SD, "Two octaves down and phantom on everything: a sub-harmonic drop.", ["sub", "drop"], "kick",
  {"sub": 0.6, "sub2": 0.6, "subPur": 0.6, "phantom": 0.4, "mono": 150})
P("Chord Machine", SD, "Transients strike a minor chord that follows the tracked bass: instant harmony.", ["chord", "tuned"], "bass",
  {"resMode": "Tuned", "pitch": "Track", "chord": "Minor", "resDecay": 1.5, "resMix": 0.5, "strike": 0.8, "resStereo": 0.9, "glowSend": 0.3})
P("Wood Xylo", SD, "Hits strike a tuned wooden bar: a xylophone voice for any rhythm.", ["xylophone", "wood"], "perc",
  {"resMode": "Body", "material": "Wood", "root": 67, "size": 0.5, "resDecay": 0.6, "resMix": 0.5, "strike": 1.0})
P("Rectifier Swarm", SD, "Rectified drive on every band at full mix: buzzing, octave-up chaos.", ["buzz", "octave"], "guitar",
  {"b1Curve": "Rectify", "b1Drv": 0.4, "b2Curve": "Rectify", "b2Drv": 0.5, "b3Curve": "Rectify", "b3Drv": 0.4, "b2Bias": 1.0})


def calibrate(measured_path):
    with open(measured_path) as f:
        measured = json.load(f)
    levels = {}
    if os.path.exists(LEVELS):
        with open(LEVELS) as f:
            levels = json.load(f)
    for name, delta in measured.items():
        # delta: output loudness minus input loudness (dB), with the current trim applied. Correct it.
        levels[name] = round(max(-24.0, min(24.0, levels.get(name, 0.0) - delta)), 2)
    with open(LEVELS, "w") as f:
        json.dump(levels, f, indent=1, sort_keys=True)
    print(f"calibrated {len(measured)} presets")


def main():
    if len(sys.argv) > 2 and sys.argv[1] == "--calibrate":
        calibrate(sys.argv[2])
    levels = {}
    if os.path.exists(LEVELS):
        with open(LEVELS) as f:
            levels = json.load(f)
    names = set()
    for p in PRESETS:
        assert p["name"] not in names, p["name"]
        names.add(p["name"])
        trim = levels.get(p["name"], 0.0)
        if abs(trim) > 0.05:
            p["params"]["outDb"] = trim
        elif "outDb" in p["params"]:
            del p["params"]["outDb"]
    cats = {}
    for p in PRESETS:
        cats[p["category"]] = cats.get(p["category"], 0) + 1
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w") as f:
        json.dump({"version": 1, "presets": PRESETS}, f, indent=1)
    print(f"wrote {len(PRESETS)} presets in {len(cats)} categories: " + ", ".join(f"{k} {v}" for k, v in cats.items()))


if __name__ == "__main__":
    main()
