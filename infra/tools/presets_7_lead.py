# Leads: the 23 factory leads redesigned for presence, air and width, and new leads covering the market's lead types.

# =====================================================================================================
# LEAD (redesigned: same names and ideas, brighter, wider, denser)
# =====================================================================================================
add("Photon Lead II", "Lead", "A bright, singing saw stack spread across the stereo field; the mod wheel brings in vibrato.",
    ["saw", "classic", "vibrato"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.22, width=0.8), osc("b", "Classic", morph=0.75, oct=1, level=0.35, unison=2, detune=0.12, width=0.7),
    filt("LP 24", 10000, 0.18, env=14, key=0.6), adsr("amp", 0.01, 0.4, 0.85, 0.3), adsr("fenv", 0.01, 0.6, 0.45, 0.3),
    lfo(1, "Sine", 5.2),
    {"voiceMode": "Legato", "glide": 0.06, "glideLegato": True},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Detune", 0.45), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.2)),
    fx(drive=("Soft", 0.15, 0.85, 1.0), chorus=(0.45, 0.35, 0.2), delay=("1/4.", 0.35, 0.4, 0.2, True), reverb=(1.3, 2.2, 0.35, 20, 0.16),
       squash=0.35, mono_low=110))

add("Solar Flare II", "Lead", "Hard-sync scream: the Plasma table swept by an envelope, flaring bright on each note.",
    ["sync", "aggressive", "classic"],
    osc("a", "Plasma", morph=0.25, level=0.8, unison=2, detune=0.12, width=0.6), osc("b", "Classic", morph=0.5, level=0.35, fine=7, unison=2, detune=0.15, width=0.8),
    filt("LP 24", 11000, 0.15, env=8, key=0.5, drive=0.25), adsr("amp", 0.003, 0.3, 0.9, 0.25), adsr("menv", 0.001, 0.7, 0.3, 0.4),
    lfo(1, "Sine", 5.5, fade=0.4),
    {"voiceMode": "Legato", "glide": 0.035},
    mods(("Mod Env", "Morph A", 0.6), ("Mod Wheel", "Morph A", 0.4), ("LFO 1", "Pitch", 0.003), ("Heat", "Filter Drive", 0.5),
         ("Aftertouch", "Morph A", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.3), ("Flux", "FX Delay", 0.4)),
    fx(drive=("Tube", 0.3, 0.9, 1.0), delay=("1/8.", 0.3, 0.35, 0.18, True), reverb=(1.0, 1.8, 0.35, 10, 0.14), squash=0.4, mono_low=110))

add("Cherenkov II", "Lead", "Blue glow in red light: an FM lead with a glassy upper partial that follows aftertouch.",
    ["fm", "glassy", "expressive"],
    osc("a", "Magnetar", morph=0.35, level=0.8, unison=2, detune=0.08, width=0.7), osc("b", "Classic", morph=0.0, oct=1, level=0.25),
    filt("LP 12", 7000, 0.1, key=0.4), adsr("amp", 0.006, 0.5, 0.8, 0.35), adsr("menv", 0.001, 0.4, 0.3, 0.3),
    lfo(1, "Triangle", 5.0),
    {"voiceMode": "Legato", "glide": 0.05},
    mods(("Mod Env", "Morph A", 0.25), ("Aftertouch", "Morph A", 0.35), ("Mod Wheel", "LFO 1 Amount", 1.0), ("LFO 1", "Pitch", 0.01),
         ("Heat", "Morph A", 0.4), ("Gravity", "Level B", -0.25), ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Reverb", 0.4)),
    fx(chorus=(0.5, 0.4, 0.3), delay=("1/4", 0.3, 0.4, 0.2, True), reverb=(1.5, 3.0, 0.4, 30, 0.2), squash=0.25, mono_low=100))

add("Hawking Whistle II", "Lead", "A thin, pure whistle radiating from the edge: sine and noise breath through a band-pass.",
    ["whistle", "breathy", "thin"],
    osc("a", "Classic", morph=0.12, level=0.8), osc("b", "Classic", morph=0.0, oct=1, level=0.2),
    filt("BP 12", 3000, 0.3, key=1.0), adsr("amp", 0.06, 0.4, 0.9, 0.4),
    {"noiseLevel": 0.3, "noiseType": "Pink", "voiceMode": "Legato", "glide": 0.12},
    lfo(1, "Sine", 5.8, fade=0.8),
    mods(("LFO 1", "Pitch", 0.004), ("Mod Wheel", "Noise Level", 0.4), ("Heat", "Cutoff", 0.25), ("Heat", "Resonance", 0.3),
         ("Gravity", "Cutoff", -0.15), ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Amp", 0.25)),
    fx(chorus=(0.35, 0.3, 0.2), delay=("1/4.", 0.35, 0.4, 0.22, True), reverb=(1.8, 4.0, 0.4, 40, 0.25), squash=0.3))

add("Comet Tail II", "Lead", "A lead that leaves a trail: long ping-pong echoes that smear as Flux rises.",
    ["delay", "trail", "wide"],
    osc("a", "Classic", morph=0.6, unison=3, detune=0.15, width=0.8), osc("b", "Fraunhofer", morph=0.3, level=0.35, oct=1, unison=2, detune=0.1, width=0.8),
    filt("LP 24", 9500, 0.2, env=14, key=0.5), adsr("amp", 0.008, 0.6, 0.8, 0.5), adsr("fenv", 0.01, 0.8, 0.45, 0.5),
    lfo(1, "Sine", 5.0, fade=0.7),
    {"voiceMode": "Legato", "glide": 0.05},
    mods(("LFO 1", "Pitch", 0.003), ("Mod Wheel", "Morph A", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.35),
         ("Orbit", "Morph B", 0.5), ("Flux", "FX Feedback", 0.35), ("Flux", "FX Delay", 0.3), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.12, 0.85, 1.0), delay=("1/4.", 0.5, 0.4, 0.3, True), reverb=(1.6, 3.2, 0.45, 20, 0.2), squash=0.3, mono_low=110))

add("Lagrange Point II", "Lead", "Two oscillators balanced a fifth apart, held in place: a hollow, stable square lead with an echo either side.",
    ["fifth", "square", "retro"],
    osc("a", "Classic", morph=0.75, level=0.7, pan=-0.2), osc("b", "Classic", morph=0.75, semi=7, level=0.45, fine=4, pan=0.2),
    filt("LP 24", 10000, 0.2, env=12, key=0.5), adsr("amp", 0.005, 0.3, 0.85, 0.25), adsr("fenv", 0.005, 0.5, 0.35, 0.3),
    lfo(1, "Sine", 5.0),
    {"voiceMode": "Legato", "glide": 0.04},
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Morph A", -0.3), ("Orbit", "Morph B", 0.2), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Level B", 0.3)),
    fx(drive=("Soft", 0.2, 0.95, 1.0), chorus=(0.4, 0.35, 0.25), delay=("1/8.", 0.3, 0.4, 0.18, True), reverb=(1.0, 1.8, 0.4, 10, 0.12), squash=0.4, mono_low=100))

