"""Synthesize the factory impulse responses (pyroomacoustics: image sources + ray tracing).
Output: data/irs/*.wav, 48 kHz, 24-bit PCM, stereo (two omni microphones 0.2 m apart).
Usage: python3 tools/make_factory_irs.py   (takes a minute)"""
import os
import numpy as np
import pyroomacoustics as pra
import soundfile as sf
from scipy.signal import butter, sosfilt, sosfiltfilt

FS = 48000
OCT = [125, 250, 500, 1000, 2000, 4000, 8000]
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "irs")
os.makedirs(OUT, exist_ok=True)

ROOMS = {
    # name: (dimensions m, energy absorption per octave band 125..8k, source, listener centre, length s)
    "small_room": ([5.0, 4.0, 2.8], [0.25, 0.28, 0.32, 0.38, 0.45, 0.50, 0.55], [1.5, 1.2, 1.5], [3.4, 2.6, 1.5], 1.6),
    "studio_hall": ([14.0, 10.0, 6.0], [0.18, 0.15, 0.13, 0.12, 0.15, 0.22, 0.35], [3.0, 5.0, 1.8], [9.0, 5.5, 1.6], 3.2),
    "large_hall": ([34.0, 22.0, 14.0], [0.20, 0.17, 0.15, 0.15, 0.17, 0.22, 0.35], [6.0, 11.0, 2.0], [20.0, 11.0, 1.8], 5.0),
}


def diffuse_tail(ir, fs, t_start):
    """Replace everything after t_start with band-wise Gaussian noise that follows the measured energy
    envelope of each octave band and channel. Ray tracing leaves the late field sparse; this makes it dense
    and decorrelated between the two channels, as a real diffuse field is."""
    n, nch = ir.shape
    i0 = int(t_start * fs)
    fade = int(0.02 * fs)
    rng = np.random.default_rng(1234)
    out = ir.copy()
    win = int(0.01 * fs)
    for c in range(nch):
        tail = np.zeros(n)
        for fc in OCT:
            hi = min(fc * 2 ** 0.5, fs * 0.45)
            sos = butter(4, [fc / 2 ** 0.5, hi], "bandpass", fs=fs, output="sos")
            band = sosfilt(sos, ir[:, c])
            e = np.convolve(band ** 2, np.ones(win) / win, mode="same")          # smoothed energy per sample
            e = np.maximum.accumulate(e[::-1])[::-1] if False else e
            noise = sosfilt(sos, rng.standard_normal(n))
            noise /= np.sqrt(np.mean(sosfilt(sos, rng.standard_normal(200000)) ** 2))
            tail += noise * np.sqrt(e)
        w = np.zeros(n); w[i0:] = 1.0
        w[i0:i0 + fade] = 0.5 - 0.5 * np.cos(np.pi * np.arange(fade) / fade)
        out[:, c] = ir[:, c] * (1 - w) + tail * w
    return out


for name, (dim, absorb, src, lis, length) in ROOMS.items():
    mat = pra.Material(energy_absorption={"coeffs": absorb, "center_freqs": OCT})
    room = pra.ShoeBox(dim, fs=FS, materials=mat, max_order=6, ray_tracing=True, air_absorption=True)
    room.set_ray_tracing(n_rays=60000, receiver_radius=0.1, energy_thres=1e-6, time_thres=length)
    room.add_source(src)
    mics = np.array([[lis[0], lis[1] - 0.1, lis[2]], [lis[0], lis[1] + 0.1, lis[2]]]).T
    room.add_microphone_array(pra.MicrophoneArray(mics, FS))
    room.compute_rir()
    n = int(length * FS)
    ir = np.zeros((n, 2))
    for c in range(2):
        r = np.asarray(room.rir[c][0])[:n]
        ir[:len(r), c] = r
    ir = diffuse_tail(ir, FS, 1.2 * np.sqrt(np.prod(dim)) / 1000.0 + 0.02)
    ir /= np.abs(ir).max() * 1.05
    path = os.path.join(OUT, name + ".wav")
    sf.write(path, ir, FS, subtype="PCM_24")
    print(name, dim, "%.1f s" % length, "->", path)
