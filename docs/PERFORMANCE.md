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

# Convolution and hybrid (M4)

Same machine and method as above (best of 5 runs of 2 s, 48 kHz, 128-sample blocks, stereo IR, synthetic decaying noise).

| Case | Budget (PRD 8.1) | Measured | 99.9th percentile block | Memory |
|---|---|---|---|---|
| Convolution, 10 s IR | 6% | 1.2% | 6% of block time | 15.7 MB |
| Convolution, 30 s IR | none (PRD gives 150 MB) | 5.4% | 14% of block time | 3x the 10 s figure (scales with IR length) |
| Hybrid, 3 s IR (first 80 ms convolved + FDN tail) | none | 2.1% | 4% | not measured; the convolver holds only 80 ms of IR |

Design: 64 taps in direct form, then four levels of uniform partitions (64, 256, 1024, 4096 taps; FFT sizes 128 to 8192;
the last level carries all remaining taps). Zero latency: a partition that starts at tap q*P only needs input blocks that
are already complete. The products with older input blocks are spread over the preceding block period, so the
only work done at a block boundary is one forward FFT, one product and one inverse FFT per level.
At the 4096-sample boundary (where every level fires at once) a 128-sample block takes about 10% of its time budget;
A separate spike run (three passes over 8 s of audio) showed no boundary block above 12%. The occasional larger spikes seen in
`fr_bench` have no relation to the boundaries and look like host scheduling on this VM.

IR length is capped at 2M samples per channel after resampling (30 s at 48 kHz, 10 s at 192 kHz), which keeps a four-channel IR near
100 MB. FFT: pffft (BSD-style licence, in `third_party/pffft`), 4-wide SIMD (SSE on x86, NEON on ARM).

## Hybrid mode

The first erMs of the IR (auto: a fifth of the mid-band T60, 20 to 80 ms) is convolved. The FDN tail is fed erMs minus 30 ms
later. Its decay times are measured from the IR (T30 fits at 125 Hz, 1 kHz, 8 kHz), and its level is matched in three bands
(low and high shelves relative to the mid gain) in a short window 60 ms after the early part.
IRs with a mid-band T60 under 0.6 s are convolved completely, because a tail that short is mostly inside the early part and
the FDN cannot follow it well.

Results against the full convolution (`tools/run_m4.py`, studio_hall and large_hall synthetic IRs):
the early part is identical to -140 dB; T60 within 14% at the three anchor frequencies (the large hall is 29% off at 500 Hz, where
its own decay is irregular); tail level within 2.4 dB up to 0.35 s and within 1 dB at 0.5 s; echo density 0.95 within 30 ms after the early part;
narrowband peak 1.8 dB; IACC 0.03.

Limits worth knowing:
- The tail decay is a three-point model. An IR whose decay is not close to exponential, or has a very uneven spectrum, will drift from it.
- The algorithmic engine could not produce reverbs under about 0.3 s before this change, because the input diffusion rang for hundreds
  of milliseconds. Allpass gains now follow the decay time, and T60 down to 0.08 s is accurate. Echo density for such short reverbs builds slower
  than the 80 ms gate (110 to 190 ms), but the reverb is gone by then.
- The two stereo channels of the tail come from the FDN's own decorrelated outputs. A source panned hard left feeds both tail channels.
