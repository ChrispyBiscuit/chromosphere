# More factory presets: pads, drones and textures (exec'd by make_presets.py with its helpers).

# =====================================================================================================
# PAD
# =====================================================================================================
add("Accretion Disk", "Pad", "Warm detuned saws swirling slowly inward, the filter breathing with a long LFO.",
    ["warm", "analog", "detuned"],
    osc("a", "Classic", morph=0.5, unison=6, detune=0.28, width=0.8), osc("b", "Classic", morph=0.7, oct=-1, level=0.45, unison=2, detune=0.15),
    filt("LP 24", 2200, 0.18, env=10, key=0.4), adsr("amp", 0.9, 1.5, 0.85, 2.2), adsr("fenv", 1.2, 2.5, 0.4, 2.0),
    lfo(1, "Triangle", 0.09, retrig=False), lfo(2, "Sine", 4.8),
    mods(("LFO 1", "Cutoff", 0.12), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Aftertouch", "Cutoff", 0.2), ("Velocity", "Amp", 0.2)),
    fx(chorus=(0.25, 0.5, 0.3), reverb=(2.0, 4.5, 0.45, 25, 0.3)))

add("Glass Firmament", "Pad", "Crystalline Quasar partials behind frosted glass, glinting when you press harder.",
    ["glassy", "digital", "bright"],
    osc("a", "Quasar", morph=0.35, unison=3, detune=0.12, width=0.7), osc("b", "Fraunhofer", morph=0.6, oct=1, level=0.35),
    filt("HP 12", 220, 0.1, key=0.2), adsr("amp", 0.6, 2.0, 0.8, 2.8), adsr("menv", 2.5, 3.0, 0.5, 2.0),
    lfo(1, "Sine", 0.15, retrig=False),
    mods(("Mod Env", "Morph A", 0.3), ("LFO 1", "Morph B", 0.2), ("Heat", "Morph A", 0.4), ("Gravity", "Cutoff", -0.3),
         ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Morph B", 0.5), ("Aftertouch", "Level B", 0.4)),
    fx(chorus=(0.4, 0.3, 0.2), delay=("1/4.", 0.4, 0.6, 0.15, True), reverb=(2.4, 5.0, 0.3, 30, 0.35)))

add("Choir Of Titan", "Pad", "A distant formant choir singing through methane haze, vowels shifting with Orbit.",
    ["choir", "formant", "vocal"],
    osc("a", "Nebula", morph=0.4, unison=4, detune=0.18, width=0.6), osc("b", "Classic", morph=0.5, level=0.4, fine=-8),
    filt("Formant", 900, 0.45, key=0.3), adsr("amp", 0.7, 1.5, 0.9, 2.0),
    lfo(1, "Sine", 5.0, fade=0.8), lfo(2, "Triangle", 0.11, retrig=False),
    mods(("LFO 2", "Cutoff", 0.15), ("LFO 1", "Pitch", 0.008), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.3),
         ("Gravity", "Sub Level", 0.4), ("Orbit", "LFO 2 Rate", 0.5), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Morph A", 0.3)),
    fx(chorus=(0.3, 0.45, 0.25), reverb=(2.2, 4.0, 0.5, 20, 0.35)))

add("Orion Strings", "Pad", "An ensemble of bowed saws, slow to swell, with a soft chorus shimmer like old string machines.",
    ["strings", "ensemble", "vintage"],
    osc("a", "Classic", morph=0.5, unison=5, detune=0.22, width=0.9), osc("b", "Classic", morph=0.5, oct=1, level=0.3, unison=3, detune=0.2),
    filt("LP 12", 3800, 0.08, key=0.5, vel=0.2), adsr("amp", 0.45, 1.0, 0.9, 1.4),
    lfo(1, "Sine", 5.4, fade=1.0),
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", -0.3),
         ("Orbit", "Detune", 0.35), ("Flux", "FX Chorus", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Amp", 0.25)),
    fx(chorus=(0.6, 0.7, 0.45), reverb=(1.8, 3.0, 0.5, 15, 0.25)))

add("Hubble Deep Field", "Pad", "A dark cinematic bed that slowly resolves detail as the filter envelope unfolds.",
    ["dark", "cinematic", "evolving"],
    osc("a", "Dark Matter", morph=0.2, unison=4, detune=0.2, width=0.8), osc("b", "Event Horizon", morph=0.1, oct=-1, level=0.4),
    filt("LP 24", 500, 0.3, env=30, key=0.3), adsr("amp", 1.8, 2.0, 0.9, 3.5), adsr("fenv", 6.0, 4.0, 0.6, 3.0),
    lfo(1, "Drift", 0.2, retrig=False),
    {"subLevel": 0.3},
    mods(("LFO 1", "Morph A", 0.3), ("Heat", "Cutoff", 0.4), ("Gravity", "Sub Level", 0.5), ("Orbit", "Morph B", 0.4),
         ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.3), ("Key", "Morph A", 0.2)),
    fx(reverb=(2.8, 7.0, 0.55, 40, 0.4)))

add("Cepheid Pulse", "Pad", "A pad that brightens and dims like a variable star, its period locked to the tempo.",
    ["rhythmic", "pulsing", "synced"],
    osc("a", "Pulsar", morph=0.4, unison=4, detune=0.2, width=0.7), osc("b", "Classic", morph=0.6, level=0.4),
    filt("LP 24", 1500, 0.25, key=0.4), adsr("amp", 0.3, 1.0, 0.9, 1.5),
    lfo(1, "Sine", sync=True, div="1/2", retrig=False),
    mods(("LFO 1", "Cutoff", 0.3), ("LFO 1", "Morph A", 0.2), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.35),
         ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Morph A", 0.3)),
    fx(delay=("1/4", 0.3, 0.5, 0.12, True), reverb=(2.0, 3.5, 0.45, 20, 0.3)))

add("Ice Giant", "Pad", "Cold, wide supersaws high in the spectrum: Neptune's blue light as a chord.",
    ["supersaw", "bright", "wide"],
    osc("a", "Classic", morph=0.5, unison=8, detune=0.32, width=1.0), osc("b", "Ion Drive", morph=0.5, oct=1, level=0.3, unison=4, detune=0.25),
    filt("HP 12", 180, 0.1, key=0.0), adsr("amp", 0.25, 1.5, 0.85, 1.8),
    lfo(1, "Sine", 5.6, fade=0.6),
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Morph B", 0.4), ("Gravity", "Cutoff", -0.25),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Reverb", 0.45), ("Aftertouch", "Level B", 0.35), ("Velocity", "Amp", 0.2)),
    fx(chorus=(0.35, 0.4, 0.2), reverb=(2.2, 4.0, 0.3, 20, 0.3), width=1.3))

