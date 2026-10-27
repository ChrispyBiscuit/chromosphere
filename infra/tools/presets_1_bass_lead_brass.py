# More factory presets: Bass, Lead, Brass & Poly. Exec'd by make_presets.py with its helpers in scope.

# =====================================================================================================
# BASS
# =====================================================================================================
add("Centre Of Mass", "Bass", "A pure sine sub with a whisper of second harmonic: the still point everything orbits.",
    ["sub", "clean", "deep"],
    osc("a", "Classic", morph=0.0, level=0.85), osc("b", "Classic", morph=0.22, oct=1, level=0.12),
    filt("LP 12", 600, 0.0, key=0.4), adsr("amp", 0.003, 0.8, 0.95, 0.15),
    {"voiceMode": "Mono", "glide": 0.03, "subLevel": 0.3, "subShape": "Sine"},
    mods(("Heat", "Level B", 0.5), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.5), ("Orbit", "Detune", 0.2),
         ("Flux", "FX Drive", 0.4), ("Mod Wheel", "Morph B", 0.5), ("Aftertouch", "Level B", 0.3), ("Velocity", "Amp", 0.15)),
    fx(drive=("Soft", 0.08, 0.5, 1.0), reverb=()))

add("Hoover Comet", "Bass", "A pitch-dropping detuned stack with a chorus tail: the classic rave growl thrown across the sky.",
    ["hoover", "rave", "detuned"],
    osc("a", "Pulsar", morph=0.65, unison=6, detune=0.45, width=0.7), osc("b", "Classic", morph=0.5, oct=-1, level=0.45),
    filt("LP 12", 2600, 0.15, env=10, key=0.4), adsr("amp", 0.004, 0.6, 0.85, 0.25), adsr("menv", 0.001, 0.18, 0.0, 0.1),
    {"voiceMode": "Legato", "glide": 0.12},
    mods(("Mod Env", "Pitch", -0.12), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.45), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Morph A", 0.45), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.2)),
    fx(chorus=(0.5, 0.6, 0.35), reverb=(1.2, 1.4, 0.5, 5, 0.06)))

add("Wobble Orbit", "Bass", "A tempo-locked wobble on the ladder; Orbit doubles its speed.",
    ["wobble", "dubstep", "lfo"],
    osc("a", "Magnetar", morph=0.35, level=0.8, unison=2, detune=0.12), osc("b", "Classic", morph=0.5, oct=-1, level=0.5),
    filt("LP 24", 500, 0.45, env=0, key=0.3, drive=0.3), adsr("amp", 0.003, 0.4, 1.0, 0.12),
    lfo(1, "Sine", sync=True, div="1/8", retrig=True),
    {"voiceMode": "Mono", "glide": 0.04},
    mods(("LFO 1", "Cutoff", 0.4), ("Heat", "Filter Drive", 0.45), ("Heat", "Morph A", 0.3), ("Gravity", "Level B", 0.45),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Drive", 0.45), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Morph A", 0.35)),
    fx(drive=("Fold", 0.18, 0.5, 0.6), reverb=()))

add("Funk Thruster", "Bass", "A snappy, envelope-quacked square for slap-and-pop basslines.",
    ["funk", "envelope", "quack"],
    osc("a", "Classic", morph=0.75, level=0.8), osc("b", "Classic", morph=0.5, level=0.35, fine=8),
    filt("BP 12", 420, 0.45, env=38, key=0.5, vel=0.4), adsr("amp", 0.001, 0.32, 0.55, 0.08), adsr("fenv", 0.001, 0.14, 0.0, 0.08),
    {"voiceMode": "Mono", "subLevel": 0.4, "subShape": "Square"},
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Resonance", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Filter Drive", 0.5), ("Aftertouch", "Cutoff", 0.3), ("Heat", "Cutoff", 0.2)),
    fx(drive=("Tube", 0.15, 0.6, 1.0), chorus=(0.6, 0.3, 0.0), reverb=()))

