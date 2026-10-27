"""Factory presets for Chromosphere | INFRA.

Every preset is designed here one by one: a few lines of parameters, its modulation, and the four macros.
Only values that differ from the parameter defaults are written. Choice parameters are written by name.

The macro convention holds in every preset (PRD section 4.6):
  HEAT     brightness and energy: cutoff, drive, warp, FM
  GRAVITY  weight and pull: sub, envelope depth, low end, glide
  ORBIT    motion: LFO depth and rate, morph sweeps, unison detune
  FLUX     space and aftermath: effect sends, delay feedback, reverb
and every preset routes the mod wheel and aftertouch somewhere musical.

Levels: tools/preset_levels.json holds per-preset output trims (dB). The render check measures each preset's
loudness (infra_preset_render --measure measured.json); `make_presets.py --calibrate measured.json` moves every trim
toward the target loudness and rewrites the presets. Two or three rounds converge (the output limiter is not linear).

Usage: python3 make_presets.py [--calibrate measured.json]  (writes ../data/presets.json)
"""
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "data", "presets.json")
LEVELS = os.path.join(HERE, "preset_levels.json")

PRESETS = []

STEP_ALL = [(1.0, 0, 1)] * 16


def mods(*slots):
    """slots: (source, destination, amount); up to eight."""
    assert len(slots) <= 8, "eight modulation slots"
    out = {}
    for i, (s, d, a) in enumerate(slots):
        out[f"m{i + 1}Src"] = s
        out[f"m{i + 1}Dst"] = d
        out[f"m{i + 1}Amt"] = round(a, 3)
    return out


def steps(lane):
    """lane: list of (velocity, transpose, ratchet), up to 16."""
    out = {}
    for i, (v, t, r) in enumerate(lane):
        out[f"s{i + 1}Vel"] = v
        if t:
            out[f"s{i + 1}Tr"] = t
        if r != 1:
            out[f"s{i + 1}Rat"] = r
    return out


def adsr(prefix, a, d, s, r):
    return {f"{prefix}A": a, f"{prefix}D": d, f"{prefix}S": s, f"{prefix}R": r}


def osc(x, table, morph=0.5, level=0.8, unison=1, detune=0.2, width=0.5, oct=0, semi=0, fine=0.0, warp="Off",
        warpAmt=0.0, pan=0.0, on=True, phaseRand=None, phase=None):
    d = {f"{x}On": on, f"{x}Table": table, f"{x}Morph": morph, f"{x}Level": level, f"{x}Unison": unison,
         f"{x}Detune": detune, f"{x}Width": width}
    if oct:
        d[f"{x}Oct"] = oct
    if semi:
        d[f"{x}Semi"] = semi
    if fine:
        d[f"{x}Fine"] = fine
    if warp != "Off":
        d[f"{x}Warp"] = warp
        d[f"{x}WarpAmt"] = warpAmt
    if pan:
        d[f"{x}Pan"] = pan
    if phaseRand is not None:
        d[f"{x}PhaseRand"] = phaseRand
    if phase is not None:
        d[f"{x}Phase"] = phase
    return d


def filt(type_="LP 24", cutoff=9000, res=0.1, env=0.0, key=0.5, drive=0.0, vel=0.0):
    return {"fType": type_, "fCutoff": cutoff, "fRes": res, "fEnv": env, "fKey": key, "fDrive": drive, "fVel": vel}


def lfo(n, shape="Sine", rate=2.0, sync=False, div=None, retrig=True, fade=0.0):
    d = {f"l{n}Shape": shape, f"l{n}Rate": rate, f"l{n}Sync": sync, f"l{n}Retrig": retrig, f"l{n}Fade": fade}
    if div:
        d[f"l{n}Div"] = div
    return d


def fx(drive=None, chorus=None, delay=None, reverb=None, width=None, out=None, sub_clean=None, squash=None, mono_low=None):
    """drive: (mode, amount, tone, mix); chorus: (rate, depth, mix); delay: (div or ms, feedback, damp, mix, pingpong);
    reverb: (size, decay, damp, pre, mix).
    sub_clean: Hz below which the drive leaves the signal clean (keeps a bass's sub round while its mids grit up);
    squash: OTT-style three-band compression 0..1 (density and presence); mono_low: Hz below which the output is mono."""
    d = {}
    if sub_clean is not None:
        d["drvLow"] = sub_clean
    if squash is not None:
        d["sqAmt"] = squash
    if mono_low is not None:
        d["monoLow"] = mono_low
    if drive:
        d.update({"drvMode": drive[0], "drvAmt": drive[1], "drvTone": drive[2], "drvMix": drive[3]})
    if chorus:
        d.update({"chRate": chorus[0], "chDepth": chorus[1], "chMix": chorus[2]})
    if delay:
        t, fb, damp, mix, pp = delay
        if isinstance(t, str):
            d.update({"dlySync": True, "dlyDiv": t})
        else:
            d.update({"dlySync": False, "dlyMs": t})
        d.update({"dlyFb": fb, "dlyDamp": damp, "dlyMix": mix, "dlyPing": pp})
    if reverb:
        d.update({"rvSize": reverb[0], "rvDecay": reverb[1], "rvDamp": reverb[2], "rvPre": reverb[3], "rvMix": reverb[4]})
    else:
        d["rvMix"] = 0.0 if reverb == () else d.get("rvMix", 0.15)
    if width is not None:
        d["width"] = width
    if out is not None:
        d["outDb"] = out
    return d


def arp(mode="Up", div="1/16", octaves=1, gate=0.6, swing=0.0, latch=False, length=16, prob=1.0, lane=None):
    d = {"arpOn": True, "arpMode": mode, "arpDiv": div, "arpOct": octaves, "arpGate": gate, "arpSwing": swing,
         "arpLatch": latch, "arpLength": length, "arpProb": prob}
    if lane:
        d.update(steps(lane))
    return d


def add(name, category, description, tags, *parts, **extra):
    params = {}
    for p in parts:
        params.update(p)
    params.update(extra)
    PRESETS.append({"name": name, "category": category, "tags": tags, "description": description,
                    "author": "Chromosphere", "params": params})


# =====================================================================================================
# INIT
# =====================================================================================================
add("Init", "Init", "A single Classic oscillator through the ladder: a blank sheet.", ["init"],
    mods(("Mod Wheel", "Cutoff", 0.3), ("Heat", "Cutoff", 0.4), ("Gravity", "Sub Level", 0.6),
         ("Orbit", "Morph A", 0.5), ("Flux", "FX Reverb", 0.5), ("Aftertouch", "Resonance", 0.3)))

