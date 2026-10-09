"""M0 check: render FDN impulse response and report Q-metrics."""
import numpy as np
import fdn_prototype as fdn, ir_analysis as ia

fs = 48000
T_LOW, T_HIGH = 2.5, 1.2
n = int(fs * 2.5 * T_LOW)
x = np.zeros(n); x[0] = 1.0
y = fdn.render(x, fs, t60_low=T_LOW, t60_high=T_HIGH)
l, r = y[:, 0], y[:, 1]
print("peak %.3f" % np.abs(y).max())

t = ia.echo_density_time(l, fs)
print("Q1 echo density reaches 1.0 at", None if t is None else "%.0f ms" % (t * 1000), "(target <= 80 ms)")
print("Q4/Q5 T60 per band (set %.1f s low, %.1f s high):" % (T_LOW, T_HIGH))
for fc, (t60, dev) in ia.t60_bands(l, fs).items():
    print("  %5d Hz  T60 %.2f s  decay-line deviation %.2f dB" % (fc, t60, dev))
print("Q6 IACC after 100 ms: %.3f (target < 0.3)" % ia.iacc(l, r, fs))
lvl, notch = ia.mono_check(l, r)
print("Q7 mono level change %.1f dB (target within 3), worst 1/3-oct notch %.1f dB (target > -6)" % (lvl, notch))
z = fdn.render(np.zeros(fs), fs)
print("Q8 silent-input peak:", np.abs(z).max())
y2 = fdn.render(x, fs, t60_low=T_LOW, t60_high=T_HIGH)
print("Q11 deterministic:", np.array_equal(y, y2))
