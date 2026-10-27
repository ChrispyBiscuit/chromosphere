# Chords (stabs, polys, keys for progressions) and Melody (plucks, mallets, bells for melodic lines).

# =====================================================================================================
# CHORDS
# =====================================================================================================
add("Mainframe Organ Stab", "Chords", "The classic house organ stab: bright, hollow, short, made to play offbeat chords.",
    ["house", "organ", "stab", "m1"],
    osc("a", "Ion Drive", unison=2, detune=0.04, width=0.8, morph=0.55, level=0.8), osc("b", "Ion Drive", morph=0.8, oct=1, level=0.45),
    filt("LP 12", 5200, 0.15, env=10, key=0.5, vel=0.3), adsr("amp", 0.001, 0.42, 0.5, 0.3), adsr("fenv", 0.001, 0.2, 0.2, 0.15),
    {"polyphony": 10},
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", -0.3), ("Orbit", "Morph A", 0.3),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Morph B", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Key", "Level B", -0.2)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.5, 0.35, 0.3), reverb=(1.0, 1.3, 0.5, 8, 0.14), squash=0.6, mono_low=220))

add("Deep Field Chord", "Chords", "Deep house minor-ninth chord: soft saws through a warm low-pass, gently pumping.",
    ["deep house", "chord", "warm"],
    osc("a", "Classic", morph=0.5, unison=3, detune=0.18, width=0.8), osc("b", "Classic", morph=0.75, level=0.45, fine=-6),
    filt("LP 24", 4200, 0.22, env=16, key=0.5, vel=0.3), adsr("amp", 0.004, 0.78, 0.5, 0.3), adsr("fenv", 0.002, 0.35, 0.25, 0.3),
    lfo(2, "Saw Down", sync=True, div="1/4", retrig=False),
    {"polyphony": 10},
    mods(("LFO 2", "Amp", 0.18), ("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.4), ("Gravity", "Sub Level", 0.3),
         ("Orbit", "Detune", 0.35), ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.3, 0.45, 0.35), delay=("1/8.", 0.3, 0.55, 0.12, True), reverb=(1.4, 1.8, 0.5, 12, 0.16), squash=0.6, mono_low=220))

add("Motor City Probe", "Chords", "Detroit techno stab: a resonant, snappy chord that ricochets through a ping-pong echo.",
    ["detroit", "techno", "stab"],
    osc("a", "Classic", morph=0.62, unison=2, detune=0.12, width=0.7), osc("b", "Pulsar", morph=0.3, level=0.4),
    filt("LP 24", 2400, 0.45, env=34, key=0.6, vel=0.35), adsr("amp", 0.001, 0.45, 0.45, 0.3), adsr("fenv", 0.001, 0.16, 0.0, 0.12),
    {"polyphony": 10},
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Resonance", 0.25), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.3),
         ("Orbit", "Morph B", 0.35), ("Flux", "FX Feedback", 0.3), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Resonance", 0.25)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), delay=("1/8.", 0.45, 0.5, 0.22, True), reverb=(1.2, 1.6, 0.5, 10, 0.14), squash=0.6, mono_low=220))

add("Subspace Dub Chord", "Chords", "Dub techno chord: a muted stab dissolving into long, darkening echoes and a big room.",
    ["dub techno", "chord", "echo"],
    osc("a", "Classic", morph=0.55, unison=2, detune=0.1, width=0.6), osc("b", "Nebula", morph=0.4, level=0.35),
    filt("LP 24", 2400, 0.3, env=20, key=0.5, vel=0.25), adsr("amp", 0.002, 0.39, 0.45, 0.3), adsr("fenv", 0.001, 0.12, 0.0, 0.1),
    {"polyphony": 10},
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.3), ("Orbit", "Morph B", 0.35),
         ("Flux", "FX Feedback", 0.35), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.3)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.2, 0.5, 0.25), delay=("1/8.", 0.62, 0.65, 0.35, True), reverb=(2.2, 3.5, 0.55, 20, 0.3), squash=0.6, mono_low=220))

add("Supernova Bloom Chords", "Chords", "Future bass supersaw chords that pump on every beat and shimmer at the top.",
    ["future bass", "supersaw", "pumping"],
    osc("a", "Classic", morph=0.5, unison=7, detune=0.3, width=1.0), osc("b", "Classic", morph=0.5, oct=1, unison=5, detune=0.25, width=1.0, level=0.4),
    filt("LP 12", 7500, 0.12, env=8, key=0.5, vel=0.25), adsr("amp", 0.003, 0.5, 0.85, 0.25), adsr("fenv", 0.002, 0.4, 0.5, 0.3),
    lfo(2, "Saw Up", sync=True, div="1/4", retrig=False),
    {"polyphony": 10},
    mods(("LFO 2", "Amp", 0.45), ("Velocity", "Cutoff", 0.2), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", -0.3),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "LFO 2 Amount", -0.8), ("Aftertouch", "Cutoff", 0.2)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.35, 0.4, 0.25), reverb=(1.6, 2.0, 0.4, 15, 0.18), squash=0.6, mono_low=220, width=1.3))

add("Trance Gate Chords", "Chords", "Trance pluck chords: bright detuned saws with a short filter snap and a dotted echo.",
    ["trance", "pluck", "chords"],
    osc("a", "Classic", morph=0.5, unison=5, detune=0.22, width=0.95), osc("b", "Classic", morph=0.5, oct=1, level=0.35, fine=8),
    filt("LP 24", 2800, 0.25, env=40, key=0.6, vel=0.3), adsr("amp", 0.001, 0.59, 0.45, 0.3), adsr("fenv", 0.001, 0.26, 0.0, 0.2),
    {"polyphony": 12},
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.3), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.25)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), delay=("1/8.", 0.4, 0.4, 0.22, True), reverb=(1.6, 2.2, 0.4, 15, 0.18), squash=0.6, mono_low=220, width=1.2))

add("Juno Constellation", "Chords", "Juno-style PWM poly: a chorused pulse chord that breathes as the pulse width moves.",
    ["juno", "pwm", "poly"],
    osc("a", "Classic", unison=2, detune=0.06, width=0.8, morph=0.8, level=0.8), osc("b", "Classic", morph=0.62, level=0.5, fine=5),
    filt("LP 24", 5500, 0.15, env=14, key=0.6, vel=0.3), adsr("amp", 0.005, 1.04, 0.65, 0.35), adsr("fenv", 0.002, 0.5, 0.35, 0.3),
    lfo(1, "Triangle", 0.7, retrig=False),
    {"polyphony": 10, "subLevel": 0.15},
    mods(("LFO 1", "Morph A", 0.12), ("LFO 1", "Morph B", -0.1), ("Aftertouch", "Resonance", 0.25), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Sub Level", 0.4), ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Cutoff", 0.35)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.55, 0.7, 0.5), reverb=(1.2, 1.6, 0.5, 10, 0.12), squash=0.6, mono_low=220))

