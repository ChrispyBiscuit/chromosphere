# Performance (M3)

Measured on a shared 4-core Xeon 2.1 GHz VM (AVX2 and AVX-512), GCC 13, Release build, 48 kHz unless noted.
Load is the best of 5 runs of 2 s each, as a percentage of one core at real time. Numbers on a VM vary by about 10%.

| Case | Budget (PRD 8.1) | Before M3 | After M3 |
|---|---|---|---|
| Normal, 128 samples | 2.0% | 2.0% | 1.06% |
| High, 128 samples | 5.0% | 3.9% | 2.18% |
| Eco, 64 samples | 1.0% | 1.1% | 0.66% |
| Normal, 96 kHz, 128 samples | 4.4% (2.2x) | 4.0% | 2.32% (2.2x of 48 kHz) |

Worst single block: rare spikes up to several times the median appear in every run on this VM,
including cases that run in a few microseconds. They look like host scheduling, but this is not proven.
The 99.9th-percentile block stays below 36% of the block time. The 25% worst-block gate needs a quiet machine to confirm.

## What changed

1. Delay reads: cubic weights and indices computed in a vector loop, then a gather loop. Each line is stored twice
   in its ring (at `i` and `i + length`), so a 4-point read never wraps.
2. Modulation noise: uniform white noise from a per-line xorshift (was Gaussian with log/cos).
3. Input diffusion: one state buffer per allpass instead of two.
4. Runtime CPU dispatch (`dsp/common/simd.h`): the FDN and ER hot loops are built for AVX-512, AVX2+FMA and baseline
   and picked at load time (GCC/Clang on x86-64 Linux). Reductions use `omp simd`.
5. `tools/perf_check.py`: counts instructions under callgrind (deterministic, so it works on noisy hosts) and fails on
   a regression above 5% against `tools/perf_baseline.json`. Run with `--update` after an intended change.

Not done yet: MSVC dispatch (Windows needs per-file `/arch` flags), NEON-specific code (the compiler vectorizes the
baseline on Apple Silicon, not measured), and the ER stage (about 11% of the time) is still scalar.

## Quality versus cost (PRD 8.3 #11), seed 9, T60 3/2/1 s

| Mode | Lines | ER taps | Instructions per second of audio | Q1 echo density | Q2 peak | Q6 IACC | Q7 notch |
|---|---|---|---|---|---|---|---|
| Eco | 16 | 16 | 79 M | 70 / 80 ms | 2.5 dB | 0.029 | -3.7 dB |
| Normal | 32 | 32 | 105 M | 60 / 60 ms | 2.1 dB | 0.024 | -3.4 dB |
| High | 64 | 64 | 157 M | 50 / 70 ms | 1.7 dB | 0.038 | -3.8 dB |

Eco is only 25% cheaper than Normal because the diffusion chains and the ER stage cost the same in both.
High is 1.5x Normal, so line count is not the cost driver any more. Most gates pass in all three modes.
Eco has the thinnest margin on Q1 (80 ms against an 80 ms limit on one channel) and Q2.

## Seed choice

Q1, Q2 and the mono notch depend on the random tap vectors and modulation. Across seeds 1 to 16 about one third have
one marginal channel (Q1 up to 100 ms, Q2 up to 3.2 dB, notch down to -6.1 dB). `tools/seed_sweep.py` lists them.
Seed 9 is the default. Re-run the sweep if the DSP changes.
