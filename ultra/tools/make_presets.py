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
def kit_house():
    return [v("Core Kick", "Kick", tune=2, decay=0.42, tone=0.55, snap=0.45, color=0.3, sweep=0.42, b=0.0),
            v("Flare Snare", "Snare", tune=1, decay=0.35, tone=0.55, snap=0.5, color=0.6, sweep=0.3, level=-4, b=0.12),
            v("Burst Clap", "Clap", decay=0.42, tone=0.5, snap=0.55, color=0.6, sweep=0.45, level=-2, b=0.15),
            v("Ion Hat", "Closed Hat", decay=0.18, tone=0.6, snap=0.5, color=0.35, sweep=0.15, level=-7, choke=1, pan=0.1),
            v("Ion Hat", "Open Hat", decay=0.6, tone=0.55, snap=0.35, color=0.4, sweep=0.25, level=-8, choke=1, pan=0.1, b=0.08),
            v("Orbit Tom", "Low Tom", tune=-4, decay=0.4, tone=0.4, snap=0.35, color=0.25, sweep=0.4, level=-5, pan=-0.25, b=0.1),
            v("Spark Rim", "Rim", decay=0.3, tone=0.5, snap=0.5, color=0.2, sweep=0.1, level=-8, pan=0.3),
            v("Corona Cymbal", "Ride", decay=0.55, tone=0.6, snap=0.3, color=0.45, sweep=0.35, level=-12, pan=-0.2, b=0.06)]


def kit_techno():
    return [v("Gravity Kick", "Kick", tune=1, decay=0.35, tone=0.4, snap=0.55, color=0.35, sweep=0.5, drive=0.25, b=0.0),
            v("Flare Snare", "Snare", tune=-2, decay=0.4, tone=0.45, snap=0.55, color=0.7, sweep=0.2, level=-5, b=0.2),
            v("Burst Clap", "Clap", decay=0.5, tone=0.45, snap=0.6, color=0.7, sweep=0.6, level=-4, b=0.25),
            v("Ion Hat", "Closed Hat", decay=0.14, tone=0.7, snap=0.6, color=0.25, sweep=0.1, level=-8, choke=1, pan=-0.15),
            v("Ion Hat", "Open Hat", decay=0.52, tone=0.65, snap=0.4, color=0.3, sweep=0.2, level=-9, choke=1, pan=-0.15, b=0.1),
            v("Pulsar FM", "Perc", tune=5, decay=0.3, tone=0.5, snap=0.6, color=0.45, sweep=0.3, level=-9, pan=0.3, a=0.25, b=0.15, lock="Color"),
            v("Spark Rim", "Rim", tune=-3, decay=0.35, tone=0.55, snap=0.6, color=0.1, sweep=0.15, level=-9, pan=-0.35, a=0.2),
            v("Corona Cymbal", "Ride", decay=0.5, tone=0.7, snap=0.4, color=0.6, sweep=0.3, level=-13, pan=0.2)]


def kit_808():
    return [v("Core Kick", "808", tune=-2, decay=0.72, tone=0.4, snap=0.35, color=0.15, sweep=0.35, b=0.0, lock="Decay"),
            v("Flare Snare", "Snare", tune=2, decay=0.35, tone=0.6, snap=0.5, color=0.55, sweep=0.25, level=-3, b=0.1),
            v("Burst Clap", "Clap", decay=0.38, tone=0.55, snap=0.6, color=0.55, sweep=0.35, level=-3, b=0.12),
            v("Ion Hat", "Closed Hat", decay=0.15, tone=0.6, snap=0.45, color=0.15, sweep=0.1, level=-8, choke=1),
            v("Ion Hat", "Open Hat", decay=0.55, tone=0.55, snap=0.35, color=0.2, sweep=0.2, level=-9, choke=1),
            v("Orbit Tom", "Tom", tune=-2, decay=0.5, tone=0.3, snap=0.3, color=0.1, sweep=0.5, level=-5, pan=-0.2),
            v("Spark Rim", "Rim", decay=0.25, tone=0.45, snap=0.4, color=0.1, sweep=0.05, level=-8, pan=0.25),
            v("Quasar Metal", "Cowbell", decay=0.35, tone=0.5, snap=0.4, color=0.0, sweep=0.0, level=-11, pan=0.2)]


def kit_trap():
    return tweak(kit_808(),
                 t1=dict(name="808", tune=-5, decay=0.85, tone=0.35, snap=0.4, color=0.35, sweep=0.3, drive=0.2),
                 t2=dict(name="Snare", tune=4, decay=0.3, tone=0.65, snap=0.6, color=0.6, sweep=0.35, level=-3),
                 t3=dict(name="Clap", decay=0.35, color=0.75, level=-4),
                 t4=dict(name="Hats", decay=0.1, tone=0.7, color=0.25, level=-8),
                 t5=dict(name="Open Hat", decay=0.48, level=-10),
                 t6=dict(model="Plasma Zap", name="Zap", tune=12, decay=0.25, tone=0.6, snap=0.6, color=0.3, sweep=0.5, level=-14, b=0.2),
                 t7=dict(model="Pulsar FM", name="Perc", tune=7, decay=0.2, tone=0.4, snap=0.5, color=0.3, sweep=0.2, level=-12, pan=0.35),
                 t8=dict(model="Solar Wind", name="Riser", tune=0, decay=0.75, tone=0.4, snap=0.0, color=0.2, sweep=0.95, level=-16, b=0.3))


def kit_dnb():
    return [v("Core Kick", "Kick", tune=4, decay=0.33, tone=0.65, snap=0.6, color=0.4, sweep=0.5, b=0.0),
            v("Flare Snare", "Snare", tune=4, decay=0.38, tone=0.65, snap=0.65, color=0.55, sweep=0.35, level=-2, b=0.18),
            v("Flare Snare", "Ghost", tune=6, decay=0.22, tone=0.6, snap=0.4, color=0.45, sweep=0.2, level=-8, pan=0.15, b=0.1),
            v("Ion Hat", "Hat", decay=0.12, tone=0.7, snap=0.5, color=0.4, sweep=0.1, level=-9, choke=1, pan=0.2),
            v("Ion Hat", "Open Hat", decay=0.45, tone=0.65, snap=0.4, color=0.45, sweep=0.2, level=-10, choke=1, pan=0.2),
            v("Corona Cymbal", "Ride", decay=0.6, tone=0.65, snap=0.35, color=0.5, sweep=0.3, level=-14, pan=-0.25),
            v("Spark Rim", "Rim", tune=2, decay=0.3, tone=0.6, snap=0.5, color=0.15, level=-10, pan=-0.3),
            v("Solar Wind", "Shaker", tune=5, decay=0.18, tone=0.35, snap=0.7, color=0.3, sweep=0.5, level=-15, pan=0.35)]


def kit_breaks():
    return [v("Core Kick", "Kick", tune=1, decay=0.4, tone=0.6, snap=0.55, color=0.55, sweep=0.4, drive=0.25, b=0.0),
            v("Flare Snare", "Snare", tune=0, decay=0.42, tone=0.55, snap=0.55, color=0.6, sweep=0.3, drive=0.2, level=-2, b=0.2),
            v("Flare Snare", "Ghost", tune=3, decay=0.25, tone=0.5, snap=0.35, color=0.5, sweep=0.2, level=-9, pan=-0.1, b=0.1),
            v("Ion Hat", "Hat", decay=0.16, tone=0.55, snap=0.45, color=0.5, sweep=0.15, level=-9, choke=1, pan=0.2),
            v("Ion Hat", "Open Hat", decay=0.5, tone=0.5, snap=0.35, color=0.55, sweep=0.2, level=-10, choke=1, pan=0.2),
            v("Corona Cymbal", "Crash", decay=0.75, tone=0.55, snap=0.5, color=0.55, sweep=0.6, level=-13, pan=-0.2, b=0.1),
            v("Orbit Tom", "Tom", tune=0, decay=0.45, tone=0.45, snap=0.45, color=0.3, sweep=0.45, level=-6, pan=-0.3),
            v("Spark Rim", "Rim", decay=0.28, tone=0.5, snap=0.5, color=0.3, level=-10, pan=0.3)]


def kit_garage():
    return [v("Core Kick", "Kick", tune=0, decay=0.45, tone=0.5, snap=0.4, color=0.25, sweep=0.38, b=0.0),
            v("Flare Snare", "Snare", tune=3, decay=0.33, tone=0.6, snap=0.55, color=0.55, sweep=0.25, level=-4, b=0.15),
            v("Burst Clap", "Clap", decay=0.4, tone=0.55, snap=0.5, color=0.5, sweep=0.4, level=-3, b=0.2),
            v("Ion Hat", "Hat", decay=0.13, tone=0.65, snap=0.5, color=0.45, sweep=0.1, level=-8, choke=1, pan=0.15),
            v("Ion Hat", "Open Hat", decay=0.5, tone=0.6, snap=0.35, color=0.5, sweep=0.2, level=-9, choke=1, pan=0.15),
            v("Spark Rim", "Rim", tune=-2, decay=0.35, tone=0.5, snap=0.5, color=0.3, level=-7, pan=-0.25, a=0.15),
            v("Solar Wind", "Shaker", tune=3, decay=0.2, tone=0.3, snap=0.65, color=0.25, sweep=0.5, level=-13, pan=0.35),
            v("Nebula Modal", "Wood", tune=7, decay=0.3, tone=0.6, snap=0.8, color=0.5, sweep=0.3, level=-10, pan=-0.35)]


