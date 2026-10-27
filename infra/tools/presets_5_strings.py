# Strings: string machines, analog poly strings, orchestral-imitative sections and solos, and cinematic hybrids.
# Classic morph: 0.5 is the saw, 0.72-0.85 the pulse family (PWM by modulating Morph A there).
# Where LFO 1 does pulse-width work, the mod-wheel vibrato uses LFO 2, so the wheel never gates the PWM.

# =====================================================================================================
# STRINGS: string machines
# =====================================================================================================
add("Solar Ensemble", "Strings", "A divide-down string machine through a thick three-phase chorus: the sound of a hundred 70s records.",
    ["strings", "string machine", "ensemble", "vintage"],
    osc("a", "Classic", morph=0.5, level=0.7, unison=3, detune=0.12, width=0.6),
    osc("b", "Classic", morph=0.78, level=0.45, oct=1, unison=2, detune=0.1, width=0.6),
    filt("LP 12", 5200, 0.05, key=0.4), adsr("amp", 0.25, 1.0, 1.0, 0.9),
    lfo(2, "Sine", 5.4, fade=0.3),
    mods(("LFO 2", "Pitch", 0.01), ("Mod Wheel", "LFO 2 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", -0.4),
         ("Orbit", "Detune", 0.4), ("Flux", "FX Chorus", 0.5), ("Aftertouch", "Cutoff", 0.2), ("Velocity", "Amp", 0.2)),
    fx(chorus=(0.6, 0.85, 0.7), reverb=(1.6, 2.6, 0.45, 15, 0.22), width=1.3))

