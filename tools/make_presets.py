"""Generate data/presets/factory.json: the factory presets, as parameter sets.

Each preset lists only the parameters that differ from the plugin defaults; everything else is reset to
its default when the preset loads. Values come from families (rooms, halls, plates, chambers, spaces,
special, impulse responses) with small deterministic variations. They have been measured for sanity
(tests/plugin_smoke.cpp renders each one) but not auditioned by ear: treat them as starting points.
Usage: python3 tools/make_presets.py
"""
import json, os

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "presets", "factory.json")
presets = []


def add(name, category, tags, params, ir=""):
    presets.append({"name": name, "category": category, "tags": tags, "ir": ir, "params": params})


def alg(size, mid, low_x, high_x, pre, er_db, tail_db=0.0, mix=30.0, mod=1.0, rate=0.7, quality="Normal", **extra):
    p = {"mode": "Algorithmic", "size": size, "decayMid": mid, "decayLow": round(mid * low_x, 3), "decayHigh": round(mid * high_x, 3),
         "preDelay": pre, "erLevel": er_db, "tailLevel": tail_db, "mix": mix, "modDepth": mod, "modRate": rate}
    if quality != "Normal":
        p["quality"] = quality
    p.update(extra)
    return p


# --- Acoustic engine: physical rooms. -------------------------------------------------------------------
# Materials are read from dsp/space/room_model.cpp so the preset tool and the engine agree.
import re, math
_SRC = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "dsp", "space", "room_model.cpp")).read()
MATERIALS = {m.group(1): [float(v) for v in m.group(2).split(",")]
             for m in re.finditer(r'\{"([^"]+)",\s*\{([^}]*)\},\s*\{[^}]*\}\}', _SRC)}
MICS = ["ORTF", "XY", "AB", "Wide AB", "Blumlein", "Mono"]
BANDS = [63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]


def air_db_per_m(f, t=20.0, hr=50.0):
    T, T0, T01 = t + 273.15, 293.15, 273.16
    C = -6.8346 * (T01 / T) ** 1.261 + 4.6151
    h = hr * 10 ** C
    frO = 24 + 4.04e4 * h * (0.02 + h) / (0.391 + h)
    frN = (T / T0) ** -0.5 * (9 + 280 * h * math.exp(-4.170 * ((T / T0) ** (-1 / 3) - 1)))
    return 8.686 * f * f * (1.84e-11 * (T / T0) ** 0.5 + (T / T0) ** -2.5 * (0.01275 * math.exp(-2239.1 / T) / (frO + f * f / frO) + 0.1068 * math.exp(-3352.0 / T) / (frN + f * f / frN)))


FIXTURE = [.20, .22, .25, .28, .30, .30, .30, .30, .30]   # as kFixtureAlpha in room_model.cpp


def eyring(L, W, H, walls, floor, ceil, t=20.0, hr=50.0, furn=0.2):
    c = 331.3 * math.sqrt(1 + t / 273.15)
    V, sf, sw = L * W * H, L * W, 2 * (L * H + W * H)
    S = 2 * sf + sw
    out = []
    for b, f in enumerate(BANDS):
        eff = lambda m: 1 - (1 - MATERIALS[m][b]) * (1 - furn * FIXTURE[b])
        a = (sf * eff(floor) + sf * eff(ceil) + sw * eff(walls)) / S
        m = air_db_per_m(f, t, hr) / 4.343
        out.append(24 * math.log(10) * V / (c * (-S * math.log(1 - min(a, 0.995)) + 4 * m * V)))
    return out


def space(L, W, H, walls, floor, ceil, dist, mix=30.0, mic="ORTF", spread=3.0, pre=0.0, er=0.0, tail=0.0, offset=0.0,
          hum=50.0, temp=20.0, furn=0.2, direct=None, quality="Normal", mod=0.3, rate=0.5):
    for m in (walls, floor, ceil):
        assert m in MATERIALS, m
    assert mic in MICS, mic
    p = {"mode": "Acoustic", "roomLength": L, "roomWidth": W, "roomHeight": H, "roomWalls": walls, "roomFloor": floor,
         "roomCeiling": ceil, "roomDistance": dist, "roomSpread": spread, "roomMic": mic, "roomOffset": offset,
         "roomHumidity": hum, "roomTemp": temp, "roomFurnish": furn, "preDelay": pre, "erLevel": er, "tailLevel": tail, "mix": mix,
         "modDepth": mod, "modRate": rate}
    if direct is not None:
        p["roomDirect"] = direct
    if quality != "Normal":
        p["quality"] = quality
    return p