def kit_lofi():
    return [v("Core Kick", "Kick", tune=-1, decay=0.4, tone=0.3, snap=0.3, color=0.3, sweep=0.35, drive=0.15, filt="LP 12", cutoff=3500, b=0.0),
            v("Flare Snare", "Snare", tune=-1, decay=0.4, tone=0.4, snap=0.4, color=0.6, sweep=0.2, drive=0.2, filt="LP 12", cutoff=6000, level=-3, b=0.2),
            v("Spark Rim", "Rim", tune=-4, decay=0.35, tone=0.4, snap=0.4, color=0.25, level=-7, pan=-0.2, b=0.15),
            v("Ion Hat", "Hat", decay=0.14, tone=0.35, snap=0.3, color=0.65, sweep=0.1, filt="LP 12", cutoff=8000, level=-10, choke=1, pan=0.2),
            v("Ion Hat", "Open Hat", decay=0.45, tone=0.35, snap=0.3, color=0.65, sweep=0.15, filt="LP 12", cutoff=7000, level=-11, choke=1, pan=0.2),
            v("Solar Wind", "Vinyl", tune=0, decay=0.9, tone=0.2, snap=1.0, color=1.0, sweep=0.5, level=-20, b=0.0),
            v("Nebula Modal", "Wood", tune=3, decay=0.35, tone=0.5, snap=0.6, color=0.55, sweep=0.35, level=-11, pan=0.3, b=0.2),
            v("Orbit Tom", "Tom", tune=-3, decay=0.4, tone=0.3, snap=0.3, color=0.2, sweep=0.4, filt="LP 12", cutoff=4000, level=-7, pan=-0.3)]


def kit_synthwave():
    return [v("Core Kick", "Kick", tune=1, decay=0.4, tone=0.55, snap=0.55, color=0.3, sweep=0.45, b=0.05),
            v("Flare Snare", "Snare", tune=-1, decay=0.55, tone=0.5, snap=0.5, color=0.65, sweep=0.3, level=-2, b=0.45),
            v("Burst Clap", "Clap", decay=0.6, tone=0.5, snap=0.5, color=0.7, sweep=0.7, level=-4, b=0.4),
            v("Ion Hat", "Hat", decay=0.15, tone=0.6, snap=0.45, color=0.3, sweep=0.1, level=-10, choke=1, pan=0.2),
            v("Ion Hat", "Open Hat", decay=0.55, tone=0.55, snap=0.35, color=0.35, sweep=0.2, level=-11, choke=1, pan=0.2),
            v("Orbit Tom", "Tom Lo", tune=-6, decay=0.55, tone=0.5, snap=0.5, color=0.2, sweep=0.6, level=-4, pan=-0.3, b=0.3),
            v("Orbit Tom", "Tom Hi", tune=3, decay=0.5, tone=0.5, snap=0.5, color=0.2, sweep=0.6, level=-4, pan=0.3, b=0.3),
            v("Corona Cymbal", "Crash", decay=0.85, tone=0.6, snap=0.5, color=0.5, sweep=0.7, level=-12, b=0.2)]


def kit_industrial():
    return [v("Gravity Kick", "Kick", tune=-1, decay=0.4, tone=0.55, snap=0.7, color=0.6, sweep=0.55, drive=0.6, b=0.05),
            v("Flare Snare", "Snare", tune=-3, decay=0.5, tone=0.5, snap=0.7, color=0.8, sweep=0.2, drive=0.55, level=-3, b=0.25),
            v("Burst Clap", "Clang", tune=-5, decay=0.45, tone=0.4, snap=0.7, color=0.85, sweep=0.5, drive=0.4, filt="Comb", cutoff=220, res=0.6, level=-5, b=0.2),
            v("Ion Hat", "Hat", decay=0.16, tone=0.55, snap=0.7, color=0.6, sweep=0.3, drive=0.3, level=-9, choke=1, pan=0.2),
            v("Corona Cymbal", "Scrape", decay=0.55, tone=0.45, snap=0.6, color=0.95, sweep=0.4, level=-12, pan=-0.2),
            v("Quasar Metal", "Anvil", tune=-7, decay=0.45, tone=0.5, snap=0.8, color=0.75, sweep=0.6, drive=0.3, level=-9, pan=0.3, b=0.2),
            v("Plasma Zap", "Spark", tune=5, decay=0.2, tone=0.7, snap=0.8, color=0.8, sweep=0.6, level=-14, pan=-0.35, a=0.3),
            v("Solar Wind", "Steam", tune=-12, decay=0.55, tone=0.5, snap=0.6, color=0.9, sweep=0.2, level=-14, b=0.2)]


def kit_world():
    return [v("Nebula Modal", "Surdo", tune=-14, decay=0.55, tone=0.3, snap=0.4, color=0.1, sweep=0.2, level=0, b=0.08, lock="Snap"),
            v("Nebula Modal", "Conga", tune=-2, decay=0.38, tone=0.45, snap=0.55, color=0.05, sweep=0.25, level=-3, pan=-0.2, b=0.12),
            v("Nebula Modal", "Conga Hi", tune=4, decay=0.32, tone=0.5, snap=0.55, color=0.05, sweep=0.25, level=-4, pan=0.2, b=0.12),
            v("Solar Wind", "Shaker", tune=4, decay=0.2, tone=0.3, snap=0.6, color=0.25, sweep=0.5, level=-11, pan=0.3, lock="Snap"),
            v("Spark Rim", "Clave", tune=2, decay=0.4, tone=0.6, snap=0.5, color=0.9, sweep=0.05, level=-8, pan=-0.3),
            v("Quasar Metal", "Agogo", tune=7, decay=0.4, tone=0.6, snap=0.4, color=0.65, sweep=0.2, level=-11, pan=0.35, b=0.15),
            v("Nebula Modal", "Woodblock", tune=12, decay=0.25, tone=0.6, snap=0.85, color=0.5, sweep=0.35, level=-9, pan=-0.35),
            v("Orbit Tom", "Djembe Lo", tune=-3, decay=0.4, tone=0.6, snap=0.65, color=0.4, sweep=0.3, level=-4, b=0.1)]


# ---- Grooves ----------------------------------------------------------------------------------------------------
# Tracks in the kit order. Four on the floor and friends.
HOUSE = [
    [pattern("x...x...x...x...", "", "....x.......x...", "x.x.x.x.x.x.x.x.", "..x...x...x...x.", "", "", "..v...v...v...v."),
     pattern("x...x...x...x...", "", "....x.......x..o", "xoxoxoxoxoxoxoxo", "..x...x...x...x.", "", "...o..o....o..o.", ""),
     pattern("x...............", "", "....x.......x...", "", "..x...x...x...x.", "", "", "x.v.x.v.x.v.x.v."),
     pattern("x...x...x...x.x.", "....f...f.f.FFFF", "....n.......n...", "x.x.x.x.x.x.x.x.", "..x...x...x...x.", "..........f.f.f.", "", "")],
    [pattern("x...x...x...x...", "", "....x.......x...", "..x...x...x...x.", "", "", "x..x..x...x..x..", ""),
     pattern("x...x...x...x...", "", "....x..o....x...", "o.x.o.x.o.x.o.x.", "", "......x.......x.", "x..x..x...x..x..", ""),
     pattern("x...x...x...x...", "", "....x.......x...", "..x...x...x...x.", "..........x.....", "", "x..x..x...x..x.x", "x...v...x...v..."),
     pattern("x...x...x...xxxx", "", "....x...x...x...", "..x...x...x...x.", "", "......f.f.f.F...", "x..x..x...x..x..", "")],
    [pattern("x...x...x...x...", "....x.......x...", "", "%.x.%.x.%.x.%.x.", "..x...x...x...x.", "", "", "vvvvvvvvvvvvvvvv"),
     pattern("x...x...x...x...", "....x.......x..g", "", "x.x.x.x.x.x.x.xr", "..x...x...x...x.", "", "", "vvvvvvvvvvvvvvvv"),
     pattern("x.......x.......", "", "", "x.x.x.x.x.x.x.x.", "..x...x...x...x.", "", "", "v.v.v.v.v.v.v.v."),
     pattern("x...x...x...x...", "....x.......xFFF", "", "x.x.x.x.x.x.x.x.", "..x...x...x...x.", "", "", "vvvvvvvvvvvvvvvv")],
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
    [pattern("x...x...x...x...", "", "", "..x...x...x...x.", "", "x.?.x.?.x.?.x.?.", "", ""),
     pattern("x...x...x...x...", "", "", "..x...x...x...xr", "", {"s": "x.?.x.?.x.?.x.?.", "l": [0, 0, 0, 0, 30, 0, 0, 0, 60, 0, 0, 0, -40]}, "..x.....x.....x.", ""),
     pattern("x.......x.......", "", "", "..x...x...x...x.", "", "", "", "vvvvvvvvvvvvvvvv"),
     pattern("x...x...x...x...", "", "....f.f.f.f.FFFF", "..x...x...x...x.", "", "x.?.x.?.x.?.x.?.", "", "")],
]