add("Oberon Poly Brass", "Chords", "OB-style brassy poly: fat saws with a filter swell on every chord, bold and warm.",
    ["oberheim", "brassy", "poly"],
    osc("a", "Classic", morph=0.5, unison=2, detune=0.15, width=0.8), osc("b", "Classic", morph=0.5, level=0.55, fine=-9),
    filt("LP 12", 3500, 0.2, env=30, key=0.6, vel=0.35), adsr("amp", 0.02, 0.78, 0.75, 0.3), adsr("fenv", 0.06, 0.45, 0.35, 0.3),
    {"polyphony": 10},
    mods(("Velocity", "Cutoff", 0.35), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.3), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Filter Drive", 0.5), ("Aftertouch", "Cutoff", 0.3), ("Key", "Cutoff", 0.15)),
    fx(drive=("Tube", 0.22, 0.7, 1.0), chorus=(0.4, 0.4, 0.25), reverb=(1.3, 1.7, 0.5, 12, 0.13), squash=0.6, mono_low=220, sub_clean=110))

add("Prophet Of Proxima", "Chords", "Prophet-style poly: saw and pulse with a polished filter envelope for lush progressions.",
    ["prophet", "poly", "classic"],
    osc("a", "Classic", unison=2, detune=0.07, width=0.8, morph=0.5, level=0.75), osc("b", "Classic", morph=0.72, level=0.5, fine=7),
    filt("LP 24", 4800, 0.2, env=20, key=0.6, vel=0.3), adsr("amp", 0.006, 0.91, 0.6, 0.4), adsr("fenv", 0.003, 0.5, 0.3, 0.35),
    lfo(1, "Sine", 0.4, retrig=False),
    {"polyphony": 10},
    mods(("LFO 1", "Morph B", 0.08), ("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.3),
         ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.4, 0.5, 0.3), delay=("1/4", 0.25, 0.5, 0.08, True), reverb=(1.4, 1.9, 0.5, 12, 0.14), squash=0.6, mono_low=220))

