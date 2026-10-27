# Basses: "II" versions of the 26 factory basses, redesigned for weight and bite, and new basses after what commercial banks offer.

# =====================================================================================================
# BASS: II versions of the factory basses (mono low end, clean sub under driven mids, squash for density)
# =====================================================================================================
add("Event Horizon Sub II", "Bass", "A sine sub with a folded edge that only appears when you lean on the keys.",
    ["sub", "deep", "mono"],
    osc("a", "Classic", morph=0.0, level=0.9), osc("b", "Event Horizon", morph=0.08, level=0.3),
    filt("LP 24", 2200, 0.05, env=18, key=0.5), adsr("amp", 0.002, 0.6, 0.9, 0.12), adsr("fenv", 0.001, 0.25, 0.0, 0.2),
    {"voiceMode": "Legato", "glide": 0.04, "drift": 0.05},
    mods(("Velocity", "Morph B", 0.35), ("Heat", "Morph B", 0.6), ("Heat", "Cutoff", 0.25),
         ("Gravity", "Level B", 0.45), ("Orbit", "Detune", 0.4), ("Flux", "FX Drive", 0.4),
         ("Mod Wheel", "Morph B", 0.5), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Tube", 0.35, 0.6, 1.0), reverb=(), sub_clean=110, squash=0.3, mono_low=120), velSens=0.3)

add("Redshift Reese II", "Bass", "Two detuned saws beating against each other, slowly sliding down the spectrum.",
    ["reese", "dnb", "detuned"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.35, width=0.5), osc("b", "Plasma", morph=0.1, oct=0, level=0.5),
    filt("LP 24", 3200, 0.3, env=12, key=0.5, drive=0.3), adsr("amp", 0.005, 0.5, 1.0, 0.2), adsr("fenv", 0.01, 0.6, 0.3, 0.3),
    lfo(1, "Triangle", 0.12, retrig=False),
    {"voiceMode": "Mono", "glide": 0.06},
    mods(("LFO 1", "Cutoff", 0.1), ("Heat", "Cutoff", 0.35), ("Heat", "Filter Drive", 0.5), ("Gravity", "Level B", 0.5),
         ("Orbit", "Detune", 0.45), ("Flux", "FX Chorus", 0.6), ("Mod Wheel", "Morph B", 0.6), ("Aftertouch", "Resonance", 0.3)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), chorus=(0.3, 0.4, 0.15), reverb=(), sub_clean=150, squash=0.3, mono_low=140), velSens=0.3)

add("Magnetar Growl II", "Bass", "FM bass whose index opens with the mod envelope: a magnetic snarl.",
    ["fm", "growl", "dubstep"],
    osc("a", "Magnetar", morph=0.25, level=0.85), osc("b", "Classic", morph=0.0, oct=0, level=0.8),
    filt("LP 12", 4200, 0.35, env=24, key=0.5, drive=0.4), adsr("amp", 0.003, 0.4, 0.85, 0.1), adsr("fenv", 0.002, 0.35, 0.2, 0.2),
    adsr("menv", 0.002, 0.45, 0.1, 0.2),
    {"voiceMode": "Mono", "glide": 0.03},
    mods(("Mod Env", "Morph A", 0.55), ("Mod Wheel", "Morph A", 0.45), ("Heat", "Morph A", 0.45), ("Heat", "Filter Drive", 0.4),
         ("Gravity", "Level B", 0.5), ("Orbit", "Morph A", 0.3), ("Flux", "FX Drive", 0.5), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.6, 0.85, 1.0), reverb=(), sub_clean=150, squash=0.3, mono_low=130), velSens=0.3)