add("Slap Of Saturn", "Bass", "A slap bass made of FM: a bright thumb attack fading into a round body.",
    ["slap", "fm", "pluck bass"],
    osc("a", "Classic", morph=0.0, level=0.85, warp="FM from B", warpAmt=0.35), osc("b", "Classic", morph=0.0, oct=1, level=0.0),
    filt("LP 12", 3200, 0.1, env=18, key=0.5), adsr("amp", 0.001, 0.9, 0.3, 0.12), adsr("fenv", 0.001, 0.18, 0.0, 0.1),
    adsr("menv", 0.001, 0.12, 0.0, 0.1),
    {"voiceMode": "Mono"},
    mods(("Mod Env", "Warp A", 0.45), ("Velocity", "Warp A", 0.3), ("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.45),
         ("Orbit", "Morph B", 0.3), ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Warp A", 0.25)),
    fx(reverb=(0.7, 0.8, 0.6, 2, 0.04)))

add("Station Organ Bass", "Bass", "House organ bass: drawbar-like sines, a short click and no filter to speak of.",
    ["house", "organ", "deep house"],
    osc("a", "Ion Drive", morph=0.2, level=0.75), osc("b", "Classic", morph=0.0, oct=-1, level=0.55),
    filt("LP 12", 5000, 0.05, key=0.5), adsr("amp", 0.001, 0.35, 0.6, 0.06),
    {"noiseLevel": 0.04, "noiseType": "Pink", "polyphony": 4},
    mods(("Heat", "Morph A", 0.4), ("Heat", "Cutoff", 0.2), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.2),
         ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Morph A", 0.5), ("Aftertouch", "Level B", 0.3), ("Velocity", "Amp", 0.2)),
    fx(chorus=(0.9, 0.25, 0.0), reverb=()))

add("Kessler Syndrome", "Bass", "A cascade of colliding debris: a folded, cosmic-ray distorted bass that breaks up hard.",
    ["distorted", "aggressive", "industrial"],
    osc("a", "Plasma", morph=0.4, level=0.75, warp="Mirror", warpAmt=0.35), osc("b", "Classic", morph=0.8, oct=-1, level=0.5),
    filt("LP 24", 1600, 0.3, env=16, key=0.3, drive=0.4), adsr("amp", 0.002, 0.5, 0.85, 0.1), adsr("fenv", 0.001, 0.3, 0.3, 0.15),
    {"voiceMode": "Mono", "glide": 0.02},
    mods(("Heat", "Warp A", 0.4), ("Heat", "FX Drive", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.35),
         ("Flux", "FX Feedback", 0.3), ("Mod Wheel", "Warp A", 0.4), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Filter Drive", 0.25)),
    fx(drive=("Cosmic Ray", 0.35, 0.55, 0.7), delay=("1/16", 0.2, 0.6, 0.0, False), reverb=()))

add("Neon Perigee", "Bass", "Synthwave octave bass: a bright pulse locked to the eighth-note delay.",
    ["synthwave", "80s", "octave"],
    osc("a", "Classic", morph=0.62, level=0.8), osc("b", "Classic", morph=0.62, oct=-1, level=0.55, fine=-5),
    filt("LP 24", 1100, 0.3, env=30, key=0.45), adsr("amp", 0.002, 0.25, 0.7, 0.1), adsr("fenv", 0.001, 0.2, 0.2, 0.12),
    {"polyphony": 2},
    mods(("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.3), ("Flux", "FX Delay", 0.35),
         ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.2), ("Flux", "FX Chorus", 0.3)),
    fx(chorus=(0.4, 0.4, 0.2), delay=("1/8", 0.25, 0.55, 0.0, True), reverb=()))

add("Deep Core Rumble", "Bass", "Techno rumble: a long, low thud smeared into dark reverb and filtered back down.",
    ["techno", "rumble", "dark"],
    osc("a", "Classic", morph=0.0, level=0.85), osc("b", "Dark Matter", morph=0.2, level=0.3),
    filt("LP 24", 260, 0.2, env=24, key=0.2), adsr("amp", 0.001, 0.9, 0.0, 0.5), adsr("fenv", 0.001, 0.12, 0.0, 0.2),
    adsr("menv", 0.001, 0.06, 0.0, 0.1),
    {"voiceMode": "Mono", "subLevel": 0.4},
    mods(("Mod Env", "Pitch", 0.2), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph B", 0.35),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Level B", 0.45), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Amp", 0.2)),
    fx(drive=("Tube", 0.25, 0.4, 1.0), reverb=(2.2, 3.5, 0.75, 0, 0.18)))

add("Escape Velocity", "Bass", "A sliding legato saw that overshoots every note, for rolling, rubbery lines.",
    ["glide", "saw", "rolling"],
    osc("a", "Classic", morph=0.5, level=0.8, unison=2, detune=0.08), osc("b", "Classic", morph=0.75, oct=-1, level=0.4),
    filt("LP 24", 900, 0.4, env=28, key=0.4), adsr("amp", 0.002, 0.5, 0.9, 0.12), adsr("fenv", 0.001, 0.3, 0.25, 0.15),
    {"voiceMode": "Legato", "glide": 0.16, "glideLegato": False},
    mods(("Heat", "Cutoff", 0.35), ("Heat", "Resonance", 0.2), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Delay", 0.3), ("Mod Wheel", "Filter Drive", 0.5), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Cutoff", 0.2)),
    fx(drive=("Soft", 0.2, 0.5, 1.0), delay=("1/16", 0.2, 0.6, 0.0, True), reverb=()))

add("Quasar Pluck Bass", "Bass", "Quasar's harmonic shimmer struck short: a glassy pluck bass for melodic house.",
    ["pluck bass", "melodic", "bright"],
    osc("a", "Quasar", morph=0.2, level=0.75), osc("b", "Classic", morph=0.0, oct=-1, level=0.55),
    filt("LP 24", 500, 0.2, env=40, key=0.5), adsr("amp", 0.001, 0.45, 0.25, 0.15), adsr("fenv", 0.001, 0.22, 0.0, 0.15),
    adsr("menv", 0.001, 0.4, 0.0, 0.2),
    mods(("Mod Env", "Morph A", 0.4), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.35),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Morph A", 0.45), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.25)),
    fx(reverb=(1.0, 1.2, 0.5, 5, 0.05)))

add("Acid Rain On Venus", "Bass", "A squelching saw acid line through the 12 dB filter, its arp lane full of slides.",
    ["acid", "arp", "squelch"],
    osc("a", "Classic", morph=0.5, level=0.85), osc("b", "Classic", on=False),
    filt("LP 12", 320, 0.82, env=50, key=0.3, drive=0.3, vel=0.3), adsr("amp", 0.002, 0.3, 0.75, 0.05), adsr("fenv", 0.001, 0.16, 0.0, 0.1),
    {"voiceMode": "Legato", "glide": 0.06, "glideLegato": True},
    arp("As Played", "1/16", octaves=1, gate=0.9, swing=0.12,
        lane=[(1.0, 0, 1), (0.6, 12, 1), (0.0, 0, 1), (0.8, 0, 1), (1.0, -12, 1), (0.5, 0, 1), (0.9, 7, 1), (0.0, 0, 1),
              (1.0, 0, 1), (0.6, 10, 1), (0.8, 12, 1), (0.0, 0, 1), (0.9, 0, 2), (0.5, 3, 1), (1.0, 0, 1), (0.6, -5, 1)]),
    mods(("Arp Step", "Cutoff", 0.2), ("Heat", "Resonance", 0.15), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.5),
         ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Filter Drive", 0.6), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Fold", 0.15, 0.6, 0.5), delay=("1/8.", 0.3, 0.6, 0.0, True), reverb=()))