HIPHOP = [
    [pattern("x.....x...x.....", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "", "", ""),
     pattern("x.....x...x..x..", "....x..o....x...", "", "x.x.x.x.x.x.x.xo", "", "", "..........x.....", ""),
     pattern("x.........x.....", "....x.......x...", "", "x...x...x...x...", "", "", "", ""),
     pattern("x.....x...x.....", "....x.......x.FF", "", "x.x.x.x.x.x.x.x.", "..............f.", "", "", "")],
    [pattern("x..x......x.....", "....x.......x...", "....x.......x...", "x.xox.xox.xox.xo", "", "", "", "......x......x.."),
     pattern("x..x......xx....", "....x.......x..o", "....x.......x...", "x.xox.xox.xox.xo", "", "", "", "......x......x.."),
     pattern("x..x..........x.", "............x...", "", "x.x.x.x.x.x.x.x.", "", "", "", ""),
     pattern("x..x......x.....", "....x.......xFFF", "....x.......x...", "x.xox.xox.xox.xo", "", "", "", "")],
    [pattern("x......xx.......", "....x.......x...", "", {"s": "x.x.x.x.x.x.x.x.", "n": [0, 0, 8, 0, 0, 0, 8, 0, 0, 0, 8, 0, 0, 0, 8]}, "", "", "........x..x....", ""),
     pattern("x......xx..x....", "....x..g....x...", "", {"s": "x.x.x.x.x.x.x.xo", "n": [0, 0, 8, 0, 0, 0, 8, 0, 0, 0, 8, 0, 0, 0, 8]}, "", "", "........x..x....", ""),
     pattern("x...............", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "", "", ""),
     pattern("x......xx.......", "....x.......x.F.", "", "x.x.x.x.x.x.x.x.", "", "", "..........f.f.f.", "")],
]

TRAP = [
    [pattern({"s": "x.........x.....x.......x.....x.", "len": 32, "p": [0] * 10 + [0] + [0] * 5 + [-2]},
             {"s": "........x...............x.......", "len": 32}, "", {"s": "x.x.x.x.x.xRx.x.x.x.x.x.x.T.x.x.", "len": 32}, "", "", "", ""),
     pattern({"s": "x.........x...x.x.......x.....x.", "len": 32, "p": [0] * 14 + [5, 0, 3]},
             {"s": "........x...............x.......", "len": 32}, "", {"s": "x.x.xRx.x.x.x.x.T.x.x.x.xRx.x.x.", "len": 32}, {"s": "..............x.................", "len": 32}, "", "", ""),
     pattern({"s": "x...............................", "len": 32}, {"s": "........x...............x.......", "len": 32}, "", {"s": "x...x...x...x...x...x...x...x...", "len": 32}, "", "", "", {"s": "x...............................", "len": 32}),
     pattern({"s": "x.........x.....x.......x.....x.", "len": 32}, {"s": "........x...............x..FfFFF", "len": 32}, "", {"s": "x.x.x.x.TTTTx.x.RRRRx.x.TTTTTTTT", "len": 32}, "", {"s": "..........................f.f.f.", "len": 32}, "", "")],
    [pattern({"s": "x.......x.x.............x.......", "len": 32, "p": [0] * 8 + [0, 0, 7]},
             {"s": "........x...............x.......", "len": 32}, {"s": "........x...............x.......", "len": 32},
             {"s": "x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.x.", "len": 32, "rate": "1/16"}, "", "", {"s": "...x.....x......", "len": 16}, ""),
     pattern({"s": "x.......x.x.......x.....x.....x.", "len": 32, "p": [0] * 8 + [0, 0, 7] + [0] * 7 + [-5]},
             {"s": "........x...............x...o...", "len": 32}, {"s": "........x...............x.......", "len": 32},
             {"s": "x.x.x.x.xRx.x.x.x.x.x.x.xTx.x.x.", "len": 32}, "", "", {"s": "...x.....x......", "len": 16}, ""),
     pattern({"s": "x.......x.x.............x.......", "len": 32}, {"s": "........x...............x.......", "len": 32}, "",
             {"s": "x..x..x..x..x..x", "rate": "1/16T", "len": 12}, "", "", "", ""),
     pattern({"s": "x.......x.x.............x.x.x.x.", "len": 32}, {"s": "........x...............x...FFFF", "len": 32},
             {"s": "........x...............x.......", "len": 32}, {"s": "x.x.x.x.x.x.x.x.TTTTTTTTRRRRRRRR", "len": 32}, "", "", "", "")],
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
]

BREAKS = [
    [pattern("x.........x.....", "....x.......x...", "......o.o....o..", "x.x.x.x.x.x.x.x.", "..............x.", "", "", ""),
     pattern("x.x.......x.....", "....x.......x..o", ".o....o.o....o..", "x.x.x.x.x.x.xox.", "......x.........", "", "", ""),
     pattern("x.........x.x...", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "x...............", "", ""),
     pattern("x.........x.....", "....x.....F.FFFF", "......o.o.......", "x.x.x.x.x.x.x.x.", "", "", "..........f..f.f", "")],
    [pattern("x..x......x.....", "....x..o.o..x...", "", "x.x.x.x.x.x.x.x.", "", "", "", "..x.......x....."),
     pattern("x..x......x..x..", "....x..o.o..x.o.", "", "x.x.x.x.x.x.x.x.", "......x.........", "", "", "..x.......x....."),
     pattern("x..x............", "....x.......x...", "", "x...x...x...x...", "", "x...............", "", ""),
     pattern("x..x......x.....", "....x..o.o..xFFF", "", "x.x.x.x.x.x.x.x.", "", "", "..........f.f.f.", "")],
]

GARAGE = [
    [pattern("x.......x.x.....", "....x.......x...", "", "..x...x...x...x.", "", "...x.....x......", "x.x.x.x.x.x.x.x.", ""),
     pattern("x.......x.x...x.", "....x.......x...", "....x.......x...", "..x..xx...x..xx.", "", "...x.....x..x...", "x.x.x.x.x.x.x.x.", ".......x.......x"),
     pattern("x.........x.....", "....x.......x...", "", "..x...x...x...x.", "", "", "x.x.x.x.x.x.x.x.", ""),
     pattern("x.......x.x.....", "....x...f.f.FFFF", "", "..x...x...x...x.", "", "...x.....x......", "x.x.x.x.x.x.x.x.", "")],
    [pattern("x...x...x...x...", "....x.......x...", "", "..x...x...x...x.", "", "x..x..x...x..x..", "xoxoxoxoxoxoxoxo", ""),
     pattern("x...x...x...x...", "....x..o....x...", "", "..x..x..x..x..x.", "", "x..x..x...x..x..", "xoxoxoxoxoxoxoxo", ""),
     pattern("x.......x.......", "....x.......x...", "", "..x...x...x...x.", "", "", "", "x..x..x...x..x.."),
     pattern("x...x...x...x...", "....x...x...FFFF", "", "..x...x...x...x.", "", "x..x..x...x..x..", "xoxoxoxoxoxoxoxo", "")],
]

LOFI = [
    [pattern("x.......x.x.....", "....x.......x...", "", {"s": "x.x.x.x.x.x.x.x.", "n": [0, 0, 12, 0, 0, 0, 12, 0, 0, 0, 12, 0, 0, 0, 12]}, "", "x...............", "", ""),
     pattern("x.......x.x...x.", "....x..g....x...", "", {"s": "x.x.x.x.x.x.x.xo", "n": [0, 0, 12, 0, 0, 0, 12, 0, 0, 0, 12, 0, 0, 0, 12]}, "", "x...............", "......x.......x.", ""),
     pattern("x.........x.....", "....x.......x...", "....x.......x...", "x...x...x...x...", "", "x...............", "", ""),
     pattern("x.......x.x.....", "....x.......x.ff", "", "x.x.x.x.x.x.x.x.", "", "x...............", "", "..........f.f.f.")],
    [pattern("x.....x...x.....", "....x.......x...", "..x.......x.....", "x.x.x.x.x.x.x.x.", "", "x...............", "", ""),
     pattern("x.....x...xx....", "....x.......x..g", "..x.......x..x..", "x.x.x.x.x.x.x.x.", "......x.........", "x...............", "", ""),
     pattern("x.........x.....", "............x...", "..x.......x.....", "x...x...x...x...", "", "x...............", "", ""),
     pattern("x.....x...x.....", "....x.......xfff", "..x.......x.....", "x.x.x.x.x.x.x.x.", "", "x...............", "", "")],
]

SYNTHWAVE = [
    [pattern("x...x...x...x...", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "", "", ""),
     pattern("x...x...x...x...", "....x.......x...", "....x.......x...", "x.x.x.x.x.x.x.x.", "..x...x...x...x.", "", "", ""),
     pattern("x.......x.......", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "", "", "x..............."),
     pattern("x...x...x...x...", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "........x.x.x.x.", "........f.f.f.f.", "x...............")],
    [pattern("x.......x.x.....", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "", "", ""),
     pattern("x.......x.x.....", "....x.......x...", "....x.......x...", "xxxxxxxxxxxxxxxx", "", "", "", ""),
     pattern("x...............", "............x...", "", "x.x.x.x.x.x.x.x.", "", "", "", "x..............."),
     pattern("x.......x.x.....", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "....x.x.x.......", "..........x.x.x.", "")],
]

