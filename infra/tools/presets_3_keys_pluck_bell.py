# More factory presets: Keys, Pluck and Bell (exec'd by make_presets.py with its helpers in scope).

# =====================================================================================================
# KEYS
# =====================================================================================================
add("Tine Of Titan", "Keys", "A warm tine piano: a soft sine body with a bell-bright bark that grows with velocity.",
    ["ep", "rhodes", "warm"],
    osc("a", "Classic", morph=0.0, level=0.8), osc("b", "Pulsar", morph=0.2, oct=2, level=0.25),
    filt("LP 12", 3200, 0.1, env=18, key=0.6, vel=0.4), adsr("amp", 0.002, 2.6, 0.25, 0.45), adsr("fenv", 0.001, 0.6, 0.1, 0.4),
    adsr("menv", 0.001, 0.25, 0.0, 0.2),
    lfo(1, "Sine", 4.8),
    mods(("Velocity", "Level B", 0.45), ("LFO 1", "Amp", 0.25), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "FX Chorus", 0.5), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Aftertouch", "Cutoff", 0.2)),
    fx(drive=("Tube", 0.12, 0.5, 1.0), chorus=(0.6, 0.35, 0.2), reverb=(1.1, 1.8, 0.5, 10, 0.12)))

add("Wurlitzer Wanderer", "Keys", "A reedy, slightly nasal electric piano that barks when you dig in.",
    ["ep", "wurli", "reedy"],
    osc("a", "Classic", morph=0.62, level=0.75), osc("b", "Classic", morph=0.0, oct=1, level=0.3),
    filt("LP 24", 1800, 0.2, env=24, key=0.7, vel=0.5, drive=0.15), adsr("amp", 0.002, 1.8, 0.3, 0.25), adsr("fenv", 0.001, 0.35, 0.15, 0.25),
    lfo(1, "Sine", 5.5),
    mods(("Velocity", "Filter Drive", 0.4), ("Heat", "Cutoff", 0.4), ("Gravity", "Level B", 0.35), ("Orbit", "LFO 1 Amount", 0.6),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Aftertouch", "Resonance", 0.25), ("LFO 1", "Amp", 0.3)),
    fx(drive=("Soft", 0.25, 0.55, 1.0), reverb=(0.9, 1.2, 0.5, 5, 0.1)))

add("Pulsar Clavinet", "Keys", "A funky, percussive clav: a narrow pulse through a snapping band-pass.",
    ["clav", "funk", "percussive"],
    osc("a", "Classic", morph=0.9, level=0.8), osc("b", "Pulsar", morph=0.6, level=0.3, fine=4),
    filt("BP 12", 1500, 0.35, env=30, key=0.8, vel=0.4), adsr("amp", 0.001, 0.9, 0.15, 0.08), adsr("fenv", 0.001, 0.15, 0.1, 0.1),
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph A", -0.3),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Filter Drive", 0.4), ("Key", "Morph B", 0.2)),
    fx(drive=("Fold", 0.15, 0.6, 0.6), delay=("1/16", 0.2, 0.5, 0.0, True), reverb=()))

add("Drawbar Observatory", "Keys", "Full drawbars through a spinning cabinet: the mod wheel throws it to fast.",
    ["organ", "drawbar", "rotary"],
    osc("a", "Ion Drive", morph=0.35, level=0.7), osc("b", "Ion Drive", morph=0.6, oct=1, level=0.45),
    filt("LP 12", 7000, 0.05, key=0.3), adsr("amp", 0.003, 0.1, 1.0, 0.06),
    lfo(1, "Sine", 0.8, retrig=False), lfo(2, "Triangle", 0.7, retrig=False),
    {"subLevel": 0.35, "subShape": "Sine", "drift": 0.1},
    mods(("LFO 1", "Pan", 0.4), ("LFO 2", "Cutoff", 0.06), ("Mod Wheel", "LFO 1 Rate", 0.6), ("Heat", "Filter Drive", 0.5),
         ("Gravity", "Sub Level", 0.5), ("Orbit", "FX Chorus", 0.6), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Morph B", 0.3)),
    fx(drive=("Tube", 0.2, 0.6, 1.0), chorus=(0.9, 0.3, 0.15), reverb=(1.2, 1.6, 0.5, 8, 0.12)))

add("Click Track Organ", "Keys", "A percussive organ with a key click and a second-harmonic blip that fades fast.",
    ["organ", "percussive", "click"],
    osc("a", "Ion Drive", morph=0.15, level=0.75), osc("b", "Classic", morph=0.0, oct=1, semi=7, level=0.4),
    filt("LP 24", 6000, 0.05, key=0.4), adsr("amp", 0.001, 0.2, 0.9, 0.05), adsr("menv", 0.001, 0.25, 0.0, 0.1),
    {"noiseLevel": 0.08, "noiseType": "Crackle"},
    mods(("Mod Env", "Level B", 0.5), ("Mod Env", "Noise Level", 0.3), ("Heat", "Morph A", 0.4), ("Gravity", "Sub Level", 0.5),
         ("Orbit", "FX Chorus", 0.5), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Cutoff", -0.3), ("Aftertouch", "Filter Drive", 0.4)),
    fx(chorus=(0.5, 0.25, 0.1), reverb=(1.0, 1.4, 0.5, 6, 0.1)))

add("Ground Station Organ", "Keys", "A house organ stab: hollow, punchy, made for offbeat chords.",
    ["house organ", "stab", "90s"],
    osc("a", "Classic", morph=0.75, level=0.7), osc("b", "Classic", morph=0.75, oct=1, level=0.45, fine=5),
    filt("LP 24", 2500, 0.25, env=20, key=0.5), adsr("amp", 0.001, 0.35, 0.4, 0.1), adsr("fenv", 0.001, 0.2, 0.2, 0.1),
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.4), ("Gravity", "Sub Level", 0.45), ("Orbit", "Morph A", 0.3),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.25)),
    fx(delay=("1/8.", 0.25, 0.6, 0.12, True), reverb=(1.0, 1.2, 0.5, 5, 0.12)))