add("Ion Saw II", "Lead", "A thick supersaw for big melodies: seven voices wide, the sub tucked in underneath.",
    ["supersaw", "trance", "big"],
    osc("a", "Classic", morph=0.5, unison=7, detune=0.38, width=1.0), osc("b", "Classic", morph=0.5, oct=1, unison=4, detune=0.3, width=1.0, level=0.35),
    filt("LP 24", 12000, 0.1, env=8, key=0.4), adsr("amp", 0.006, 0.5, 0.85, 0.4), adsr("fenv", 0.005, 0.6, 0.5, 0.4),
    {"subLevel": 0.2, "polyphony": 8},
    mods(("Mod Wheel", "Detune", 0.3), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.4),
         ("Flux", "FX Reverb", 0.35), ("Flux", "FX Delay", 0.2), ("Aftertouch", "Cutoff", 0.2), ("Velocity", "Cutoff", 0.1)),
    fx(chorus=(0.4, 0.3, 0.2), delay=("1/8.", 0.3, 0.4, 0.18, True), reverb=(1.6, 2.8, 0.4, 15, 0.22), squash=0.6, mono_low=120, width=1.3))

add("Doppler Glide II", "Lead", "Portamento lead whose pitch bends like a passing craft: long glide, pitch-bend to an octave.",
    ["glide", "portamento", "expressive"],
    osc("a", "Nebula", morph=0.15, level=0.8, unison=3, detune=0.12, width=0.8), osc("b", "Classic", morph=0.5, level=0.35, oct=-1),
    filt("LP 24", 6000, 0.25, env=12, key=0.6), adsr("amp", 0.01, 0.5, 0.85, 0.35), adsr("fenv", 0.02, 0.6, 0.5, 0.4),
    lfo(1, "Sine", 5.3, fade=0.6),
    {"voiceMode": "Legato", "glide": 0.22, "bendRange": 12},
    mods(("LFO 1", "Pitch", 0.003), ("Mod Wheel", "Morph A", 0.7), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4),
         ("Orbit", "Detune", 0.35), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Pitch", 0.04), ("Flux", "FX Reverb", 0.25)),
    fx(drive=("Soft", 0.15, 0.85, 1.0), delay=("1/4", 0.35, 0.4, 0.2, True), reverb=(1.2, 2.5, 0.4, 20, 0.15), squash=0.35, mono_low=110))