INDUSTRIAL = [
    [pattern("x...x...x...x...", "....x.......x...", "..x.......x..x..", "xxxxxxxxxxxxxxxx", "", "x..x..x...x..x..", "", ""),
     pattern("x...x...x.x.x...", "....x.......x...", "..x...x...x..x..", "xxxxxxxxxxxxxxxx", "....x.......x...", "x..x..x...x..x..", "......x.......x.", ""),
     pattern("x...x...x...x...", "", "", "", "", "x.x.x.x.x.x.x.x.", "", "x..............."),
     pattern("x...x...x...xxxx", "....x...F.F.FFFF", "..x.......x..x..", "xxxxxxxxxxxxxxxx", "", "x..x..x...x..x..", "", "")],
    [pattern("x..x..x...x..x..", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "....x.......x...", {"s": "x.x.x.x.", "rate": "1/32"}, ""),
     pattern("x..x..x...x..x.x", "....x.......x...", "......x.......x.", "x.x.x.x.x.x.x.x.", "", "....x.......x...", {"s": "x.x.x.x.", "rate": "1/32"}, "x..............."),
     pattern("x...............", "....x.......x...", "", "", "", "", "", "x.......x......."),
     pattern("x..x..x...x..x..", "....x...F.F.FFFF", "", "x.x.x.x.x.x.x.x.", "", "....x.......x...", "", "")],
]

WORLD = [
    [pattern("x.......x.......", "..x..x....x..x..", "....x.x.....x.x.", "xoxoxoxoxoxoxoxo", "x..x..x...x.x...", "", "......x.......x.", ""),
     pattern("x.......x...x...", "..x..x.o..x..x..", "....x.x..o..x.x.", "xoxoxoxoxoxoxoxo", "x..x..x...x.x...", "...x.......x....", "......x.......x.", ""),
     pattern("x...............", "", "", "xoxoxoxoxoxoxoxo", "x..x..x...x.x...", "", "", "x.......x......."),
     pattern("x.......x.......", "..x..x....xFxFxF", "....x.x.....xxxx", "xoxoxoxoxoxoxoxo", "x..x..x...x.x...", "", "......x.......x.", "")],
    [pattern({"s": "x.....x.....", "len": 12, "rate": "1/8T"}, {"s": "..x..x..x..x", "len": 12, "rate": "1/8T"}, {"s": "....x.....x.", "len": 12, "rate": "1/8T"},
             {"s": "xoxoxoxoxoxo", "len": 12, "rate": "1/8T"}, {"s": "x.x.xx.x.x.x", "len": 12, "rate": "1/8T"}, {"s": "x.x.xx.x.x.x", "len": 12, "rate": "1/8T"}, "", ""),
     pattern({"s": "x.....x..x..", "len": 12, "rate": "1/8T"}, {"s": "..x..x..x.ox", "len": 12, "rate": "1/8T"}, {"s": "....x.....x.", "len": 12, "rate": "1/8T"},
             {"s": "xoxoxoxoxoxo", "len": 12, "rate": "1/8T"}, {"s": "x.x.xx.x.x.x", "len": 12, "rate": "1/8T"}, {"s": "x.x.xx.x.x.x", "len": 12, "rate": "1/8T"}, {"s": "...x.....x..", "len": 12, "rate": "1/8T"}, ""),
     pattern({"s": "x...........", "len": 12, "rate": "1/8T"}, "", "", {"s": "xoxoxoxoxoxo", "len": 12, "rate": "1/8T"}, {"s": "x.x.xx.x.x.x", "len": 12, "rate": "1/8T"}, "", "", ""),
     pattern({"s": "x.....x.....", "len": 12, "rate": "1/8T"}, {"s": "..x..x..xFFF", "len": 12, "rate": "1/8T"}, {"s": "....x...xxxx", "len": 12, "rate": "1/8T"},
             {"s": "xoxoxoxoxoxo", "len": 12, "rate": "1/8T"}, {"s": "x.x.xx.x.x.x", "len": 12, "rate": "1/8T"}, "", "", "")],
]

ELECTRO = [
    [pattern("x.....x...x.....", "....x.......x...", "", "x.x.x.x.x.x.x.x.", "", "", "", "..x..x....x....."),
     pattern("x.....x...x..x..", "....x.......x...", "....x.......x...", "x.x.x.x.x.x.x.x.", "", "", "......x.......x.", "..x..x....x....."),
     pattern("x.....x.........", "", "", "x.x.x.x.x.x.x.x.", "", "", "", "x.....x...x....."),
     pattern("x.....x...x.....", "....x.....F.FFFF", "", "x.x.x.x.x.x.x.x.", "", "", "", "..x..x....x.....")],
    [pattern("x..x....x.......", "....x.......x...", "", "xxxxxxxxxxxxxxxx", "", "", "", "....x.......x..."),
     pattern("x..x....x..x....", "....x.......x..o", "", "xxxxxxxxxxxxxxxx", "..............x.", "", "", "....x.......x..."),
     pattern("x..x............", "............x...", "", "x.x.x.x.x.x.x.x.", "", "", "", ""),
     pattern("x..x....x.......", "....x...F.F.FFFF", "", "xxxxxxxxxxxxxxxx", "", "", "", "....x.......x...")],
]

# ---- The presets ------------------------------------------------------------------------------------------------
PRESETS = []


def add(name, cat, desc, tags, kit, groove, tempo, swing=0.5, mst=None, macros=(0, 0, 0, 0), patterns_extra=None):
    PRESETS.append(dict(name=name, category=cat, description=desc, tags=tags, kit=kit, groove=groove, tempo=tempo,
                        swing=swing, master=mst or master(), macros=macros))


# House
add("Photon Floor", "House", "Classic four-on-the-floor: round kick, off-beat open hat, a clap on two and four.", ["house", "classic", "dance"],
    kit_house(), HOUSE[0], 124, 0.54, master(glue=0.5, thr=-14, ratio=2.5, rdecay=1.2, size=0.9, rret=0.6))
add("Coronal House", "House", "Deep house with a soft rim figure, a long ride and a low-passed warmth.", ["house", "deep", "warm"],
    tweak(kit_house(), t1=dict(decay=0.48, tone=0.4, color=0.2), t3=dict(b=0.3, sweep=0.6), t8=dict(level=-10)),
    HOUSE[1], 121, 0.56, master(glue=0.4, high=-3, low=1.5, rdecay=2.2, rret=0.7, ediv="1/4.", fb=0.3, eret=0.4))
add("Garage Sunrise", "House", "Shuffled late-night house; the shaker-ride rolls on every step.", ["house", "shuffle"],
    tweak(kit_house(), t8=dict(model="Solar Wind", name="Shaker", tune=4, decay=0.15, tone=0.3, snap=0.7, color=0.25, sweep=0.5, level=-12, b=0.05)),
    HOUSE[2], 122, 0.6, master(glue=0.3, rdecay=1.4))
add("Disco Ion", "House", "Disco-flavoured: open hats on the off-beat, a bright clap, a cowbell answer.", ["house", "disco", "bright"],
    tweak(kit_house(), t5=dict(decay=0.55, tone=0.7, level=-6), t7=dict(model="Quasar Metal", name="Cowbell", decay=0.3, color=0.05, tone=0.55, level=-11)),
    HOUSE[0], 118, 0.52, master(glue=0.5, high=2, rdecay=1.0, rret=0.5))
add("Warehouse Hydrogen", "House", "Raw jacking house: driven kick, gritty clap, rim chatter.", ["house", "raw", "jacking"],
    tweak(kit_house(), t1=dict(drive=0.35, color=0.5, snap=0.6), t3=dict(drive=0.4, color=0.7), t7=dict(level=-6)),
    HOUSE[1], 126, 0.55, master(drive=0.2, dmode="Tube", glue=0.6, thr=-16, ratio=3, rdecay=0.9))
add("Lunar Lounge", "House", "Mellow house: tuned-down kick, brushed shaker, a wide ride and plenty of air.", ["house", "mellow", "lounge"],
    tweak(kit_house(), t1=dict(tune=-1, decay=0.5, tone=0.35), t4=dict(tone=0.4, color=0.5), t8=dict(level=-10, b=0.2)),
    HOUSE[2], 116, 0.58, master(glue=0.3, high=-2.5, rdecay=2.8, size=1.4, rret=0.75), macros=(0, 0.2, 0.1, 0.25))
add("Solar Piano Stomp", "House", "Piano-house energy: punchy kick, double clap, a tom pickup into the fill.", ["house", "big room"],
    tweak(kit_house(), t3=dict(color=0.8, level=-1), t6=dict(level=-3)),
    HOUSE[0], 126, 0.5, master(glue=0.6, ratio=3, makeup=1, rdecay=1.5, rret=0.6, low=1))
add("Afterglow Rim", "House", "Minimal house built on rimshots and a ticking hat; the kick sits deep.", ["house", "minimal"],
    tweak(kit_house(), t1=dict(decay=0.36, tone=0.45), t7=dict(level=-5, a=0.2), t2=dict(level=-7)),
    HOUSE[1], 123, 0.57, master(glue=0.3, ediv="1/8.", fb=0.4, eret=0.6, rdecay=1.3))
add("Plasma Jack", "House", "Chicago jack: open hat sixteenths, a heavy clap, the cowbell on the and.", ["house", "chicago", "jack"],
    tweak(kit_house(), t8=dict(model="Quasar Metal", name="Cowbell", decay=0.3, color=0.0, tone=0.5, level=-11), t3=dict(drive=0.3)),
    HOUSE[2], 125, 0.53, master(drive=0.15, glue=0.5, rdecay=0.8, rret=0.5))

