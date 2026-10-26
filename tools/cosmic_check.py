"""Behavioural checks for the cosmic recipes, on renders from cs_render.
A 220 Hz tone burst (0.4 s) goes in; the late tail (1.5 to 3 s) is analysed per recipe."""
import os, subprocess, sys, tempfile
import numpy as np, soundfile as sf

exe = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../build/cs_render")
fs = 48000
fails = 0

def render(mode, macros=(0.5, 0.5, 0.5), seconds=4.0):
    with tempfile.TemporaryDirectory() as d:
        src = os.path.join(d, "in.wav"); out = os.path.join(d, "out.wav")
        t = np.arange(int(seconds * fs)) / fs
        x = np.sin(2 * np.pi * 220 * t) * (t < 0.4) * 0.5
        sf.write(src, np.stack([x, x], 1), fs, subtype="FLOAT")
        subprocess.run([exe, "--in", src, "--out", out, "--mode", mode, "--macro", *map(str, macros), "--er", "0.5", "--t60", "4", "3", "1.5", "--size", "1.6"], check=True)
        y, _ = sf.read(out, dtype="float64")
    return y

def band_db(y, f0, a=1.5, b=3.0):
    seg = y[int(a * fs):int(b * fs), 0] * np.hanning(int((b - a) * fs))
    S = np.abs(np.fft.rfft(seg)) ** 2
    f = np.fft.rfftfreq(len(seg), 1 / fs)
    m = (f > f0 / 2 ** (1 / 12)) & (f < f0 * 2 ** (1 / 12))
    return 10 * np.log10(S[m].sum() + 1e-30)

def verdict(ok):
    global fails
    fails += (not ok)
    return "ok" if ok else "FAIL"

base = render("algo")
b220, b440, b880, b110 = [band_db(base, f) for f in (220, 440, 880, 110)]
print("algorithmic reference: 110 Hz %.0f dB, 220 %.0f, 440 %.0f, 880 %.0f" % (b110, b220, b440, b880))

sh = render("shimmer", (0.8, 0.6, 0.7))                   # +12 interval
d440, d880 = band_db(sh, 440) - b440, band_db(sh, 880) - b880
print("shimmer: octave up +%.0f dB, two octaves +%.0f dB  %s" % (d440, d880, verdict(d440 > 10)))

rs = render("redshift", (0.8, 0.9, 0.5))                  # -12
d110 = band_db(rs, 110) - b110
print("redshift: octave down +%.0f dB  %s" % (d110, verdict(d110 > 10)))

co = render("corona", (0.8, 0.0, 0.8))                    # fifth
d330 = band_db(co, 330) - band_db(base, 330)
print("corona: fifth up +%.0f dB  %s" % (d330, verdict(d330 > 10)))

pu = render("pulsar", (0.5, 0.8, 0.0))
env = np.abs(pu[:, 0]); env = np.convolve(env, np.ones(480) / 480, mode="same")[int(0.6 * fs):int(1.6 * fs)]
depth = 20 * np.log10((env.max() + 1e-9) / (env[env > 0].min() + 1e-9))
print("pulsar: gate depth %.0f dB over 1 s  %s" % (depth, verdict(depth > 20)))

eh = render("eventhorizon", (1.0, 0.5, 0.0))
d660 = band_db(eh, 660) - band_db(base, 660)              # third harmonic from the loop saturation
print("event horizon: third harmonic +%.0f dB  %s" % (d660, verdict(d660 > 6)))

for name in ("wormhole", "nebula", "solarwind", "aurora", "supernova"):
    y = render(name)
    tail = 20 * np.log10(np.sqrt(np.mean(y[int(1.5 * fs):int(3 * fs)] ** 2)) + 1e-9)
    print("%-10s late tail %.0f dBFS, peak %.2f  %s" % (name, tail, np.abs(y).max(), verdict(np.isfinite(y).all() and tail > -80 and np.abs(y).max() < 4)))

sn = render("supernova", (0.6, 1.0, 0.0), 3.0)
# Reverse pre-stage: with the burst ending at 0.4 s, the reversed chunk plays the burst backwards later, so
# there is a swell (energy rising) somewhere after the burst.
e = np.convolve(sn[:, 0] ** 2, np.ones(2400) / 2400, mode="same")
seg = e[int(0.5 * fs):int(2.0 * fs)]
rises = np.sum(np.diff(seg[::2400]) > 0)
print("supernova: rising energy windows after the burst: %d  %s" % (rises, verdict(rises >= 2)))
sys.exit(1 if fails else 0)