add("Nebula Reese", "Bass", "A wide, slow-phasing Nebula reese with the notch drifting through it.",
    ["reese", "notch", "wide"],
    osc("a", "Nebula", morph=0.4, unison=5, detune=0.3, width=0.6), osc("b", "Classic", morph=0.0, oct=-1, level=0.45),
    filt("Notch", 900, 0.5, key=0.4), adsr("amp", 0.006, 0.6, 1.0, 0.25),
    lfo(1, "Triangle", 0.18, retrig=False), lfo(2, "Drift", 0.1, retrig=False),
    {"voiceMode": "Mono", "glide": 0.05},
    mods(("LFO 1", "Cutoff", 0.15), ("LFO 2", "Morph A", 0.2), ("Heat", "Morph A", 0.35), ("Gravity", "Level B", 0.45),
         ("Orbit", "Detune", 0.4), ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.2, 0.5, 1.0), chorus=(0.25, 0.4, 0.1), reverb=()))

add("Event Horizon Growl", "Bass", "Formant-filtered talking bass: the vowel opens and closes with the mod envelope.",
    ["formant", "talking", "growl"],
    osc("a", "Event Horizon", morph=0.3, level=0.75), osc("b", "Classic", morph=0.5, oct=-1, level=0.45),
    filt("Formant", 700, 0.6, env=0, key=0.0), adsr("amp", 0.003, 0.4, 0.9, 0.12), adsr("menv", 0.01, 0.35, 0.3, 0.2),
    lfo(1, "Square", sync=True, div="1/16", retrig=True),
    {"voiceMode": "Mono", "glide": 0.03},
    mods(("Mod Env", "Cutoff", 0.25), ("Heat", "Morph A", 0.45), ("Gravity", "Level B", 0.45), ("Orbit", "LFO 1 Amount", 0.6),
         ("LFO 1", "Cutoff", 0.15), ("Flux", "FX Drive", 0.4), ("Mod Wheel", "Cutoff", 0.35), ("Aftertouch", "Morph A", 0.3)),
    fx(drive=("Tube", 0.3, 0.5, 1.0), reverb=()))

add("Dark Matter Pulse", "Bass", "An invisible weight felt more than heard: a sidechain-pumping sub that swells on the beat.",
    ["pump", "sub", "sidechain"],
    osc("a", "Dark Matter", morph=0.1, level=0.7), osc("b", "Classic", morph=0.0, oct=-1, level=0.55),
    filt("LP 24", 700, 0.15, key=0.3), adsr("amp", 0.01, 0.6, 1.0, 0.25),
    lfo(1, "Saw Up", sync=True, div="1/4", retrig=False),
    {"subLevel": 0.3, "polyphony": 4},
    mods(("LFO 1", "Amp", 0.5), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.45), ("Orbit", "LFO 1 Amount", 0.5),
         ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Morph A", 0.5), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.15)),
    fx(reverb=()))