# Physical rooms (Acoustic engine). Dimensions in metres (length from the stage to the back, width,
# height); one material each for the walls, floor and ceiling; source-to-listener distance; microphone pair.
# Real spaces are modelled on published dimensions and surfaces (docs/ACOUSTIC.md); their names describe the
# kind of space, not the venue.
rooms = [
    # name, (L, W, H), walls, floor, ceiling, distance, extra, tags
    ("Closet", (2.0, 1.5, 2.4), "Gypsum board", "Carpet", "Gypsum board", 0.7, dict(rt=0.18, spread=0.4, mic="XY"), ["tiny", "dry"]),
    ("Vocal Booth", (2.2, 1.8, 2.4), "Gypsum board", "Carpet", "Acoustic tile", 0.6, dict(rt=0.12, spread=0.3, mic="XY"), ["small", "dry", "vocal"]),
    ("Small Wood Room", (4.5, 3.8, 2.8), "Wood panelling", "Wood floor", "Wood panelling", 2.0, dict(rt=0.5, spread=1.0), ["small", "warm", "wood"]),
    ("Drum Room", (7.0, 5.5, 3.5), "Brick", "Wood floor", "Gypsum board", 3.0, dict(rt=0.8, spread=2.0, mic="AB"), ["drums", "tight"]),
    ("Living Room", (5.0, 4.0, 2.5), "Gypsum board", "Thick carpet", "Plaster", 2.5, dict(rt=0.4, spread=1.2), ["medium", "warm", "domestic"]),
    ("Tiled Bathroom", (2.6, 2.0, 2.5), "Glazed tile", "Glazed tile", "Plaster", 1.0, dict(rt=0.8, spread=0.4, mic="XY"), ["small", "bright", "hard"]),
    ("Studio A", (9.0, 7.0, 4.0), "Wood panelling", "Wood floor", "Acoustic tile", 3.0, dict(rt=0.4, spread=2.0), ["medium", "neutral", "studio"]),
    ("Studio B Live Room", (11.0, 8.0, 5.0), "Brick", "Wood floor", "Wood panelling", 4.0, dict(rt=0.9, spread=2.5, mic="AB"), ["medium", "live", "studio"]),
    ("Big Studio Live Room", (18.35, 11.65, 7.31), "Wood panelling", "Wood floor", "Plaster", 6.0, dict(rt=1.2, spread=4.0, mic="AB"), ["large", "studio", "classic"]),
    ("Stairwell", (3.0, 3.0, 14.0), "Painted concrete", "Painted concrete", "Painted concrete", 2.0, dict(rt=4.0, spread=0.8, mic="XY"), ["hard", "bright", "odd"]),
    ("Garage", (7.0, 6.0, 3.0), "Concrete block", "Rough concrete", "Painted concrete", 4.0, dict(rt=1.6, spread=2.0), ["hard", "medium"]),
    ("Dead Room", (6.0, 5.0, 3.0), "Curtains", "Thick carpet", "Gypsum board", 2.5, dict(rt=0.15, spread=1.5), ["dry", "controlled"]),
    ("Piano Room", (8.0, 6.0, 4.0), "Plaster", "Parquet", "Wood panelling", 3.0, dict(rt=0.9, spread=1.5), ["acoustic", "warm"]),
    ("Warm Den", (5.5, 4.5, 2.7), "Wood panelling", "Thick carpet", "Gypsum board", 2.2, dict(rt=0.4, spread=1.2), ["warm", "dark"]),
    ("Bright Room", (6.0, 5.0, 3.0), "Plaster", "Polished stone", "Plaster", 2.5, dict(rt=1.0, spread=1.5), ["bright", "clear"]),
    ("Guitar Cabinet Room", (4.0, 3.5, 2.6), "Brick", "Carpet", "Gypsum board", 1.0, dict(rt=0.45, spread=0.3, mic="Mono"), ["guitar", "tight"]),
    ("Wide Room", (12.0, 9.0, 3.5), "Gypsum board", "Wood floor", "Gypsum board", 4.0, dict(rt=0.9, spread=5.0, mic="Wide AB"), ["wide", "medium"]),
    ("Snare Room", (6.0, 5.0, 3.2), "Brick", "Wood floor", "Plaster", 2.0, dict(rt=0.9, spread=0.5), ["drums", "snare"]),
    ("Overheads Room", (9.0, 7.0, 4.5), "Wood panelling", "Wood floor", "Plaster", 2.5, dict(rt=0.9, spread=2.0, mic="AB"), ["drums", "wide"]),
    ("Podcast Room", (4.0, 3.5, 2.6), "Gypsum board", "Thick carpet", "Acoustic tile", 0.8, dict(rt=0.19, spread=0.3, mic="XY"), ["speech", "dry"]),
    ("Control Room", (6.0, 5.0, 3.0), "Wood panelling", "Carpet", "Acoustic tile", 2.0, dict(rt=0.25, spread=1.5), ["neutral", "tight"]),
]
halls = [
    ("Small Hall", (22, 14, 9), "Plaster on lath", "Parquet", "Plaster on lath", 10, dict(rt=1.4, spread=4), ["small", "natural"]),
    ("Medium Hall", (30, 18, 12), "Plaster on lath", "Parquet", "Plaster on lath", 14, dict(rt=1.7, spread=6), ["medium", "natural"]),
    ("Concert Hall", (40, 22, 16), "Plaster on lath", "Empty seats", "Plaster on lath", 18, dict(rt=2.0, spread=8), ["large", "natural", "orchestral"]),
    ("Grand Hall", (48.8, 19.8, 17.8), "Plaster on lath", "Empty seats", "Plaster on lath", 20, dict(rt=2.0, spread=8, mic="AB"), ["large", "lush", "shoebox"]),
    ("Warm Hall", (36, 24, 15), "Heavy wood", "Parquet", "Plaster on lath", 16, dict(rt=1.9, spread=8, hum=60), ["warm", "dark"]),
    ("Bright Hall", (32, 20, 13), "Plaster", "Parquet", "Plaster", 12, dict(rt=2.2, spread=6), ["bright", "clear"]),
    ("Choir Hall", (34, 18, 18), "Plaster", "Parquet", "Plaster on lath", 14, dict(rt=2.6, spread=8, mic="AB"), ["vocal", "choir", "warm"]),
    ("Chamber Hall", (20, 12, 8), "Plaster on lath", "Parquet", "Plaster", 8, dict(rt=1.3, spread=4), ["small", "strings"]),
    ("Recital Hall", (24, 15, 10), "Heavy wood", "Parquet", "Plaster on lath", 9, dict(rt=1.5, spread=3), ["piano", "acoustic"]),
    ("Opera House", (30, 28, 20), "Heavy wood", "Audience", "Plaster on lath", 15, dict(rt=1.4, spread=10), ["large", "vocal"]),
    ("Symphony Hall", (38, 23, 18.6), "Plaster on lath", "Empty seats", "Plaster on lath", 18, dict(rt=1.9, spread=8, mic="AB"), ["large", "orchestral", "shoebox"]),
    ("Lush Hall", (44, 27.7, 17), "Plaster on lath", "Empty seats", "Plaster on lath", 20, dict(rt=2.0, spread=10, mic="Wide AB"), ["lush", "wide"]),
    ("Vocal Hall", (28, 18, 12), "Plaster on lath", "Parquet", "Heavy wood", 10, dict(rt=1.6, spread=3, pre=10.0), ["vocal", "pop"]),
    ("Snare Hall", (30, 20, 12), "Plaster", "Wood floor", "Plaster", 8, dict(rt=1.8, spread=1), ["drums", "snare", "big"]),
    ("Ambient Hall", (50, 30, 20), "Plaster", "Wood floor", "Plaster", 32, dict(rt=3.0, spread=12, mic="Wide AB"), ["ambient", "soft"]),
    ("Tight Hall", (24, 16, 10), "Wood panelling", "Empty seats", "Plaster", 10, dict(rt=1.2, spread=5), ["tight", "mix"]),
    ("Dark Hall", (40, 24, 16), "Heavy wood", "Parquet", "Heavy wood", 18, dict(rt=2.2, spread=8, hum=25), ["dark", "smooth"]),
    ("Hall Of Strings", (36, 20, 15), "Plaster on lath", "Parquet", "Wood panelling", 15, dict(rt=2.0, spread=9, mic="AB"), ["strings", "warm"]),
    ("Early Hall", (26, 16, 11), "Plaster on lath", "Parquet", "Plaster on lath", 5, dict(rt=1.6, spread=4), ["early", "clear"]),
    ("Wide Hall", (40, 30, 14), "Plaster on lath", "Parquet", "Plaster on lath", 16, dict(rt=1.9, spread=12, mic="Wide AB"), ["wide", "stereo"]),
]
chambers = [
    ("Stone Chamber", (8, 6, 4), "Rough stone", "Rough stone", "Rough stone", 4, dict(rt=2.5, spread=1.5), ["stone", "natural"]),
    ("Echo Chamber", (7, 5, 3.5), "Plaster", "Painted concrete", "Plaster", 4, dict(rt=2.5, spread=1.0, mic="AB"), ["classic", "bright"]),
    ("Tile Chamber", (6, 4, 3), "Glazed tile", "Glazed tile", "Glazed tile", 3, dict(rt=2.8, spread=1.0), ["hard", "bright"]),
    ("Wood Chamber", (6, 5, 3.5), "Heavy wood", "Heavy wood", "Heavy wood", 3, dict(rt=1.2, spread=1.0), ["wood", "warm"]),
    ("Concrete Chamber", (10, 7, 4.5), "Painted concrete", "Painted concrete", "Painted concrete", 5, dict(rt=3.0, spread=2.0, mic="AB"), ["hard", "big"]),
    ("Small Chamber", (4, 3, 2.6), "Plaster", "Painted concrete", "Plaster", 2, dict(rt=1.5, spread=0.5), ["small", "tight"]),
    ("Vocal Chamber", (7, 5, 3.5), "Plaster", "Wood floor", "Plaster", 3.5, dict(rt=2.0, spread=0.5), ["vocal", "warm"]),
    ("Strings Chamber", (9, 6, 4), "Plaster", "Wood floor", "Heavy wood", 4.5, dict(rt=2.2, spread=2.0, mic="AB"), ["strings", "natural"]),
    ("Brass Chamber", (8, 6, 4), "Glazed tile", "Painted concrete", "Plaster", 4, dict(rt=2.5, spread=1.5), ["brass", "bright"]),
    ("Dark Chamber", (9, 6, 4), "Brick", "Rough concrete", "Brick", 4, dict(rt=2.2, spread=1.5, hum=20), ["dark", "smooth"]),
    ("Live Chamber", (12, 8, 5), "Plaster", "Painted concrete", "Plaster", 6, dict(rt=3.0, spread=3.0, mic="Wide AB"), ["live", "wide"]),
    ("Basement", (10, 8, 2.6), "Cinder block", "Rough concrete", "Rough concrete", 4, dict(rt=1.0, spread=2.0), ["dark", "hard"]),
]
spaces = [
    ("Cathedral", (120, 48, 33), "Rough stone", "Polished stone", "Rough stone", 30, dict(rt=8.0, spread=10, mic="AB", pre=0.0), ["huge", "stone", "choir"]),
    ("Cave", (60, 25, 15), "Rough stone", "Rough stone", "Rough stone", 20, dict(rt=5.0, spread=6, hum=90, temp=10), ["dark", "natural"]),
    ("Hangar", (100, 60, 20), "Sheet metal", "Painted concrete", "Sheet metal", 30, dict(rt=6.0, spread=10, mic="Wide AB"), ["huge", "metal"]),
    ("Cistern", (60, 30, 9), "Painted concrete", "Water", "Painted concrete", 18, dict(rt=8.0, spread=6, hum=90, temp=12), ["dark", "huge"]),
    ("Arena", (90, 70, 30), "Concrete block", "Empty seats", "Sheet metal", 35, dict(rt=3.0, spread=15, mic="Wide AB"), ["huge", "live"]),
    ("Warehouse", (50, 30, 10), "Brick", "Painted concrete", "Sheet metal", 20, dict(rt=3.5, spread=8), ["hard", "big"]),
    ("Tunnel", (100, 6, 5), "Painted concrete", "Rough concrete", "Painted concrete", 25, dict(rt=4.0, spread=2, mic="AB"), ["hard", "long"]),
    ("Church", (30, 12, 12), "Plaster", "Polished stone", "Heavy wood", 12, dict(rt=3.5, spread=4), ["stone", "choir"]),
    ("Canyon", (100, 40, 45), "Rough stone", "Rough stone", "Open sky", 40, dict(spread=10, mic="Wide AB", hum=20, temp=30), ["outdoor", "huge"]),
    ("Courtyard", (30, 20, 15), "Brick", "Polished stone", "Open sky", 10, dict(spread=4, mic="AB"), ["outdoor", "slap"]),
    ("Subway", (80, 12, 6), "Glazed tile", "Polished stone", "Painted concrete", 15, dict(rt=3.0, spread=4), ["hard", "urban"]),
    ("Parking Garage", (60, 40, 2.6), "Painted concrete", "Rough concrete", "Painted concrete", 15, dict(rt=2.6, spread=4, mic="AB"), ["hard", "urban"]),
    ("Mausoleum", (18, 18, 30), "Polished stone", "Polished stone", "Polished stone", 8, dict(rt=15.0, spread=3, mic="AB"), ["dark", "stone", "long"]),
    ("Abbey", (60, 20, 22), "Rough stone", "Polished stone", "Rough stone", 22, dict(rt=6.0, spread=8, mic="AB"), ["stone", "choir", "lush"]),
    ("Indoor Pool", (25, 15, 7), "Glazed tile", "Water", "Plaster", 10, dict(rt=3.5, spread=5, hum=80, temp=28), ["hard", "bright"]),
    ("Giant Hall", (80, 40, 25), "Plaster", "Wood floor", "Plaster", 30, dict(rt=5.0, spread=12, mic="Wide AB"), ["huge", "lush"]),
    ("Fuel Tank", (100, 9, 13.5), "Painted concrete", "Painted concrete", "Painted concrete", 30, dict(furn=0.0, spread=3, mic="AB", hum=95, temp=8), ["huge", "endless", "odd"]),
]
REVIEW = []
for cat, fam_list, mix in (("Rooms", rooms, 26.0), ("Halls", halls, 28.0), ("Chambers", chambers, 28.0), ("Spaces", spaces, 30.0)):
    for name, (L, W, H), walls, floor, ceil, dist, extra, tags in fam_list:
        extra = dict(extra)
        # Furnishing: solved so the mid-band decay matches the space's typical measured RT (the "rt" entry).
        rt = extra.pop("rt", None)
        if rt is not None:
            lo, hi = 0.0, 1.0
            for _ in range(40):
                f = 0.5 * (lo + hi)
                t1k = eyring(L, W, H, walls, floor, ceil, extra.get("temp", 20.0), extra.get("hum", 50.0), f)[4]
                lo, hi = (f, hi) if t1k > rt else (lo, f)
            extra["furn"] = round(0.5 * (lo + hi), 3)
        add(name, cat, tags, space(L, W, H, walls, floor, ceil, dist, mix=extra.pop("mix", mix), **extra))
        t = eyring(L, W, H, walls, floor, ceil, extra.get("temp", 20.0), extra.get("hum", 50.0), extra.get("furn", 0.2))
        REVIEW.append("%-22s %-9s furn %5.3f  T60 125 %5.2f  1k %5.2f  4k %5.2f  8k %5.2f s%s" % (name, cat, extra.get("furn", 0.2), t[1], t[4], t[6], t[7],
                      "" if rt is None or abs(t[4] / rt - 1) < 0.05 else "   (target %g s not reachable)" % rt))

