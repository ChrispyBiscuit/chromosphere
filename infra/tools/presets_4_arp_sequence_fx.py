# More factory presets: Arp, Sequence and FX (exec'd by make_presets.py with its helpers).


def _lane(text):
    """Compact step lane: space-separated steps, each 'vel[/transpose][xratchet]', '-' for a rest.
    e.g. "1 .4 .7/12 - 1x2 .5/-12" -> [(1.0, 0, 1), (0.4, 0, 1), (0.7, 12, 1), (0.0, 0, 1), (1.0, 0, 2), (0.5, -12, 1)]"""
    out = []
    for tok in text.split():
        if tok == "-":
            out.append((0.0, 0, 1))
            continue
        rat = 1
        if "x" in tok:
            tok, r = tok.split("x")
            rat = int(r)
        tr = 0
        if "/" in tok:
            tok, t = tok.split("/")
            tr = int(t)
        out.append((float(tok), tr, rat))
    return out


# =====================================================================================================
# ARP
# =====================================================================================================
add("Trance Gate Nova", "Arp", "A supersaw arpeggio in 16ths, gated hard and climbing two octaves: the classic euphoric lift.",
    ["trance", "supersaw", "uplifting"],
    osc("a", "Classic", morph=0.5, unison=6, detune=0.3, width=0.8), osc("b", "Classic", morph=0.5, oct=1, level=0.35, unison=2, detune=0.2),
    filt("LP 24", 3800, 0.25, env=14, key=0.5), adsr("amp", 0.002, 0.25, 0.6, 0.15), adsr("fenv", 0.001, 0.22, 0.2, 0.15),
    arp("Up", "1/16", octaves=2, gate=0.45,
        lane=_lane("1 .55 .8 .55 1 .55 .8/12 .55 1 .55 .8 .55 1/7 .55 .9/12 .6")),
    mods(("Arp Step", "Cutoff", 0.15), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.15)),
    fx(delay=("1/8.", 0.4, 0.5, 0.2, True), reverb=(1.6, 2.8, 0.4, 15, 0.22)))

add("Neon Grid Runner", "Arp", "Synthwave eighths on a bright pulse with a ping-pong echo, cruising down a grid at night.",
    ["synthwave", "retro", "pulse"],
    osc("a", "Classic", morph=0.78, level=0.8, unison=2, detune=0.12), osc("b", "Classic", morph=0.5, fine=8, level=0.4),
    filt("LP 24", 2400, 0.3, env=22, key=0.5), adsr("amp", 0.002, 0.35, 0.5, 0.2), adsr("fenv", 0.001, 0.25, 0.15, 0.2),
    lfo(1, "Triangle", 0.3, retrig=False),
    arp("Up/Down", "1/8", octaves=2, gate=0.6),
    mods(("LFO 1", "Morph A", 0.1), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.35), ("Orbit", "LFO 1 Rate", 0.4),
         ("Flux", "FX Delay", 0.45), ("Mod Wheel", "Morph A", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Chorus", 0.3)),
    fx(chorus=(0.35, 0.45, 0.25), delay=("1/4.", 0.45, 0.45, 0.25, True), reverb=(1.4, 2.0, 0.45, 12, 0.18)))

add("Kepler Cascade", "Arp", "Down-and-up triplets through a glassy Quasar table: water falling between three moons.",
    ["triplet", "glassy", "cascading"],
    osc("a", "Quasar", morph=0.4, level=0.75), osc("b", "Nebula", morph=0.6, oct=1, level=0.3),
    filt("LP 12", 6000, 0.15, env=10, key=0.6), adsr("amp", 0.001, 0.6, 0.25, 0.5), adsr("fenv", 0.001, 0.4, 0.1, 0.4),
    arp("Down/Up", "1/8T", octaves=3, gate=0.5),
    mods(("Velocity", "Morph A", 0.3), ("Heat", "Morph A", 0.4), ("Gravity", "Sub Level", 0.35), ("Orbit", "Morph B", 0.45),
         ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Cutoff", 0.3), ("Aftertouch", "Level B", 0.35), ("Heat", "Cutoff", 0.25)),
    fx(delay=("1/4T", 0.4, 0.55, 0.22, True), reverb=(2.0, 3.6, 0.4, 25, 0.28)))

add("Played Constellation", "Arp", "Plays your chord back exactly as you drew it, star by star, with a halo of echoes.",
    ["as played", "ambient", "sparkle"],
    osc("a", "Pulsar", morph=0.3, level=0.7), osc("b", "Classic", morph=0.2, oct=1, level=0.25),
    filt("LP 24", 5200, 0.2, env=8, key=0.6), adsr("amp", 0.003, 0.8, 0.3, 0.8), adsr("fenv", 0.001, 0.5, 0.2, 0.5),
    lfo(1, "Sine", 5.0),
    arp("As Played", "1/8", octaves=1, gate=0.7, latch=True),
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.35),
         ("Orbit", "Morph A", 0.4), ("Flux", "FX Reverb", 0.5), ("Aftertouch", "Morph A", 0.3), ("Flux", "FX Delay", 0.3)),
    fx(delay=("1/4.", 0.5, 0.5, 0.3, True), reverb=(2.2, 4.5, 0.4, 30, 0.32)))

add("Converging Orbits", "Arp", "Notes from the outside in: the chord folds toward its centre like satellites decaying.",
    ["converge", "hypnotic", "pluck"],
    osc("a", "Fraunhofer", morph=0.2, level=0.8), osc("b", "Classic", morph=0.5, oct=-1, level=0.35),
    filt("LP 24", 1800, 0.35, env=30, key=0.5), adsr("amp", 0.001, 0.4, 0.2, 0.3), adsr("fenv", 0.001, 0.3, 0.05, 0.25),
    adsr("menv", 0.001, 0.5, 0.0, 0.3),
    arp("Converge", "1/16", octaves=2, gate=0.5, swing=0.1),
    mods(("Mod Env", "Morph A", 0.4), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.35),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.2)),
    fx(delay=("1/8.", 0.4, 0.5, 0.2, True), reverb=(1.5, 2.2, 0.45, 10, 0.18)))

