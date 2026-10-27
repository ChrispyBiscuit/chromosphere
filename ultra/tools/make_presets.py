#!/usr/bin/env python3
"""Factory presets for Chromosphere | ULTRA -> ultra/data/presets.json.

Each preset is a kit (eight tracks: model and the six dials, shape, mix), a master bus, macros, tempo and swing, and
up to eight patterns written in a step notation (below). Presets are written one by one: a genre gives the starting
kit and the grooves, each preset then picks its own sounds, groove, tempo and space.

Step notation (one character per step, spaces ignored):
  .  rest            x  hit (100)        X  accent (125)      v  medium (82)      o  ghost (60)     g  soft ghost (40)
  r  ratchet x2      R  ratchet x3       T  ratchet x4        ?  50% chance       %  75% chance
  f  fill only       n  not in fills     h  loop 1 of 2       H  loop 2 of 2      q  loop 4 of 4    F  fill flam (x2)
A track can also be a dict: {"s": steps, "len": n, "rate": "1/32", "p": [pitch...], "l": [lock...], "n": [nudge...]}.

Usage:
  python3 make_presets.py                       write data/presets.json
  python3 make_presets.py --calibrate m.json    adjust each preset's output level from measured loudness
                                                (ultra_preset_render --measure m.json), then write
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "data", "presets.json")
LEVELS = os.path.join(HERE, "preset_levels.json")

# Must match ultra/dsp/ultra/params.h.
MODELS = ["Core Kick", "Gravity Kick", "Flare Snare", "Burst Clap", "Ion Hat", "Corona Cymbal", "Orbit Tom",
          "Pulsar FM", "Quasar Metal", "Spark Rim", "Solar Wind", "Plasma Zap", "Nebula Modal"]
FILTERS = ["Off", "LP 24", "LP 12", "BP 12", "HP 12", "HP 24", "Notch", "Comb", "Formant"]
LOCKS = ["Decay", "Tone", "Snap", "Color", "Sweep", "Drive", "Cutoff", "Pan", "Send A", "Send B"]
RATES = ["1/32", "1/16T", "1/16", "1/16.", "1/8T", "1/8", "1/4"]
CONDS = ["Always", "Fill", "Not fill", "1st", "Not 1st", "1:2", "2:2", "1:3", "2:3", "3:3", "1:4", "2:4", "3:4", "4:4"]
DIVS = ["8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/2T", "1/4.", "1/4", "1/4T", "1/8.", "1/8", "1/8T", "1/16.", "1/16", "1/16T", "1/32"]
DRIVES = ["Soft", "Tube", "Fold", "Cosmic Ray"]
GM = [36, 38, 39, 42, 46, 45, 50, 56]
TARGET_LOUDNESS = -15.0      # dBFS, 90th percentile of 50 ms RMS windows (ultra_preset_render)

# Per-track parameters that are the same default on every track (anything else is always written).
GENERIC = {"Drive": 0.0, "Filter": "Off", "Cutoff": 20000.0, "Res": 0.1, "Vel": 0.7, "Pan": 0.0, "SendA": 0.0,
           "Mute": False, "Solo": False, "Out": "Main"}


# ---- Sounds -----------------------------------------------------------------------------------------------------
def v(model, name, tune=0.0, decay=0.5, tone=0.5, snap=0.5, color=0.5, sweep=0.5, drive=0.0, filt="Off", cutoff=20000.0,
      res=0.1, vel=0.7, level=0.0, pan=0.0, a=0.0, b=0.05, choke=0, lock="Tone"):
    assert model in MODELS and filt in FILTERS and lock in LOCKS, (model, filt, lock)
    return dict(model=model, name=name, tune=tune, decay=decay, tone=tone, snap=snap, color=color, sweep=sweep, drive=drive,
                filt=filt, cutoff=cutoff, res=res, vel=vel, level=level, pan=pan, a=a, b=b, choke=choke, lock=lock)


def tweak(kit, **changes):
    """changes: t1=dict(decay=.6), t4=dict(...). Returns a new kit."""
    out = [dict(t) for t in kit]
    for k, ch in changes.items():
        out[int(k[1:]) - 1].update(ch)
    return out


def master(drive=0.0, dmode="Soft", dtone=0.7, dmix=1.0, glue=0.0, thr=-12.0, ratio=2.0, atk=10.0, rel=200.0, makeup=0.0,
           esync=True, ediv="1/8.", ems=300.0, fb=0.35, edamp=0.4, eret=0.8, e2r=0.0, ping=True,
           size=1.0, rdecay=1.6, rdamp=0.4, pre=8.0, rret=0.8, low=0.0, high=0.0, width=1.0, out=0.0):
    assert dmode in DRIVES and ediv in DIVS
    return {"drvMode": dmode, "drvAmt": drive, "drvTone": dtone, "drvMix": dmix, "glThr": thr, "glRatio": ratio, "glAtk": atk,
            "glRel": rel, "glMakeup": makeup, "glMix": glue, "dlySync": esync, "dlyDiv": ediv, "dlyMs": ems, "dlyFb": fb,
            "dlyDamp": edamp, "dlyRet": eret, "dlyToRv": e2r, "dlyPing": ping, "rvSize": size, "rvDecay": rdecay,
            "rvDamp": rdamp, "rvPre": pre, "rvRet": rret, "eqLow": low, "eqHigh": high, "width": width, "outDb": out}


# ---- Patterns ---------------------------------------------------------------------------------------------------
STEP = {".": None, "x": (100,), "X": (125,), "v": (82,), "o": (60,), "g": (40,),
        "r": (95, 100, 2), "R": (95, 100, 3), "T": (90, 100, 4), "?": (90, 50), "%": (85, 75),
        "f": (100, 100, 1, "Fill"), "n": (100, 100, 1, "Not fill"), "h": (100, 100, 1, "1:2"), "H": (100, 100, 1, "2:2"),
        "q": (105, 100, 1, "4:4"), "F": (115, 100, 2, "Fill")}


def lane_str(values, default):
    last = -1
    for i, x in enumerate(values):
        if x != default:
            last = i
    return ",".join(str(int(x)) for x in values[:last + 1])


def track(spec):
    if not spec:
        return {"len": 16}
    if isinstance(spec, str):
        spec = {"s": spec}
    steps = spec["s"].replace(" ", "")
    n = spec.get("len", len(steps))
    assert 1 <= n <= 32 and len(steps) <= 32, spec
    vel, ch, rat, cond = [0] * 32, [100] * 32, [1] * 32, [0] * 32
    for i, c in enumerate(steps):
        d = STEP[c]
        if d is None:
            continue
        vel[i] = d[0]
        if len(d) > 1:
            ch[i] = d[1]
        if len(d) > 2:
            rat[i] = d[2]
        if len(d) > 3:
            cond[i] = CONDS.index(d[3])
    out = {"len": n}
    rate = spec.get("rate", "1/16")
    assert rate in RATES
    if rate != "1/16":
        out["rate"] = rate
    for key, lane, default in (("v", vel, 0), ("c", ch, 100), ("r", rat, 1), ("k", cond, 0)):
        s = lane_str(lane, default)
        if s:
            out[key] = s
    for key, src in (("p", "p"), ("n", "n"), ("l", "l")):
        if src in spec:
            vals = list(spec[src]) + [0] * (32 - len(spec[src]))
            s = lane_str(vals, 0)
            if s:
                out[key] = s
    return out


def pattern(*tracks_):
    tr = list(tracks_) + [None] * (8 - len(tracks_))
    return [track(t) for t in tr]


# ---- Kits by genre ----------------------------------------------------------------------------------------------
# Every kit uses the same slots, so any groove fits any kit:
#   1 kick   2 snare   3 clap   4 closed hat   5 open hat   6 perc A (conga, tom, FM)   7 perc B (rim, cowbell, bell)
#   8 top (shaker, ride, riser)
# Dial meanings follow engine 2 (ultra/dsp/ultra/voice.cpp): Core Kick Color = Punch, Gravity Kick Snap = Top and
# Color = Drive, Orbit Tom Color = Conga, Corona Cymbal Color = Crash, Spark Rim Sweep = Body.
def kit_deep():
    return [v("Core Kick", "Kick", tune=-1, decay=0.52, tone=0.35, snap=0.25, color=0.25, sweep=0.4, b=0.0),
            v("Flare Snare", "Snare", tune=2, decay=0.35, tone=0.45, snap=0.4, color=0.55, sweep=0.3, level=-9, b=0.15),
            v("Burst Clap", "Clap", tune=-1, decay=0.45, tone=0.4, snap=0.5, color=0.5, sweep=0.55, level=-3, b=0.25),
            v("Ion Hat", "Closed Hat", decay=0.22, tone=0.45, snap=0.35, color=0.55, sweep=0.15, level=-11, choke=1, pan=0.12, vel=0.8),
            v("Ion Hat", "Open Hat", decay=0.55, tone=0.45, snap=0.3, color=0.6, sweep=0.2, level=-12, choke=1, pan=0.12, b=0.1),
            v("Orbit Tom", "Conga", tune=12, decay=0.35, tone=0.45, snap=0.45, color=0.8, sweep=0.3, level=-8, pan=-0.3, b=0.12),
            v("Spark Rim", "Rim", tune=-2, decay=0.35, tone=0.4, snap=0.4, color=0.15, sweep=0.5, level=-12, pan=0.3, a=0.15, b=0.1),
            v("Solar Wind", "Shaker", tune=10, decay=0.22, tone=0.35, snap=0.55, color=0.7, sweep=0.5, level=-16, pan=-0.2, b=0.05, vel=0.85)]


def kit_tech():
    return [v("Core Kick", "Kick", tune=0, decay=0.44, tone=0.55, snap=0.55, color=0.55, sweep=0.5, b=0.0),
            v("Flare Snare", "Snare", tune=1, decay=0.35, tone=0.55, snap=0.6, color=0.6, sweep=0.35, level=-7, b=0.15),
            v("Burst Clap", "Clap", decay=0.4, tone=0.55, snap=0.6, color=0.55, sweep=0.45, level=-2, b=0.2),
            v("Ion Hat", "Closed Hat", decay=0.18, tone=0.6, snap=0.55, color=0.45, sweep=0.2, level=-10, choke=1, pan=-0.1, vel=0.8),
            v("Ion Hat", "Open Hat", decay=0.5, tone=0.6, snap=0.4, color=0.5, sweep=0.25, level=-10, choke=1, pan=-0.1, b=0.08),
            v("Pulsar FM", "Perc", tune=7, decay=0.25, tone=0.45, snap=0.6, color=0.35, sweep=0.25, level=-12, pan=0.3, a=0.2, b=0.1),
            v("Orbit Tom", "Bongo", tune=17, decay=0.3, tone=0.5, snap=0.55, color=0.75, sweep=0.35, level=-9, pan=-0.3, b=0.1),
            v("Solar Wind", "Shaker", tune=12, decay=0.2, tone=0.4, snap=0.6, color=0.75, sweep=0.5, level=-15, pan=0.2, vel=0.85)]


def kit_house():
    return [v("Core Kick", "Kick", tune=1, decay=0.46, tone=0.5, snap=0.45, color=0.45, sweep=0.45, b=0.0),
            v("Flare Snare", "Snare", tune=1, decay=0.38, tone=0.55, snap=0.55, color=0.6, sweep=0.3, level=-5, b=0.15),
            v("Burst Clap", "Clap", decay=0.45, tone=0.5, snap=0.55, color=0.55, sweep=0.5, level=-2, b=0.2),
            v("Ion Hat", "Closed Hat", decay=0.2, tone=0.55, snap=0.5, color=0.4, sweep=0.15, level=-9, choke=1, pan=0.1),
            v("Ion Hat", "Open Hat", decay=0.58, tone=0.55, snap=0.35, color=0.45, sweep=0.25, level=-9, choke=1, pan=0.1, b=0.08),
            v("Orbit Tom", "Low Tom", tune=-2, decay=0.45, tone=0.4, snap=0.4, color=0.2, sweep=0.4, level=-6, pan=-0.25, b=0.1),
            v("Quasar Metal", "Cowbell", decay=0.3, tone=0.5, snap=0.45, color=0.0, sweep=0.0, level=-13, pan=0.3),
            v("Corona Cymbal", "Ride", tune=1, decay=0.5, tone=0.6, snap=0.35, color=0.2, sweep=0.35, level=-14, pan=-0.2, b=0.06)]


def kit_trance():
    return [v("Gravity Kick", "Kick", tune=-1, decay=0.4, tone=0.55, snap=0.65, color=0.55, sweep=0.6, b=0.0),
            v("Flare Snare", "Snare", tune=2, decay=0.4, tone=0.6, snap=0.6, color=0.7, sweep=0.3, level=-6, b=0.25),
            v("Burst Clap", "Clap", decay=0.45, tone=0.6, snap=0.65, color=0.6, sweep=0.5, level=-3, b=0.25),
            v("Ion Hat", "Closed Hat", decay=0.15, tone=0.7, snap=0.5, color=0.6, sweep=0.15, level=-13, pan=0.15),
            v("Ion Hat", "Open Hat", decay=0.48, tone=0.7, snap=0.45, color=0.65, sweep=0.2, level=-7, pan=-0.05, b=0.08),
            v("Pulsar FM", "Perc", tune=12, decay=0.2, tone=0.4, snap=0.6, color=0.5, sweep=0.3, level=-15, pan=0.3, a=0.3, b=0.15),
            v("Corona Cymbal", "Ride", tune=3, decay=0.4, tone=0.7, snap=0.5, color=0.15, sweep=0.4, level=-14, pan=-0.25),
            v("Solar Wind", "Riser", tune=0, decay=0.9, tone=0.45, snap=0.0, color=0.25, sweep=0.95, level=-17, b=0.3)]


def kit_psy():
    return tweak(kit_trance(),
                 t1=dict(tune=1, decay=0.3, tone=0.6, snap=0.7, color=0.45, sweep=0.55),
                 t4=dict(decay=0.12, tone=0.75, color=0.55, level=-12),
                 t5=dict(decay=0.4, level=-9),
                 t6=dict(model="Plasma Zap", name="Zap", tune=14, decay=0.25, tone=0.6, snap=0.7, color=0.3, sweep=0.55, level=-17, a=0.35, b=0.2),
                 t7=dict(model="Pulsar FM", name="Blip", tune=19, decay=0.15, tone=0.55, snap=0.7, color=0.7, sweep=0.2, level=-16, pan=0.35, a=0.3),
                 t8=dict(model="Solar Wind", name="Shaker", tune=12, decay=0.18, tone=0.45, snap=0.7, color=0.7, sweep=0.5, level=-16, b=0.05))


def kit_dnb():
    return [v("Gravity Kick", "Kick", tune=4, decay=0.32, tone=0.6, snap=0.7, color=0.4, sweep=0.55, b=0.0),
            v("Flare Snare", "Snare", tune=3, decay=0.5, tone=0.6, snap=0.8, color=0.6, sweep=0.4, level=-1, b=0.15),
            v("Flare Snare", "Ghost", tune=5, decay=0.3, tone=0.55, snap=0.45, color=0.5, sweep=0.2, level=-12, pan=0.15, b=0.1),
            v("Ion Hat", "Hat", decay=0.14, tone=0.65, snap=0.55, color=0.55, sweep=0.15, level=-11, choke=1, pan=0.2),
            v("Ion Hat", "Open Hat", decay=0.45, tone=0.6, snap=0.4, color=0.55, sweep=0.2, level=-12, choke=1, pan=0.2),
            v("Corona Cymbal", "Ride", tune=2, decay=0.45, tone=0.6, snap=0.4, color=0.2, sweep=0.35, level=-16, pan=-0.25),
            v("Spark Rim", "Rim", tune=1, decay=0.3, tone=0.55, snap=0.55, color=0.15, sweep=0.5, level=-12, pan=-0.3),
            v("Solar Wind", "Shaker", tune=10, decay=0.18, tone=0.45, snap=0.65, color=0.7, sweep=0.5, level=-17, pan=0.35)]


def kit_techno():
    return [v("Gravity Kick", "Kick", tune=0, decay=0.42, tone=0.4, snap=0.5, color=0.45, sweep=0.5, b=0.0),
            v("Flare Snare", "Snare", tune=-2, decay=0.4, tone=0.45, snap=0.55, color=0.7, sweep=0.2, level=-6, b=0.2),
            v("Burst Clap", "Clap", decay=0.5, tone=0.45, snap=0.6, color=0.7, sweep=0.6, level=-4, b=0.25),
            v("Ion Hat", "Closed Hat", decay=0.15, tone=0.65, snap=0.6, color=0.35, sweep=0.1, level=-10, choke=1, pan=-0.15),
            v("Ion Hat", "Open Hat", decay=0.5, tone=0.6, snap=0.4, color=0.4, sweep=0.2, level=-10, choke=1, pan=-0.15, b=0.1),
            v("Pulsar FM", "Perc", tune=5, decay=0.3, tone=0.5, snap=0.6, color=0.45, sweep=0.3, level=-11, pan=0.3, a=0.25, b=0.15, lock="Color"),
            v("Spark Rim", "Rim", tune=-3, decay=0.35, tone=0.55, snap=0.6, color=0.1, sweep=0.6, level=-11, pan=-0.35, a=0.2),
            v("Corona Cymbal", "Ride", decay=0.45, tone=0.65, snap=0.4, color=0.3, sweep=0.4, level=-15, pan=0.2)]


def kit_garage():
    return [v("Core Kick", "Kick", tune=0, decay=0.42, tone=0.5, snap=0.45, color=0.4, sweep=0.42, b=0.0),
            v("Flare Snare", "Snare", tune=3, decay=0.35, tone=0.6, snap=0.6, color=0.55, sweep=0.3, level=-4, b=0.15),
            v("Burst Clap", "Clap", decay=0.4, tone=0.55, snap=0.55, color=0.5, sweep=0.45, level=-3, b=0.2),
            v("Ion Hat", "Hat", decay=0.17, tone=0.6, snap=0.5, color=0.5, sweep=0.15, level=-9, choke=1, pan=0.15),
            v("Ion Hat", "Open Hat", decay=0.5, tone=0.6, snap=0.4, color=0.5, sweep=0.2, level=-10, choke=1, pan=0.15),
            v("Spark Rim", "Rim", tune=1, decay=0.3, tone=0.55, snap=0.5, color=0.2, sweep=0.4, level=-10, pan=-0.3, a=0.15),
            v("Orbit Tom", "Tom", tune=2, decay=0.4, tone=0.45, snap=0.45, color=0.35, sweep=0.4, level=-7, pan=0.25, b=0.1),
            v("Solar Wind", "Shaker", tune=11, decay=0.2, tone=0.4, snap=0.6, color=0.7, sweep=0.5, level=-15, pan=-0.25)]


def kit_melodic():
    return [v("Core Kick", "Kick", tune=-2, decay=0.55, tone=0.4, snap=0.35, color=0.35, sweep=0.45, b=0.0),
            v("Flare Snare", "Snare", tune=0, decay=0.45, tone=0.45, snap=0.45, color=0.65, sweep=0.25, level=-8, b=0.3),
            v("Burst Clap", "Clap", decay=0.55, tone=0.45, snap=0.5, color=0.6, sweep=0.7, level=-4, b=0.35),
            v("Ion Hat", "Closed Hat", decay=0.2, tone=0.5, snap=0.4, color=0.6, sweep=0.2, level=-12, choke=1, pan=0.15),
            v("Ion Hat", "Open Hat", decay=0.6, tone=0.5, snap=0.3, color=0.65, sweep=0.3, level=-12, choke=1, pan=0.15, b=0.15),
            v("Orbit Tom", "Tom", tune=-3, decay=0.55, tone=0.4, snap=0.35, color=0.3, sweep=0.45, level=-7, pan=-0.3, a=0.2, b=0.25),
            v("Nebula Modal", "Pluck", tune=7, decay=0.4, tone=0.5, snap=0.6, color=0.55, sweep=0.35, level=-13, pan=0.3, a=0.35, b=0.3),
            v("Solar Wind", "Shaker", tune=9, decay=0.25, tone=0.35, snap=0.5, color=0.65, sweep=0.5, level=-16, pan=-0.2, b=0.1)]


# ---- Grooves ----------------------------------------------------------------------------------------------------
# Each groove is four patterns: main, variation, break, fill. Tracks in slot order.
CONGA_P = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, -2]

DEEP = [
    [pattern("x...x...x...x...", "", "....x.......x...", "o..go..go..go..g", "..x...x...x...x.", {"s": "......x...x..x..", "p": CONGA_P}, "...x.....x....x.", "gvgvgvgvgvgvgvgv"),
     pattern("x...x...x...x...", "...........g....", "....x.......x..g", "o..go..go..go..g", "..x...x...x...x.", {"s": "......x.x.x..x.o", "p": CONGA_P}, "...x.....x..x.x.", "gvgvgvgvgvgvgvgv"),
     pattern("x...............", "", "....x.......x...", "o..go..go..go..g", "..x...x...x...x.", {"s": "......x...x..x..", "p": CONGA_P}, "", "gvgvgvgvgvgvgvgv"),
     pattern("x...x...x...x...", "............f.ff", "....x.......x...", "o..go..go..go..g", "..x...x...x.....", {"s": "......x...x.FfFF", "p": CONGA_P}, "...x.....x....x.", "gvgvgvgvgvgvgvgv")],
    [pattern("x...x...x...x...", "", "....x.......x...", "..x...x...x...x.", "", "", "x..x..x...x..x..", "vgvgvgvgvgvgvgvg"),
     pattern("x...x...x..xx...", "", "....x.......x...", "..x...x...x...x.", "..............x.", {"s": "..........x.....", "p": [0] * 10 + [7]}, "x..x..x...x..x..", "vgvgvgvgvgvgvgvg"),
     pattern("x...............", "", "", "..x...x...x...x.", "", "", "x..x..x...x..x..", "vgvgvgvgvgvgvgvg"),
     pattern("x...x...x...x...", "", "....x.......xFFF", "..x...x...x...x.", "", "", "x..x..x...x..x..", "vgvgvgvgvgvgvgvg")],
    [pattern("x...x...x...x...", "....x.......x...", "", "xgxgxgxgxgxgxgxg", "..x...x...x...x.", "", "......v.......v.", ""),
     pattern("x...x...x...x...", "....x.......x..g", "", "xgxgxgxgxgxgxgxr", "..x...x...x...x.", {"s": ".......x......x.", "p": [0] * 14 + [3]}, "......v.......v.", ""),
     pattern("x.......x.......", "", "", "xgxgxgxgxgxgxgxg", "..x...x...x...x.", "", "", "vvvvvvvvvvvvvvvv"),
     pattern("x...x...x...x...", "....x.......xFFF", "", "xgxgxgxgxgxgxgxg", "..x...x...x...x.", "", "......v.......v.", "")],
]

TECH = [
    [pattern("x...x...x...x...", "", "....x.......x...", "xoxoxoxoxoxoxoxo", "..x...x...x...x.", {"s": "..x..x....x..x..", "l": [0, 0, 20, 0, 0, -20, 0, 0, 0, 0, 40]}, "...x..x....x....", "vgvgvgvgvgvgvgvg"),
     pattern("x...x...x...x..o", "", "....x.......x...", "xoxoxoxoxoxoxoxr", "..x...x...x...x.", {"s": "..x..x....x..x.x", "l": [0, 0, 20, 0, 0, -20, 0, 0, 0, 0, 40]}, "...x..x....x.x..", "vgvgvgvgvgvgvgvg"),
     pattern("x...............", "", "....x.......x...", "xoxoxoxoxoxoxoxo", "", {"s": "..x..x....x..x..", "l": [0, 0, 20, 0, 0, -20, 0, 0, 0, 0, 40]}, "", "vgvgvgvgvgvgvgvg"),
     pattern("x...x...x...x...", "............FFFF", "....x.......x...", "xoxoxoxoxoxoxoxo", "..x...x...x.....", {"s": "..x..x....x..x..", "l": [0, 0, 20, 0, 0, -20, 0, 0, 0, 0, 40]}, "...x..x....x....", "vgvgvgvgvgvgvgvg")],
    [pattern("x...x...x...x...", "....o.......o...", "....x.......x...", "..x...x...x...x.", "", {"s": "x..x..x.", "len": 7}, {"s": "..x..x..x..x", "len": 12}, "xxxxxxxxxxxxxxxx"),
     pattern("x...x...x...x...", "....o.......o..g", "....x.......x...", "..x...x...x...xr", "..............x.", {"s": "x..x..x.", "len": 7}, {"s": "..x..x..x..x", "len": 12}, "xxxxxxxxxxxxxxxx"),
     pattern("x.......x.......", "", "", "..x...x...x...x.", "", {"s": "x..x..x.", "len": 7}, "", "xxxxxxxxxxxxxxxx"),
     pattern("x...x...x...x...", "....o...f.f.FFFF", "....x.......x...", "..x...x...x...x.", "", {"s": "x..x..x.", "len": 7}, {"s": "..x..x..x..x", "len": 12}, "xxxxxxxxxxxxxxxx")],
    [pattern("x...x...x...x...", "", "....x.......x...", "x.x.x.x.x.x.x.x.", "..x...x...x...x.", "", {"s": "x.....x...x.....", "p": [0] * 6 + [5] + [0] * 3 + [-5]}, "..v...v...v...v."),
     pattern("x...x...x...x...", "", "....x..o....x...", "x.x.x.x.x.x.x.xr", "..x...x...x...x.", {"s": "...........x..x.", "p": [0] * 11 + [3, 0, 0, 7]}, {"s": "x.....x...x...x.", "p": [0] * 6 + [5] + [0] * 3 + [-5]}, "..v...v...v...v."),
     pattern("x...............", "", "....x.......x...", "", "..x...x...x...x.", "", {"s": "x.....x...x.....", "p": [0] * 6 + [5] + [0] * 3 + [-5]}, "vvvvvvvvvvvvvvvv"),
     pattern("x...x...x...x...", "........f.f.FFFF", "....x.......x...", "x.x.x.x.x.x.x.x.", "..x...x...x...x.", "", {"s": "x.....x...x.....", "p": [0] * 6 + [5] + [0] * 3 + [-5]}, "..v...v...v...v.")],
]

HOUSE = [
    [pattern("x...x...x...x...", "", "....x.......x...", "x.x.x.x.x.x.x.x.", "..x...x...x...x.", "", "", "..v...v...v...v."),
     pattern("x...x...x...x...", "", "....x.......x..o", "xoxoxoxoxoxoxoxo", "..x...x...x...x.", "", "...o..o....o..o.", ""),
     pattern("x...............", "", "....x.......x...", "", "..x...x...x...x.", "", "", "x.v.x.v.x.v.x.v."),
     pattern("x...x...x...x.x.", "....f...f.f.FFFF", "....n.......n...", "x.x.x.x.x.x.x.x.", "..x...x...x...x.", "..........f.f.f.", "", "")],
    [pattern("x...x...x...x...", "....x.......x...", "....x.......x...", "..x...x...x...x.", "", "", "..x...x...x...x.", "x.x.x.x.x.x.x.x."),
     pattern("x...x...x...x...", "....x.......x...", "....x..o....x...", "o.x.o.x.o.x.o.x.", "", "......x.......x.", "..x...x...x...x.", "x.x.x.x.x.x.x.x."),
     pattern("x...x...x...x...", "", "....x.......x...", "..x...x...x...x.", "..........x.....", "", "", "x...v...x...v..."),
     pattern("x...x...x...xxxx", "....x...x...x...", "....x...x...x...", "..x...x...x...x.", "", "......f.f.f.F...", "..x...x...x...x.", "")],
]

TRANCE = [
    [pattern("x...x...x...x...", "", "....x.......x...", "xxxxxxxxxxxxxxxx", "..x...x...x...x.", "", "..v...v...v...v.", ""),
     pattern("x...x...x...x...", "", "....x.......x...", "xxxxxxxxxxxxxxxx", "..x...x...x...x.", {"s": ".x..x..x..x..x..", "p": [0, 0, 0, 0, 5, 0, 0, 7, 0, 0, 12, 0, 0, 5]}, "..v...v...v...v.", ""),
     pattern("x...............", "", "....x.......x...", "x.x.x.x.x.x.x.x.", "..x...x...x...x.", "", "", "x..............."),
     pattern("x...x...x...x...", {"s": "x.x.x.x.xxxxTTTT", "l": [0, 0, 0, 0, 0, 0, 0, 0, 10, 20, 30, 40, 50, 60, 70, 80]}, "....x.......x...", "xxxxxxxxxxxxxxxx", "..x...x...x...x.", "", "..v...v...v...v.", "")],
    [pattern("x...x...x...x...", "", "....x.......x...", "x.xxx.xxx.xxx.xx", "..x...x...x...x.", "", "", ""),
     pattern("x...x...x...x...", "", "....x.......x...", "x.xxx.xxx.xxx.xx", "..x...x...x...x.", "", "x.v.x.v.x.v.x.v.", ""),
     pattern("................", "", "", "x.xxx.xxx.xxx.xx", "..x...x...x...x.", "", "", "x..............."),
     pattern("x...x...x...x...", "....x...x.x.xxxx", "....x.......x...", "x.xxx.xxx.xxx.xx", "..x...x...x...x.", "", "", "")],
]

PSY = [
    [pattern("x...x...x...x...", "", "....x.......x...", "xxxxxxxxxxxxxxxx", "..x...x...x...x.", {"s": "..x..x....x..x..", "p": [0, 0, 12, 0, 0, 7, 0, 0, 0, 0, 5]}, "x.x..x.x..x.x..x", "vvvvvvvvvvvvvvvv"),
     pattern("x...x...x...x...", "", "....x.......x...", "xxxxxxxxxxxxxxxx", "..x...x...x...x.", {"s": "..x..x.x..x..x.x", "p": [0, 0, 12, 0, 0, 7, 0, 3, 0, 0, 5, 0, 0, 0, 0, 10]}, "x.x..x.x..x.x..x", "vvvvvvvvvvvvvvvv"),
     pattern("x...............", "", "", "xxxxxxxxxxxxxxxx", "..x...x...x...x.", {"s": "..x..x....x..x..", "p": [0, 0, 12, 0, 0, 7, 0, 0, 0, 0, 5]}, "", "vvvvvvvvvvvvvvvv"),
     pattern("x...x...x...x...", "........f.f.FFFF", "....x.......x...", "xxxxxxxxxxxxxxxx", "..x...x...x...x.", "", "x.x..x.x..x.x..x", "vvvvvvvvvvvvvvvv")],
]

DNB = [
    [pattern("x.........x.....", "....x.......x...", "......o..o....o.", "x.x.x.x.x.x.x.x.", "..............x.", "", "", "xoxoxoxoxoxoxoxo"),
     pattern("x.........x.x...", "....x.......x...", ".o....o..o..o.o.", "x.x.x.x.x.x.x.xr", "......x.........", "", "", "xoxoxoxoxoxoxoxo"),
     pattern("x...............", "....x.......x...", "", "", "", "x.x.x.x.x.x.x.x.", "", ""),
     pattern("x.........x.....", "....x.......xFFF", "......o..o....o.", "x.x.x.x.x.x.x.x.", "", "", "..........f.f.f.", "")],
    [pattern("x.....x...x.....", "....x.......x...", "..o....o...o..o.", "x.x.x.x.x.x.x.x.", "", "x...x...x...x...", "", ""),
     pattern("x.....x...x...x.", "....x.......x...", "..o..o.o...o..o.", "x.x.xrx.x.x.x.x.", "", "x...x...x...x...", "", ""),
     pattern("x.....x.........", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "x.x.x.x.x.x.x.x.", "", ""),
     pattern("x.....x...x.....", "....x...F.F.FFFF", "..o....o...o..o.", "x.x.x.x.x.x.x.x.", "", "", "", "")],
    [pattern("x.x.......x.....", "....x.......x...", ".o..o.o..o..o..o", "xxxxxxxxxxxxxxxx", "", "", "......x.......x.", ""),
     pattern("x.x.......x..x..", "....x..o....x...", ".o..o.o..o..oo.o", "xxxxxxxxxxxxxxxr", "..............x.", "", "......x.......x.", ""),
     pattern("x...............", "....x.......x...", ".o..o.o..o..o..o", "x.x.x.x.x.x.x.x.", "", "", "", "vvvvvvvvvvvvvvvv"),
     pattern("x.x.......x.....", "....x...x.xxFFTT", ".o..o.o..o..o..o", "xxxxxxxxxxxxxxxx", "", "", "", "")],
]

# Halftime (drum and bass tempo, the snare on beat three) and jungle (break chops, ratchets).
HALFTIME = [
    [pattern("x.........x.....", "........x.......", "", "x.x.x.x.x.x.x.x.", "", "", "...x.......x..x.", ""),
     pattern("x.........x..x..", "........x.......", "", "x.x.x.x.x.x.x.xr", "..............x.", "", "...x.......x..x.", ""),
     pattern("x...............", "........x.......", "", "", "", "x...x...x...x...", "", ""),
     pattern("x.........x.....", "........x...FFFF", "", "x.x.x.x.x.x.x.x.", "", "", "", "")],
]
JUNGLE = [
    [pattern("x.........x.....", "....x.......x..x", "..o..o.o...o.o..", "x.xxx.xxx.xxx.xx", "......x.........", "", "x...x...x...x...", ""),
     pattern("x.x.......x.....", "....x..x.r..x.x.", ".o.o.o.o..oo.o.o", "x.xxx.xxx.xxx.xr", "......x.......x.", "", "x...x...x...x...", ""),
     pattern("x...............", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "x.x.x.x.x.x.x.x.", "", ""),
     pattern("x.x.......x.....", "....x.r.x.rRTTTT", ".o.o.o.o..oo.o.o", "x.xxx.xxx.xxx.xx", "", "", "", "")],
]

TECHNO = [
    [pattern("x...x...x...x...", "", "", "..x...x...x...x.", "", "x..x..x..x..x...", "......x.......x.", "..v...v...v...v."),
     pattern("x...x...x...x...", "", "....x.......x...", "..x...x...x...x.", "", "x..x..x..x..x.x.", "......x.....x.x.", "..v...v...v...v."),
     pattern("x...............", "", "", "..x...x...x...x.", "....x.......x...", {"s": "x..x..x..x..x..x", "l": [0, 0, 0, 20, 0, 0, 40, 0, 0, 60, 0, 0, 80]}, "", ""),
     pattern("x...x...x...x...", "............FFFF", "....f...f...f...", "..x...x...x...x.", "", "x..x..x..x..x...", "......x.......x.", "")],
    [pattern("x...x...x...x...", "", "", "xxxxxxxxxxxxxxxx", "..x...x...x...x.", {"s": "x.....x...x.....", "len": 12}, "", ""),
     pattern("x...x...x...x...", "", "....x.......x...", "xoxoxoxoxoxoxoxo", "..x...x...x...x.", {"s": "x.....x...x.....", "len": 12}, {"s": "..x..x..x..", "len": 11}, ""),
     pattern("x...x...x...x...", "", "", "", "", {"s": "x.x.x.x.x.x.x.x.", "rate": "1/32"}, "", "x..............."),
     pattern("x...x...x...x.x.", "....f...f...FFFF", "", "xxxxxxxxxxxxxxxx", "..x...x...x...x.", {"s": "x.....x...x.....", "len": 12}, "", "")],
]

GARAGE = [
    [pattern("x.........x.....", "....x.......x...", "", {"s": "x.x.x.x.x.x.x.x.", "n": [0, 0, 8, 0, 0, 0, 8, 0, 0, 0, 8, 0, 0, 0, 8]}, "..............x.", "", "...x..x....x..x.", "vgvgvgvgvgvgvgvg"),
     pattern("x.........x..x..", "....x..o....x...", "", {"s": "x.x.x.x.x.x.x.xr", "n": [0, 0, 8, 0, 0, 0, 8, 0, 0, 0, 8, 0, 0, 0, 8]}, "......x.......x.", "..........x.....", "...x..x....x..x.", "vgvgvgvgvgvgvgvg"),
     pattern("x...............", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "", "", "vgvgvgvgvgvgvgvg"),
     pattern("x.........x.....", "....x.......xFFF", "", "x.x.x.x.x.x.x.x.", "", "..........f.f.f.", "...x..x....x..x.", "")],
]

BREAKS = [
    [pattern("x.........x.....", "....x.......x...", "......o.o....o..", "x.x.x.x.x.x.x.x.", "..............x.", "", "", ""),
     pattern("x.x.......x.....", "....x.......x..o", ".o....o.o....o..", "x.x.x.x.x.x.xox.", "......x.........", "", "", ""),
     pattern("x...............", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "", "", ""),
     pattern("x.........x.....", "....x...x.x.FFFF", "......o.o....o..", "x.x.x.x.x.x.x.x.", "", "", "", "")],
]

MELODIC = [
    [pattern("x...x...x...x...", "", "....x.......x...", "..x...x...x...x.", "..............x.", {"s": "x.....x...x.....", "p": [0] * 6 + [-5] + [0] * 3 + [3]}, {"s": "..x..x....x..x..", "p": [0, 0, 7, 0, 0, 12, 0, 0, 0, 0, 5, 0, 0, 10]}, "vgvgvgvgvgvgvgvg"),
     pattern("x...x...x...x...", "", "....x.......x..g", "..x...x...x...x.", "..............x.", {"s": "x.....x...x...x.", "p": [0] * 6 + [-5] + [0] * 3 + [3, 0, 0, 0, -2]}, {"s": "..x..x.x..x..x..", "p": [0, 0, 7, 0, 0, 12, 0, 3, 0, 0, 5, 0, 0, 10]}, "vgvgvgvgvgvgvgvg"),
     pattern("x...............", "", "", "..x...x...x...x.", "", "", {"s": "..x..x....x..x..", "p": [0, 0, 7, 0, 0, 12, 0, 0, 0, 0, 5, 0, 0, 10]}, "vgvgvgvgvgvgvgvg"),
     pattern("x...x...x...x...", "........f.f.FFFF", "....x.......x...", "..x...x...x...x.", "", {"s": "x.....x...x.FfFF", "p": [0] * 6 + [-5] + [0] * 3 + [3, 0, 0, 3, 5, 7]}, "", "vgvgvgvgvgvgvgvg")],
]


# ---- The presets ------------------------------------------------------------------------------------------------
PRESETS = []


def add(name, cat, desc, tags, kit, groove, tempo, swing=0.5, mst=None, macros=(0, 0, 0, 0), patterns_extra=None):
    PRESETS.append(dict(name=name, category=cat, description=desc, tags=tags, kit=kit, groove=groove, tempo=tempo,
                        swing=swing, master=mst or master(), macros=macros))


# Deep House
DEEP_M = dict(glue=0.4, thr=-14, ratio=2.0, rdecay=2.0, size=1.2, rret=0.6, high=-2, low=1)
add("Midnight Corona", "Deep House", "Round, warm kick; soft clap with a room tail; congas, rim and shaker under a swung groove.",
    ["deep house", "warm", "swing"], kit_deep(), DEEP[0], 122, 0.56, master(**DEEP_M))
add("Velvet Orbit", "Deep House", "Minimal deep groove: rim figure, an off-beat closed hat, the kick sitting low.",
    ["deep house", "minimal"], tweak(kit_deep(), t1=dict(decay=0.58, tone=0.3), t7=dict(level=-8)), DEEP[1], 120, 0.58, master(**DEEP_M))
add("Low Sun Lounge", "Deep House", "Lazy and wide: long ride-like shaker, tuned-down kick, a darker clap.",
    ["deep house", "lounge", "dark"], tweak(kit_deep(), t1=dict(tune=-2, decay=0.6), t3=dict(tone=0.3, sweep=0.7, b=0.35)), DEEP[0], 118, 0.6,
    master(glue=0.3, rdecay=2.8, size=1.5, rret=0.7, high=-3, low=1.5), macros=(0, 0.15, 0.1, 0.2))
add("Garden of Stars", "Deep House", "Organic deep house: congas lead, bongo-ish ghosts, a soft snare instead of a clap.",
    ["deep house", "organic", "percussion"], tweak(kit_deep(), t6=dict(level=-5, decay=0.4), t2=dict(level=-5)), DEEP[2], 121, 0.57, master(**DEEP_M))
add("Chord Room", "Deep House", "Classic US deep: snare on two and four, sixteenth hats with ghosts, a cowbell answer.",
    ["deep house", "classic", "us"], tweak(kit_deep(), t2=dict(level=-3), t7=dict(model="Quasar Metal", name="Cowbell", decay=0.28, tone=0.45, snap=0.4, color=0.0, sweep=0.0, level=-15)),
    DEEP[2], 123, 0.55, master(**DEEP_M))
add("Late Train", "Deep House", "Dubby deep: echo on the rim and congas, long dark reverb, a muted kick.",
    ["deep house", "dub"], tweak(kit_deep(), t6=dict(a=0.35), t7=dict(a=0.5), t1=dict(tone=0.28)), DEEP[1], 119, 0.57,
    master(glue=0.35, ediv="1/8.", fb=0.5, edamp=0.6, eret=0.5, rdecay=3.2, size=1.6, rret=0.6, high=-3, low=1))
add("Analog Dawn", "Deep House", "Warm and saturated: tube drive on the bus, punchier kick, a bright clap.",
    ["deep house", "analog", "warm"], tweak(kit_deep(), t1=dict(color=0.4, snap=0.35), t3=dict(tone=0.55)), DEEP[0], 124, 0.55,
    master(drive=0.18, dmode="Tube", glue=0.5, rdecay=1.6, high=-1, low=1))
add("Lagoon Dub", "Deep House", "Deep and sparse: kick, clap, a rolling conga and very little else.",
    ["deep house", "sparse"], tweak(kit_deep(), t4=dict(level=-16), t8=dict(level=-20)), DEEP[0], 120, 0.58, master(**DEEP_M))
add("Saturn Soul", "Deep House", "Soulful: snare and clap layered, shuffled shaker, a big room.",
    ["deep house", "soulful"], tweak(kit_deep(), t2=dict(level=-4), t3=dict(sweep=0.7)), DEEP[2], 122, 0.58, master(glue=0.4, rdecay=2.4, size=1.4, rret=0.65, high=-1.5))
add("Blue Hour", "Deep House", "Lo-fi deep: dusty, low-passed hats, a rounder kick and swing on everything.",
    ["deep house", "lo-fi"], tweak(kit_deep(), t4=dict(tone=0.3, color=0.7), t5=dict(tone=0.3), t1=dict(tone=0.25, color=0.2)), DEEP[1], 118, 0.62,
    master(glue=0.4, high=-4, low=2, rdecay=1.8))
add("Polar Night", "Deep House", "Moody: a longer kick, sparse rims, the clap thrown into the reverb.",
    ["deep house", "moody"], tweak(kit_deep(), t1=dict(decay=0.62), t3=dict(b=0.45)), DEEP[1], 121, 0.56, master(glue=0.4, rdecay=3.5, size=1.8, rret=0.55, high=-2.5))
add("Coral Groove", "Deep House", "Bouncy: conga accents moved around the bar, shaker eighths, a punchy kick.",
    ["deep house", "bouncy"], tweak(kit_deep(), t1=dict(color=0.35, snap=0.35)), DEEP[0], 123, 0.55, master(**DEEP_M))

# Tech House
TECH_M = dict(glue=0.55, thr=-15, ratio=2.5, rdecay=1.4, size=1.0, rret=0.5, low=1)
add("Photon Groove", "Tech House", "Punchy tech house: tight kick, layered clap, sixteenth hats, FM perc licks with tone locks.",
    ["tech house", "groovy"], kit_tech(), TECH[0], 126, 0.54, master(**TECH_M))
add("Solar Shuffle", "Tech House", "Rolling percussion: a seven-step FM loop and a twelve-step bongo drifting over the bar.",
    ["tech house", "polymeter", "rolling"], kit_tech(), TECH[1], 125, 0.55, master(**TECH_M))
add("Plasma Bounce", "Tech House", "Bongos and a ride-like shaker; the kick drives with more punch.",
    ["tech house", "bounce"], tweak(kit_tech(), t1=dict(color=0.7, snap=0.6)), TECH[2], 127, 0.53, master(**TECH_M))
add("Warehouse Ion", "Tech House", "Raw and driven: saturated kick and clap, tube on the bus.",
    ["tech house", "raw"], tweak(kit_tech(), t1=dict(drive=0.25), t3=dict(drive=0.3)), TECH[0], 128, 0.52,
    master(drive=0.25, dmode="Tube", glue=0.65, thr=-17, ratio=3, rdecay=1.0))
add("Rooftop Flare", "Tech House", "Summer tech house: brighter hats, open-hat lifts, conga-like bongos.",
    ["tech house", "bright"], tweak(kit_tech(), t4=dict(tone=0.7), t5=dict(tone=0.7)), TECH[2], 124, 0.56, master(**TECH_M))
add("Minimal Pulsar", "Tech House", "Minimal tech: short, dry and clicky, the perc carrying the groove.",
    ["tech house", "minimal"], tweak(kit_tech(), t1=dict(decay=0.36), t3=dict(sweep=0.3, decay=0.32), t6=dict(level=-9)), TECH[1], 126, 0.55,
    master(glue=0.5, rdecay=0.9, rret=0.35))
add("Gravity Jack", "Tech House", "Jacking tech house: snare ghosts, the clap on every two and four, cowbell chatter.",
    ["tech house", "jack"], tweak(kit_tech(), t7=dict(model="Quasar Metal", name="Cowbell", tune=0, decay=0.3, tone=0.5, snap=0.5, color=0.0, sweep=0.0, level=-13)),
    TECH[1], 127, 0.55, master(**TECH_M))
add("Neon Rumble", "Tech House", "Darker: the kick feeds the reverb for a rumble, hats pushed back.",
    ["tech house", "dark", "rumble"], tweak(kit_tech(), t1=dict(b=0.2, tone=0.45)), TECH[0], 125, 0.53,
    master(glue=0.55, rdecay=2.8, size=1.6, rret=0.45, high=-2, low=2))
add("Ion Swing", "Tech House", "Heavily swung, with a half-open hat on the off-beats and FM blips.",
    ["tech house", "swing"], tweak(kit_tech(), t5=dict(decay=0.42)), TECH[0], 124, 0.6, master(**TECH_M))
add("Afterhours Orbit", "Tech House", "After-hours: hypnotic, a longer kick, echoes on the perc.",
    ["tech house", "hypnotic"], tweak(kit_tech(), t1=dict(decay=0.5), t6=dict(a=0.45)), TECH[1], 124, 0.54,
    master(glue=0.5, ediv="1/8.", fb=0.45, eret=0.45, rdecay=2.0))
add("Bassline Kicker", "Tech House", "Big, saturated kick with a tom bass-line feel on the bongo track.",
    ["tech house", "big"], tweak(kit_tech(), t1=dict(color=0.75, decay=0.48), t7=dict(model="Orbit Tom", name="Tom Bass", tune=-5, decay=0.35, tone=0.4, snap=0.4, color=0.2, sweep=0.35, level=-7)),
    TECH[2], 126, 0.54, master(glue=0.6, thr=-16, ratio=3, rdecay=1.2, low=1.5))
add("Sunset Terrace", "Tech House", "Lighter, percussive tech house with shakers and congas.",
    ["tech house", "percussive"], tweak(kit_tech(), t7=dict(tune=10, color=0.85)), TECH[2], 123, 0.57, master(**TECH_M))

# Trance
TRANCE_M = dict(glue=0.6, thr=-15, ratio=3, makeup=1, rdecay=2.6, size=1.6, rret=0.55, ediv="1/8.", fb=0.35, eret=0.3, width=1.15)
add("Uplift Engine", "Trance", "Uplifting trance: hard layered kick, off-beat open hat, sixteenth closed hats, clap on two and four.",
    ["trance", "uplifting"], kit_trance(), TRANCE[0], 138, 0.5, master(**TRANCE_M))
add("Stellar Gate", "Trance", "Gated-hat trance: a 3-3-2 closed hat, the ride on the off-beat, big room.",
    ["trance", "classic"], kit_trance(), TRANCE[1], 136, 0.5, master(**TRANCE_M))
add("Supernova Build", "Trance", "A build: snare roll with rising tone in the fill, the riser on the break.",
    ["trance", "build"], tweak(kit_trance(), t2=dict(level=-4, lock="Tone")), TRANCE[0], 138, 0.5, master(**TRANCE_M), macros=(0.15, 0, 0, 0))
add("Tech Trance Core", "Trance", "Tech trance: drier, punchier kick with more top, FM perc arpeggio.",
    ["trance", "tech trance"], tweak(kit_trance(), t1=dict(snap=0.8, color=0.65, decay=0.42), t6=dict(level=-12)), TRANCE[0], 140, 0.5,
    master(glue=0.6, thr=-16, ratio=3, rdecay=1.6, size=1.2, rret=0.45))
add("Progressive Drift", "Trance", "Progressive trance: softer kick, rolling hats, long reverb.",
    ["trance", "progressive"], tweak(kit_trance(), t1=dict(color=0.35, snap=0.5, decay=0.55), t5=dict(level=-9)), TRANCE[1], 132, 0.5,
    master(glue=0.5, rdecay=3.5, size=2.0, rret=0.6, width=1.2))
add("Hard Trance Pulse", "Trance", "Hard trance: distorted kick, Fold on the bus, a driving clap.",
    ["trance", "hard"], tweak(kit_trance(), t1=dict(color=0.8, tone=0.65)), TRANCE[0], 145, 0.5,
    master(drive=0.25, dmode="Tube", glue=0.7, thr=-17, ratio=4, rdecay=2.0))
add("Psy Nebula", "Trance", "Psytrance: short tight kick, sixteenth hats, zaps and FM blips.",
    ["trance", "psytrance"], kit_psy(), PSY[0], 145, 0.5, master(glue=0.6, thr=-15, ratio=3, rdecay=1.8, size=1.3, rret=0.45, ediv="1/16.", fb=0.4, eret=0.35))
add("Full On Flare", "Trance", "Full-on psy: rolling zaps, a pitched FM line, bright hats.",
    ["trance", "psytrance", "full on"], tweak(kit_psy(), t4=dict(tone=0.85), t6=dict(level=-14)), PSY[0], 146, 0.5,
    master(glue=0.65, thr=-16, ratio=3, rdecay=1.5, ediv="1/16.", fb=0.45, eret=0.4))
add("Goa Sunrise", "Trance", "Goa-flavoured psy at a slower tempo, with more echo and space.",
    ["trance", "goa"], tweak(kit_psy(), t1=dict(decay=0.36)), PSY[0], 140, 0.5,
    master(glue=0.5, rdecay=2.8, size=1.6, rret=0.55, ediv="1/8.", fb=0.5, eret=0.45))
add("Vocal Trance Air", "Trance", "Airy and open: lighter kick, wide hats, a snare instead of the clap.",
    ["trance", "vocal"], tweak(kit_trance(), t1=dict(color=0.4, snap=0.55), t2=dict(level=-3)), TRANCE[1], 134, 0.5, master(**TRANCE_M))
add("Eclipse Anthem", "Trance", "Anthem: big room, crash on the break, a heavier clap.",
    ["trance", "anthem"], tweak(kit_trance(), t8=dict(model="Corona Cymbal", name="Crash", tune=0, decay=0.7, tone=0.6, snap=0.4, color=0.85, sweep=0.8, level=-12)),
    TRANCE[0], 138, 0.5, master(**TRANCE_M))
add("Orbit Trance", "Trance", "Driving 140 with a rolling 3-3-2 hat and a short kick.",
    ["trance", "driving"], tweak(kit_trance(), t1=dict(decay=0.4)), TRANCE[1], 140, 0.5, master(**TRANCE_M))

# Drum & Bass
DNB_M = dict(glue=0.6, thr=-15, ratio=3, rdecay=1.2, size=0.9, rret=0.45, low=1)
add("Liquid Corona", "Drum & Bass", "Liquid DnB: tight kick, crisp long snare, ghost notes, shaker and hats.",
    ["drum and bass", "liquid"], kit_dnb(), DNB[0], 174, 0.5, master(**DNB_M))
add("Roller Engine", "Drum & Bass", "Roller: ride on the beats, hat chatter, a kick that skips.",
    ["drum and bass", "roller"], kit_dnb(), DNB[1], 174, 0.5, master(**DNB_M))
add("Neuro Pulse", "Drum & Bass", "Neurofunk: harder kick with more top, a cracking snare, busy sixteenth hats.",
    ["drum and bass", "neuro"], tweak(kit_dnb(), t1=dict(color=0.6, snap=0.85, decay=0.28), t2=dict(snap=0.95, tone=0.7, drive=0.25)), DNB[2], 174, 0.5,
    master(drive=0.2, dmode="Tube", glue=0.7, thr=-17, ratio=4, rdecay=0.9))
add("Jungle Comet", "Drum & Bass", "Jungle: chopped break pattern, ratchets and rolls, the snare pitched up.",
    ["drum and bass", "jungle"], tweak(kit_dnb(), t1=dict(model="Core Kick", tune=3, decay=0.35, tone=0.55, snap=0.5, color=0.45, sweep=0.5), t2=dict(tune=6, decay=0.42)),
    JUNGLE[0], 168, 0.53, master(glue=0.55, rdecay=1.0, high=-1))
add("Halftime Void", "Drum & Bass", "Halftime: the snare on beat three, deep kick, sparse hats.",
    ["drum and bass", "halftime"], tweak(kit_dnb(), t1=dict(tune=1, decay=0.42), t2=dict(decay=0.6)), HALFTIME[0], 170, 0.5,
    master(glue=0.6, rdecay=1.8, size=1.3, rret=0.55, low=1.5))
add("Jump Up Spark", "Drum & Bass", "Jump-up: bouncy two-step with a big clap-snare and a rim.",
    ["drum and bass", "jump up"], tweak(kit_dnb(), t2=dict(color=0.75, sweep=0.5), t7=dict(level=-9)), DNB[0], 175, 0.5, master(**DNB_M))
add("Atmos Drift", "Drum & Bass", "Atmospheric: soft hats, ride wash, a long reverb on the snare.",
    ["drum and bass", "atmospheric"], tweak(kit_dnb(), t2=dict(b=0.35), t6=dict(level=-13)), DNB[1], 172, 0.5,
    master(glue=0.5, rdecay=3.0, size=1.8, rret=0.6, high=-1.5))
add("Minimal Stepper", "Drum & Bass", "Minimal DnB: dry, the snare and kick doing everything.",
    ["drum and bass", "minimal"], tweak(kit_dnb(), t4=dict(level=-14), t8=dict(level=-22)), DNB[0], 172, 0.5, master(glue=0.5, rdecay=0.8, rret=0.3))
add("Amen Orbit", "Drum & Bass", "Break-style: a jungle chop with a brighter snare and swing.",
    ["drum and bass", "breaks"], tweak(kit_dnb(), t1=dict(model="Core Kick", decay=0.4, tone=0.6, snap=0.55, color=0.5, sweep=0.45), t2=dict(tone=0.7, snap=0.7)),
    JUNGLE[0], 165, 0.55, master(glue=0.55, rdecay=0.9))
add("Dancefloor Liquid", "Drum & Bass", "Dancefloor liquid: a rounder kick, shaker sixteenths, open hat lifts.",
    ["drum and bass", "liquid"], tweak(kit_dnb(), t1=dict(color=0.3, snap=0.55)), DNB[0], 174, 0.5, master(**DNB_M))
add("Tech Step Grid", "Drum & Bass", "Techstep: cold, metallic, rim and ride driving.",
    ["drum and bass", "techstep"], tweak(kit_dnb(), t7=dict(level=-9, color=0.3), t6=dict(level=-13)), DNB[1], 174, 0.5, master(**DNB_M))
add("Halftime Gravity", "Drum & Bass", "Heavy halftime: distorted kick, a slapping snare, Tube on the bus.",
    ["drum and bass", "halftime", "heavy"], tweak(kit_dnb(), t1=dict(color=0.7, decay=0.45), t2=dict(drive=0.3)), HALFTIME[0], 174, 0.5,
    master(drive=0.25, dmode="Tube", glue=0.65, rdecay=1.5))

# Techno
TECHNO_M = dict(glue=0.6, thr=-15, ratio=3, rdecay=3.0, size=1.6, rret=0.5, low=1)
add("Event Horizon", "Techno", "Peak-time techno: driven kick, a twelve-step perc loop drifting against the bar.",
    ["techno", "peak", "polymeter"], kit_techno(), TECHNO[1], 132, 0.5, master(**TECHNO_M))
add("Neutron Pulse", "Techno", "Hypnotic: a rolling 1/32 perc, the open hat off-beat, locked tone sweeps.",
    ["techno", "hypnotic"], kit_techno(), TECHNO[0], 130, 0.5, master(glue=0.5, rdecay=2.5, ediv="1/8.", fb=0.45, eret=0.35))
add("Tachyon Rumble", "Techno", "Rumble techno: the kick fed to the reverb, saturated, hats pushed.",
    ["techno", "rumble"], tweak(kit_techno(), t1=dict(color=0.6, b=0.25, decay=0.45)), TECHNO[0], 134, 0.5,
    master(drive=0.25, dmode="Tube", glue=0.6, e2r=0.6, ediv="1/4", fb=0.4, eret=0.25, rdecay=4, size=1.8, rret=0.4, low=2))
add("Hard Orbit", "Techno", "Hard techno: distorted kick and clap, metallic perc, sixteenth hats.",
    ["techno", "hard"], tweak(kit_techno(), t1=dict(color=0.85, tone=0.55), t3=dict(drive=0.5, color=0.85)), TECHNO[1], 145, 0.5,
    master(drive=0.35, dmode="Tube", glue=0.7, thr=-18, ratio=4, rdecay=1.8))
add("Deep Space Dub", "Techno", "Dub techno: echoes on the rim and perc, long dark reverb.",
    ["techno", "dub"], tweak(kit_techno(), t6=dict(a=0.6, b=0.4), t7=dict(a=0.6, b=0.4), t1=dict(tone=0.3, color=0.3)), TECHNO[0], 122, 0.5,
    master(glue=0.4, ediv="1/4.", fb=0.62, edamp=0.7, eret=0.55, e2r=0.5, rdecay=6, size=2.2, rret=0.6, high=-3), macros=(0, 0, 0, 0.35))
add("Cold Fusion", "Techno", "Minimal techno: everything short and dry, the rim carrying the groove.",
    ["techno", "minimal"], tweak(kit_techno(), t1=dict(decay=0.32), t7=dict(level=-6), t8=dict(level=-18)), TECHNO[0], 127, 0.53,
    master(glue=0.3, rdecay=0.9, rret=0.4))
add("Melodic Machine", "Techno", "Melodic techno drums: softer kick, a modal pluck line, long space.",
    ["techno", "melodic"], tweak(kit_melodic(), t1=dict(model="Gravity Kick", tune=-1, decay=0.45, tone=0.45, snap=0.45, color=0.35, sweep=0.5)), MELODIC[0], 124, 0.5,
    master(glue=0.5, rdecay=3.5, size=2.0, rret=0.55, ediv="1/8.", fb=0.4, eret=0.35))
add("Industrial Flare", "Techno", "Industrial: Fold on the bus, the clap crushed, a rumbling kick.",
    ["techno", "industrial"], tweak(kit_techno(), t1=dict(color=0.7), t3=dict(drive=0.6)), TECHNO[1], 138, 0.5,
    master(drive=0.3, dmode="Fold", glue=0.6, rdecay=2.5, size=1.5))

# House
HOUSE_M = dict(glue=0.5, thr=-14, ratio=2.5, rdecay=1.3, size=0.9, rret=0.55)
add("Photon Floor", "House", "Classic four-on-the-floor: round kick, off-beat open hat, a clap on two and four.",
    ["house", "classic"], kit_house(), HOUSE[0], 124, 0.54, master(**HOUSE_M))
add("Disco Ion", "House", "Disco house: open hats on the off-beat, a bright clap, the cowbell answering.",
    ["house", "disco"], tweak(kit_house(), t5=dict(tone=0.7, level=-7), t7=dict(level=-11)), HOUSE[1], 120, 0.52, master(glue=0.5, high=1.5, rdecay=1.0, rret=0.5))
add("Chicago Plasma", "House", "Chicago jack: driven kick, a heavy clap, cowbell on the and.",
    ["house", "chicago", "jack"], tweak(kit_house(), t1=dict(drive=0.3, color=0.6), t3=dict(drive=0.3)), HOUSE[1], 125, 0.55,
    master(drive=0.15, dmode="Tube", glue=0.55, rdecay=0.8, rret=0.5))
add("Piano Stomp", "House", "Piano-house energy: punchy kick, double clap, the tom pickup into the fill.",
    ["house", "piano"], tweak(kit_house(), t3=dict(color=0.75, level=-1), t6=dict(level=-4)), HOUSE[0], 126, 0.5,
    master(glue=0.6, ratio=3, makeup=1, rdecay=1.5, rret=0.6, low=1))
add("French Filter", "House", "French-touch: filtered, pumping, a driven bus.",
    ["house", "french", "filter"], tweak(kit_house(), t4=dict(filt="LP 12", cutoff=7000.0), t5=dict(filt="LP 12", cutoff=6000.0)), HOUSE[0], 123, 0.53,
    master(drive=0.2, dmode="Soft", glue=0.75, thr=-18, ratio=4, rel=120.0, rdecay=1.0))
add("Sunrise Garage", "House", "Garage house: shuffled, the shaker on every step, snare and clap together.",
    ["house", "garage"], tweak(kit_house(), t8=dict(model="Solar Wind", name="Shaker", tune=11, decay=0.2, tone=0.4, snap=0.6, color=0.7, sweep=0.5, level=-14)), HOUSE[0], 122, 0.6,
    master(**HOUSE_M))

# UK Garage & Breaks
add("Two Step Photon", "UK Garage & Breaks", "Two-step: skipping kick, shuffled hats nudged late, rimshot chatter.",
    ["uk garage", "2-step"], kit_garage(), GARAGE[0], 132, 0.62, master(glue=0.5, rdecay=1.2))
add("Speed Garage Ion", "UK Garage & Breaks", "Speed garage: a punchier kick, the open hat lifting, more top end.",
    ["uk garage", "speed garage"], tweak(kit_garage(), t1=dict(color=0.6, snap=0.55), t5=dict(level=-8)), GARAGE[0], 134, 0.58, master(glue=0.6, rdecay=1.0))
add("Breakbeat Comet", "UK Garage & Breaks", "Breakbeat: a punchy kick, ghost snares, the open hat on the and.",
    ["breaks", "breakbeat"], kit_garage(), BREAKS[0], 130, 0.53, master(glue=0.55, rdecay=1.0))
add("Nu Skool Flare", "UK Garage & Breaks", "Nu-skool breaks: a harder kick, driven snare, Tube on the bus.",
    ["breaks", "nu skool"], tweak(kit_garage(), t1=dict(model="Gravity Kick", decay=0.38, tone=0.55, snap=0.6, color=0.55, sweep=0.5), t2=dict(drive=0.3)), BREAKS[0], 134, 0.5,
    master(drive=0.2, dmode="Tube", glue=0.65, rdecay=0.9))
add("Bassline Orbit", "UK Garage & Breaks", "Bassline house: bouncy, a tom bass-line feel, the shaker shuffling.",
    ["uk garage", "bassline"], tweak(kit_garage(), t7=dict(tune=-7, decay=0.35, color=0.2, level=-6)), GARAGE[0], 136, 0.6, master(glue=0.55, rdecay=1.0))
add("Grime Pulse", "UK Garage & Breaks", "Grime-style: sparse kick, a hard snare, sixteenth hats with rolls.",
    ["uk garage", "grime"], tweak(kit_garage(), t2=dict(snap=0.8, tone=0.65, level=-2)), BREAKS[0], 140, 0.5, master(glue=0.6, rdecay=0.9))

# Progressive & Melodic
MEL_M = dict(glue=0.45, thr=-14, ratio=2.2, rdecay=3.0, size=1.8, rret=0.6, ediv="1/8.", fb=0.4, eret=0.35, high=-1, width=1.15)
add("Aurora Path", "Progressive & Melodic", "Melodic house: soft kick, a tuned tom line, modal plucks into echo and reverb.",
    ["melodic", "progressive"], kit_melodic(), MELODIC[0], 122, 0.52, master(**MEL_M))
add("Afro Nebula", "Progressive & Melodic", "Afro house: congas and toms leading, the shaker swung, a deep kick.",
    ["afro house", "percussion"], tweak(kit_melodic(), t6=dict(model="Orbit Tom", name="Conga", tune=12, decay=0.38, tone=0.5, snap=0.5, color=0.8, sweep=0.3, level=-6)), MELODIC[0], 120, 0.57,
    master(**MEL_M))
add("Cosmic Progressive", "Progressive & Melodic", "Progressive house: rolling, the pluck pattern moving every bar.",
    ["progressive", "house"], kit_melodic(), MELODIC[0], 124, 0.5, master(**MEL_M), macros=(0, 0, 0.1, 0.25))
add("Glass Horizon", "Progressive & Melodic", "Glass-like modal tones over a soft four-on-the-floor.",
    ["melodic", "glass"], tweak(kit_melodic(), t7=dict(color=0.95, tone=0.7, decay=0.5, tune=12)), MELODIC[0], 118, 0.52, master(**MEL_M))
add("Organic Drift", "Progressive & Melodic", "Organic downtempo: kalimba-like bar plucks, toms, long tails.",
    ["melodic", "organic", "downtempo"], tweak(kit_melodic(), t7=dict(color=0.5, tone=0.6, snap=0.4), t1=dict(decay=0.6)), MELODIC[0], 112, 0.56,
    master(glue=0.4, rdecay=3.8, size=2.0, rret=0.65, ediv="1/4.", fb=0.45, eret=0.4, high=-2))
add("Stargazer", "Progressive & Melodic", "Melodic techno: a darker kick, hats pushed back, wide space.",
    ["melodic", "techno"], tweak(kit_melodic(), t1=dict(model="Gravity Kick", tune=-2, decay=0.5, tone=0.4, snap=0.4, color=0.4, sweep=0.5), t4=dict(level=-14)), MELODIC[0], 123, 0.5,
    master(**MEL_M))

# More of each
add("Opal Basement", "Deep House", "Basement deep: dry and close, the rim and shaker tight to the kick.",
    ["deep house", "dry"], tweak(kit_deep(), t3=dict(sweep=0.3, b=0.1), t7=dict(a=0.0)), DEEP[1], 121, 0.56, master(glue=0.45, rdecay=0.9, rret=0.35, high=-2))
add("Moonwater", "Deep House", "Floaty deep: soft kick, the open hat wide in the room, echoing congas.",
    ["deep house", "floaty"], tweak(kit_deep(), t1=dict(color=0.15, snap=0.15), t6=dict(a=0.4)), DEEP[0], 119, 0.57,
    master(glue=0.35, ediv="1/8.", fb=0.45, eret=0.4, rdecay=2.6, size=1.6, rret=0.65, high=-2.5))
add("Kinetic Deep", "Deep House", "Driving deep house: a punchier kick, snare ghosts and sixteenth hats.",
    ["deep house", "driving"], tweak(kit_deep(), t1=dict(color=0.45, snap=0.4, tone=0.45)), DEEP[2], 124, 0.54, master(**DEEP_M))
add("Circuit Groove", "Tech House", "Tech house with a punchy tom bass line on the perc track and dry hats.",
    ["tech house", "tom"], tweak(kit_tech(), t6=dict(model="Orbit Tom", name="Tom", tune=-3, decay=0.35, tone=0.45, snap=0.45, color=0.25, sweep=0.4, level=-8)),
    TECH[0], 126, 0.54, master(**TECH_M))
add("Hypercube", "Tech House", "Twisted tech: FM ratio locks, a seven-step perc and claps in the reverb.",
    ["tech house", "twisted"], tweak(kit_tech(), t6=dict(lock="Color", tone=0.6), t3=dict(b=0.35)), TECH[1], 127, 0.53, master(**TECH_M))
add("Golden Hour Tech", "Tech House", "Warm, rolling tech house: rounder kick, conga-like bongos, shaker.",
    ["tech house", "warm"], tweak(kit_tech(), t1=dict(color=0.45, tone=0.45), t7=dict(color=0.85, tune=12)), TECH[2], 124, 0.56,
    master(glue=0.5, rdecay=1.6, high=-1.5, low=1))
add("Ascension", "Trance", "Classic 138: lighter open hats, ride sixteenths, the snare roll building.",
    ["trance", "classic"], tweak(kit_trance(), t5=dict(level=-8), t7=dict(level=-12)), TRANCE[0], 138, 0.5, master(**TRANCE_M))
add("Dark Psy Vortex", "Trance", "Dark psy: shorter kick, darker hats, zaps in the echo.",
    ["trance", "psytrance", "dark"], tweak(kit_psy(), t4=dict(tone=0.55), t6=dict(a=0.5)), PSY[0], 148, 0.5,
    master(glue=0.65, rdecay=1.6, ediv="1/16.", fb=0.55, eret=0.45, high=-1.5))
add("Balearic Trance", "Trance", "Balearic: softer, a rim instead of the perc, gentle 132.",
    ["trance", "balearic"], tweak(kit_trance(), t1=dict(color=0.35, snap=0.5), t6=dict(model="Spark Rim", name="Rim", tune=0, decay=0.3, tone=0.5, snap=0.5, color=0.2, sweep=0.5, level=-12)),
    TRANCE[1], 132, 0.5, master(glue=0.5, rdecay=3.0, size=1.8, rret=0.6, width=1.2))
add("Nightrider", "Drum & Bass", "Dark roller: a heavier kick, ride on the beats, rim chatter.",
    ["drum and bass", "dark", "roller"], tweak(kit_dnb(), t1=dict(color=0.55, tone=0.5), t7=dict(level=-10)), DNB[1], 174, 0.5,
    master(glue=0.6, rdecay=1.4, high=-1.5, low=1.5))
add("Sky Liquid", "Drum & Bass", "Bright liquid: brighter hats and snare, open hat lifts, shaker on top.",
    ["drum and bass", "liquid", "bright"], tweak(kit_dnb(), t2=dict(tone=0.7), t4=dict(tone=0.75)), DNB[0], 172, 0.5, master(**DNB_M))
add("Rinse Out", "Drum & Bass", "Jump-up energy with a jungle snare roll in the fill.",
    ["drum and bass", "jump up"], tweak(kit_dnb(), t1=dict(color=0.55, snap=0.8)), JUNGLE[0], 175, 0.5, master(glue=0.65, rdecay=0.9))
add("Acid Pulsar", "Techno", "Acid-era techno: Pulsar FM lead perc with ratio locks, tight hats.",
    ["techno", "acid"], tweak(kit_techno(), t6=dict(tune=0, decay=0.35, tone=0.75, color=0.55, lock="Color", level=-8)), TECHNO[0], 131, 0.52,
    master(glue=0.5, ediv="1/16.", fb=0.5, eret=0.4, rdecay=1.8))
add("Pulsar Array", "Techno", "Polyrhythmic techno: twelve- and eleven-step loops over a straight kick.",
    ["techno", "polymeter"], tweak(kit_techno(), t7=dict(model="Quasar Metal", name="Bell", tune=12, decay=0.3, color=0.8, sweep=0.3, level=-13)), TECHNO[1], 129, 0.5,
    master(glue=0.5, rdecay=3, ediv="1/8.", fb=0.35, eret=0.4))
add("Soulful Signal", "House", "Soulful house: snare and clap, shaker eighths, a warm room.",
    ["house", "soulful"], tweak(kit_house(), t2=dict(level=-3)), HOUSE[1], 122, 0.57, master(glue=0.45, rdecay=1.8, size=1.3, rret=0.6, high=-1))
add("Electric Ballroom", "House", "Vocal-house drums: tight kick, bright clap, ride and open hats.",
    ["house", "vocal"], tweak(kit_house(), t8=dict(level=-11)), HOUSE[0], 124, 0.52, master(**HOUSE_M))
add("Acid House Orbit", "House", "Acid house: a 909-style kick and clap, the cowbell and open hats.",
    ["house", "acid"], tweak(kit_house(), t1=dict(color=0.55, snap=0.55)), HOUSE[1], 123, 0.54, master(glue=0.5, rdecay=1.0))
add("Funky Flare", "House", "Funky house: tom pickups, cowbell, a shuffled groove.",
    ["house", "funky"], tweak(kit_house(), t6=dict(level=-4)), HOUSE[0], 125, 0.57, master(**HOUSE_M))
add("Garage Nightbus", "UK Garage & Breaks", "Dark two-step: deeper kick, a rim that rides the swing.",
    ["uk garage", "dark"], tweak(kit_garage(), t1=dict(tune=-2, decay=0.5), t6=dict(level=-8)), GARAGE[0], 130, 0.63, master(glue=0.5, rdecay=1.6, high=-1.5))
add("Breakdance Comet", "UK Garage & Breaks", "Electro-breaks: a punchy kick, snare and clap, sixteenth hats.",
    ["breaks", "electro"], tweak(kit_garage(), t1=dict(color=0.65, snap=0.6)), BREAKS[0], 128, 0.52, master(glue=0.6, rdecay=1.0))
add("Future Garage Mist", "UK Garage & Breaks", "Future garage: soft hats, the snare in a big reverb, slow swing.",
    ["uk garage", "future garage"], tweak(kit_garage(), t2=dict(b=0.4), t4=dict(level=-12)), GARAGE[0], 132, 0.6,
    master(glue=0.4, rdecay=3.2, size=1.8, rret=0.6, high=-2.5))
add("Big Beat Booster", "UK Garage & Breaks", "Big beat: a driven kick and snare at 120, Tube on the bus.",
    ["breaks", "big beat"], tweak(kit_garage(), t1=dict(drive=0.35), t2=dict(drive=0.35, level=-2)), BREAKS[0], 120, 0.52,
    master(drive=0.3, dmode="Tube", glue=0.7, thr=-18, ratio=4, rdecay=1.2))
add("Desert Bloom", "Progressive & Melodic", "Organic house: hand drums leading, a bar pluck and space.",
    ["organic house", "percussion"], tweak(kit_melodic(), t6=dict(model="Orbit Tom", name="Conga", tune=10, decay=0.4, tone=0.5, snap=0.5, color=0.85, sweep=0.3, level=-6), t7=dict(color=0.5)),
    MELODIC[0], 117, 0.56, master(**MEL_M))
add("Northern Lights", "Progressive & Melodic", "Progressive trance-house: a firmer kick, glass plucks, wide reverb.",
    ["progressive", "melodic"], tweak(kit_melodic(), t1=dict(color=0.5, snap=0.45), t7=dict(color=0.9, tune=12)), MELODIC[0], 126, 0.5, master(**MEL_M))
add("Slow Burn", "Progressive & Melodic", "Downtempo melodic: 108, a soft kick, toms and long tails.",
    ["melodic", "downtempo"], tweak(kit_melodic(), t1=dict(decay=0.62, color=0.25)), MELODIC[0], 108, 0.55,
    master(glue=0.4, rdecay=4.0, size=2.2, rret=0.65, ediv="1/4.", fb=0.45, eret=0.4, high=-2))


# ---- Output -----------------------------------------------------------------------------------------------------
def track_params(i, t):
    x = "t%d" % (i + 1)
    p = {x + "Model": t["model"], x + "Tune": t["tune"], x + "Decay": t["decay"], x + "Tone": t["tone"], x + "Snap": t["snap"],
         x + "Color": t["color"], x + "Sweep": t["sweep"], x + "Level": t["level"], x + "SendB": t["b"],
         x + "Choke": ["None", "1", "2", "3", "4"][t["choke"]], x + "Note": GM[i], x + "Lock": t["lock"]}
    extra = {"Drive": t["drive"], "Filter": t["filt"], "Cutoff": t["cutoff"], "Res": t["res"], "Vel": t["vel"], "Pan": t["pan"], "SendA": t["a"]}
    for k, val in extra.items():
        if val != GENERIC[k]:
            p[x + k] = val
    return p


def build(levels):
    out = [{"name": "Init", "category": "Init", "author": "Chromosphere", "tags": ["init"],
            "description": "One of each drum and the Init beat: a starting point.", "params": {},
            "names": ["Kick", "Snare", "Clap", "Closed Hat", "Open Hat", "Low Tom", "High Tom", "Metal"],
            "patterns": [pattern("x...x...x...x...", "", "....x.......x...", "v.o.v.o.v.o.v.o.", "...............v")]}]
    seen = set()
    for pr in PRESETS:
        assert pr["name"] not in seen, pr["name"]
        seen.add(pr["name"])
        params = {}
        for i, t in enumerate(pr["kit"]):
            params.update(track_params(i, t))
        m = dict(pr["master"])
        m["outDb"] = round(m["outDb"] + levels.get(pr["name"], 0.0), 2)
        params.update(m)
        params["tempo"] = pr["tempo"]
        params["swing"] = pr["swing"]
        for j, mv in enumerate(pr["macros"]):
            if mv:
                params["macro%d" % (j + 1)] = mv
        out.append({"name": pr["name"], "category": pr["category"], "author": "Chromosphere", "tags": pr["tags"],
                    "description": pr["description"], "params": params, "names": [t["name"] for t in pr["kit"]],
                    "patterns": pr["groove"]})
    return {"version": 1, "presets": out}


def main():
    levels = {}
    if os.path.exists(LEVELS):
        levels = json.load(open(LEVELS))
    if len(sys.argv) > 2 and sys.argv[1] == "--calibrate":
        measured = json.load(open(sys.argv[2]))
        for name, loud in measured.items():
            if name == "Init":
                continue
            adj = max(-12.0, min(12.0, levels.get(name, 0.0) + (TARGET_LOUDNESS - loud)))
            levels[name] = round(adj, 2)
        json.dump(levels, open(LEVELS, "w"), indent=1, sort_keys=True)
    data = build(levels)
    # One compact preset per line: small to embed, readable in a diff.
    with open(OUT, "w") as f:
        f.write('{"version": 1, "presets": [\n')
        f.write(",\n".join(json.dumps(p, separators=(",", ":")) for p in data["presets"]))
        f.write("\n]}\n")
    cats = {}
    for p in data["presets"]:
        cats[p["category"]] = cats.get(p["category"], 0) + 1
    print("wrote %d presets: %s" % (len(data["presets"]), ", ".join("%s %d" % kv for kv in cats.items())))


if __name__ == "__main__":
    main()