add("Digital Piano 1984", "Keys", "A crystalline FM piano from the era of the first space shuttles.",
    ["fm", "dx", "80s"],
    osc("a", "Classic", morph=0.0, level=0.75, warp="FM from B", warpAmt=0.35), osc("b", "Classic", morph=0.0, oct=1, level=0.15),
    filt("LP 12", 9000, 0.05, key=0.5), adsr("amp", 0.001, 2.2, 0.2, 0.4), adsr("menv", 0.001, 0.6, 0.1, 0.3),
    mods(("Mod Env", "Warp A", 0.35), ("Velocity", "Warp A", 0.3), ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "FX Chorus", 0.6), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Morph A", 0.3), ("Aftertouch", "Cutoff", -0.2)),
    fx(chorus=(0.7, 0.4, 0.25), reverb=(1.3, 2.0, 0.45, 12, 0.15)))

add("Toy Rover", "Keys", "A toy piano left on Mars: thin tines, a little out of tune, very small.",
    ["toy piano", "cute", "lo-fi"],
    osc("a", "Quasar", morph=0.2, level=0.6), osc("b", "Classic", morph=0.0, oct=2, semi=7, level=0.3, fine=-12),
    filt("HP 12", 400, 0.1, key=0.5), adsr("amp", 0.001, 0.7, 0.0, 0.3), adsr("menv", 0.001, 0.1, 0.0, 0.1),
    {"drift": 0.4},
    mods(("Mod Env", "Level B", 0.4), ("Heat", "Morph A", 0.4), ("Gravity", "Cutoff", -0.3), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Resonance", 0.3), ("Flux", "FX Reverb", 0.3)),
    fx(drive=("Cosmic Ray", 0.15, 0.5, 0.4), delay=("1/8", 0.25, 0.6, 0.1, True), reverb=(0.8, 1.0, 0.5, 5, 0.12)))

add("Vibraphone Of Vega", "Keys", "Soft mallets on metal bars with the motor on: the tremolo follows the mod wheel.",
    ["vibraphone", "mallet", "jazz"],
    osc("a", "Classic", morph=0.0, level=0.75), osc("b", "Classic", morph=0.0, oct=2, level=0.2),
    filt("LP 12", 5000, 0.05, key=0.6, vel=0.3), adsr("amp", 0.001, 3.0, 0.0, 1.2), adsr("menv", 0.001, 0.12, 0.0, 0.1),
    lfo(1, "Sine", 5.0, retrig=False),
    mods(("LFO 1", "Amp", 0.2), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Mod Env", "Level B", 0.5), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Sub Level", 0.35), ("Orbit", "LFO 1 Rate", 0.35), ("Flux", "FX Reverb", 0.45), ("Aftertouch", "Level B", 0.3)),
    fx(reverb=(1.4, 2.4, 0.5, 15, 0.18)))

