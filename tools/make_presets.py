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
    p = {"size": size, "decayMid": mid, "decayLow": round(mid * low_x, 3), "decayHigh": round(mid * high_x, 3),
         "preDelay": pre, "erLevel": er_db, "tailLevel": tail_db, "mix": mix, "modDepth": mod, "modRate": rate}
    if quality != "Normal":
        p["quality"] = quality
    p.update(extra)
    return p


# Rooms: small to medium, short and fairly dark.
rooms = [
    ("Closet", 0.5, 0.35, ["tiny", "dry"]), ("Vocal Booth", 0.55, 0.45, ["small", "dry", "vocal"]),
    ("Small Wood Room", 0.65, 0.6, ["small", "warm", "wood"]), ("Drum Room", 0.8, 0.7, ["drums", "tight"]),
    ("Living Room", 0.85, 0.8, ["medium", "warm"]), ("Tiled Bathroom", 0.6, 0.9, ["small", "bright", "hard"]),
    ("Studio A", 0.95, 0.75, ["medium", "neutral"]), ("Studio B Live Room", 1.1, 0.95, ["medium", "live"]),
    ("Stairwell", 0.9, 1.2, ["hard", "bright", "odd"]), ("Garage", 1.05, 1.1, ["hard", "medium"]),
    ("Dead Room", 0.7, 0.4, ["dry", "controlled"]), ("Piano Room", 1.0, 0.85, ["acoustic", "warm"]),
    ("Warm Den", 0.75, 0.65, ["warm", "dark"]), ("Bright Room", 0.8, 0.7, ["bright", "clear"]),
    ("Guitar Cabinet Room", 0.7, 0.55, ["guitar", "tight"]), ("Wide Room", 1.1, 0.8, ["wide", "medium"]),
    ("Snare Room", 0.75, 0.5, ["drums", "snare"]), ("Overheads Room", 1.0, 0.65, ["drums", "wide"]),
    ("Podcast Room", 0.55, 0.4, ["speech", "dry"]), ("Control Room", 0.85, 0.55, ["neutral", "tight"]),
]
for i, (n, size, t, tags) in enumerate(rooms):
    bright = "bright" in tags or "hard" in tags
    add(n, "Rooms", tags, alg(size, t, 1.15 if not bright else 0.95, 0.55 if not bright else 0.85,
                              pre=float(i % 4) * 2.5, er_db=-3.0 - (i % 3), tail_db=-1.0, mix=24.0 + (i % 4) * 3,
                              mod=0.6 + 0.1 * (i % 4), diffusion=True))

halls = [
    ("Small Hall", 1.2, 1.8, ["small", "natural"]), ("Medium Hall", 1.5, 2.3, ["medium", "natural"]),
    ("Concert Hall", 1.8, 2.9, ["large", "natural", "orchestral"]), ("Grand Hall", 2.1, 3.6, ["large", "lush"]),
    ("Warm Hall", 1.6, 2.6, ["warm", "dark"]), ("Bright Hall", 1.5, 2.2, ["bright", "clear"]),
    ("Choir Hall", 1.7, 3.2, ["vocal", "choir", "warm"]), ("Chamber Hall", 1.3, 2.0, ["small", "strings"]),
    ("Recital Hall", 1.4, 2.1, ["piano", "acoustic"]), ("Opera House", 2.0, 3.4, ["large", "vocal"]),
    ("Symphony Hall", 2.2, 3.8, ["large", "orchestral"]), ("Lush Hall", 1.9, 3.1, ["lush", "wide"]),
    ("Vocal Hall", 1.5, 2.4, ["vocal", "pop"]), ("Snare Hall", 1.6, 2.0, ["drums", "snare", "big"]),
    ("Ambient Hall", 2.1, 4.2, ["ambient", "soft"]), ("Tight Hall", 1.3, 1.6, ["tight", "mix"]),
    ("Dark Hall", 1.8, 3.0, ["dark", "smooth"]), ("Hall Of Strings", 1.7, 2.7, ["strings", "warm"]),
    ("Early Hall", 1.4, 2.2, ["early", "clear"]), ("Wide Hall", 1.9, 2.8, ["wide", "stereo"]),
]
for i, (n, size, t, tags) in enumerate(halls):
    dark = "dark" in tags or "warm" in tags
    bright = "bright" in tags or "clear" in tags
    add(n, "Halls", tags, alg(size, t, 1.25 if dark else 1.1, 0.4 if dark else (0.7 if bright else 0.55),
                              pre=10.0 + (i % 5) * 7.0, er_db=-9.0 + (i % 3), tail_db=0.0, mix=26.0 + (i % 5) * 3,
                              mod=1.0 + 0.1 * (i % 5), rate=0.6 + 0.05 * (i % 4)))

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