add("Digital Dawn Keys", "Chords", "80s FM keys chords: a glassy tine attack over a soft body, the DX ballad sound.",
    ["fm", "dx", "80s", "keys"],
    osc("a", "Classic", unison=2, detune=0.04, width=0.8, morph=0.0, level=0.75, warp="FM from B", warpAmt=0.15), osc("b", "Classic", morph=0.0, oct=2, level=0.0),
    filt("LP 12", 9000, 0.05, key=0.4, vel=0.2), adsr("amp", 0.001, 2.08, 0.45, 0.5), adsr("menv", 0.001, 0.4, 0.05, 0.3),
    {"polyphony": 12},
    mods(("Mod Env", "Warp A", 0.2), ("Velocity", "Warp A", 0.3), ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.25),
         ("Orbit", "Detune", 0.25), ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Warp A", 0.3), ("Aftertouch", "Cutoff", 0.2)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.5, 0.45, 0.4), reverb=(1.3, 1.8, 0.4, 10, 0.15), squash=0.6, mono_low=220))

add("Faded Signal Keys", "Chords", "Lo-fi keys: a warbling, detuned electric piano through tape grit and a dusty chorus.",
    ["lo-fi", "keys", "tape"],
    osc("a", "Classic", unison=2, detune=0.05, width=0.8, morph=0.15, level=0.75, warp="FM from B", warpAmt=0.24), osc("b", "Classic", morph=0.0, oct=1, level=0.25, fine=-12),
    filt("LP 12", 6500, 0.1, key=0.5, vel=0.3), adsr("amp", 0.002, 1.82, 0.45, 0.45),
    lfo(1, "Drift", 0.6, retrig=False),
    {"polyphony": 10, "drift": 0.6, "noiseLevel": 0.04, "noiseType": "Crackle"},
    mods(("LFO 1", "Pitch", 0.006), ("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.3),
         ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Cutoff", 0.2)),
    fx(drive=("Cosmic Ray", 0.25, 0.55, 0.4), chorus=(0.35, 0.7, 0.45), reverb=(1.1, 1.4, 0.6, 8, 0.12), squash=0.6, mono_low=220))

add("Neo Soul Nebula", "Chords", "Neo-soul electric piano chords: bell-tipped, round, with a lazy tremolo across the stereo field.",
    ["neo soul", "ep", "rhodes"],
    osc("a", "Classic", unison=2, detune=0.04, width=0.8, morph=0.05, level=0.8, warp="FM from B", warpAmt=0.22), osc("b", "Classic", morph=0.0, oct=1, level=0.0),
    filt("LP 12", 6500, 0.05, key=0.5, vel=0.35), adsr("amp", 0.001, 2.34, 0.45, 0.5), adsr("menv", 0.001, 0.5, 0.1, 0.3),
    lfo(1, "Sine", 4.2, retrig=False),
    {"polyphony": 12},
    mods(("LFO 1", "Pan", 0.4), ("Mod Env", "Warp A", 0.3), ("Aftertouch", "Cutoff", 0.2), ("Heat", "Warp A", 0.3),
         ("Gravity", "Sub Level", 0.25), ("Orbit", "LFO 1 Amount", 0.6), ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "LFO 1 Rate", 0.3)),
    fx(drive=("Tube", 0.22, 0.6, 1.0), chorus=(0.3, 0.4, 0.3), reverb=(1.2, 1.5, 0.5, 10, 0.12), squash=0.6, mono_low=220, sub_clean=100))

add("Martian Wurli", "Chords", "Wurli-style keys: reedy and barky when hit hard, mellow when played softly.",
    ["wurlitzer", "keys", "reedy"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.8, morph=0.25, level=0.75, warp="FM from B", warpAmt=0.25), osc("b", "Classic", morph=0.4, oct=1, level=0.0),
    filt("LP 12", 4200, 0.18, env=12, key=0.5, vel=0.45), adsr("amp", 0.001, 1.56, 0.45, 0.3), adsr("fenv", 0.001, 0.3, 0.2, 0.2),
    lfo(1, "Sine", 5.5, retrig=False),
    {"polyphony": 10},
    mods(("LFO 1", "Amp", 0.08), ("Velocity", "Warp A", 0.4), ("Heat", "Filter Drive", 0.4), ("Gravity", "Sub Level", 0.25),
         ("Orbit", "LFO 1 Amount", 0.6), ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "LFO 1 Rate", 0.3), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Tube", 0.25, 0.65, 1.0), chorus=(0.4, 0.35, 0.25), reverb=(1.0, 1.2, 0.5, 8, 0.12), squash=0.6, mono_low=220, sub_clean=100))

add("Glass Horizon Stab", "Chords", "Glassy digital pad-stab: a bright Quasar chord with a short swell and a shimmering tail.",
    ["digital", "glassy", "stab"],
    osc("a", "Quasar", morph=0.45, unison=4, detune=0.15, width=0.9), osc("b", "Fraunhofer", morph=0.6, oct=1, level=0.35),
    filt("HP 12", 220, 0.1, key=0.0, vel=0.2), adsr("amp", 0.01, 1.04, 0.45, 0.6), adsr("menv", 0.001, 0.6, 0.2, 0.5),
    {"polyphony": 10},
    mods(("Mod Env", "Morph A", 0.3), ("Velocity", "Amp", 0.25), ("Heat", "Morph B", 0.35), ("Gravity", "Cutoff", -0.2),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Morph A", 0.35), ("Aftertouch", "Level B", 0.3)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.4, 0.5, 0.3), delay=("1/8.", 0.3, 0.4, 0.12, True), reverb=(1.8, 2.6, 0.35, 20, 0.22), squash=0.6, mono_low=220, width=1.2))

add("Hyperspace Pluck Chords", "Chords", "Hyperpop future pluck chords: bright, bouncy, pitch-flicked on the attack and very wide.",
    ["hyperpop", "future", "pluck"],
    osc("a", "Classic", morph=0.5, unison=6, detune=0.25, width=1.0), osc("b", "Plasma", morph=0.3, oct=1, level=0.35),
    filt("LP 12", 2600, 0.2, env=40, key=0.6, vel=0.3), adsr("amp", 0.001, 0.59, 0.45, 0.3), adsr("fenv", 0.001, 0.2, 0.0, 0.15),
    adsr("menv", 0.001, 0.05, 0.0, 0.05),
    {"polyphony": 12},
    mods(("Mod Env", "Pitch", 0.08), ("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.3),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Morph B", 0.4), ("Aftertouch", "Resonance", 0.25)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), delay=("1/16.", 0.35, 0.3, 0.18, True), reverb=(1.3, 1.6, 0.35, 10, 0.15), squash=0.6, mono_low=220, width=1.3))

add("Neon Grid Poly", "Chords", "Synthwave poly: lush detuned saws, a slow filter and a gated reverb glow.",
    ["synthwave", "poly", "80s"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.2, width=0.9), osc("b", "Classic", morph=0.8, level=0.4, fine=-7),
    filt("LP 24", 5000, 0.2, env=14, key=0.6, vel=0.3), adsr("amp", 0.01, 1.17, 0.7, 0.5), adsr("fenv", 0.05, 0.8, 0.4, 0.4),
    {"polyphony": 10},
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.3), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Delay", 0.25)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.45, 0.5, 0.35), delay=("1/4.", 0.3, 0.5, 0.1, True), reverb=(1.8, 2.2, 0.45, 15, 0.2), squash=0.6, mono_low=220, width=1.2))

add("Solina Satellite", "Chords", "String-machine chords: an ensemble of buzzing saws and a deep triple chorus.",
    ["string machine", "solina", "ensemble"],
    osc("a", "Classic", morph=0.5, unison=3, detune=0.12, width=0.9), osc("b", "Classic", morph=0.85, oct=1, level=0.4),
    filt("LP 12", 6500, 0.05, key=0.5, vel=0.2), adsr("amp", 0.12, 0.8, 0.85, 0.7),
    {"polyphony": 12},
    mods(("Velocity", "Amp", 0.2), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", -0.3), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Cutoff", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.3)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.7, 0.8, 0.6), reverb=(1.6, 2.2, 0.45, 15, 0.18), squash=0.6, mono_low=220))

add("Jump Gate Stab", "Chords", "80s brass stab: a punchy, bright chord with a fast filter blat, built for big riffs.",
    ["brass", "stab", "80s"],
    osc("a", "Classic", morph=0.5, unison=3, detune=0.15, width=0.8), osc("b", "Classic", morph=0.5, level=0.5, fine=8),
    filt("LP 24", 3000, 0.2, env=30, key=0.6, vel=0.35), adsr("amp", 0.005, 0.59, 0.55, 0.3), adsr("fenv", 0.02, 0.25, 0.3, 0.2),
    {"polyphony": 10},
    mods(("Velocity", "Cutoff", 0.35), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.3), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Filter Drive", 0.45), ("Aftertouch", "Cutoff", 0.3), ("Key", "Cutoff", 0.15)),
    fx(drive=("Tube", 0.25, 0.75, 1.0), reverb=(1.2, 1.4, 0.5, 8, 0.14), squash=0.6, mono_low=220, sub_clean=110))

add("Pixel Nova Chords", "Chords", "Chiptune chords: stepped square waves with an arpeggio feel and a crisp bit-crushed edge.",
    ["chiptune", "8-bit", "square"],
    osc("a", "Classic", unison=2, detune=0.05, width=0.8, morph=0.75, level=0.7, warp="Quantize", warpAmt=0.6), osc("b", "Classic", morph=0.9, oct=1, level=0.45),
    filt("HP 12", 300, 0.05, key=0.0), adsr("amp", 0.001, 0.52, 0.5, 0.3),
    lfo(1, "Square", sync=True, div="1/16", retrig=True),
    {"polyphony": 10},
    mods(("LFO 1", "Level B", 0.3), ("Velocity", "Amp", 0.2), ("Heat", "Warp A", 0.35), ("Gravity", "Level B", -0.3),
         ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Morph A", 0.3), ("Aftertouch", "Cutoff", 0.2)),
    fx(drive=("Cosmic Ray", 0.25, 0.7, 0.4), delay=("1/8", 0.3, 0.5, 0.12, True), reverb=(0.9, 1.0, 0.5, 5, 0.1), squash=0.6, mono_low=220))

add("Event Horizon Swell", "Chords", "Ambient chord swell: each chord blooms slowly, glows, and hangs in a vast hall.",
    ["ambient", "swell", "pad"],
    osc("a", "Nebula", morph=0.4, unison=4, detune=0.18, width=1.0), osc("b", "Quasar", morph=0.6, oct=1, level=0.3),
    filt("LP 12", 4500, 0.1, env=12, key=0.5), adsr("amp", 0.6, 1.5, 0.85, 1.6), adsr("fenv", 0.8, 1.5, 0.5, 1.2),
    lfo(1, "Sine", 0.15, retrig=False),
    {"polyphony": 12},
    mods(("LFO 1", "Morph A", 0.2), ("Velocity", "Amp", 0.2), ("Heat", "Cutoff", 0.35), ("Gravity", "Cutoff", -0.2),
         ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Morph B", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.25, 0.5, 0.3), reverb=(2.6, 5.0, 0.4, 30, 0.35), squash=0.6, mono_low=220, width=1.3))

add("Cathedral Of Ceres Organ", "Chords", "Gospel organ: full drawbars with a fast rotary shimmer and a growl when pushed.",
    ["gospel", "organ", "drawbar"],
    osc("a", "Ion Drive", unison=2, detune=0.03, width=0.8, morph=0.35, level=0.75), osc("b", "Ion Drive", morph=0.75, oct=1, level=0.45),
    filt("LP 12", 9000, 0.05, key=0.4), adsr("amp", 0.004, 0.2, 1.0, 0.12),
    lfo(1, "Sine", 6.5, retrig=False),
    {"polyphony": 12, "subLevel": 0.2},
    mods(("LFO 1", "Pan", 0.35), ("LFO 1", "Pitch", 0.002), ("Heat", "Filter Drive", 0.4), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "LFO 1 Rate", 0.35), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "LFO 1 Amount", 0.8), ("Aftertouch", "Level B", 0.3)),
    fx(drive=("Tube", 0.22, 0.6, 1.0), chorus=(1.2, 0.3, 0.25), reverb=(1.6, 2.0, 0.5, 12, 0.15), squash=0.6, mono_low=220, sub_clean=110))

add("Parisian Orbit Filter", "Chords", "French house chord: a disco sample-like loop through a sweeping, phasing filter.",
    ["french house", "filtered", "disco"],
    osc("a", "Classic", morph=0.5, unison=3, detune=0.14, width=0.8), osc("b", "Pulsar", morph=0.5, level=0.4),
    filt("LP 24", 2400, 0.4, env=10, key=0.5, vel=0.25), adsr("amp", 0.003, 0.52, 0.7, 0.3), adsr("fenv", 0.002, 0.3, 0.3, 0.2),
    lfo(1, "Sine", sync=True, div="2 bars", retrig=False),
    {"polyphony": 10},
    mods(("LFO 1", "Cutoff", 0.35), ("Velocity", "Cutoff", 0.25), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.3),
         ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.3, 0.7, 1.0), delay=("1/8", 0.3, 0.5, 0.1, True), reverb=(1.0, 1.2, 0.5, 8, 0.1), squash=0.6, mono_low=220, sub_clean=110))

add("Two Step Transit", "Chords", "UK garage chord stab: a short organ-like hit with a little resonance, swung and punchy.",
    ["ukg", "garage", "stab"],
    osc("a", "Ion Drive", unison=2, detune=0.04, width=0.8, morph=0.45, level=0.75), osc("b", "Classic", morph=0.6, oct=1, level=0.4),
    filt("LP 24", 2600, 0.3, env=24, key=0.6, vel=0.3), adsr("amp", 0.001, 0.39, 0.45, 0.3), adsr("fenv", 0.001, 0.14, 0.0, 0.1),
    {"polyphony": 10},
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.3), ("Orbit", "Morph A", 0.3),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Delay", 0.25)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.5, 0.3, 0.25), delay=("1/16.", 0.25, 0.5, 0.08, True), reverb=(1.0, 1.1, 0.5, 6, 0.12), squash=0.6, mono_low=220))

add("Velvet Pulsar Rhodes", "Chords", "Rhodes-style tine keys: warm and bell-tipped, rounding off as the note decays.",
    ["rhodes", "ep", "tine"],
    osc("a", "Classic", unison=2, detune=0.04, width=0.8, morph=0.0, level=0.8, warp="FM from B", warpAmt=0.28), osc("b", "Classic", morph=0.0, oct=1, level=0.0),
    filt("LP 12", 7000, 0.05, key=0.5, vel=0.4), adsr("amp", 0.001, 2.86, 0.45, 0.5), adsr("menv", 0.001, 0.35, 0.05, 0.3),
    lfo(1, "Sine", 3.6, retrig=False),
    {"polyphony": 12},
    mods(("Mod Env", "Warp A", 0.4), ("Aftertouch", "Cutoff", 0.2), ("LFO 1", "Pan", 0.3), ("Heat", "Warp A", 0.3),
         ("Gravity", "Sub Level", 0.25), ("Orbit", "LFO 1 Amount", 0.6), ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "LFO 1 Rate", 0.3)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.35, 0.45, 0.35), reverb=(1.2, 1.5, 0.5, 10, 0.13), squash=0.6, mono_low=220))

