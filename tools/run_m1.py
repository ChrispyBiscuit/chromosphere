"""M1 check: render with the C++ renderer and run the PRD Section 7 metrics on the result.
Usage: python3 run_m1.py [path/to/cs_render]
"""
import subprocess, sys, tempfile, os
import numpy as np
import soundfile as sf
import ir_analysis as ia

exe = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "../build/cs_render")
fs = 48000
T = (3.0, 2.0, 1.0)


def render(extra, seconds=7.5):
    with tempfile.TemporaryDirectory() as d:
        out = os.path.join(d, "ir.wav")
        subprocess.run([exe, "--impulse", str(seconds), "--out", out, "--t60", *map(str, T), *extra], check=True)
        y, sr = sf.read(out, dtype="float64")
        assert sr == fs
        return y[:, 0], y[:, 1]


def target(fc):
    rate = np.interp(np.log2(fc), np.log2([125, 1000, 8000]), [60 / t for t in T])
    return 60 / rate


fails = 0
def verdict(ok):
    global fails
    fails += (not ok)
    return "ok" if ok else "FAIL"

l, r = render([])
t = ia.echo_density_time(l, fs)
print("Q1 echo density 0.95 at %s (target <= 80 ms)  %s" % ("never" if t is None else "%.0f ms" % (t * 1e3), verdict(t is not None and t <= 0.08)))
print("Q4/Q5 T60 per octave band (want %s s at 125 Hz / 1 kHz / 8 kHz):" % (T,))
for fc, (t60, dev) in ia.t60_bands(l, fs).items():
    err = 100 * (t60 / target(fc) - 1)
    print("  %5d Hz  T60 %.2f s (want %.2f, %+.1f%%)  deviation %.2f dB  %s" % (fc, t60, target(fc), err, dev, verdict(abs(err) <= 5 and dev <= 1.5)))
pk, n = ia.tail_peaks_db(l, fs)
print("Q2 narrowband peak %.1f dB over %d segments (target < 3)  %s" % (pk, n, verdict(pk < 3)))
iac = ia.iacc(l, r, fs)
print("Q6 IACC %.3f (target < 0.3)  %s" % (iac, verdict(iac < 0.3)))
lvl, notch = ia.mono_check(l, r)
print("Q7 mono level %.1f dB (within 4), worst notch %.1f dB (> -6)  %s" % (lvl, notch, verdict(abs(lvl) <= 4 and notch > -6)))
l2, _ = render(["--tail", "0", "--er", "0"], 1)
print("Q8 silent/zero-gain peak %g  %s" % (np.abs(l2).max(), verdict(np.abs(l2).max() < 1e-6)))
lb, _ = render(["--block", "37"])
print("Block-size independence (512 vs 37):", "identical" if np.array_equal(l, lb) else "DIFFERS", verdict(np.array_equal(l, lb)))
big_l, _ = render(["--size", "3"], 2)
t = ia.echo_density_time(big_l, fs)
print("Q1 max size: 0.95 at %s (target <= 150 ms)  %s" % ("never" if t is None else "%.0f ms" % (t * 1e3), verdict(t is not None and t <= 0.15)))
sys.exit(1 if fails else 0)