# Techno
add("Event Horizon", "Techno", "Peak-time techno: rumbling FM kick, a 3-step perc loop drifting against the bar.", ["techno", "peak", "polymeter"],
    kit_techno(), TECHNO[1], 132, 0.5, master(glue=0.6, thr=-15, ratio=3, rdecay=3.5, size=1.6, rret=0.55, low=1))
add("Neutron Pulse", "Techno", "Hypnotic: a rolling 1/32 perc, an open hat on the off-beat, locked tone sweeps.", ["techno", "hypnotic"],
    kit_techno(), TECHNO[0], 130, 0.5, master(glue=0.5, rdecay=2.5, ediv="1/8.", fb=0.45, eret=0.35))
add("Dark Matter Groove", "Techno", "Dark and sparse: chance hits on the perc, a deep kick and long reverb throws.", ["techno", "dark", "chance"],
    tweak(kit_techno(), t1=dict(tune=-1, decay=0.45, color=0.45), t6=dict(model="Nebula Modal", name="Metal Perc", tune=7, decay=0.35, color=0.85, snap=0.7)),
    TECHNO[2], 128, 0.5, master(glue=0.5, rdecay=5, size=2, rret=0.65, high=-1), macros=(0, 0.2, 0, 0.3))
add("Tachyon Rumble", "Techno", "Rumble techno: the reverb fed by echo, the kick saturated, hats pushed.", ["techno", "rumble"],
    tweak(kit_techno(), t1=dict(drive=0.5, b=0.25, decay=0.4)),
    TECHNO[0], 134, 0.5, master(drive=0.25, dmode="Tube", glue=0.6, e2r=0.6, ediv="1/4", fb=0.4, eret=0.25, rdecay=4, size=1.8, rret=0.4, low=2))
add("Industrial Orbit", "Techno", "Hard techno: distorted clap, metallic perc, 16th hats.", ["techno", "hard"],
    tweak(kit_techno(), t3=dict(drive=0.5, color=0.85), t6=dict(color=0.7, tone=0.7)),
    TECHNO[1], 140, 0.5, master(drive=0.35, dmode="Tube", glue=0.7, thr=-18, ratio=4, rdecay=1.8))
add("Acid Wavelength", "Techno", "Acid-era: Pulsar FM lead perc with ratio locks, tight hats, ride wash.", ["techno", "acid"],
    tweak(kit_techno(), t6=dict(tune=0, decay=0.35, tone=0.75, color=0.55, lock="Color", level=-7)),
    TECHNO[2], 131, 0.52, master(glue=0.5, ediv="1/16.", fb=0.5, eret=0.4, rdecay=1.8))
add("Cold Fusion", "Techno", "Minimal techno: everything short and dry, the rim carrying the groove.", ["techno", "minimal", "dry"],
    tweak(kit_techno(), t1=dict(decay=0.28), t7=dict(level=-5), t8=dict(level=-17)),
    TECHNO[0], 127, 0.53, master(glue=0.3, rdecay=0.9, rret=0.4))
add("Deep Space Dub", "Techno", "Dub techno: chords of echo on the rim and perc, long dark reverb.", ["techno", "dub", "space"],
    tweak(kit_techno(), t6=dict(a=0.6, b=0.4), t7=dict(a=0.6, b=0.4), t1=dict(tone=0.3)),
    TECHNO[0], 122, 0.5, master(glue=0.4, ediv="1/4.", fb=0.62, edamp=0.7, eret=0.55, e2r=0.5, rdecay=6, size=2.2, rret=0.6, high=-3), macros=(0, 0, 0, 0.35))
add("Pulsar Array", "Techno", "Polyrhythmic techno: 12- and 11-step loops spinning over a straight kick.", ["techno", "polymeter"],
    tweak(kit_techno(), t7=dict(model="Quasar Metal", name="Bell", tune=12, decay=0.3, color=0.8, level=-12)),
    TECHNO[1], 129, 0.5, master(glue=0.5, rdecay=3, ediv="1/8.", fb=0.35, eret=0.4))

# Electro
add("Kraftwerk Satellite", "Electro", "Classic electro: syncopated 808 kick, a clap backbeat and zaps.", ["electro", "808", "classic"],
    tweak(kit_808(), t8=dict(model="Plasma Zap", name="Zap", tune=10, decay=0.25, tone=0.55, snap=0.6, color=0.3, sweep=0.55, level=-12)),
    ELECTRO[0], 128, 0.5, master(glue=0.4, rdecay=1.4))
add("Robot Rock Signal", "Electro", "Machine funk: 16th hats, a busy kick and a ringing cowbell.", ["electro", "funk"],
    tweak(kit_808(), t8=dict(decay=0.4, level=-10)),
    ELECTRO[1], 122, 0.54, master(glue=0.5, rdecay=1.2))
add("Detroit Particle", "Electro", "Detroit electro: FM toms, dry kit, echoing zaps.", ["electro", "detroit"],
    tweak(kit_808(), t6=dict(model="Pulsar FM", name="FM Tom", tune=-5, decay=0.35, tone=0.4, color=0.1), t8=dict(model="Plasma Zap", name="Zap", tune=7, decay=0.3, sweep=0.6, a=0.4, level=-12)),
    ELECTRO[0], 130, 0.5, master(glue=0.4, ediv="1/8.", fb=0.45, eret=0.5, rdecay=1.5))
add("Breakdance Beacon", "Electro", "B-boy electro: snappy snare, booming kick tail, rim fills.", ["electro", "b-boy"],
    tweak(kit_808(), t1=dict(decay=0.62), t2=dict(snap=0.7, level=-2)),
    ELECTRO[1], 112, 0.55, master(glue=0.5, rdecay=1.2))
add("Freestyle Flux", "Electro", "Miami freestyle: fast 808, handclaps, bright hats.", ["electro", "miami", "freestyle"],
    tweak(kit_808(), t1=dict(decay=0.68, tune=-1), t3=dict(level=-2)),
    ELECTRO[0], 118, 0.5, master(glue=0.5, high=1.5, rdecay=1.1))
add("Bleep Constellation", "Electro", "Bleep electro: FM bleeps and metallic bells, short drums.", ["electro", "bleep"],
    tweak(kit_808(), t6=dict(model="Pulsar FM", name="Bleep", tune=12, decay=0.15, tone=0.3, color=0.0, sweep=0.0, level=-12),
          t8=dict(model="Quasar Metal", name="Bell", tune=12, decay=0.35, color=0.9, level=-13, a=0.3)),
    ELECTRO[1], 126, 0.5, master(glue=0.4, ediv="1/16.", fb=0.4, eret=0.5, rdecay=1.8))
add("Orbital Funk", "Electro", "Swung electro funk with a lazy hat and a tom answer.", ["electro", "swing"],
    kit_808(), ELECTRO[0], 108, 0.6, master(glue=0.4, rdecay=1.3))
add("Cosmic Ray Electro", "Electro", "Bit-crushed electro through the Cosmic Ray drive.", ["electro", "lo-fi", "crushed"],
    kit_808(), ELECTRO[1], 124, 0.5, master(drive=0.45, dmode="Cosmic Ray", dmix=0.6, glue=0.4, rdecay=1.0))
add("Satellite Sequence", "Electro", "Sequenced electro with Spark rims and a Nebula conga line.", ["electro", "percussive"],
    tweak(kit_808(), t7=dict(model="Nebula Modal", name="Conga", tune=2, decay=0.35, color=0.05, snap=0.6, level=-6)),
    ELECTRO[0], 127, 0.52, master(glue=0.4, rdecay=1.4))

# Hip-Hop
add("Boom Bap Sun", "Hip-Hop", "Boom bap: dusty kick, cracking snare, swung hats.", ["hip-hop", "boom bap"],
    tweak(kit_808(), t1=dict(name="Kick", decay=0.42, tone=0.45, color=0.35, drive=0.2), t2=dict(drive=0.25, decay=0.45)),
    HIPHOP[0], 90, 0.58, master(glue=0.5, drive=0.15, dmode="Tube", high=-1.5, rdecay=1.0, rret=0.5))
add("Golden Era Flare", "Hip-Hop", "Golden-era: a kick that skips, ghost notes on the snare, an open hat lift.", ["hip-hop", "golden era"],
    tweak(kit_808(), t1=dict(name="Kick", decay=0.4, tone=0.5, color=0.4), t2=dict(color=0.65, decay=0.42)),
    HIPHOP[0], 94, 0.6, master(glue=0.5, rdecay=0.9))
add("West Coast Orbit", "Hip-Hop", "G-funk bounce: claps doubled with snare, tight hats, a long 808.", ["hip-hop", "west coast", "g-funk"],
    kit_808(), HIPHOP[1], 92, 0.55, master(glue=0.5, rdecay=1.2, low=1))
add("Dilla Drift", "Hip-Hop", "Drunk-timed hats nudged late, a loose kick and a soft snare.", ["hip-hop", "swing", "nudge"],
    tweak(kit_808(), t1=dict(name="Kick", decay=0.4, tone=0.35, color=0.3), t2=dict(tone=0.45, level=-4)),
    HIPHOP[2], 88, 0.62, master(glue=0.4, high=-2, rdecay=1.0))