add("Marimba Keys", "Keys", "Hollow wooden keys tuned in a formant filter: woody, round and quick.",
    ["mallet", "wood", "formant"],
    osc("a", "Nebula", morph=0.3, level=0.75), osc("b", "Classic", morph=0.0, oct=2, level=0.2),
    filt("Formant", 900, 0.4, env=12, key=0.7), adsr("amp", 0.001, 0.8, 0.0, 0.25), adsr("fenv", 0.001, 0.12, 0.0, 0.1),
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Morph A", 0.4), ("Gravity", "Sub Level", 0.4), ("Orbit", "Cutoff", 0.15),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Morph A", 0.3), ("Key", "Morph A", -0.15)),
    fx(reverb=(1.0, 1.4, 0.5, 8, 0.12)))

add("Dark Room Rhodes", "Keys", "A dusty tine piano through a tube: low, intimate, a bit of grit under the hammer.",
    ["ep", "lo-fi", "dark"],
    osc("a", "Classic", morph=0.08, level=0.8), osc("b", "Dark Matter", morph=0.1, oct=1, level=0.2),
    filt("LP 24", 1600, 0.15, env=16, key=0.6, vel=0.35, drive=0.2), adsr("amp", 0.003, 2.4, 0.3, 0.35), adsr("fenv", 0.001, 0.5, 0.1, 0.3),
    lfo(1, "Drift", 0.4, retrig=False),
    {"drift": 0.35, "noiseLevel": 0.04, "noiseType": "Crackle"},
    mods(("LFO 1", "Pitch", 0.004), ("Heat", "Filter Drive", 0.4), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.45),
         ("Orbit", "FX Chorus", 0.5), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Noise Level", 0.3), ("Aftertouch", "Cutoff", 0.2)),
    fx(drive=("Tube", 0.3, 0.4, 1.0), chorus=(0.4, 0.3, 0.15), reverb=(0.9, 1.4, 0.6, 10, 0.12)))

add("Station Keys", "Keys", "A bright digital poly key for comping: the Fraunhofer lines give it a glassy top.",
    ["digital", "comping", "bright"],
    osc("a", "Fraunhofer", morph=0.4, level=0.7, unison=2, detune=0.12, width=0.6), osc("b", "Classic", morph=0.5, oct=-1, level=0.3),
    filt("LP 24", 3000, 0.15, env=24, key=0.6, vel=0.3), adsr("amp", 0.002, 1.4, 0.35, 0.3), adsr("fenv", 0.001, 0.5, 0.2, 0.3),
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.35),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Resonance", 0.25), ("Flux", "FX Reverb", 0.3)),
    fx(chorus=(0.5, 0.3, 0.15), delay=("1/4.", 0.25, 0.5, 0.1, True), reverb=(1.2, 1.8, 0.45, 10, 0.15)))

add("Harpsichord Hyperion", "Keys", "A plucked baroque register: two bright strings an octave apart, quick and courtly.",
    ["harpsichord", "baroque", "plucked"],
    osc("a", "Classic", morph=0.5, level=0.7), osc("b", "Classic", morph=0.85, oct=1, level=0.4, fine=3),
    filt("HP 12", 250, 0.1, key=0.4), adsr("amp", 0.001, 1.6, 0.0, 0.3), adsr("menv", 0.001, 0.08, 0.0, 0.05),
    mods(("Mod Env", "Morph B", 0.3), ("Velocity", "Level B", 0.3), ("Heat", "Morph A", 0.35), ("Gravity", "Cutoff", -0.3),
         ("Orbit", "Detune", 0.25), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Level B", -0.4), ("Aftertouch", "Morph A", 0.25)),
    fx(reverb=(1.1, 1.6, 0.4, 12, 0.14)))

# =====================================================================================================
# PLUCK
# =====================================================================================================
add("Koto Of Kepler", "Pluck", "A plucked silk string with a bending attack: the pitch settles a breath after the pick.",
    ["koto", "asian", "string"],
    osc("a", "Fraunhofer", morph=0.2, level=0.75), osc("b", "Classic", morph=0.3, oct=1, level=0.25),
    filt("LP 24", 2200, 0.3, env=36, key=0.7, vel=0.3), adsr("amp", 0.001, 1.2, 0.0, 0.4), adsr("fenv", 0.001, 0.25, 0.0, 0.2),
    adsr("menv", 0.001, 0.07, 0.0, 0.05),
    mods(("Mod Env", "Pitch", 0.012), ("Velocity", "Cutoff", 0.25), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Morph A", 0.3), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Morph A", 0.25)),
    fx(delay=("1/8.", 0.3, 0.6, 0.12, True), reverb=(1.3, 2.0, 0.45, 15, 0.15)))