add("Pulsar Voice II", "Lead", "Pulsar synthesis as a lead: a formant that brightens as you play harder.",
    ["formant", "vocal", "pulsar"],
    osc("a", "Pulsar", morph=0.4, level=0.85, unison=2, detune=0.08, width=0.7), osc("b", "Classic", morph=0.5, level=0.3, oct=1),
    filt("LP 12", 10000, 0.15, key=0.5), adsr("amp", 0.01, 0.4, 0.85, 0.3),
    lfo(1, "Sine", 5.0, fade=0.5),
    {"voiceMode": "Legato", "glide": 0.05},
    mods(("Velocity", "Morph A", 0.35), ("Mod Wheel", "Morph A", 0.4), ("LFO 1", "Pitch", 0.003), ("Heat", "Morph A", 0.35),
         ("Gravity", "Level B", 0.4), ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Reverb", 0.35), ("Aftertouch", "Morph A", 0.3)),
    fx(chorus=(0.4, 0.35, 0.25), delay=("1/8.", 0.3, 0.4, 0.15, True), reverb=(1.2, 2.2, 0.4, 20, 0.15), squash=0.5, mono_low=110))

add("Lyman Break II", "Lead", "A clean square lead with a pinch of glide, voiced like the first line of the hydrogen series.",
    ["square", "classic", "clean"],
    osc("a", "Classic", morph=0.75, level=0.75, pan=-0.2), osc("b", "Classic", morph=0.75, level=0.4, fine=7, pan=0.35, oct=1),
    filt("LP 24", 11000, 0.15, env=10, key=0.6), adsr("amp", 0.006, 0.3, 0.9, 0.25), adsr("fenv", 0.005, 0.4, 0.5, 0.3),
    lfo(1, "Sine", 5.4, fade=0.3),
    {"voiceMode": "Legato", "glide": 0.05, "glideLegato": True},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Detune", 0.35), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.25)),
    fx(drive=("Soft", 0.15, 0.95, 1.0), chorus=(0.4, 0.35, 0.25), delay=("1/8.", 0.3, 0.4, 0.18, True), reverb=(1.3, 2.0, 0.4, 15, 0.12), squash=0.4, mono_low=100))

add("Sync Horizon II", "Lead", "A bending Plasma scream that rises in brightness as the pitch wheel comes up.",
    ["sync", "screaming", "bend"],
    osc("a", "Plasma", morph=0.5, level=0.75, unison=3, detune=0.12, width=0.8), osc("b", "Classic", morph=0.5, oct=-1, level=0.3),
    filt("LP 24", 12000, 0.2, env=10, key=0.5, drive=0.3), adsr("amp", 0.002, 0.4, 0.9, 0.3), adsr("fenv", 0.002, 0.5, 0.55, 0.3),
    lfo(1, "Sine", 5.8, fade=0.5),
    {"voiceMode": "Legato", "glide": 0.04, "bendRange": 12},
    mods(("Pitch Bend", "Morph A", 0.4), ("LFO 1", "Pitch", 0.014), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Morph A", 0.4),
         ("Gravity", "Level B", 0.35), ("Orbit", "Detune", 0.35), ("Flux", "FX Delay", 0.4), ("Aftertouch", "Filter Drive", 0.4)),
    fx(drive=("Tube", 0.35, 0.9, 1.0), delay=("1/4", 0.35, 0.4, 0.2, True), reverb=(1.4, 2.4, 0.4, 20, 0.14), squash=0.45, mono_low=110))

add("Chiptune Probe II", "Lead", "An 8-bit pulse quantized to coarse steps, with an arpeggio of fifths and octaves on every note.",
    ["chiptune", "8-bit", "quantize"],
    osc("a", "Classic", morph=0.85, level=0.7, warp="Quantize", warpAmt=0.6), osc("b", "Classic", morph=0.85, level=0.25, fine=8, pan=0.6, warp="Quantize", warpAmt=0.6),
    filt("LP 12", 14000, 0.0, key=0.0), adsr("amp", 0.001, 0.2, 0.8, 0.05),
    arp("Up", "1/32", octaves=1, gate=0.95,
        lane=[(1.0, 0, 1), (1.0, 7, 1), (1.0, 12, 1), (1.0, 7, 1)], length=4),
    mods(("Heat", "Warp A", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph A", -0.3), ("Flux", "FX Delay", 0.35),
         ("Mod Wheel", "Morph A", -0.4), ("Aftertouch", "Warp A", 0.3), ("Velocity", "Amp", 0.2), ("Heat", "Cutoff", 0.2)),
    fx(delay=("1/8", 0.25, 0.2, 0.15, True), reverb=(), squash=0.25, mono_low=100))

add("Voyager Voice II", "Lead", "A vocal lead: the formant filter sweeps from 'oo' to 'ah' on the mod wheel.",
    ["vocal", "formant", "expressive"],
    osc("a", "Nebula", morph=0.55, level=0.75, unison=3, detune=0.12, width=0.8), osc("b", "Classic", morph=0.5, level=0.35),
    filt("Formant", 700, 0.55, env=0, key=0.3), adsr("amp", 0.05, 0.4, 0.9, 0.4),
    lfo(1, "Sine", 5.0, fade=0.6),
    {"voiceMode": "Legato", "glide": 0.08, "glideLegato": True},
    mods(("Mod Wheel", "Cutoff", 0.4), ("LFO 1", "Pitch", 0.01), ("Aftertouch", "LFO 1 Amount", 1.0), ("Heat", "Morph A", 0.35),
         ("Gravity", "Sub Level", 0.35), ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.4), ("Velocity", "Cutoff", 0.15)),
    fx(drive=("Soft", 0.15, 0.9, 1.0), chorus=(0.3, 0.4, 0.25), delay=("1/8.", 0.3, 0.4, 0.12, True), reverb=(1.8, 3.0, 0.4, 25, 0.2),
       squash=0.35, mono_low=110))

add("Fifth Dimension II", "Lead", "Parallel fifths through a warm ladder: the old monosynth trick, tuned in deep space. Centred like a classic mono.",
    ["fifths", "mono", "warm"],
    osc("a", "Classic", morph=0.5, level=0.7), osc("b", "Classic", morph=0.5, semi=7, level=0.55, fine=-3),
    filt("LP 24", 5000, 0.25, env=18, key=0.6), adsr("amp", 0.004, 0.4, 0.85, 0.25), adsr("fenv", 0.002, 0.45, 0.4, 0.3),
    lfo(1, "Triangle", 5.2, fade=0.4),
    {"voiceMode": "Legato", "glide": 0.07, "glideLegato": True},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.45),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Resonance", 0.3), ("Heat", "Filter Drive", 0.3)),
    fx(drive=("Tube", 0.25, 0.8, 1.0), delay=("1/4.", 0.3, 0.45, 0.16, True), reverb=(1.2, 1.8, 0.45, 10, 0.1), squash=0.35))

add("Portamento Comet II", "Lead", "A long, sweeping glide between every note, trailing a bright tail of echoes.",
    ["portamento", "glide", "smooth"],
    osc("a", "Pulsar", morph=0.45, level=0.75, unison=2, detune=0.1, width=0.8), osc("b", "Classic", morph=0.25, oct=1, level=0.25),
    filt("LP 24", 7000, 0.2, env=10, key=0.6), adsr("amp", 0.01, 0.5, 0.9, 0.4), adsr("fenv", 0.01, 0.6, 0.5, 0.4),
    lfo(1, "Sine", 4.8, fade=0.8),
    {"voiceMode": "Legato", "glide": 0.35},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Morph A", 0.35), ("Gravity", "Level B", 0.3),
         ("Orbit", "Morph A", 0.3), ("Flux", "FX Delay", 0.45), ("Aftertouch", "Cutoff", 0.3), ("Heat", "Cutoff", 0.25)),
    fx(delay=("1/4.", 0.5, 0.35, 0.25, True), reverb=(1.8, 3.5, 0.35, 20, 0.18), squash=0.35, mono_low=110))