add("Diverging Jets", "Arp", "From the middle out: twin jets of a quasar spraying notes to both registers.",
    ["diverge", "wide", "bright"],
    osc("a", "Plasma", morph=0.3, level=0.7, unison=3, detune=0.18, width=0.9), osc("b", "Classic", morph=0.75, oct=1, level=0.3),
    filt("LP 24", 4200, 0.25, env=16, key=0.5), adsr("amp", 0.002, 0.3, 0.45, 0.25), adsr("fenv", 0.001, 0.25, 0.1, 0.2),
    arp("Diverge", "1/16", octaves=3, gate=0.5),
    mods(("Random", "Pan", 0.6), ("Heat", "Morph A", 0.4), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Delay", 0.45), ("Mod Wheel", "Cutoff", 0.3), ("Aftertouch", "Resonance", 0.3), ("Heat", "Cutoff", 0.25)),
    fx(delay=("1/16.", 0.35, 0.5, 0.18, True), reverb=(1.6, 2.4, 0.4, 12, 0.2)))

add("Chord Pulsar", "Arp", "Whole chords fired like a pulsar beam, with off-beat accents and a stutter at the bar's end.",
    ["chord", "house", "stab"],
    osc("a", "Classic", morph=0.5, unison=3, detune=0.2, width=0.6), osc("b", "Nebula", morph=0.4, level=0.4),
    filt("LP 24", 2600, 0.3, env=26, key=0.5), adsr("amp", 0.001, 0.22, 0.0, 0.15), adsr("fenv", 0.001, 0.18, 0.0, 0.12),
    arp("Chord", "1/8", octaves=1, gate=0.4,
        lane=_lane("- 1 - .7 - 1 .5 .8 - 1 - .7 - 1 .9x2 .6x3")),
    mods(("Arp Step", "Cutoff", 0.2), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.2)),
    fx(chorus=(0.3, 0.4, 0.2), delay=("1/8.", 0.3, 0.5, 0.15, True), reverb=(1.3, 1.8, 0.45, 10, 0.2)))