add("Harp Of Lyra", "Pluck", "A concert harp across the constellations: round, gentle, long ringing strings.",
    ["harp", "gentle", "string"],
    osc("a", "Classic", morph=0.25, level=0.75), osc("b", "Nebula", morph=0.1, oct=1, level=0.2),
    filt("LP 12", 2800, 0.1, env=24, key=0.7, vel=0.35), adsr("amp", 0.002, 2.4, 0.0, 1.0), adsr("fenv", 0.001, 0.6, 0.0, 0.5),
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.3), ("Orbit", "Morph B", 0.4),
         ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Cutoff", 0.2), ("Key", "Cutoff", 0.1)),
    fx(reverb=(1.6, 2.8, 0.4, 20, 0.2)))

add("Marimba Moon", "Pluck", "A low marimba: a sine with a hollow knock that disappears in a moment.",
    ["marimba", "mallet", "wood"],
    osc("a", "Classic", morph=0.0, level=0.8), osc("b", "Classic", morph=0.0, oct=2, semi=-2, level=0.3),
    filt("LP 12", 4000, 0.05, key=0.5), adsr("amp", 0.001, 0.9, 0.0, 0.2), adsr("menv", 0.001, 0.05, 0.0, 0.05),
    mods(("Mod Env", "Level B", 0.6), ("Velocity", "Level B", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.45),
         ("Orbit", "FX Delay", 0.35), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Morph A", 0.35), ("Aftertouch", "Cutoff", -0.25)),
    fx(delay=("1/8.", 0.2, 0.6, 0.0, True), reverb=(1.1, 1.4, 0.5, 8, 0.12)))

add("Kalimba Signal", "Pluck", "A thumb piano with a metal tongue: the comb filter rings at the note.",
    ["kalimba", "metallic", "african"],
    osc("a", "Classic", morph=0.0, level=0.7), osc("b", "Pulsar", morph=0.4, oct=1, level=0.25),
    filt("Comb", 440, 0.62, env=0, key=1.0), adsr("amp", 0.001, 1.1, 0.0, 0.35), adsr("menv", 0.001, 0.04, 0.0, 0.04),
    {"noiseLevel": 0.1, "noiseType": "Pink"},
    mods(("Mod Env", "Noise Level", 0.4), ("Heat", "Resonance", 0.25), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph B", 0.35),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.3), ("Aftertouch", "Level B", 0.3), ("Velocity", "Level B", 0.25)),
    fx(delay=("1/4.", 0.25, 0.6, 0.12, True), reverb=(1.0, 1.4, 0.5, 8, 0.12)))

add("Staccato Satellites", "Pluck", "Short, bowed string stabs in tight unison: a pizzicato section in orbit.",
    ["strings", "staccato", "pizzicato"],
    osc("a", "Classic", morph=0.5, level=0.7, unison=4, detune=0.18, width=0.7), osc("b", "Classic", morph=0.5, oct=-1, level=0.3),
    filt("LP 24", 1800, 0.15, env=20, key=0.6, vel=0.35), adsr("amp", 0.004, 0.35, 0.0, 0.2), adsr("fenv", 0.001, 0.2, 0.0, 0.2),
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Resonance", 0.3),
         ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Delay", 0.2)),
    fx(chorus=(0.3, 0.3, 0.15), reverb=(1.5, 2.2, 0.45, 15, 0.18)))

add("Nylon Telescope", "Pluck", "A soft nylon-string guitar: a hollow body, a bright pick, a short sustain.",
    ["guitar", "nylon", "acoustic"],
    osc("a", "Classic", morph=0.35, level=0.75), osc("b", "Fraunhofer", morph=0.0, oct=1, level=0.2),
    filt("LP 24", 1600, 0.2, env=30, key=0.8, vel=0.35), adsr("amp", 0.001, 1.5, 0.1, 0.3), adsr("fenv", 0.001, 0.18, 0.05, 0.2),
    {"noiseLevel": 0.06, "noiseType": "Pink"},
    adsr("menv", 0.001, 0.02, 0.0, 0.02),
    mods(("Mod Env", "Noise Level", 0.5), ("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.35),
         ("Orbit", "Detune", 0.2), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Morph A", 0.35), ("Aftertouch", "Resonance", 0.25)),
    fx(reverb=(1.0, 1.4, 0.5, 10, 0.12)))

add("Steel String Comet", "Pluck", "A bright steel-string strum with a long metallic shimmer behind it.",
    ["guitar", "steel", "bright"],
    osc("a", "Fraunhofer", morph=0.55, level=0.7, unison=2, detune=0.08, width=0.5), osc("b", "Classic", morph=0.5, oct=1, level=0.25, fine=6),
    filt("LP 12", 3600, 0.2, env=24, key=0.7, vel=0.3), adsr("amp", 0.001, 2.0, 0.0, 0.6), adsr("fenv", 0.001, 0.4, 0.0, 0.3),
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.35), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Resonance", 0.25), ("Flux", "FX Reverb", 0.3)),
    fx(chorus=(0.5, 0.35, 0.15), reverb=(1.3, 2.0, 0.45, 12, 0.15)))

add("FM Spark", "Pluck", "A short FM blip: the index falls in a few milliseconds, leaving a clean sine.",
    ["fm", "short", "digital"],
    osc("a", "Classic", morph=0.0, level=0.75, warp="FM from B", warpAmt=0.6), osc("b", "Classic", morph=0.0, oct=1, semi=7, level=0.1),
    filt("LP 12", 9000, 0.05, key=0.5), adsr("amp", 0.001, 0.4, 0.0, 0.15), adsr("menv", 0.001, 0.08, 0.0, 0.05),
    mods(("Mod Env", "Warp A", 0.6), ("Velocity", "Warp A", 0.3), ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.45),
         ("Orbit", "Pitch B", 0.25), ("Flux", "FX Delay", 0.45), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Cutoff", -0.25)),
    fx(delay=("1/16.", 0.35, 0.5, 0.15, True), reverb=(1.0, 1.2, 0.5, 5, 0.1)))

add("Cavity Ring-Down", "Pluck", "A noise burst ringing in a tuned comb: a struck tube, half string, half wind.",
    ["comb", "physical", "metallic"],
    osc("a", "Classic", morph=0.0, level=0.0, on=False), osc("b", "Classic", on=False),
    filt("Comb", 300, 0.78, env=0, key=1.0), adsr("amp", 0.001, 1.6, 0.0, 0.5), adsr("menv", 0.001, 0.03, 0.0, 0.03),
    {"noiseLevel": 0.85, "noiseType": "White"},
    mods(("Mod Env", "Noise Level", 0.15), ("Heat", "Resonance", 0.2), ("Gravity", "Sub Level", 0.4), ("Orbit", "Cutoff", 0.06),
         ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Resonance", 0.2), ("Aftertouch", "Resonance", 0.15), ("Velocity", "Amp", 0.3)),
    fx(reverb=(1.2, 2.0, 0.45, 10, 0.15)))

add("Future Pluck Chord", "Pluck", "A wide, sidechain-shaped supersaw chord pluck for future bass drops.",
    ["future bass", "chord", "supersaw"],
    osc("a", "Classic", morph=0.5, level=0.7, unison=7, detune=0.3, width=0.9), osc("b", "Ion Drive", morph=0.5, oct=1, level=0.25, unison=3, detune=0.2),
    filt("LP 24", 1200, 0.25, env=48, key=0.5, vel=0.25), adsr("amp", 0.002, 0.55, 0.15, 0.25), adsr("fenv", 0.001, 0.3, 0.0, 0.25),
    lfo(1, "Saw Down", sync=True, div="1/4", retrig=True),
    mods(("LFO 1", "Amp", 0.25), ("Heat", "Cutoff", 0.4), ("Gravity", "Sub Level", 0.45), ("Orbit", "Detune", 0.45),
         ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Morph B", 0.5), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.15, 0.6, 1.0), chorus=(0.4, 0.4, 0.2), reverb=(1.6, 2.4, 0.4, 15, 0.2)))

add("Dewdrop Pluck", "Pluck", "A tiny, liquid water-drop pluck: the filter sweeps up as the note falls away.",
    ["water", "drop", "cute"],
    osc("a", "Nebula", morph=0.6, level=0.7), osc("b", "Classic", morph=0.0, oct=1, level=0.3),
    filt("BP 12", 900, 0.6, env=40, key=0.8), adsr("amp", 0.001, 0.35, 0.0, 0.2), adsr("fenv", 0.001, 0.15, 0.0, 0.1),
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Resonance", 0.25), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.4),
         ("Flux", "FX Delay", 0.45), ("Mod Wheel", "Resonance", 0.3), ("Aftertouch", "Morph A", 0.3), ("Flux", "FX Reverb", 0.3)),
    fx(delay=("1/8.", 0.35, 0.5, 0.18, True), reverb=(1.4, 2.0, 0.4, 15, 0.18)))

add("Pulsar Mallet", "Pluck", "A wooden mallet with a pulsar's attack: periodic, percussive, quickly gone.",
    ["mallet", "percussive", "digital"],
    osc("a", "Pulsar", morph=0.35, level=0.7), osc("b", "Classic", morph=0.0, oct=-1, level=0.4),
    filt("LP 24", 2000, 0.25, env=30, key=0.7, vel=0.35), adsr("amp", 0.001, 0.6, 0.0, 0.2), adsr("fenv", 0.001, 0.12, 0.0, 0.1),
    mods(("Velocity", "Morph A", 0.3), ("Heat", "Morph A", 0.4), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.3),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.25)),
    fx(drive=("Fold", 0.12, 0.5, 0.5), delay=("1/16.", 0.3, 0.6, 0.1, True), reverb=(1.0, 1.4, 0.5, 6, 0.12)))

add("Quantum Banjo", "Pluck", "A twangy, nasal pluck from a quantised wave: half banjo, half bitcrusher.",
    ["banjo", "twang", "lo-fi"],
    osc("a", "Classic", morph=0.7, level=0.7, warp="Quantize", warpAmt=0.35), osc("b", "Classic", morph=0.9, oct=1, level=0.2),
    filt("BP 12", 1800, 0.35, env=24, key=0.8), adsr("amp", 0.001, 0.7, 0.0, 0.2), adsr("fenv", 0.001, 0.12, 0.0, 0.1),
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Warp A", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Warp A", 0.3), ("Aftertouch", "Resonance", 0.25), ("Flux", "FX Delay", 0.2)),
    fx(reverb=(0.9, 1.2, 0.5, 5, 0.1)))

add("Slingshot Pluck", "Pluck", "A deep pluck that swings past the low end and slingshots up through the filter.",
    ["deep", "bass pluck", "slingshot"],
    osc("a", "Magnetar", morph=0.25, level=0.75), osc("b", "Classic", morph=0.0, oct=-1, level=0.5),
    filt("LP 24", 500, 0.35, env=50, key=0.4, vel=0.3), adsr("amp", 0.001, 0.8, 0.0, 0.3), adsr("fenv", 0.001, 0.35, 0.0, 0.25),
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Morph A", 0.4), ("Heat", "Cutoff", 0.25), ("Gravity", "Level B", 0.5),
         ("Orbit", "Morph A", 0.3), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Filter Drive", 0.5), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Tube", 0.2, 0.5, 1.0), delay=("1/8.", 0.3, 0.6, 0.12, True), reverb=()))