add("Supersaw Armada II", "Lead", "Eight-voice unison saws on both oscillators: a fleet of leads in perfect formation.",
    ["supersaw", "trance", "unison"],
    osc("a", "Classic", morph=0.5, unison=8, detune=0.4, width=1.0, level=0.65), osc("b", "Classic", morph=0.5, oct=1, unison=6, detune=0.3, width=1.0, level=0.35),
    filt("LP 12", 11000, 0.1, env=8, key=0.5), adsr("amp", 0.005, 0.5, 0.85, 0.35), adsr("fenv", 0.005, 0.6, 0.55, 0.35),
    {"voiceMode": "Poly", "polyphony": 8},
    mods(("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.35), ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.45),
         ("Mod Wheel", "Level B", 0.5), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.15), ("Flux", "FX Delay", 0.3)),
    fx(delay=("1/8.", 0.35, 0.4, 0.18, True), reverb=(2.0, 3.0, 0.4, 20, 0.22), squash=0.6, mono_low=120, width=1.3))

add("Starlight Flute II", "Lead", "A breathy, soft lead: a near-sine with pink breath noise, a delayed vibrato and a little room.",
    ["flute", "soft", "breathy"],
    osc("a", "Classic", morph=0.18, level=0.75), osc("b", "Classic", morph=0.05, oct=1, level=0.35),
    filt("LP 12", 9000, 0.1, env=10, key=0.7), adsr("amp", 0.06, 0.3, 0.85, 0.3), adsr("fenv", 0.04, 0.3, 0.6, 0.3),
    lfo(1, "Sine", 5.0, fade=1.0),
    {"noiseLevel": 0.22, "noiseType": "White", "voiceMode": "Legato", "glide": 0.03},
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Noise Level", 0.4), ("Gravity", "Level B", -0.3),
         ("Orbit", "LFO 1 Rate", 0.2), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Noise Level", 0.2)),
    fx(chorus=(0.3, 0.3, 0.2), delay=("1/4.", 0.25, 0.4, 0.12, True), reverb=(1.6, 2.6, 0.45, 20, 0.22), squash=0.25))

add("Hawking Radiation II", "Lead", "Ring-modulated and cold, a lead that seems to leak out of the event horizon.",
    ["ring mod", "metallic", "cold"],
    osc("a", "Event Horizon", morph=0.3, level=0.7, warp="Ring by B", warpAmt=0.5, unison=2, detune=0.1, width=0.7), osc("b", "Classic", morph=0.0, semi=7, level=0.0),
    filt("LP 24", 10000, 0.2, env=8, key=0.5), adsr("amp", 0.004, 0.4, 0.85, 0.3), adsr("fenv", 0.002, 0.4, 0.45, 0.3),
    lfo(1, "Sine", 5.6, fade=0.4),
    {"voiceMode": "Legato", "glide": 0.05},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Warp A", 0.35), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Pitch B", 0.15), ("Flux", "FX Delay", 0.4), ("Aftertouch", "Warp A", 0.3), ("Heat", "Cutoff", 0.2)),
    fx(chorus=(0.35, 0.3, 0.2), delay=("1/8.", 0.35, 0.35, 0.18, True), reverb=(1.5, 2.4, 0.4, 15, 0.15), squash=0.35, mono_low=110))

add("Magnetar Whistle II", "Lead", "A piercing resonant band-pass whistle, sharp enough to cut through any mix.",
    ["whistle", "resonant", "cutting"],
    osc("a", "Magnetar", morph=0.25, level=0.7, unison=2, detune=0.06, width=0.6), osc("b", "Classic", morph=0.5, level=0.2),
    filt("BP 12", 2600, 0.7, env=12, key=1.0), adsr("amp", 0.01, 0.3, 0.9, 0.25), adsr("fenv", 0.005, 0.3, 0.6, 0.3),
    lfo(1, "Sine", 6.0, fade=0.3),
    {"voiceMode": "Legato", "glide": 0.06, "glideLegato": True},
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Resonance", 0.2), ("Gravity", "Sub Level", 0.35),
         ("Orbit", "Morph A", 0.35), ("Flux", "FX Reverb", 0.35), ("Aftertouch", "Cutoff", 0.3), ("Heat", "Morph A", 0.3)),
    fx(delay=("1/4", 0.3, 0.4, 0.18, True), reverb=(1.2, 2.0, 0.4, 10, 0.15), squash=0.3))

add("Gravity Assist II", "Lead", "A fat, detuned mono lead that slingshots up with the mod envelope on each new note.",
    ["mono", "fat", "pitch envelope"],
    osc("a", "Classic", morph=0.5, level=0.7, unison=3, detune=0.18, width=0.6), osc("b", "Ion Drive", morph=0.5, oct=-1, level=0.4),
    filt("LP 24", 6000, 0.25, env=16, key=0.5, drive=0.25), adsr("amp", 0.003, 0.4, 0.9, 0.25), adsr("fenv", 0.002, 0.4, 0.45, 0.3),
    adsr("menv", 0.001, 0.12, 0.0, 0.1), lfo(1, "Sine", 5.4, fade=0.5),
    {"voiceMode": "Mono", "glide": 0.03},
    mods(("Mod Env", "Pitch", -0.06), ("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.35), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Filter Drive", 0.4)),
    fx(drive=("Tube", 0.3, 0.85, 1.0), delay=("1/8.", 0.3, 0.4, 0.16, True), reverb=(1.2, 1.8, 0.45, 10, 0.1), squash=0.4, mono_low=110))

add("Fraunhofer Lines II", "Lead", "A spectral lead: dark absorption lines sweep across the Fraunhofer table as you hold a note.",
    ["spectral", "evolving", "bright"],
    osc("a", "Fraunhofer", morph=0.25, level=0.75, unison=3, detune=0.1, width=0.8), osc("b", "Classic", morph=0.5, oct=1, level=0.25),
    filt("LP 24", 12000, 0.15, env=6, key=0.5), adsr("amp", 0.01, 0.5, 0.9, 0.35), adsr("menv", 0.3, 2.5, 0.6, 1.0),
    lfo(2, "Triangle", 0.2, retrig=True),
    {"voiceMode": "Legato", "glide": 0.05},
    mods(("Mod Env", "Morph A", 0.45), ("LFO 2", "Morph A", 0.12), ("Mod Wheel", "Morph A", 0.35), ("Aftertouch", "Cutoff", 0.3),
         ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.35), ("Orbit", "LFO 2 Rate", 0.4), ("Flux", "FX Reverb", 0.35)),
    fx(drive=("Soft", 0.15, 0.9, 1.0), delay=("1/4.", 0.3, 0.4, 0.18, True), reverb=(1.6, 2.6, 0.4, 20, 0.16), squash=0.35, mono_low=110))