add("Orbital Resonance", "Arp", "The Orbit mode circling the chord in 1/8T while a resonant filter traces its ellipse.",
    ["orbit", "resonant", "moving"],
    osc("a", "Classic", morph=0.5, level=0.8), osc("b", "Classic", morph=0.75, fine=-7, level=0.45),
    filt("LP 24", 1100, 0.65, env=24, key=0.5), adsr("amp", 0.002, 0.35, 0.4, 0.2), adsr("fenv", 0.001, 0.3, 0.1, 0.2),
    lfo(1, "Sine", sync=True, div="2 bars", retrig=False),
    arp("Orbit", "1/8T", octaves=2, gate=0.55),
    mods(("LFO 1", "Cutoff", 0.2), ("Heat", "Resonance", 0.25), ("Gravity", "Level B", 0.35), ("Orbit", "LFO 1 Rate", 0.4),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Filter Drive", 0.5), ("Aftertouch", "Cutoff", 0.25), ("Heat", "Cutoff", 0.3)),
    fx(drive=("Soft", 0.15, 0.6, 1.0), delay=("1/4T", 0.45, 0.5, 0.2, True), reverb=(1.6, 2.6, 0.4, 15, 0.2)))

add("Pulsar Clockwork", "Arp", "Pulsar mode ticking out the chord with machine precision; ratchets mark every fourth beat.",
    ["pulsar", "precise", "techno"],
    osc("a", "Ion Drive", morph=0.3, level=0.75), osc("b", "Classic", morph=0.0, oct=-1, level=0.35),
    filt("LP 12", 1800, 0.4, env=24, key=0.5), adsr("amp", 0.001, 0.3, 0.2, 0.12), adsr("fenv", 0.001, 0.15, 0.0, 0.1),
    arp("Pulsar", "1/16", octaves=2, gate=0.4,
        lane=_lane("1 .5 .6 .5 1x2 .5 .6 .5 1 .5 .6 .5 1x3 .5 .7 .5")),
    mods(("Arp Step", "Cutoff", 0.25), ("Heat", "Morph A", 0.4), ("Gravity", "Level B", 0.4), ("Orbit", "Cutoff", 0.1),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Morph A", 0.3), ("Heat", "Cutoff", 0.2)),
    fx(drive=("Tube", 0.2, 0.55, 1.0), delay=("1/16.", 0.3, 0.6, 0.12, True), reverb=(1.0, 1.2, 0.5, 5, 0.1)))

add("Asteroid Belt", "Arp", "Random notes over four octaves at 60% probability: rocks tumbling through the dark.",
    ["random", "generative", "ambient"],
    osc("a", "Dark Matter", morph=0.4, level=0.7), osc("b", "Quasar", morph=0.6, oct=1, level=0.3),
    filt("LP 12", 4000, 0.2, env=12, key=0.6), adsr("amp", 0.001, 0.9, 0.0, 0.9), adsr("fenv", 0.001, 0.5, 0.0, 0.5),
    arp("Random", "1/16", octaves=4, gate=0.6, prob=0.6, latch=True),
    mods(("Random", "Morph A", 0.35), ("Random", "Pan", 0.7), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.3),
         ("Orbit", "Morph B", 0.4), ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Resonance", 0.3)),
    fx(delay=("1/8.", 0.5, 0.55, 0.3, True), reverb=(2.4, 5.0, 0.4, 30, 0.35)))

add("8-Bit Probe", "Arp", "Chiptune arpeggio on a quantised square: a lander sending home scores in 1/32 notes.",
    ["chiptune", "8-bit", "retro"],
    osc("a", "Classic", morph=0.75, level=0.7, warp="Quantize", warpAmt=0.6), osc("b", "Classic", on=False),
    filt("LP 12", 12000, 0.0, key=0.0), adsr("amp", 0.0, 0.1, 0.8, 0.05),
    lfo(1, "Square", 6.0),
    {"voiceMode": "Mono", "drift": 0.0},
    arp("Up", "1/32", octaves=2, gate=0.9),
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Morph A", -0.3), ("Flux", "FX Delay", 0.4), ("Aftertouch", "Cutoff", -0.3), ("Velocity", "Amp", 0.2)),
    fx(delay=("1/8", 0.3, 0.3, 0.12, False), reverb=()))

add("Swing State", "Arp", "A loose, swung down-arp on Rhodes-like FM, for late-night house and lo-fi grooves.",
    ["swing", "house", "fm"],
    osc("a", "Classic", morph=0.0, level=0.8, warp="FM from B", warpAmt=0.25), osc("b", "Classic", morph=0.0, oct=1, level=0.0),
    filt("LP 12", 3200, 0.1, env=10, key=0.6, vel=0.3), adsr("amp", 0.002, 0.7, 0.2, 0.4), adsr("fenv", 0.001, 0.4, 0.1, 0.3),
    arp("Down", "1/16", octaves=1, gate=0.7, swing=0.45,
        lane=_lane("1 .4 .7 .4 .9 - .7 .5 1 .4 .6 .4 .9 .5 .7/12 .4")),
    mods(("Velocity", "Warp A", 0.25), ("Heat", "Warp A", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Pan", 0.3),
         ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Cutoff", 0.3), ("Aftertouch", "Warp A", 0.25), ("Flux", "FX Reverb", 0.3)),
    fx(chorus=(0.5, 0.4, 0.2), reverb=(1.2, 1.6, 0.5, 8, 0.15)))

add("Hyperion Steps", "Arp", "Big-room trance in triplet 16ths; every fourth note leaps an octave on a bright wavetable sweep.",
    ["trance", "triplet", "big"],
    osc("a", "Ion Drive", morph=0.5, unison=5, detune=0.28, width=0.8), osc("b", "Classic", morph=0.5, oct=-1, level=0.4),
    filt("LP 24", 3000, 0.3, env=18, key=0.5), adsr("amp", 0.001, 0.25, 0.55, 0.2), adsr("fenv", 0.001, 0.2, 0.15, 0.15),
    lfo(1, "Saw Up", sync=True, div="4 bars", retrig=False),
    arp("Up", "1/16T", octaves=1, gate=0.5, length=12,
        lane=_lane("1 .6 .7 1/12 .6 .7 1 .6 .7 1/12 .6 .8")),
    mods(("LFO 1", "Morph A", 0.35), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.35), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.3), ("Aftertouch", "Cutoff", 0.25), ("Arp Step", "Cutoff", 0.12)),
    fx(delay=("1/8T", 0.4, 0.5, 0.2, True), reverb=(1.8, 3.0, 0.4, 15, 0.22)))

add("Lunar Lullaby", "Arp", "A slow, latched up/down arpeggio in quarter notes, soft as moonlight on a cold sea.",
    ["ambient", "soft", "latch"],
    osc("a", "Nebula", morph=0.3, level=0.7), osc("b", "Classic", morph=0.15, oct=1, level=0.25, fine=4),
    filt("LP 12", 2800, 0.1, env=6, key=0.6), adsr("amp", 0.08, 1.2, 0.5, 1.5), adsr("fenv", 0.05, 1.0, 0.3, 1.0),
    lfo(1, "Sine", 4.6, fade=0.8),
    arp("Up/Down", "1/4", octaves=2, gate=1.2, latch=True),
    mods(("LFO 1", "Pitch", 0.008), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.35),
         ("Orbit", "Morph A", 0.4), ("Flux", "FX Reverb", 0.5), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Delay", 0.3)),
    fx(chorus=(0.2, 0.5, 0.25), delay=("1/2", 0.45, 0.6, 0.2, True), reverb=(2.6, 6.0, 0.35, 40, 0.4)))

add("Retro Rocket Lane", "Arp", "Dotted-eighth echo arpeggio, the U2-style guitar delay reborn as a rocket's flashing lights.",
    ["dotted", "echo", "anthemic"],
    osc("a", "Classic", morph=0.62, level=0.8), osc("b", "Fraunhofer", morph=0.3, oct=1, level=0.3),
    filt("LP 24", 3600, 0.2, env=12, key=0.6), adsr("amp", 0.001, 0.3, 0.35, 0.25), adsr("fenv", 0.001, 0.25, 0.1, 0.2),
    arp("Up", "1/8", octaves=1, gate=0.45),
    mods(("Velocity", "Cutoff", 0.2), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.35), ("Orbit", "Morph B", 0.4),
         ("Flux", "FX Feedback", 0.3), ("Mod Wheel", "Morph A", 0.3), ("Aftertouch", "Resonance", 0.3), ("Flux", "FX Delay", 0.35)),
    fx(drive=("Tube", 0.15, 0.6, 0.6), delay=("1/8.", 0.55, 0.45, 0.35, False), reverb=(1.6, 2.5, 0.45, 15, 0.2)))

add("Polymeter Planets", "Arp", "A five-step lane against a four-on-the-floor: planets lapping each other in a 5:4 resonance.",
    ["polyrhythm", "idm", "pluck"],
    osc("a", "Pulsar", morph=0.5, level=0.75), osc("b", "Classic", morph=0.0, oct=-1, level=0.3),
    filt("LP 24", 2000, 0.4, env=28, key=0.5), adsr("amp", 0.001, 0.3, 0.1, 0.25), adsr("fenv", 0.001, 0.22, 0.0, 0.2),
    arp("Up", "1/16", octaves=2, gate=0.5, length=5,
        lane=_lane("1 .5 .8/7 - .6")),
    mods(("Arp Step", "Morph A", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.35),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.2)),
    fx(delay=("1/16.", 0.4, 0.5, 0.2, True), reverb=(1.4, 1.8, 0.45, 10, 0.16)))