# =====================================================================================================
# BELL
# =====================================================================================================
add("Tubular Transit", "Bell", "Orchestral tubular bells: a deep strike with an inharmonic hum that hangs in the hall.",
    ["tubular", "orchestral", "fm"],
    osc("a", "Classic", morph=0.0, level=0.7, warp="FM from B", warpAmt=0.45), osc("b", "Classic", morph=0.0, oct=1, semi=4, fine=-14, level=0.25),
    filt("LP 12", 6000, 0.05, key=0.5), adsr("amp", 0.001, 6.0, 0.0, 3.0), adsr("menv", 0.001, 2.5, 0.0, 2.0),
    mods(("Mod Env", "Warp A", 0.3), ("Velocity", "Warp A", 0.25), ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Pitch B", 0.15), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Cutoff", -0.2)),
    fx(reverb=(2.0, 4.0, 0.4, 25, 0.25)))

add("Glass Planetarium", "Bell", "Struck glass under a dome: pure, high and a little cold.",
    ["glass", "pure", "crystal"],
    osc("a", "Classic", morph=0.0, level=0.6), osc("b", "Classic", morph=0.0, oct=2, semi=7, level=0.35),
    filt("HP 12", 500, 0.1, key=0.5), adsr("amp", 0.001, 3.0, 0.0, 2.0), adsr("menv", 0.001, 0.8, 0.0, 0.6),
    lfo(1, "Sine", 4.5),
    mods(("Mod Env", "Level B", 0.4), ("Aftertouch", "Level B", 0.3), ("Heat", "Level B", 0.35), ("Gravity", "Cutoff", -0.3),
         ("Orbit", "FX Chorus", 0.5), ("Flux", "FX Reverb", 0.5), ("LFO 1", "Pitch", 0.004), ("Mod Wheel", "LFO 1 Amount", 1.0)),
    fx(chorus=(0.3, 0.3, 0.1), reverb=(2.2, 4.5, 0.35, 30, 0.28)))