add("Kuiper Square Lead II", "Lead", "A hollow, folded square that grows teeth when Heat comes up: lo-fi and gritty.",
    ["lo-fi", "fold", "gritty"],
    osc("a", "Classic", morph=0.7, level=0.7, warp="Bend", warpAmt=0.3, pan=-0.25), osc("b", "Classic", morph=0.7, oct=-1, level=0.3, fine=10, pan=0.25),
    filt("LP 12", 11000, 0.2, env=10, key=0.5), adsr("amp", 0.003, 0.4, 0.85, 0.2), adsr("fenv", 0.002, 0.3, 0.45, 0.2),
    lfo(1, "Triangle", 5.5, fade=0.4),
    {"voiceMode": "Legato", "glide": 0.04},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "FX Drive", 0.5), ("Gravity", "Level B", 0.4),
         ("Orbit", "Warp A", 0.3), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Warp A", 0.2)),
    fx(drive=("Fold", 0.3, 0.9, 0.55), chorus=(0.4, 0.3, 0.2), delay=("1/16.", 0.3, 0.45, 0.14, True), reverb=(), squash=0.4, mono_low=100))

add("Dyson Swarm Lead II", "Lead", "Mirror-warped Ion Drive saws in a wide swarm: a hard-edged lead for big drops.",
    ["edm", "mirror", "hard"],
    osc("a", "Ion Drive", morph=0.6, level=0.7, unison=6, detune=0.25, width=1.0, warp="Mirror", warpAmt=0.25),
    osc("b", "Classic", morph=0.5, oct=-1, level=0.2),
    filt("HP 12", 250, 0.15, key=0.0), adsr("amp", 0.002, 0.4, 0.9, 0.25),
    lfo(1, "Sine", 5.5, fade=0.4),
    {"voiceMode": "Legato", "glide": 0.04},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Warp A", 0.4), ("Gravity", "Level B", 0.4),
         ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Morph A", 0.3), ("Velocity", "Warp A", 0.2)),
    fx(drive=("Soft", 0.25, 0.85, 1.0), delay=("1/8", 0.3, 0.4, 0.14, True), reverb=(1.6, 2.2, 0.4, 10, 0.16), squash=0.5, mono_low=120, width=1.2))

# =====================================================================================================
# LEAD (new)
# =====================================================================================================
add("Event Horizon Anthem", "Lead", "The trance supersaw: eight wide voices per oscillator, a dotted echo and a hall that opens with Flux.",
    ["supersaw", "trance", "anthem"],
    osc("a", "Classic", morph=0.5, unison=8, detune=0.42, width=1.0, level=0.7), osc("b", "Classic", morph=0.5, oct=1, unison=8, detune=0.35, width=1.0, level=0.4),
    filt("LP 24", 13000, 0.08, env=6, key=0.4), adsr("amp", 0.004, 0.6, 0.85, 0.45), adsr("fenv", 0.004, 0.7, 0.6, 0.4),
    {"subLevel": 0.15, "polyphony": 8},
    mods(("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.4),
         ("Flux", "FX Delay", 0.3), ("Mod Wheel", "Level B", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.12)),
    fx(chorus=(0.35, 0.3, 0.2), delay=("1/8.", 0.4, 0.4, 0.22, True), reverb=(2.2, 3.5, 0.35, 25, 0.25), squash=0.6, mono_low=120, width=1.35))

add("Mainstage Nova", "Lead", "Big-room festival lead: hard detuned saws, a square an octave up, squashed and pushed to the front.",
    ["big room", "festival", "edm"],
    osc("a", "Classic", morph=0.5, unison=7, detune=0.3, width=0.9, level=0.7), osc("b", "Classic", morph=0.8, oct=1, unison=3, detune=0.2, width=0.8, level=0.45),
    filt("LP 24", 12000, 0.1, env=10, key=0.4), adsr("amp", 0.002, 0.4, 0.9, 0.2), adsr("fenv", 0.002, 0.3, 0.6, 0.2),
    {"polyphony": 8},
    mods(("Heat", "FX Drive", 0.4), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.4),
         ("Mod Wheel", "Cutoff", 0.3), ("Aftertouch", "Level B", 0.3), ("Velocity", "Cutoff", 0.15), ("Heat", "Cutoff", 0.25)),
    fx(drive=("Tube", 0.3, 0.9, 1.0), delay=("1/8", 0.3, 0.4, 0.14, True), reverb=(1.8, 2.4, 0.35, 15, 0.18), squash=0.6, mono_low=120, width=1.25))