# =====================================================================================================
# SEQUENCE
# =====================================================================================================
add("Berlin Ascent", "Sequence", "Berlin-school ostinato: a resonant saw sequence with a slow filter climb, hypnotic for minutes.",
    ["berlin school", "ostinato", "analog"],
    osc("a", "Classic", morph=0.5, level=0.8), osc("b", "Classic", morph=0.5, fine=-5, level=0.5),
    filt("LP 24", 700, 0.55, env=30, key=0.4), adsr("amp", 0.001, 0.25, 0.2, 0.15), adsr("fenv", 0.001, 0.2, 0.0, 0.15),
    lfo(1, "Triangle", sync=True, div="8 bars", retrig=False),
    {"voiceMode": "Mono"},
    arp("Up", "1/16", octaves=1, gate=0.5,
        lane=_lane("1 .6/12 .7/7 .6/12 .9 .6/12 .7/3 .6/12 1 .6/12 .7/7 .6/12 .9/-5 .6/12 .7/3 .6/10")),
    mods(("LFO 1", "Cutoff", 0.3), ("Heat", "Resonance", 0.25), ("Gravity", "Sub Level", 0.45), ("Orbit", "LFO 1 Rate", 0.4),
         ("Flux", "FX Delay", 0.45), ("Mod Wheel", "Filter Drive", 0.5), ("Aftertouch", "Cutoff", 0.25), ("Heat", "Cutoff", 0.25)),
    fx(drive=("Tube", 0.15, 0.55, 1.0), delay=("1/8.", 0.5, 0.55, 0.25, True), reverb=(1.8, 3.0, 0.4, 20, 0.22)))

add("Phobos Patrol", "Sequence", "Dark electro: a syncopated bass sequence with octave jumps and a ratcheted pickup.",
    ["electro", "bass", "syncopated"],
    osc("a", "Magnetar", morph=0.2, level=0.8), osc("b", "Classic", morph=0.75, oct=-1, level=0.4),
    filt("LP 24", 900, 0.4, env=34, key=0.4, drive=0.25), adsr("amp", 0.001, 0.22, 0.3, 0.08), adsr("fenv", 0.001, 0.16, 0.0, 0.1),
    {"voiceMode": "Mono", "glide": 0.02},
    arp("Up", "1/16", octaves=1, gate=0.45,
        lane=_lane("1 - .6 1/12 - .7 - 1 .6/12 - 1 .5 - .8/-12 .7x2 .9x3")),
    mods(("Arp Step", "Cutoff", 0.25), ("Heat", "Morph A", 0.4), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.25),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Filter Drive", 0.5), ("Aftertouch", "Cutoff", 0.25), ("Heat", "Filter Drive", 0.3)),
    fx(drive=("Fold", 0.2, 0.5, 0.6), delay=("1/16.", 0.3, 0.6, 0.12, True), reverb=()))

