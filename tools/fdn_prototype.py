"""Python prototype of the FDN late tail (M0). Offline, not shipped in the plugin.

Signal path: input -> allpass diffusion -> FDN (Householder matrix,
per-line first-order decay filter, slow random delay modulation) -> stereo taps.
"""
import numpy as np
from scipy.signal import lfilter


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


def render(x, fs=48000, lines=32, size=1.0, t60_low=2.0, t60_high=1.0,
           xover_hz=3000.0, mod_depth=1.0, mod_rate_hz=0.7, seed=1,
           min_ms=15.0, max_ms=60.0, diffusion=True):
    """x: mono (n,) -> stereo (n, 2). Deterministic for a given seed."""
    rng = np.random.default_rng(seed)
    n_in = len(x)
    if diffusion:
        for d_ms in (4.7, 7.3, 11.1, 15.9):
            x = allpass(x, int(d_ms * fs / 1000), 0.6)
    d = primes_near(np.linspace(min_ms, max_ms, lines) * fs / 1000 * size)
    N = lines
    # Decay filter: H(z)=(b0+b1 z^-1)/(1+a1 z^-1), gain Gdc at DC, Gny at Nyquist.
    gdc = 10 ** (-3 * d / (fs * t60_low))
    gny = 10 ** (-3 * d / (fs * t60_high))
    a1 = -np.exp(-2 * np.pi * xover_hz / fs)
    b0 = 0.5 * (gdc * (1 + a1) + gny * (1 - a1))
    b1 = 0.5 * (gdc * (1 + a1) - gny * (1 - a1))
    # Slow smoothed noise modulation, one stream per line.
    noise = rng.standard_normal((N, n_in)).astype(np.float32)
    k = np.exp(-2 * np.pi * mod_rate_hz / fs)
    mod = lfilter([1 - k], [1, -k], noise, axis=1)
    mod *= mod_depth / (mod.std() + 1e-12)
    M = int(d.max() + mod_depth * 6 + 8)
    buf = np.zeros((N, M)); s = np.zeros(N)
    idx = np.arange(N)
    c = np.where(np.arange(N) % 2 == 0, 1.0, -1.0) / np.sqrt(N)
    tap_l = np.where((np.arange(N) // 1) % 2 == 0, 1.0, -1.0)
    tap_r = np.where((np.arange(N) // 2) % 2 == 0, 1.0, -1.0)
    tap_l, tap_r = tap_l / np.sqrt(N), tap_r / np.sqrt(N)
    out = np.zeros((n_in, 2))
    for n in range(n_in):
        pos = n - (d + mod[:, n])
        i0 = np.floor(pos).astype(np.int64)
        fr = pos - i0
        y = buf[idx, i0 % M] * (1 - fr) + buf[idx, (i0 + 1) % M] * fr
        out[n, 0] = tap_l @ y
        out[n, 1] = tap_r @ y
        z = b0 * y + s
        s = b1 * y - a1 * z
        buf[:, n % M] = z - (2.0 / N) * z.sum() + c * x[n]
    return out
