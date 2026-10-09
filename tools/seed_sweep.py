"""Sweep the random seed (tap vectors + modulation) and report Q1/Q2/Q6/Q7 per seed.
Usage: python3 seed_sweep.py [first] [last] [quality]"""
import sys, subprocess, tempfile, os
import numpy as np, soundfile as sf
import ir_analysis as ia

exe = os.path.join(os.path.dirname(__file__), "../build/fr_render")
first, last = int(sys.argv[1]) if len(sys.argv) > 1 else 1, int(sys.argv[2]) if len(sys.argv) > 2 else 20
quality = sys.argv[3] if len(sys.argv) > 3 else "normal"
fs = 48000
for seed in range(first, last + 1):
    with tempfile.TemporaryDirectory() as d:
        o = os.path.join(d, "a.wav")
        subprocess.run([exe, "--impulse", "6", "--out", o, "--seed", str(seed), "--quality", quality, "--t60", "3", "2", "1"], check=True)
        y, _ = sf.read(o, dtype="float64")
    l, r = y[:, 0], y[:, 1]
    t = ia.echo_density_time(l, fs); t2 = ia.echo_density_time(r, fs)
    lvl, notch = ia.mono_check(l, r)
    big = None
    print("seed %2d  Q1 %s/%s ms  Q2 %.1f dB  Q6 %.3f  Q7 %.1f dB / %.1f dB" % (
        seed, "-" if t is None else int(t * 1e3), "-" if t2 is None else int(t2 * 1e3),
        ia.tail_peaks_db(l, fs)[0], ia.iacc(l, r, fs), lvl, notch))