add("Minimal Transit", "Sequence", "Minimal techno blip: one note, a moving accent and microscopic filter changes per step.",
    ["minimal", "techno", "blip"],
    osc("a", "Classic", morph=0.15, level=0.75), osc("b", "Classic", on=False),
    filt("LP 12", 1600, 0.45, env=24, key=0.6), adsr("amp", 0.001, 0.2, 0.15, 0.08), adsr("fenv", 0.001, 0.1, 0.0, 0.08),
    {"voiceMode": "Mono", "noiseLevel": 0.08},
    arp("Up", "1/16", octaves=1, gate=0.35,
        lane=_lane(".6 - 1 .4 - .6 .8 - .5 - 1 .4 .7 - .9 .5")),
    mods(("Arp Step", "Cutoff", 0.3), ("Heat", "Resonance", 0.3), ("Gravity", "Sub Level", 0.45), ("Orbit", "Cutoff", 0.12),
         ("Flux", "FX Delay", 0.45), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Cutoff", 0.3), ("Heat", "Cutoff", 0.2)),
    fx(delay=("1/8.", 0.5, 0.6, 0.25, True), reverb=(1.2, 1.4, 0.5, 5, 0.12)))

add("Glitch Telemetry", "Sequence", "IDM data stream: ratchets, rests and wild transposes at 70% probability, never quite repeating.",
    ["idm", "glitch", "generative"],
    osc("a", "Event Horizon", morph=0.4, level=0.7), osc("b", "Classic", morph=0.75, oct=1, level=0.3),
    filt("LP 12", 5000, 0.3, env=20, key=0.5), adsr("amp", 0.001, 0.12, 0.0, 0.06), adsr("fenv", 0.001, 0.1, 0.0, 0.06),
    adsr("menv", 0.001, 0.1, 0.0, 0.06),
    arp("Random", "1/16", octaves=2, gate=0.4, prob=0.7,
        lane=_lane("1x2 - .6/7 1x4 - .5/-12 .9 .4x3 - 1/12 .6 - .8x2 .5/5 - 1x3")),
    mods(("Mod Env", "Morph A", 0.4), ("Random", "Cutoff", 0.15), ("Heat", "Morph A", 0.35), ("Gravity", "Sub Level", 0.35),
         ("Orbit", "Pan", 0.6), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Cosmic Ray", 0.25, 0.6, 0.5), delay=("1/32", 0.4, 0.4, 0.15, True), reverb=(1.0, 1.2, 0.5, 5, 0.12)))

add("Synthwave Drive Sequence", "Sequence", "Driving eighth-note bass in octaves under a pink-neon sky, glide on the jumps.",
    ["synthwave", "bass", "octave"],
    osc("a", "Classic", morph=0.5, level=0.8, unison=2, detune=0.1), osc("b", "Classic", morph=0.78, oct=-1, level=0.45),
    filt("LP 24", 1100, 0.3, env=26, key=0.4), adsr("amp", 0.002, 0.3, 0.6, 0.1), adsr("fenv", 0.001, 0.22, 0.1, 0.12),
    {"voiceMode": "Legato", "glide": 0.03},
    arp("Up", "1/8", octaves=1, gate=0.8,
        lane=_lane("1 .7/12 .9 .7/12 1 .7/12 .9 .7/12 1/-2 .7/10 .9/-2 .7/10 1/-4 .7/8 .9/-4 .8/8")),
    mods(("Velocity", "Cutoff", 0.15), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Chorus", 0.45), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.25)),
    fx(chorus=(0.4, 0.4, 0.2), reverb=(1.2, 1.5, 0.5, 8, 0.1)))

add("House Of Titan", "Sequence", "A classic house organ-stab riff with off-beat chords and a ghost note before the one.",
    ["house", "organ", "stab"],
    osc("a", "Ion Drive", morph=0.2, level=0.7), osc("b", "Classic", morph=0.0, oct=1, level=0.4),
    filt("LP 12", 3400, 0.15, env=12, key=0.5), adsr("amp", 0.001, 0.25, 0.0, 0.15), adsr("fenv", 0.001, 0.2, 0.0, 0.12),
    arp("Chord", "1/16", octaves=1, gate=0.5, swing=0.3,
        lane=_lane("- - 1 - - - 1 .4 - - 1 - - .5 1 -")),
    mods(("Velocity", "Cutoff", 0.2), ("Heat", "Morph A", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph B", 0.3),
         ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Cutoff", 0.3), ("Aftertouch", "Level B", 0.3), ("Heat", "Cutoff", 0.25)),
    fx(chorus=(0.6, 0.35, 0.2), reverb=(1.3, 1.6, 0.5, 8, 0.18)))

add("Seven Sisters", "Sequence", "A seven-step polymeter in Pleiades blue: odd-length phrase drifting against the bar.",
    ["polymeter", "melodic", "bell"],
    osc("a", "Quasar", morph=0.3, level=0.75), osc("b", "Classic", morph=0.0, oct=1, level=0.25),
    filt("LP 12", 5200, 0.15, env=10, key=0.6), adsr("amp", 0.001, 0.6, 0.0, 0.5), adsr("fenv", 0.001, 0.4, 0.0, 0.4),
    arp("As Played", "1/8", octaves=1, gate=0.8, length=7,
        lane=_lane("1 .6/7 .8/12 .6/5 1/-5 .7/7 .9/3")),
    mods(("Arp Step", "Morph A", 0.25), ("Heat", "Morph A", 0.35), ("Gravity", "Level B", 0.35), ("Orbit", "Pan", 0.4),
         ("Flux", "FX Delay", 0.45), ("Mod Wheel", "Cutoff", 0.3), ("Aftertouch", "Morph A", 0.25), ("Flux", "FX Reverb", 0.3)),
    fx(delay=("1/4.", 0.45, 0.5, 0.25, True), reverb=(2.0, 3.5, 0.4, 20, 0.28)))

add("Acid Equinox", "Sequence", "Acid line with slides on legato steps, accents that bite and an octave run at the end.",
    ["acid", "303", "squelch"],
    osc("a", "Classic", morph=0.72, level=0.85), osc("b", "Classic", on=False),
    filt("LP 24", 420, 0.75, env=48, key=0.3, drive=0.3, vel=0.3), adsr("amp", 0.001, 0.3, 0.7, 0.05), adsr("fenv", 0.001, 0.18, 0.0, 0.1),
    {"voiceMode": "Legato", "glide": 0.06, "glideLegato": True},
    arp("Up", "1/16", octaves=1, gate=1.05,
        lane=_lane("1 .5 .5/3 1/12 - .5 .9/-12 .5 1/7 .5 - .6/10 1/12 .5/12 .8/3 1/12")),
    mods(("Velocity", "Resonance", 0.2), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.45), ("Orbit", "Morph A", -0.35),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Filter Drive", 0.6), ("Aftertouch", "Cutoff", 0.3), ("Heat", "Resonance", 0.2)),
    fx(drive=("Tube", 0.3, 0.55, 1.0), delay=("1/8.", 0.35, 0.6, 0.15, True), reverb=()))

add("Space Invader March", "Sequence", "Chiptune march: square-wave bass descending four steps, the arcade cabinet in orbit.",
    ["chiptune", "march", "bass"],
    osc("a", "Classic", morph=0.75, level=0.7, warp="Quantize", warpAmt=0.7), osc("b", "Classic", on=False),
    filt("LP 12", 9000, 0.0, key=0.0), adsr("amp", 0.0, 0.15, 0.5, 0.05),
    {"voiceMode": "Mono", "drift": 0.0, "subLevel": 0.3, "subShape": "Square"},
    arp("Down", "1/8", octaves=1, gate=0.6, length=8,
        lane=_lane("1 .8/-1 .9/-2 .8/-3 1 .8/-1 .9/-2 .8/-3")),
    mods(("Velocity", "Amp", 0.2), ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph A", -0.3),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Cutoff", -0.4), ("Aftertouch", "Warp A", 0.25), ("Heat", "Cutoff", 0.2)),
    fx(delay=("1/8T", 0.25, 0.3, 0.1, False), reverb=()))

add("Vangelis Relay", "Sequence", "A cinematic brass sequence under a vast hall, in the style of eighties sci-fi scores.",
    ["cinematic", "brass", "80s"],
    osc("a", "Classic", morph=0.5, unison=3, detune=0.18, width=0.6), osc("b", "Classic", morph=0.75, fine=-6, level=0.45),
    filt("LP 24", 1600, 0.2, env=24, key=0.6), adsr("amp", 0.01, 0.5, 0.5, 0.4), adsr("fenv", 0.02, 0.4, 0.2, 0.4),
    lfo(1, "Sine", 5.0, fade=0.5),
    arp("Up/Down", "1/8T", octaves=2, gate=0.8),
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.5), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Cutoff", 0.2)),
    fx(chorus=(0.3, 0.5, 0.25), reverb=(2.8, 5.5, 0.35, 30, 0.35)))

add("Morse Pulsar", "Sequence", "A short, rhythmic beep code at 1/16T, like a pulsar mistaken for a message.",
    ["morse", "beep", "rhythmic"],
    osc("a", "Classic", morph=0.0, level=0.75), osc("b", "Classic", on=False),
    filt("LP 12", 7000, 0.0, key=0.5), adsr("amp", 0.001, 0.06, 0.9, 0.02),
    {"voiceMode": "Mono", "drift": 0.0},
    arp("Up", "1/16T", octaves=1, gate=0.6, length=12,
        lane=_lane("1 - 1 1 1 - - 1 - 1 - -")),
    mods(("Velocity", "Amp", 0.2), ("Heat", "Morph A", 0.4), ("Gravity", "Sub Level", 0.3), ("Orbit", "Pan", 0.4),
         ("Flux", "FX Delay", 0.5), ("Mod Wheel", "Cutoff", -0.4), ("Aftertouch", "Morph A", 0.3), ("Heat", "Cutoff", 0.2)),
    fx(delay=("1/8T", 0.45, 0.5, 0.25, True), reverb=(1.6, 2.4, 0.45, 15, 0.2)))

add("Breakbeat Comet", "Sequence", "A breakbeat-shaped pluck sequence with ghost notes and rolls, made for jungle and DnB.",
    ["breakbeat", "dnb", "rolls"],
    osc("a", "Fraunhofer", morph=0.1, level=0.8), osc("b", "Classic", morph=0.0, oct=-1, level=0.35),
    filt("LP 24", 2200, 0.3, env=28, key=0.5), adsr("amp", 0.001, 0.2, 0.0, 0.12), adsr("fenv", 0.001, 0.15, 0.0, 0.1),
    arp("As Played", "1/16", octaves=1, gate=0.5,
        lane=_lane("1 - .4 - 1 .3 - .8 - .4 1 - .6x2 - .9x4 .5")),
    mods(("Arp Step", "Cutoff", 0.25), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.3),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Morph A", 0.25)),
    fx(drive=("Soft", 0.15, 0.6, 1.0), delay=("1/16", 0.3, 0.55, 0.12, True), reverb=(1.1, 1.3, 0.5, 5, 0.12)))

add("Converge Engine", "Sequence", "Converge mode at 1/16 with a transposing lane: an engine spooling up toward ignition.",
    ["converge", "build", "techno"],
    osc("a", "Plasma", morph=0.2, level=0.7, unison=2, detune=0.15), osc("b", "Classic", morph=0.5, oct=-1, level=0.4),
    filt("LP 24", 1500, 0.45, env=30, key=0.5), adsr("amp", 0.001, 0.2, 0.3, 0.1), adsr("fenv", 0.001, 0.15, 0.0, 0.1),
    lfo(1, "Saw Up", sync=True, div="2 bars", retrig=False),
    arp("Converge", "1/16", octaves=2, gate=0.45,
        lane=_lane("1 .6 .7 .6 1/2 .6 .7 .6/2 1/5 .6 .7/5 .6 1/7 .7/7 .8x2 .9x4")),
    mods(("LFO 1", "Cutoff", 0.2), ("Heat", "Morph A", 0.4), ("Gravity", "Level B", 0.35), ("Orbit", "LFO 1 Rate", 0.35),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.3), ("Arp Step", "Cutoff", 0.15)),
    fx(drive=("Tube", 0.2, 0.6, 1.0), delay=("1/16.", 0.35, 0.55, 0.15, True), reverb=(1.2, 1.4, 0.5, 5, 0.12)))

add("Kraut Satellite", "Sequence", "Motorik eighths on a reedy Nebula organ, endlessly forward like a probe leaving the system.",
    ["krautrock", "motorik", "organ"],
    osc("a", "Nebula", morph=0.5, level=0.7), osc("b", "Classic", morph=0.75, oct=-1, level=0.4),
    filt("LP 24", 2000, 0.3, env=16, key=0.5), adsr("amp", 0.002, 0.3, 0.6, 0.12), adsr("fenv", 0.001, 0.25, 0.2, 0.15),
    lfo(1, "Triangle", sync=True, div="4 bars", retrig=False),
    arp("Up", "1/8", octaves=1, gate=0.75,
        lane=_lane("1 .7 .8/7 .7 1/12 .7 .8/7 .7 1/5 .7 .8/12 .7 1/3 .7 .8/7 .7")),
    mods(("LFO 1", "Morph A", 0.25), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.35), ("Orbit", "LFO 1 Rate", 0.3),
         ("Flux", "FX Chorus", 0.45), ("Mod Wheel", "Morph A", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Delay", 0.3)),
    fx(chorus=(0.3, 0.45, 0.2), delay=("1/4", 0.35, 0.55, 0.15, True), reverb=(1.5, 2.2, 0.45, 12, 0.18)))

# =====================================================================================================
# FX
# =====================================================================================================
add("Ascension Riser", "FX", "An eight-second white-noise and saw riser that opens and accelerates into the drop.",
    ["riser", "build", "noise"],
    osc("a", "Classic", morph=0.5, level=0.55, unison=5, detune=0.35, width=1.0), osc("b", "Classic", on=False),
    filt("HP 12", 200, 0.3, env=60, key=0.0), adsr("amp", 6.0, 1.0, 1.0, 0.8), adsr("fenv", 8.0, 1.0, 1.0, 0.5),
    adsr("menv", 8.0, 1.0, 1.0, 0.5),
    {"noiseLevel": 0.6, "noiseType": "White"},
    lfo(1, "Square", 2.0),
    mods(("Mod Env", "Pitch", 0.25), ("Mod Env", "LFO 1 Rate", 0.6), ("Heat", "Resonance", 0.4), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(delay=("1/8", 0.4, 0.5, 0.15, True), reverb=(2.4, 4.0, 0.4, 20, 0.35)))

add("Orbital Decay Downlifter", "FX", "A falling, filtering sweep: a satellite losing altitude and burning up in the hiss.",
    ["downlifter", "fall", "sweep"],
    osc("a", "Plasma", morph=0.8, level=0.6, unison=4, detune=0.3), osc("b", "Classic", on=False),
    filt("LP 24", 8000, 0.35, env=-60, key=0.0), adsr("amp", 0.001, 4.0, 0.0, 3.0), adsr("fenv", 0.001, 4.0, 0.0, 3.0),
    adsr("menv", 0.001, 4.0, 0.0, 3.0),
    {"noiseLevel": 0.4, "noiseType": "Solar Wind"},
    mods(("Mod Env", "Pitch", 0.4), ("Mod Env", "Morph A", 0.5), ("Heat", "Filter Drive", 0.4), ("Gravity", "Pitch", -0.3),
         ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Resonance", 0.3)),
    fx(reverb=(2.6, 5.0, 0.4, 25, 0.38)))

add("Red Alert Siren", "FX", "A two-tone starship alarm whooping on a square LFO; Orbit speeds it toward panic.",
    ["siren", "alarm", "sci-fi"],
    osc("a", "Classic", morph=0.75, level=0.6), osc("b", "Classic", morph=0.5, fine=12, level=0.4),
    filt("LP 12", 5000, 0.3, key=0.5), adsr("amp", 0.01, 0.5, 1.0, 0.3),
    lfo(1, "Square", 1.4, retrig=True), lfo(2, "Sine", 0.5, retrig=True),
    {"voiceMode": "Mono"},
    mods(("LFO 1", "Pitch", 0.4), ("LFO 2", "Cutoff", 0.15), ("Heat", "FX Drive", 0.45), ("Gravity", "Sub Level", 0.35),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Soft", 0.2, 0.5, 1.0), reverb=(1.8, 2.5, 0.45, 15, 0.25)))

add("Proximity Warning", "FX", "Cockpit beeps that quicken as the mod wheel comes up: something is closing fast.",
    ["alarm", "beep", "cockpit"],
    osc("a", "Classic", morph=0.0, level=0.7), osc("b", "Classic", morph=0.0, oct=1, level=0.3),
    filt("LP 12", 6000, 0.0, key=0.5), adsr("amp", 0.001, 0.05, 1.0, 0.02),
    lfo(1, "Square", 3.0, retrig=True),
    {"voiceMode": "Mono", "drift": 0.0},
    mods(("LFO 1", "Amp", 1.0), ("Mod Wheel", "LFO 1 Rate", 0.6), ("Heat", "Morph A", 0.4), ("Gravity", "Level B", 0.35),
         ("Orbit", "Pan", 0.4), ("Flux", "FX Delay", 0.4), ("Aftertouch", "Pitch", 0.08), ("Heat", "Cutoff", 0.2)),
    fx(delay=("1/16", 0.3, 0.4, 0.15, True), reverb=(1.2, 1.4, 0.5, 5, 0.15)))

add("Deep Space Transmission", "FX", "A crackling radio carrier through the formant filter, voices half-heard across the void.",
    ["radio", "transmission", "formant"],
    osc("a", "Ion Drive", morph=0.5, level=0.5), osc("b", "Classic", on=False),
    filt("Formant", 900, 0.6, key=0.3), adsr("amp", 0.3, 1.0, 1.0, 1.2),
    {"noiseLevel": 0.45, "noiseType": "Crackle"},
    lfo(1, "S&H", 9.0, retrig=False), lfo(2, "Drift", 0.3, retrig=False),
    mods(("LFO 1", "Cutoff", 0.25), ("LFO 2", "Morph A", 0.4), ("Heat", "Resonance", 0.3), ("Gravity", "Sub Level", 0.3),
         ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Cosmic Ray", 0.3, 0.4, 0.6), delay=("1/4.", 0.45, 0.6, 0.2, True), reverb=(2.0, 3.0, 0.45, 20, 0.28)))

add("Android Diplomat", "FX", "A robot voice on the formant filter: vowels shaped by an envelope, words in an alien tongue.",
    ["robot", "vocal", "formant"],
    osc("a", "Classic", morph=0.5, level=0.75, unison=2, detune=0.08), osc("b", "Classic", morph=0.75, oct=-1, level=0.4),
    filt("Formant", 500, 0.65, env=24, key=0.4), adsr("amp", 0.01, 0.4, 0.8, 0.2), adsr("fenv", 0.02, 0.35, 0.3, 0.2),
    lfo(1, "S&H", sync=True, div="1/8", retrig=True),
    {"voiceMode": "Legato", "glide": 0.04},
    mods(("LFO 1", "Cutoff", 0.3), ("Heat", "Resonance", 0.3), ("Gravity", "Level B", 0.4), ("Orbit", "LFO 1 Rate", 0.4),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Morph A", 0.3), ("Velocity", "Cutoff", 0.2)),
    fx(drive=("Fold", 0.15, 0.5, 0.5), reverb=(1.4, 1.8, 0.45, 10, 0.18)))

add("Plasma Rifle", "FX", "A fat sci-fi blaster: pitch dives two octaves through drive and a short, bright slap.",
    ["laser", "blaster", "zap"],
    osc("a", "Magnetar", morph=0.5, level=0.7), osc("b", "Classic", morph=0.75, oct=-1, level=0.4),
    filt("LP 24", 4500, 0.3, env=30, key=0.0), adsr("amp", 0.001, 0.4, 0.0, 0.2), adsr("fenv", 0.001, 0.3, 0.0, 0.2),
    adsr("menv", 0.001, 0.35, 0.0, 0.2),
    {"noiseLevel": 0.2},
    mods(("Mod Env", "Pitch", 0.85), ("Mod Env", "Morph A", 0.5), ("Heat", "FX Drive", 0.5), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.35, 0.6, 1.0), delay=("1/16", 0.3, 0.5, 0.12, True), reverb=(1.2, 1.4, 0.5, 5, 0.12)))

add("Ion Engine Spool-Up", "FX", "The low hum of a ship at rest: a beating sub drone with a slow thermal shimmer above it.",
    ["engine", "hum", "drone"],
    osc("a", "Classic", morph=0.25, level=0.7, unison=3, detune=0.08), osc("b", "Ion Drive", morph=0.3, oct=1, level=0.25),
    filt("LP 24", 500, 0.35, key=0.3), adsr("amp", 1.5, 1.0, 1.0, 2.0),
    {"subLevel": 0.5, "noiseLevel": 0.15, "noiseType": "Pink"},
    lfo(1, "Drift", 0.4, retrig=False), lfo(2, "Sine", 7.0, retrig=False),
    mods(("LFO 1", "Cutoff", 0.15), ("LFO 2", "Level B", 0.25), ("Heat", "Cutoff", 0.4), ("Gravity", "Sub Level", 0.45),
         ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Pitch A", 0.15), ("Aftertouch", "Filter Drive", 0.4)),
    fx(drive=("Tube", 0.2, 0.4, 1.0), reverb=(2.2, 4.0, 0.5, 20, 0.25)))

add("Quantum Teleport", "FX", "Dematerialise: a rising shimmer of ratcheted, randomised partials vanishing into the reverb.",
    ["teleport", "shimmer", "transition"],
    osc("a", "Quasar", morph=0.5, level=0.6, unison=4, detune=0.25, width=1.0), osc("b", "Nebula", morph=0.8, oct=2, level=0.3),
    filt("HP 12", 400, 0.3, env=30, key=0.5), adsr("amp", 0.3, 2.0, 0.0, 2.5), adsr("fenv", 1.5, 1.0, 1.0, 1.5),
    adsr("menv", 2.0, 1.0, 1.0, 1.5),
    lfo(1, "S&H", 16.0, retrig=False),
    arp("Random", "1/32", octaves=3, gate=0.4),
    mods(("Mod Env", "Pitch", 0.3), ("LFO 1", "Pan", 0.8), ("Heat", "Morph A", 0.4), ("Gravity", "Level B", 0.3),
         ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(delay=("1/16T", 0.5, 0.4, 0.25, True), reverb=(2.8, 6.0, 0.3, 30, 0.45)))

add("Hyperspace Whoosh", "FX", "A wide stereo noise sweep through the band-pass: the stars stretching into lines and back.",
    ["whoosh", "sweep", "transition"],
    osc("a", "Classic", on=False), osc("b", "Classic", on=False),
    filt("BP 12", 600, 0.55, env=48, key=0.0), adsr("amp", 1.2, 1.5, 0.0, 1.5), adsr("fenv", 1.5, 1.8, 0.0, 1.5),
    {"noiseLevel": 0.9, "noiseType": "Pink"},
    lfo(1, "Sine", 0.6, retrig=True),
    mods(("LFO 1", "Pan", 0.9), ("Heat", "Resonance", 0.35), ("Gravity", "Cutoff", -0.2), ("Orbit", "LFO 1 Rate", 0.4),
         ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Resonance", 0.3), ("Heat", "Cutoff", 0.2)),
    fx(reverb=(2.4, 3.5, 0.4, 15, 0.35), width=1.6))

add("Asteroid Impact", "FX", "A sub-heavy impact with a gritty crackle tail and a long cavern of reverb.",
    ["impact", "boom", "cinematic"],
    osc("a", "Classic", morph=0.0, level=0.8, oct=-2), osc("b", "Dark Matter", morph=0.8, oct=-1, level=0.4),
    filt("LP 24", 220, 0.25, env=60, key=0.0, drive=0.4), adsr("amp", 0.001, 2.5, 0.0, 2.5), adsr("fenv", 0.001, 0.5, 0.0, 0.5),
    adsr("menv", 0.001, 0.4, 0.0, 0.4),
    {"noiseLevel": 0.4, "noiseType": "Crackle", "subLevel": 0.6},
    mods(("Mod Env", "Pitch", 0.5), ("Velocity", "Cutoff", 0.25), ("Heat", "FX Drive", 0.5), ("Gravity", "Sub Level", 0.45),
         ("Orbit", "Morph B", 0.35), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.3, 0.5, 1.0), reverb=(3.0, 7.0, 0.5, 15, 0.4)))

add("Data Corruption", "FX", "Bit-crushed glitch bursts: random pitch, ratchets and folds, a hard drive failing in orbit.",
    ["glitch", "digital", "random"],
    osc("a", "Event Horizon", morph=0.6, level=0.65, warp="Quantize", warpAmt=0.8), osc("b", "Classic", morph=0.75, level=0.3),
    filt("LP 12", 7000, 0.3, key=0.3), adsr("amp", 0.001, 0.1, 0.6, 0.05),
    lfo(1, "S&H", sync=True, div="1/32", retrig=False),
    arp("Random", "1/32", octaves=3, gate=0.3, prob=0.55,
        lane=_lane("1x4 - .7 1x2 - - .9x3 .5 - 1 .6x4 - .8 - 1x2 .4")),
    mods(("LFO 1", "Morph A", 0.5), ("LFO 1", "Pitch A", 0.3), ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.3),
         ("Orbit", "Pan", 0.7), ("Flux", "FX Delay", 0.45), ("Mod Wheel", "Cutoff", 0.35), ("Aftertouch", "Resonance", 0.3)),
    fx(drive=("Cosmic Ray", 0.4, 0.6, 0.7), delay=("1/32", 0.45, 0.3, 0.2, True), reverb=(1.0, 1.0, 0.5, 2, 0.1)))

add("Solar Sail Unfurl", "FX", "A slow metallic swell through the comb filter, sheets of foil catching the sunlight.",
    ["swell", "metallic", "comb"],
    osc("a", "Fraunhofer", morph=0.4, level=0.6, unison=3, detune=0.15, width=0.9), osc("b", "Classic", on=False),
    filt("Comb", 300, 0.6, env=24, key=1.0), adsr("amp", 2.5, 2.0, 0.8, 3.0), adsr("fenv", 3.0, 2.0, 0.6, 2.0),
    {"noiseLevel": 0.25, "noiseType": "Solar Wind"},
    lfo(1, "Sine", 0.15, retrig=False),
    mods(("LFO 1", "Cutoff", 0.08), ("Heat", "Resonance", 0.3), ("Gravity", "Sub Level", 0.3), ("Orbit", "Morph A", 0.45),
         ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Morph A", 0.3), ("Heat", "Morph A", 0.25)),
    fx(chorus=(0.15, 0.6, 0.3), reverb=(2.8, 7.0, 0.35, 40, 0.42)))

add("Countdown Pulse", "FX", "A launch-control tone that beeps once a beat and climbs a semitone per bar when the arp runs.",
    ["countdown", "tension", "beep"],
    osc("a", "Pulsar", morph=0.2, level=0.7), osc("b", "Classic", morph=0.0, oct=1, level=0.25),
    filt("LP 12", 4500, 0.2, env=10, key=0.5), adsr("amp", 0.001, 0.15, 0.0, 0.1), adsr("fenv", 0.001, 0.1, 0.0, 0.1),
    arp("Up", "1/4", octaves=1, gate=0.3, length=16,
        lane=_lane("1 .7 .7 .7 1/1 .7/1 .7/1 .7/1 1/2 .7/2 .7/2 .7/2 1/3 .7/3 .8/3x2 1/4x4")),
    mods(("Arp Step", "Cutoff", 0.25), ("Heat", "Morph A", 0.4), ("Gravity", "Level B", 0.35), ("Orbit", "Pan", 0.4),
         ("Flux", "FX Delay", 0.45), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Pitch", 0.05), ("Flux", "FX Reverb", 0.3)),
    fx(delay=("1/8", 0.4, 0.5, 0.2, True), reverb=(1.8, 2.8, 0.45, 15, 0.25)))