add("Tachyon Screech", "Lead", "A hardstyle screech: folded saws through a swept formant, the mod envelope tearing each note open.",
    ["hardstyle", "screech", "distorted"],
    osc("a", "Classic", morph=0.5, unison=4, detune=0.2, width=0.8), osc("b", "Plasma", morph=0.6, level=0.5, unison=2, detune=0.1, width=0.8),
    filt("Formant", 1600, 0.6, env=24, key=0.4, drive=0.4), adsr("amp", 0.002, 0.3, 0.9, 0.2), adsr("fenv", 0.001, 0.25, 0.3, 0.2),
    adsr("menv", 0.001, 0.35, 0.2, 0.2), lfo(1, "Sine", 6.0, fade=0.3),
    {"voiceMode": "Legato", "glide": 0.06},
    mods(("Mod Env", "Morph B", 0.5), ("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "FX Drive", 0.45),
         ("Gravity", "Sub Level", 0.4), ("Orbit", "Cutoff", 0.2), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Resonance", 0.3)),
    fx(drive=("Fold", 0.5, 0.95, 0.8), delay=("1/8", 0.25, 0.4, 0.1, True), reverb=(1.6, 2.0, 0.4, 10, 0.14), squash=0.6, mono_low=120, width=1.2))

add("Sync Lightspeed", "Lead", "The classic sync screamer: FM from B swept by the mod envelope, a hard edge that sings when held.",
    ["sync", "screamer", "classic"],
    osc("a", "Classic", morph=0.45, level=0.8, warp="FM from B", warpAmt=0.22, unison=2, detune=0.1, width=0.7),
    osc("b", "Classic", morph=0.0, oct=1, semi=7, level=0.0),
    filt("LP 24", 6000, 0.2, env=10, key=0.5, drive=0.3), adsr("amp", 0.002, 0.4, 0.9, 0.25), adsr("fenv", 0.002, 0.4, 0.5, 0.25),
    adsr("menv", 0.001, 0.6, 0.25, 0.3), lfo(1, "Sine", 5.6, fade=0.5),
    {"voiceMode": "Legato", "glide": 0.04},
    mods(("Mod Env", "Warp A", 0.3), ("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Warp A", 0.35),
         ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.3), ("Flux", "FX Delay", 0.4), ("Aftertouch", "Warp A", 0.3)),
    fx(drive=("Tube", 0.3, 0.9, 1.0), delay=("1/4.", 0.3, 0.4, 0.18, True), reverb=(1.3, 2.0, 0.4, 15, 0.13), squash=0.45, mono_low=110))

add("Hoover Comet Lead", "Lead", "The rave hoover as a lead: pulse-width saws an octave apart, chorused wide, with a pitch dive on each note.",
    ["hoover", "rave", "mentasm"],
    osc("a", "Pulsar", morph=0.6, unison=5, detune=0.35, width=1.0, level=0.7), osc("b", "Classic", morph=0.5, oct=-1, unison=3, detune=0.3, width=0.9, level=0.45),
    filt("LP 24", 9000, 0.15, env=8, key=0.4), adsr("amp", 0.005, 0.5, 0.9, 0.3), adsr("menv", 0.001, 0.18, 0.0, 0.1),
    lfo(1, "Sine", 5.0, fade=0.5),
    {"voiceMode": "Legato", "glide": 0.1},
    mods(("Mod Env", "Pitch", 0.08), ("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.3),
         ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.4), ("Flux", "FX Chorus", 0.4), ("Aftertouch", "Morph A", 0.3)),
    fx(drive=("Soft", 0.2, 0.85, 1.0), chorus=(0.6, 0.6, 0.45), reverb=(1.6, 2.4, 0.4, 15, 0.15), squash=0.45, mono_low=120))

add("Bit Rover", "Lead", "A chip square for game melodies: a quantized pulse with a quick pitch blip and a vibrato that waits.",
    ["chiptune", "square", "8-bit"],
    osc("a", "Classic", morph=0.8, level=0.75, warp="Quantize", warpAmt=0.5), osc("b", "Classic", morph=0.8, level=0.35, oct=1, fine=-8, warp="Quantize", warpAmt=0.5, pan=0.5),
    filt("LP 12", 15000, 0.0, key=0.0), adsr("amp", 0.001, 0.25, 0.85, 0.06), adsr("menv", 0.001, 0.04, 0.0, 0.03),
    lfo(1, "Triangle", 6.5, fade=0.35),
    {"voiceMode": "Legato", "glide": 0.02},
    mods(("Mod Env", "Pitch", 0.04), ("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Warp A", 0.3),
         ("Gravity", "Sub Level", 0.4), ("Orbit", "Morph A", -0.3), ("Flux", "FX Delay", 0.4), ("Aftertouch", "Warp B", 0.3)),
    fx(delay=("1/8", 0.3, 0.3, 0.18, True), reverb=(1.0, 1.2, 0.4, 5, 0.08), squash=0.25, mono_low=100))

add("Redshift Whistle", "Lead", "The G-funk whistle: a near-sine with a long portamento and a vibrato that blooms after the attack. Centred by design.",
    ["g-funk", "whistle", "portamento"],
    osc("a", "Classic", morph=0.16, level=0.8, oct=1), osc("b", "Classic", morph=0.35, level=0.2),
    filt("LP 12", 11000, 0.05, key=0.8), adsr("amp", 0.02, 0.3, 0.9, 0.3),
    lfo(1, "Sine", 5.6, fade=0.6),
    {"voiceMode": "Legato", "glide": 0.18, "bendRange": 12},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Morph A", 0.3), ("Gravity", "Level B", 0.3),
         ("Orbit", "LFO 1 Rate", 0.3), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Pitch", 0.03), ("Velocity", "Morph A", 0.15)),
    fx(drive=("Tube", 0.2, 0.95, 1.0), delay=("1/8.", 0.25, 0.4, 0.12, True), reverb=(1.2, 1.6, 0.4, 10, 0.12), squash=0.4))

add("Moog Of Mars", "Lead", "A mono analog lead in the classic manner: three saws into a driven ladder with glide. Centred by design.",
    ["analog", "mono", "moog"],
    osc("a", "Classic", morph=0.5, level=0.7, fine=-6), osc("b", "Classic", morph=0.5, level=0.6, fine=6),
    filt("LP 24", 4500, 0.35, env=20, key=0.7, drive=0.35), adsr("amp", 0.003, 0.5, 0.85, 0.2), adsr("fenv", 0.002, 0.5, 0.4, 0.3),
    lfo(1, "Triangle", 5.4, fade=0.5),
    {"voiceMode": "Mono", "glide": 0.08, "subLevel": 0.25, "subShape": "Square", "drift": 0.3},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Cutoff", 0.3), ("Heat", "Filter Drive", 0.3)),
    fx(drive=("Tube", 0.3, 0.85, 1.0), delay=("1/4", 0.25, 0.5, 0.1, True), reverb=(1.0, 1.4, 0.45, 8, 0.08), squash=0.4))

add("Pulsar Pick Lead", "Lead", "A pluck lead for hooks: a bright snap that settles to a short sustain, echoed wide.",
    ["pluck", "hook", "bright"],
    osc("a", "Classic", morph=0.55, unison=3, detune=0.14, width=0.9), osc("b", "Fraunhofer", morph=0.2, oct=1, level=0.35),
    filt("LP 24", 2500, 0.25, env=40, key=0.6), adsr("amp", 0.001, 0.45, 0.25, 0.25), adsr("fenv", 0.001, 0.3, 0.1, 0.2),
    {"polyphony": 8},
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.35), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Delay", 0.4), ("Mod Wheel", "Resonance", 0.3), ("Aftertouch", "Cutoff", 0.25), ("Flux", "FX Reverb", 0.25)),
    fx(drive=("Soft", 0.2, 0.9, 1.0), delay=("1/8.", 0.4, 0.35, 0.25, True), reverb=(1.4, 2.0, 0.35, 15, 0.16), squash=0.45, mono_low=110))

