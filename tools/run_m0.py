"""M0 check: render FDN impulse response and report Q-metrics."""
import numpy as np
import fdn_prototype as fdn, ir_analysis as ia

fs = 48000
T_LOW, T_MID, T_HIGH = 3.0, 2.0, 1.0
n = int(fs * 2.5 * T_LOW)
x = np.zeros(n); x[0] = 1.0
y = fdn.render(x, fs, t60_low=T_LOW, t60_mid=T_MID, t60_high=T_HIGH)
l, r = y[:, 0], y[:, 1]
print("peak %.3f" % np.abs(y).max())

t = ia.echo_density_time(l, fs)
print("Q1 echo density reaches 0.95 at", None if t is None else "%.0f ms" % (t * 1000), "(target <= 80 ms)")
print("Q4/Q5 T60 per octave band (set %.1f s @125 Hz, %.1f s @1 kHz, %.1f s @8 kHz):" % (T_LOW, T_MID, T_HIGH))
for fc, (t60, dev) in ia.t60_bands(l, fs).items():
    want = fdn.t60_target(fc, T_LOW, T_MID, T_HIGH)
    err = 100 * (t60 / want - 1)
    print("  %5d Hz  T60 %.2f s (want %.2f, %+.1f%%)  decay-line deviation %.2f dB  %s"
          % (fc, t60, want, err, dev, "ok" if abs(err) <= 5 and dev <= 1.5 else "FAIL"))
pk, nseg = ia.tail_peaks_db(l, fs)
print("Q2 largest narrowband peak over envelope, 1-8 kHz: %.1f dB over %d segments (target < 3)" % (pk, nseg))
print("Q6 IACC after 100 ms: %.3f (target < 0.3)" % ia.iacc(l, r, fs))
lvl, notch = ia.mono_check(l, r)
print("Q7 mono level change %.1f dB (target within 4), worst 1/3-oct notch %.1f dB (target > -6)" % (lvl, notch))
z = fdn.render(np.zeros(fs), fs)
print("Q8 silent-input peak:", np.abs(z).max())
y2 = fdn.render(x, fs, t60_low=T_LOW, t60_mid=T_MID, t60_high=T_HIGH)
print("Q11 deterministic:", np.array_equal(y, y2))

big = fdn.render(x[:int(fs * 2)], fs, size=3.0, t60_low=T_LOW, t60_mid=T_MID, t60_high=T_HIGH)
t = ia.echo_density_time(big[:, 0], fs)
print("Q1 max size (x3) reaches 0.95 at", None if t is None else "%.0f ms" % (t * 1000), "(target <= 150 ms)")
