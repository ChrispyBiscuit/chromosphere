"""Objective quality metrics for reverb impulse responses (PRD Section 7)."""
import numpy as np
from scipy.signal import butter, sosfilt
from scipy.special import erfc

BANDS = [125, 250, 500, 1000, 2000, 4000, 8000]


def edc_db(h):
    e = np.cumsum((h ** 2)[::-1])[::-1]
    return 10 * np.log10(e / e[0] + 1e-30)


def t60_fit(h, fs, lo=-5.0, hi=-35.0):
    """T30 extrapolated to 60 dB, plus max deviation from a straight line (Q5)."""
    e = edc_db(h)
    i0 = np.argmax(e <= lo); i1 = np.argmax(e <= hi)
    t = np.arange(i0, i1) / fs
    slope, icpt = np.polyfit(t, e[i0:i1], 1)
    dev = np.max(np.abs(e[i0:i1] - (slope * t + icpt)))
    return -60.0 / slope, dev


def octave_band(h, fs, fc):
    sos = butter(4, [fc / 2 ** 0.5, min(fc * 2 ** 0.5, fs * 0.45)], "bandpass", fs=fs, output="sos")
    return sosfilt(sos, h)


def t60_bands(h, fs):
    return {fc: t60_fit(octave_band(h, fs, fc), fs) for fc in BANDS}


def echo_density(h, fs, win_ms=20.0):
    """Abel-Huang normalized echo density per window; returns (times, density)."""
    w = int(win_ms * fs / 1000)
    norm = erfc(1 / np.sqrt(2))
    ts, ds = [], []
    for s in range(0, len(h) - w, w // 2):
        seg = h[s:s + w]
        sd = seg.std()
        ts.append((s + w / 2) / fs)
        ds.append(0.0 if sd == 0 else np.mean(np.abs(seg) > sd) / norm)
    return np.array(ts), np.array(ds)


def echo_density_time(h, fs, target=1.0, tol=0.05):
    """First time the density stays within tol of target (Q1)."""
    t, d = echo_density(h, fs)
    ok = np.abs(d - target) <= tol
    for i in range(len(ok)):
        if ok[i:i + 3].all():
            return t[i]
    return None


def iacc(l, r, fs, start_s=0.1, dur_s=0.5):
    """Max normalized cross-correlation within +-1 ms (Q6)."""
    a = int(start_s * fs); b = a + int(dur_s * fs)
    l, r = l[a:b], r[a:b]
    m = int(0.001 * fs)
    den = np.sqrt((l ** 2).sum() * (r ** 2).sum()) + 1e-30
    best = 0.0
    for lag in range(-m, m + 1):
        v = np.dot(l[max(0, lag):len(l) + min(0, lag)], r[max(0, -lag):len(r) - max(0, lag)])
        best = max(best, abs(v) / den)
    return best


def mono_check(l, r, start_s=0.1):
    """Mono level change vs stereo RMS (dB) and worst 1/3-oct notch depth (dB)."""
    a = int(start_s * 48000)
    l, r = l[a:], r[a:]
    rms_st = np.sqrt((l ** 2 + r ** 2).mean() / 2)
    rms_mono = np.sqrt((((l + r) / 2) ** 2).mean())
    lvl = 20 * np.log10(rms_mono / rms_st)
    spec = lambda x: np.abs(np.fft.rfft(x * np.hanning(len(x)))) ** 2
    f = np.fft.rfftfreq(len(l), 1 / 48000)
    ps, pm = spec(l) + spec(r), spec((l + r) / 2) * 2
    worst = 0.0
    for fc in 1000 * 2 ** (np.arange(-9, 12) / 3):
        if fc > 16000:
            break
        m = (f >= fc / 2 ** (1 / 6)) & (f < fc * 2 ** (1 / 6))
        if m.any():
            worst = min(worst, 10 * np.log10(pm[m].sum() / ps[m].sum()))
    return lvl, worst