add("Crate Digger", "Hip-Hop", "Sample-style kit: low-passed hats, crunchy snare, rim fills.", ["hip-hop", "dusty"],
    tweak(kit_lofi(), t6=dict(level=-30)),
    HIPHOP[0], 86, 0.57, master(drive=0.3, dmode="Tube", glue=0.5, high=-3, rdecay=0.8))
add("Cypher Nebula", "Hip-Hop", "Rap cypher: hard kick, clap stack, conga pickup.", ["hip-hop", "hard"],
    tweak(kit_808(), t1=dict(name="Kick", decay=0.45, color=0.55, drive=0.3), t7=dict(model="Nebula Modal", name="Conga", tune=3, decay=0.35, color=0.05, snap=0.6, level=-7)),
    HIPHOP[1], 96, 0.53, master(glue=0.6, ratio=3, rdecay=1.0))
add("Mellow Eclipse", "Hip-Hop", "Jazzy hip-hop: brushed snare, ride, soft kick.", ["hip-hop", "jazzy"],
    tweak(kit_808(), t1=dict(name="Kick", decay=0.38, tone=0.3, snap=0.3), t2=dict(color=0.85, tone=0.35, level=-5),
          t8=dict(model="Corona Cymbal", name="Ride", decay=0.6, tone=0.5, color=0.4, sweep=0.25, level=-13)),
    HIPHOP[2], 84, 0.6, master(glue=0.3, high=-2, rdecay=1.6, rret=0.6))
add("Bounce Particle", "Hip-Hop", "Southern bounce: chopped cowbell, claps and rolling hats.", ["hip-hop", "bounce"],
    kit_808(), HIPHOP[1], 98, 0.5, master(glue=0.5, rdecay=1.0))
add("Midnight Snap", "Hip-Hop", "Snap music: finger-snap claps and an 808 that slides.", ["hip-hop", "snap"],
    tweak(kit_808(), t2=dict(level=-30), t3=dict(name="Snap", tune=7, decay=0.25, color=0.0, snap=0.8, level=-1)),
    HIPHOP[1], 80, 0.5, master(glue=0.4, rdecay=1.2, rret=0.7))

# Trap
add("Supernova Trap", "Trap", "Modern trap: long gliding 808, hat rolls in triplets and fours, a late snare.", ["trap", "808", "rolls"],
    kit_trap(), TRAP[0], 140, 0.5, master(glue=0.5, rdecay=1.3, low=1.5))
add("Dark Star Drill", "Trap", "Drill: sliding 808 pitches, triplet hats, snares on three.", ["trap", "drill"],
    tweak(kit_trap(), t4=dict(decay=0.12, tone=0.6)), TRAP[1], 144, 0.5, master(glue=0.5, rdecay=1.5))
add("Ion Rolls", "Trap", "Hat-roll showcase: ratchets of three and four over a quiet kit.", ["trap", "hats"],
    tweak(kit_trap(), t4=dict(level=-6)), TRAP[0], 150, 0.5, master(glue=0.4, rdecay=1.0))
add("Plasma Phonk", "Trap", "Phonk: cowbell melody, crushed drums, a distorted 808.", ["trap", "phonk"],
    tweak(kit_trap(), t1=dict(drive=0.6, color=0.6), t7=dict(model="Quasar Metal", name="Cowbell", tune=0, decay=0.35, color=0.0, level=-7)),
    TRAP[1], 132, 0.5, master(drive=0.4, dmode="Cosmic Ray", dmix=0.4, glue=0.6, rdecay=1.0))
add("Rage Corona", "Trap", "Rage: saturated everything, open hats, a huge reverb clap.", ["trap", "rage"],
    tweak(kit_trap(), t3=dict(b=0.4, level=-3), t1=dict(drive=0.5)), TRAP[0], 150, 0.5,
    master(drive=0.3, dmode="Tube", glue=0.6, ratio=3, rdecay=2.5, size=1.6, rret=0.6))
add("Gravity Well 808", "Trap", "Slow, heavy trap with a sub that fills the room.", ["trap", "slow", "808"],
    tweak(kit_trap(), t1=dict(decay=0.92, tune=-7)), TRAP[1], 130, 0.5, master(glue=0.4, low=2, rdecay=1.5), macros=(0, 0.25, 0, 0))
add("Cloud Orbit", "Trap", "Cloud rap: soft hats, washed clap, a gentle 808.", ["trap", "cloud", "ambient"],
    tweak(kit_trap(), t1=dict(drive=0.0, color=0.15), t3=dict(b=0.55, level=-5), t4=dict(tone=0.45, level=-10)),
    TRAP[0], 136, 0.5, master(glue=0.3, rdecay=4, size=2, rret=0.7, high=-2), macros=(0, 0, 0, 0.3))
add("Zero-G Bounce", "Trap", "Bouncy trap with a zap, a perc and claps on two and four.", ["trap", "bounce"],
    tweak(kit_trap(), t6=dict(level=-10)), TRAP[1], 146, 0.5, master(glue=0.5, rdecay=1.2))
add("Hypergiant", "Trap", "Stadium trap: everything big, crash risers, glue on the bus.", ["trap", "big"],
    tweak(kit_trap(), t8=dict(level=-12)), TRAP[0], 142, 0.5, master(glue=0.7, thr=-16, ratio=3, makeup=1.5, rdecay=2.0, rret=0.6))

# Drum & Bass
add("Amen Corona", "Drum & Bass", "Two-step DnB: kick on one and the and of three, snares on two and four, ghost rolls.", ["dnb", "two-step"],
    kit_dnb(), DNB[0], 174, 0.5, master(glue=0.6, ratio=3, rdecay=1.4, rret=0.5))
add("Liquid Plasma", "Drum & Bass", "Liquid DnB: soft ride, shaker, a smooth snare.", ["dnb", "liquid"],
    tweak(kit_dnb(), t2=dict(tone=0.5, decay=0.45, b=0.3), t1=dict(tone=0.5)), DNB[0], 172, 0.5,
    master(glue=0.4, rdecay=2.5, rret=0.6, high=-1))
add("Neurofunk Reactor", "Drum & Bass", "Neuro: heavy driven drums, metallic ghosts, tight glue.", ["dnb", "neuro"],
    tweak(kit_dnb(), t1=dict(drive=0.4, color=0.6), t2=dict(drive=0.45, color=0.65), t3=dict(model="Quasar Metal", name="Metal", tune=-5, color=0.7, decay=0.25)),
    DNB[1], 174, 0.5, master(drive=0.3, dmode="Tube", glue=0.7, thr=-18, ratio=4, rdecay=1.0))
add("Jungle Nebula", "Drum & Bass", "Jungle: busy ghost snares, a rolling ride, swung shaker.", ["dnb", "jungle"],
    tweak(kit_dnb(), t3=dict(level=-6)), DNB[1], 168, 0.55, master(glue=0.5, rdecay=1.5))
add("Halftime Horizon", "Drum & Bass", "Halftime at 170: kick and snare at half speed, hats at full.", ["dnb", "halftime"],
    kit_dnb(), DNB[1], 170, 0.5, master(glue=0.5, rdecay=2.0, size=1.4))
add("Atmos Drift", "Drum & Bass", "Atmospheric DnB with a big reverb and soft hats.", ["dnb", "atmospheric"],
    tweak(kit_dnb(), t2=dict(b=0.35), t4=dict(tone=0.5, level=-11)), DNB[0], 172, 0.5,
    master(glue=0.4, rdecay=4.5, size=2.0, rret=0.6, high=-1.5), macros=(0, 0, 0, 0.25))
add("Rollers Pulse", "Drum & Bass", "Rollers: minimal two-step with a bouncing rim.", ["dnb", "rollers"],
    tweak(kit_dnb(), t7=dict(level=-7)), DNB[0], 175, 0.5, master(glue=0.6, rdecay=1.2))
add("Darkside Shuffle", "Drum & Bass", "Dark shuffled DnB: lower snares, longer tails.", ["dnb", "dark"],
    tweak(kit_dnb(), t2=dict(tune=-2, decay=0.5), t1=dict(tune=1)), DNB[1], 172, 0.56, master(glue=0.5, rdecay=2.0, high=-2))
add("Footwork Photon", "Drum & Bass", "Footwork-tinged 160: triplet-ish kicks and snapping claps.", ["footwork", "160"],
    tweak(kit_dnb(), t3=dict(model="Burst Clap", name="Clap", decay=0.3, color=0.4, level=-5)), DNB[1], 160, 0.5, master(glue=0.5, rdecay=1.0))

# Breakbeat
add("Funky Drummer Star", "Breakbeat", "A breakbeat in the old style: ghost snares, open hat lift, saturated kit.", ["breaks", "funk"],
    kit_breaks(), BREAKS[0], 100, 0.55, master(drive=0.2, dmode="Tube", glue=0.5, rdecay=1.0, rret=0.5))
add("Big Beat Flare", "Breakbeat", "Big beat: fat driven break, crash on the one.", ["breaks", "big beat"],
    tweak(kit_breaks(), t1=dict(drive=0.5), t2=dict(drive=0.5)), BREAKS[1], 110, 0.52,
    master(drive=0.35, dmode="Tube", glue=0.7, thr=-18, ratio=4, makeup=1, rdecay=1.4))