add("Ion Thruster II", "Bass", "Pulse bass with a tempo-locked pulse-width wobble that speeds up as Orbit comes up.",
    ["pwm", "synthwave", "pulse"],
    osc("a", "Classic", morph=0.8, level=0.85), osc("b", "Classic", morph=0.55, oct=0, level=0.5, fine=-6),
    filt("LP 24", 1800, 0.35, env=36, key=0.6), adsr("amp", 0.002, 0.3, 0.85, 0.12), adsr("fenv", 0.001, 0.22, 0.15, 0.15),
    lfo(1, "Triangle", sync=True, div="1/8", retrig=False),
    {"polyphony": 4},
    mods(("LFO 1", "Morph A", 0.1), ("Heat", "Cutoff", 0.4), ("Gravity", "Level B", 0.4), ("Orbit", "LFO 1 Rate", 0.25),
         ("Velocity", "Cutoff", 0.2), ("Flux", "FX Delay", 0.3), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.7, 0.85, 1.0), delay=("1/8.", 0.25, 0.5, 0.0, True), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("Tidal Lock II", "Bass", "A rubbery square that pulls itself into pitch with a short glide on every note.",
    ["glide", "acid", "mono"],
    osc("a", "Classic", morph=0.72, level=0.85), osc("b", "Classic", morph=0.0, level=0.35),
    filt("LP 24", 1100, 0.6, env=40, key=0.6, drive=0.3), adsr("amp", 0.002, 0.4, 0.8, 0.08), adsr("fenv", 0.001, 0.18, 0.0, 0.12),
    {"voiceMode": "Legato", "glide": 0.09, "glideLegato": True},
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Resonance", 0.3), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.6),
         ("Orbit", "Morph A", -0.4), ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Filter Drive", 0.6), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), delay=("1/16.", 0.3, 0.55, 0.0, True), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("Perihelion II", "Bass", "A plucked Fraunhofer bass: bright attack, a dark spectral line sweeping through the decay.",
    ["pluck bass", "bright", "modern"],
    osc("a", "Fraunhofer", morph=0.0, level=0.85, unison=2, detune=0.15, width=0.4), osc("b", "Classic", morph=0.0, oct=0, level=0.55),
    filt("LP 24", 1500, 0.25, env=42, key=0.6), adsr("amp", 0.001, 0.55, 0.7, 0.15), adsr("fenv", 0.001, 0.3, 0.1, 0.2),
    adsr("menv", 0.001, 0.8, 0.0, 0.3),
    mods(("Mod Env", "Morph A", 0.7), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Morph A", 0.5), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.7, 0.85, 1.0), reverb=(0.8, 1.2, 0.5, 5, 0.05), sub_clean=150, squash=0.3, mono_low=130), velSens=0.3)

add("Graviton II", "Bass", "Pure low weight: a triangle and a sine sub with a slow, heavy swell on the mod wheel.",
    ["sub", "808", "clean"],
    osc("a", "Classic", morph=0.3, level=0.8), osc("b", "Classic", morph=0.0, level=0.3),
    filt("LP 12", 2400, 0.0, key=0.5), adsr("amp", 0.002, 1.8, 0.0, 0.4),
    {"subShape": "Sine", "voiceMode": "Mono", "glide": 0.05},
    mods(("Velocity", "Amp", 0.15), ("Heat", "Morph A", 0.3), ("Heat", "Cutoff", 0.4), ("Gravity", "Cutoff", -0.2),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Drive", 0.5), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Pitch", 0.02)),
    fx(drive=("Tube", 0.45, 0.85, 1.0), reverb=(), sub_clean=90, squash=0.3, mono_low=120), velSens=0.3, ampS=0.6)

add("Neutron Star II", "Bass", "Collapsed and dense: a folded wave through the comb filter tuned to the note.",
    ["metallic", "comb", "dense"],
    osc("a", "Event Horizon", morph=0.6, level=0.8), osc("b", "Classic", morph=0.0, oct=0, level=0.8),
    filt("Comb", 262, 0.55, env=0, key=1.0), adsr("amp", 0.002, 0.5, 0.7, 0.15),
    {"voiceMode": "Mono"},
    mods(("Heat", "Morph A", 0.45), ("Heat", "Resonance", 0.3), ("Gravity", "Level B", 0.5), ("Orbit", "Cutoff", 0.08),
         ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Morph A", 0.3), ("Velocity", "Morph A", 0.2)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), reverb=(), sub_clean=150, squash=0.3, mono_low=130), velSens=0.3)

add("Rogue Planet II", "Bass", "A wandering, unbound bass: Dark Matter spectra drifting under a resonant low-pass.",
    ["evolving", "dark", "movement"],
    osc("a", "Dark Matter", morph=0.3, level=0.8, unison=2, detune=0.12), osc("b", "Classic", morph=0.5, oct=0, level=0.4),
    filt("LP 24", 1900, 0.45, env=20, key=0.5), adsr("amp", 0.004, 0.6, 0.95, 0.2), adsr("fenv", 0.003, 0.5, 0.2, 0.3),
    lfo(1, "Drift", 0.25, retrig=False),
    mods(("LFO 1", "Morph A", 0.25), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.45), ("Orbit", "Morph A", 0.4),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Morph A", 0.5), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), chorus=(0.2, 0.3, 0.0), reverb=(), sub_clean=150, squash=0.3, mono_low=140), velSens=0.3, polyphony=4)

add("Coronal Loop II", "Bass", "Acid in infrared: the ladder at the edge of oscillation with an accented 16th lane.",
    ["acid", "303", "arp"],
    osc("a", "Classic", morph=0.5, level=0.85), osc("b", "Classic", morph=0.0, level=0.85),
    filt("LP 24", 700, 0.78, env=46, key=0.6, drive=0.3, vel=0.25), adsr("amp", 0.002, 0.3, 0.95, 0.08), adsr("fenv", 0.001, 0.2, 0.0, 0.1),
    {"voiceMode": "Legato", "glide": 0.05, "glideLegato": True},
    arp("Up", "1/16", octaves=2, gate=0.55, swing=0.2, length=16,
        lane=[(1.0, 0, 1), (0.4, 0, 1), (0.7, 12, 1), (0.4, 0, 1), (1.0, 0, 1), (0.5, -12, 1), (0.8, 0, 2), (0.4, 0, 1),
              (1.0, 7, 1), (0.4, 0, 1), (0.6, 0, 1), (0.9, 12, 1), (0.4, 0, 1), (1.0, 0, 1), (0.4, 0, 1), (0.7, 3, 1)]),
    mods(("Arp Step", "Cutoff", 0.25), ("Heat", "Resonance", 0.2), ("Heat", "Cutoff", 0.25), ("Gravity", "Level B", 0.5),
         ("Orbit", "Morph A", 0.25), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Filter Drive", 0.6), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), delay=("1/8.", 0.35, 0.6, 0.0, True), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("Centre Of Mass II", "Bass", "A pure sine sub with a whisper of second harmonic: the still point everything orbits.",
    ["sub", "clean", "deep"],
    osc("a", "Classic", morph=0.0, level=0.85), osc("b", "Classic", morph=0.5, oct=1, level=0.3),
    filt("LP 12", 1800, 0.0, key=0.5), adsr("amp", 0.003, 0.8, 0.95, 0.15),
    {"voiceMode": "Mono", "glide": 0.03},
    mods(("Heat", "Level B", 0.5), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.5), ("Orbit", "Detune", 0.2),
         ("Flux", "FX Drive", 0.4), ("Mod Wheel", "Morph B", 0.5), ("Aftertouch", "Level B", 0.3), ("Velocity", "Amp", 0.15)),
    fx(drive=("Tube", 0.35, 0.85, 1.0), reverb=(), sub_clean=90, mono_low=120), velSens=0.3)

add("Hoover Comet II", "Bass", "A pitch-dropping detuned stack with a chorus tail: the classic rave growl thrown across the sky.",
    ["hoover", "rave", "detuned"],
    osc("a", "Pulsar", morph=0.85, unison=6, detune=0.45, width=0.8), osc("b", "Classic", morph=0.0, oct=0, level=0.6),
    filt("LP 12", 6000, 0.15, env=10, key=0.6), adsr("amp", 0.004, 0.6, 0.85, 0.25), adsr("menv", 0.001, 0.18, 0.0, 0.1),
    {"voiceMode": "Legato", "glide": 0.12},
    mods(("Mod Env", "Pitch", -0.12), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.45), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Morph A", 0.45), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.2)),
    fx(drive=("Soft", 0.7, 0.85, 1.0), chorus=(0.5, 0.6, 0.35), reverb=(1.2, 1.4, 0.5, 5, 0.06), sub_clean=150, squash=0.3, mono_low=150), velSens=0.3)

add("Wobble Orbit II", "Bass", "A tempo-locked wobble on the ladder; Orbit doubles its speed.",
    ["wobble", "dubstep", "lfo"],
    osc("a", "Magnetar", morph=0.45, level=0.8, unison=2, detune=0.12), osc("b", "Classic", morph=0.5, oct=0, level=0.5),
    filt("LP 24", 1700, 0.45, env=0, key=0.6, drive=0.4), adsr("amp", 0.003, 0.4, 1.0, 0.12),
    lfo(1, "Sine", sync=True, div="1/8", retrig=True),
    {"voiceMode": "Mono", "glide": 0.04},
    mods(("LFO 1", "Cutoff", 0.5), ("Heat", "Filter Drive", 0.45), ("Heat", "Morph A", 0.3), ("Gravity", "Level B", 0.45),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Drive", 0.45), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Morph A", 0.35)),
    fx(drive=("Fold", 0.55, 0.85, 0.7), reverb=(), sub_clean=150, squash=0.3, mono_low=140), velSens=0.3)

add("Funk Thruster II", "Bass", "A snappy, envelope-quacked square for slap-and-pop basslines.",
    ["funk", "envelope", "quack"],
    osc("a", "Classic", morph=0.75, level=0.8), osc("b", "Classic", morph=0.5, level=0.35, fine=8),
    filt("LP 24", 700, 0.55, env=40, key=0.6, vel=0.4), adsr("amp", 0.001, 0.32, 0.75, 0.08), adsr("fenv", 0.001, 0.14, 0.0, 0.08),
    {"voiceMode": "Mono", "drift": 0.1},
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Resonance", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Filter Drive", 0.5), ("Aftertouch", "Cutoff", 0.3), ("Heat", "Cutoff", 0.2)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), chorus=(0.6, 0.3, 0.0), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("Slap Of Saturn II", "Bass", "A slap bass made of FM: a bright thumb attack fading into a round body.",
    ["slap", "fm", "pluck bass"],
    osc("a", "Magnetar", morph=0.45, level=0.65), osc("b", "Classic", morph=0.0, level=0.9),
    filt("LP 12", 4500, 0.1, env=18, key=0.6), adsr("amp", 0.001, 0.9, 0.3, 0.12), adsr("fenv", 0.001, 0.18, 0.0, 0.1),
    adsr("menv", 0.001, 0.15, 0.0, 0.1),
    {"voiceMode": "Mono"},
    mods(("Mod Env", "Morph A", 0.45), ("Velocity", "Morph A", 0.15), ("Heat", "Morph A", 0.3), ("Gravity", "Level B", 0.45),
         ("Orbit", "Morph B", 0.3), ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Morph A", 0.25)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), reverb=(0.7, 0.8, 0.6, 2, 0.04), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("Station Organ Bass II", "Bass", "House organ bass: drawbar-like sines, a short click and no filter to speak of.",
    ["house", "organ", "deep house"],
    osc("a", "Classic", morph=0.0, level=0.8), osc("b", "Ion Drive", morph=0.5, oct=1, level=0.5),
    filt("LP 12", 7000, 0.05, key=0.5), adsr("amp", 0.001, 0.35, 0.6, 0.06),
    {"noiseLevel": 0.05, "noiseType": "Pink", "polyphony": 4},
    mods(("Heat", "Morph B", 0.4), ("Heat", "Cutoff", 0.2), ("Gravity", "Level B", 0.4), ("Orbit", "Morph B", 0.2),
         ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Morph B", 0.5), ("Aftertouch", "Level B", 0.3), ("Velocity", "Amp", 0.2)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), chorus=(0.9, 0.25, 0.0), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("Kessler Syndrome II", "Bass", "A cascade of colliding debris: a folded, cosmic-ray distorted bass that breaks up hard.",
    ["distorted", "aggressive", "industrial"],
    osc("a", "Plasma", morph=0.4, level=0.75, warp="Mirror", warpAmt=0.35), osc("b", "Classic", morph=0.8, oct=0, level=0.5),
    filt("LP 24", 3000, 0.3, env=16, key=0.5, drive=0.5), adsr("amp", 0.002, 0.5, 0.85, 0.1), adsr("fenv", 0.001, 0.3, 0.3, 0.15),
    {"voiceMode": "Mono", "glide": 0.02},
    mods(("Heat", "Warp A", 0.4), ("Heat", "FX Drive", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.35),
         ("Flux", "FX Feedback", 0.3), ("Mod Wheel", "Warp A", 0.4), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Filter Drive", 0.25)),
    fx(drive=("Fold", 0.55, 0.85, 0.8), delay=("1/16", 0.2, 0.6, 0.0, False), reverb=(), sub_clean=150, squash=0.3, mono_low=140), velSens=0.3)

add("Neon Perigee II", "Bass", "Synthwave octave bass: a bright pulse locked to the eighth-note delay.",
    ["synthwave", "80s", "octave"],
    osc("a", "Classic", morph=0.62, level=0.8), osc("b", "Classic", morph=0.22, oct=0, level=0.6, fine=-5),
    filt("LP 24", 2400, 0.3, env=30, key=0.6), adsr("amp", 0.002, 0.25, 0.85, 0.1), adsr("fenv", 0.001, 0.2, 0.2, 0.12),
    {"polyphony": 2},
    mods(("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.3), ("Flux", "FX Delay", 0.35),
         ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.2), ("Flux", "FX Chorus", 0.3)),
    fx(drive=("Soft", 0.7, 0.85, 1.0), chorus=(0.4, 0.4, 0.2), delay=("1/8", 0.25, 0.55, 0.0, True), reverb=(), sub_clean=150, squash=0.3, mono_low=130), velSens=0.3)

add("Deep Core Rumble II", "Bass", "Techno rumble: a long, low thud smeared into dark reverb and filtered back down.",
    ["techno", "rumble", "dark"],
    osc("a", "Classic", morph=0.15, level=0.85), osc("b", "Dark Matter", morph=0.3, level=0.4),
    filt("LP 24", 700, 0.25, env=28, key=0.5), adsr("amp", 0.001, 0.9, 0.0, 0.5), adsr("fenv", 0.001, 0.15, 0.0, 0.2),
    adsr("menv", 0.001, 0.06, 0.0, 0.1),
    {"voiceMode": "Mono"},
    mods(("Mod Env", "Pitch", 0.2), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.4), ("Orbit", "Morph B", 0.35),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Level B", 0.45), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Amp", 0.2)),
    fx(drive=("Tube", 0.5, 0.45, 1.0), reverb=(2.2, 3.5, 0.75, 0, 0.18), sub_clean=90, squash=0.45, mono_low=150), velSens=0.3)

add("Escape Velocity II", "Bass", "A sliding legato saw that overshoots every note, for rolling, rubbery lines.",
    ["glide", "saw", "rolling"],
    osc("a", "Classic", morph=0.5, level=0.8, unison=2, detune=0.08), osc("b", "Classic", morph=0.0, oct=0, level=0.5),
    filt("LP 24", 2000, 0.4, env=28, key=0.6), adsr("amp", 0.002, 0.5, 0.9, 0.12), adsr("fenv", 0.001, 0.3, 0.25, 0.15),
    {"voiceMode": "Legato", "glide": 0.16, "glideLegato": False},
    mods(("Heat", "Cutoff", 0.35), ("Heat", "Resonance", 0.2), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Delay", 0.3), ("Mod Wheel", "Filter Drive", 0.5), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Cutoff", 0.2)),
    fx(drive=("Tube", 0.75, 0.85, 1.0), delay=("1/16", 0.2, 0.6, 0.0, True), reverb=(), sub_clean=130, squash=0.3, mono_low=140), velSens=0.3)

add("Quasar Pluck Bass II", "Bass", "Quasar's harmonic shimmer struck short: a glassy pluck bass for melodic house.",
    ["pluck bass", "melodic", "bright"],
    osc("a", "Quasar", morph=0.2, level=0.75), osc("b", "Classic", morph=0.0, oct=0, level=0.7),
    filt("LP 24", 1400, 0.2, env=42, key=0.6), adsr("amp", 0.001, 0.45, 0.65, 0.15), adsr("fenv", 0.001, 0.25, 0.05, 0.15),
    adsr("menv", 0.001, 0.4, 0.0, 0.2),
    mods(("Mod Env", "Morph A", 0.4), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.35),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Morph A", 0.45), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.7, 0.85, 1.0), reverb=(1.0, 1.2, 0.5, 5, 0.05), sub_clean=150, squash=0.3, mono_low=130), velSens=0.3)

add("Acid Rain On Venus II", "Bass", "A squelching square-ish acid line through the 12 dB filter, its arp lane full of slides.",
    ["acid", "arp", "squelch"],
    osc("a", "Classic", morph=0.7, level=0.85), osc("b", "Classic", morph=0.0, level=0.22),
    filt("LP 12", 1300, 0.82, env=50, key=0.6, drive=0.35, vel=0.3), adsr("amp", 0.002, 0.3, 0.9, 0.08), adsr("fenv", 0.001, 0.16, 0.0, 0.1),
    {"voiceMode": "Legato", "glide": 0.06, "glideLegato": True},
    arp("As Played", "1/16", octaves=1, gate=0.9, swing=0.12,
        lane=[(1.0, 0, 1), (0.6, 12, 1), (0.5, 0, 1), (0.8, 0, 1), (1.0, -12, 1), (0.5, 0, 1), (0.9, 7, 1), (0.5, 0, 1),
              (1.0, 0, 1), (0.6, 10, 1), (0.8, 12, 1), (0.0, 0, 1), (0.9, 0, 2), (0.5, 3, 1), (1.0, 0, 1), (0.6, -5, 1)]),
    mods(("Arp Step", "Cutoff", 0.2), ("Heat", "Resonance", 0.15), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.5),
         ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Filter Drive", 0.6), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), delay=("1/8.", 0.3, 0.6, 0.0, True), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("Nebula Reese II", "Bass", "A wide, slow-phasing Nebula reese with the notch drifting through it.",
    ["reese", "notch", "wide"],
    osc("a", "Nebula", morph=0.4, unison=5, detune=0.3, width=0.7), osc("b", "Classic", morph=0.0, oct=0, level=0.6),
    filt("Notch", 1200, 0.5, key=0.5), adsr("amp", 0.006, 0.6, 1.0, 0.25),
    lfo(1, "Triangle", 0.18, retrig=False), lfo(2, "Drift", 0.1, retrig=False),
    {"voiceMode": "Mono", "glide": 0.05},
    mods(("LFO 1", "Cutoff", 0.15), ("LFO 2", "Morph A", 0.2), ("Heat", "Morph A", 0.35), ("Gravity", "Level B", 0.45),
         ("Orbit", "Detune", 0.4), ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), chorus=(0.25, 0.4, 0.1), reverb=(), sub_clean=150, squash=0.3, mono_low=150), velSens=0.3)

add("Event Horizon Growl II", "Bass", "A talking bass: a resonant vowel opens and closes with the mod envelope.",
    ["formant", "talking", "growl"],
    osc("a", "Event Horizon", morph=0.7, level=0.75), osc("b", "Classic", morph=0.0, oct=0, level=0.7),
    filt("LP 12", 1400, 0.72, env=0, key=0.4), adsr("amp", 0.003, 0.4, 0.9, 0.12), adsr("menv", 0.01, 0.35, 0.3, 0.2),
    lfo(1, "Square", sync=True, div="1/16", retrig=True),
    {"voiceMode": "Mono", "glide": 0.03},
    mods(("Mod Env", "Cutoff", 0.4), ("Heat", "Morph A", 0.45), ("Gravity", "Level B", 0.45), ("Orbit", "LFO 1 Amount", 0.6),
         ("LFO 1", "Cutoff", 0.15), ("Flux", "FX Drive", 0.4), ("Mod Wheel", "Cutoff", 0.35), ("Aftertouch", "Morph A", 0.3)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), reverb=(), sub_clean=150, squash=0.3, mono_low=130), velSens=0.3)

add("Dark Matter Pulse II", "Bass", "An invisible weight felt more than heard: a sidechain-pumping sub that swells on the beat.",
    ["pump", "sub", "sidechain"],
    osc("a", "Dark Matter", morph=0.35, level=0.75), osc("b", "Classic", morph=0.0, oct=0, level=0.55),
    filt("LP 24", 1600, 0.15, key=0.5), adsr("amp", 0.01, 0.6, 1.0, 0.25),
    lfo(1, "Saw Up", sync=True, div="1/4", retrig=False),
    {"polyphony": 4},
    mods(("LFO 1", "Amp", 0.5), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.45), ("Orbit", "LFO 1 Amount", 0.5),
         ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Morph A", 0.5), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.15)),
    fx(drive=("Tube", 0.45, 0.85, 1.0), reverb=(), sub_clean=90, mono_low=120), velSens=0.3)

add("Ion Drive Stepper II", "Bass", "A quantized, stair-stepped Ion Drive tone: digital and gritty, made for minimal techno.",
    ["digital", "quantize", "minimal"],
    osc("a", "Ion Drive", morph=0.85, level=0.8, oct=1, warp="Quantize", warpAmt=0.6), osc("b", "Classic", morph=0.0, oct=0, level=0.75),
    filt("HP 12", 40, 0.1, key=0.0), adsr("amp", 0.001, 0.22, 0.8, 0.08), adsr("menv", 0.001, 0.25, 0.0, 0.1),
    {"voiceMode": "Mono"},
    mods(("Mod Env", "Warp A", 0.35), ("Heat", "Warp A", 0.4), ("Gravity", "Level B", 0.45), ("Orbit", "Morph A", 0.4),
         ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Warp A", 0.35), ("Aftertouch", "Morph A", 0.3), ("Velocity", "Level A", 0.2)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), delay=("1/16.", 0.3, 0.6, 0.0, True), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

# =====================================================================================================
# BASS: new, after what commercial banks offer (808s, neuro, wobble, reese, hoover, analog, FM, house, ...)
# =====================================================================================================
add("Supermassive 808", "Bass", "A long 808 that drops into its note, round underneath and saturated on top.",
    ["808", "trap", "distorted"],
    osc("a", "Classic", morph=0.0, level=0.9), osc("b", "Classic", morph=0.5, oct=1, level=0.3),
    filt("LP 12", 3000, 0.0, key=0.5), adsr("amp", 0.001, 3.0, 0.0, 0.5), adsr("menv", 0.001, 0.08, 0.0, 0.08),
    {"voiceMode": "Mono", "glide": 0.0},
    mods(("Mod Env", "Pitch", 0.25), ("Heat", "FX Drive", 0.4), ("Gravity", "Level B", 0.4), ("Orbit", "Level B", 0.3),
         ("Flux", "FX Reverb", 0.2), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Amp", 0.2)),
    fx(drive=("Tube", 0.65, 0.85, 1.0), reverb=(), sub_clean=90, squash=0.3, mono_low=120), velSens=0.3)

add("Event Horizon Slide", "Bass", "Trap 808 that glides between overlapping notes, distorted just enough to cut.",
    ["808", "glide", "trap"],
    osc("a", "Classic", morph=0.0, level=0.9), osc("b", "Classic", morph=0.5, oct=1, level=0.18),
    filt("LP 12", 2600, 0.0, key=0.5), adsr("amp", 0.001, 2.5, 0.3, 0.4), adsr("menv", 0.001, 0.06, 0.0, 0.06),
    {"voiceMode": "Legato", "glide": 0.12, "glideLegato": True},
    mods(("Mod Env", "Pitch", 0.15), ("Heat", "FX Drive", 0.45), ("Gravity", "Level B", 0.4), ("Orbit", "Level B", 0.35),
         ("Flux", "FX Reverb", 0.2), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Amp", 0.2)),
    fx(drive=("Soft", 0.6, 0.65, 1.0), reverb=(), sub_clean=90, squash=0.3, mono_low=120), velSens=0.3)

add("Pulsar Neuro", "Bass", "Neuro growl: FM-snarled Magnetar through a comb tuned to the note, chewed by a sixteenth LFO.",
    ["neuro", "growl", "dnb"],
    osc("a", "Magnetar", morph=0.4, level=0.8, warp="FM from B", warpAmt=0.35), osc("b", "Classic", morph=0.0, oct=0, level=0.55),
    filt("Comb", 262, 0.5, env=0, key=1.0, drive=0.4), adsr("amp", 0.002, 0.4, 0.95, 0.1),
    lfo(1, "Triangle", sync=True, div="1/16", retrig=True), lfo(2, "S&H", sync=True, div="1/8", retrig=False),
    {"voiceMode": "Mono", "glide": 0.03},
    mods(("LFO 1", "Resonance", 0.3), ("LFO 2", "Morph A", 0.3), ("Heat", "Warp A", 0.4), ("Gravity", "Level B", 0.4),
         ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Drive", 0.4), ("Mod Wheel", "Warp A", 0.45), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), reverb=(), sub_clean=150, squash=0.3, mono_low=140), velSens=0.3)

add("Ring Of Saturn Growl", "Bass", "A ring-modulated growl with a comb resonance: metallic, wet and mean.",
    ["growl", "ring", "dubstep"],
    osc("a", "Plasma", morph=0.3, level=0.8, warp="Ring by B", warpAmt=0.25), osc("b", "Classic", morph=0.0, level=0.9),
    filt("Comb", 131, 0.6, env=12, key=1.0), adsr("amp", 0.002, 0.4, 0.95, 0.12), adsr("fenv", 0.002, 0.3, 0.3, 0.2),
    lfo(1, "Sine", sync=True, div="1/8T", retrig=True),
    {"voiceMode": "Mono"},
    mods(("LFO 1", "Warp A", 0.3), ("Heat", "Resonance", 0.3), ("Gravity", "Level B", 0.4), ("Orbit", "LFO 1 Rate", 0.4),
         ("Flux", "FX Drive", 0.4), ("Mod Wheel", "Warp A", 0.4), ("Aftertouch", "Morph A", 0.3), ("Heat", "FX Drive", 0.3)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), reverb=(), sub_clean=150, squash=0.3, mono_low=140), velSens=0.3)

add("Gravity Well Wobble", "Bass", "Classic half-time wobble: the ladder pulled open by a triplet LFO, squashed hard.",
    ["wobble", "dubstep", "halftime"],
    osc("a", "Classic", morph=0.5, level=0.8, unison=3, detune=0.15, width=0.5), osc("b", "Magnetar", morph=0.6, level=0.5),
    filt("LP 24", 1300, 0.5, env=0, key=0.6, drive=0.45), adsr("amp", 0.002, 0.4, 1.0, 0.1),
    lfo(1, "Sine", sync=True, div="1/4T", retrig=True),
    {"voiceMode": "Mono", "glide": 0.03},
    mods(("LFO 1", "Cutoff", 0.55), ("LFO 1", "Morph B", 0.3), ("Heat", "Filter Drive", 0.4), ("Gravity", "Level B", 0.4),
         ("Orbit", "LFO 1 Rate", 0.5), ("Flux", "FX Drive", 0.4), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Fold", 0.55, 0.85, 0.6), reverb=(), sub_clean=150, squash=0.3, mono_low=140), velSens=0.3)

add("Photon Belt Reese", "Bass", "A rolling DnB reese: eight detuned saws phasing slowly over a locked sub.",
    ["reese", "dnb", "rolling"],
    osc("a", "Classic", morph=0.5, unison=8, detune=0.3, width=0.7), osc("b", "Classic", morph=0.5, unison=4, detune=0.2, width=0.6, fine=-9, level=0.5),
    filt("LP 24", 2800, 0.25, env=8, key=0.5, drive=0.3), adsr("amp", 0.004, 0.5, 1.0, 0.18), adsr("fenv", 0.01, 0.5, 0.5, 0.3),
    lfo(1, "Drift", 0.2, retrig=False),
    {"voiceMode": "Mono", "glide": 0.05},
    mods(("LFO 1", "Detune", 0.15), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Filter Drive", 0.4), ("Velocity", "Cutoff", 0.15)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), chorus=(0.2, 0.3, 0.1), reverb=(), sub_clean=150, squash=0.3, mono_low=150), velSens=0.3)

add("Mothership Hoover", "Bass", "The rave hoover reborn: a pulse stack that swoops down into every note.",
    ["hoover", "rave", "swoop"],
    osc("a", "Pulsar", morph=0.75, unison=8, detune=0.5, width=0.9), osc("b", "Classic", morph=0.0, oct=0, level=0.6),
    filt("LP 12", 5000, 0.1, key=0.5), adsr("amp", 0.003, 0.6, 0.9, 0.3), adsr("menv", 0.001, 0.25, 0.0, 0.1),
    {"voiceMode": "Legato", "glide": 0.15},
    mods(("Mod Env", "Pitch", -0.2), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Chorus", 0.5), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Cutoff", 0.3), ("Flux", "FX Reverb", 0.25)),
    fx(drive=("Soft", 0.7, 0.85, 1.0), chorus=(0.6, 0.6, 0.4), reverb=(1.3, 1.6, 0.5, 8, 0.08), sub_clean=150, squash=0.3, mono_low=150), velSens=0.3)

add("Ladder To Orbit", "Bass", "Fat analog mono: two saws and a square sub through the 24 dB ladder, warm and pushing.",
    ["analog", "fat", "moog"],
    osc("a", "Classic", morph=0.5, level=0.8), osc("b", "Classic", morph=0.5, level=0.6, fine=-7),
    filt("LP 24", 1300, 0.35, env=30, key=0.6, drive=0.35), adsr("amp", 0.002, 0.5, 0.85, 0.15), adsr("fenv", 0.001, 0.35, 0.3, 0.2),
    {"voiceMode": "Legato", "glide": 0.05, "drift": 0.1},
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Cutoff", 0.4), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Chorus", 0.35), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Filter Drive", 0.4), ("Heat", "Filter Drive", 0.3)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("Moonwalk Funk", "Bass", "G-funk style: a bright, quacking mono line with a short glide and a round square sub.",
    ["funk", "g-funk", "glide"],
    osc("a", "Classic", morph=0.6, level=0.8), osc("b", "Classic", morph=0.8, level=0.35, fine=6),
    filt("LP 24", 900, 0.55, env=36, key=0.6, vel=0.35), adsr("amp", 0.002, 0.45, 0.9, 0.1), adsr("fenv", 0.001, 0.2, 0.15, 0.1),
    {"voiceMode": "Legato", "glide": 0.07, "glideLegato": True, "drift": 0.1},
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Resonance", 0.3), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Filter Drive", 0.5), ("Aftertouch", "Cutoff", 0.3), ("Heat", "Cutoff", 0.25)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), chorus=(0.5, 0.3, 0.1), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("Lunar Basement", "Bass", "Deep house sub: a soft sine body with a felt organ overtone and a short pluck.",
    ["deep house", "sub", "warm"],
    osc("a", "Classic", morph=0.08, level=0.85), osc("b", "Ion Drive", morph=0.3, oct=1, level=0.25),
    filt("LP 12", 2200, 0.05, env=14, key=0.5), adsr("amp", 0.002, 0.6, 0.6, 0.12), adsr("fenv", 0.001, 0.2, 0.2, 0.1),
    {"voiceMode": "Mono", "glide": 0.02},
    mods(("Heat", "Level B", 0.4), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.45), ("Orbit", "Morph B", 0.3),
         ("Flux", "FX Chorus", 0.35), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.2)),
    fx(drive=("Tube", 0.3, 0.6, 1.0), reverb=(), sub_clean=90, squash=0.35, mono_low=120), velSens=0.3)

add("Bunker Signal", "Bass", "Industrial techno bass: a distorted thud with a gritty tail and a dark room behind it.",
    ["techno", "industrial", "distorted"],
    osc("a", "Plasma", morph=0.35, level=0.8), osc("b", "Classic", morph=0.0, oct=0, level=0.45),
    filt("LP 24", 3500, 0.3, env=30, key=0.6, drive=0.5), adsr("amp", 0.001, 0.5, 0.8, 0.25), adsr("fenv", 0.001, 0.18, 0.1, 0.2),
    adsr("menv", 0.001, 0.05, 0.0, 0.05),
    {"voiceMode": "Mono"},
    mods(("Mod Env", "Pitch", 0.15), ("Heat", "FX Drive", 0.4), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.35),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Filter Drive", 0.5), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Cutoff", 0.2)),
    fx(drive=("Fold", 0.55, 0.85, 0.7), reverb=(1.6, 2.2, 0.7, 5, 0.06), sub_clean=150, squash=0.3, mono_low=140), velSens=0.3)

add("Hyperspace Roller", "Bass", "Psytrance rolling bass: short, tight and punchy, made for galloping sixteenths.",
    ["psytrance", "rolling", "tight"],
    osc("a", "Classic", morph=0.7, level=0.85), osc("b", "Classic", morph=0.0, oct=0, level=0.45),
    filt("LP 24", 2600, 0.45, env=44, key=0.6, drive=0.3), adsr("amp", 0.001, 0.15, 0.6, 0.05), adsr("fenv", 0.001, 0.08, 0.0, 0.05),
    {"voiceMode": "Mono"},
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Delay", 0.3), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Filter Drive", 0.4), ("Heat", "Filter Drive", 0.3)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("Two-Step Telescope", "Bass", "UK garage bass: a bouncing sine-saw with a quick filter blip and a hint of wobble.",
    ["ukg", "garage", "2-step"],
    osc("a", "Classic", morph=0.3, level=0.85), osc("b", "Classic", morph=0.0, oct=0, level=0.4),
    filt("LP 12", 1800, 0.3, env=24, key=0.6), adsr("amp", 0.002, 0.4, 0.6, 0.1), adsr("fenv", 0.001, 0.15, 0.1, 0.1),
    lfo(1, "Sine", sync=True, div="1/8", retrig=True, fade=0.2),
    {"voiceMode": "Mono", "glide": 0.03},
    mods(("LFO 1", "Cutoff", 0.15), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "LFO 1 Amount", 0.5),
         ("Flux", "FX Chorus", 0.3), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.2)),
    fx(drive=("Tube", 0.35, 0.7, 1.0), reverb=(), sub_clean=100, squash=0.5, mono_low=120), velSens=0.3)

add("Electro Satellite", "Bass", "Electro square bass with a twitchy pulse-width and a sine underneath for weight.",
    ["electro", "square", "pwm"],
    osc("a", "Classic", morph=0.8, level=0.8), osc("b", "Classic", morph=0.0, oct=0, level=0.7),
    filt("LP 24", 2000, 0.4, env=30, key=0.6), adsr("amp", 0.001, 0.3, 0.9, 0.08), adsr("fenv", 0.001, 0.15, 0.2, 0.1),
    lfo(1, "S&H", sync=True, div="1/16", retrig=False),
    {"voiceMode": "Mono"},
    mods(("LFO 1", "Morph A", 0.08), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "LFO 1 Amount", 0.6),
         ("Flux", "FX Delay", 0.3), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Cutoff", 0.2)),
    fx(drive=("Soft", 0.7, 0.85, 1.0), delay=("1/16", 0.2, 0.5, 0.0, True), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("DX Orbit Bass", "Bass", "FM bass in the DX tradition: a woody attack and a clean, solid body.",
    ["fm", "dx", "80s"],
    osc("a", "Magnetar", morph=0.55, level=0.8), osc("b", "Classic", morph=0.0, level=0.75),
    filt("LP 12", 6000, 0.0, key=0.5), adsr("amp", 0.001, 0.7, 0.45, 0.15), adsr("menv", 0.001, 0.3, 0.2, 0.15),
    {"voiceMode": "Mono"},
    mods(("Mod Env", "Morph A", 0.4), ("Velocity", "Morph A", 0.2), ("Heat", "Morph A", 0.35), ("Gravity", "Level B", 0.45),
         ("Orbit", "Morph B", 0.3), ("Flux", "FX Chorus", 0.35), ("Mod Wheel", "Morph A", 0.3), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.35, 0.85, 1.0), chorus=(0.4, 0.3, 0.1), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("Future Horizon Sub", "Bass", "Future bass sub: a round sine under a short, bright saw blip on each note.",
    ["future bass", "sub", "modern"],
    osc("a", "Classic", morph=0.0, level=0.85), osc("b", "Classic", morph=0.5, oct=1, level=0.35, unison=3, detune=0.2, width=0.6),
    filt("LP 24", 1500, 0.1, env=36, key=0.6), adsr("amp", 0.002, 0.8, 0.8, 0.2), adsr("fenv", 0.001, 0.25, 0.1, 0.15),
    {"voiceMode": "Mono", "glide": 0.03},
    mods(("Heat", "Level B", 0.4), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.45), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.2)),
    fx(drive=("Tube", 0.3, 0.7, 1.0), reverb=(), sub_clean=90, squash=0.45, mono_low=130), velSens=0.3)

add("Retro Thrust Pluck", "Bass", "Synthwave pluck bass: a snappy saw with a fast filter drop and an eighth echo.",
    ["synthwave", "pluck bass", "80s"],
    osc("a", "Classic", morph=0.5, level=0.8), osc("b", "Classic", morph=0.0, oct=0, level=0.6),
    filt("LP 24", 1300, 0.3, env=44, key=0.6), adsr("amp", 0.001, 0.35, 0.65, 0.12), adsr("fenv", 0.001, 0.18, 0.0, 0.1),
    {"polyphony": 2},
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.4), ("Aftertouch", "Cutoff", 0.3), ("Flux", "FX Chorus", 0.3)),
    fx(drive=("Soft", 0.7, 0.85, 1.0), chorus=(0.4, 0.4, 0.2), delay=("1/8", 0.25, 0.55, 0.08, True), reverb=(), sub_clean=150, squash=0.3, mono_low=130), velSens=0.3)

add("Club Orbit Stab", "Bass", "House mid-bass stab: a short, punchy chord of saws for chopped basslines.",
    ["house", "stab", "mid bass"],
    osc("a", "Classic", morph=0.5, level=0.75, unison=3, detune=0.15, width=0.5), osc("b", "Classic", morph=0.0, oct=0, level=0.55),
    filt("LP 24", 1600, 0.25, env=36, key=0.6), adsr("amp", 0.001, 0.3, 0.8, 0.1), adsr("fenv", 0.001, 0.15, 0.0, 0.1),
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Reverb", 0.35), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.3), ("Flux", "FX Chorus", 0.3)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), reverb=(0.9, 1.0, 0.5, 4, 0.06), sub_clean=150, squash=0.3, mono_low=140), velSens=0.3)

add("Donk Of Jupiter", "Bass", "The bouncy donk: a hollow, pitched knock with a fast pitch blip and a ringing body.",
    ["donk", "hard house", "bouncy"],
    osc("a", "Classic", morph=0.5, level=0.85, warp="FM from B", warpAmt=0.35), osc("b", "Classic", morph=0.0, level=0.45),
    filt("LP 12", 3000, 0.65, env=30, key=0.8), adsr("amp", 0.001, 0.35, 0.35, 0.1), adsr("fenv", 0.001, 0.08, 0.0, 0.05),
    adsr("menv", 0.001, 0.04, 0.0, 0.04),
    {"voiceMode": "Mono"},
    mods(("Mod Env", "Pitch", 0.15), ("Heat", "Warp A", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.3),
         ("Flux", "FX Delay", 0.3), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Cutoff", 0.25)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("Asteroid Crusher", "Bass", "A rock-flavoured driven bass: pick attack, saturated mids and a sub that never breaks up.",
    ["distorted", "rock", "gritty"],
    osc("a", "Classic", morph=0.6, level=0.8), osc("b", "Classic", morph=0.5, level=0.45, fine=10),
    filt("LP 24", 4000, 0.45, env=20, key=0.7, drive=0.5), adsr("amp", 0.001, 0.6, 0.75, 0.12), adsr("fenv", 0.001, 0.12, 0.4, 0.1),
    {"voiceMode": "Mono"},
    mods(("Velocity", "Filter Drive", 0.3), ("Heat", "FX Drive", 0.4), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Reverb", 0.3), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Filter Drive", 0.4), ("Heat", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.8, 0.9, 1.0), reverb=(), sub_clean=150, squash=0.3, mono_low=130), velSens=0.3)

add("Zero Kelvin Sub", "Bass", "The cleanest sub in the bank: a pure sine with a faint octave for small speakers.",
    ["sub", "clean", "pure"],
    osc("a", "Classic", morph=0.0, level=0.9), osc("b", "Classic", morph=0.5, oct=1, level=0.5),
    filt("LP 12", 1500, 0.0, key=0.5), adsr("amp", 0.003, 0.5, 1.0, 0.12),
    {"voiceMode": "Mono", "glide": 0.02},
    mods(("Heat", "Level B", 0.4), ("Heat", "FX Drive", 0.25), ("Gravity", "Level A", 0.1), ("Orbit", "Morph B", 0.3),
         ("Flux", "FX Reverb", 0.2), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Level B", 0.3), ("Velocity", "Amp", 0.15)),
    fx(drive=("Soft", 0.25, 0.6, 1.0), reverb=(), sub_clean=90, mono_low=120), velSens=0.3)

add("Sulphur Acid", "Bass", "A tight, dry acid line for jacking basslines: squelch, accent, no wobble.",
    ["acid", "303", "tight"],
    osc("a", "Classic", morph=0.75, level=0.85), osc("b", "Classic", morph=0.0, level=0.35),
    filt("LP 24", 600, 0.75, env=48, key=0.6, drive=0.35, vel=0.35), adsr("amp", 0.002, 0.25, 0.75, 0.06), adsr("fenv", 0.001, 0.14, 0.0, 0.08),
    {"voiceMode": "Legato", "glide": 0.05, "glideLegato": True},
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Resonance", 0.15), ("Heat", "Cutoff", 0.3), ("Gravity", "Level B", 0.45),
         ("Orbit", "Morph A", -0.3), ("Flux", "FX Delay", 0.35), ("Mod Wheel", "Filter Drive", 0.6), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), reverb=(), sub_clean=150, squash=0.3, mono_low=120), velSens=0.3)

add("Coronagraph Reese", "Bass", "A dark, notched reese with slow comb motion: neurofunk pads made of low end.",
    ["reese", "neurofunk", "dark"],
    osc("a", "Classic", morph=0.5, unison=6, detune=0.28, width=0.7), osc("b", "Classic", morph=0.0, oct=0, level=0.4),
    filt("Comb", 262, 0.45, env=0, key=1.0), adsr("amp", 0.005, 0.5, 1.0, 0.2),
    lfo(1, "Triangle", 0.15, retrig=False),
    {"voiceMode": "Mono", "glide": 0.05},
    mods(("LFO 1", "Cutoff", 0.06), ("Heat", "Resonance", 0.3), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Morph A", 0.45), ("Aftertouch", "Resonance", 0.3), ("Heat", "FX Drive", 0.3)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), chorus=(0.2, 0.3, 0.1), reverb=(), sub_clean=150, squash=0.3, mono_low=150), velSens=0.3)

add("Talkbox Titan", "Bass", "A vocal 'yoy' bass: a resonant vowel sweeps on every note, over a solid sub.",
    ["yoy", "formant", "vocal"],
    osc("a", "Classic", morph=0.5, level=0.8, unison=2, detune=0.1), osc("b", "Classic", morph=0.0, oct=0, level=0.55),
    filt("LP 12", 450, 0.72, env=30, key=0.4), adsr("amp", 0.002, 0.4, 0.95, 0.1), adsr("fenv", 0.02, 0.3, 0.2, 0.15),
    {"voiceMode": "Mono", "glide": 0.04},
    mods(("Velocity", "Cutoff", 0.2), ("Heat", "Morph A", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Drive", 0.35), ("Mod Wheel", "Cutoff", 0.4), ("Aftertouch", "Resonance", 0.3), ("Heat", "FX Drive", 0.3)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), reverb=(), sub_clean=150, squash=0.3, mono_low=140), velSens=0.3)

add("Pulsar Mid Bass", "Bass", "A modern mid bass: bright Pulsar morph, clean sub, squashed for weight and bite.",
    ["mid bass", "modern", "edm"],
    osc("a", "Pulsar", morph=0.7, level=0.8, unison=3, detune=0.15, width=0.5), osc("b", "Classic", morph=0.0, oct=0, level=0.5),
    filt("LP 24", 5000, 0.2, env=18, key=0.7, drive=0.3), adsr("amp", 0.002, 0.4, 0.85, 0.12), adsr("fenv", 0.001, 0.3, 0.4, 0.15),
    adsr("menv", 0.001, 0.4, 0.2, 0.2),
    {"voiceMode": "Mono", "glide": 0.03},
    mods(("Mod Env", "Morph A", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Morph A", 0.35),
         ("Flux", "FX Drive", 0.35), ("Mod Wheel", "Morph A", 0.4), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Cutoff", 0.2)),
    fx(drive=("Tube", 0.7, 0.85, 1.0), reverb=(), sub_clean=150, squash=0.3, mono_low=140), velSens=0.3)