add("Ion Drive Stepper", "Bass", "A quantized, stair-stepped Ion Drive tone: digital and gritty, made for minimal techno.",
    ["digital", "quantize", "minimal"],
    osc("a", "Ion Drive", morph=0.55, level=0.75, warp="Quantize", warpAmt=0.45), osc("b", "Classic", morph=0.25, oct=-1, level=0.5),
    filt("HP 12", 60, 0.1, key=0.0), adsr("amp", 0.001, 0.22, 0.5, 0.08), adsr("menv", 0.001, 0.25, 0.0, 0.1),
    {"voiceMode": "Mono"},
    mods(("Mod Env", "Warp A", 0.35), ("Heat", "Warp A", 0.4), ("Gravity", "Level B", 0.45), ("Orbit", "Morph A", 0.4),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Warp A", 0.35), ("Aftertouch", "Morph A", 0.3), ("Velocity", "Level A", 0.2)),
    fx(drive=("Cosmic Ray", 0.2, 0.6, 0.4), delay=("1/16.", 0.3, 0.6, 0.0, True), reverb=()))

# =====================================================================================================
# LEAD
# =====================================================================================================
add("Lyman Break", "Lead", "A clean square lead with a pinch of glide, voiced like the first line of the hydrogen series.",
    ["square", "classic", "clean"],
    osc("a", "Classic", morph=0.75, level=0.75), osc("b", "Classic", morph=0.75, level=0.3, fine=6),
    filt("LP 24", 3800, 0.15, env=8, key=0.6), adsr("amp", 0.006, 0.3, 0.9, 0.25), adsr("fenv", 0.005, 0.4, 0.5, 0.3),
    lfo(1, "Sine", 5.4, fade=0.3),
    {"voiceMode": "Legato", "glide": 0.05, "glideLegato": True},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Detune", 0.35), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.25)),
    fx(delay=("1/8.", 0.3, 0.5, 0.15, True), reverb=(1.3, 2.0, 0.45, 15, 0.12)))

add("Sync Horizon", "Lead", "A bending Plasma scream that rises in brightness as the pitch wheel comes up.",
    ["sync", "screaming", "bend"],
    osc("a", "Plasma", morph=0.45, level=0.75, unison=2, detune=0.1), osc("b", "Classic", morph=0.5, oct=-1, level=0.3),
    filt("LP 24", 7000, 0.2, env=10, key=0.5, drive=0.3), adsr("amp", 0.002, 0.4, 0.9, 0.3), adsr("fenv", 0.002, 0.5, 0.5, 0.3),
    lfo(1, "Sine", 5.8, fade=0.5),
    {"voiceMode": "Legato", "glide": 0.04, "bendRange": 12},
    mods(("Pitch Bend", "Morph A", 0.4), ("LFO 1", "Pitch", 0.014), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Morph A", 0.4),
         ("Gravity", "Level B", 0.35), ("Orbit", "Detune", 0.35), ("Flux", "FX Delay", 0.4), ("Aftertouch", "Filter Drive", 0.4)),
    fx(drive=("Tube", 0.3, 0.6, 1.0), delay=("1/4", 0.35, 0.55, 0.18, True), reverb=(1.4, 2.4, 0.4, 20, 0.12)))

add("Chiptune Probe", "Lead", "An 8-bit pulse quantized to coarse steps, with an arpeggio of fifths and octaves on every note.",
    ["chiptune", "8-bit", "quantize"],
    osc("a", "Classic", morph=0.85, level=0.7, warp="Quantize", warpAmt=0.6), osc("b", "Classic", on=False),
    filt("LP 12", 12000, 0.0, key=0.0), adsr("amp", 0.001, 0.2, 0.8, 0.05),
    arp("Up", "1/32", octaves=1, gate=0.95,
        lane=[(1.0, 0, 1), (1.0, 7, 1), (1.0, 12, 1), (1.0, 7, 1)], length=4),
    mods(("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph A", -0.3), ("Flux", "FX Delay", 0.35),
         ("Mod Wheel", "Morph A", -0.4), ("Aftertouch", "Warp A", 0.3), ("Velocity", "Amp", 0.2), ("Heat", "Cutoff", 0.2)),
    fx(delay=("1/8", 0.25, 0.2, 0.12, True), reverb=()))

add("Voyager Voice", "Lead", "A vocal lead: the formant filter sweeps from 'oo' to 'ah' on the mod wheel.",
    ["vocal", "formant", "expressive"],
    osc("a", "Nebula", morph=0.5, level=0.75, unison=2, detune=0.12, width=0.4), osc("b", "Classic", morph=0.5, level=0.35),
    filt("Formant", 450, 0.55, env=0, key=0.2), adsr("amp", 0.05, 0.4, 0.9, 0.4),
    lfo(1, "Sine", 5.0, fade=0.6),
    {"voiceMode": "Legato", "glide": 0.08, "glideLegato": True},
    mods(("Mod Wheel", "Cutoff", 0.4), ("LFO 1", "Pitch", 0.01), ("Aftertouch", "LFO 1 Amount", 1.0), ("Heat", "Morph A", 0.35),
         ("Gravity", "Sub Level", 0.35), ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.4), ("Velocity", "Cutoff", 0.15)),
    fx(chorus=(0.3, 0.4, 0.2), reverb=(1.8, 3.0, 0.4, 25, 0.2)))

add("Fifth Dimension", "Lead", "Parallel fifths through a warm ladder: the old monosynth trick, tuned in deep space.",
    ["fifths", "mono", "warm"],
    osc("a", "Classic", morph=0.5, level=0.7), osc("b", "Classic", morph=0.5, semi=7, level=0.55, fine=-3),
    filt("LP 24", 2600, 0.25, env=16, key=0.6), adsr("amp", 0.004, 0.4, 0.85, 0.25), adsr("fenv", 0.002, 0.45, 0.35, 0.3),
    lfo(1, "Triangle", 5.2, fade=0.4),
    {"voiceMode": "Legato", "glide": 0.07, "glideLegato": True},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.45),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Resonance", 0.3), ("Heat", "Filter Drive", 0.3)),
    fx(drive=("Soft", 0.15, 0.5, 1.0), delay=("1/4.", 0.3, 0.5, 0.14, True), reverb=(1.2, 1.8, 0.5, 10, 0.1)))