add("Choir Of Io", "Lead", "A vocal lead singing 'ah': stacked Nebula voices through the formant, vowels shifting with the mod wheel.",
    ["vocal", "formant", "choir lead"],
    osc("a", "Nebula", morph=0.7, unison=4, detune=0.12, width=0.9, level=0.75), osc("b", "Pulsar", morph=0.4, level=0.35),
    filt("Formant", 900, 0.5, env=6, key=0.3), adsr("amp", 0.03, 0.4, 0.9, 0.4), adsr("fenv", 0.02, 0.5, 0.5, 0.4),
    lfo(1, "Sine", 5.2, fade=0.7),
    {"voiceMode": "Legato", "glide": 0.06},
    mods(("Mod Wheel", "Cutoff", 0.45), ("LFO 1", "Pitch", 0.008), ("Aftertouch", "LFO 1 Amount", 1.0), ("Heat", "Morph A", 0.3),
         ("Gravity", "Level B", 0.35), ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.4), ("Velocity", "Cutoff", 0.15)),
    fx(drive=("Soft", 0.2, 0.9, 1.0), chorus=(0.35, 0.4, 0.3), delay=("1/4.", 0.3, 0.4, 0.14, True), reverb=(1.8, 3.0, 0.35, 25, 0.22),
       squash=0.4, mono_low=110))

add("Solar Storm Guitar", "Lead", "A distorted, guitar-like lead: driven square and saw through a resonant low-pass, bends to a whole tone.",
    ["distorted", "guitar", "rock"],
    osc("a", "Classic", morph=0.6, level=0.75, unison=2, detune=0.08, width=0.6), osc("b", "Classic", morph=0.75, oct=-1, level=0.4),
    filt("LP 24", 8500, 0.3, env=14, key=0.5, drive=0.5), adsr("amp", 0.002, 0.8, 0.75, 0.3), adsr("fenv", 0.002, 0.6, 0.4, 0.3),
    lfo(1, "Sine", 5.5, fade=0.6),
    {"voiceMode": "Legato", "glide": 0.03, "bendRange": 2},
    mods(("LFO 1", "Pitch", 0.012), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "FX Drive", 0.4), ("Gravity", "Level B", 0.4),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Delay", 0.35), ("Aftertouch", "Filter Drive", 0.35), ("Velocity", "Cutoff", 0.2)),
    fx(drive=("Tube", 0.65, 0.9, 1.0), delay=("1/4", 0.3, 0.5, 0.14, True), reverb=(1.4, 1.8, 0.45, 15, 0.14), squash=0.5, mono_low=120))