add("Eminent Domain", "Strings", "An organ-born ensemble: bright octave-up pulses with a slow, swimming phase.",
    ["strings", "string machine", "eminent", "bright"],
    osc("a", "Classic", morph=0.82, level=0.6, unison=2, detune=0.08, width=0.5),
    osc("b", "Classic", morph=0.82, level=0.5, oct=1, unison=2, detune=0.14, width=0.8),
    filt("LP 12", 6500, 0.0, key=0.3), adsr("amp", 0.12, 1.0, 1.0, 0.7),
    lfo(1, "Triangle", 0.35, retrig=False), lfo(2, "Sine", 5.6, fade=0.2),
    mods(("LFO 1", "Morph A", 0.06), ("LFO 2", "Pitch", 0.01), ("Mod Wheel", "LFO 2 Amount", 1.0), ("Heat", "Cutoff", 0.3),
         ("Gravity", "Level A", 0.3), ("Orbit", "Morph B", 0.2), ("Flux", "FX Chorus", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(chorus=(0.8, 0.7, 0.65), reverb=(1.4, 2.2, 0.5, 10, 0.2), width=1.2))

add("Vox Polaris", "Strings", "Polymoog-style vox strings: a formant body under the ensemble, half choir and half bow.",
    ["strings", "vox", "string machine", "choir"],
    osc("a", "Classic", morph=0.5, level=0.7, unison=4, detune=0.16, width=0.7),
    osc("b", "Nebula", morph=0.4, level=0.5, unison=2, detune=0.1, width=0.5),
    filt("Formant", 1400, 0.35, key=0.2), adsr("amp", 0.3, 1.2, 0.95, 1.1),
    lfo(1, "Drift", 0.15, retrig=False), lfo(2, "Sine", 5.0, fade=0.4),
    mods(("LFO 1", "Cutoff", 0.06), ("LFO 2", "Pitch", 0.009), ("Mod Wheel", "LFO 2 Amount", 1.0), ("Heat", "Cutoff", 0.3),
         ("Gravity", "Level B", 0.4), ("Orbit", "Morph B", 0.35), ("Flux", "FX Chorus", 0.45), ("Aftertouch", "Resonance", 0.25)),
    fx(chorus=(0.45, 0.75, 0.55), reverb=(1.8, 3.0, 0.45, 20, 0.25), width=1.2))

# =====================================================================================================
# STRINGS: analog poly synth strings
# =====================================================================================================
add("Pulsewidth Nebula", "Strings", "Juno-style strings: one pulse oscillator, its width breathing, through the chorus.",
    ["strings", "juno", "pwm", "analog"],
    osc("a", "Classic", morph=0.78, level=0.75, unison=2, detune=0.06, width=0.4), osc("b", "Classic", on=False),
    filt("LP 24", 3800, 0.12, env=6, key=0.5), adsr("amp", 0.35, 1.0, 1.0, 1.0), adsr("fenv", 0.4, 1.2, 0.6, 1.0),
    lfo(1, "Triangle", 0.55, retrig=False), lfo(2, "Sine", 5.2, fade=0.5),
    {"subLevel": 0.25, "subShape": "Square"},
    mods(("LFO 1", "Morph A", 0.09), ("LFO 2", "Pitch", 0.01), ("Mod Wheel", "LFO 2 Amount", 1.0), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Sub Level", 0.45), ("Orbit", "LFO 1 Rate", 0.4), ("Flux", "FX Chorus", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(chorus=(0.5, 0.8, 0.6), reverb=(1.5, 2.4, 0.5, 12, 0.2), width=1.25))

add("Oberon Strings", "Strings", "Brassy two-oscillator strings with a filter that swells into each chord: big, warm, 1979.",
    ["strings", "oberheim", "brassy", "analog"],
    osc("a", "Classic", morph=0.5, level=0.75, unison=2, detune=0.1, width=0.5),
    osc("b", "Classic", morph=0.5, level=0.65, fine=9, unison=2, detune=0.1, width=0.7),
    filt("LP 12", 1500, 0.2, env=24, key=0.5, vel=0.25), adsr("amp", 0.18, 1.0, 0.9, 0.8), adsr("fenv", 0.45, 1.5, 0.5, 0.9),
    lfo(1, "Sine", 5.3, fade=0.6),
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Heat", "Filter Drive", 0.3),
         ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(drive=("Tube", 0.12, 0.55, 1.0), chorus=(0.35, 0.5, 0.3), reverb=(1.6, 2.8, 0.45, 15, 0.22)))

add("Prophecy Strings", "Strings", "A polished five-voice-era string patch: saw and pulse, a soft filter sweep, gently detuned.",
    ["strings", "prophet", "poly", "analog"],
    osc("a", "Classic", morph=0.5, level=0.7, unison=3, detune=0.14, width=0.5),
    osc("b", "Classic", morph=0.74, level=0.5, fine=-6, unison=2, detune=0.12, width=0.6),
    filt("LP 24", 2600, 0.15, env=10, key=0.5), adsr("amp", 0.4, 1.0, 1.0, 1.2), adsr("fenv", 0.6, 1.8, 0.4, 1.2),
    lfo(1, "Triangle", 0.25, retrig=False), lfo(2, "Sine", 5.0, fade=0.5),
    mods(("LFO 1", "Morph B", 0.08), ("LFO 2", "Pitch", 0.009), ("Mod Wheel", "LFO 2 Amount", 1.0), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Level B", 0.35), ("Orbit", "Detune", 0.4), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(chorus=(0.3, 0.45, 0.25), reverb=(1.8, 3.2, 0.5, 18, 0.25)))

add("Off-World Dawn", "Strings", "CS-80 expressive strings: press into a chord and it brightens and sings with vibrato.",
    ["strings", "cs-80", "expressive", "cinematic"],
    osc("a", "Classic", morph=0.5, level=0.75, unison=2, detune=0.12, width=0.6),
    osc("b", "Classic", morph=0.8, level=0.55, unison=2, detune=0.1, width=0.6),
    filt("LP 12", 1300, 0.25, env=18, key=0.5, vel=0.3), adsr("amp", 0.3, 1.5, 0.95, 1.6), adsr("fenv", 0.5, 2.0, 0.5, 1.5),
    lfo(1, "Sine", 5.6, fade=0.3),
    mods(("LFO 1", "Pitch", 0.013), ("Aftertouch", "LFO 1 Amount", 1.0), ("Aftertouch", "Cutoff", 0.35), ("Mod Wheel", "Resonance", 0.4),
         ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.4), ("Orbit", "Detune", 0.35), ("Flux", "FX Delay", 0.35)),
    fx(delay=("1/4.", 0.4, 0.6, 0.12, True), chorus=(0.25, 0.4, 0.3), reverb=(2.4, 5.0, 0.5, 30, 0.32)))

add("Neon Horizon Strings", "Strings", "80s soundtrack strings: wide, bright and a little glossy, made for the title sequence.",
    ["strings", "80s", "soundtrack", "synthwave"],
    osc("a", "Classic", morph=0.5, level=0.7, unison=5, detune=0.22, width=0.8),
    osc("b", "Classic", morph=0.5, level=0.45, oct=1, unison=3, detune=0.18, width=0.8),
    filt("LP 24", 4500, 0.1, env=8, key=0.4), adsr("amp", 0.22, 1.0, 1.0, 1.0), adsr("fenv", 0.3, 1.2, 0.6, 1.0),
    lfo(1, "Sine", 5.2, fade=0.5),
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", -0.35),
         ("Orbit", "Detune", 0.4), ("Flux", "FX Delay", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.15)),
    fx(chorus=(0.4, 0.6, 0.4), delay=("1/8.", 0.35, 0.55, 0.1, True), reverb=(2.0, 3.5, 0.45, 20, 0.28), width=1.3))

# =====================================================================================================
# STRINGS: orchestral-imitative
# =====================================================================================================
add("Galactic Section", "Strings", "A full string section with a slow bow: the attack swells in and the body opens with it.",
    ["strings", "section", "orchestral", "slow"],
    osc("a", "Classic", morph=0.5, level=0.7, unison=6, detune=0.2, width=0.8),
    osc("b", "Classic", morph=0.62, level=0.45, oct=-1, unison=3, detune=0.15, width=0.6),
    filt("LP 12", 1800, 0.08, env=14, key=0.6, vel=0.2), adsr("amp", 0.7, 1.5, 1.0, 1.4), adsr("fenv", 0.9, 2.0, 0.55, 1.4),
    lfo(1, "Sine", 4.8, fade=1.0),
    mods(("LFO 1", "Pitch", 0.008), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Velocity", "Cutoff", 0.25), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.3), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(chorus=(0.2, 0.3, 0.2), reverb=(2.6, 4.0, 0.5, 25, 0.3)))

add("Lone Violin of Vega", "Strings", "A solo violin line: legato glide, a resonant body, and vibrato that arrives after the note settles.",
    ["strings", "solo", "violin", "legato"],
    osc("a", "Classic", morph=0.5, level=0.8), osc("b", "Fraunhofer", morph=0.3, level=0.35, fine=4),
    filt("BP 12", 2200, 0.3, env=8, key=0.8, vel=0.2), adsr("amp", 0.12, 0.8, 0.9, 0.35), adsr("fenv", 0.15, 0.6, 0.6, 0.35),
    lfo(1, "Sine", 5.8, fade=0.7),
    {"voiceMode": "Legato", "glide": 0.07, "glideLegato": True, "noiseLevel": 0.05, "noiseType": "Pink"},
    mods(("LFO 1", "Pitch", 0.012), ("Aftertouch", "LFO 1 Amount", 0.6), ("Mod Wheel", "LFO 1 Amount", 0.6), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Level B", 0.35), ("Orbit", "LFO 1 Rate", 0.25), ("Flux", "FX Reverb", 0.4), ("Velocity", "Cutoff", 0.2)),
    fx(reverb=(1.8, 2.6, 0.5, 20, 0.25)))

add("Saturnine Cello", "Strings", "A cello voice: dark, woody, a little rosin at the start of each stroke.",
    ["strings", "solo", "cello", "legato"],
    osc("a", "Classic", morph=0.5, level=0.8, oct=-1), osc("b", "Classic", morph=0.66, level=0.4, oct=-1, fine=-5),
    filt("Formant", 900, 0.3, env=10, key=0.6, vel=0.2), adsr("amp", 0.15, 1.0, 0.9, 0.5), adsr("fenv", 0.2, 0.8, 0.5, 0.5),
    lfo(1, "Sine", 5.0, fade=0.8),
    {"voiceMode": "Legato", "glide": 0.06, "glideLegato": True, "noiseLevel": 0.06, "noiseType": "Pink"},
    mods(("LFO 1", "Pitch", 0.01), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.3), ("Gravity", "Sub Level", 0.4),
         ("Orbit", "Morph B", 0.3), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Noise Level", 0.2)),
    fx(reverb=(1.8, 2.8, 0.55, 18, 0.22)))

add("Contrabass Gravity Well", "Strings", "Section basses in the lowest octave: slow bows, deep and heavy, the floor of the orchestra.",
    ["strings", "contrabass", "low", "section"],
    osc("a", "Classic", morph=0.5, level=0.8, oct=-2, unison=3, detune=0.12, width=0.4),
    osc("b", "Classic", morph=0.3, level=0.45, oct=-1, unison=2, detune=0.1, width=0.4),
    filt("LP 12", 700, 0.1, env=10, key=0.6, vel=0.2), adsr("amp", 0.35, 1.2, 1.0, 0.9), adsr("fenv", 0.4, 1.0, 0.6, 0.9),
    lfo(1, "Sine", 4.5, fade=1.0),
    mods(("LFO 1", "Pitch", 0.006), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.5),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Reverb", 0.35), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Amp", 0.2)),
    fx(reverb=(2.2, 3.0, 0.6, 20, 0.2)), subLevel=0.2)

add("Pizzicato Pulsars", "Strings", "Plucked strings: a short, round snap with a little wood, and a hall around it.",
    ["strings", "pizzicato", "pluck", "orchestral"],
    osc("a", "Classic", morph=0.45, level=0.8, unison=2, detune=0.08, width=0.5), osc("b", "Classic", morph=0.22, level=0.4, oct=-1),
    filt("LP 12", 900, 0.15, env=36, key=0.6, vel=0.4), adsr("amp", 0.001, 0.35, 0.0, 0.3), adsr("fenv", 0.001, 0.12, 0.0, 0.15),
    mods(("Velocity", "Cutoff", 0.25), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.35),
         ("Flux", "FX Reverb", 0.45), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.25), ("Key", "Cutoff", 0.1)),
    fx(reverb=(2.0, 2.4, 0.5, 15, 0.28)))

add("Spiccato Countdown", "Strings", "A staccato string ostinato on the arp: driving 16ths with bowed accents, ready for the chase scene.",
    ["strings", "spiccato", "ostinato", "arp"],
    osc("a", "Classic", morph=0.5, level=0.8, unison=4, detune=0.15, width=0.6), osc("b", "Classic", morph=0.5, level=0.4, oct=-1),
    filt("LP 12", 1600, 0.12, env=20, key=0.6, vel=0.35), adsr("amp", 0.004, 0.18, 0.0, 0.12), adsr("fenv", 0.002, 0.12, 0.0, 0.1),
    arp("Up/Down", "1/16", octaves=1, gate=0.4,
        lane=[(1.0, 0, 1), (0.55, 0, 1), (0.7, 0, 1), (0.55, 0, 1), (0.95, 0, 1), (0.55, 0, 1), (0.7, 0, 1), (0.6, 0, 1),
              (1.0, 0, 1), (0.55, 0, 1), (0.7, 0, 1), (0.55, 0, 1), (0.9, 0, 1), (0.6, 0, 1), (0.8, 0, 1), (0.65, 0, 1)]),
    mods(("Velocity", "Cutoff", 0.3), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.4), ("Orbit", "Detune", 0.3),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Filter Drive", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Arp Step", "Amp", 0.15)),
    fx(reverb=(1.8, 2.0, 0.5, 12, 0.22)))

add("Tremolo Transmission", "Strings", "Bowed tremolo: a section trembling fast and quiet, rising with the mod wheel.",
    ["strings", "tremolo", "section", "tension"],
    osc("a", "Classic", morph=0.5, level=0.7, unison=5, detune=0.18, width=0.8), osc("b", "Classic", morph=0.62, level=0.4, unison=2, detune=0.12),
    filt("LP 12", 2200, 0.1, env=8, key=0.5), adsr("amp", 0.3, 1.0, 1.0, 1.0),
    lfo(1, "Triangle", 11.0, retrig=False), lfo(2, "Sine", 5.0, fade=0.6),
    mods(("LFO 1", "Amp", 0.45), ("Mod Wheel", "LFO 1 Rate", 0.3), ("LFO 2", "Pitch", 0.006), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Level B", 0.35), ("Orbit", "LFO 1 Rate", 0.25), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(reverb=(2.2, 3.2, 0.5, 20, 0.28)))

add("Ponticello Ice", "Strings", "Sul ponticello: bowed at the bridge, glassy and thin, all upper partials and frost.",
    ["strings", "ponticello", "glassy", "eerie"],
    osc("a", "Quasar", morph=0.6, level=0.65, unison=4, detune=0.14, width=0.7), osc("b", "Classic", morph=0.5, level=0.4, oct=1, unison=2),
    filt("HP 12", 900, 0.25, env=4, key=0.5), adsr("amp", 0.5, 1.0, 1.0, 1.2),
    lfo(1, "Sine", 6.2, fade=0.4), lfo(2, "Drift", 0.2, retrig=False),
    {"noiseLevel": 0.08, "noiseType": "White"},
    mods(("LFO 1", "Pitch", 0.006), ("Mod Wheel", "LFO 1 Amount", 1.0), ("LFO 2", "Morph A", 0.12), ("Heat", "Cutoff", 0.3),
         ("Gravity", "Level B", -0.3), ("Orbit", "Morph A", 0.3), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Noise Level", 0.3)),
    fx(reverb=(2.4, 4.5, 0.35, 25, 0.32)))

add("Flautando Halo", "Strings", "Harmonics and flautando: airy, breathy, barely touching the string, more air than tone.",
    ["strings", "flautando", "harmonics", "airy"],
    osc("a", "Classic", morph=0.15, level=0.65, oct=1, unison=3, detune=0.1, width=0.8), osc("b", "Nebula", morph=0.2, level=0.4, unison=2, detune=0.12),
    filt("LP 12", 3200, 0.05, key=0.5), adsr("amp", 0.6, 1.5, 0.9, 1.8),
    lfo(1, "Sine", 4.6, fade=1.0),
    {"noiseLevel": 0.18, "noiseType": "Pink"},
    mods(("LFO 1", "Pitch", 0.008), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Noise Level", 0.3), ("Gravity", "Level B", 0.35),
         ("Orbit", "Morph B", 0.35), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Amp", 0.2)),
    fx(chorus=(0.2, 0.35, 0.2), reverb=(2.6, 5.0, 0.45, 30, 0.35)))

add("Octave Binary", "Strings", "Unison strings doubled at the octave: violins and violas together, a broad singing line.",
    ["strings", "octaves", "unison", "section"],
    osc("a", "Classic", morph=0.5, level=0.7, unison=4, detune=0.16, width=0.7),
    osc("b", "Classic", morph=0.5, level=0.55, oct=-1, unison=4, detune=0.16, width=0.7),
    filt("LP 12", 2400, 0.08, env=10, key=0.6, vel=0.2), adsr("amp", 0.25, 1.2, 1.0, 1.0), adsr("fenv", 0.35, 1.4, 0.6, 1.0),
    lfo(1, "Sine", 5.1, fade=0.6),
    {"voiceMode": "Legato", "glide": 0.05, "glideLegato": True},
    mods(("LFO 1", "Pitch", 0.009), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Velocity", "Cutoff", 0.2), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Level B", 0.35), ("Orbit", "Detune", 0.35), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Cutoff", 0.3)),
    fx(reverb=(2.2, 3.2, 0.5, 20, 0.26)))

# =====================================================================================================
# STRINGS: modern and cinematic
# =====================================================================================================
add("Hybrid Ascension", "Strings", "A cinematic hybrid swell: strings rising out of a filtered pulse bed, the trailer crescendo in one note.",
    ["strings", "cinematic", "hybrid", "swell"],
    osc("a", "Classic", morph=0.5, level=0.7, unison=6, detune=0.22, width=0.8), osc("b", "Ion Drive", morph=0.3, level=0.5, oct=-1, unison=2, detune=0.1),
    filt("LP 24", 500, 0.2, env=40, key=0.5), adsr("amp", 1.2, 2.0, 1.0, 2.0), adsr("fenv", 2.5, 2.0, 0.8, 2.0),
    lfo(1, "Sine", sync=True, div="1/8", retrig=False),
    mods(("LFO 1", "Amp", 0.15), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.4), ("Gravity", "Sub Level", 0.45),
         ("Orbit", "Morph B", 0.35), ("Flux", "FX Reverb", 0.45), ("Aftertouch", "Filter Drive", 0.4), ("Velocity", "Cutoff", 0.2)),
    fx(drive=("Tube", 0.1, 0.5, 1.0), reverb=(2.8, 6.0, 0.5, 30, 0.35)))

add("Umbra Low Strings", "Strings", "Dark low strings for the shadow side: cellos and basses, muted and menacing.",
    ["strings", "dark", "low", "cinematic"],
    osc("a", "Dark Matter", morph=0.35, level=0.7, oct=-1, unison=4, detune=0.15, width=0.6),
    osc("b", "Classic", morph=0.5, level=0.55, oct=-1, unison=3, detune=0.12, width=0.5),
    filt("LP 24", 750, 0.15, env=10, key=0.5), adsr("amp", 0.6, 1.5, 1.0, 1.5), adsr("fenv", 0.8, 1.5, 0.5, 1.5),
    lfo(1, "Drift", 0.1, retrig=False),
    mods(("LFO 1", "Morph A", 0.15), ("Heat", "Cutoff", 0.35), ("Gravity", "Sub Level", 0.5), ("Orbit", "Morph A", 0.35),
         ("Flux", "FX Reverb", 0.4), ("Mod Wheel", "Resonance", 0.35), ("Aftertouch", "Cutoff", 0.3), ("Velocity", "Amp", 0.2)),
    fx(reverb=(2.6, 5.0, 0.6, 25, 0.3)))

add("Shimmering Rings", "Strings", "Strings in an enormous space: a long glittering tail that keeps ringing after the bow lifts.",
    ["strings", "shimmer", "ambient", "reverb"],
    osc("a", "Classic", morph=0.5, level=0.65, unison=5, detune=0.18, width=0.9), osc("b", "Fraunhofer", morph=0.6, level=0.4, oct=1, unison=3, detune=0.14),
    filt("LP 12", 3600, 0.08, key=0.5), adsr("amp", 0.8, 2.0, 0.9, 3.0),
    lfo(1, "Sine", 5.0, fade=0.8), lfo(2, "Triangle", 0.08, retrig=False),
    mods(("LFO 1", "Pitch", 0.008), ("Mod Wheel", "LFO 1 Amount", 1.0), ("LFO 2", "Morph B", 0.2), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Level B", -0.3), ("Orbit", "Morph B", 0.3), ("Flux", "FX Feedback", 0.3), ("Aftertouch", "Cutoff", 0.25)),
    fx(chorus=(0.25, 0.5, 0.3), delay=("1/4", 0.5, 0.35, 0.15, True), reverb=(3.0, 12.0, 0.25, 40, 0.45), width=1.3))

add("Morphing Magnetosphere", "Strings", "Evolving wavetable strings: a slow sweep through the Pulsar frames turns the section's colour over time.",
    ["strings", "evolving", "wavetable", "movement"],
    osc("a", "Pulsar", morph=0.3, level=0.65, unison=5, detune=0.18, width=0.8), osc("b", "Classic", morph=0.5, level=0.45, unison=3, detune=0.15),
    filt("LP 12", 2800, 0.12, key=0.5), adsr("amp", 0.5, 1.5, 1.0, 1.8),
    lfo(1, "Triangle", 0.06, retrig=False), lfo(2, "Sine", 5.2, fade=0.6),
    mods(("LFO 1", "Morph A", 0.35), ("LFO 2", "Pitch", 0.008), ("Mod Wheel", "LFO 2 Amount", 1.0), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Level B", 0.35), ("Orbit", "LFO 1 Rate", 0.45), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Morph A", 0.25)),
    fx(chorus=(0.3, 0.45, 0.25), reverb=(2.4, 4.5, 0.45, 25, 0.32)))

add("Tape From Voyager", "Strings", "Lo-fi strings off a worn tape: detuned, gently wobbling in pitch, the highs rolled away.",
    ["strings", "lo-fi", "tape", "nostalgic"],
    osc("a", "Classic", morph=0.5, level=0.7, unison=4, detune=0.2, width=0.6), osc("b", "Classic", morph=0.78, level=0.45, fine=-12, unison=2, detune=0.1),
    filt("LP 12", 2000, 0.1, key=0.3), adsr("amp", 0.3, 1.2, 1.0, 1.2),
    lfo(1, "Sine", 0.6, retrig=False), lfo(2, "S&H", 9.0, retrig=False),
    {"drift": 0.7, "noiseLevel": 0.06, "noiseType": "Crackle"},
    mods(("LFO 1", "Pitch", 0.007), ("Drift", "Pitch", 0.006), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.35),
         ("Orbit", "LFO 1 Rate", 0.35), ("Flux", "FX Chorus", 0.4), ("Mod Wheel", "Noise Level", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(drive=("Soft", 0.15, 0.35, 1.0), chorus=(0.3, 0.5, 0.35), reverb=(1.6, 2.4, 0.6, 10, 0.2)))

add("Supersaw Strings of Orion", "Strings", "A wall of eight-voice saws played like a string pad: huge, bright and euphoric.",
    ["strings", "supersaw", "trance", "wide"],
    osc("a", "Classic", morph=0.5, level=0.65, unison=8, detune=0.3, width=1.0), osc("b", "Classic", morph=0.5, level=0.45, oct=-1, unison=4, detune=0.22, width=0.8),
    filt("LP 24", 5200, 0.1, env=6, key=0.4), adsr("amp", 0.2, 1.0, 1.0, 1.0), adsr("fenv", 0.3, 1.0, 0.7, 1.0),
    lfo(1, "Sine", 5.0, fade=0.5),
    mods(("LFO 1", "Pitch", 0.008), ("Mod Wheel", "LFO 1 Amount", 1.0), ("Heat", "Cutoff", 0.35), ("Gravity", "Level B", 0.35),
         ("Orbit", "Detune", 0.3), ("Flux", "FX Delay", 0.4), ("Aftertouch", "Cutoff", 0.25), ("Velocity", "Cutoff", 0.15)),
    fx(delay=("1/8.", 0.35, 0.5, 0.12, True), reverb=(2.4, 3.5, 0.45, 20, 0.28), width=1.3))

add("Heliosphere Bed", "Strings", "An ambient string bed with a very long release: chords hang and overlap as you move between them.",
    ["strings", "ambient", "bed", "long release"],
    osc("a", "Nebula", morph=0.5, level=0.6, unison=4, detune=0.15, width=0.9), osc("b", "Classic", morph=0.5, level=0.45, unison=4, detune=0.2, width=0.9),
    filt("LP 12", 1900, 0.08, key=0.4), adsr("amp", 1.5, 2.0, 1.0, 6.0),
    lfo(1, "Triangle", 0.05, retrig=False), lfo(2, "Sine", 4.8, fade=1.2),
    mods(("LFO 1", "Morph A", 0.25), ("LFO 2", "Pitch", 0.007), ("Mod Wheel", "LFO 2 Amount", 1.0), ("Heat", "Cutoff", 0.35),
         ("Gravity", "Level B", 0.3), ("Orbit", "Morph A", 0.3), ("Flux", "FX Reverb", 0.4), ("Aftertouch", "Cutoff", 0.25)),
    fx(chorus=(0.15, 0.4, 0.25), reverb=(3.0, 9.0, 0.4, 40, 0.4), width=1.25))