add("Portamento Comet", "Lead", "A long, sweeping glide between every note, trailing a bright tail of echoes.",
    ["portamento", "glide", "smooth"],
    osc("a", "Pulsar", morph=0.4, level=0.75), osc("b", "Classic", morph=0.25, oct=1, level=0.2),
    filt("LP 24", 3200, 0.2, env=10, key=0.6), adsr("amp", 0.01, 0.5, 0.9, 0.4), adsr("fenv", 0.01, 0.6, 0.5, 0.4),
    lfo(1, "Sine", 4.8, fade=0.8),
    {"voiceMode": "Legato", "glide": 0.35},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Morph A", 0.35), ("Gravity", "Level B", 0.3),
         ("Orbit", "Morph A", 0.3), ("Flux", "FX Delay", 0.45), ("Aftertouch", "Cutoff", 0.3), ("Heat", "Cutoff", 0.25)),
    fx(delay=("1/4.", 0.5, 0.5, 0.22, True), reverb=(1.8, 3.5, 0.4, 20, 0.18)))

add("Supersaw Armada", "Lead", "Eight-voice unison saws on both oscillators: a fleet of leads in perfect formation.",
    ["supersaw", "trance", "unison"],
    osc("a", "Classic", morph=0.5, unison=8, detune=0.4, width=0.85, level=0.65), osc("b", "Classic", morph=0.5, oct=1, unison=4, detune=0.3, width=0.7, level=0.3),
    filt("LP 12", 6500, 0.1, env=8, key=0.5), adsr("amp", 0.005, 0.5, 0.85, 0.35), adsr("fenv", 0.005, 0.6, 0.5, 0.35),
    {"voiceMode": "Poly", "polyphony": 8},
    mods(("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.35), ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.45),
         ("Mod Wheel", "Level B", 0.5), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.15), ("Flux", "FX Delay", 0.3)),
    fx(delay=("1/8.", 0.35, 0.5, 0.15, True), reverb=(2.0, 3.0, 0.45, 20, 0.2)))

add("Starlight Flute", "Lead", "A breathy, soft lead: a near-sine with pink breath noise and a delayed vibrato.",
    ["flute", "soft", "breathy"],
    osc("a", "Classic", morph=0.12, level=0.75), osc("b", "Classic", morph=0.0, oct=1, level=0.15),
    filt("LP 12", 2400, 0.1, env=10, key=0.7), adsr("amp", 0.06, 0.3, 0.85, 0.3), adsr("fenv", 0.04, 0.3, 0.6, 0.3),
    lfo(1, "Sine", 5.0, fade=1.0),
    {"noiseLevel": 0.12, "noiseType": "Pink", "voiceMode": "Legato", "glide": 0.03},
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Noise Level", 0.4), ("Gravity", "Level B", -0.3),
         ("Orbit", "LFO 1 Rate", 0.2), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Noise Level", 0.2)),
    fx(reverb=(1.6, 2.6, 0.5, 20, 0.2)))

add("Hawking Radiation", "Lead", "Ring-modulated and cold, a lead that seems to leak out of the event horizon.",
    ["ring mod", "metallic", "cold"],
    osc("a", "Event Horizon", morph=0.25, level=0.7, warp="Ring by B", warpAmt=0.5), osc("b", "Classic", morph=0.0, semi=7, level=0.0),
    filt("LP 24", 5000, 0.2, env=8, key=0.5), adsr("amp", 0.004, 0.4, 0.85, 0.3), adsr("fenv", 0.002, 0.4, 0.4, 0.3),
    lfo(1, "Sine", 5.6, fade=0.4),
    {"voiceMode": "Legato", "glide": 0.05},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Warp A", 0.35), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Pitch B", 0.15), ("Flux", "FX Delay", 0.4), ("Aftertouch", "Warp A", 0.3), ("Heat", "Cutoff", 0.2)),
    fx(delay=("1/8.", 0.35, 0.45, 0.14, True), reverb=(1.5, 2.4, 0.4, 15, 0.15)))

add("Magnetar Whistle", "Lead", "A piercing resonant band-pass whistle, sharp enough to cut through any mix.",
    ["whistle", "resonant", "cutting"],
    osc("a", "Magnetar", morph=0.2, level=0.7), osc("b", "Classic", morph=0.5, level=0.2),
    filt("BP 12", 1800, 0.75, env=12, key=1.0), adsr("amp", 0.01, 0.3, 0.9, 0.25), adsr("fenv", 0.005, 0.3, 0.6, 0.3),
    lfo(1, "Sine", 6.0, fade=0.3),
    {"voiceMode": "Legato", "glide": 0.06, "glideLegato": True},
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Resonance", 0.2), ("Gravity", "Sub Level", 0.35),
         ("Orbit", "Morph A", 0.35), ("Flux", "FX Reverb", 0.35), ("Aftertouch", "Cutoff", 0.3), ("Heat", "Morph A", 0.3)),
    fx(delay=("1/4", 0.3, 0.5, 0.12, True), reverb=(1.2, 2.0, 0.4, 10, 0.14)))