# =====================================================================================================
# BASS
# =====================================================================================================
add("Event Horizon Sub", "Bass", "A sine sub with a folded edge that only appears when you lean on the keys.",
    ["sub", "deep", "mono"],
    osc("a", "Event Horizon", morph=0.0, level=0.9), osc("b", "Classic", morph=0.0, oct=-1, level=0.5),
    filt("LP 24", 900, 0.05, env=18, key=0.3), adsr("amp", 0.002, 0.6, 0.9, 0.12), adsr("fenv", 0.001, 0.25, 0.0, 0.2),
    {"voiceMode": "Legato", "glide": 0.04, "drift": 0.05},
    mods(("Velocity", "Morph A", 0.35), ("Heat", "Morph A", 0.6), ("Heat", "Cutoff", 0.25),
         ("Gravity", "Level B", 0.45), ("Orbit", "Detune", 0.4), ("Flux", "FX Drive", 0.4),
         ("Mod Wheel", "Morph A", 0.5), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Tube", 0.15, 0.5, 1.0), reverb=()))

add("Redshift Reese", "Bass", "Two detuned saws beating against each other, slowly sliding down the spectrum.",
    ["reese", "dnb", "detuned"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.35, width=0.3), osc("b", "Plasma", morph=0.1, oct=-1, level=0.5),
    filt("LP 24", 1400, 0.3, env=12, key=0.3, drive=0.2), adsr("amp", 0.005, 0.5, 1.0, 0.2), adsr("fenv", 0.01, 0.6, 0.3, 0.3),
    lfo(1, "Triangle", 0.12, retrig=False),
    {"voiceMode": "Mono", "glide": 0.06, "subLevel": 0.4},
    mods(("LFO 1", "Cutoff", 0.08), ("Heat", "Cutoff", 0.35), ("Heat", "Filter Drive", 0.5), ("Gravity", "Sub Level", 0.5),
         ("Orbit", "Detune", 0.45), ("Flux", "FX Chorus", 0.6), ("Mod Wheel", "Morph B", 0.6), ("Aftertouch", "Resonance", 0.3)),
    fx(drive=("Soft", 0.2, 0.6, 1.0), chorus=(0.3, 0.4, 0.15), reverb=()))

add("Magnetar Growl", "Bass", "FM bass whose index opens with the mod envelope: a magnetic snarl.",
    ["fm", "growl", "dubstep"],
    osc("a", "Magnetar", morph=0.15, level=0.85), osc("b", "Classic", morph=0.0, oct=-1, level=0.4),
    filt("LP 12", 2200, 0.35, env=24, key=0.2, drive=0.3), adsr("amp", 0.003, 0.4, 0.85, 0.1), adsr("fenv", 0.002, 0.35, 0.2, 0.2),
    adsr("menv", 0.002, 0.45, 0.1, 0.2),
    {"voiceMode": "Mono", "glide": 0.03},
    mods(("Mod Env", "Morph A", 0.55), ("Mod Wheel", "Morph A", 0.45), ("Heat", "Morph A", 0.45), ("Heat", "Filter Drive", 0.4),
         ("Gravity", "Level B", 0.5), ("Orbit", "Morph A", 0.3), ("Flux", "FX Drive", 0.5), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.35, 0.55, 1.0), reverb=()))

add("Ion Thruster", "Bass", "Pulse bass with a tempo-locked pulse-width wobble that speeds up as Orbit comes up.",
    ["pwm", "synthwave", "pulse"],
    osc("a", "Classic", morph=0.8, level=0.85), osc("b", "Classic", morph=0.55, oct=-1, level=0.5, fine=-6),
    filt("LP 24", 700, 0.35, env=36, key=0.4), adsr("amp", 0.002, 0.3, 0.75, 0.12), adsr("fenv", 0.001, 0.22, 0.15, 0.15),
    lfo(1, "Triangle", sync=True, div="1/8", retrig=False),
    {"polyphony": 4},
    mods(("LFO 1", "Morph A", 0.1), ("Heat", "Cutoff", 0.4), ("Gravity", "Level B", 0.4), ("Orbit", "LFO 1 Rate", 0.25),
         ("Velocity", "Cutoff", 0.2), ("Flux", "FX Delay", 0.3), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8.", 0.25, 0.5, 0.0, True), reverb=()))

add("Tidal Lock", "Bass", "A rubbery square that pulls itself into pitch with a short glide on every note.",
    ["glide", "acid", "mono"],
    osc("a", "Classic", morph=0.72, level=0.85), osc("b", "Classic", morph=0.5, level=0.0, on=False),
    filt("LP 24", 450, 0.62, env=40, key=0.3, drive=0.2), adsr("amp", 0.002, 0.4, 0.8, 0.08), adsr("fenv", 0.001, 0.18, 0.0, 0.12),
    {"voiceMode": "Legato", "glide": 0.09, "glideLegato": True},
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Resonance", 0.3), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.6),
         ("Orbit", "Pitch", 0.0), ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Filter Drive", 0.6), ("Aftertouch", "Cutoff", 0.3)),
    {"m5Src": "Orbit", "m5Dst": "Morph A", "m5Amt": -0.4},
    fx(drive=("Soft", 0.25, 0.5, 1.0), delay=("1/16.", 0.3, 0.55, 0.0, True), reverb=()))

add("Perihelion", "Bass", "A plucked Fraunhofer bass: bright attack, a dark spectral line sweeping through the decay.",
    ["pluck bass", "bright", "modern"],
    osc("a", "Fraunhofer", morph=0.0, level=0.85, unison=2, detune=0.15, width=0.2), osc("b", "Classic", morph=0.0, oct=-1, level=0.55),
    filt("LP 24", 600, 0.25, env=42, key=0.5), adsr("amp", 0.001, 0.55, 0.35, 0.15), adsr("fenv", 0.001, 0.3, 0.05, 0.2),
    adsr("menv", 0.001, 0.8, 0.0, 0.3),
    mods(("Mod Env", "Morph A", 0.7), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Morph A", 0.5), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.25)),
    fx(reverb=(0.8, 1.2, 0.5, 5, 0.05)))

add("Graviton", "Bass", "Pure low weight: a triangle and a sine sub with a slow, heavy swell on the mod wheel.",
    ["sub", "808", "clean"],
    osc("a", "Classic", morph=0.22, level=0.8), osc("b", "Classic", morph=0.0, level=0.0, on=False),
    filt("LP 12", 1200, 0.0, key=0.2), adsr("amp", 0.002, 1.8, 0.0, 0.4),
    {"subLevel": 0.7, "subShape": "Sine", "voiceMode": "Mono", "glide": 0.05},
    mods(("Velocity", "Amp", 0.15), ("Heat", "Morph A", 0.3), ("Heat", "Cutoff", 0.4), ("Gravity", "Cutoff", -0.2),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Drive", 0.5), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Pitch", 0.02)),
    fx(drive=("Tube", 0.1, 0.4, 1.0), reverb=()), ampS=0.6)

add("Neutron Star", "Bass", "Collapsed and dense: a folded wave through the comb filter tuned to the note.",
    ["metallic", "comb", "dense"],
    osc("a", "Event Horizon", morph=0.45, level=0.8), osc("b", "Classic", morph=0.0, oct=-1, level=0.45),
    filt("Comb", 220, 0.55, env=0, key=1.0), adsr("amp", 0.002, 0.5, 0.7, 0.15),
    {"voiceMode": "Mono"},
    mods(("Heat", "Morph A", 0.45), ("Heat", "Resonance", 0.3), ("Gravity", "Level B", 0.5), ("Orbit", "Cutoff", 0.08),
         ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Morph A", 0.3), ("Velocity", "Morph A", 0.2)),
    fx(drive=("Soft", 0.15, 0.6, 1.0), reverb=()))

add("Rogue Planet", "Bass", "A wandering, unbound bass: Dark Matter spectra drifting under a resonant low-pass.",
    ["evolving", "dark", "movement"],
    osc("a", "Dark Matter", morph=0.3, level=0.8, unison=2, detune=0.12), osc("b", "Classic", morph=0.5, oct=-1, level=0.4),
    filt("LP 24", 800, 0.45, env=20, key=0.3), adsr("amp", 0.004, 0.6, 0.8, 0.2), adsr("fenv", 0.003, 0.5, 0.2, 0.3),
    lfo(1, "Drift", 0.25, retrig=False),
    mods(("LFO 1", "Morph A", 0.25), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.45), ("Orbit", "Morph A", 0.4),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Morph A", 0.5), ("Aftertouch", "Cutoff", 0.25)),
    fx(chorus=(0.2, 0.3, 0.0), reverb=()), polyphony=4)

add("Coronal Loop", "Bass", "Acid in infrared: the ladder at the edge of oscillation with an accented 16th lane.",
    ["acid", "303", "arp"],
    osc("a", "Classic", morph=0.5, level=0.85), osc("b", "Classic", on=False),
    filt("LP 24", 380, 0.78, env=46, key=0.3, drive=0.25, vel=0.25), adsr("amp", 0.002, 0.3, 0.7, 0.06), adsr("fenv", 0.001, 0.2, 0.0, 0.1),
    {"voiceMode": "Legato", "glide": 0.05, "glideLegato": True},
    arp("Up", "1/16", octaves=2, gate=0.55, swing=0.2, length=16,
        lane=[(1.0, 0, 1), (0.4, 0, 1), (0.7, 12, 1), (0.0, 0, 1), (1.0, 0, 1), (0.5, -12, 1), (0.8, 0, 2), (0.4, 0, 1),
              (1.0, 7, 1), (0.0, 0, 1), (0.6, 0, 1), (0.9, 12, 1), (0.4, 0, 1), (1.0, 0, 1), (0.0, 0, 1), (0.7, 3, 1)]),
    mods(("Arp Step", "Cutoff", 0.25), ("Heat", "Resonance", 0.2), ("Heat", "Cutoff", 0.25), ("Gravity", "Sub Level", 0.5),
         ("Orbit", "Detune", 0.0), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Filter Drive", 0.6), ("Aftertouch", "Cutoff", 0.3)),
    {"m5Src": "Orbit", "m5Dst": "Morph A", "m5Amt": 0.25},
    fx(drive=("Tube", 0.3, 0.6, 1.0), delay=("1/8.", 0.35, 0.6, 0.0, True), reverb=()))

# =====================================================================================================
# LEAD
# =====================================================================================================
# =====================================================================================================
# PAD
# =====================================================================================================
add("Infrared Dawn", "Pad", "The signature: warm saws rising through the ladder, the sun coming up over the limb.",
    ["warm", "signature", "evolving"],
    osc("a", "Classic", morph=0.5, unison=5, detune=0.28, width=0.8), osc("b", "Fraunhofer", morph=0.2, unison=3, detune=0.2, level=0.45, oct=1),
    filt("LP 24", 900, 0.2, env=20, key=0.4), adsr("amp", 1.2, 2.0, 0.85, 2.5), adsr("fenv", 2.5, 4.0, 0.4, 3.0),
    adsr("menv", 3.0, 5.0, 0.6, 3.0), lfo(1, "Sine", 0.15, retrig=False), lfo(2, "Drift", 0.4, retrig=False),
    mods(("Mod Env", "Morph B", 0.6), ("LFO 1", "Cutoff", 0.06), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Cutoff", 0.3), ("Aftertouch", "Morph B", 0.3)),
    fx(chorus=(0.25, 0.5, 0.35), reverb=(2.0, 5.0, 0.45, 30, 0.35)))

add("Heliopause", "Pad", "The edge of the sun's wind: a thin, airy pad of filtered noise and a high, distant sine.",
    ["airy", "noise", "ambient"],
    osc("a", "Classic", morph=0.05, level=0.45, oct=1), osc("b", "Nebula", morph=0.6, level=0.5, unison=4, detune=0.2, width=1.0),
    filt("BP 12", 1800, 0.45, key=0.3), adsr("amp", 2.5, 3.0, 0.8, 4.0), lfo(1, "Drift", 0.2, retrig=False), lfo(2, "Sine", 0.07, retrig=False),
    {"noiseLevel": 0.45, "noiseType": "Solar Wind"},
    mods(("LFO 1", "Cutoff", 0.15), ("LFO 2", "Morph B", 0.3), ("Heat", "Cutoff", 0.3), ("Gravity", "Level A", 0.3),
         ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Resonance", 0.3)),
    fx(chorus=(0.15, 0.6, 0.3), reverb=(2.6, 8.0, 0.4, 60, 0.45), width=1.3))

add("Oort Cloud", "Pad", "A distant shell of icy bodies: glassy Quasar partials scattered by random note offsets.",
    ["glassy", "cold", "shimmer"],
    osc("a", "Quasar", morph=0.4, unison=4, detune=0.12, width=0.9), osc("b", "Classic", morph=0.0, oct=1, level=0.3),
    filt("LP 12", 6000, 0.1, key=0.4), adsr("amp", 1.5, 3.0, 0.7, 4.0), lfo(1, "Triangle", 0.11, retrig=False),
    mods(("Random", "Morph A", 0.25), ("LFO 1", "Morph A", 0.2), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.35),
         ("Orbit", "Morph A", 0.4), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Level B", 0.3)),
    fx(chorus=(0.2, 0.5, 0.3), delay=("1/4.", 0.45, 0.6, 0.15, True), reverb=(2.4, 7.0, 0.35, 40, 0.4)))

add("Nebula Choir", "Pad", "A vowel choir moving A through U, voices spread across the sky.",
    ["choir", "vocal", "vowel"],
    osc("a", "Nebula", morph=0.0, unison=6, detune=0.18, width=1.0), osc("b", "Nebula", morph=0.5, level=0.4, oct=-1, unison=2),
    filt("Formant", 600, 0.4, key=0.0), adsr("amp", 0.9, 2.0, 0.85, 2.5),
    lfo(1, "Sine", 0.09, retrig=False), lfo(2, "Sine", 4.8, fade=1.2),
    mods(("LFO 1", "Morph A", 0.6), ("LFO 2", "Pitch", 0.002), ("Heat", "Cutoff", 0.4), ("Gravity", "Level B", 0.4),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Cutoff", 0.5), ("Aftertouch", "Morph A", 0.3)),
    fx(chorus=(0.3, 0.6, 0.4), reverb=(2.2, 4.5, 0.45, 25, 0.35)))

add("Cosmic Microwave", "Pad", "The background hum of everything: warm, low, and faintly noisy.",
    ["warm", "lo-fi", "background"],
    osc("a", "Classic", morph=0.35, unison=4, detune=0.2, width=0.7), osc("b", "Ion Drive", morph=0.3, level=0.4),
    filt("LP 24", 1400, 0.15, key=0.3), adsr("amp", 1.0, 2.0, 0.9, 3.0), lfo(1, "Drift", 0.3, retrig=False),
    {"noiseLevel": 0.12, "noiseType": "Crackle", "drift": 0.5},
    mods(("LFO 1", "Cutoff", 0.08), ("Heat", "Cutoff", 0.35), ("Heat", "FX Drive", 0.3), ("Gravity", "Sub Level", 0.35),
         ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Cosmic Ray", 0.12, 0.5, 0.4), chorus=(0.3, 0.5, 0.3), reverb=(2.0, 4.0, 0.6, 20, 0.3)))

add("Aurora Borealis", "Pad", "Curtains of light: a slow sweep of the Fraunhofer lines with the chorus breathing.",
    ["shimmer", "moving", "lush"],
    osc("a", "Fraunhofer", morph=0.0, unison=5, detune=0.22, width=1.0), osc("b", "Classic", morph=0.5, level=0.35, oct=-1),
    filt("LP 12", 5200, 0.2, key=0.3), adsr("amp", 1.4, 2.0, 0.85, 3.5), lfo(1, "Triangle", 0.05, retrig=False), lfo(2, "Sine", 0.13, retrig=False),
    mods(("LFO 1", "Morph A", 0.9), ("LFO 2", "Pan", 0.3), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.4),
         ("Orbit", "LFO 1 Rate", 0.6), ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(chorus=(0.35, 0.7, 0.45), reverb=(2.4, 6.0, 0.4, 30, 0.4), width=1.3))

add("Red Giant", "Pad", "Swollen and warm, near the end of its life: a big pulse-width pad with a slow bloom.",
    ["big", "warm", "pwm"],
    osc("a", "Classic", morph=0.8, unison=4, detune=0.25, width=0.8), osc("b", "Classic", morph=0.5, unison=3, detune=0.3, level=0.5, oct=-1),
    filt("LP 24", 1100, 0.25, env=18, key=0.3), adsr("amp", 0.8, 2.0, 0.9, 2.5), adsr("fenv", 2.0, 3.0, 0.4, 2.5),
    lfo(1, "Sine", 0.3, retrig=False),
    mods(("LFO 1", "Morph A", 0.12), ("Heat", "Cutoff", 0.4), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.4),
         ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Cutoff", 0.35), ("Aftertouch", "Resonance", 0.25)),
    fx(chorus=(0.3, 0.5, 0.3), reverb=(2.2, 4.0, 0.5, 25, 0.3)))

add("Stellar Nursery", "Pad", "Gas collapsing into new stars: Dark Matter frames morphing under a slow envelope.",
    ["evolving", "texture", "birth"],
    osc("a", "Dark Matter", morph=0.0, unison=4, detune=0.15, width=0.9), osc("b", "Quasar", morph=0.2, level=0.35, oct=1),
    filt("LP 24", 2400, 0.2, key=0.3), adsr("amp", 2.0, 3.0, 0.85, 4.0), adsr("menv", 6.0, 8.0, 0.8, 4.0),
    lfo(1, "Drift", 0.15, retrig=False),
    mods(("Mod Env", "Morph A", 0.9), ("LFO 1", "Morph B", 0.4), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", -0.3),
         ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Morph A", 0.5), ("Aftertouch", "Cutoff", 0.25)),
    fx(chorus=(0.2, 0.5, 0.25), delay=("1/2", 0.4, 0.6, 0.12, True), reverb=(2.6, 7.0, 0.4, 40, 0.4)))

add("Zodiacal Light", "Pad", "Faint dust lit by the sun: a soft organ-like pad with a gentle shimmer on top.",
    ["soft", "organ", "gentle"],
    osc("a", "Ion Drive", morph=0.4, unison=3, detune=0.12, width=0.7), osc("b", "Classic", morph=0.0, oct=2, level=0.18),
    filt("LP 12", 3500, 0.1, key=0.4), adsr("amp", 0.7, 2.0, 0.9, 2.0), lfo(1, "Sine", 0.2, retrig=False),
    mods(("LFO 1", "Level B", 0.15), ("Heat", "Morph A", 0.5), ("Gravity", "Sub Level", 0.35), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Level B", 0.3), ("Heat", "Cutoff", 0.25)),
    fx(chorus=(0.4, 0.4, 0.35), reverb=(2.0, 5.0, 0.4, 30, 0.35)))

add("Dark Side", "Pad", "The far side, never lit: a low, filtered, slowly breathing pad for film cues.",
    ["dark", "cinematic", "low"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.2, width=0.8, oct=-1), osc("b", "Event Horizon", morph=0.25, level=0.4),
    filt("LP 24", 420, 0.3, key=0.2), adsr("amp", 1.5, 2.0, 0.9, 3.5), lfo(1, "Sine", 0.08, retrig=False),
    mods(("LFO 1", "Cutoff", 0.08), ("Heat", "Cutoff", 0.45), ("Gravity", "Sub Level", 0.45), ("Orbit", "Morph B", 0.5),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Morph B", 0.3), ("Orbit", "Detune", 0.3)),
    fx(chorus=(0.2, 0.4, 0.2), reverb=(2.4, 6.0, 0.6, 30, 0.35)))

add("Lightyears", "Pad", "Distance as a sound: a slow-attack string pad that keeps going long after you let go.",
    ["strings", "long release", "cinematic"],
    osc("a", "Classic", morph=0.5, unison=6, detune=0.24, width=0.9), osc("b", "Classic", morph=0.5, unison=4, detune=0.3, level=0.45, oct=1),
    filt("LP 12", 2600, 0.15, key=0.4), adsr("amp", 1.8, 2.5, 0.85, 5.0), lfo(1, "Sine", 4.6, fade=2.0),
    mods(("LFO 1", "Pitch", 0.002), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.3), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Amp", 0.2), ("Velocity", "Cutoff", 0.1)),
    fx(chorus=(0.25, 0.6, 0.4), reverb=(2.8, 8.0, 0.45, 40, 0.4)))

# =====================================================================================================
# KEYS
# =====================================================================================================
add("Ion Drive Organ", "Keys", "Drawbars from flute to full registration on the Morph dial, with a spinning chorus.",
    ["organ", "drawbar", "classic"],
    osc("a", "Ion Drive", morph=0.55, level=0.8, phaseRand=0.0), osc("b", "Ion Drive", morph=0.0, oct=-1, level=0.3),
    filt("LP 12", 9000, 0.0, key=0.0), adsr("amp", 0.003, 0.2, 1.0, 0.06),
    {"velSens": 0.0, "drift": 0.05},
    mods(("Mod Wheel", "FX Chorus", 0.5), ("Heat", "Morph A", 0.45), ("Heat", "FX Drive", 0.4), ("Gravity", "Level B", 0.45),
         ("Orbit", "FX Chorus", 0.4), ("Flux", "FX Reverb", 0.35), ("Aftertouch", "Morph A", 0.25), ("Gravity", "Sub Level", 0.3)),
    fx(drive=("Tube", 0.15, 0.55, 1.0), chorus=(0.9, 0.35, 0.35), reverb=(1.2, 1.8, 0.5, 10, 0.15)))

add("Lunar Rhodes", "Keys", "A tine piano from a moon base: FM bell-tone attack over a soft body, auto-pan on the wheel.",
    ["electric piano", "fm", "soft"],
    osc("a", "Magnetar", morph=0.1, level=0.75), osc("b", "Classic", morph=0.0, level=0.55),
    filt("LP 12", 4500, 0.05, key=0.5, vel=0.4), adsr("amp", 0.002, 2.4, 0.0, 0.5), adsr("menv", 0.001, 0.4, 0.0, 0.3),
    lfo(1, "Sine", 4.5, retrig=False),
    mods(("Mod Env", "Morph A", 0.25), ("LFO 1", "Pan", 0.6), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Morph A", 0.35),
         ("Gravity", "Level B", 0.3), ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Reverb", 0.35), ("Aftertouch", "Morph A", 0.2)),
    fx(chorus=(0.6, 0.3, 0.2), reverb=(1.2, 2.0, 0.5, 10, 0.18)), velSens=0.8)

add("Starlight EP", "Keys", "A bell-topped electric piano with a long, gentle decay and a little stereo shimmer.",
    ["electric piano", "bell", "wide"],
    osc("a", "Quasar", morph=0.2, level=0.55), osc("b", "Classic", morph=0.2, level=0.6),
    filt("LP 12", 5000, 0.05, key=0.5, vel=0.4), adsr("amp", 0.002, 3.0, 0.0, 0.6), adsr("menv", 0.001, 0.6, 0.0, 0.3),
    mods(("Mod Env", "Level A", 0.3), ("Velocity", "Morph A", 0.3), ("Heat", "Morph A", 0.4), ("Gravity", "Level B", 0.3),
         ("Orbit", "FX Chorus", 0.4), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "FX Chorus", 0.4), ("Aftertouch", "Cutoff", 0.2)),
    fx(chorus=(0.5, 0.4, 0.25), reverb=(1.4, 2.5, 0.45, 15, 0.2)), velSens=0.8)

add("Telemetry Keys", "Keys", "Data streaming home: a clipped, slightly bit-crushed digital piano.",
    ["digital", "lo-fi", "crunchy"],
    osc("a", "Dark Matter", morph=0.55, level=0.7), osc("b", "Classic", morph=0.25, level=0.5),
    filt("LP 12", 5500, 0.1, key=0.5, vel=0.3), adsr("amp", 0.001, 1.5, 0.2, 0.3),
    mods(("Velocity", "Morph A", 0.2), ("Heat", "FX Drive", 0.5), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.35),
         ("Orbit", "Morph A", 0.35), ("Flux", "FX Delay", 0.35), ("Mod Wheel", "FX Drive", 0.5), ("Aftertouch", "Cutoff", 0.2)),
    fx(drive=("Cosmic Ray", 0.25, 0.6, 0.6), delay=("1/8.", 0.3, 0.5, 0.1, True), reverb=(1.0, 1.5, 0.5, 5, 0.12)), velSens=0.8)

add("Clavis Orbita", "Keys", "A funky clav in orbit: a narrow pulse through a velocity-bright band-pass.",
    ["clav", "funk", "percussive"],
    osc("a", "Classic", morph=0.95, level=0.8), osc("b", "Classic", morph=0.85, level=0.4, fine=8),
    filt("BP 12", 1600, 0.4, env=24, key=0.6, vel=0.4), adsr("amp", 0.001, 0.5, 0.25, 0.08), adsr("fenv", 0.001, 0.15, 0.0, 0.1),
    mods(("Velocity", "Cutoff", 0.2), ("Heat", "Cutoff", 0.3), ("Heat", "Resonance", 0.25), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Morph A", -0.3), ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.2, 0.6, 1.0), chorus=(1.2, 0.2, 0.0), reverb=(0.8, 1.0, 0.5, 5, 0.08)), velSens=0.85)

add("Observatory Organ", "Keys", "Full registration under a dome: big, reverberant, with slow Leslie motion.",
    ["organ", "church", "big"],
    osc("a", "Ion Drive", morph=0.9, level=0.75, phaseRand=0.0), osc("b", "Ion Drive", morph=0.6, oct=1, level=0.3),
    filt("LP 12", 8000, 0.0, key=0.0), adsr("amp", 0.02, 0.3, 1.0, 0.8), lfo(1, "Sine", 0.8, retrig=False),
    {"velSens": 0.0},
    mods(("LFO 1", "Pan", 0.25), ("Heat", "Morph A", 0.1), ("Heat", "FX Drive", 0.35), ("Gravity", "Sub Level", 0.5),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "LFO 1 Rate", 0.6), ("Aftertouch", "Level B", 0.3)),
    fx(drive=("Tube", 0.1, 0.5, 1.0), chorus=(0.6, 0.3, 0.2), reverb=(2.6, 5.0, 0.5, 30, 0.35)))

add("Glass Console", "Keys", "A crystalline keyboard: quantized phase on a sine gives a soft digital sparkle.",
    ["crystal", "digital", "soft"],
    osc("a", "Classic", morph=0.1, level=0.75, warp="Quantize", warpAmt=0.75), osc("b", "Quasar", morph=0.3, level=0.35, oct=1),
    filt("LP 12", 7000, 0.05, key=0.5, vel=0.3), adsr("amp", 0.002, 1.8, 0.2, 0.5),
    mods(("Velocity", "Warp A", 0.15), ("Heat", "Warp A", 0.25), ("Gravity", "Sub Level", 0.35), ("Orbit", "Morph B", 0.4),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Warp A", 0.2), ("Aftertouch", "Level B", 0.3), ("Flux", "FX Reverb", 0.25)),
    fx(delay=("1/8", 0.3, 0.6, 0.12, True), reverb=(1.4, 2.4, 0.4, 15, 0.2)))

add("Mission Control", "Keys", "Tight, punchy poly keys for chords: square and saw, short filter envelope.",
    ["poly", "chords", "punchy"],
    osc("a", "Classic", morph=0.6, unison=2, detune=0.12, width=0.5), osc("b", "Classic", morph=0.75, level=0.45, oct=-1),
    filt("LP 24", 1500, 0.2, env=30, key=0.5, vel=0.3), adsr("amp", 0.002, 0.9, 0.4, 0.25), adsr("fenv", 0.001, 0.4, 0.15, 0.25),
    mods(("Velocity", "Cutoff", 0.2), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Cutoff", 0.35), ("Aftertouch", "Resonance", 0.3), ("Flux", "FX Chorus", 0.3)),
    fx(chorus=(0.5, 0.35, 0.2), reverb=(1.2, 1.8, 0.45, 10, 0.15)))

# =====================================================================================================
# PLUCK
# =====================================================================================================
add("Pulsar Pluck", "Pluck", "Short pulsar bursts: a dry, rhythmic pluck that loves a dotted echo.",
    ["pluck", "rhythmic", "dry"],
    osc("a", "Pulsar", morph=0.55, level=0.8), osc("b", "Classic", morph=0.0, level=0.3),
    filt("LP 24", 1800, 0.25, env=36, key=0.5, vel=0.3), adsr("amp", 0.001, 0.35, 0.0, 0.25), adsr("fenv", 0.001, 0.2, 0.0, 0.2),
    mods(("Velocity", "Morph A", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.35),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.2)),
    fx(delay=("1/8.", 0.35, 0.5, 0.2, True), reverb=(1.2, 1.8, 0.4, 10, 0.15)))

add("Quantum Pluck", "Pluck", "Superposed states: two detuned plucks that collapse into one as the note decays.",
    ["pluck", "detuned", "wide"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.3, width=1.0), osc("b", "Fraunhofer", morph=0.4, level=0.4, oct=1),
    filt("LP 24", 1200, 0.3, env=42, key=0.5, vel=0.3), adsr("amp", 0.001, 0.6, 0.0, 0.4), adsr("fenv", 0.001, 0.3, 0.0, 0.3),
    adsr("menv", 0.001, 0.5, 0.0, 0.3),
    mods(("Mod Env", "Detune", 0.35), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph B", 0.5),
         ("Flux", "FX Delay", 0.35), ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Cutoff", 0.35), ("Aftertouch", "Resonance", 0.3)),
    fx(delay=("1/4", 0.3, 0.5, 0.15, True), reverb=(1.4, 2.2, 0.4, 15, 0.2)))

add("Kuiper Pluck", "Pluck", "Icy bodies at the edge: a glassy FM pluck with a bright, short ring, scattered in stereo.",
    ["fm", "glass", "bright"],
    osc("a", "Magnetar", morph=0.6, level=0.7, unison=2, detune=0.05), osc("b", "Classic", morph=0.0, level=0.35, oct=-1),
    filt("LP 12", 9000, 0.05, key=0.4, vel=0.3), adsr("amp", 0.001, 0.7, 0.0, 0.4), adsr("menv", 0.001, 0.25, 0.0, 0.2),
    mods(("Mod Env", "Morph A", 0.35), ("Random", "Pan", 0.3), ("Heat", "Morph A", 0.3), ("Gravity", "Level B", 0.4),
         ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Morph A", 0.3), ("Aftertouch", "Level B", 0.3)),
    fx(delay=("1/8.", 0.3, 0.55, 0.15, True), reverb=(1.6, 2.8, 0.35, 20, 0.25)))

add("Satellite Ping", "Pluck", "A beacon in low orbit: a sine ping with a short pitch drop and a long echo.",
    ["ping", "sonar", "echo"],
    osc("a", "Classic", morph=0.0, level=0.85, unison=2, detune=0.0), osc("b", "Classic", on=False),
    filt("LP 12", 9000, 0.0, key=0.0), adsr("amp", 0.001, 0.5, 0.0, 0.5), adsr("menv", 0.001, 0.05, 0.0, 0.05),
    mods(("Mod Env", "Pitch", 0.05), ("Heat", "Morph A", 0.3), ("Gravity", "Sub Level", 0.5), ("Random", "Pan", 0.5),
         ("Flux", "FX Feedback", 0.4), ("Orbit", "Detune", 0.35), ("Mod Wheel", "Morph A", 0.25), ("Aftertouch", "FX Delay", 0.3)),
    fx(delay=("1/4.", 0.5, 0.6, 0.35, True), reverb=(1.8, 3.0, 0.4, 20, 0.25)))

add("Micrometeorite", "Pluck", "Tiny, fast impacts: a noise transient exciting a short, metallic comb pluck.",
    ["percussive", "metallic", "comb"],
    osc("a", "Classic", morph=0.5, level=0.5), osc("b", "Classic", on=False),
    filt("Comb", 440, 0.75, key=1.0), adsr("amp", 0.001, 0.4, 0.0, 0.3), adsr("menv", 0.001, 0.03, 0.0, 0.03),
    {"noiseLevel": 0.15, "noiseType": "White"},
    mods(("Mod Env", "Noise Level", 0.4), ("Random", "Cutoff", 0.02), ("Heat", "Resonance", 0.2), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Cutoff", 0.06), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Resonance", 0.2), ("Aftertouch", "Level A", 0.3)),
    fx(reverb=(1.2, 1.6, 0.4, 5, 0.18)))

add("Cassini Harp", "Pluck", "Rings of Saturn plucked like a harp: a soft triangle with a long, singing decay.",
    ["harp", "soft", "gentle"],
    osc("a", "Classic", morph=0.25, level=0.8), osc("b", "Classic", morph=0.0, oct=1, level=0.25),
    filt("LP 12", 2800, 0.1, env=24, key=0.6, vel=0.3), adsr("amp", 0.001, 2.5, 0.0, 1.2), adsr("fenv", 0.001, 0.6, 0.0, 0.5),
    mods(("Velocity", "Cutoff", 0.2), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.3), ("Orbit", "FX Chorus", 0.4),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Cutoff", 0.2), ("Flux", "FX Delay", 0.25)),
    fx(chorus=(0.3, 0.4, 0.2), delay=("1/4", 0.25, 0.6, 0.1, True), reverb=(2.0, 3.5, 0.4, 25, 0.3)))

add("Spectral Pick", "Pluck", "A guitar-like pick with the Fraunhofer lines sliding; aftertouch bends the string.",
    ["guitar", "twang", "spectral"],
    osc("a", "Fraunhofer", morph=0.7, level=0.8), osc("b", "Plasma", morph=0.15, level=0.35),
    filt("LP 24", 2200, 0.2, env=30, key=0.5, vel=0.3), adsr("amp", 0.001, 0.9, 0.15, 0.3), adsr("fenv", 0.001, 0.35, 0.1, 0.3),
    adsr("menv", 0.001, 0.6, 0.0, 0.3),
    mods(("Mod Env", "Morph A", -0.5), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph B", 0.4),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Pitch", 0.04), ("Flux", "FX Reverb", 0.25)),
    fx(drive=("Tube", 0.15, 0.55, 1.0), delay=("1/8.", 0.3, 0.5, 0.12, True), reverb=(1.2, 1.8, 0.45, 10, 0.15)))

add("Starfield Pluck", "Pluck", "Hundreds of points of light: a wide supersaw pluck for melodic house.",
    ["supersaw", "house", "wide"],
    osc("a", "Classic", morph=0.5, unison=7, detune=0.3, width=1.0), osc("b", "Classic", morph=0.75, level=0.35, oct=1),
    filt("LP 24", 1400, 0.25, env=40, key=0.5, vel=0.3), adsr("amp", 0.001, 0.5, 0.0, 0.35), adsr("fenv", 0.001, 0.28, 0.0, 0.3),
    mods(("Velocity", "Cutoff", 0.2), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Delay", 0.35), ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8.", 0.35, 0.5, 0.18, True), reverb=(1.6, 2.4, 0.4, 15, 0.22)))

# =====================================================================================================
# BELL
# =====================================================================================================
add("Quasar Bells", "Bell", "A quasar's beacon: sparse bell partials, long and clear, panned by velocity.",
    ["bell", "clear", "long"],
    osc("a", "Quasar", morph=0.55, level=0.75), osc("b", "Classic", morph=0.0, oct=2, level=0.2),
    filt("LP 12", 9000, 0.0, key=0.3), adsr("amp", 0.001, 4.0, 0.0, 2.0), adsr("menv", 0.001, 1.5, 0.0, 1.0),
    mods(("Mod Env", "Morph A", 0.3), ("Orbit", "Morph A", 0.25), ("Heat", "Morph A", 0.35), ("Gravity", "Sub Level", 0.3),
         ("Orbit", "Pan", 0.0), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Morph A", 0.35), ("Aftertouch", "Level B", 0.3)),
    {"m5Src": "Random", "m5Dst": "Pan", "m5Amt": 0.35},
    fx(delay=("1/4.", 0.35, 0.6, 0.15, True), reverb=(2.4, 5.0, 0.35, 30, 0.35)))

add("Europa Ice", "Bell", "Ice cracking over a hidden ocean: an FM bell with a cold metallic edge.",
    ["fm", "metallic", "cold"],
    osc("a", "Magnetar", morph=0.75, level=0.7), osc("b", "Classic", morph=0.0, semi=7, oct=1, level=0.25),
    filt("LP 12", 8000, 0.05, key=0.3, vel=0.3), adsr("amp", 0.001, 3.0, 0.0, 1.5), adsr("menv", 0.001, 0.9, 0.0, 0.5),
    mods(("Mod Env", "Morph A", 0.25), ("Heat", "Morph A", 0.25), ("Gravity", "Sub Level", 0.3), ("Orbit", "Pitch", 0.0),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Morph A", 0.2), ("Orbit", "Pitch B", 0.01)),
    {"m4Src": "Random", "m4Dst": "Pan", "m4Amt": 0.3},
    fx(chorus=(0.3, 0.3, 0.15), reverb=(2.4, 6.0, 0.3, 40, 0.38)))

add("Titan Chimes", "Bell", "Wind chimes on a hazy moon: random detuned bells with slow ring.",
    ["chimes", "random", "ambient"],
    osc("a", "Quasar", morph=0.8, level=0.65), osc("b", "Quasar", morph=0.3, level=0.4, semi=12, fine=12),
    filt("LP 12", 7500, 0.05, key=0.3), adsr("amp", 0.001, 5.0, 0.0, 3.0),
    mods(("Random", "Pitch", 0.002), ("Random", "Pan", 0.6), ("Heat", "Morph A", 0.2), ("Gravity", "Sub Level", 0.25),
         ("Orbit", "Morph B", 0.4), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Morph A", 0.2), ("Aftertouch", "Level B", 0.3)),
    fx(delay=("1/2", 0.4, 0.6, 0.15, True), reverb=(2.8, 8.0, 0.35, 40, 0.42)))

add("Bell Of Andromeda", "Bell", "A grand, slow-struck bell with a hum tone underneath and a long bloom of reverb.",
    ["bell", "grand", "cinematic"],
    osc("a", "Quasar", morph=0.4, level=0.7), osc("b", "Magnetar", morph=0.5, level=0.35, oct=-1),
    filt("LP 12", 6000, 0.05, key=0.3), adsr("amp", 0.003, 6.0, 0.0, 4.0), adsr("menv", 0.001, 3.0, 0.0, 2.0),
    {"subLevel": 0.2},
    mods(("Mod Env", "Morph B", 0.3), ("Heat", "Morph A", 0.35), ("Gravity", "Sub Level", 0.35), ("Orbit", "Morph A", 0.3),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Morph B", 0.35), ("Aftertouch", "Level B", 0.3), ("Velocity", "Morph A", 0.15)),
    fx(reverb=(3.0, 10.0, 0.35, 50, 0.4)))

add("Ringworld", "Bell", "Ring modulation around a vast circle: clangorous and alien, tuned to a minor seventh.",
    ["ring mod", "alien", "metallic"],
    osc("a", "Classic", morph=0.0, level=0.75, warp="Ring by B", warpAmt=0.9), osc("b", "Classic", morph=0.0, semi=10, oct=1, level=0.0),
    filt("LP 12", 9000, 0.0, key=0.3), adsr("amp", 0.001, 2.5, 0.0, 1.5), adsr("menv", 0.001, 1.2, 0.0, 1.0),
    mods(("Mod Env", "Pitch B", 0.02), ("Heat", "Morph B", 0.5), ("Gravity", "Sub Level", 0.3), ("Orbit", "Pitch B", 0.02),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Warp A", -0.5), ("Aftertouch", "Morph B", 0.3), ("Velocity", "Morph B", 0.2)),
    fx(delay=("1/4.", 0.3, 0.6, 0.12, True), reverb=(2.0, 4.0, 0.35, 25, 0.3)))

add("Celestial Sphere", "Bell", "Music of the spheres: soft sine bells a fifth apart, slowly turning in stereo.",
    ["soft", "sine", "stereo"],
    osc("a", "Classic", morph=0.0, level=0.6, pan=-0.3), osc("b", "Classic", morph=0.0, semi=7, oct=1, level=0.35, pan=0.3),
    filt("LP 12", 8000, 0.0, key=0.0), adsr("amp", 0.003, 3.0, 0.0, 2.5), lfo(1, "Sine", 0.25, retrig=False),
    mods(("LFO 1", "Pan", 0.4), ("Heat", "Morph A", 0.3), ("Gravity", "Sub Level", 0.3), ("Orbit", "LFO 1 Rate", 0.5),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Morph B", 0.4), ("Aftertouch", "Level B", 0.3), ("Velocity", "Level B", 0.2)),
    fx(chorus=(0.2, 0.4, 0.2), delay=("1/2", 0.35, 0.6, 0.12, True), reverb=(2.6, 7.0, 0.35, 30, 0.4)))

add("Radio Beacon", "Bell", "A distress beacon: square bell tone, bit-crushed and repeating into the void.",
    ["beacon", "lo-fi", "echo"],
    osc("a", "Classic", morph=0.75, level=0.6), osc("b", "Quasar", morph=0.5, level=0.4, oct=1),
    filt("BP 12", 2400, 0.3, key=0.5), adsr("amp", 0.001, 1.2, 0.0, 0.6),
    mods(("Velocity", "Cutoff", 0.15), ("Heat", "FX Drive", 0.5), ("Gravity", "Sub Level", 0.3), ("Orbit", "Cutoff", 0.1),
         ("Flux", "FX Feedback", 0.4), ("Flux", "FX Delay", 0.3), ("Mod Wheel", "FX Drive", 0.4), ("Aftertouch", "Resonance", 0.3)),
    fx(drive=("Cosmic Ray", 0.35, 0.5, 0.7), delay=("1/4.", 0.5, 0.5, 0.3, True), reverb=(2.0, 3.5, 0.5, 20, 0.25)))

add("Dyson Bells", "Bell", "A shell of mirrors around a star: bright bells smeared into a shimmering wall.",
    ["shimmer", "wall", "bright"],
    osc("a", "Quasar", morph=0.65, unison=3, detune=0.1, width=0.9), osc("b", "Fraunhofer", morph=0.8, level=0.3, oct=1),
    filt("LP 12", 7500, 0.05, key=0.3), adsr("amp", 0.002, 4.0, 0.2, 3.0),
    mods(("Velocity", "Morph A", 0.2), ("Heat", "Morph B", 0.4), ("Gravity", "Sub Level", 0.3), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Reverb", 0.35), ("Flux", "FX Delay", 0.25), ("Mod Wheel", "FX Chorus", 0.5), ("Aftertouch", "Level B", 0.3)),
    fx(chorus=(0.3, 0.6, 0.3), delay=("1/8.", 0.45, 0.6, 0.2, True), reverb=(2.8, 9.0, 0.35, 40, 0.45)))

# =====================================================================================================
# BRASS & POLY
# =====================================================================================================
add("Supergiant Brass", "Brass & Poly", "Big analog brass: saws with a swelling filter and a hint of growl.",
    ["brass", "analog", "big"],
    osc("a", "Classic", morph=0.5, unison=3, detune=0.18, width=0.6), osc("b", "Classic", morph=0.5, level=0.5, fine=-8, unison=2),
    filt("LP 24", 900, 0.15, env=30, key=0.5, vel=0.25, drive=0.15), adsr("amp", 0.04, 0.5, 0.85, 0.35), adsr("fenv", 0.07, 0.6, 0.45, 0.35),
    lfo(1, "Sine", 5.0, fade=0.8),
    mods(("LFO 1", "Pitch", 0.002), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Cutoff", 0.3), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Cutoff", 0.15)),
    fx(chorus=(0.4, 0.3, 0.2), reverb=(1.6, 2.4, 0.45, 15, 0.2)))

add("Mothership", "Brass & Poly", "Vast and descending: a detuned poly with a slow downward pitch at the start of each chord.",
    ["poly", "cinematic", "huge"],
    osc("a", "Classic", morph=0.5, unison=6, detune=0.3, width=0.9), osc("b", "Plasma", morph=0.3, unison=3, detune=0.25, level=0.4, oct=-1),
    filt("LP 24", 1600, 0.2, env=20, key=0.4), adsr("amp", 0.3, 1.5, 0.85, 1.8), adsr("fenv", 0.6, 1.5, 0.5, 1.5),
    adsr("menv", 0.001, 1.2, 0.0, 0.5),
    mods(("Mod Env", "Pitch", 0.04), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.45), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Morph B", 0.5), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.1)),
    fx(chorus=(0.3, 0.5, 0.3), reverb=(2.4, 4.5, 0.45, 25, 0.3)))

add("Saturn V", "Brass & Poly", "Five engines lighting: a heavy, driven brass stab with the low end in front.",
    ["stab", "driven", "powerful"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.22, width=0.6), osc("b", "Classic", morph=0.75, oct=-1, level=0.5),
    filt("LP 24", 800, 0.2, env=40, key=0.4, drive=0.35), adsr("amp", 0.005, 0.5, 0.5, 0.3), adsr("fenv", 0.005, 0.35, 0.2, 0.3),
    mods(("Velocity", "Cutoff", 0.2), ("Heat", "Filter Drive", 0.5), ("Heat", "Cutoff", 0.25), ("Gravity", "Level B", 0.4),
         ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Tube", 0.3, 0.55, 1.0), reverb=(1.4, 1.8, 0.5, 10, 0.15)))

add("Corona Poly", "Brass & Poly", "Classic poly synth chords with a halo: the Bend warp adds a bright corona.",
    ["poly", "chords", "bright"],
    osc("a", "Classic", morph=0.5, unison=2, detune=0.15, width=0.6, warp="Bend", warpAmt=0.3), osc("b", "Classic", morph=0.7, level=0.4, fine=7),
    filt("LP 24", 2400, 0.2, env=20, key=0.5, vel=0.25), adsr("amp", 0.01, 1.0, 0.7, 0.6), adsr("fenv", 0.01, 0.8, 0.4, 0.5),
    mods(("Velocity", "Warp A", 0.2), ("Heat", "Warp A", 0.5), ("Heat", "Cutoff", 0.2), ("Gravity", "Sub Level", 0.35),
         ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Cutoff", 0.35), ("Aftertouch", "Warp A", 0.25)),
    fx(chorus=(0.45, 0.4, 0.3), reverb=(1.6, 2.4, 0.45, 15, 0.22)))

add("Hyperdrive Stab", "Brass & Poly", "Jumping to lightspeed: a short, bright chord stab with a pitch zip.",
    ["stab", "house", "zip"],
    osc("a", "Classic", morph=0.5, unison=5, detune=0.28, width=0.9), osc("b", "Classic", morph=0.75, level=0.4, oct=1),
    filt("LP 24", 1800, 0.3, env=36, key=0.5, vel=0.3), adsr("amp", 0.001, 0.35, 0.0, 0.3), adsr("fenv", 0.001, 0.22, 0.0, 0.2),
    adsr("menv", 0.001, 0.06, 0.0, 0.05),
    mods(("Mod Env", "Pitch", 0.05), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Delay", 0.35), ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8.", 0.3, 0.5, 0.15, True), reverb=(1.6, 2.2, 0.45, 10, 0.2)))

add("Starfleet Horns", "Brass & Poly", "Heroic, warm horns: a soft-attack brass section for themes.",
    ["horns", "heroic", "orchestral"],
    osc("a", "Classic", morph=0.45, unison=4, detune=0.16, width=0.7), osc("b", "Nebula", morph=0.1, level=0.35),
    filt("LP 24", 1100, 0.12, env=24, key=0.5, vel=0.35), adsr("amp", 0.08, 0.7, 0.85, 0.5), adsr("fenv", 0.12, 0.8, 0.5, 0.5),
    lfo(1, "Sine", 4.8, fade=1.0),
    mods(("LFO 1", "Pitch", 0.002), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.35), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Cutoff", 0.3), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Cutoff", 0.15)),
    fx(chorus=(0.3, 0.3, 0.2), reverb=(2.0, 3.0, 0.45, 25, 0.28)))

add("Orbital Stack", "Brass & Poly", "A layered poly: saw, square and fifth stacked, with chord motion on Orbit.",
    ["layered", "poly", "rich"],
    osc("a", "Classic", morph=0.55, unison=3, detune=0.2, width=0.7), osc("b", "Classic", morph=0.75, semi=7, level=0.35, unison=2),
    filt("LP 24", 2000, 0.2, env=18, key=0.5), adsr("amp", 0.02, 1.0, 0.75, 0.6), adsr("fenv", 0.02, 0.8, 0.4, 0.5),
    lfo(1, "Triangle", 0.25, retrig=False),
    mods(("LFO 1", "Morph A", 0.1), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph B", 0.4),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(chorus=(0.4, 0.4, 0.25), reverb=(1.8, 2.8, 0.45, 15, 0.22)))

add("Warp Core", "Brass & Poly", "Reactor hum turned into chords: FM by oscillator B with a pulsing amp.",
    ["fm", "pulsing", "sci-fi"],
    osc("a", "Classic", morph=0.3, level=0.75, warp="FM from B", warpAmt=0.35, unison=2, detune=0.1), osc("b", "Classic", morph=0.0, level=0.0, oct=1),
    filt("LP 24", 3000, 0.2, key=0.4), adsr("amp", 0.05, 1.0, 0.8, 0.8),
    lfo(1, "Sine", sync=True, div="1/8", retrig=True),
    mods(("LFO 1", "Amp", -0.35), ("Heat", "Warp A", 0.45), ("Gravity", "Sub Level", 0.4), ("Orbit", "LFO 1 Rate", 0.3),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Warp A", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Warp A", 0.15)),
    fx(chorus=(0.4, 0.3, 0.2), reverb=(1.6, 2.4, 0.45, 15, 0.22)))

# =====================================================================================================
# ARP
# =====================================================================================================
add("Orbit Arp", "Arp", "The ORBIT mode: notes circling inward and back out, with the filter breathing per step.",
    ["arp", "orbit", "signature"],
    osc("a", "Classic", morph=0.55, unison=2, detune=0.12, width=0.6), osc("b", "Fraunhofer", morph=0.4, level=0.3, oct=1),
    filt("LP 24", 1500, 0.3, env=30, key=0.5), adsr("amp", 0.002, 0.3, 0.3, 0.2), adsr("fenv", 0.001, 0.2, 0.0, 0.15),
    arp("Orbit", "1/16", octaves=2, gate=0.5),
    mods(("Arp Step", "Cutoff", 0.15), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph B", 0.5),
         ("Flux", "FX Delay", 0.4), ("Flux", "FX Reverb", 0.25), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8.", 0.4, 0.5, 0.2, True), reverb=(1.6, 2.4, 0.45, 15, 0.2)))

add("Binary Stars", "Arp", "Two stars in each other's pull: Pulsar mode pedals the root under every note.",
    ["arp", "pedal", "pulsar"],
    osc("a", "Pulsar", morph=0.45, level=0.8), osc("b", "Classic", morph=0.25, level=0.35, oct=-1),
    filt("LP 24", 2200, 0.25, env=24, key=0.5), adsr("amp", 0.001, 0.25, 0.2, 0.15), adsr("fenv", 0.001, 0.15, 0.0, 0.12),
    arp("Pulsar", "1/16", octaves=1, gate=0.45, swing=0.15),
    mods(("Arp Step", "Morph A", 0.2), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.4),
         ("Flux", "FX Delay", 0.4), ("Flux", "FX Reverb", 0.25), ("Mod Wheel", "Morph A", 0.3), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8.", 0.35, 0.5, 0.18, True), reverb=(1.4, 2.0, 0.45, 10, 0.18)))

add("Gravitational Waves", "Arp", "Ripples in spacetime: an up/down arp whose rate and filter swell on a slow LFO.",
    ["arp", "swell", "moving"],
    osc("a", "Classic", morph=0.5, unison=3, detune=0.2, width=0.8), osc("b", "Classic", morph=0.75, level=0.3, oct=1),
    filt("LP 24", 1200, 0.35, env=26, key=0.5), adsr("amp", 0.002, 0.25, 0.3, 0.2), adsr("fenv", 0.001, 0.18, 0.0, 0.15),
    lfo(1, "Sine", sync=True, div="2 bars", retrig=False),
    arp("Up/Down", "1/16", octaves=3, gate=0.5),
    mods(("LFO 1", "Cutoff", 0.15), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "LFO 1 Rate", 0.5),
         ("Flux", "FX Delay", 0.4), ("Flux", "FX Reverb", 0.25), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8.", 0.4, 0.5, 0.2, True), reverb=(1.6, 2.6, 0.45, 15, 0.22)))

add("Kepler Orbits", "Arp", "Elliptical paths: triplet arp with accents falling on the planets' perihelia.",
    ["arp", "triplet", "accent"],
    osc("a", "Classic", morph=0.62, level=0.8), osc("b", "Quasar", morph=0.3, level=0.3, oct=1),
    filt("LP 24", 1800, 0.3, env=26, key=0.5), adsr("amp", 0.001, 0.3, 0.25, 0.2), adsr("fenv", 0.001, 0.18, 0.0, 0.15),
    arp("Up", "1/8T", octaves=2, gate=0.55, length=12,
        lane=[(1.0, 0, 1), (0.5, 0, 1), (0.6, 0, 1), (0.9, 0, 1), (0.5, 0, 1), (0.6, 0, 1), (1.0, 0, 1), (0.5, 0, 1), (0.6, 0, 2),
              (0.9, 0, 1), (0.5, 0, 1), (0.6, 12, 1)]),
    mods(("Arp Step", "Cutoff", 0.25), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph B", 0.5),
         ("Flux", "FX Delay", 0.4), ("Flux", "FX Reverb", 0.25), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/4T", 0.35, 0.5, 0.18, True), reverb=(1.6, 2.4, 0.45, 15, 0.2)))

add("Astro Clock", "Arp", "Precise, ticking, mechanical: a dry square arp with ratchets on the off-beats.",
    ["arp", "ratchet", "mechanical"],
    osc("a", "Classic", morph=0.75, level=0.8), osc("b", "Classic", on=False),
    filt("LP 24", 2600, 0.2, env=18, key=0.5), adsr("amp", 0.001, 0.15, 0.15, 0.08), adsr("fenv", 0.001, 0.1, 0.0, 0.08),
    arp("Up", "1/16", octaves=2, gate=0.4,
        lane=[(1.0, 0, 1), (0.6, 0, 2), (0.8, 0, 1), (0.6, 0, 3), (1.0, 0, 1), (0.6, 0, 2), (0.8, 0, 1), (0.6, 0, 1),
              (1.0, 0, 1), (0.6, 0, 2), (0.8, 0, 1), (0.6, 0, 4), (1.0, 0, 1), (0.6, 0, 2), (0.8, 0, 1), (0.6, 0, 1)]),
    mods(("Arp Step", "Cutoff", 0.2), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph A", -0.4),
         ("Flux", "FX Delay", 0.35), ("Flux", "FX Reverb", 0.25), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/16.", 0.3, 0.5, 0.1, True), reverb=(1.0, 1.2, 0.5, 5, 0.1)))

add("Satellite Swarm", "Arp", "Many small craft crossing the sky: random order, random pan, sparse probability.",
    ["arp", "random", "generative"],
    osc("a", "Quasar", morph=0.5, level=0.7), osc("b", "Classic", morph=0.0, oct=1, level=0.3),
    filt("LP 12", 6000, 0.05, key=0.4), adsr("amp", 0.001, 0.6, 0.0, 0.5),
    arp("Random", "1/16", octaves=3, gate=0.4, prob=0.6),
    mods(("Random", "Pan", 0.7), ("Random", "Morph A", 0.3), ("Heat", "Morph A", 0.3), ("Gravity", "Sub Level", 0.3),
         ("Orbit", "Morph B", 0.4), ("Flux", "FX Delay", 0.35), ("Mod Wheel", "FX Reverb", 0.4), ("Aftertouch", "Level B", 0.3)),
    fx(delay=("1/8.", 0.45, 0.55, 0.25, True), reverb=(2.2, 4.0, 0.4, 25, 0.3)))

add("Pulsar Timing", "Arp", "A millisecond pulsar: a rapid, pulsing chord arp locked to 1/32 with a gating rhythm.",
    ["arp", "fast", "chord"],
    osc("a", "Pulsar", morph=0.6, level=0.75), osc("b", "Classic", morph=0.5, level=0.3, oct=-1),
    filt("LP 24", 2400, 0.25, env=18, key=0.5), adsr("amp", 0.001, 0.08, 0.2, 0.05), adsr("fenv", 0.001, 0.07, 0.0, 0.05),
    arp("Chord", "1/32", octaves=2, gate=0.5, length=16,
        lane=[(1.0, 0, 1), (0.0, 0, 1), (0.7, 0, 1), (0.0, 0, 1), (0.9, 0, 1), (0.6, 0, 1), (0.0, 0, 1), (0.7, 0, 1),
              (1.0, 0, 1), (0.0, 0, 1), (0.6, 0, 1), (0.6, 0, 1), (0.9, 0, 1), (0.0, 0, 1), (0.7, 0, 1), (0.0, 0, 1)]),
    mods(("Arp Step", "Morph A", 0.2), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.35),
         ("Flux", "FX Delay", 0.35), ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8", 0.3, 0.5, 0.12, True), reverb=(1.6, 2.2, 0.45, 10, 0.2)))

add("Ion Cascade", "Arp", "Charged particles falling down the field: a descending arp with transposed echoes.",
    ["arp", "descending", "cascade"],
    osc("a", "Fraunhofer", morph=0.5, level=0.75, unison=2, detune=0.12), osc("b", "Classic", morph=0.0, level=0.3, oct=1),
    filt("LP 24", 2400, 0.25, env=20, key=0.5), adsr("amp", 0.001, 0.4, 0.2, 0.3), adsr("fenv", 0.001, 0.2, 0.0, 0.2),
    arp("Down", "1/16", octaves=3, gate=0.5, length=8,
        lane=[(1.0, 0, 1), (0.7, 0, 1), (0.8, 0, 1), (0.6, -12, 1), (0.9, 0, 1), (0.7, 0, 1), (0.8, 7, 1), (0.6, 0, 1)]),
    mods(("Arp Step", "Cutoff", 0.15), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph A", 0.4),
         ("Flux", "FX Delay", 0.4), ("Flux", "FX Feedback", 0.25), ("Mod Wheel", "Resonance", 0.3), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8.", 0.45, 0.5, 0.22, True), reverb=(1.6, 2.6, 0.45, 15, 0.22)))

add("Galaxy Spin", "Arp", "A spiral galaxy turning: Converge mode with a slowly opening, rotating stereo field.",
    ["arp", "converge", "stereo"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.2, width=1.0), osc("b", "Nebula", morph=0.4, level=0.3),
    filt("LP 24", 1600, 0.3, env=28, key=0.5), adsr("amp", 0.001, 0.3, 0.25, 0.25), adsr("fenv", 0.001, 0.2, 0.0, 0.15),
    lfo(1, "Sine", sync=True, div="1 bar", retrig=False),
    arp("Converge", "1/16", octaves=2, gate=0.5, swing=0.1),
    mods(("LFO 1", "Pan", 0.5), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Delay", 0.35), ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8.", 0.4, 0.5, 0.2, True), reverb=(1.8, 2.8, 0.45, 15, 0.25)))

add("Event Horizon Arp", "Arp", "Falling in: a diverging arp whose folds harden as notes approach the edge.",
    ["arp", "diverge", "folded"],
    osc("a", "Event Horizon", morph=0.3, level=0.75), osc("b", "Classic", morph=0.0, level=0.35, oct=-1),
    filt("LP 24", 2000, 0.3, env=20, key=0.5), adsr("amp", 0.001, 0.3, 0.2, 0.2), adsr("fenv", 0.001, 0.2, 0.0, 0.15),
    arp("Diverge", "1/16", octaves=2, gate=0.5),
    mods(("Key", "Morph A", 0.4), ("Heat", "Morph A", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Cutoff", 0.1),
         ("Flux", "FX Delay", 0.4), ("Flux", "FX Reverb", 0.25), ("Mod Wheel", "Morph A", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8.", 0.35, 0.5, 0.18, True), reverb=(1.4, 2.0, 0.45, 10, 0.18)))

# =====================================================================================================
# SEQUENCE
# =====================================================================================================
add("Mission Sequence", "Sequence", "Hold one note: a full 16-step bassline plays itself, transposed by the lane.",
    ["sequence", "bassline", "one-finger"],
    osc("a", "Classic", morph=0.55, level=0.85), osc("b", "Classic", morph=0.75, level=0.3, oct=-1),
    filt("LP 24", 600, 0.5, env=38, key=0.4, drive=0.2), adsr("amp", 0.001, 0.25, 0.5, 0.08), adsr("fenv", 0.001, 0.16, 0.0, 0.1),
    {"voiceMode": "Mono"},
    arp("Up", "1/16", gate=0.6, swing=0.12,
        lane=[(1.0, 0, 1), (0.0, 0, 1), (0.6, 12, 1), (0.7, 0, 1), (0.0, 0, 1), (0.8, 3, 1), (0.6, 0, 1), (1.0, 7, 1),
              (0.0, 0, 1), (0.7, 0, 1), (0.6, 12, 1), (0.0, 0, 1), (0.9, 5, 1), (0.6, 0, 2), (0.0, 0, 1), (0.8, -2, 1)]),
    mods(("Arp Step", "Cutoff", 0.25), ("Heat", "Cutoff", 0.3), ("Heat", "Resonance", 0.15), ("Gravity", "Level B", 0.45),
         ("Orbit", "Morph A", 0.3), ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Filter Drive", 0.5), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Tube", 0.2, 0.55, 1.0), delay=("1/8.", 0.3, 0.6, 0.0, True), reverb=()))

add("Countdown", "Sequence", "T-minus: a descending sequence in eighth-note triplets that lands on the root.",
    ["sequence", "descending", "tension"],
    osc("a", "Plasma", morph=0.25, level=0.75), osc("b", "Classic", morph=0.5, level=0.35, oct=-1),
    filt("LP 24", 1800, 0.3, env=24, key=0.5), adsr("amp", 0.001, 0.25, 0.3, 0.15), adsr("fenv", 0.001, 0.18, 0.0, 0.15),
    arp("Up", "1/8T", gate=0.5, length=12,
        lane=[(1.0, 12, 1), (0.6, 10, 1), (0.7, 7, 1), (0.9, 5, 1), (0.6, 3, 1), (0.7, 0, 1), (1.0, 10, 1), (0.6, 7, 1),
              (0.7, 5, 1), (0.9, 3, 1), (0.6, 2, 1), (1.0, 0, 2)]),
    mods(("Arp Step", "Morph A", 0.25), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.35),
         ("Flux", "FX Delay", 0.35), ("Flux", "FX Reverb", 0.25), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/4T", 0.3, 0.5, 0.15, True), reverb=(1.4, 2.0, 0.45, 10, 0.18)))

add("Telemetry Bus", "Sequence", "Data on the line: a gated, bit-crushed digital sequence with probability dropouts.",
    ["sequence", "glitch", "digital"],
    osc("a", "Dark Matter", morph=0.4, level=0.75), osc("b", "Classic", morph=0.75, level=0.3),
    filt("BP 12", 1800, 0.4, env=20, key=0.6), adsr("amp", 0.001, 0.12, 0.2, 0.05), adsr("fenv", 0.001, 0.1, 0.0, 0.05),
    arp("As Played", "1/16", gate=0.4, prob=0.82,
        lane=[(1.0, 0, 1), (0.7, 12, 1), (0.8, 0, 2), (0.6, 7, 1), (1.0, 0, 1), (0.7, 0, 3), (0.8, 12, 1), (0.6, 0, 1),
              (1.0, 5, 1), (0.7, 0, 1), (0.8, 0, 2), (0.6, 12, 1), (1.0, 0, 1), (0.7, 7, 1), (0.8, 0, 1), (0.6, 0, 4)]),
    mods(("Arp Step", "Morph A", 0.3), ("Heat", "FX Drive", 0.5), ("Gravity", "Sub Level", 0.4), ("Orbit", "Cutoff", 0.15),
         ("Flux", "FX Delay", 0.35), ("Flux", "FX Reverb", 0.2), ("Mod Wheel", "FX Drive", 0.4), ("Aftertouch", "Resonance", 0.3)),
    fx(drive=("Cosmic Ray", 0.3, 0.6, 0.7), delay=("1/16.", 0.3, 0.5, 0.12, True), reverb=(1.2, 1.5, 0.5, 5, 0.12)))

add("Deep Space Network", "Sequence", "Dishes listening across the dark: a slow, wide, echoing chord sequence.",
    ["sequence", "chords", "ambient"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.22, width=1.0), osc("b", "Quasar", morph=0.4, level=0.3),
    filt("LP 24", 1400, 0.25, env=20, key=0.4), adsr("amp", 0.01, 0.8, 0.3, 0.6), adsr("fenv", 0.01, 0.5, 0.1, 0.4),
    arp("Chord", "1/8", octaves=1, gate=0.7, length=8,
        lane=[(1.0, 0, 1), (0.0, 0, 1), (0.6, 0, 1), (0.8, 5, 1), (0.0, 0, 1), (0.7, 0, 1), (0.6, 7, 1), (0.0, 0, 1)]),
    mods(("Arp Step", "Cutoff", 0.15), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Delay", 0.35), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Morph B", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/4.", 0.5, 0.55, 0.3, True), reverb=(2.4, 5.0, 0.45, 25, 0.35)))

add("Launch Window", "Sequence", "Rising energy before ignition: an up-arp sequence that climbs octaves over a bar.",
    ["sequence", "rising", "build"],
    osc("a", "Classic", morph=0.5, unison=3, detune=0.2, width=0.8), osc("b", "Plasma", morph=0.3, level=0.3),
    filt("LP 24", 1000, 0.35, env=30, key=0.5), adsr("amp", 0.001, 0.25, 0.3, 0.15), adsr("fenv", 0.001, 0.15, 0.0, 0.12),
    arp("Up", "1/16", octaves=4, gate=0.5),
    mods(("Key", "Cutoff", 0.2), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph B", 0.5),
         ("Flux", "FX Delay", 0.35), ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8.", 0.35, 0.5, 0.18, True), reverb=(1.6, 2.4, 0.45, 15, 0.22)))

add("Morse From Mars", "Sequence", "A message in code: a sine beep sequence of dots and dashes.",
    ["sequence", "morse", "beeps"],
    osc("a", "Classic", morph=0.0, level=0.8, unison=2, detune=0.0), osc("b", "Classic", on=False),
    filt("LP 12", 9000, 0.0, key=0.0), adsr("amp", 0.002, 0.1, 1.0, 0.01),
    arp("Up", "1/16", gate=0.9, length=16,
        lane=[(1.0, 0, 1), (0.0, 0, 1), (1.0, 0, 1), (0.0, 0, 1), (1.0, 0, 1), (1.0, 0, 1), (1.0, 0, 1), (0.0, 0, 1),
              (1.0, 0, 1), (0.0, 0, 1), (0.0, 0, 1), (1.0, 0, 1), (1.0, 0, 1), (1.0, 0, 1), (0.0, 0, 1), (0.0, 0, 1)]),
    mods(("Heat", "Morph A", 0.5), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.4), ("Flux", "FX Delay", 0.35),
         ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "FX Drive", 0.4), ("Random", "Pan", 0.3)),
    fx(drive=("Cosmic Ray", 0.15, 0.5, 0.5), delay=("1/8.", 0.4, 0.6, 0.2, True), reverb=(1.6, 2.4, 0.45, 15, 0.2)))

add("Retrograde", "Sequence", "Apparent backwards motion: a Down/Up pattern with the swing pulled late.",
    ["sequence", "swing", "groove"],
    osc("a", "Classic", morph=0.62, level=0.8), osc("b", "Fraunhofer", morph=0.5, level=0.3, oct=1),
    filt("LP 24", 1800, 0.3, env=24, key=0.5), adsr("amp", 0.001, 0.22, 0.3, 0.12), adsr("fenv", 0.001, 0.15, 0.0, 0.1),
    arp("Down/Up", "1/16", octaves=2, gate=0.5, swing=0.45),
    mods(("Arp Step", "Cutoff", 0.15), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph B", 0.4),
         ("Flux", "FX Delay", 0.35), ("Flux", "FX Reverb", 0.25), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8.", 0.3, 0.5, 0.15, True), reverb=(1.4, 1.8, 0.45, 10, 0.15)))

add("Cosmic Ray Funk", "Sequence", "Funk from orbit: a bit-crushed clav sequence with ghost notes and pushes.",
    ["sequence", "funk", "crushed"],
    osc("a", "Classic", morph=0.92, level=0.8), osc("b", "Classic", morph=0.6, level=0.3, oct=-1),
    filt("BP 12", 1400, 0.35, env=26, key=0.6, vel=0.3), adsr("amp", 0.001, 0.2, 0.2, 0.06), adsr("fenv", 0.001, 0.1, 0.0, 0.08),
    arp("As Played", "1/16", gate=0.4, swing=0.3,
        lane=[(1.0, 0, 1), (0.3, 0, 1), (0.0, 0, 1), (0.8, 12, 1), (0.3, 0, 1), (1.0, 0, 1), (0.0, 0, 1), (0.6, 7, 1),
              (0.3, 0, 1), (1.0, 0, 1), (0.3, 0, 1), (0.0, 0, 1), (0.9, 10, 1), (0.3, 0, 1), (0.7, 0, 1), (0.3, 0, 2)]),
    mods(("Arp Step", "Cutoff", 0.25), ("Heat", "FX Drive", 0.4), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", -0.3),
         ("Flux", "FX Delay", 0.3), ("Flux", "FX Chorus", 0.3), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Cosmic Ray", 0.2, 0.6, 0.5), delay=("1/16.", 0.25, 0.5, 0.08, True), reverb=(1.0, 1.2, 0.5, 5, 0.1)))

# =====================================================================================================
# DRONE
# =====================================================================================================
add("Dyson Sphere", "Drone", "Energy harvested from an entire star: a dense, endless, slowly turning drone.",
    ["drone", "dense", "endless"],
    osc("a", "Classic", morph=0.5, unison=8, detune=0.25, width=1.0, oct=-1), osc("b", "Fraunhofer", morph=0.3, unison=4, detune=0.2, level=0.5),
    filt("LP 24", 900, 0.3, key=0.2), adsr("amp", 3.0, 2.0, 1.0, 6.0), lfo(1, "Sine", 0.03, retrig=False), lfo(2, "Drift", 0.1, retrig=False),
    {"subLevel": 0.3},
    mods(("LFO 1", "Morph B", 0.8), ("LFO 2", "Cutoff", 0.12), ("Heat", "Cutoff", 0.4), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "LFO 1 Rate", 0.6), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Detune", 0.3)),
    fx(chorus=(0.15, 0.6, 0.35), reverb=(3.0, 12.0, 0.45, 60, 0.45), width=1.3))

add("Interstellar Medium", "Drone", "The thin gas between stars: filtered solar-wind noise with a faint harmonic glow.",
    ["drone", "noise", "thin"],
    osc("a", "Nebula", morph=0.3, level=0.35, unison=4, detune=0.12, width=1.0), osc("b", "Classic", on=False),
    filt("BP 12", 900, 0.6, key=0.5), adsr("amp", 4.0, 2.0, 1.0, 6.0), lfo(1, "Drift", 0.08, retrig=False),
    {"noiseLevel": 0.7, "noiseType": "Solar Wind"},
    mods(("LFO 1", "Cutoff", 0.2), ("LFO 1", "Morph A", 0.4), ("Heat", "Resonance", 0.3), ("Gravity", "Cutoff", -0.2),
         ("Orbit", "LFO 1 Rate", 0.6), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Level A", 0.5), ("Aftertouch", "Resonance", 0.25)),
    fx(reverb=(3.0, 14.0, 0.4, 80, 0.5), width=1.5))

add("Golden Record", "Drone", "A greeting sent outward: a warm, organ-like drone with a slow vowel morph.",
    ["drone", "warm", "vowel"],
    osc("a", "Ion Drive", morph=0.6, unison=3, detune=0.1, width=0.8), osc("b", "Nebula", morph=0.0, level=0.45, unison=2),
    filt("LP 12", 2400, 0.2, key=0.3), adsr("amp", 3.0, 2.0, 1.0, 6.0), lfo(1, "Triangle", 0.04, retrig=False),
    mods(("LFO 1", "Morph B", 1.0), ("Heat", "Morph A", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "LFO 1 Rate", 0.6),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Level B", 0.3), ("Heat", "Cutoff", 0.2)),
    fx(chorus=(0.2, 0.5, 0.35), reverb=(2.6, 9.0, 0.45, 40, 0.42)))

add("Singularity", "Drone", "Everything pulled into one point: a low, folding drone that grinds as Heat rises.",
    ["drone", "dark", "grinding"],
    osc("a", "Event Horizon", morph=0.2, unison=3, detune=0.15, width=0.8, oct=-1), osc("b", "Classic", morph=0.0, oct=-2, level=0.5),
    filt("LP 24", 600, 0.4, key=0.2, drive=0.2), adsr("amp", 2.0, 2.0, 1.0, 5.0), lfo(1, "Sine", 0.05, retrig=False),
    mods(("LFO 1", "Morph A", 0.3), ("Heat", "Morph A", 0.5), ("Heat", "FX Drive", 0.4), ("Gravity", "Level B", 0.4),
         ("Orbit", "LFO 1 Rate", 0.6), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Resonance", 0.3)),
    fx(drive=("Tube", 0.2, 0.4, 1.0), reverb=(2.8, 10.0, 0.6, 40, 0.4)))

add("Solar Wind Drone", "Drone", "Particles streaming off the sun: a moving comb drone fed by wind noise.",
    ["drone", "comb", "wind"],
    osc("a", "Classic", morph=0.5, level=0.3), osc("b", "Classic", on=False),
    filt("Comb", 110, 0.85, key=1.0), adsr("amp", 3.0, 2.0, 1.0, 5.0), lfo(1, "Drift", 0.12, retrig=False),
    {"noiseLevel": 0.6, "noiseType": "Solar Wind"},
    mods(("LFO 1", "Resonance", 0.1), ("Heat", "Level A", 0.4), ("Gravity", "Sub Level", 0.4), ("Orbit", "LFO 1 Rate", 0.5),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Noise Level", 0.3), ("Aftertouch", "Resonance", 0.12)),
    fx(chorus=(0.15, 0.5, 0.3), reverb=(2.8, 10.0, 0.4, 50, 0.42), width=1.3))

add("Black Body", "Drone", "Perfect thermal radiation: a pure, glowing, almost static sine cluster.",
    ["drone", "pure", "minimal"],
    osc("a", "Classic", morph=0.0, unison=5, detune=0.08, width=1.0), osc("b", "Classic", morph=0.15, semi=7, level=0.35, unison=3, detune=0.06),
    filt("LP 12", 5000, 0.0, key=0.0), adsr("amp", 4.0, 2.0, 1.0, 6.0), lfo(1, "Sine", 0.06, retrig=False),
    mods(("LFO 1", "Level B", 0.2), ("Heat", "Morph A", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Morph B", 0.4), ("Aftertouch", "Level B", 0.3), ("Orbit", "LFO 1 Rate", 0.4)),
    fx(chorus=(0.1, 0.4, 0.25), reverb=(3.0, 12.0, 0.35, 60, 0.45)))

add("Cold Dark Matter", "Drone", "Unseen mass holding everything together: sub-heavy, dark, shifting spectra.",
    ["drone", "sub", "dark"],
    osc("a", "Dark Matter", morph=0.5, unison=4, detune=0.12, width=0.9, oct=-1), osc("b", "Classic", morph=0.0, oct=-2, level=0.45),
    filt("LP 24", 500, 0.35, key=0.3), adsr("amp", 2.5, 2.0, 1.0, 5.0), lfo(1, "Drift", 0.06, retrig=False),
    mods(("LFO 1", "Morph A", 0.5), ("Heat", "Cutoff", 0.45), ("Gravity", "Level B", 0.4), ("Orbit", "LFO 1 Rate", 0.6),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Resonance", 0.3), ("Orbit", "Detune", 0.3)),
    fx(reverb=(2.8, 10.0, 0.6, 40, 0.4)))

add("Heat Death", "Drone", "The last warmth of the universe fading: a slowly closing filter that never quite stops.",
    ["drone", "fading", "melancholy"],
    osc("a", "Classic", morph=0.5, unison=6, detune=0.2, width=1.0), osc("b", "Quasar", morph=0.2, level=0.3),
    filt("LP 24", 2400, 0.2, key=0.3), adsr("amp", 2.0, 3.0, 1.0, 8.0), adsr("menv", 0.5, 20.0, 0.0, 8.0),
    mods(("Mod Env", "Cutoff", 0.25), ("Heat", "Cutoff", 0.4), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Morph B", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.1)),
    fx(chorus=(0.12, 0.5, 0.3), reverb=(3.0, 15.0, 0.45, 60, 0.45)))

# =====================================================================================================
# TEXTURE
# =====================================================================================================
add("Radio Telescope", "Texture", "Listening to the sky: crackling static and slow tuned whistles drifting by.",
    ["texture", "static", "radio"],
    osc("a", "Classic", morph=0.0, level=0.35), osc("b", "Classic", on=False),
    filt("BP 12", 1400, 0.65, key=1.0), adsr("amp", 1.0, 2.0, 1.0, 3.0), lfo(1, "Drift", 0.15, retrig=False), lfo(2, "Sine", 0.05, retrig=False),
    {"noiseLevel": 0.8, "noiseType": "Crackle"},
    mods(("LFO 1", "Pitch", 0.15), ("LFO 2", "Cutoff", 0.3), ("Heat", "Resonance", 0.25), ("Gravity", "Sub Level", 0.3),
         ("Orbit", "LFO 1 Rate", 0.6), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Level A", 0.5), ("Aftertouch", "Noise Level", 0.3)),
    fx(drive=("Cosmic Ray", 0.2, 0.6, 0.5), delay=("1/4.", 0.4, 0.6, 0.15, True), reverb=(2.4, 6.0, 0.4, 30, 0.35)))

add("Magnetosphere", "Texture", "The field that shields us: slowly ringing comb resonances in a shifting stereo field.",
    ["texture", "resonant", "stereo"],
    osc("a", "Dark Matter", morph=0.6, level=0.4, unison=4, detune=0.2, width=1.0), osc("b", "Classic", on=False),
    filt("Comb", 300, 0.8, key=0.8), adsr("amp", 2.0, 2.0, 1.0, 4.0), lfo(1, "Sine", 0.07, retrig=False), lfo(2, "Drift", 0.2, retrig=False),
    {"noiseLevel": 0.2, "noiseType": "Pink"},
    mods(("LFO 1", "Cutoff", 0.1), ("LFO 2", "Morph A", 0.5), ("Heat", "Resonance", 0.15), ("Gravity", "Cutoff", -0.15),
         ("Orbit", "LFO 1 Rate", 0.6), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Resonance", 0.12)),
    fx(chorus=(0.2, 0.6, 0.3), reverb=(2.6, 8.0, 0.4, 40, 0.4), width=1.4))

add("Crackle Of Jupiter", "Texture", "Jupiter's radio storms: crackle noise through a sweeping formant filter.",
    ["texture", "crackle", "storm"],
    osc("a", "Pulsar", morph=0.8, level=0.25), osc("b", "Classic", on=False),
    filt("Formant", 800, 0.6, key=0.0), adsr("amp", 0.5, 2.0, 1.0, 3.0), lfo(1, "Triangle", 0.12, retrig=False),
    {"noiseLevel": 0.9, "noiseType": "Crackle"},
    mods(("LFO 1", "Cutoff", 0.4), ("Heat", "Resonance", 0.3), ("Gravity", "Sub Level", 0.3), ("Orbit", "LFO 1 Rate", 0.6),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Level A", 0.5), ("Aftertouch", "Cutoff", 0.3), ("Heat", "FX Drive", 0.3)),
    fx(delay=("1/8.", 0.4, 0.6, 0.15, True), reverb=(2.4, 5.0, 0.4, 25, 0.35)))

add("Ion Storm", "Texture", "Charged clouds: S&H-modulated noise bursts and pitch, stepping in time.",
    ["texture", "s&h", "rhythmic"],
    osc("a", "Plasma", morph=0.5, level=0.45), osc("b", "Classic", on=False),
    filt("LP 24", 1800, 0.55, key=0.5), adsr("amp", 0.01, 1.0, 1.0, 1.0),
    lfo(1, "S&H", sync=True, div="1/16", retrig=False), lfo(2, "S&H", sync=True, div="1/8", retrig=False),
    {"noiseLevel": 0.4, "noiseType": "White"},
    mods(("LFO 1", "Cutoff", 0.25), ("LFO 2", "Pitch", 0.1), ("Heat", "Resonance", 0.25), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8.", 0.4, 0.5, 0.2, True), reverb=(1.8, 3.0, 0.45, 15, 0.25)))

add("Spectrograph", "Texture", "Splitting light into lines: the Fraunhofer table scanned end to end like a readout.",
    ["texture", "scanning", "spectral"],
    osc("a", "Fraunhofer", morph=0.0, unison=3, detune=0.1, width=0.8), osc("b", "Classic", morph=0.0, level=0.3, oct=-1),
    filt("LP 12", 9000, 0.1, key=0.3), adsr("amp", 0.5, 2.0, 1.0, 2.5), lfo(1, "Saw Up", 0.08, retrig=True),
    mods(("LFO 1", "Morph A", 0.5), ("Heat", "Cutoff", 0.2), ("Gravity", "Level B", 0.4), ("Orbit", "LFO 1 Rate", 0.6),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Detune", 0.4), ("Aftertouch", "Morph A", 0.3), ("Flux", "FX Chorus", 0.3)),
    fx(chorus=(0.3, 0.4, 0.2), reverb=(2.2, 4.0, 0.4, 20, 0.3)))

add("Sunspot Cycle", "Texture", "Eleven years in a few bars: dark spots of filtering that come and go.",
    ["texture", "slow", "filter"],
    osc("a", "Classic", morph=0.5, unison=5, detune=0.2, width=0.9), osc("b", "Event Horizon", morph=0.4, level=0.35),
    filt("Notch", 1200, 0.4, key=0.3), adsr("amp", 1.0, 2.0, 1.0, 3.0), lfo(1, "Triangle", 0.09, retrig=False), lfo(2, "Drift", 0.2, retrig=False),
    mods(("LFO 1", "Cutoff", 0.4), ("LFO 2", "Resonance", 0.2), ("Heat", "Morph B", 0.4), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "LFO 1 Rate", 0.6), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Detune", 0.3)),
    fx(chorus=(0.25, 0.5, 0.3), reverb=(2.4, 5.0, 0.45, 25, 0.35)))

add("Sputnik", "Texture", "The first signal from orbit: a lonely beep cycling through the static.",
    ["texture", "beep", "retro"],
    osc("a", "Classic", morph=0.7, level=0.5), osc("b", "Classic", on=False),
    filt("BP 12", 1500, 0.3, key=0.5), adsr("amp", 0.01, 1.0, 1.0, 0.5), lfo(1, "Square", 1.2, retrig=True),
    {"noiseLevel": 0.25, "noiseType": "Crackle"},
    mods(("LFO 1", "Level A", 0.5), ("Heat", "FX Drive", 0.4), ("Gravity", "Sub Level", 0.3), ("Orbit", "LFO 1 Rate", 0.5),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "LFO 1 Rate", 0.5), ("Aftertouch", "Noise Level", 0.3), ("Flux", "FX Reverb", 0.25)),
    fx(drive=("Cosmic Ray", 0.25, 0.5, 0.6), delay=("1/4.", 0.45, 0.6, 0.2, True), reverb=(2.0, 3.0, 0.45, 20, 0.25)))

add("Planetarium", "Texture", "Lights dimming, stars appearing one by one: a sparse, twinkling bed.",
    ["texture", "twinkle", "sparse"],
    osc("a", "Quasar", morph=0.7, level=0.5, unison=2, detune=0.1), osc("b", "Classic", morph=0.0, oct=2, level=0.2),
    filt("LP 12", 8000, 0.05, key=0.3), adsr("amp", 0.5, 2.0, 1.0, 3.0),
    lfo(1, "S&H", 6.0, retrig=False), lfo(2, "Sine", 0.1, retrig=False),
    mods(("LFO 1", "Level B", 0.3), ("LFO 1", "Pan", 0.4), ("Aftertouch", "Morph A", 0.3), ("Heat", "Morph A", 0.25),
         ("Gravity", "Sub Level", 0.3), ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Level B", 0.4)),
    fx(delay=("1/8.", 0.45, 0.6, 0.2, True), reverb=(2.6, 7.0, 0.35, 40, 0.42)))

# =====================================================================================================
# FX
# =====================================================================================================
add("Re-Entry", "FX", "Burning through the atmosphere: a falling, distorted noise roar on each held note.",
    ["fx", "falling", "noise"],
    osc("a", "Plasma", morph=0.7, level=0.5, unison=3, detune=0.1), osc("b", "Classic", on=False),
    filt("LP 24", 6000, 0.45, env=-60, key=0.0, drive=0.4), adsr("amp", 0.05, 3.0, 0.6, 1.5), adsr("fenv", 0.01, 4.0, 0.0, 1.5),
    adsr("menv", 0.01, 4.0, 0.0, 1.5),
    {"noiseLevel": 0.6, "noiseType": "White"},
    mods(("Mod Env", "Pitch", 0.5), ("Heat", "FX Drive", 0.5), ("Gravity", "Pitch", -0.2), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Noise Level", 0.3), ("Random", "Pan", 0.3)),
    fx(drive=("Fold", 0.3, 0.6, 0.6), reverb=(2.4, 4.0, 0.45, 20, 0.3)))

add("Liftoff", "FX", "Ignition and climb: noise and saw rising together over four seconds.",
    ["fx", "riser", "build"],
    osc("a", "Classic", morph=0.5, unison=6, detune=0.3, width=1.0), osc("b", "Classic", on=False),
    filt("LP 24", 300, 0.4, env=72, key=0.0), adsr("amp", 3.0, 1.0, 1.0, 1.0), adsr("fenv", 4.0, 1.0, 1.0, 1.0),
    adsr("menv", 4.0, 1.0, 1.0, 1.0),
    {"noiseLevel": 0.5, "noiseType": "White"},
    mods(("Mod Env", "Pitch", 0.5), ("Mod Env", "Detune", 0.4), ("Heat", "Resonance", 0.3), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(delay=("1/8.", 0.4, 0.5, 0.15, True), reverb=(2.4, 5.0, 0.4, 20, 0.35)))

add("Supernova Riser", "FX", "A star collapsing then exploding: a swelling, chaotic riser that ends in a flash.",
    ["fx", "riser", "explosion"],
    osc("a", "Event Horizon", morph=0.0, unison=4, detune=0.3, width=1.0), osc("b", "Magnetar", morph=0.0, level=0.5),
    filt("LP 24", 400, 0.3, env=70, key=0.0, drive=0.3), adsr("amp", 4.0, 1.0, 1.0, 2.5), adsr("fenv", 5.0, 1.0, 1.0, 2.0),
    adsr("menv", 5.0, 1.0, 1.0, 2.0),
    {"noiseLevel": 0.3},
    mods(("Mod Env", "Morph A", 1.0), ("Aftertouch", "Morph B", 0.5), ("Mod Env", "Pitch", 0.25), ("Heat", "FX Drive", 0.5),
         ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Noise Level", 0.5)),
    fx(drive=("Soft", 0.2, 0.6, 1.0), reverb=(3.0, 8.0, 0.4, 30, 0.4)))

add("Wormhole Transit", "FX", "Folding space: a flanging, pitch-bending tunnel of comb-filtered noise.",
    ["fx", "tunnel", "sweep"],
    osc("a", "Classic", morph=0.5, level=0.3, unison=4, detune=0.4, width=1.0), osc("b", "Classic", on=False),
    filt("Comb", 200, 0.85, key=0.5), adsr("amp", 0.5, 2.0, 1.0, 2.0), lfo(1, "Triangle", 0.25, retrig=True),
    {"noiseLevel": 0.6, "noiseType": "Pink"},
    mods(("LFO 1", "Cutoff", 0.4), ("Heat", "Resonance", 0.12), ("Gravity", "Sub Level", 0.4), ("Orbit", "LFO 1 Rate", 0.6),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Pitch", 0.2), ("Aftertouch", "Detune", 0.3), ("Flux", "FX Delay", 0.3)),
    fx(chorus=(0.3, 0.7, 0.3), delay=("1/8.", 0.5, 0.5, 0.15, True), reverb=(2.4, 5.0, 0.4, 25, 0.35), width=1.4))

add("Alien Signal", "FX", "Something answering back: a modulated FM chatter that never repeats the same way.",
    ["fx", "alien", "chatter"],
    osc("a", "Classic", morph=0.0, level=0.6, warp="FM from B", warpAmt=0.6), osc("b", "Classic", morph=0.5, level=0.0, semi=5),
    filt("BP 12", 1800, 0.4, key=0.5), adsr("amp", 0.01, 1.0, 1.0, 1.0),
    lfo(1, "S&H", 9.0, retrig=False), lfo(2, "Drift", 1.5, retrig=False),
    mods(("LFO 1", "Pitch B", 0.3), ("LFO 2", "Warp A", 0.3), ("Aftertouch", "Cutoff", 0.3), ("Heat", "Warp A", 0.3),
         ("Gravity", "Sub Level", 0.3), ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "LFO 1 Rate", 0.5)),
    fx(delay=("1/8.", 0.45, 0.5, 0.2, True), reverb=(2.0, 3.0, 0.45, 20, 0.25)))

add("Laser Grid", "FX", "Targeting lasers: a fast falling zap on every note, sharp and dry.",
    ["fx", "zap", "laser"],
    osc("a", "Plasma", morph=0.6, level=0.7), osc("b", "Classic", on=False),
    filt("LP 12", 9000, 0.2, key=0.0), adsr("amp", 0.001, 0.25, 0.0, 0.15), adsr("menv", 0.001, 0.2, 0.0, 0.15),
    mods(("Mod Env", "Pitch", 1.0), ("Mod Env", "Morph A", 0.4), ("Heat", "FX Drive", 0.4), ("Gravity", "Pitch", -0.5),
         ("Orbit", "Morph A", 0.4), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.3), ("Aftertouch", "Cutoff", 0.3)),
    fx(delay=("1/16.", 0.4, 0.5, 0.15, True), reverb=(1.4, 1.5, 0.45, 5, 0.15)))

add("Meteor Shower", "FX", "Streaks across the sky: random pitched noise bursts in a long stereo echo.",
    ["fx", "random", "bursts"],
    osc("a", "Classic", morph=0.0, level=0.4), osc("b", "Classic", on=False),
    filt("BP 12", 2400, 0.5, key=0.5), adsr("amp", 0.001, 0.5, 0.0, 0.4), adsr("menv", 0.001, 0.5, 0.0, 0.4),
    {"noiseLevel": 0.7, "noiseType": "White"},
    arp("Random", "1/16", octaves=4, gate=0.4, prob=0.45),
    mods(("Mod Env", "Pitch", -0.3), ("Random", "Pan", 0.8), ("Aftertouch", "Resonance", 0.3), ("Heat", "Resonance", 0.3),
         ("Gravity", "Sub Level", 0.3), ("Orbit", "Cutoff", 0.15), ("Flux", "FX Feedback", 0.3), ("Mod Wheel", "FX Reverb", 0.4)),
    fx(delay=("1/8.", 0.5, 0.5, 0.3, True), reverb=(2.4, 4.0, 0.4, 20, 0.35)))

add("Gamma Burst", "FX", "The most violent flash in the universe: a short, extreme folded impact.",
    ["fx", "impact", "hit"],
    osc("a", "Event Horizon", morph=1.0, level=0.8, oct=-1), osc("b", "Magnetar", morph=1.0, level=0.5, oct=-2),
    filt("LP 24", 300, 0.3, env=72, key=0.0, drive=0.5), adsr("amp", 0.001, 1.5, 0.0, 1.5), adsr("fenv", 0.001, 0.6, 0.0, 0.6),
    adsr("menv", 0.001, 0.3, 0.0, 0.3),
    {"noiseLevel": 0.5, "subLevel": 0.6},
    mods(("Mod Env", "Pitch", 0.3), ("Heat", "FX Drive", 0.5), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Cutoff", 0.15)),
    fx(drive=("Tube", 0.3, 0.6, 1.0), reverb=(3.0, 6.0, 0.5, 10, 0.35)))

add("Tractor Beam", "FX", "Caught and pulled in: a wobbling, rising hum locked to the tempo.",
    ["fx", "wobble", "sci-fi"],
    osc("a", "Magnetar", morph=0.3, level=0.6, unison=3, detune=0.2), osc("b", "Classic", morph=0.0, oct=-1, level=0.4),
    filt("LP 24", 900, 0.5, key=0.3), adsr("amp", 0.3, 1.0, 1.0, 1.0), adsr("menv", 4.0, 1.0, 1.0, 1.0),
    lfo(1, "Sine", sync=True, div="1/16", retrig=True),
    mods(("LFO 1", "Cutoff", 0.25), ("Mod Env", "Pitch", 0.25), ("Aftertouch", "Cutoff", 0.3), ("Heat", "Morph A", 0.4),
         ("Gravity", "Level B", 0.4), ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Resonance", 0.4)),
    fx(reverb=(2.0, 3.0, 0.45, 15, 0.3)))

add("Docking Sequence", "FX", "Clamps engaging: a mechanical, ratcheting servo sound in rhythm.",
    ["fx", "mechanical", "servo"],
    osc("a", "Classic", morph=0.8, level=0.55, warp="Quantize", warpAmt=0.8), osc("b", "Classic", on=False),
    filt("BP 12", 1200, 0.5, env=24, key=0.5), adsr("amp", 0.001, 0.15, 0.0, 0.08), adsr("fenv", 0.001, 0.1, 0.0, 0.08),
    {"noiseLevel": 0.3},
    arp("Up", "1/16", gate=0.3,
        lane=[(1.0, 0, 3), (0.0, 0, 1), (0.7, 12, 1), (0.0, 0, 1), (0.9, 0, 4), (0.0, 0, 1), (0.6, 7, 2), (0.0, 0, 1),
              (1.0, 0, 3), (0.0, 0, 1), (0.7, 12, 1), (0.5, 0, 1), (0.9, -12, 2), (0.0, 0, 1), (0.6, 0, 1), (0.0, 0, 1)]),
    mods(("Arp Step", "Cutoff", 0.3), ("Heat", "Warp A", 0.2), ("Gravity", "Sub Level", 0.4), ("Orbit", "Cutoff", 0.15),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Resonance", 0.3), ("Aftertouch", "Noise Level", 0.3), ("Flux", "FX Reverb", 0.25)),
    fx(drive=("Cosmic Ray", 0.25, 0.6, 0.5), delay=("1/16.", 0.3, 0.5, 0.1, True), reverb=(1.2, 1.4, 0.5, 5, 0.12)))


# =====================================================================================================
# The rest of the library lives in presets_*.py next to this file, written with the same helpers.
# (Check a file on its own with check_presets.py.)
# =====================================================================================================
def _load_more():
    import glob
    for path in sorted(glob.glob(os.path.join(HERE, "presets_*.py"))):
        exec(compile(open(path).read(), path, "exec"), globals())


if __name__ == "__main__":
    _load_more()


TARGET_LOUDNESS = -16.0      # dBFS, 90th percentile of 50 ms RMS windows over the test phrase


def calibrate(measured_path):
    with open(measured_path) as f:
        measured = json.load(f)
    levels = {}
    if os.path.exists(LEVELS):
        with open(LEVELS) as f:
            levels = json.load(f)
    for name, loud in measured.items():
        if name == "Init":
            continue
        base = next((p["params"].get("outDb", 0.0) for p in PRESETS if p["name"] == name), 0.0)
        trim = levels.get(name, 0.0) + (TARGET_LOUDNESS - loud) * 0.9
        levels[name] = round(max(-30.0 - base, min(18.0 - base, trim)), 1)
    with open(LEVELS, "w") as f:
        json.dump(dict(sorted(levels.items())), f, indent=1)
    print(f"updated {len(levels)} trims in {os.path.relpath(LEVELS)}")


def main():
    import sys
    if len(sys.argv) > 2 and sys.argv[1] == "--calibrate":
        calibrate(sys.argv[2])
    names = [p["name"] for p in PRESETS]
    assert len(names) == len(set(names)), "duplicate preset names"
    levels = {}
    if os.path.exists(LEVELS):
        with open(LEVELS) as f:
            levels = json.load(f)
    for p in PRESETS:
        trim = levels.get(p["name"])
        if trim is not None:
            p["params"]["outDb"] = round(max(-30.0, min(18.0, p["params"].get("outDb", 0.0) + trim)), 1)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w") as f:
        json.dump({"version": 1, "presets": PRESETS}, f, indent=1)
    cats = {}
    for p in PRESETS:
        cats[p["category"]] = cats.get(p["category"], 0) + 1
    print(f"{len(PRESETS)} presets -> {os.path.relpath(OUT)}")
    for c, n in cats.items():
        print(f"  {c:14s} {n}")


if __name__ == "__main__":
    main()
