#!/usr/bin/env python3
"""Render ULTRA drum voices and draw a waveform + spectrogram sheet for each, with a few measurements.

Usage: voice_report.py RENDER_BIN OUT_DIR [spec.json]
  RENDER_BIN  path to ultra_voice_render
  spec.json   optional list of {"name", "model", "tune", "decay", "tone", "snap", "color", "sweep", "secs", "drive"}
              (default: every model at its default dials)

Writes OUT_DIR/<name>.wav, OUT_DIR/<name>.png and OUT_DIR/report.txt. Needs numpy, scipy and matplotlib.
"""
import json
import os
import subprocess
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from scipy.io import wavfile
from scipy.signal import spectrogram

MODELS = ["Core Kick", "Gravity Kick", "Flare Snare", "Burst Clap", "Ion Hat", "Corona Cymbal", "Orbit Tom",
          "Pulsar FM", "Quasar Metal", "Spark Rim", "Solar Wind", "Plasma Zap", "Nebula Modal"]


def measure(x, fs):
    pk = float(np.max(np.abs(x))) + 1e-12
    env = np.abs(x)
    above = np.nonzero(env > pk * 1e-3)[0]
    length = (above[-1] / fs) if len(above) else 0.0
    spec = np.abs(np.fft.rfft(x * np.hanning(len(x)))) ** 2
    f = np.fft.rfftfreq(len(x), 1 / fs)
    centroid = float(np.sum(f * spec) / (np.sum(spec) + 1e-20))
    # Dominant frequency in successive 10 ms windows over the first 120 ms (kick pitch trajectory).
    traj = []
    for ms in (1, 5, 10, 20, 40, 80, 120):
        a = int(ms * fs / 1000)
        w = x[a:a + 2048]
        if len(w) < 2048 or np.max(np.abs(w)) < pk * 1e-3:
            break
        s = np.abs(np.fft.rfft(w * np.hanning(2048), 16384))
        traj.append((ms, float(np.fft.rfftfreq(16384, 1 / fs)[np.argmax(s)])))
    rms = float(np.sqrt(np.mean(x[: max(1, int(0.05 * fs))] ** 2)))
    return dict(peak_db=20 * np.log10(pk), length=length, centroid=centroid, traj=traj, rms50_db=20 * np.log10(rms + 1e-12))


def sheet(x, fs, title, path):
    fig, ax = plt.subplots(2, 1, figsize=(9, 5.5), gridspec_kw={"height_ratios": [1, 2]})
    t = np.arange(len(x)) / fs
    ax[0].plot(t, x, lw=0.5, color="#5b2bd6")
    ax[0].set_xlim(0, t[-1]); ax[0].set_ylim(-1, 1); ax[0].set_title(title, fontsize=9)
    f, tt, S = spectrogram(x, fs, nperseg=1024, noverlap=896)
    ax[1].pcolormesh(tt, f, 10 * np.log10(S + 1e-14), vmin=-130, vmax=-30, shading="auto", cmap="magma")
    ax[1].set_yscale("symlog", linthresh=200); ax[1].set_ylim(20, fs / 2); ax[1].set_xlim(0, t[-1])
    ax[1].set_ylabel("Hz"); ax[1].set_xlabel("s")
    fig.tight_layout(); fig.savefig(path, dpi=80); plt.close(fig)


def main():
    render, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    if len(sys.argv) > 3:
        specs = json.load(open(sys.argv[3]))
    else:
        specs = [dict(name=f"{m:02d} {MODELS[m]}", model=m) for m in range(len(MODELS))]
    lines = []
    for s in specs:
        d = dict(tune=0, decay=0.5, tone=0.5, snap=0.5, color=0.5, sweep=0.5, vel=1.0, secs=1.5, drive=0.0)
        d.update(s)
        wav = os.path.join(out, d["name"] + ".wav")
        subprocess.run([render, wav, str(d["model"]), *(str(d[k]) for k in ("tune", "decay", "tone", "snap", "color", "sweep", "vel", "secs", "drive"))], check=True)
        fs, x = wavfile.read(wav)
        m = measure(x.astype(np.float64), fs)
        traj = " ".join(f"{ms}ms:{hz:.0f}" for ms, hz in m["traj"])
        lines.append(f"{d['name']:<28} peak {m['peak_db']:6.1f} dB  rms50 {m['rms50_db']:6.1f} dB  len {m['length']:5.2f} s  "
                     f"centroid {m['centroid']:7.0f} Hz  pitch {traj}")
        sheet(x, fs, f"{d['name']}  ({MODELS[d['model']]})", os.path.join(out, d["name"] + ".png"))
    open(os.path.join(out, "report.txt"), "w").write("\n".join(lines) + "\n")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