add("Gravity Assist", "Lead", "A fat, detuned mono lead that slingshots up with the mod envelope on each new note.",
    ["mono", "fat", "pitch envelope"],
    osc("a", "Classic", morph=0.5, level=0.7, unison=3, detune=0.18, width=0.3), osc("b", "Ion Drive", morph=0.5, oct=-1, level=0.4),
    filt("LP 24", 3000, 0.25, env=14, key=0.5, drive=0.2), adsr("amp", 0.003, 0.4, 0.9, 0.25), adsr("fenv", 0.002, 0.4, 0.4, 0.3),
    adsr("menv", 0.001, 0.12, 0.0, 0.1), lfo(1, "Sine", 5.4, fade=0.5),
    {"voiceMode": "Mono", "glide": 0.03},
    mods(("Mod Env", "Pitch", -0.06), ("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.35), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Filter Drive", 0.4)),
    fx(drive=("Tube", 0.2, 0.55, 1.0), delay=("1/8.", 0.3, 0.5, 0.12, True), reverb=(1.2, 1.8, 0.5, 10, 0.1)))

add("Fraunhofer Lines", "Lead", "A spectral lead: dark absorption lines sweep across the Fraunhofer table as you hold a note.",
    ["spectral", "evolving", "bright"],
    osc("a", "Fraunhofer", morph=0.2, level=0.75, unison=2, detune=0.1, width=0.5), osc("b", "Classic", morph=0.5, oct=-1, level=0.3),
    filt("LP 24", 5500, 0.15, env=6, key=0.5), adsr("amp", 0.01, 0.5, 0.9, 0.35), adsr("menv", 0.3, 2.5, 0.6, 1.0),
    lfo(2, "Triangle", 0.2, retrig=True),
    {"voiceMode": "Legato", "glide": 0.05},
    mods(("Mod Env", "Morph A", 0.45), ("LFO 2", "Morph A", 0.12), ("Mod Wheel", "Morph A", 0.35), ("Aftertouch", "Cutoff", 0.3),
         ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.35), ("Orbit", "LFO 2 Rate", 0.4), ("Flux", "FX Reverb", 0.35)),
    fx(delay=("1/4.", 0.3, 0.5, 0.12, True), reverb=(1.6, 2.6, 0.45, 20, 0.15)))

add("Kuiper Square Lead", "Lead", "A hollow, folded square that grows teeth when Heat comes up: lo-fi and gritty.",
    ["lo-fi", "fold", "gritty"],
    osc("a", "Classic", morph=0.7, level=0.7, warp="Bend", warpAmt=0.3), osc("b", "Classic", morph=0.7, oct=-1, level=0.3, fine=10),
    filt("LP 12", 4000, 0.2, env=10, key=0.5), adsr("amp", 0.003, 0.4, 0.85, 0.2), adsr("fenv", 0.002, 0.3, 0.4, 0.2),
    lfo(1, "Triangle", 5.5, fade=0.4),
    {"voiceMode": "Legato", "glide": 0.04},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "FX Drive", 0.5), ("Gravity", "Level B", 0.4),
         ("Orbit", "Warp A", 0.3), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Warp A", 0.2)),
    fx(drive=("Fold", 0.25, 0.4, 0.5), delay=("1/16.", 0.3, 0.6, 0.1, True), reverb=()))

add("Dyson Swarm Lead", "Lead", "Mirror-warped Ion Drive saws in a tight swarm: a hard-edged lead for big drops.",
    ["edm", "mirror", "hard"],
    osc("a", "Ion Drive", morph=0.6, level=0.7, unison=4, detune=0.22, width=0.6, warp="Mirror", warpAmt=0.25),
    osc("b", "Classic", morph=0.5, oct=-1, level=0.35),
    filt("HP 12", 120, 0.15, key=0.0), adsr("amp", 0.002, 0.4, 0.9, 0.25),
    lfo(1, "Sine", 5.5, fade=0.4),
    {"voiceMode": "Legato", "glide": 0.04},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Warp A", 0.4), ("Gravity", "Level B", 0.4),
         ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Morph A", 0.3), ("Velocity", "Warp A", 0.2)),
    fx(drive=("Soft", 0.2, 0.6, 1.0), delay=("1/8", 0.3, 0.5, 0.1, True), reverb=(1.6, 2.2, 0.45, 10, 0.14)))

# =====================================================================================================
# BRASS & POLY
# =====================================================================================================
add("Orion Stab", "Brass & Poly", "A short, punchy house chord stab: detuned saws with a fast filter snap.",
    ["stab", "house", "chord"],
    osc("a", "Classic", morph=0.5, unison=3, detune=0.2, width=0.6, level=0.7), osc("b", "Classic", morph=0.75, level=0.35, fine=-7),
    filt("LP 24", 900, 0.25, env=40, key=0.5), adsr("amp", 0.001, 0.3, 0.0, 0.18), adsr("fenv", 0.001, 0.2, 0.0, 0.15),
    {"polyphony": 10},
    mods(("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.35), ("Orbit", "Detune", 0.35), ("Flux", "FX Delay", 0.4),
         ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.3)),
    fx(delay=("1/8.", 0.3, 0.5, 0.14, True), reverb=(1.4, 1.8, 0.5, 10, 0.15)))