add("Tape From Titan", "Lead", "A lo-fi lead: a soft square on worn tape, wobbling in pitch and crushed by the Cosmic Ray stage.",
    ["lo-fi", "tape", "wobble"],
    osc("a", "Classic", morph=0.7, level=0.75), osc("b", "Classic", morph=0.3, oct=1, level=0.2, pan=0.4),
    filt("LP 12", 8000, 0.1, key=0.5), adsr("amp", 0.01, 0.4, 0.85, 0.3),
    lfo(1, "Sine", 5.0, fade=0.5), lfo(2, "Drift", 0.6, retrig=False),
    {"voiceMode": "Legato", "glide": 0.04, "drift": 0.5, "noiseLevel": 0.05, "noiseType": "Crackle"},
    mods(("LFO 2", "Pitch", 0.004), ("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "FX Drive", 0.4),
         ("Gravity", "Level B", -0.2), ("Orbit", "LFO 2 Rate", 0.4), ("Flux", "FX Chorus", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Cosmic Ray", 0.25, 0.75, 0.5), chorus=(0.3, 0.5, 0.3), delay=("1/8.", 0.3, 0.5, 0.15, True), reverb=(1.2, 1.6, 0.5, 10, 0.12),
       squash=0.35, mono_low=100))

add("Aurora Future Lead", "Lead", "A future bass lead: bright detuned saws with a pitch drop on the attack, wide and squashed.",
    ["future bass", "bright", "wide"],
    osc("a", "Classic", morph=0.5, unison=6, detune=0.25, width=1.0, level=0.7), osc("b", "Ion Drive", morph=0.45, oct=1, unison=3, detune=0.2, width=1.0, level=0.35),
    filt("LP 24", 12000, 0.12, env=8, key=0.4), adsr("amp", 0.002, 0.5, 0.8, 0.25), adsr("menv", 0.001, 0.1, 0.0, 0.08),
    lfo(1, "Sine", 5.4, fade=0.5),
    {"polyphony": 8},
    mods(("Mod Env", "Pitch", 0.05), ("LFO 1", "Pitch", 0.008), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.3),
         ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Level B", 0.3)),
    fx(drive=("Soft", 0.2, 0.9, 1.0), chorus=(0.4, 0.35, 0.2), delay=("1/8", 0.3, 0.4, 0.15, True), reverb=(1.8, 2.6, 0.35, 15, 0.2),
       squash=0.6, mono_low=120, width=1.2))

add("Neon Orbit Lead", "Lead", "A synthwave lead: a warm saw-square blend with a ping-pong echo on the dotted eighth and a glossy chorus.",
    ["synthwave", "80s", "ping-pong"],
    osc("a", "Classic", morph=0.55, unison=2, detune=0.12, width=0.7, level=0.75), osc("b", "Classic", morph=0.75, level=0.4, fine=5, pan=0.3),
    filt("LP 24", 9500, 0.2, env=12, key=0.5), adsr("amp", 0.006, 0.5, 0.85, 0.35), adsr("fenv", 0.005, 0.5, 0.5, 0.3),
    lfo(1, "Sine", 5.0, fade=0.6),
    {"voiceMode": "Legato", "glide": 0.05},
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Feedback", 0.3), ("Flux", "FX Delay", 0.3), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.15, 0.85, 1.0), chorus=(0.5, 0.45, 0.35), delay=("1/8.", 0.45, 0.4, 0.28, True), reverb=(1.6, 2.6, 0.4, 20, 0.18),
       squash=0.35, mono_low=110))

add("Jump Gate", "Lead", "An 80s brass lead: saws with a slow filter swell and a bit of bite, as on a big synth-pop riff.",
    ["80s", "brass lead", "synth pop"],
    osc("a", "Classic", morph=0.5, unison=3, detune=0.15, width=0.8, level=0.75), osc("b", "Classic", morph=0.5, level=0.5, fine=8, unison=2, detune=0.1, width=0.8),
    filt("LP 24", 3400, 0.2, env=26, key=0.6), adsr("amp", 0.015, 0.5, 0.85, 0.3), adsr("fenv", 0.06, 0.5, 0.45, 0.3),
    lfo(1, "Sine", 5.2, fade=0.5),
    {"polyphony": 8},
    mods(("Velocity", "Cutoff", 0.25), ("LFO 1", "Pitch", 0.008), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Sub Level", 0.35), ("Orbit", "Detune", 0.3), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Soft", 0.2, 0.9, 1.0), chorus=(0.4, 0.35, 0.25), reverb=(1.6, 2.2, 0.4, 15, 0.16), squash=0.45, mono_low=110))

add("Nebula Drift Lead", "Lead", "An ambient soft lead: a gentle Nebula tone with a slow attack, floating in chorus and a long hall.",
    ["ambient", "soft", "floating"],
    osc("a", "Nebula", morph=0.4, unison=3, detune=0.1, width=0.9, level=0.75), osc("b", "Classic", morph=0.1, oct=1, level=0.2),
    filt("LP 12", 7000, 0.1, key=0.5), adsr("amp", 0.12, 0.6, 0.85, 0.8),
    lfo(1, "Sine", 4.8, fade=1.2), lfo(2, "Triangle", 0.15, retrig=False),
    {"voiceMode": "Legato", "glide": 0.1},
    mods(("LFO 2", "Morph A", 0.15), ("LFO 1", "Pitch", 0.008), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.3),
         ("Gravity", "Level B", 0.3), ("Orbit", "LFO 2 Rate", 0.4), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Morph A", 0.25)),
    fx(chorus=(0.3, 0.5, 0.35), delay=("1/4.", 0.4, 0.45, 0.2, True), reverb=(2.4, 5.0, 0.4, 40, 0.3), squash=0.3, mono_low=100))

add("Bell Of Enceladus", "Lead", "An FM bell-lead: an inharmonic Magnetar strike that settles into a glassy, singing sustain.",
    ["fm", "bell lead", "glassy"],
    osc("a", "Magnetar", morph=0.5, level=0.75, unison=2, detune=0.06, width=0.8), osc("b", "Quasar", morph=0.3, oct=1, semi=7, level=0.3),
    filt("LP 12", 13000, 0.05, key=0.4), adsr("amp", 0.002, 1.2, 0.5, 0.6), adsr("menv", 0.001, 0.5, 0.15, 0.4),
    lfo(1, "Sine", 5.0, fade=0.7),
    {"voiceMode": "Legato", "glide": 0.03},
    mods(("Mod Env", "Morph A", 0.4), ("LFO 1", "Pitch", 0.008), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Morph A", 0.3),
         ("Gravity", "Level B", -0.3), ("Orbit", "Morph B", 0.3), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Morph A", 0.3)),
    fx(chorus=(0.4, 0.3, 0.2), delay=("1/4", 0.35, 0.35, 0.2, True), reverb=(1.8, 3.2, 0.35, 25, 0.22), squash=0.35, mono_low=100))

add("Acid Equinox Lead", "Lead", "An acid lead: a resonant ladder squelching up on accents, sliding between notes.",
    ["acid", "303", "squelch"],
    osc("a", "Classic", morph=0.5, level=0.8), osc("b", "Classic", morph=0.75, level=0.25, fine=5, pan=0.3),
    filt("LP 24", 1500, 0.72, env=40, key=0.6, drive=0.35, vel=0.35), adsr("amp", 0.002, 0.4, 0.85, 0.1), adsr("fenv", 0.001, 0.25, 0.1, 0.15),
    {"voiceMode": "Legato", "glide": 0.06, "glideLegato": True},
    lfo(1, "Sine", 5.5, fade=0.5),
    mods(("Velocity", "Cutoff", 0.25), ("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Resonance", 0.2),
         ("Gravity", "Sub Level", 0.4), ("Orbit", "Cutoff", 0.15), ("Flux", "FX Delay", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.4, 0.85, 1.0), delay=("1/8.", 0.4, 0.4, 0.2, True), reverb=(1.0, 1.4, 0.45, 8, 0.1), squash=0.45, mono_low=110))