# Plates: the algorithmic engine with its early reflections off (a plate has no room).
plates = [
    ("Vocal Plate", 1.0, 1.9, ["vocal", "bright"]), ("Snare Plate", 0.9, 1.4, ["drums", "snare"]),
    ("Short Plate", 0.8, 0.9, ["tight", "bright"]), ("Long Plate", 1.2, 3.0, ["long", "smooth"]),
    ("Dark Plate", 1.0, 2.0, ["dark", "smooth"]), ("Bright Plate", 1.0, 1.8, ["bright", "airy"]),
    ("Gold Foil", 1.1, 2.6, ["bright", "classic"]), ("Steel Plate", 1.0, 2.2, ["metal", "dense"]),
    ("Drum Plate", 0.9, 1.1, ["drums", "tight"]), ("Guitar Plate", 1.0, 1.7, ["guitar", "classic"]),
    ("Pre-delayed Plate", 1.1, 2.1, ["vocal", "clear"]), ("Wide Plate", 1.2, 2.4, ["wide", "stereo"]),
    ("Plate Swell", 1.3, 3.8, ["long", "ambient"]), ("Tiny Plate", 0.7, 0.7, ["tiny", "tight"]),
    ("Mono Plate", 0.9, 1.6, ["mono", "classic"]),
]
for i, (n, size, t, tags) in enumerate(plates):
    dark = "dark" in tags
    add(n, "Plates", tags, alg(size, t, 0.9, 0.85 if dark else 1.25, pre=(i % 4) * 6.0, er_db=-40.0, tail_db=0.0 if not dark else 1.0,
                               mix=24.0 + (i % 4) * 2, mod=1.4 + 0.1 * (i % 3), rate=0.9))