add("Nu Skool Orbit", "Breakbeat", "Nu skool breaks: tight, punchy, a rim pattern.", ["breaks", "nu skool"],
    tweak(kit_breaks(), t1=dict(drive=0.15), t2=dict(drive=0.1)), BREAKS[0], 132, 0.5, master(glue=0.6, rdecay=1.2))
add("Trip Hop Nebula", "Breakbeat", "Trip hop: slow, heavy, filtered and wet.", ["breaks", "trip hop"],
    tweak(kit_breaks(), t1=dict(decay=0.5, tone=0.35), t2=dict(decay=0.5, tone=0.4, b=0.35), t4=dict(filt="LP 12", cutoff=6000)),
    BREAKS[1], 84, 0.56, master(drive=0.25, dmode="Tube", glue=0.4, high=-3, rdecay=3.0, rret=0.6), macros=(0, 0.2, 0, 0.2))
add("Florida Breaks", "Breakbeat", "Florida breaks: bass-heavy kick and an 808 cowbell.", ["breaks", "florida"],
    tweak(kit_breaks(), t1=dict(model="Core Kick", decay=0.6, color=0.2, drive=0.1), t8=dict(model="Quasar Metal", name="Cowbell", color=0.0, decay=0.3, level=-11)),
    BREAKS[0], 130, 0.5, master(glue=0.5, rdecay=1.0, low=1.5))
add("Rave Signal", "Breakbeat", "Early rave: a sped-up break with a stab-like zap.", ["breaks", "rave"],
    tweak(kit_breaks(), t8=dict(model="Plasma Zap", name="Zap", tune=10, decay=0.25, sweep=0.5, level=-11)), BREAKS[1], 138, 0.5,
    master(drive=0.2, glue=0.6, rdecay=1.5))
add("Broken Corona", "Breakbeat", "Broken beat: lazy hats, displaced kicks, warm and wide.", ["breaks", "broken beat"],
    kit_breaks(), BREAKS[1], 116, 0.58, master(glue=0.4, rdecay=1.6, width=1.2))
add("Lofi Breakpoint", "Breakbeat", "A crunchy break through the crusher, kept in the low-mids.", ["breaks", "crushed"],
    kit_breaks(), BREAKS[0], 92, 0.56, master(drive=0.5, dmode="Cosmic Ray", dmix=0.5, glue=0.5, high=-3, rdecay=0.9))
add("Electro Breaks", "Breakbeat", "Electro breaks: zaps, a tom run, a driven snare.", ["breaks", "electro"],
    tweak(kit_breaks(), t3=dict(model="Plasma Zap", name="Zap", tune=6, decay=0.25, sweep=0.55, level=-12)), BREAKS[0], 128, 0.5,
    master(glue=0.6, rdecay=1.2))

# UK Garage
add("Two-Step Photon", "UK Garage", "Two-step garage: skipping kicks, a shuffled hat and a rim answer.", ["ukg", "2-step"],
    kit_garage(), GARAGE[0], 132, 0.6, master(glue=0.5, rdecay=1.3))
add("Speed Garage Sun", "UK Garage", "Speed garage: four-to-the-floor with a skippy shaker line.", ["ukg", "speed garage"],
    kit_garage(), GARAGE[1], 130, 0.56, master(glue=0.6, rdecay=1.2, low=1))
add("Bassline Orbit", "UK Garage", "Bassline house: stepping kicks and stabs of rim.", ["ukg", "bassline"],
    tweak(kit_garage(), t1=dict(decay=0.5, drive=0.2)), GARAGE[1], 134, 0.54, master(glue=0.6, rdecay=1.0))
add("Late Night Stepper", "UK Garage", "Late-night 2-step with wood and shaker, a wet clap.", ["ukg", "late night"],
    tweak(kit_garage(), t3=dict(b=0.4)), GARAGE[0], 130, 0.62, master(glue=0.4, rdecay=2.2, rret=0.7))
add("Grime Pulsar", "UK Garage", "Grime: square-wave FM perc, sparse kick, snare on three.", ["grime"],
    tweak(kit_garage(), t6=dict(model="Pulsar FM", name="Square", tune=0, decay=0.3, tone=0.5, color=0.15, level=-7), t2=dict(level=-2)),
    GARAGE[0], 140, 0.5, master(glue=0.5, rdecay=1.0))
add("Future Garage Haze", "UK Garage", "Future garage: washed claps, vinyl air, wide shuffled hats.", ["ukg", "future garage"],
    tweak(kit_garage(), t3=dict(b=0.55, decay=0.6), t8=dict(model="Solar Wind", name="Vinyl", decay=0.9, snap=1.0, color=1.0, tone=0.2, level=-20)),
    GARAGE[0], 132, 0.62, master(glue=0.4, rdecay=4.0, size=1.8, rret=0.65, high=-2), macros=(0, 0, 0, 0.3))
add("UK Funky Comet", "UK Garage", "UK funky: tribal rims and congas over a four-four kick.", ["ukg", "funky"],
    tweak(kit_garage(), t8=dict(name="Conga", model="Nebula Modal", tune=0, decay=0.35, color=0.05, snap=0.6, level=-6)),
    GARAGE[1], 128, 0.58, master(glue=0.5, rdecay=1.3))
add("Garage Shimmer", "UK Garage", "Shuffled garage with a bright, airy hat and a soft kick.", ["ukg", "bright"],
    tweak(kit_garage(), t4=dict(tone=0.8), t1=dict(tone=0.4)), GARAGE[1], 130, 0.6, master(glue=0.4, high=1.5, rdecay=1.5))
add("Dubstep Precursor", "UK Garage", "Early dubstep: halftime snare, deep kick, lots of space.", ["dubstep", "140"],
    tweak(kit_garage(), t1=dict(decay=0.55, tune=-2), t2=dict(decay=0.45, b=0.3)), GARAGE[0], 140, 0.5,
    master(glue=0.4, rdecay=2.5, size=1.6, ediv="1/8.", fb=0.4, eret=0.4))

# Lo-Fi
add("Rainy Window", "Lo-Fi", "Lo-fi beat: soft kick, warm snare, vinyl crackle, loose hats.", ["lofi", "chill"],
    kit_lofi(), LOFI[0], 78, 0.6, master(drive=0.2, dmode="Tube", glue=0.4, high=-3, rdecay=1.4, rret=0.5))
add("Tape Sun", "Lo-Fi", "Tape-saturated lo-fi with a rim on the backbeat and a wood block.", ["lofi", "tape"],
    tweak(kit_lofi(), t2=dict(level=-6)), LOFI[1], 82, 0.58, master(drive=0.3, dmode="Tube", glue=0.4, high=-4, rdecay=1.2))
add("Study Orbit", "Lo-Fi", "Quiet study beat: low-passed, slow, gentle.", ["lofi", "study"],
    tweak(kit_lofi(), t1=dict(cutoff=2500), t2=dict(cutoff=4000)), LOFI[0], 72, 0.62, master(glue=0.3, high=-5, rdecay=1.6))
add("Cosmic Ray Cassette", "Lo-Fi", "Bit-reduced cassette drums through Cosmic Ray.", ["lofi", "crushed"],
    kit_lofi(), LOFI[1], 85, 0.57, master(drive=0.45, dmode="Cosmic Ray", dmix=0.45, glue=0.4, high=-3, rdecay=1.0))
add("Jazz Nebula", "Lo-Fi", "Jazzy lo-fi: brush snare, ride, upright-ish wood.", ["lofi", "jazz"],
    tweak(kit_lofi(), t2=dict(color=0.9, tone=0.35), t5=dict(model="Corona Cymbal", name="Ride", decay=0.6, tone=0.45, color=0.4, choke=0, level=-13)),
    LOFI[0], 76, 0.63, master(glue=0.3, high=-2, rdecay=1.8))
add("Bedroom Pulsar", "Lo-Fi", "Bedroom-producer beat with an FM perc and a warm room.", ["lofi", "bedroom"],
    tweak(kit_lofi(), t7=dict(model="Pulsar FM", name="Perc", tune=5, decay=0.25, color=0.3, tone=0.3, level=-12)), LOFI[1], 80, 0.6,
    master(drive=0.2, glue=0.4, high=-3, rdecay=1.5))
add("Slow Eclipse", "Lo-Fi", "Sleepy beat at 70 with long tails and a damped snare.", ["lofi", "slow"],
    tweak(kit_lofi(), t2=dict(decay=0.5, cutoff=3500)), LOFI[0], 70, 0.6, master(glue=0.3, high=-4, rdecay=2.5, rret=0.6))
add("Dusty Corona", "Lo-Fi", "Dusty boom-bap-leaning lo-fi with a crunchy rim.", ["lofi", "dusty"],
    tweak(kit_lofi(), t3=dict(drive=0.4)), LOFI[1], 88, 0.56, master(drive=0.3, dmode="Tube", glue=0.5, high=-3, rdecay=0.9))
add("Night Bus Drift", "Lo-Fi", "Late-night lo-fi: soft and wide with a little echo on the rim.", ["lofi", "night"],
    tweak(kit_lofi(), t3=dict(a=0.35)), LOFI[0], 74, 0.6, master(glue=0.3, high=-3, ediv="1/8.", fb=0.35, eret=0.4, rdecay=2.0, width=1.25))

