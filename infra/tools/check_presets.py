"""Checks preset files written with the make_presets.py helpers, without writing presets.json.

    python3 check_presets.py [--rework] presets_more_bass.py [...]

--rework allows a preset to reuse the name of a factory preset (a redesign that replaces it; remove the original).

Every parameter id must exist, choices must be spelled exactly, numbers must be inside their ranges, names must be
unique (across the files given and the factory set), and the macro / mod wheel / aftertouch convention must hold.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import make_presets as mp  # noqa: E402  (helpers only: main() is not run on import)

SRC = open(os.path.join(HERE, "..", "plugin", "PluginProcessor.cpp")).read()
HDR = open(os.path.join(HERE, "..", "dsp", "infra", "params.h")).read()


def names(var):
    m = re.search(var + r"\[[^\]]*\]\s*=\s*\{(.*?)\};", HDR, re.S)
    return re.findall(r'"([^"]*)"', m.group(1))


CHOICES = {"Table": names("kTableNames"), "Warp": names("kWarpNames"), "subShape": names("kSubShapeNames"),
           "noiseType": names("kNoiseNames"), "fType": names("kFilterNames"), "Shape": names("kLfoShapeNames"),
           "Div": names("kDivisionNames"), "Src": names("kModSourceNames"), "Dst": names("kModDestNames"),
           "voiceMode": names("kVoiceModeNames"), "arpMode": names("kArpModeNames"), "drvMode": names("kDriveModeNames")}


def choices_for(pid):
    for k, v in CHOICES.items():
        if pid == k or pid.endswith(k):
            return v
    return None


# Parameter ids and ranges, from the processor's layout (expanded for the per-osc / per-LFO / per-slot / per-step loops).
RANGES = {}
for x in ("a", "b"):
    RANGES.update({x + "On": "bool", x + "Table": "choice", x + "Morph": (0, 1), x + "Oct": (-3, 3), x + "Semi": (-12, 12),
                   x + "Fine": (-100, 100), x + "Level": (0, 1), x + "Pan": (-1, 1), x + "Unison": (1, 8), x + "Detune": (0, 1),
                   x + "Width": (0, 1), x + "Warp": "choice", x + "WarpAmt": (0, 1), x + "Phase": (0, 1), x + "PhaseRand": (0, 1)})
RANGES.update({"subShape": "choice", "subOct": (1, 2), "subLevel": (0, 1), "noiseType": "choice", "noiseLevel": (0, 1),
               "fType": "choice", "fCutoff": (20, 20000), "fRes": (0, 1), "fDrive": (0, 1), "fEnv": (-72, 72), "fKey": (0, 1),
               "fVel": (0, 1), "ampCurve": (-1, 1), "velSens": (0, 1)})
for e in ("amp", "fenv", "menv"):
    RANGES.update({e + "A": (0, 20), e + "D": (0.001, 20), e + "S": (0, 1), e + "R": (0.001, 20)})
for n in (1, 2):
    RANGES.update({f"l{n}Shape": "choice", f"l{n}Rate": (0.01, 40), f"l{n}Sync": "bool", f"l{n}Div": "choice",
                   f"l{n}Retrig": "bool", f"l{n}Fade": (0, 10)})
for s in range(1, 9):
    RANGES.update({f"m{s}Src": "choice", f"m{s}Dst": "choice", f"m{s}Amt": (-1, 1)})
for m in range(1, 5):
    RANGES[f"macro{m}"] = (0, 1)
RANGES.update({"voiceMode": "choice", "polyphony": (1, 16), "glide": (0, 2), "glideLegato": "bool", "bendRange": (0, 24),
               "drift": (0, 1), "tune": (-100, 100), "arpOn": "bool", "arpMode": "choice", "arpDiv": "choice", "arpOct": (1, 4),
               "arpGate": (0.05, 2), "arpSwing": (0, 1), "arpLatch": "bool", "arpLength": (1, 16), "arpProb": (0, 1)})
for s in range(1, 17):
    RANGES.update({f"s{s}Vel": (0, 1), f"s{s}Tr": (-12, 12), f"s{s}Rat": (1, 4)})
RANGES.update({"drvMode": "choice", "drvAmt": (0, 1), "drvTone": (0, 1), "drvMix": (0, 1), "chRate": (0.02, 8), "chDepth": (0, 1),
               "chMix": (0, 1), "dlySync": "bool", "dlyDiv": "choice", "dlyMs": (1, 2000), "dlyFb": (0, 0.95), "dlyDamp": (0, 1),
               "dlyMix": (0, 1), "dlyPing": "bool", "rvSize": (0.5, 3), "rvDecay": (0.2, 30), "rvDamp": (0, 1), "rvPre": (0, 250),
               "rvMix": (0, 1), "width": (0, 2), "outDb": (-30, 18), "drvLow": (0, 400), "sqAmt": (0, 1), "monoLow": (0, 300)})



def check(preset):
    errs = []
    for k, v in preset["params"].items():
        r = RANGES.get(k)
        if r is None:
            errs.append(f"unknown parameter {k}")
        elif r == "bool":
            if not isinstance(v, bool):
                errs.append(f"{k} must be True/False, got {v!r}")
        elif r == "choice":
            opts = choices_for(k)
            if v not in opts:
                errs.append(f'{k}: no choice named "{v}" (one of {opts})')
        else:
            if isinstance(v, bool) or not isinstance(v, (int, float)) or not (r[0] <= v <= r[1]):
                errs.append(f"{k}={v!r} outside {r}")
    srcs = {v for k, v in preset["params"].items() if re.fullmatch(r"m\dSrc", k)}
    for need in ("Heat", "Gravity", "Orbit", "Flux", "Mod Wheel", "Aftertouch"):
        if need not in srcs:
            errs.append(f"no modulation slot uses {need}")
    for s in range(1, 9):
        if f"m{s}Src" in preset["params"] and preset["params"].get(f"m{s}Amt", 0) == 0:
            errs.append(f"m{s} has zero amount")
    if not preset["description"] or len(preset["tags"]) < 2:
        errs.append("needs a description and at least two tags")
    return errs


def main():
    args = [a for a in sys.argv[1:] if a != "--rework"]
    rework = len(args) != len(sys.argv) - 1
    factory = set() if rework else {p["name"] for p in mp.PRESETS}
    mp.PRESETS.clear()
    for path in args:
        exec(compile(open(path).read(), path, "exec"), vars(mp))
    seen, bad = set(), 0
    for p in mp.PRESETS:
        errs = check(p)
        if p["name"] in factory or p["name"] in seen:
            errs.append("duplicate name")
        seen.add(p["name"])
        if p["category"] not in ("Bass", "Lead", "Pad", "Keys", "Pluck", "Bell", "Brass & Poly", "Strings", "Chords", "Melody", "Arp", "Sequence", "Drone", "Texture", "FX"):
            errs.append(f"unknown category {p['category']}")
        if errs:
            bad += 1
            print(f"{p['name']}:")
            for e in errs:
                print("   ", e)
    cats = {}
    for p in mp.PRESETS:
        cats[p["category"]] = cats.get(p["category"], 0) + 1
    print(f"{len(mp.PRESETS)} presets checked, {bad} with problems; by category: {cats}")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