special = [
    ("Infinite Wash", alg(2.5, 30.0, 1.0, 0.7, 40.0, -20.0, mix=40.0, mod=2.0, rate=0.4), ["infinite", "ambient"]),
    ("Dark Swell", alg(2.4, 12.0, 1.2, 0.2, 120.0, -30.0, mix=45.0, mod=1.8), ["dark", "swell"]),
    ("Bright Shimmer Bed", alg(2.0, 9.0, 0.8, 1.4, 60.0, -30.0, mix=40.0, mod=3.0, rate=1.4), ["bright", "airy", "ambient"]),
    ("Chorused Space", alg(1.8, 5.0, 1.0, 0.6, 30.0, -12.0, mix=35.0, mod=3.8, rate=1.2), ["chorus", "wide"]),
    ("Pre-delay Slap", alg(1.0, 1.5, 1.0, 0.6, 140.0, -6.0, mix=25.0), ["slap", "vocal"]),
    ("Tail Only Hall", alg(1.8, 3.2, 1.1, 0.5, 20.0, -60.0, mix=35.0), ["tail", "clean"]),
    ("Early Reflections Only", alg(1.4, 0.4, 1.0, 0.8, 0.0, 0.0, tail_db=-60.0, mix=35.0), ["early", "tight"]),
    ("Metallic Tank", alg(0.6, 2.5, 0.7, 1.4, 0.0, -40.0, mix=30.0, mod=0.0, diffusion=False), ["metal", "odd", "effect"]),
    ("Wobble Room", alg(1.2, 2.4, 1.0, 0.6, 15.0, -9.0, mix=35.0, mod=4.0, rate=2.6), ["wobble", "effect"]),
    ("Sub Rumble Hall", alg(2.2, 6.0, 2.0, 0.3, 50.0, -18.0, mix=30.0), ["low", "dark", "cinematic"]),
    ("Air Only", alg(1.5, 2.0, 0.3, 1.5, 15.0, -30.0, mix=22.0), ["bright", "airy"]),
    ("Gated Room", alg(0.9, 0.25, 1.0, 0.9, 0.0, -2.0, mix=40.0, mod=0.3), ["gated", "drums", "short"]),
    ("Eco Big Hall", alg(2.0, 3.5, 1.1, 0.5, 20.0, -9.0, mix=28.0, quality="Eco"), ["cheap", "many instances"]),
    ("High Quality Hall", alg(1.9, 3.2, 1.1, 0.55, 20.0, -9.0, mix=28.0, quality="High"), ["best", "final mix"]),
    ("Full Wet Pad", alg(2.2, 7.0, 1.1, 0.5, 40.0, -20.0, mix=100.0, mod=2.0), ["pad", "send"]),
]
for n, p, tags in special:
    add(n, "Special", tags, p)