# Synthwave
add("Neon Horizon", "Synthwave", "Synthwave: gated-reverb snare, straight hats, tom fills.", ["synthwave", "80s"],
    kit_synthwave(), SYNTHWAVE[0], 100, 0.5, master(glue=0.5, rdecay=1.0, size=1.3, rret=0.7))
add("Outrun Corona", "Synthwave", "Outrun: driving 16th hats and a huge snare.", ["synthwave", "outrun"],
    tweak(kit_synthwave(), t2=dict(b=0.55)), SYNTHWAVE[1], 118, 0.5, master(glue=0.5, rdecay=1.2, rret=0.75))
add("Darkwave Pulse", "Synthwave", "Darkwave: cold kit, metallic clap, long hall.", ["synthwave", "darkwave"],
    tweak(kit_synthwave(), t3=dict(color=0.85, tone=0.4), t1=dict(tone=0.45)), SYNTHWAVE[0], 124, 0.5,
    master(glue=0.4, rdecay=3.0, size=1.8, rret=0.6, high=-1))
add("Retro Racer", "Synthwave", "Fast retro drums with a crash on every phrase.", ["synthwave", "fast"],
    kit_synthwave(), SYNTHWAVE[1], 130, 0.5, master(glue=0.6, rdecay=1.2))
add("Power Ballad Star", "Synthwave", "Slow power-ballad drums: massive snare, tom rolls.", ["synthwave", "ballad"],
    tweak(kit_synthwave(), t2=dict(decay=0.65, b=0.6)), SYNTHWAVE[0], 76, 0.5, master(glue=0.5, rdecay=2.5, rret=0.7))
add("Vapor Orbit", "Synthwave", "Vaporwave: slowed, wide and washed.", ["synthwave", "vaporwave"],
    tweak(kit_synthwave(), t1=dict(tune=-3), t2=dict(tune=-3)), SYNTHWAVE[1], 84, 0.5,
    master(glue=0.3, high=-3, rdecay=4.0, size=2.0, rret=0.7, width=1.3), macros=(0, 0.2, 0, 0.3))
add("Chrome Machine", "Synthwave", "Machine-pop drums: clap and snare together, tight hats.", ["synthwave", "pop"],
    kit_synthwave(), SYNTHWAVE[0], 112, 0.5, master(glue=0.6, rdecay=1.1))
add("Midnight Drive", "Synthwave", "Mid-tempo night drive, Simmons-style toms.", ["synthwave", "toms"],
    tweak(kit_synthwave(), t6=dict(sweep=0.75), t7=dict(sweep=0.75)), SYNTHWAVE[1], 108, 0.5, master(glue=0.5, rdecay=1.6))
add("Laser Grid", "Synthwave", "Synthwave with laser zaps on the off-beats.", ["synthwave", "zap"],
    tweak(kit_synthwave(), t8=dict(model="Plasma Zap", name="Laser", tune=12, decay=0.3, sweep=0.7, snap=0.6, level=-12, a=0.3)),
    SYNTHWAVE[0], 114, 0.5, master(glue=0.5, ediv="1/8.", fb=0.35, eret=0.4, rdecay=1.4))

# Industrial
add("Foundry", "Industrial", "Industrial: distorted kick, comb-filtered clang, metal scrape.", ["industrial", "metal"],
    kit_industrial(), INDUSTRIAL[0], 128, 0.5, master(drive=0.3, dmode="Tube", glue=0.6, ratio=4, rdecay=2.0))
add("EBM Signal", "Industrial", "EBM: marching kick and snare, hats and sparks.", ["industrial", "ebm"],
    tweak(kit_industrial(), t1=dict(drive=0.45), t5=dict(level=-30)), INDUSTRIAL[1], 120, 0.5, master(drive=0.2, glue=0.6, rdecay=1.4))
add("Pressure Chamber", "Industrial", "Heavy and compressed, steam bursts, anvil hits.", ["industrial", "heavy"],
    kit_industrial(), INDUSTRIAL[1], 132, 0.5, master(drive=0.4, dmode="Fold", dmix=0.5, glue=0.8, thr=-20, ratio=5, rdecay=1.6))
add("Rust Belt", "Industrial", "Slow industrial with a long reverb and metallic hits.", ["industrial", "slow"],
    tweak(kit_industrial(), t6=dict(b=0.45)), INDUSTRIAL[0], 96, 0.5, master(drive=0.3, glue=0.5, rdecay=4.5, size=2.2, rret=0.6))
add("Power Noise", "Industrial", "Power noise: everything overdriven, crushed and loud.", ["industrial", "noise"],
    tweak(kit_industrial(), t1=dict(drive=0.8), t2=dict(drive=0.8)), INDUSTRIAL[0], 140, 0.5,
    master(drive=0.6, dmode="Cosmic Ray", dmix=0.6, glue=0.7, ratio=6, rdecay=1.2))
add("Assembly Line", "Industrial", "Mechanical sequence: 1/32 sparks, steady kick.", ["industrial", "mechanical"],
    kit_industrial(), INDUSTRIAL[1], 126, 0.5, master(drive=0.25, glue=0.6, rdecay=1.5))
add("Furnace Floor", "Industrial", "Industrial techno with a pounding kick and claps.", ["industrial", "techno"],
    tweak(kit_industrial(), t3=dict(model="Burst Clap", filt="Off", name="Clap", tune=0, color=0.7)), INDUSTRIAL[0], 136, 0.5,
    master(drive=0.3, dmode="Tube", glue=0.7, rdecay=2.2))
add("Static Field", "Industrial", "Static and crackle, gated snare, sparse kick.", ["industrial", "glitch"],
    tweak(kit_industrial(), t8=dict(level=-10)), INDUSTRIAL[1], 110, 0.5, master(drive=0.2, glue=0.5, rdecay=1.0))
add("Hammer Orbit", "Industrial", "Hammering anvil and comb clang in a 3-against-4 loop.", ["industrial", "polymeter"],
    kit_industrial(), INDUSTRIAL[0], 124, 0.5, master(drive=0.3, glue=0.6, rdecay=2.0))

# World & Percussion
add("Nebula Congas", "World & Percussion", "Modal congas, a surdo pulse, shaker and clave.", ["percussion", "latin"],
    kit_world(), WORLD[0], 112, 0.55, master(glue=0.3, rdecay=1.4, rret=0.6))
add("Afro Orbit", "World & Percussion", "Afro-house percussion: woodblock, agogo, djembe.", ["percussion", "afro"],
    kit_world(), WORLD[0], 122, 0.56, master(glue=0.4, rdecay=1.6))
add("Six-Eight Sun", "World & Percussion", "Six-eight groove in triplet eighths: bell, shaker, congas.", ["percussion", "6/8", "triplet"],
    kit_world(), WORLD[1], 100, 0.5, master(glue=0.3, rdecay=1.5))
add("Batucada Flare", "World & Percussion", "Samba-flavoured: surdo, high congas, busy shaker.", ["percussion", "samba"],
    tweak(kit_world(), t1=dict(decay=0.6)), WORLD[0], 104, 0.52, master(glue=0.4, rdecay=1.0))
add("Gamelan Quasar", "World & Percussion", "Metallophone-style bells and gongs on modal and bell models.", ["percussion", "bells"],
    tweak(kit_world(), t2=dict(color=0.95, tone=0.7, decay=0.6, name="Gong Lo"), t3=dict(color=0.95, tone=0.7, decay=0.55, name="Gong Hi"), t6=dict(color=1.0, decay=0.6, name="Bell")),
    WORLD[1], 92, 0.5, master(glue=0.3, rdecay=3.0, size=1.6, rret=0.6))
add("Tabla Pulsar", "World & Percussion", "Tabla-like FM and modal drums with pitch sweeps.", ["percussion", "tabla"],
    tweak(kit_world(), t2=dict(model="Pulsar FM", name="Dayan", tune=5, decay=0.3, color=0.1, tone=0.35, sweep=0.15),
          t1=dict(model="Orbit Tom", name="Bayan", tune=-7, decay=0.45, sweep=0.7, level=-2)),
    WORLD[0], 96, 0.55, master(glue=0.3, rdecay=1.4))
add("Taiko Gravity", "World & Percussion", "Taiko: huge low drums and wood clicks.", ["percussion", "taiko"],
    tweak(kit_world(), t1=dict(model="Gravity Kick", name="Taiko", tune=0, decay=0.55, color=0.2, sweep=0.2, tone=0.3)),
    WORLD[1], 88, 0.5, master(glue=0.5, rdecay=2.5, size=1.8, rret=0.65))
add("Steel Drum Comet", "World & Percussion", "Steel-pan-like modal glass tones over hand percussion.", ["percussion", "steel"],
    tweak(kit_world(), t2=dict(color=0.9, tone=0.8, decay=0.5, tune=7, name="Pan Lo"), t3=dict(color=0.9, tone=0.8, decay=0.5, tune=12, name="Pan Hi")),
    WORLD[0], 110, 0.55, master(glue=0.3, rdecay=1.4))
add("Desert Caravan", "World & Percussion", "Frame-drum style groove with shakers and clave in triplets.", ["percussion", "frame drum"],
    tweak(kit_world(), t1=dict(model="Orbit Tom", name="Frame Drum", tune=-10, decay=0.5, tone=0.6, snap=0.6, color=0.35, sweep=0.2)),
    WORLD[1], 98, 0.5, master(glue=0.3, rdecay=1.8))


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
