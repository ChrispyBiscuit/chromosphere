"""Python prototype of the FDN late tail (M0). Offline, not shipped in the plugin.

Signal path: input -> allpass diffusion -> FDN (Householder matrix,
per-line first-order decay filter, slow random delay modulation) -> stereo taps.
"""
import numpy as np
from scipy.signal import lfilter, freqz
from scipy.optimize import least_squares

ANCHORS = (125.0, 1000.0, 8000.0)   # low, mid, high T60 anchor frequencies (Hz)
FIT_FREQS = 125.0 * 2.0 ** np.arange(0, 7.01, 0.5)  # half-octave grid 125 Hz .. 16 kHz


def primes_near(targets):
    """Return distinct primes, one near each target (in samples)."""
    def is_prime(n):
        if n < 2:
            return False
        for k in range(2, int(n ** 0.5) + 1):
            if n % k == 0:
                return False
        return True
    out, used = [], set()
    for t in targets:
        n = int(round(t))
        while not is_prime(n) or n in used:
            n += 1
        used.add(n)
        out.append(n)
    return np.array(out)


def allpass(x, delay, g):
    """Schroeder allpass: (-g + z^-d) / (1 - g z^-d)."""
    b = np.zeros(delay + 1); a = np.zeros(delay + 1)
    b[0], b[-1] = -g, 1.0
    a[0], a[-1] = 1.0, -g
    return lfilter(b, a, x)


def t60_target(f, t60_low, t60_mid, t60_high):
    """T60 at frequency f: decay rate (dB/s) interpolated linearly in log-frequency."""
    rate = 60.0 / np.array([t60_low, t60_mid, t60_high])
    return 60.0 / np.interp(np.log2(f), np.log2(ANCHORS), rate)


SHELVES = (("lo", 200.0), ("lo", 700.0), ("hi", 1500.0), ("hi", 5000.0))


def _shelf(kind, gain, fc, fs):
    """First-order shelf by bilinear transform. 'lo': gain at DC, 1 at Nyquist.
    'hi': 1 at DC, gain at Nyquist. Returns normalized (b0, b1, a1)."""
    k = 1 / np.tan(np.pi * fc / fs)
    if kind == "lo":
        b0, b1 = k + gain, gain - k
    else:
        b0, b1 = gain * k + 1, 1 - gain * k
    return b0 / (k + 1), b1 / (k + 1), (1 - k) / (k + 1)


def design_decay_filter(delay, t60s, fs):
    """Fit a gain plus four first-order shelves so the per-pass loss of a line with
    `delay` samples gives the target T60 on a half-octave grid, 125 Hz .. 16 kHz.
    Returns (gain, sections) with sections an array of (b0, b1, a1)."""
    want_db = -60.0 * delay / (fs * t60_target(FIT_FREQS, *t60s))
    w = 2 * np.pi * FIT_FREQS / fs
    z1 = np.exp(-1j * w)

    def sections(p):
        return np.array([_shelf(kind, 10 ** (g / 20), fc, fs)
                         for (kind, fc), g in zip(SHELVES, p[1:])])

    def model(p):
        h = np.ones(len(w), dtype=complex)
        for b0, b1, a1 in sections(p):
            h = h * (b0 + b1 * z1) / (1 + a1 * z1)
        return p[0] + 20 * np.log10(np.abs(h))

    p0 = np.concatenate([[want_db[len(want_db) // 2]], np.zeros(len(SHELVES))])
    p = least_squares(lambda p: model(p) - want_db, p0).x
    return 10 ** (p[0] / 20), sections(p)


def render(x, fs=48000, lines=32, size=1.0, t60_low=2.0, t60_mid=1.6, t60_high=1.0,
           mod_depth=1.0, mod_rate_hz=0.7, seed=1,
           min_ms=10.0, max_ms=55.0, diffusion=True,
           ap_ms=(1.9, 2.9, 4.3, 6.1, 8.3, 11.3, 14.9, 19.7)):
    """x: mono (n,) -> stereo (n, 2). Deterministic for a given seed."""
    n_in = len(x)
    if diffusion:
        for d_ms in ap_ms:
            x = allpass(x, int(d_ms * fs / 1000), 0.6)
    d = primes_near(np.linspace(min_ms, max_ms, lines) * fs / 1000 * size)
    N = lines
    # Per-line decay filter: gain followed by four first-order shelves.
    dec = [design_decay_filter(di, (t60_low, t60_mid, t60_high), fs) for di in d]
    g0 = np.array([q[0] for q in dec])
    sec = np.array([q[1] for q in dec])            # (N, shelves, 3): b0, b1, a1
    st = np.zeros((N, len(SHELVES)))
    # Slow smoothed noise modulation, one stream per line.
    noise = np.stack([np.random.default_rng([seed, i]).standard_normal(n_in)
                      for i in range(N)]).astype(np.float32)
    k = np.exp(-2 * np.pi * mod_rate_hz / fs)
    mod = lfilter([1 - k], [1, -k], noise, axis=1)
    mod *= mod_depth / (mod.std() + 1e-12)
    M = int(d.max() + mod_depth * 6 + 16)
    buf = np.zeros((N, M))
    idx = np.arange(N)
    c = np.where(np.arange(N) % 2 == 0, 1.0, -1.0) / np.sqrt(N)
    # Random orthogonal output taps (left and right are uncorrelated by construction).
    trng = np.random.default_rng(seed + 1000)   # taps independent of signal length
    tl = trng.standard_normal(N)
    tr = trng.standard_normal(N)
    tr -= tl * (tl @ tr) / (tl @ tl)
    tap_l = tl / np.linalg.norm(tl)
    tap_r = tr / np.linalg.norm(tr)
    out = np.zeros((n_in, 2))
    for n in range(n_in):
        pos = n - (d + mod[:, n])
        i0 = np.floor(pos).astype(np.int64)
        fr = pos - i0
        # 4-point cubic Lagrange read (Normal-mode interpolation, PRD 5.9).
        ym1, y0, y1, y2 = (buf[idx, (i0 + k) % M] for k in (-1, 0, 1, 2))
        y = (y0 + fr * (0.5 * (y1 - ym1) + fr * ((ym1 - 2.5 * y0 + 2 * y1 - 0.5 * y2)
             + fr * 0.5 * (y2 - ym1 + 3 * (y0 - y1)))))
        out[n, 0] = tap_l @ y
        out[n, 1] = tap_r @ y
        z = g0 * y
        for k in range(len(SHELVES)):
            o = sec[:, k, 0] * z + st[:, k]
            st[:, k] = sec[:, k, 1] * z - sec[:, k, 2] * o
            z = o
        buf[:, n % M] = z - (2.0 / N) * z.sum() + c * x[n]
    return out