chambers = [
    ("Stone Chamber", 1.0, 1.7, ["stone", "natural"]), ("Echo Chamber", 1.1, 2.0, ["classic", "bright"]),
    ("Tile Chamber", 0.9, 1.5, ["hard", "bright"]), ("Wood Chamber", 0.9, 1.3, ["wood", "warm"]),
    ("Concrete Chamber", 1.2, 2.4, ["hard", "big"]), ("Small Chamber", 0.8, 1.2, ["small", "tight"]),
    ("Vocal Chamber", 1.0, 1.6, ["vocal", "warm"]), ("Strings Chamber", 1.1, 1.9, ["strings", "natural"]),
    ("Brass Chamber", 1.0, 1.5, ["brass", "bright"]), ("Dark Chamber", 1.1, 2.1, ["dark", "smooth"]),
    ("Live Chamber", 1.3, 2.2, ["live", "wide"]), ("Basement", 0.95, 1.8, ["dark", "hard"]),
]
for i, (n, size, t, tags) in enumerate(chambers):
    add(n, "Chambers", tags, alg(size, t, 1.1, 0.5 if "dark" in tags or "warm" in tags else 0.7, pre=5.0 + (i % 3) * 4.0,
                                 er_db=-8.0 + (i % 2), tail_db=0.0, mix=28.0, mod=0.9))

spaces = [
    ("Cathedral", 2.6, 8.5, ["huge", "stone", "choir"]), ("Cave", 2.2, 6.0, ["dark", "natural"]),
    ("Hangar", 2.8, 5.5, ["huge", "metal"]), ("Cistern", 2.4, 9.5, ["dark", "huge"]),
    ("Arena", 2.9, 4.8, ["huge", "live"]), ("Warehouse", 2.5, 4.0, ["hard", "big"]),
    ("Tunnel", 2.0, 5.0, ["hard", "long"]), ("Church", 2.3, 5.4, ["stone", "choir"]),
    ("Canyon", 3.0, 7.0, ["outdoor", "huge"]), ("Subway", 1.9, 3.6, ["hard", "urban"]),
    ("Parking Garage", 2.1, 4.4, ["hard", "urban"]), ("Mausoleum", 2.2, 7.5, ["dark", "stone"]),
    ("Abbey", 2.5, 6.5, ["stone", "choir", "lush"]), ("Indoor Pool", 1.8, 3.4, ["hard", "bright"]),
    ("Giant Hall", 3.0, 10.0, ["huge", "lush"]),
]
for i, (n, size, t, tags) in enumerate(spaces):
    dark = "dark" in tags or "stone" in tags
    add(n, "Spaces", tags, alg(size, t, 1.25, 0.35 if dark else 0.5, pre=25.0 + (i % 4) * 18.0, er_db=-12.0, tail_db=0.0,
                               mix=30.0 + (i % 3) * 3, mod=1.4 + 0.1 * (i % 4), rate=0.5))

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

# Default preset first, as a clean reference.
presets.insert(0, {"name": "Init", "category": "Rooms", "tags": ["default"], "ir": "", "params": {}})
names = [p["name"] for p in presets]
assert len(names) == len(set(names)), "duplicate preset names"
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w") as f:
    json.dump({"version": 1, "presets": presets}, f, indent=1)
print(len(presets), "presets ->", OUT)