# Impulse-response presets (the three factory IRs). Convolution = IR only; Hybrid = IR early part + matched FDN tail.
irs = [("small_room", "Small Room", False), ("studio_hall", "Studio Hall", True), ("large_hall", "Large Hall", True)]
for key, label, long_tail in irs:
    base = {"erLevel": 0.0, "mix": 30.0}
    add(label + " (IR)", "Impulse Responses", ["convolution", "natural"], dict(base, mode="Convolution"), ir=key)
    add(label + " Wide (IR)", "Impulse Responses", ["convolution", "wide"], dict(base, mode="Convolution", irWidth=1.6), ir=key)
    add(label + " Dark (IR)", "Impulse Responses", ["convolution", "dark"], dict(base, mode="Convolution", irHighCut=4500.0), ir=key)
    add(label + " Tight (IR)", "Impulse Responses", ["convolution", "tight"], dict(base, mode="Convolution", irDecay=0.5), ir=key)
    add(label + " Pre-delayed (IR)", "Impulse Responses", ["convolution", "vocal"], dict(base, mode="Convolution", preDelay=45.0), ir=key)
    if long_tail:
        add(label + " Hybrid", "Impulse Responses", ["hybrid", "natural"], dict(base, mode="Hybrid", tailLevel=0.0), ir=key)
        add(label + " Hybrid Long", "Impulse Responses", ["hybrid", "long"], dict(base, mode="Hybrid", tailScale=2.0), ir=key)
        add(label + " Hybrid Dark", "Impulse Responses", ["hybrid", "dark"], dict(base, mode="Hybrid", tailScale=1.3, irHighCut=5000.0), ir=key)
        add(label + " Reverse", "Impulse Responses", ["convolution", "reverse", "effect"], dict(base, mode="Convolution", irReverse=True, irLowCut=120.0), ir=key)
        add(label + " Stretched", "Impulse Responses", ["convolution", "huge"], dict(base, mode="Convolution", irStretch=1.6), ir=key)

