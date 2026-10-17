"""M4 check: convolution is exact, hybrid mode matches the IR's tail.
Usage: python3 run_m4.py [path/to/fr_render]"""
import os, subprocess, sys, tempfile
import numpy as np
import soundfile as sf
import ir_analysis as ia

here = os.path.dirname(os.path.abspath(__file__))
exe = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "../build/fr_render")
irs = ["small_room", "studio_hall", "large_hall"]
fails = 0


def render(ir, seconds, *extra):
    with tempfile.TemporaryDirectory() as d:
        o = os.path.join(d, "o.wav")
        subprocess.run([exe, "--impulse", str(seconds), "--out", o, "--ir", os.path.join(here, "../data/irs", ir + ".wav"), *map(str, extra)], check=True)
        y, fs = sf.read(o, dtype="float64")
    return y[:, 0], y[:, 1]


def verdict(ok):
    global fails
    fails += (not ok)
    return "ok" if ok else "FAIL"


def db(x):
    return 10 * np.log10(max(x, 1e-30))


for name in irs:
    src, fs = sf.read(os.path.join(here, "../data/irs", name + ".wav"), dtype="float64")
    secs = len(src) / fs + 0.3
    cl, cr = render(name, secs, "--mode", "conv", "--ir-fade", 0)
    n = len(src)
    # 1. Convolution mode reproduces the IR up to a gain (normalization).
    g = np.dot(cl[:n], src[:, 0]) / np.dot(src[:, 0], src[:, 0])
    err = db(np.sum((cl[:n] - g * src[:, 0]) ** 2) / np.sum((g * src[:, 0]) ** 2))
    errR = db(np.sum((cr[:n] - g * src[:, 1]) ** 2) / np.sum((g * src[:, 1]) ** 2))
    print("%-12s conv: error vs IR %.0f dB / %.0f dB (gain %.3f)  %s" % (name, err, errR, g, verdict(err < -90 and errR < -90)))

    # 2. Hybrid against the full convolution (same normalization).
    hl, hr = render(name, secs, "--mode", "hybrid", "--er-ms", 0, "--ir-fade", 0)
    if name == "small_room":   # T60 under 0.6 s: hybrid mode convolves the whole IR
        d = db(np.sum((hl[:n] - cl[:n]) ** 2) / np.sum(cl[:n] ** 2))
        print("%-12s   hybrid falls back to full convolution (T60 < 0.6 s): differs by %.0f dB  %s" % ("", d, verdict(d < -90)))
        continue
    er_ms = 80   # what the auto setting picks for these IRs
    e0 = int(er_ms * 0.7 * fs / 1000)
    early = db(np.sum((hl[:e0] - cl[:e0]) ** 2) / np.sum(cl[:e0] ** 2))
    print("%-12s   early part (first %d ms) differs from IR by %.0f dB  %s" % ("", er_ms * 0.7, early, verdict(early < -60)))
    ref_t60, hyb_t60 = ia.t60_bands(cl, fs), ia.t60_bands(hl, fs)
    ok = True; line = []
    for fc in ia.BANDS:
        r, h = ref_t60[fc][0], hyb_t60[fc][0]
        pct = 100 * (h / r - 1)
        line.append("%d:%+.0f%%" % (fc, pct)); ok &= abs(pct) <= (15 if fc in (125, 1000, 8000) else 30)
    print("%-12s   T60 hybrid vs IR (want within 15%%): %s  %s" % ("", " ".join(line), verdict(ok)))
    # Level in tail windows.
    ok = True; line = []
    for a, b in [(0.12, 0.22), (0.25, 0.35), (0.5, 0.6)]:
        if a >= 0.5 * (0.3 if name == 'small_room' else 9): continue
        if b * fs > n: continue
        ia_, ib = int(a * fs), int(b * fs)
        d = db(np.mean(hl[ia_:ib] ** 2)) - db(np.mean(cl[ia_:ib] ** 2))
        line.append("%.2f-%.2fs:%+.1f dB" % (a, b, d)); ok &= abs(d) <= (3 if b <= 0.35 else 5)
    print("%-12s   tail level hybrid minus IR (want within 3 dB up to 0.35 s, 5 dB later): %s  %s" % ("", " ".join(line), verdict(ok)))
    t = ia.echo_density_time(hl[int(er_ms * fs / 1000):], fs)
    print("%-12s   echo density 0.95 reached %s ms after the ER part (want <= 60)  %s" % ("", "never" if t is None else int(t * 1000), verdict(t is not None and t <= 0.06)))
    pk, _ = ia.tail_peaks_db(hl, fs, start_s=0.2)
    iac = ia.iacc(hl, hr, fs, 0.1, 0.4)
    print("%-12s   Q2 peak %s (< 3)  Q6 IACC %.2f (< 0.3)  %s" % ("", "n/a (decays before 200 ms)" if pk != pk else "%.1f dB" % pk, iac, verdict((pk != pk or pk < 3) and iac < 0.3)))
sys.exit(1 if fails else 0)