add("Cosmic Horn Section", "Brass & Poly", "Brass with breath: a slow filter swell and a slight pitch scoop into each note.",
    ["brass", "horns", "swell"],
    osc("a", "Classic", morph=0.5, unison=2, detune=0.12, width=0.4, level=0.7), osc("b", "Classic", morph=0.6, level=0.4, fine=5),
    filt("LP 24", 1100, 0.15, env=28, key=0.6, vel=0.3), adsr("amp", 0.04, 0.6, 0.85, 0.3), adsr("fenv", 0.08, 0.7, 0.45, 0.35),
    adsr("menv", 0.001, 0.08, 0.0, 0.1), lfo(1, "Sine", 5.0, fade=0.6),
    {"polyphony": 8},
    mods(("Mod Env", "Pitch", -0.03), ("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Sub Level", 0.35), ("Orbit", "Detune", 0.3), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(reverb=(1.6, 2.2, 0.5, 15, 0.18)))

add("Andromeda Poly", "Brass & Poly", "A warm, chorused polysynth in the 80s style: one oscillator, one chorus, endless chords.",
    ["polysynth", "juno", "chorus"],
    osc("a", "Classic", morph=0.62, level=0.75), osc("b", "Classic", on=False),
    filt("LP 24", 2200, 0.2, env=14, key=0.6), adsr("amp", 0.006, 0.6, 0.75, 0.4), adsr("fenv", 0.004, 0.6, 0.35, 0.4),
    lfo(1, "Triangle", 0.6, retrig=False),
    {"subLevel": 0.25, "subShape": "Square", "polyphony": 12},
    mods(("LFO 1", "Morph A", 0.08), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.35), ("Orbit", "LFO 1 Amount", 0.5),
         ("Flux", "FX Chorus", 0.45), ("Mod Wheel", "Cutoff", 0.35), ("Aftertouch", "Resonance", 0.3), ("Velocity", "Cutoff", 0.2)),
    fx(chorus=(0.5, 0.65, 0.5), reverb=(1.3, 1.8, 0.5, 10, 0.12)))

add("Hoover Mothership", "Brass & Poly", "The rave hoover chord: pulse-width chorus stacks dropping in pitch on every hit.",
    ["hoover", "rave", "chord"],
    osc("a", "Pulsar", morph=0.7, unison=5, detune=0.4, width=0.8, level=0.65), osc("b", "Classic", morph=0.8, oct=-1, unison=3, detune=0.3, level=0.35),
    filt("LP 12", 3500, 0.1, env=6, key=0.5), adsr("amp", 0.003, 0.6, 0.8, 0.35), adsr("menv", 0.001, 0.25, 0.0, 0.2),
    {"glide": 0.08, "polyphony": 8},
    mods(("Mod Env", "Pitch", -0.08), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Morph A", 0.45), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.3)),
    fx(chorus=(0.7, 0.7, 0.45), reverb=(1.8, 2.4, 0.45, 10, 0.15)))

add("Synchrotron Sweep", "Brass & Poly", "A wide pad-brass whose cutoff rides a slow sweep across each chord.",
    ["sweep", "wide", "brass"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.25, width=0.8, level=0.65), osc("b", "Ion Drive", morph=0.4, level=0.35),
    filt("LP 24", 600, 0.35, env=32, key=0.5), adsr("amp", 0.15, 1.0, 0.9, 0.8), adsr("fenv", 1.2, 2.0, 0.6, 1.0),
    lfo(1, "Triangle", sync=True, div="2 bars", retrig=True),
    {"polyphony": 8},
    mods(("LFO 1", "Cutoff", 0.15), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.35), ("Orbit", "LFO 1 Amount", 0.6),
         ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Orbit", "Detune", 0.3)),
    fx(chorus=(0.3, 0.5, 0.2), reverb=(2.2, 3.5, 0.45, 25, 0.22)))

add("Brass Of Titan", "Brass & Poly", "Low, heavy brass in the cold of Saturn's moon: a growling unison with tube grit.",
    ["brass", "heavy", "low"],
    osc("a", "Classic", morph=0.5, unison=3, detune=0.15, width=0.4, level=0.7), osc("b", "Magnetar", morph=0.2, oct=-1, level=0.4),
    filt("LP 24", 800, 0.2, env=30, key=0.6, drive=0.25, vel=0.3), adsr("amp", 0.03, 0.8, 0.85, 0.4), adsr("fenv", 0.05, 0.8, 0.4, 0.4),
    {"polyphony": 6},
    mods(("Heat", "Filter Drive", 0.4), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Morph B", 0.45), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Filter Drive", 0.2)),
    fx(drive=("Tube", 0.25, 0.5, 1.0), reverb=(1.8, 2.5, 0.5, 15, 0.16)))