# Cosmic suite: three per design, from restrained to extreme.
cosmic = [
    ("Shimmer", [("Shimmer Hall", (0.45, 0.6, 0.6), 1.6, 4.0, 30), ("Shimmer Cathedral", (0.7, 0.6, 0.75), 2.4, 8.0, 35), ("Shimmer Fifths", (0.6, 0.3, 0.6), 1.8, 5.0, 32)]),
    ("Redshift", [("Redshift Drift", (0.5, 0.9, 0.5), 1.8, 5.0, 30), ("Redshift Abyss", (0.8, 0.9, 0.8), 2.6, 10.0, 38), ("Redshift Fifth Down", (0.6, 0.4, 0.4), 1.6, 4.0, 30)]),
    ("Corona", [("Corona Halo", (0.5, 0.5, 0.7), 1.8, 5.0, 30), ("Corona Choir", (0.75, 0.3, 0.8), 2.4, 8.0, 36), ("Corona Soft", (0.35, 0.7, 0.5), 1.5, 3.5, 28)]),
    ("Wormhole", [("Wormhole", (0.5, 0.5, 0.6), 1.8, 4.5, 32), ("Wormhole Deep", (0.8, 0.8, 0.9), 2.5, 7.0, 38), ("Wormhole Subtle", (0.25, 0.3, 0.5), 1.4, 3.0, 28)]),
    ("Event Horizon", [("Event Horizon", (0.6, 0.5, 0.4), 2.0, 6.0, 32), ("Event Horizon Collapse", (0.9, 0.9, 0.7), 2.6, 12.0, 40), ("Event Horizon Warm", (0.35, 0.2, 0.2), 1.6, 4.0, 30)]),
    ("Nebula", [("Nebula", (0.5, 0.5, 0.5), 2.2, 6.0, 40), ("Nebula Dust Cloud", (0.85, 0.8, 0.7), 2.8, 9.0, 50), ("Nebula Sparse", (0.2, 0.3, 0.3), 2.0, 5.0, 35)]),
    ("Pulsar", [("Pulsar", (0.45, 0.6, 0.3), 1.8, 4.0, 40), ("Pulsar Fast", (0.8, 0.9, 0.5), 1.6, 3.0, 45), ("Pulsar Breath", (0.2, 0.2, 0.7), 2.2, 6.0, 40)]),
    ("Solar Wind", [("Solar Wind", (0.6, 0.4, 0.5), 2.0, 5.0, 34), ("Solar Wind Storm", (0.95, 0.8, 0.8), 2.5, 7.0, 40), ("Solar Wind Calm", (0.3, 0.15, 0.3), 1.8, 4.0, 30)]),
    ("Aurora", [("Aurora", (0.6, 0.5, 0.3), 2.0, 5.0, 34), ("Aurora Borealis", (0.85, 0.7, 0.6), 2.6, 8.0, 40), ("Aurora Faint", (0.35, 0.3, 0.1), 1.8, 4.0, 30)]),
    ("Supernova", [("Supernova", (0.5, 0.8, 0.3), 2.2, 6.0, 40), ("Supernova Remnant", (0.8, 1.0, 0.6), 2.8, 10.0, 48), ("Supernova Flash", (0.25, 0.9, 0.0), 1.8, 4.0, 38)]),
]
for mode, items in cosmic:
    for name, (a, b, c), size, mid, mix in items:
        add(name, "Cosmic", ["cosmic", mode.lower().replace(" ", "")], {"mode": mode, "cosmicA": a, "cosmicB": b, "cosmicC": c, "size": size,
                                                                        "decayMid": mid, "decayLow": round(mid * 1.1, 2), "decayHigh": round(mid * 0.45, 2), "mix": mix, "erLevel": -12.0, "modDepth": 1.5})