add("Gamelan Of Ganymede", "Bell", "A bronze gong-chime: ring modulation tuned just off the octave beats against itself.",
    ["gamelan", "metallic", "inharmonic"],
    osc("a", "Classic", morph=0.0, level=0.65, warp="Ring by B", warpAmt=0.55), osc("b", "Classic", morph=0.0, oct=1, semi=3, fine=22, level=0.2),
    filt("BP 12", 2200, 0.25, key=0.7), adsr("amp", 0.001, 3.5, 0.0, 2.5), adsr("menv", 0.001, 1.2, 0.0, 1.0),
    mods(("Mod Env", "Warp A", 0.3), ("Heat", "Warp A", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Pitch B", 0.12),
         ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Warp A", 0.3), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.25)),
    fx(reverb=(1.8, 3.5, 0.4, 20, 0.22)))

add("Music Box Voyager", "Bell", "A wind-up music box on the Golden Record: tiny comb teeth, a soft mechanical tick.",
    ["music box", "lullaby", "delicate"],
    osc("a", "Quasar", morph=0.0, level=0.6), osc("b", "Classic", morph=0.0, oct=2, level=0.3),
    filt("HP 12", 600, 0.1, key=0.5), adsr("amp", 0.001, 1.8, 0.0, 1.0), adsr("menv", 0.001, 0.03, 0.0, 0.03),
    {"noiseLevel": 0.05, "noiseType": "Crackle"},
    mods(("Mod Env", "Noise Level", 0.4), ("Heat", "Morph A", 0.35), ("Gravity", "Cutoff", -0.3), ("Orbit", "Detune", 0.2),
         ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Level B", -0.4), ("Aftertouch", "Morph A", 0.25), ("Velocity", "Level B", 0.3)),
    fx(reverb=(1.5, 2.6, 0.4, 15, 0.22)))

add("Celesta Constellation", "Bell", "A celesta's soft metal plates over felt hammers: sugar-plum stardust.",
    ["celesta", "sweet", "orchestral"],
    osc("a", "Classic", morph=0.0, level=0.7), osc("b", "Pulsar", morph=0.0, oct=2, level=0.25),
    filt("LP 12", 7000, 0.05, key=0.6, vel=0.3), adsr("amp", 0.001, 1.6, 0.0, 0.8), adsr("menv", 0.001, 0.3, 0.0, 0.2),
    mods(("Mod Env", "Level B", 0.5), ("Velocity", "Level B", 0.3), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.3),
         ("Orbit", "FX Chorus", 0.5), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Morph B", 0.4), ("Aftertouch", "Cutoff", 0.2)),
    fx(chorus=(0.4, 0.25, 0.1), reverb=(1.6, 2.6, 0.4, 18, 0.22)))

add("Cathedral Of Ceres", "Bell", "A great church bell: a low strike, a minor-third hum and a tail that won't stop.",
    ["church bell", "huge", "inharmonic"],
    osc("a", "Classic", morph=0.0, level=0.7, oct=-1, warp="FM from B", warpAmt=0.5), osc("b", "Classic", morph=0.0, semi=3, fine=8, level=0.3),
    filt("LP 24", 3500, 0.1, key=0.4), adsr("amp", 0.001, 9.0, 0.0, 5.0), adsr("menv", 0.001, 3.0, 0.0, 3.0),
    {"subLevel": 0.2, "subShape": "Sine"},
    mods(("Mod Env", "Warp A", 0.35), ("Velocity", "Warp A", 0.2), ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.5),
         ("Orbit", "Pitch B", 0.1), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Cutoff", 0.3), ("Aftertouch", "Level B", 0.3)),
    fx(reverb=(2.6, 8.0, 0.45, 40, 0.3)))

add("Solar Wind Chimes", "Bell", "Aluminium chimes stirred by the solar wind: random strikes in a slow breeze.",
    ["chimes", "random", "ambient"],
    osc("a", "Classic", morph=0.0, level=0.6), osc("b", "Classic", morph=0.0, oct=2, semi=2, fine=-18, level=0.3),
    filt("HP 12", 800, 0.1, key=0.5), adsr("amp", 0.001, 2.5, 0.0, 2.0), adsr("menv", 0.001, 0.5, 0.0, 0.4),
    {"noiseLevel": 0.08, "noiseType": "Solar Wind"},
    arp("Random", "1/8", octaves=3, gate=0.3, prob=0.55, latch=True),
    mods(("Random", "Pan", 0.7), ("Mod Env", "Level B", 0.4), ("Heat", "Level B", 0.3), ("Gravity", "Cutoff", -0.3),
         ("Orbit", "Noise Level", 0.4), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Noise Level", 0.35), ("Aftertouch", "Level B", 0.3)),
    fx(reverb=(2.4, 5.0, 0.35, 25, 0.32)))

add("Magnetar Bell", "Bell", "A hard, bright FM strike from the Magnetar table: modern and metallic.",
    ["fm", "metallic", "modern"],
    osc("a", "Magnetar", morph=0.5, level=0.65), osc("b", "Classic", morph=0.0, oct=1, semi=5, level=0.2),
    filt("LP 12", 8000, 0.1, key=0.5), adsr("amp", 0.001, 2.4, 0.0, 1.4), adsr("menv", 0.001, 0.6, 0.0, 0.5),
    mods(("Mod Env", "Morph A", 0.4), ("Velocity", "Morph A", 0.3), ("Heat", "Morph A", 0.35), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Morph B", 0.3), ("Flux", "FX Delay", 0.45), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Cutoff", -0.25)),
    fx(delay=("1/4.", 0.3, 0.5, 0.15, True), reverb=(1.8, 3.0, 0.4, 20, 0.22)))

add("Ion Cloud Bells", "Bell", "Detuned bells swimming in chorus: soft attacks, glowing like an ion trail.",
    ["soft", "chorus", "pad bell"],
    osc("a", "Ion Drive", morph=0.2, level=0.6, unison=3, detune=0.15, width=0.8), osc("b", "Classic", morph=0.0, oct=1, semi=7, level=0.25),
    filt("LP 24", 4500, 0.1, key=0.5), adsr("amp", 0.02, 3.0, 0.15, 2.2), adsr("menv", 0.001, 0.8, 0.0, 0.6),
    lfo(1, "Sine", 0.3, retrig=False),
    mods(("LFO 1", "Morph A", 0.15), ("Mod Env", "Level B", 0.4), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.35),
         ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Level B", 0.3)),
    fx(chorus=(0.4, 0.5, 0.3), reverb=(2.2, 4.0, 0.4, 25, 0.28)))

add("Event Horizon Gong", "Bell", "A gong falling into a black hole: the fold deepens as it rings out.",
    ["gong", "dark", "evolving"],
    osc("a", "Event Horizon", morph=0.2, level=0.65, oct=-1), osc("b", "Classic", morph=0.0, semi=6, fine=15, level=0.3),
    filt("LP 24", 2400, 0.2, key=0.4), adsr("amp", 0.002, 7.0, 0.0, 4.0), adsr("menv", 0.5, 5.0, 0.6, 4.0),
    mods(("Mod Env", "Morph A", 0.5), ("Velocity", "Cutoff", 0.25), ("Heat", "Morph A", 0.3), ("Gravity", "Sub Level", 0.5),
         ("Orbit", "Pitch B", 0.1), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Resonance", 0.3), ("Aftertouch", "Cutoff", 0.3)),
    {"subLevel": 0.25},
    fx(drive=("Cosmic Ray", 0.12, 0.5, 0.4), reverb=(2.5, 7.0, 0.45, 30, 0.3)))

add("Formant Chime", "Bell", "A chime that almost speaks: the formant filter tunes a vowel into its ring.",
    ["formant", "vocal", "chime"],
    osc("a", "Nebula", morph=0.5, level=0.65), osc("b", "Classic", morph=0.0, oct=1, semi=7, level=0.25),
    filt("Formant", 1200, 0.45, env=10, key=0.6), adsr("amp", 0.001, 2.2, 0.0, 1.4), adsr("fenv", 0.001, 0.8, 0.0, 0.6),
    lfo(1, "Triangle", 0.25, retrig=False),
    mods(("LFO 1", "Cutoff", 0.08), ("Velocity", "Cutoff", 0.2), ("Heat", "Morph A", 0.35), ("Gravity", "Sub Level", 0.35),
         ("Orbit", "LFO 1 Amount", 0.8), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Cutoff", 0.35), ("Aftertouch", "Resonance", 0.25)),
    fx(reverb=(1.8, 3.0, 0.4, 20, 0.24)))

add("Satellite Carillon", "Bell", "A tower of small bells rung from orbit: bright, interlocking, made for melodies.",
    ["carillon", "bright", "melodic"],
    osc("a", "Classic", morph=0.0, level=0.65, warp="FM from B", warpAmt=0.3), osc("b", "Classic", morph=0.0, oct=2, semi=-2, fine=10, level=0.25),
    filt("LP 12", 9000, 0.05, key=0.5, vel=0.25), adsr("amp", 0.001, 2.8, 0.0, 1.6), adsr("menv", 0.001, 0.9, 0.0, 0.7),
    mods(("Mod Env", "Warp A", 0.3), ("Velocity", "Warp A", 0.25), ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "FX Delay", 0.4), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Cutoff", -0.2)),
    fx(drive=("Soft", 0.08, 0.6, 1.0), delay=("1/4", 0.25, 0.55, 0.1, True), reverb=(2.0, 3.6, 0.4, 25, 0.25)))