add("Plasma Chord Memory", "Brass & Poly", "A bright, digital chord stab with a ringing comb tail, for garage and UK funky.",
    ["stab", "comb", "garage"],
    osc("a", "Plasma", morph=0.3, level=0.65), osc("b", "Classic", morph=0.5, level=0.35),
    filt("Comb", 600, 0.45, env=12, key=1.0), adsr("amp", 0.001, 0.35, 0.0, 0.25), adsr("fenv", 0.001, 0.2, 0.0, 0.2),
    {"polyphony": 10},
    mods(("Heat", "Morph A", 0.4), ("Heat", "Resonance", 0.2), ("Gravity", "Level B", 0.4), ("Orbit", "Cutoff", 0.1),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Morph A", 0.3), ("Velocity", "Morph A", 0.2)),
    fx(delay=("1/8.", 0.35, 0.55, 0.16, True), reverb=(1.2, 1.4, 0.5, 5, 0.1)))

add("Quasar Brass", "Brass & Poly", "Quasar's bright overtones voiced as a modern, glossy brass for big chorus hooks.",
    ["brass", "modern", "bright"],
    osc("a", "Quasar", morph=0.35, unison=3, detune=0.18, width=0.6, level=0.65), osc("b", "Classic", morph=0.5, level=0.4, fine=-6),
    filt("LP 24", 1400, 0.15, env=24, key=0.6, vel=0.3), adsr("amp", 0.02, 0.6, 0.85, 0.35), adsr("fenv", 0.04, 0.6, 0.45, 0.35),
    {"polyphony": 8},
    mods(("Heat", "Cutoff", 0.35), ("Heat", "Morph A", 0.25), ("Gravity", "Sub Level", 0.35), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Morph A", 0.45), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Cutoff", 0.2)),
    fx(chorus=(0.4, 0.4, 0.15), reverb=(1.6, 2.0, 0.45, 15, 0.16)))

add("Cassiopeia Strings", "Brass & Poly", "Ensemble strings from an old string machine: soft saws drowned in a slow triple chorus.",
    ["strings", "ensemble", "vintage"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.2, width=0.7, level=0.6), osc("b", "Classic", morph=0.5, oct=1, level=0.25),
    filt("LP 12", 3200, 0.05, key=0.5), adsr("amp", 0.35, 1.0, 0.9, 1.0),
    lfo(1, "Sine", 0.4, retrig=False),
    {"polyphony": 12},
    mods(("LFO 1", "Detune", 0.1), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.3), ("Orbit", "LFO 1 Amount", 0.6),
         ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Level B", 0.45), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.35)),
    fx(chorus=(0.6, 0.8, 0.55), reverb=(2.0, 3.0, 0.5, 20, 0.22)))

add("Sunrise Fanfare", "Brass & Poly", "A triumphant, open fifths fanfare with a bright attack bloom: launch-day brass.",
    ["fanfare", "fifths", "cinematic"],
    osc("a", "Classic", morph=0.5, unison=2, detune=0.12, level=0.65), osc("b", "Classic", morph=0.5, semi=7, level=0.4),
    filt("LP 24", 1600, 0.15, env=30, key=0.6, vel=0.4), adsr("amp", 0.02, 0.8, 0.8, 0.5), adsr("fenv", 0.02, 0.5, 0.4, 0.5),
    lfo(1, "Sine", 5.0, fade=0.8),
    {"polyphony": 8},
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.35),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Reverb", 0.45), ("Aftertouch", "Filter Drive", 0.35), ("Velocity", "Cutoff", 0.2)),
    fx(reverb=(2.4, 3.5, 0.45, 25, 0.25)))

add("Heliosphere Poly", "Brass & Poly", "A soft-attack poly with a high-pass edge: airy chords that sit above everything else.",
    ["airy", "high-pass", "poly"],
    osc("a", "Nebula", morph=0.45, unison=3, detune=0.2, width=0.8, level=0.65), osc("b", "Classic", morph=0.3, oct=1, level=0.3),
    filt("HP 24", 300, 0.2, env=0, key=0.4), adsr("amp", 0.08, 0.8, 0.8, 0.8),
    lfo(1, "Triangle", 0.3, retrig=False),
    {"polyphony": 12},
    mods(("LFO 1", "Morph A", 0.15), ("Heat", "Morph A", 0.35), ("Gravity", "Cutoff", -0.3), ("Orbit", "LFO 1 Amount", 0.6),
         ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Level B", 0.45), ("Aftertouch", "Morph A", 0.3), ("Flux", "FX Chorus", 0.35)),
    fx(chorus=(0.35, 0.5, 0.25), reverb=(2.2, 3.5, 0.4, 25, 0.25)))

add("Pulsar Brass Stab", "Brass & Poly", "A tight, ratcheting brass stab locked to the arpeggiator's chord mode.",
    ["stab", "rhythmic", "chord"],
    osc("a", "Pulsar", morph=0.5, unison=2, detune=0.15, width=0.5, level=0.65), osc("b", "Classic", morph=0.5, level=0.4, fine=6),
    filt("LP 24", 1000, 0.25, env=34, key=0.6), adsr("amp", 0.002, 0.25, 0.0, 0.15), adsr("fenv", 0.001, 0.18, 0.0, 0.12),
    arp("Chord", "1/8", gate=0.4, swing=0.1,
        lane=[(1.0, 0, 1), (0.0, 0, 1), (0.6, 0, 1), (0.8, 0, 1), (0.0, 0, 1), (1.0, 0, 2), (0.0, 0, 1), (0.6, 0, 1)], length=8),
    mods(("Arp Step", "Cutoff", 0.15), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.35), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.2)),
    fx(delay=("1/8.", 0.3, 0.5, 0.12, True), reverb=(1.4, 1.8, 0.5, 10, 0.14)))