add("Starlight Supersaw Wall", "Chords", "A wall of supersaw chords for drops: huge, bright and wide, glued with squash.",
    ["supersaw", "edm", "wide"],
    osc("a", "Classic", morph=0.5, unison=8, detune=0.32, width=1.0), osc("b", "Classic", morph=0.5, oct=1, unison=6, detune=0.28, width=1.0, level=0.45),
    filt("LP 12", 9000, 0.1, key=0.5, vel=0.2), adsr("amp", 0.005, 0.6, 0.85, 0.35),
    {"polyphony": 10, "subLevel": 0.1},
    mods(("Velocity", "Amp", 0.2), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Resonance", 0.3), ("Aftertouch", "Cutoff", 0.2), ("Flux", "FX Delay", 0.25)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), delay=("1/8.", 0.3, 0.45, 0.1, True), reverb=(1.8, 2.4, 0.4, 15, 0.2), squash=0.6, mono_low=220, width=1.3))

add("Cosmic Disco Clav Chord", "Chords", "Funky clav-like chord stab: bright, percussive, a little resonant snap on top.",
    ["funk", "clav", "stab"],
    osc("a", "Classic", unison=2, detune=0.05, width=0.8, morph=0.85, level=0.75), osc("b", "Pulsar", morph=0.5, oct=1, level=0.3),
    filt("LP 12", 5000, 0.4, env=24, key=0.7, vel=0.45), adsr("amp", 0.001, 0.52, 0.45, 0.3), adsr("fenv", 0.001, 0.12, 0.1, 0.08),
    {"polyphony": 10},
    mods(("Velocity", "Cutoff", 0.35), ("Heat", "Resonance", 0.25), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.3),
         ("Orbit", "Morph A", -0.3), ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Filter Drive", 0.4)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.6, 0.3, 0.25), delay=("1/16", 0.2, 0.5, 0.08, True), reverb=(0.9, 1.0, 0.5, 5, 0.1), squash=0.6, mono_low=220))

add("Polar Cap Poly", "Chords", "Bright polysynth for pop progressions: crisp saws, a gentle bloom and a clean hall.",
    ["pop", "poly", "bright"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.16, width=0.9), osc("b", "Fraunhofer", morph=0.4, oct=1, level=0.3),
    filt("LP 12", 8000, 0.12, env=10, key=0.5, vel=0.3), adsr("amp", 0.005, 0.91, 0.7, 0.4), adsr("fenv", 0.01, 0.6, 0.5, 0.4),
    {"polyphony": 10},
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", -0.3), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Morph B", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Chorus", 0.3)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.4, 0.4, 0.25), reverb=(1.5, 1.8, 0.4, 12, 0.16), squash=0.6, mono_low=220, width=1.2))

add("Moonlit Felt Keys", "Chords", "Soft felt-like keys for intimate chords: a muted hammer, warm body and a little room.",
    ["felt", "keys", "soft"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.8, morph=0.08, level=0.75, warp="FM from B", warpAmt=0.2), osc("b", "Classic", morph=0.0, oct=1, level=0.3),
    filt("LP 12", 7000, 0.05, env=18, key=0.6, vel=0.5), adsr("amp", 0.002, 2.34, 0.45, 0.5), adsr("fenv", 0.001, 0.2, 0.1, 0.2),
    {"polyphony": 12, "noiseLevel": 0.03, "noiseType": "Pink"},
    mods(("Velocity", "Cutoff", 0.4), ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.25), ("Orbit", "Detune", 0.25),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Noise Level", 0.3), ("Aftertouch", "Cutoff", 0.2), ("Key", "Cutoff", 0.15)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.3, 0.3, 0.2), reverb=(1.0, 1.3, 0.55, 6, 0.16), squash=0.6, mono_low=220))

add("Hyperion Hall Stab", "Chords", "Orchestral-hit style stab: brass, strings and a choir-ish formant in one huge chord.",
    ["orchestral", "hit", "stab"],
    osc("a", "Classic", morph=0.5, unison=5, detune=0.2, width=0.9), osc("b", "Nebula", morph=0.5, oct=1, level=0.45),
    filt("Formant", 3200, 0.4, env=8, key=0.4, vel=0.25), adsr("amp", 0.001, 1.04, 0.45, 0.5), adsr("fenv", 0.001, 0.3, 0.0, 0.3),
    {"polyphony": 10, "noiseLevel": 0.05},
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.3), ("Orbit", "Morph B", 0.35),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Resonance", 0.25), ("Flux", "FX Delay", 0.2)),
    fx(drive=("Tube", 0.22, 0.75, 1.0), reverb=(2.0, 2.5, 0.45, 15, 0.22), squash=0.6, mono_low=220, width=1.2))

add("Aurora Glass Harp Chord", "Chords", "Strummed glass chord: a bright, airy chord whose notes ring out against each other.",
    ["glass", "strum", "airy"],
    osc("a", "Fraunhofer", morph=0.3, unison=2, detune=0.1, width=0.8), osc("b", "Quasar", morph=0.5, oct=1, level=0.35),
    filt("HP 12", 300, 0.1, key=0.0, vel=0.2), adsr("amp", 0.001, 1.82, 0.45, 0.9),
    {"polyphony": 12},
    mods(("Velocity", "Amp", 0.25), ("Heat", "Morph A", 0.35), ("Gravity", "Cutoff", -0.25), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Morph B", 0.35), ("Aftertouch", "Level B", 0.3), ("Flux", "FX Delay", 0.25)),
    fx(drive=("Soft", 0.3, 0.9, 1.0), chorus=(0.4, 0.4, 0.3), delay=("1/8.", 0.3, 0.4, 0.12, True), reverb=(1.8, 2.6, 0.35, 15, 0.22), squash=0.6, mono_low=220, width=1.3))

# =====================================================================================================
# MELODY
# =====================================================================================================
add("Starling Pluck", "Melody", "A bright, clean pluck that sings a melody clearly: saw snap, quick decay, airy echo.",
    ["pluck", "bright", "melody"],
    osc("a", "Classic", morph=0.5, unison=2, detune=0.12, width=0.8), osc("b", "Fraunhofer", morph=0.4, oct=1, level=0.3),
    filt("LP 12", 3400, 0.2, env=36, key=0.7, vel=0.35), adsr("amp", 0.001, 0.6, 0.18, 0.45), adsr("fenv", 0.001, 0.22, 0.0, 0.2),
    mods(("Velocity", "Cutoff", 0.35), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", -0.3), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.3)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), delay=("1/8.", 0.35, 0.35, 0.2, True), reverb=(1.3, 1.6, 0.35, 10, 0.16), squash=0.45, mono_low=200, width=1.2))

add("Moonstone Pluck", "Melody", "A soft, round pluck: a sine-and-triangle tone with a gentle bloom, mellow and warm.",
    ["pluck", "soft", "round"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.35, level=0.75), osc("b", "Classic", morph=0.25, oct=1, level=0.5, fine=6),
    filt("LP 12", 10000, 0.1, env=30, key=0.7, vel=0.35), adsr("amp", 0.002, 0.84, 0.18, 0.45), adsr("fenv", 0.001, 0.3, 0.0, 0.2),
    mods(("Velocity", "Cutoff", 0.35), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.25), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Morph A", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.3)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.4, 0.4, 0.35), delay=("1/8.", 0.3, 0.5, 0.16, True), reverb=(1.2, 1.6, 0.45, 10, 0.18), squash=0.45, mono_low=200, width=1.2))

add("Tongue Of Triton", "Melody", "Kalimba: a metal tongue plucked by the thumb, with a woody knock and ringing overtones.",
    ["kalimba", "mallet", "african"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.65, morph=0.0, level=0.7, warp="FM from B", warpAmt=0.12), osc("b", "Classic", morph=0.0, oct=2, semi=7, level=0.0),
    filt("LP 12", 6000, 0.05, key=0.4, vel=0.3), adsr("amp", 0.001, 1.08, 0.18, 0.5), adsr("menv", 0.001, 0.08, 0.0, 0.06),
    {"noiseLevel": 0.08, "noiseType": "Pink"},
    mods(("Mod Env", "Warp A", 0.2), ("Mod Env", "Noise Level", 0.4), ("Aftertouch", "Cutoff", 0.2), ("Heat", "Warp A", 0.3),
         ("Gravity", "Sub Level", 0.2), ("Orbit", "Detune", 0.25), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Warp A", 0.3)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), delay=("1/8.", 0.3, 0.45, 0.14, True), reverb=(1.2, 1.5, 0.4, 8, 0.18), squash=0.45, mono_low=200, width=1.2))

add("Rosewood Rings", "Melody", "Marimba: a warm wooden bar with a soft mallet, quick and resonant.",
    ["marimba", "mallet", "wood"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.0, level=0.75, warp="FM from B", warpAmt=0.15), osc("b", "Classic", morph=0.0, oct=2, level=0.0),
    filt("LP 12", 3500, 0.15, env=12, key=0.9, vel=0.35), adsr("amp", 0.001, 0.66, 0.18, 0.45), adsr("fenv", 0.001, 0.1, 0.0, 0.1),
    adsr("menv", 0.001, 0.06, 0.0, 0.05),
    mods(("Mod Env", "Warp A", 0.3), ("Velocity", "Cutoff", 0.35), ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.2),
         ("Orbit", "Detune", 0.25), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Resonance", 0.3), ("Aftertouch", "Cutoff", 0.2)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.3, 0.3, 0.2), reverb=(1.1, 1.3, 0.45, 8, 0.18), squash=0.45, mono_low=200, width=1.2))

add("Vega Vibes", "Melody", "Vibraphone: a sweet metal bar with a motor tremolo shimmering across the stereo field.",
    ["vibraphone", "mallet", "tremolo"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.65, morph=0.0, level=0.75, warp="FM from B", warpAmt=0.2), osc("b", "Classic", morph=0.0, oct=2, level=0.0),
    filt("LP 12", 10000, 0.05, key=0.4, vel=0.3), adsr("amp", 0.001, 2.4, 0.18, 1.2), adsr("menv", 0.001, 0.12, 0.0, 0.1),
    lfo(1, "Sine", 5.0, retrig=False),
    mods(("LFO 1", "Pan", 0.5), ("Mod Env", "Warp A", 0.35), ("Aftertouch", "Cutoff", 0.2), ("Heat", "Warp A", 0.3),
         ("Gravity", "Sub Level", 0.2), ("Orbit", "LFO 1 Amount", 0.6), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "LFO 1 Rate", 0.3)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), reverb=(1.4, 2.0, 0.4, 10, 0.2), squash=0.45, mono_low=200, width=1.2))

add("Crystal Spire Bell", "Melody", "Glass bell: a clear, inharmonic strike that rings long and bright.",
    ["bell", "glass", "bright"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.0, level=0.7, warp="FM from B", warpAmt=0.15), osc("b", "Classic", morph=0.0, oct=1, semi=11, level=0.0),
    filt("LP 12", 8000, 0.05, key=0.3), adsr("amp", 0.001, 2.88, 0.18, 1.6), adsr("menv", 0.001, 0.6, 0.1, 0.5),
    mods(("Mod Env", "Warp A", 0.15), ("Velocity", "Warp A", 0.3), ("Heat", "Warp A", 0.3), ("Gravity", "Cutoff", -0.2),
         ("Orbit", "Detune", 0.25), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Warp A", 0.3), ("Aftertouch", "Level B", 0.3)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.3, 0.3, 0.25), delay=("1/4", 0.3, 0.4, 0.12, True), reverb=(1.8, 2.8, 0.3, 15, 0.22), squash=0.45, mono_low=200, width=1.2))

add("Clockwork Comet Box", "Melody", "Music box: tiny tuned tines plucked by a pinned drum, twinkling and delicate.",
    ["music box", "twinkle", "delicate"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.0, oct=1, level=0.7, warp="FM from B", warpAmt=0.12), osc("b", "Classic", morph=0.0, oct=3, level=0.0),
    filt("LP 12", 7500, 0.05, key=0.3), adsr("amp", 0.001, 1.32, 0.18, 0.8), adsr("menv", 0.001, 0.15, 0.0, 0.1),
    mods(("Mod Env", "Warp A", 0.15), ("Velocity", "Amp", 0.25), ("Heat", "Warp A", 0.3), ("Gravity", "Cutoff", -0.2),
         ("Orbit", "Detune", 0.25), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Warp A", 0.3), ("Aftertouch", "Cutoff", 0.2)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.4, 0.3, 0.25), reverb=(1.4, 2.0, 0.35, 10, 0.22), squash=0.45, mono_low=200, width=1.2))

add("Celestia Keys", "Melody", "Celesta: soft hammers on steel plates, sweet and sparkling, made for little melodies.",
    ["celesta", "sparkle", "keys"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.0, level=0.75, warp="FM from B", warpAmt=0.1), osc("b", "Classic", morph=0.0, oct=2, level=0.0),
    filt("LP 12", 6000, 0.05, key=0.4, vel=0.3), adsr("amp", 0.001, 1.68, 0.18, 0.9), adsr("menv", 0.001, 0.2, 0.0, 0.15),
    mods(("Mod Env", "Warp A", 0.15), ("Velocity", "Warp A", 0.3), ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.2),
         ("Orbit", "Detune", 0.25), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Warp A", 0.3), ("Aftertouch", "Cutoff", 0.2)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.35, 0.35, 0.3), reverb=(1.4, 1.8, 0.35, 10, 0.2), squash=0.45, mono_low=200, width=1.2))

add("Harp Of Hydra", "Melody", "Concert-harp-like pluck: bright string snap with a resonant body, rippling into the hall.",
    ["harp", "string", "pluck"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.55, level=0.7), osc("b", "Classic", morph=0.1, oct=1, level=0.35),
    filt("LP 12", 8000, 0.15, env=36, key=0.8, vel=0.35), adsr("amp", 0.001, 1.56, 0.18, 0.8), adsr("fenv", 0.001, 0.3, 0.0, 0.3),
    mods(("Velocity", "Cutoff", 0.35), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", -0.3), ("Orbit", "Detune", 0.25),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Resonance", 0.3), ("Aftertouch", "Cutoff", 0.25), ("Key", "Cutoff", 0.15)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.3, 0.3, 0.25), reverb=(1.6, 2.2, 0.4, 12, 0.22), squash=0.45, mono_low=200, width=1.2))

add("Kyoto Orbit Koto", "Melody", "Koto-like pluck: a twangy, slightly bent string with a woody resonance.",
    ["koto", "string", "asian"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.58, level=0.7), osc("b", "Plasma", morph=0.2, oct=1, level=0.25),
    filt("LP 12", 6500, 0.3, env=24, key=0.8, vel=0.35), adsr("amp", 0.001, 1.08, 0.18, 0.5), adsr("fenv", 0.001, 0.18, 0.0, 0.15),
    adsr("menv", 0.001, 0.08, 0.0, 0.06),
    mods(("Mod Env", "Pitch", 0.02), ("Velocity", "Cutoff", 0.35), ("Heat", "Resonance", 0.25), ("Gravity", "Level B", -0.3),
         ("Orbit", "Morph A", 0.3), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Resonance", 0.3), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.3, 0.3, 0.2), delay=("1/8.", 0.25, 0.5, 0.1, True), reverb=(1.3, 1.6, 0.4, 10, 0.2), squash=0.45, mono_low=200, width=1.2))

add("Island Moon Steel", "Melody", "Steel pan: a ringing, wobbling metal note with the characteristic hollow overtone.",
    ["steel pan", "caribbean", "metallic"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.0, level=0.7, warp="Ring by B", warpAmt=0.6), osc("b", "Classic", morph=0.0, oct=1, semi=0, fine=12, level=0.0),
    filt("LP 12", 10000, 0.1, key=0.5, vel=0.3), adsr("amp", 0.002, 1.2, 0.18, 0.5), adsr("menv", 0.001, 0.25, 0.1, 0.2),
    lfo(1, "Sine", 6.0, retrig=True),
    mods(("Mod Env", "Warp A", 0.3), ("LFO 1", "Amp", 0.08), ("Aftertouch", "Cutoff", 0.2), ("Heat", "Warp A", 0.3),
         ("Gravity", "Sub Level", 0.2), ("Orbit", "LFO 1 Amount", 0.6), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "LFO 1 Rate", 0.3)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.3, 0.3, 0.25), reverb=(1.2, 1.5, 0.4, 8, 0.18), squash=0.45, mono_low=200, width=1.2))

add("Breath Of Belinda", "Melody", "Flute-like melody voice: a breathy sine with a slow vibrato that blooms on long notes.",
    ["flute", "breathy", "soft"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.3, level=0.75), osc("b", "Classic", morph=0.0, oct=1, level=0.3),
    filt("LP 12", 9000, 0.05, key=0.5, vel=0.3), adsr("amp", 0.04, 0.72, 0.8, 0.45),
    lfo(1, "Sine", 5.2, retrig=True, fade=0.5),
    {"noiseLevel": 0.2, "noiseType": "Pink"},
    mods(("LFO 1", "Pitch", 0.006), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Velocity", "Noise Level", 0.3), ("Heat", "Noise Level", 0.3),
         ("Gravity", "Level B", -0.2), ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Reverb", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.3, 0.35, 0.3), delay=("1/8.", 0.3, 0.55, 0.12, True), reverb=(1.4, 1.8, 0.45, 12, 0.2), squash=0.45, mono_low=200, width=1.2))

add("Lunar Whistler", "Melody", "A pure whistle for melodies: thin and bright, with a touch of breath and a lilting vibrato.",
    ["whistle", "pure", "bright"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.65, morph=0.0, oct=1, level=0.75), osc("b", "Classic", morph=0.2, oct=2, level=0.25),
    filt("LP 12", 9000, 0.05, key=0.5), adsr("amp", 0.03, 0.5, 0.85, 0.2),
    lfo(1, "Sine", 5.8, retrig=True, fade=0.35),
    {"noiseLevel": 0.15, "noiseType": "White", "voiceMode": "Legato", "glide": 0.05},
    mods(("LFO 1", "Pitch", 0.008), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Velocity", "Amp", 0.2), ("Heat", "Noise Level", 0.3),
         ("Gravity", "Level B", -0.2), ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Pitch", 0.01)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), delay=("1/8.", 0.35, 0.4, 0.2, True), reverb=(1.4, 1.8, 0.4, 12, 0.2), squash=0.45, mono_low=200, width=1.2))

add("Mallet Of Mimas", "Melody", "Synth mallet: a round, hollow bar sound that sits between marimba and synth bell.",
    ["mallet", "synth", "hollow"],
    osc("a", "Pulsar", unison=2, detune=0.03, width=0.4, morph=0.2, level=0.7), osc("b", "Classic", morph=0.0, oct=2, level=0.25),
    filt("LP 12", 4500, 0.2, env=18, key=0.9, vel=0.35), adsr("amp", 0.001, 0.84, 0.18, 0.45), adsr("fenv", 0.001, 0.12, 0.0, 0.1),
    mods(("Velocity", "Cutoff", 0.35), ("Heat", "Morph A", 0.35), ("Gravity", "Level B", -0.3), ("Orbit", "Detune", 0.25),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Resonance", 0.3), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.3)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.35, 0.35, 0.25), delay=("1/8.", 0.3, 0.45, 0.14, True), reverb=(1.2, 1.5, 0.4, 8, 0.18), squash=0.45, mono_low=200, width=1.2))

add("Belltower Pluck", "Melody", "Bell-pluck: a pluck with a bell's shine on the attack and a soft, round tail.",
    ["bell", "pluck", "shine"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.5, level=0.6), osc("b", "Classic", morph=0.0, oct=2, level=0.55, fine=3),
    filt("LP 12", 10000, 0.15, env=36, key=0.7, vel=0.35), adsr("amp", 0.001, 0.96, 0.18, 0.5), adsr("fenv", 0.001, 0.25, 0.0, 0.2),
    mods(("Velocity", "Cutoff", 0.35), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", -0.3), ("Orbit", "Detune", 0.25),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Morph A", 0.35), ("Aftertouch", "Level B", 0.3), ("Flux", "FX Reverb", 0.3)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.35, 0.35, 0.3), delay=("1/8.", 0.3, 0.4, 0.15, True), reverb=(1.4, 1.8, 0.35, 10, 0.2), squash=0.45, mono_low=200, width=1.2))

add("Hollow Sky Pluck", "Melody", "Hollow pluck: a square-ish, woody tone with a soft filter snap and plenty of space.",
    ["pluck", "hollow", "woody"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.75, level=0.7), osc("b", "Classic", morph=0.75, oct=1, level=0.25, fine=-5),
    filt("LP 12", 6500, 0.25, env=20, key=0.8, vel=0.35), adsr("amp", 0.001, 0.66, 0.18, 0.45), adsr("fenv", 0.001, 0.15, 0.0, 0.12),
    mods(("Velocity", "Cutoff", 0.35), ("Heat", "Resonance", 0.25), ("Gravity", "Level B", -0.3), ("Orbit", "Morph A", 0.3),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Cutoff", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.3)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.35, 0.4, 0.3), delay=("1/8.", 0.35, 0.45, 0.18, True), reverb=(1.3, 1.6, 0.4, 10, 0.18), squash=0.45, mono_low=200, width=1.2))

add("Siren Of Saturn", "Melody", "Vocal formant pluck: an 'ah' that snaps to 'oh', like a chopped vocal playing the melody.",
    ["vocal", "formant", "chop"],
    osc("a", "Classic", morph=0.5, unison=2, detune=0.1, width=0.35), osc("b", "Nebula", morph=0.4, level=0.35),
    filt("Formant", 2400, 0.4, env=14, key=0.4, vel=0.3), adsr("amp", 0.002, 0.54, 0.2, 0.45), adsr("fenv", 0.001, 0.2, 0.0, 0.15),
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.3), ("Orbit", "Morph B", 0.35),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Resonance", 0.25), ("Flux", "FX Reverb", 0.3)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.3, 0.35, 0.25), delay=("1/8.", 0.3, 0.45, 0.15, True), reverb=(1.2, 1.5, 0.4, 10, 0.16), squash=0.4, mono_low=200, width=1.2))

add("Dusty Orrery", "Melody", "Lo-fi music box: a warped, slightly detuned tinkle through crackle and a soft tape crush.",
    ["lo-fi", "music box", "dusty"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.0, oct=1, level=0.7, warp="FM from B", warpAmt=0.1), osc("b", "Classic", morph=0.0, oct=3, level=0.0),
    filt("LP 12", 6000, 0.1, key=0.3, vel=0.3), adsr("amp", 0.001, 1.2, 0.18, 0.7), adsr("menv", 0.001, 0.15, 0.0, 0.1),
    lfo(1, "Drift", 0.8, retrig=False),
    {"drift": 0.6, "noiseLevel": 0.05, "noiseType": "Crackle"},
    mods(("LFO 1", "Pitch", 0.008), ("Mod Env", "Warp A", 0.15), ("Heat", "Warp A", 0.3), ("Gravity", "Cutoff", -0.2),
         ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Cutoff", 0.2)),
    fx(drive=("Cosmic Ray", 0.2, 0.6, 0.35), chorus=(0.35, 0.6, 0.4), reverb=(1.2, 1.6, 0.5, 10, 0.2), squash=0.45, mono_low=200, width=1.2))

add("Chime Of Charon", "Melody", "Chime keys: tubular-chime shimmer on a keyboard, crisp and glistening.",
    ["chime", "keys", "shimmer"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.0, level=0.7, warp="FM from B", warpAmt=0.14), osc("b", "Classic", morph=0.0, oct=2, semi=4, level=0.0),
    filt("LP 12", 7000, 0.05, key=0.3), adsr("amp", 0.001, 1.92, 0.18, 1.2), adsr("menv", 0.001, 0.3, 0.05, 0.2),
    mods(("Mod Env", "Warp A", 0.15), ("Velocity", "Warp A", 0.3), ("Heat", "Warp A", 0.3), ("Gravity", "Cutoff", -0.2),
         ("Orbit", "Detune", 0.25), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Warp A", 0.3), ("Aftertouch", "Level B", 0.3)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.35, 0.3, 0.3), delay=("1/4", 0.25, 0.4, 0.1, True), reverb=(1.6, 2.4, 0.35, 12, 0.22), squash=0.45, mono_low=200, width=1.2))

add("Rover Toy Piano", "Melody", "Toy piano: a tinny, clunky little hammer on metal rods, charming and a bit out of tune.",
    ["toy piano", "tinny", "cute"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.0, level=0.7, warp="FM from B", warpAmt=0.12), osc("b", "Classic", morph=0.0, oct=2, semi=5, fine=-15, level=0.0),
    filt("LP 12", 6000, 0.15, key=0.7, vel=0.35), adsr("amp", 0.001, 0.72, 0.18, 0.45), adsr("menv", 0.001, 0.1, 0.0, 0.08),
    {"noiseLevel": 0.06, "noiseType": "White", "drift": 0.4},
    mods(("Mod Env", "Warp A", 0.15), ("Mod Env", "Noise Level", 0.3), ("Aftertouch", "Cutoff", 0.2), ("Heat", "Warp A", 0.3),
         ("Gravity", "Sub Level", 0.2), ("Orbit", "Detune", 0.3), ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Warp A", 0.3)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.4, 0.3, 0.25), reverb=(1.0, 1.2, 0.45, 6, 0.18), squash=0.45, mono_low=200, width=1.2))

add("Electric Tine Of Io", "Melody", "Bright FM tine: a DX-style electric piano tone tuned for single-note melodies, crisp and singing.",
    ["fm", "tine", "bright"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.0, level=0.75, warp="FM from B", warpAmt=0.42), osc("b", "Classic", morph=0.0, oct=1, level=0.0),
    filt("LP 12", 11000, 0.05, key=0.4, vel=0.3), adsr("amp", 0.001, 1.68, 0.2, 0.5), adsr("menv", 0.001, 0.35, 0.1, 0.3),
    lfo(1, "Sine", 5.0, retrig=True, fade=0.4),
    mods(("Mod Env", "Warp A", 0.35), ("LFO 1", "Pitch", 0.004), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Aftertouch", "Cutoff", 0.2),
         ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.2), ("Orbit", "Detune", 0.25), ("Flux", "FX Reverb", 0.35)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.4, 0.4, 0.35), delay=("1/8.", 0.3, 0.45, 0.14, True), reverb=(1.3, 1.6, 0.4, 10, 0.18), squash=0.45, mono_low=200, width=1.2))

add("Sine Of Selene", "Melody", "Sine bell melody: a pure, glowing tone with a faint bell partial and a long, gentle tail.",
    ["sine", "bell", "pure"],
    osc("a", "Classic", unison=2, detune=0.03, width=0.4, morph=0.0, level=0.75), osc("b", "Classic", morph=0.0, oct=2, semi=7, level=0.35),
    filt("LP 12", 12000, 0.05, key=0.3), adsr("amp", 0.002, 1.92, 0.18, 1.2),
    mods(("Velocity", "Amp", 0.25), ("Heat", "Level B", 0.35), ("Gravity", "Sub Level", 0.25), ("Orbit", "Detune", 0.25),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Level B", 0.3), ("Aftertouch", "Level B", 0.25), ("Flux", "FX Delay", 0.25)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), chorus=(0.35, 0.4, 0.35), delay=("1/8.", 0.3, 0.45, 0.15, True), reverb=(1.6, 2.2, 0.35, 12, 0.22), squash=0.45, mono_low=200, width=1.2))

add("Comet Spark Pluck", "Melody", "Future pluck for melodies: a punchy, pitch-flicked saw pluck with a crisp echo, very modern.",
    ["future", "pluck", "edm"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.2, width=0.9), osc("b", "Plasma", morph=0.3, oct=1, level=0.3),
    filt("LP 12", 2800, 0.2, env=42, key=0.7, vel=0.35), adsr("amp", 0.001, 0.43, 0.18, 0.45), adsr("fenv", 0.001, 0.18, 0.0, 0.15),
    adsr("menv", 0.001, 0.04, 0.0, 0.04),
    mods(("Mod Env", "Pitch", 0.06), ("Velocity", "Cutoff", 0.35), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.3),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Morph B", 0.35), ("Aftertouch", "Resonance", 0.25)),
    fx(drive=("Soft", 0.22, 0.95, 1.0), delay=("1/16.", 0.35, 0.3, 0.2, True), reverb=(1.3, 1.6, 0.35, 10, 0.16), squash=0.6, mono_low=200, width=1.3))