add("Vacuum Tube Dawn", "Pad", "Lo-fi and warm: a softly saturated triangle pad with tape-like wow from analog drift.",
    ["lo-fi", "warm", "saturated"],
    osc("a", "Classic", morph=0.22, unison=3, detune=0.15, width=0.5), osc("b", "Classic", morph=0.75, level=0.35, fine=6),
    filt("LP 12", 1800, 0.12, key=0.3), adsr("amp", 0.5, 1.2, 0.85, 1.6),
    lfo(1, "Drift", 0.6, retrig=False),
    {"drift": 0.6, "noiseLevel": 0.06, "noiseType": "Pink"},
    mods(("LFO 1", "Pitch", 0.006), ("Heat", "FX Drive", 0.45), ("Gravity", "Cutoff", -0.3), ("Orbit", "LFO 1 Amount", 0.8),
         ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Cutoff", 0.35), ("Aftertouch", "Noise Level", 0.3), ("Heat", "Cutoff", 0.25)),
    fx(drive=("Tube", 0.3, 0.4, 1.0), chorus=(0.2, 0.6, 0.3), reverb=(1.5, 2.5, 0.6, 10, 0.22)))

add("Magnetic Reconnection", "Pad", "Two FM-coupled layers snapping into new shapes as the mod envelope crosses over.",
    ["fm", "evolving", "digital"],
    osc("a", "Magnetar", morph=0.3, unison=2, detune=0.1, warp="FM from B", warpAmt=0.15), osc("b", "Classic", morph=0.0, oct=1, level=0.35),
    filt("LP 24", 3000, 0.2, key=0.5), adsr("amp", 0.6, 1.8, 0.8, 2.2), adsr("menv", 3.0, 3.0, 0.3, 2.0),
    lfo(1, "Triangle", 0.18, retrig=False),
    mods(("Mod Env", "Warp A", 0.35), ("LFO 1", "Morph A", 0.25), ("Heat", "Warp A", 0.4), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Warp A", 0.3), ("Aftertouch", "Cutoff", 0.25)),
    fx(chorus=(0.3, 0.4, 0.2), reverb=(2.0, 4.0, 0.45, 20, 0.3)))

add("Ring Of Fire", "Pad", "A ring-modulated eclipse halo: bell-edged harmonics around a warm core.",
    ["ring mod", "metallic", "halo"],
    osc("a", "Classic", morph=0.4, unison=3, detune=0.18, width=0.7, warp="Ring by B", warpAmt=0.35), osc("b", "Fraunhofer", morph=0.3, semi=7, level=0.3),
    filt("BP 12", 1600, 0.25, key=0.5), adsr("amp", 0.8, 2.0, 0.8, 2.5),
    lfo(1, "Sine", 0.12, retrig=False),
    mods(("LFO 1", "Warp A", 0.15), ("Heat", "Warp A", 0.35), ("Gravity", "Cutoff", -0.3), ("Orbit", "Morph B", 0.45),
         ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Warp A", 0.25), ("Velocity", "Cutoff", 0.15)),
    fx(reverb=(2.5, 5.0, 0.4, 30, 0.35)))

add("Starship Bridge", "Pad", "The quiet hum of a flight deck: a soft square pad with a notch drifting through it.",
    ["soft", "sci-fi", "notch"],
    osc("a", "Classic", morph=0.75, unison=4, detune=0.16, width=0.6), osc("b", "Ion Drive", morph=0.2, oct=-1, level=0.35),
    filt("Notch", 1200, 0.35, key=0.3), adsr("amp", 0.4, 1.5, 0.85, 1.8),
    lfo(1, "Triangle", 0.07, retrig=False),
    mods(("LFO 1", "Cutoff", 0.25), ("Heat", "Morph A", 0.3), ("Gravity", "Level B", 0.4), ("Orbit", "LFO 1 Rate", 0.5),
         ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Cutoff", 0.3), ("Aftertouch", "Morph B", 0.3), ("Flux", "FX Reverb", 0.3)),
    fx(chorus=(0.3, 0.5, 0.2), reverb=(1.8, 3.0, 0.5, 15, 0.25)))

add("Aurora Australis", "Pad", "Southern lights: a shimmering high pad that ripples in slow curtains of morphing colour.",
    ["shimmer", "airy", "evolving"],
    osc("a", "Nebula", morph=0.6, unison=5, detune=0.24, width=1.0), osc("b", "Quasar", morph=0.2, oct=1, level=0.3),
    filt("LP 12", 6500, 0.12, key=0.4), adsr("amp", 1.2, 2.0, 0.85, 3.0),
    lfo(1, "Sine", 0.1, retrig=False), lfo(2, "Triangle", 0.23, retrig=False),
    mods(("LFO 1", "Morph A", 0.35), ("LFO 2", "Morph B", 0.3), ("Heat", "Cutoff", 0.3), ("Gravity", "Cutoff", -0.3),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Delay", 0.45), ("Mod Wheel", "Level B", 0.5), ("Aftertouch", "Morph A", 0.25)),
    fx(chorus=(0.5, 0.5, 0.3), delay=("1/4.", 0.5, 0.7, 0.18, True), reverb=(3.0, 8.0, 0.35, 40, 0.42)))

add("Plasma Torus", "Pad", "A buzzing, folded pad with motion in the middle: Io's plasma ring humming around Jupiter.",
    ["gritty", "motion", "folded"],
    osc("a", "Plasma", morph=0.35, unison=4, detune=0.2, width=0.7, warp="Mirror", warpAmt=0.25), osc("b", "Classic", morph=0.5, oct=-1, level=0.4),
    filt("LP 24", 1700, 0.3, env=8, key=0.4, drive=0.2), adsr("amp", 0.35, 1.2, 0.85, 1.4),
    lfo(1, "Saw Down", sync=True, div="1 bar", retrig=False),
    mods(("LFO 1", "Warp A", 0.2), ("LFO 1", "Cutoff", 0.15), ("Heat", "Filter Drive", 0.5), ("Gravity", "Level B", 0.4),
         ("Orbit", "Morph A", 0.4), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Warp A", 0.35), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.15, 0.6, 1.0), reverb=(1.8, 3.0, 0.5, 15, 0.25)))

add("Silent Running", "Pad", "Barely there: a hushed sine-and-air pad for scoring the space between scenes.",
    ["soft", "ambient", "minimal"],
    osc("a", "Classic", morph=0.0, unison=2, detune=0.08, width=0.6), osc("b", "Classic", morph=0.22, oct=1, level=0.25),
    filt("LP 12", 2500, 0.05, key=0.4), adsr("amp", 1.5, 2.0, 0.9, 3.5),
    lfo(1, "Sine", 4.5, fade=1.5),
    {"noiseLevel": 0.08, "noiseType": "Solar Wind"},
    mods(("LFO 1", "Pitch", 0.006), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Noise Level", 0.4), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.5), ("Aftertouch", "Level B", 0.35), ("Heat", "Cutoff", 0.3)),
    fx(reverb=(2.6, 6.0, 0.4, 35, 0.4)))

add("Lyman Alpha", "Pad", "A bright ultraviolet line: one resonant band sweeping the Fraunhofer spectrum as you hold.",
    ["sweep", "resonant", "bright"],
    osc("a", "Fraunhofer", morph=0.2, unison=4, detune=0.2, width=0.8), osc("b", "Classic", morph=0.5, level=0.35, fine=-5),
    filt("BP 12", 700, 0.55, env=36, key=0.4), adsr("amp", 0.5, 2.0, 0.85, 2.0), adsr("fenv", 4.0, 5.0, 0.5, 3.0),
    adsr("menv", 5.0, 3.0, 0.7, 3.0),
    mods(("Mod Env", "Morph A", 0.5), ("Heat", "Resonance", 0.3), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.4),
         ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Resonance", 0.25)),
    fx(chorus=(0.3, 0.4, 0.2), reverb=(2.2, 4.5, 0.4, 25, 0.32)))

add("Generation Ship", "Pad", "A centuries-long voyage in a chord: Ion Drive layers that slowly quantize into steps.",
    ["evolving", "stepped", "cinematic"],
    osc("a", "Ion Drive", morph=0.3, unison=4, detune=0.2, width=0.8, warp="Quantize", warpAmt=0.1), osc("b", "Nebula", morph=0.5, oct=-1, level=0.4),
    filt("LP 24", 2600, 0.15, key=0.4), adsr("amp", 1.0, 2.0, 0.9, 2.8), adsr("menv", 8.0, 4.0, 0.8, 3.0),
    lfo(1, "S&H", sync=True, div="1 bar", retrig=False),
    mods(("Mod Env", "Warp A", 0.4), ("LFO 1", "Morph B", 0.15), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4),
         ("Orbit", "Morph A", 0.4), ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Warp A", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(chorus=(0.25, 0.4, 0.2), reverb=(2.5, 5.5, 0.45, 30, 0.35)))

# =====================================================================================================
# DRONE
# =====================================================================================================
add("Barycentre", "Drone", "Two bodies in orbit: a pure fifth beating slowly around a shared low centre.",
    ["fifths", "beating", "deep"],
    osc("a", "Classic", morph=0.22, level=0.7, unison=2, detune=0.06), osc("b", "Classic", morph=0.22, semi=7, fine=3, level=0.55),
    filt("LP 24", 1100, 0.2, key=0.3), adsr("amp", 3.0, 2.0, 1.0, 5.0),
    lfo(1, "Sine", 0.05, retrig=False),
    {"subLevel": 0.5, "subShape": "Sine"},
    mods(("LFO 1", "Cutoff", 0.15), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.5), ("Orbit", "Detune", 0.45),
         ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Resonance", 0.3), ("Orbit", "LFO 1 Rate", 0.3)),
    fx(reverb=(3.0, 10.0, 0.5, 40, 0.45)))

add("Schwarzschild Radius", "Drone", "A sub drone at the point of no return, its overtones bending inward with Gravity.",
    ["sub", "dark", "heavy"],
    osc("a", "Event Horizon", morph=0.15, level=0.75, warp="Bend", warpAmt=0.2), osc("b", "Classic", morph=0.0, oct=-1, level=0.6),
    filt("LP 24", 400, 0.35, key=0.2, drive=0.2), adsr("amp", 4.0, 2.0, 1.0, 6.0),
    lfo(1, "Drift", 0.08, retrig=False),
    {"subLevel": 0.6, "subOct": 2},
    mods(("LFO 1", "Warp A", 0.2), ("Heat", "Cutoff", 0.4), ("Gravity", "Warp A", 0.45), ("Orbit", "Morph A", 0.4),
         ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Filter Drive", 0.5), ("Aftertouch", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.3)),
    fx(drive=("Tube", 0.2, 0.4, 1.0), reverb=(2.8, 9.0, 0.6, 30, 0.38)))

add("Solar Wind Tunnel", "Drone", "Charged particles streaming past: filtered solar-wind noise around a hollow tone.",
    ["noise", "wind", "airy"],
    osc("a", "Nebula", morph=0.5, level=0.45), osc("b", "Classic", on=False),
    filt("BP 12", 900, 0.6, key=0.4), adsr("amp", 2.5, 2.0, 1.0, 5.0),
    lfo(1, "Drift", 0.15, retrig=False), lfo(2, "Sine", 0.04, retrig=False),
    {"noiseLevel": 0.7, "noiseType": "Solar Wind"},
    mods(("LFO 1", "Cutoff", 0.3), ("LFO 2", "Pan", 0.6), ("Heat", "Resonance", 0.3), ("Gravity", "Cutoff", -0.35),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Noise Level", -0.5), ("Aftertouch", "Cutoff", 0.3)),
    fx(reverb=(3.0, 8.0, 0.4, 40, 0.42), width=1.4))

add("Resonant Cavity", "Drone", "A comb filter tuned to the note rings like a metal chamber fed with pink noise and a pulse.",
    ["comb", "metallic", "resonant"],
    osc("a", "Classic", morph=0.85, level=0.45), osc("b", "Classic", on=False),
    filt("Comb", 220, 0.82, key=1.0), adsr("amp", 2.0, 2.0, 1.0, 5.0),
    lfo(1, "Triangle", 0.09, retrig=False),
    {"noiseLevel": 0.4, "noiseType": "Pink"},
    mods(("LFO 1", "Resonance", 0.08), ("Heat", "Resonance", 0.15), ("Gravity", "Cutoff", -0.2), ("Orbit", "Morph A", 0.4),
         ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Resonance", 0.15), ("Velocity", "Noise Level", 0.2)),
    fx(reverb=(2.4, 7.0, 0.45, 25, 0.38)))

add("Long Night Of Pluto", "Drone", "Frozen and far: a slowly morphing Dark Matter drone with ice-cold upper partials.",
    ["cold", "morphing", "ambient"],
    osc("a", "Dark Matter", morph=0.0, unison=3, detune=0.1, width=0.8), osc("b", "Quasar", morph=0.5, oct=1, level=0.25),
    filt("LP 12", 2000, 0.15, key=0.3), adsr("amp", 5.0, 2.0, 1.0, 8.0),
    lfo(1, "Sine", 0.03, retrig=False), lfo(2, "Triangle", 0.07, retrig=False),
    mods(("LFO 1", "Morph A", 0.5), ("LFO 2", "Morph B", 0.4), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", -0.3),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Level B", 0.5), ("Aftertouch", "Cutoff", 0.25)),
    fx(reverb=(3.0, 12.0, 0.35, 50, 0.5)))

add("Ion Engine Idle", "Drone", "A spacecraft at rest: a steady electric hum with a faint Square-LFO relay click.",
    ["hum", "machine", "sci-fi"],
    osc("a", "Ion Drive", morph=0.4, level=0.6, unison=2, detune=0.05), osc("b", "Classic", morph=0.75, oct=-1, level=0.45),
    filt("LP 24", 900, 0.4, key=0.3, drive=0.15), adsr("amp", 1.5, 1.0, 1.0, 3.0),
    lfo(1, "Square", 0.5, retrig=False),
    {"noiseLevel": 0.08, "noiseType": "Crackle"},
    mods(("LFO 1", "Cutoff", 0.06), ("Heat", "Filter Drive", 0.5), ("Gravity", "Level B", 0.4), ("Orbit", "LFO 1 Rate", 0.5),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Cutoff", 0.3), ("Heat", "Cutoff", 0.25)),
    fx(delay=(430, 0.45, 0.6, 0.12, True), reverb=(1.8, 4.0, 0.5, 20, 0.3)))

add("Cosmic Background Hum", "Drone", "The universe's oldest light as a chord: octaves and a fifth under a veil of hiss.",
    ["octaves", "warm", "hiss"],
    osc("a", "Classic", morph=0.22, unison=3, detune=0.1, width=0.7), osc("b", "Classic", morph=0.22, oct=1, semi=7, level=0.35),
    filt("LP 12", 1600, 0.1, key=0.3), adsr("amp", 4.0, 2.0, 1.0, 6.0),
    lfo(1, "Drift", 0.1, retrig=False),
    {"subLevel": 0.4, "noiseLevel": 0.12, "noiseType": "Pink"},
    mods(("LFO 1", "Cutoff", 0.1), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Level B", 0.5), ("Aftertouch", "Noise Level", 0.3), ("Velocity", "Cutoff", 0.15)),
    fx(chorus=(0.15, 0.4, 0.2), reverb=(3.0, 10.0, 0.5, 40, 0.45)))

add("Tidal Heating", "Drone", "Europa flexed by Jupiter: a folding drone that strains and relaxes on a slow cycle.",
    ["folded", "pulsing", "tension"],
    osc("a", "Classic", morph=0.5, level=0.65, warp="Mirror", warpAmt=0.2), osc("b", "Magnetar", morph=0.3, oct=-1, level=0.4),
    filt("LP 24", 1300, 0.3, key=0.3), adsr("amp", 3.0, 2.0, 1.0, 5.0),
    lfo(1, "Sine", sync=True, div="2 bars", retrig=False),
    mods(("LFO 1", "Warp A", 0.3), ("LFO 1", "Cutoff", 0.15), ("Heat", "FX Drive", 0.45), ("Gravity", "Level B", 0.4),
         ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Warp A", 0.35), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Fold", 0.15, 0.5, 0.5), reverb=(2.5, 7.0, 0.5, 30, 0.4)))

add("Deep Field Organ", "Drone", "A cathedral pipe drone in the void, with Pulsar harmonics drawn out on the mod wheel.",
    ["organ", "sacred", "sustained"],
    osc("a", "Pulsar", morph=0.2, level=0.6), osc("b", "Classic", morph=0.0, oct=1, level=0.4),
    filt("LP 12", 3000, 0.08, key=0.5), adsr("amp", 1.2, 1.0, 1.0, 4.0),
    lfo(1, "Sine", 5.8, fade=1.0),
    {"subLevel": 0.5, "subShape": "Triangle"},
    mods(("LFO 1", "Pitch", 0.005), ("Aftertouch", "LFO 1 Amount", 1.0), ("Heat", "Morph A", 0.45), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Morph A", 0.5), ("Heat", "Cutoff", 0.25)),
    fx(reverb=(3.0, 9.0, 0.5, 45, 0.45)))

add("Gas Giant Storm", "Drone", "The Great Red Spot: a turbulent, swirling low roar with S&H gusts of filter.",
    ["storm", "turbulent", "low"],
    osc("a", "Plasma", morph=0.3, unison=4, detune=0.25, width=0.8), osc("b", "Classic", morph=0.5, oct=-1, level=0.45),
    filt("LP 24", 700, 0.45, key=0.2, drive=0.25), adsr("amp", 3.0, 2.0, 1.0, 5.0),
    lfo(1, "S&H", 3.0, retrig=False), lfo(2, "Sine", 0.06, retrig=False),
    {"noiseLevel": 0.25, "noiseType": "Pink"},
    mods(("LFO 1", "Cutoff", 0.12), ("LFO 2", "Morph A", 0.35), ("Heat", "Filter Drive", 0.5), ("Gravity", "Level B", 0.4),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.2, 0.5, 1.0), reverb=(2.6, 6.0, 0.55, 25, 0.36)))

add("Voyager Golden Hum", "Drone", "A warm formant drone that seems to speak, its vowel turning once a minute.",
    ["formant", "vocal", "slow"],
    osc("a", "Classic", morph=0.5, unison=3, detune=0.12, width=0.6), osc("b", "Nebula", morph=0.3, oct=-1, level=0.4),
    filt("Formant", 600, 0.5, key=0.2), adsr("amp", 3.0, 2.0, 1.0, 5.0),
    lfo(1, "Triangle", 0.017, retrig=False),
    mods(("LFO 1", "Cutoff", 0.3), ("Heat", "Resonance", 0.3), ("Gravity", "Level B", 0.4), ("Orbit", "LFO 1 Rate", 0.6),
         ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Cutoff", 0.35), ("Aftertouch", "Morph B", 0.35), ("Heat", "Cutoff", 0.15)),
    fx(chorus=(0.2, 0.4, 0.2), reverb=(2.6, 8.0, 0.45, 35, 0.42)))

add("Interferometer", "Drone", "Two Quasar tones a few cents apart: interference fringes sweeping across the stereo field.",
    ["beating", "stereo", "pure"],
    osc("a", "Quasar", morph=0.1, level=0.6, pan=-0.6), osc("b", "Quasar", morph=0.1, fine=4, level=0.6, pan=0.6),
    filt("LP 12", 4000, 0.05, key=0.4), adsr("amp", 3.0, 2.0, 1.0, 5.0),
    lfo(1, "Saw Up", 0.05, retrig=False),
    mods(("LFO 1", "Morph A", 0.3), ("Heat", "Morph B", 0.4), ("Gravity", "Cutoff", -0.3), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Morph B", 0.4), ("Aftertouch", "Level B", 0.3), ("Orbit", "LFO 1 Rate", 0.3)),
    fx(reverb=(2.8, 9.0, 0.4, 40, 0.42), width=1.5))

# =====================================================================================================
# TEXTURE
# =====================================================================================================
add("Particle Detector", "Texture", "Collision tracks: random S&H blips scattering across the spectrum and the stereo field.",
    ["granular", "random", "blips"],
    osc("a", "Quasar", morph=0.5, level=0.55), osc("b", "Classic", on=False),
    filt("BP 12", 2500, 0.5, key=0.3), adsr("amp", 0.5, 1.0, 1.0, 2.0),
    lfo(1, "S&H", sync=True, div="1/32", retrig=False), lfo(2, "S&H", 9.0, retrig=False),
    mods(("LFO 1", "Morph A", 0.6), ("LFO 2", "Pan", 0.9), ("Heat", "Resonance", 0.3),
         ("Gravity", "Cutoff", -0.3), ("Orbit", "LFO 2 Rate", 0.5), ("Flux", "FX Delay", 0.45), ("Mod Wheel", "Resonance", 0.3)),
    {"m8Src": "Aftertouch", "m8Dst": "FX Reverb", "m8Amt": 0.4},
    fx(delay=("1/16.", 0.45, 0.5, 0.2, True), reverb=(2.0, 3.5, 0.4, 15, 0.3)))

add("Static Of Saturn", "Texture", "Radio bursts from the rings: crackle and crunching noise through a wandering band-pass.",
    ["radio", "noise", "crackle"],
    osc("a", "Classic", morph=0.8, level=0.25), osc("b", "Classic", on=False),
    filt("BP 12", 1400, 0.55, key=0.2), adsr("amp", 0.3, 1.0, 1.0, 1.5),
    lfo(1, "Drift", 0.4, retrig=False), lfo(2, "Square", sync=True, div="1/8T", retrig=False),
    {"noiseLevel": 0.8, "noiseType": "Crackle"},
    mods(("LFO 1", "Cutoff", 0.35), ("LFO 2", "Noise Level", 0.3), ("Heat", "FX Drive", 0.5), ("Gravity", "Level A", 0.5),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "LFO 2 Amount", 1.0), ("Aftertouch", "Resonance", 0.3)),
    fx(drive=("Cosmic Ray", 0.25, 0.5, 0.6), reverb=(1.6, 2.5, 0.5, 10, 0.25)))

add("Shutter Array", "Texture", "A pad chopped by tempo-synced gates, like light through a rotating telescope shutter.",
    ["gated", "rhythmic", "synced"],
    osc("a", "Nebula", morph=0.4, unison=4, detune=0.2, width=0.8), osc("b", "Classic", morph=0.5, level=0.35),
    filt("LP 24", 3500, 0.2, key=0.4), adsr("amp", 0.05, 1.0, 1.0, 1.0),
    lfo(1, "Square", sync=True, div="1/16", retrig=True), lfo(2, "Triangle", 0.15, retrig=False),
    mods(("LFO 1", "Amp", -0.9), ("LFO 2", "Morph A", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4),
         ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Delay", 0.45), ("Mod Wheel", "LFO 1 Amount", -0.8), ("Aftertouch", "Cutoff", 0.25)),
    fx(delay=("1/8.", 0.35, 0.5, 0.15, True), reverb=(1.8, 3.0, 0.45, 15, 0.28)))

add("Micrometeor Rain", "Texture", "Countless tiny impacts on a hull: high crackle sparkling in a long stereo echo.",
    ["sparkle", "impacts", "high"],
    osc("a", "Fraunhofer", morph=0.9, level=0.3), osc("b", "Classic", on=False),
    filt("HP 24", 3000, 0.3, key=0.2), adsr("amp", 0.2, 1.0, 1.0, 2.0),
    lfo(1, "S&H", 14.0, retrig=False),
    {"noiseLevel": 0.75, "noiseType": "Crackle"},
    mods(("LFO 1", "Pan", 0.9), ("LFO 1", "Cutoff", 0.2), ("Heat", "Cutoff", 0.3), ("Gravity", "Cutoff", -0.4),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Feedback", 0.35), ("Mod Wheel", "Level A", 0.5), ("Aftertouch", "Noise Level", 0.3)),
    fx(delay=("1/8", 0.5, 0.4, 0.3, True), reverb=(2.4, 5.0, 0.3, 20, 0.35), width=1.5))

add("Orbital Debris Field", "Texture", "Tumbling fragments: metallic Magnetar shards drifting in and out of phase.",
    ["metallic", "drifting", "dissonant"],
    osc("a", "Magnetar", morph=0.6, unison=3, detune=0.4, width=1.0, warp="Ring by B", warpAmt=0.3), osc("b", "Fraunhofer", morph=0.8, semi=11, level=0.3),
    filt("HP 12", 600, 0.2, key=0.3), adsr("amp", 1.0, 1.5, 0.9, 3.0),
    lfo(1, "Drift", 0.3, retrig=False), lfo(2, "S&H", 0.8, retrig=False),
    mods(("LFO 1", "Morph A", 0.4), ("LFO 2", "Morph B", 0.4), ("Heat", "Warp A", 0.35), ("Gravity", "Cutoff", -0.3),
         ("Orbit", "LFO 2 Rate", 0.5), ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Warp A", 0.4), ("Aftertouch", "Level B", 0.35)),
    fx(reverb=(2.6, 6.0, 0.35, 30, 0.4)))

add("Carrier Wave", "Texture", "Tuning across a dead band: a pure carrier sliding under bursts of white noise and hiss.",
    ["radio", "tuning", "lo-fi"],
    osc("a", "Classic", morph=0.0, level=0.5), osc("b", "Classic", morph=0.0, semi=12, level=0.2),
    filt("BP 12", 1800, 0.4, key=0.0), adsr("amp", 0.3, 1.0, 1.0, 1.5),
    lfo(1, "Saw Up", 0.25, retrig=False), lfo(2, "S&H", 2.0, retrig=False),
    {"noiseLevel": 0.45, "noiseType": "White"},
    mods(("LFO 1", "Pitch", 0.06), ("LFO 2", "Noise Level", 0.35), ("Heat", "FX Drive", 0.5),
         ("Gravity", "Cutoff", -0.3), ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Resonance", 0.4)),
    {"m8Src": "Aftertouch", "m8Dst": "Noise Level", "m8Amt": -0.4},
    fx(drive=("Cosmic Ray", 0.2, 0.4, 0.5), reverb=(1.4, 2.0, 0.6, 5, 0.2)))

add("Crystal Lattice", "Texture", "A frozen pattern of glints: quantized Quasar steps pinging through a tempo echo.",
    ["glassy", "stepped", "shimmer"],
    osc("a", "Quasar", morph=0.3, level=0.55, warp="Quantize", warpAmt=0.5), osc("b", "Classic", morph=0.0, oct=2, level=0.2),
    filt("HP 12", 900, 0.15, key=0.3), adsr("amp", 0.4, 1.0, 1.0, 2.5),
    lfo(1, "S&H", sync=True, div="1/8", retrig=False), lfo(2, "Sine", 0.12, retrig=False),
    mods(("LFO 1", "Morph A", 0.5), ("LFO 2", "Warp A", 0.3), ("Heat", "Warp A", 0.3), ("Gravity", "Level B", -0.3),
         ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Delay", 0.45), ("Mod Wheel", "Level B", 0.5), ("Aftertouch", "Morph A", 0.3)),
    fx(delay=("1/8.", 0.55, 0.3, 0.25, True), reverb=(2.6, 5.0, 0.25, 25, 0.38)))

add("Sunquake", "Texture", "Helioseismic rumble: deep, slow pressure waves rolling under a seething surface hiss.",
    ["rumble", "low", "seething"],
    osc("a", "Event Horizon", morph=0.2, level=0.55, oct=-1), osc("b", "Plasma", morph=0.6, level=0.25),
    filt("LP 24", 500, 0.4, key=0.1, drive=0.3), adsr("amp", 1.5, 2.0, 1.0, 4.0),
    lfo(1, "Sine", 0.3, retrig=False), lfo(2, "Drift", 1.5, retrig=False),
    {"noiseLevel": 0.35, "noiseType": "Solar Wind", "subLevel": 0.4},
    mods(("LFO 1", "Amp", 0.35), ("LFO 2", "Cutoff", 0.2), ("Heat", "Filter Drive", 0.5), ("Gravity", "Sub Level", 0.5),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.25, 0.4, 1.0), reverb=(2.5, 6.0, 0.6, 20, 0.35)))

add("Telemetry Noise Floor", "Texture", "Data from a failing probe: stuttering bit-quantized fragments under the hiss.",
    ["glitch", "digital", "lo-fi"],
    osc("a", "Ion Drive", morph=0.6, level=0.45, warp="Quantize", warpAmt=0.75), osc("b", "Classic", morph=0.75, oct=1, level=0.2),
    filt("LP 12", 5000, 0.3, key=0.2), adsr("amp", 0.1, 1.0, 1.0, 1.0),
    lfo(1, "Square", sync=True, div="1/16T", retrig=False), lfo(2, "S&H", sync=True, div="1/16", retrig=False),
    {"noiseLevel": 0.2, "noiseType": "White"},
    mods(("LFO 1", "Amp", -0.6), ("LFO 2", "Morph A", 0.5), ("Heat", "Warp A", 0.25), ("Gravity", "Cutoff", -0.35),
         ("Orbit", "LFO 2 Rate", 0.4), ("Flux", "FX Delay", 0.45), ("Mod Wheel", "LFO 1 Amount", -0.9), ("Aftertouch", "Noise Level", 0.3)),
    fx(drive=("Cosmic Ray", 0.35, 0.5, 0.6), delay=("1/16", 0.4, 0.6, 0.15, True), reverb=(1.4, 2.0, 0.5, 5, 0.2)))

add("Bell Chamber Echoes", "Texture", "A shimmering metallic cloud: comb-filtered ring mod hanging in a very long reverb.",
    ["shimmer", "metallic", "ambient"],
    osc("a", "Fraunhofer", morph=0.5, level=0.5, warp="Ring by B", warpAmt=0.5), osc("b", "Classic", morph=0.0, semi=5, level=0.3),
    filt("Comb", 900, 0.6, key=0.8), adsr("amp", 1.5, 2.0, 0.9, 5.0),
    lfo(1, "Triangle", 0.2, retrig=False), lfo(2, "Sine", 0.06, retrig=False),
    mods(("LFO 1", "Warp A", 0.25), ("LFO 2", "Cutoff", 0.08), ("Heat", "Resonance", 0.2), ("Gravity", "Cutoff", -0.2),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Morph A", 0.5), ("Aftertouch", "Warp A", 0.3)),
    fx(reverb=(3.0, 12.0, 0.25, 60, 0.5), width=1.4))

add("Exoplanet Rainforest", "Texture", "An alien canopy at night: chirping, filtered pink-noise calls in a dense wet space.",
    ["organic", "alien", "nature"],
    osc("a", "Pulsar", morph=0.7, level=0.35), osc("b", "Classic", on=False),
    filt("BP 12", 3000, 0.7, key=0.3), adsr("amp", 0.8, 1.0, 1.0, 2.5),
    lfo(1, "S&H", 5.0, retrig=False), lfo(2, "Saw Down", 7.0, retrig=False),
    {"noiseLevel": 0.6, "noiseType": "Pink"},
    mods(("LFO 1", "Cutoff", 0.35), ("LFO 2", "Pitch", 0.1), ("Heat", "Resonance", 0.2),
         ("Gravity", "Cutoff", -0.35), ("Orbit", "LFO 2 Rate", 0.5), ("Flux", "FX Reverb", 0.5), ("Mod Wheel", "Level A", 0.5)),
    {"m8Src": "Aftertouch", "m8Dst": "LFO 1 Rate", "m8Amt": 0.4},
    fx(delay=("1/8T", 0.4, 0.6, 0.15, True), reverb=(2.6, 4.0, 0.5, 15, 0.42)))

add("Dyson Swarm", "Texture", "Millions of collectors circling a star: dense detuned motion that glitters and swirls.",
    ["swarm", "dense", "motion"],
    osc("a", "Pulsar", morph=0.4, unison=8, detune=0.5, width=1.0), osc("b", "Nebula", morph=0.6, oct=1, unison=6, detune=0.45, level=0.3),
    filt("BP 12", 2000, 0.3, key=0.4), adsr("amp", 1.0, 1.5, 0.9, 3.0),
    lfo(1, "Saw Up", sync=True, div="1 bar", retrig=False), lfo(2, "S&H", 6.0, retrig=False),
    mods(("LFO 1", "Morph A", 0.4), ("LFO 2", "Morph B", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", -0.3),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(chorus=(0.6, 0.6, 0.3), reverb=(2.4, 5.0, 0.4, 25, 0.38), width=1.3))