# Vintage suite: in the manner of hardware from 1970 to 1990, three per unit.
vintage = [
    ("Steel Plate", [("Steel Plate Classic", (0.4, 0.4, 0.2), 1.0, 2.6, 28, -60.0), ("Steel Plate Bright", (0.1, 0.3, 0.1), 1.0, 3.4, 30, -60.0), ("Steel Plate Damped", (0.85, 0.6, 0.5), 0.9, 1.4, 26, -60.0)]),
    ("Spring Tank", [("Spring Tank", (0.5, 0.5, 0.5), 1.0, 1.8, 26, -60.0), ("Spring Tank Surf", (1.0, 0.85, 0.7), 1.1, 2.6, 34, -60.0), ("Spring Tank Amp", (0.0, 0.4, 0.35), 0.9, 1.3, 22, -60.0)]),
    ("Digital '76", [("Digital '76", (0.5, 0.4, 0.4), 1.4, 3.0, 30, -14.0), ("Digital '76 Lush", (0.7, 0.3, 0.8), 1.8, 5.0, 34, -14.0), ("Digital '76 Lo-Fi", (0.2, 0.9, 0.3), 1.2, 2.4, 28, -14.0)]),
    ("Concert '78", [("Concert '78", (0.5, 0.5, 0.6), 1.8, 3.2, 30, -12.0), ("Concert '78 Cathedral", (0.6, 0.6, 0.5), 2.6, 7.0, 36, -14.0), ("Concert '78 Tight", (0.3, 0.4, 0.8), 1.2, 1.8, 26, -10.0)]),
    ("Random Hall", [("Random Hall", (0.4, 0.5, 0.5), 1.6, 2.8, 30, -10.0), ("Random Hall Spin", (0.8, 0.8, 0.4), 2.0, 4.5, 34, -12.0), ("Random Hall Chamber", (0.3, 0.3, 0.9), 0.9, 1.4, 26, -8.0)]),
    ("Nonlinear '81", [("Nonlinear '81 Gate", (0.4, 0.7, 0.0), 1.0, 1.0, 34, -60.0), ("Nonlinear '81 Reverse", (0.6, 0.8, 1.0), 1.0, 1.0, 36, -60.0), ("Nonlinear '81 Snare", (0.25, 0.9, 0.15), 1.0, 1.0, 30, -60.0)]),
    ("Space Station", [("Space Station", (0.5, 0.6, 0.5), 1.0, 1.0, 30, -60.0), ("Space Station Orbit", (0.8, 0.85, 0.3), 1.0, 1.0, 36, -60.0), ("Space Station Grainy", (0.4, 0.7, 0.9), 1.0, 1.0, 30, -60.0)]),
    ("Hall '85", [("Hall '85", (0.4, 0.3, 0.4), 1.4, 2.4, 30, -10.0), ("Hall '85 Crunchy", (0.85, 0.7, 0.8), 1.4, 2.0, 30, -8.0), ("Hall '85 Big", (0.3, 0.2, 0.3), 2.2, 4.5, 34, -12.0)]),
]
for mode, items in vintage:
    for name, (a, b, c), size, mid, mix, er in items:
        add(name, "Vintage", ["vintage", mode.lower().replace(" ", "").replace("'", "")], {"mode": mode, "cosmicA": a, "cosmicB": b, "cosmicC": c, "size": size,
                                                                       "decayMid": mid, "decayLow": round(mid * 1.15, 2), "decayHigh": round(mid * 0.5, 2), "mix": mix, "erLevel": er})

# Default preset first, as a clean reference.
presets.insert(0, {"name": "Init", "category": "Rooms", "tags": ["default"], "ir": "", "params": {}})
names = [p["name"] for p in presets]
assert len(names) == len(set(names)), "duplicate preset names"
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w") as f:
    json.dump({"version": 1, "presets": presets}, f, indent=1)
print("\n".join(REVIEW))
print(len(presets), "presets ->", OUT)
