# PRD: High-Quality Real-Time Reverb Plugin (VST3 / AAX / AU)

Status: Draft 2 (personal project; no business, telemetry, or release-process goals)
Working name: TBD ("the plugin")
Date: 2026-10-09

---

## 1. Summary

Build a studio-grade reverb plugin for music production and post production. The plugin ships as VST3, AAX Native, and AU (CLAP optional). Sound quality is the first goal. The plugin must run in real time on a CPU, at low load, with zero reported latency in algorithmic mode.

The design combines three engines in one product:
1. **Algorithmic engine.** A modulated feedback delay network (FDN) with per-band decay and a dense early-reflection stage.
2. **Convolution engine.** Zero-latency partitioned convolution for measured or synthesized impulse responses (IRs).
3. **Hybrid mode.** Early reflections from an IR or a room model, plus an algorithmic tail matched to it.

Offline tools (Python, pyroomacoustics) create the room models, IRs, and test data. No Python and no GPU code runs inside the plugin.

## 2. Problem and opportunity

Most reverb plugins sound cheap and synthetic. Common causes: low echo density at onset, metallic ringing, one decay time for all frequencies, static tails, weak early reflections, poor stereo decorrelation, and DSP shortcuts that add aliasing. Convolution reverbs avoid some faults but lack control, and long IRs cost memory and CPU.

Opportunity: an engine that is measured against objective quality targets (Section 7), tuned by ear, and optimized for low CPU use so users can run many instances.

## 3. Goals and non-goals

### Goals
- G1. Natural, colorless, dense reverb that passes the quality gates in Section 7.
- G2. Real-time safe: no allocation, locks, or file access on the audio thread.
- G3. Low CPU load (budgets in Section 8).
- G4. Zero reported latency in algorithmic and zero-latency convolution modes.
- G5. Runs in major DAWs on Windows and macOS (Intel and Apple Silicon), including Pro Tools via AAX.
- G6. Deterministic, sample-accurate, automation-safe behavior.
- G7. Clear controls that a user can understand without reverb theory.

### Non-goals (v1)
- GPU processing.
- AAX DSP (Avid HDX hardware).
- Python or machine learning inference in the plugin.
- Linux packaging for VST3 (possible later; code stays portable).
- Surround beyond stereo (see Section 5.8 for v1.1 plan).
- Cloud features, accounts, or telemetry.

## 4. Users

| User | Needs |
|---|---|
| Mix engineer | Natural rooms and plates, fast recall, low CPU on large sessions |
| Music producer | Big, creative spaces, easy presets, tempo-synced pre-delay |
| Post / film engineer | Matched rooms, IR loading, mono compatibility, Pro Tools support |
| Sound designer | Extreme sizes, freeze, modulation, long tails |

## 5. Features

### 5.1 Algorithmic engine (primary)
Signal path: input → pre-delay → input filters → early reflections → input diffusion → FDN late tail → output decorrelation → mix.

**Early reflections (ER)**
- 16 to 64 taps, per tap: delay, gain, 3-band filter, pan.
- Tap tables derived from room models (image source method, offline) for shoebox rooms, plus hand-tuned tables for plate, hall, chamber, and spring styles.
- Controls: room size, shape (aspect), source and listener position (X/Y), ER level, ER-to-tail crossfade time.
- Position changes do not click (see 8.5).

**Late tail (FDN)**
- Lines: 16 (Eco), 32 (Normal), 64 (High). Prime or mutually co-prime delay lengths, scaled by the size control.
- Feedback matrix: Householder or Hadamard (fast, O(N log N)) with an optional random orthogonal blend to reduce patterns.
- Per-line decay filters set from a target T60 curve, with at least 3 bands (low, mid, high) and crossover frequencies. High-quality mode uses a graphic response of 8 to 10 bands.
- Modulation: slow random (smoothed noise) delay modulation with decorrelated phases, depth in samples, rate in Hz. Interpolation: cubic Lagrange or allpass, chosen by quality mode.
- Input diffusion: 4 to 8 allpass stages before the FDN to raise early echo density.
- Output taps from different lines to left and right, with a decorrelation matrix. Mono sum must not comb-filter.
- Damping, density, and diffusion controls map to internal values (Section 5.5).

**Freeze (infinite hold)**
- Loop gain is capped at 0.99999 per pass (the tail lasts for hours and cannot grow). The input is always muted while frozen, with a smoothed fade (an open input would build the level up without limit, so there is no option to leave it on). Fade-in and fade-out are 50 ms. In Convolution mode freeze has no effect. The NaN/energy watchdog (Section 13) is not built yet.

### 5.2 Convolution engine
- Load WAV/AIFF/FLAC IRs, mono, stereo, true stereo (4-channel). Length cap: 2M samples per channel after resampling to the host rate (30 s at 48 kHz, 20 s at 96 kHz, 10 s at 192 kHz). Longer IRs get a fade-out and truncation. This keeps memory under the 150 MB budget (Section 8.1).
- Zero-latency partitioned FFT convolution: a short direct-form head, then uniform and then non-uniform partitions. All convolution work runs on the audio thread of the instance (single thread; Pro Tools runs one thread per channel, so extra threads give no gain and add risk).
- IR tools: trim, fade, reverse, stretch (time scale), pre-delay, low/high cut, decay-shaping EQ, normalize, stereo width.
- Sample-rate conversion on load (offline thread, high-quality polyphase). Never on the audio thread.
- IR browser with factory IR pack (original captures or synthesized from room models; licensing in Section 14).

### 5.3 Hybrid mode
- ER taken from the first N ms of an IR (or from the room model), fused into a smooth transition to the FDN tail.
- Tail analysis tool (offline, in app or script) estimates T60 per band and echo density from an IR and sets the FDN to match.
- Gives the user convolution realism for placement plus algorithmic flexibility for tail length and modulation.

### 5.4 Shared controls
Pre-delay (ms, or tempo-synced), size, decay time (T60), low/mid/high decay multipliers, crossovers, damping, diffusion, density, modulation depth and rate, ER level, tail level, width, input and output EQ (low cut, high cut, shelves), dry/wet, output gain, ducking (sidechain-free ducking from input level), freeze.

### 5.5 Parameter design rules
- Few primary controls on the front panel; advanced controls in a second panel.
- Every control maps to a measurable acoustic effect (decay time, density, etc.), not to an arbitrary internal gain.
- All parameters smoothed (Section 8.5), automatable, and saved in state.

### 5.6 Presets
- 100+ factory presets (rooms, halls, plates, chambers, springs, spaces, special effects).
- User presets, A/B compare, undo/redo, preset browsing with tags.
- State is versioned; old sessions load in new versions without sound changes.

### 5.7 UI
- Resizable, HiDPI, vector graphics.
- Real-time display: decay curve (T60 vs frequency), time-domain envelope, ER taps view.
- Meter: input, output, tail level.
- Keyboard and screen-reader accessible controls where host allows.
- UI never blocks audio; UI reads from lock-free queues.

### 5.8 Channel formats
- v1: mono-in/stereo-out, stereo-in/stereo-out.
- v1.1: LCR, 5.1, 7.1, and 7.1.2 (Atmos bed) with per-channel FDN output taps and a decorrelation matrix. Design the DSP graph for N channels from the start.

### 5.9 Quality modes
| Mode | FDN lines | ER taps | Interpolation | Intended use |
|---|---|---|---|---|
| Eco | 16 | 16 | Linear/Lagrange-3 | Many instances, tracking |
| Normal | 32 | 32 | Lagrange-3 | Default |
| High | 64 | 64 | Lagrange-5 / allpass | Final mix, bounce |

Offline and live output are the same by default (needed for Pro Tools AudioSuite match, Section 10). An optional "render quality" setting, off by default, lets offline bounce use High. When it is on, offline output differs from live.

## 6. Architecture

### 6.1 Stack
- C++20.
- JUCE 8 for plugin wrappers (VST3, AU, AAX) and UI. Avid AAX SDK for AAX.
- SIMD: runtime dispatch for SSE4.2, AVX2, AVX-512 (optional), and ARM NEON. Use xsimd or Google Highway.
- FFT: pffft, KFR, or vendor libraries (Accelerate on macOS, IPP or MKL on Windows). Choose by benchmark.
- Build: CMake, CI on Windows, macOS (x64 + arm64).
- Offline tools: Python (pyroomacoustics, numpy, scipy) for room models, IR generation, and analysis. Output data files are checked into the repo.

### 6.2 Module layout
```
/dsp        core engine (no JUCE dependency)
  fdn/      delay lines, matrices, decay filters, modulation
  er/       early-reflection tap engine
  conv/     partitioned convolution
  common/   smoothers, filters, SIMD helpers, denormal control
/plugin     JUCE wrappers, parameters, state, presets
/ui         editor, displays
/tools      Python: room models, IR synthesis, analysis, test harness
/tests      unit, regression, perf, plugin validation
```
The `dsp` library has no host or UI dependency, so it can be tested and benchmarked on its own.

### 6.3 Threads
- Audio thread: only DSP. No allocation, locks, system calls, or logging.
- Message/UI thread: UI, preset browsing.
- Worker thread: IR load, resample, FFT planning, convolution partition prep. Results move to the audio thread by atomic pointer swap with deferred free. The worker never runs during signal processing of the reverb itself.
- Each instance has a single-threaded audio path. Hosts can run many instances on many cores; the plugin does not need to create its own audio threads.

## 7. Quality requirements (objective gates)

Release is blocked until all of these pass on the reference test suite.

| ID | Metric | Target |
|---|---|---|
| Q1 | Normalized echo density (Abel-Huang) | Reaches 1.0 within 80 ms at default size; within 150 ms at max size |
| Q2 | Late-tail spectral coloration | No narrowband peak above 3 dB over the smoothed envelope in 1 to 8 kHz after 200 ms |
| Q3 | Modal ringing | No audible ringing on pitched test signals (sine bursts, plucked tones) at any setting; verified by spectrogram review and ABX |
| Q4 | T60 accuracy | Within ±5% of the set value per band (low, mid, high), 125 Hz to 8 kHz |
| Q5 | Decay shape | Log-decay linear within ±1.5 dB (Schroeder integral) over 30 dB range |
| Q6 | Stereo tail decorrelation | Interaural cross-correlation (IACC) < 0.3 after 100 ms at default width |
| Q7 | Mono compatibility | Mono sum level change within 4 dB of stereo RMS (uncorrelated channels lose 3 dB by nature); no deep notches (> 6 dB) in the mono-summed tail spectrum |
| Q8 | Noise floor | < -120 dBFS with silent input, 32-bit float |
| Q9 | Interpolation images | Image and aliasing products from modulated delay interpolation < -100 dB for full-scale input sweeps |
| Q10 | Parameter changes | No clicks or zipper noise; peak transient < -60 dB relative to signal during sweeps |
| Q11 | Determinism | Same input and parameters produce bit-identical output on repeated offline renders on the same machine, with the same SIMD path and a fixed modulation seed |
| Q12 | Listening test | Your own blind ABX against 2 to 3 reference reverbs you choose: at or above them on naturalness and smoothness, and you hear no "metallic" or "grainy" artifacts |

Test conditions: run Q1 to Q10 on a fixed matrix: 3 sizes (min, default, max) × 3 quality modes × 3 decay settings, plus 5 factory presets. Smoothing for Q2 and Q7 is 1/3 octave. T60 for Q4 and Q5 is measured by T30 extrapolation.

Tools: Python analysis scripts in `/tools/analysis`, run in CI on rendered impulse and sweep files.

## 8. Performance and real-time requirements

### 8.1 Budgets (targets, to validate on the reference machines)
Reference machines: Apple M1 (base), Intel/AMD x64 with AVX2 (mid-range laptop class from the last 5 years).

| Scenario | Target |
|---|---|
| Stereo, Normal, 48 kHz, 128-sample buffer | ≤ 2% of one core |
| Stereo, High, 48 kHz, 128-sample buffer | ≤ 5% of one core |
| Stereo, Eco, 48 kHz, 64-sample buffer | ≤ 1% of one core |
| Convolution, 10 s stereo IR, 48 kHz, 128-sample buffer | ≤ 6% of one core |
| 96 kHz and 192 kHz | Scale linearly; no more than 2.2x and 4.5x of 48 kHz load |
| Worst-case block time | < 25% of the buffer duration at any block (no spikes) |
| Memory per instance | Algorithmic < 50 MB; convolution < 150 MB with a maximum-length true-stereo IR |
| Instance count | 20 instances of Normal in one host session without dropouts at 256-sample buffer (host spreads them over cores; each instance stays within the single-core budget above) |

### 8.2 Real-time safety rules
- No heap allocation, mutexes, file or network I/O, or exceptions on the audio thread.
- Denormals: set flush-to-zero/denormals-are-zero flags per thread and add tiny DC offsets or noise in feedback paths where needed. Test with silence after loud input.
- No dependency on the host buffer size. Support variable and odd sizes (1 to 8192 samples), including changing sizes during playback.
- Handle sample-rate changes and `prepareToPlay` repeated calls safely.
- Bypass and suspend: tail continues or stops in a defined way; no clicks.

### 8.3 Optimization plan
1. **Structure for SIMD.** Store FDN delay lines and filter state in structure-of-arrays layout. 32 lines = 4 AVX2 vectors of 8 floats (8 NEON vectors of 4 floats on ARM).
2. **Fast matrix.** Use the fast Walsh-Hadamard or Householder transform, not a dense matrix multiply.
3. **Block processing.** Process in blocks where delay length ≥ block size so lines can be read and written as contiguous vectors. Short lines handled in a sample-by-sample path.
4. **Cheap modulation.** Pre-compute modulation signals per block with a slow control rate (e.g., every 16 to 32 samples) and interpolate. Use shared noise sources with phase offsets.
5. **Cheap decay filters.** Use first-order shelving pairs or a 3-band crossover with shared coefficients per group of lines. High mode uses cascaded biquads with coefficient interpolation only when parameters change.
6. **ER stage.** Sparse taps in a single circular buffer with vectorized gather where possible. Reduce tap count dynamically in Eco mode.
7. **Convolution.** Partition scheme tuned by benchmark (head size, partition sizes). Use frequency-domain delay line and split-complex layout for SIMD multiply-accumulate.
8. **Cache behavior.** Keep hot state in L1/L2; align buffers to 64 bytes; avoid power-of-two delay-buffer strides that cause cache set conflicts.
9. **Precision.** Float for signal path; double for filter coefficient design and for slow recursive state where needed.
10. **Profile continuously.** Run a benchmark suite (Google Benchmark) locally before commits; flag regressions greater than 5% (use instruction counts or repeated runs to reduce noise). Use Instruments (macOS), VTune/Superluminal (Windows) for hot-spot work.
11. **Quality-versus-cost trade study.** For each quality knob (line count, ER taps, interpolation), measure the Q-metric change against CPU saved. Set mode definitions from the data, not by guess.

### 8.4 Latency
- Algorithmic mode: 0 samples reported. Look-ahead features, if any, are reported to the host.
- Convolution: zero-latency design. If a mode needs latency, it is reported and labeled.
- Linear-phase EQ options are excluded from zero-latency paths.

### 8.5 Parameter smoothing and safe changes
- Gain and mix parameters: smoothed per sample or per small block (e.g., 5 to 20 ms).
- Delay-length parameters (size, position): change the read pointer through smoothed fractional delay, slowly enough to avoid audible pitch glide, or use two parallel engines with equal-power crossfade for large jumps. Choose by listening test.
- Decay parameters: update filter coefficients at control rate, interpolated.
- Preset recall: crossfade wet signal; avoid hard cuts of the tail where possible.

## 9. Platform and format requirements

| Item | Requirement |
|---|---|
| OS | Windows 10/11 x64; macOS 12+ (Intel and Apple Silicon, universal binary) |
| Formats | VST3, AAX Native (Pro Tools 2023+), AU v2/v3; CLAP optional |
| Sample rates | 44.1 kHz to 192 kHz |
| Bit depth | 32-bit float host I/O; 64-bit double-precision host processing supported by converting at the edges |
| Hosts to test | Pro Tools, Logic Pro, Ableton Live, Cubase/Nuendo, Studio One, Reaper, Bitwig, FL Studio, Digital Performer, Ardour (VST3) |
| Validation | pluginval (strictness 10), Avid AAX validator and DigiShell tests, Apple `auval`, Steinberg VST3 validator |
| Signing | Apple notarization; Windows code signing; AAX PACE/iLok signing via Avid developer account |

## 10. Testing strategy

1. **Unit tests** for filters, delay lines, matrices (orthogonality), smoothers, convolution (compare with direct convolution).
2. **Energy and stability tests.** FDN stays stable for all parameter ranges, including freeze and extreme sizes. Random parameter fuzzing for 24 h of audio, check for NaN, Inf, DC drift, and overflow.
3. **Objective quality tests** (Section 7) in CI on rendered files.
4. **Golden-file regression tests** with tolerance, for each preset.
5. **Reference comparison.** Render rooms with pyroomacoustics; compare ER timing and decay against the plugin's room-model outputs.
6. **Performance tests** on CI hardware and on reference machines (Section 8).
7. **Host matrix tests** (manual + scripted): load, automate, save/restore, offline bounce, sample-rate change, buffer-size change, multiple instances, bypass, freeze.
8. **Listening tests.** Structured ABX and MUSHRA-style sessions with test material: drums, vocals, solo piano, acoustic guitar, orchestral, speech, clicks, sine bursts.
9. **Pro Tools-specific tests.** Automation write/read; session recall; sample-rate and buffer-size changes. AudioSuite (offline render matching real time) is out of scope: JUCE's AAX wrapper does not support it.

## 11. Offline tools (Python, not shipped in the plugin)

- `room_model.py`: pyroomacoustics shoebox/polyhedral rooms → ER tap tables and IRs for presets.
- `ir_analysis.py`: T60 per band, echo density, IACC, spectral flatness, EDC (Schroeder) plots.
- `fdn_prototype.py`: quick Python prototype of FDN designs for tuning before C++ port.
- `tail_match.py`: fit FDN parameters to a measured IR (used in hybrid mode and for preset creation).
- All tools run headless in CI and output JSON/CSV and plots.

Note: pyroomacoustics gives fixed source/receiver IRs and specular reflections. It does not supply late-tail realism or real-time behavior. It is a data source and reference, not an engine.

## 12. Milestones

| Phase | Deliverable | Exit criteria |
|---|---|---|
| M0 Research (2 wks) | Prototype FDN in Python; reference rooms; quality metric scripts | Q1 to Q7 measurable on prototype |
| M1 Core DSP (6 wks) | C++ FDN + ER + diffusion; CLI test renderer; benchmark harness | Q1, Q4, Q5, Q8 pass; CPU within 2x of target |
| M2 Plugin shell (3 wks) | JUCE VST3/AU plugin, parameters, smoothing, state | Passes pluginval; no clicks (Q10) |
| M3 Optimization (4 wks) | SIMD paths, quality modes, perf CI | Section 8 budgets met on reference machines |
| M4 Convolution + hybrid (6 wks) | Partitioned convolution, IR tools, hybrid mode | Zero-latency; CPU budget met; Q-gates for hybrid |
| M5 UI and presets (5 wks) | Full UI, displays, 100+ presets | Usability review pass |
| M6 AAX and hosts (4 wks) | AAX build, Pro Tools tests, signing | Avid validation pass; host matrix pass |
| M7 Own testing | Daily use in your own sessions, ABX listening, bug fixing | Q12 pass; no crashes in your sessions |

Durations are rough and assume one developer working part time. Adjust freely. Build an AAX skeleton in M2 so Pro Tools problems show early.

## 13. Risks

| Risk | Impact | Mitigation |
|---|---|---|
| Quality targets hard to reach by metrics alone | High | Pair objective metrics with listening tests early (M1); keep tuning budget |
| CPU budget missed in High mode | Medium | Quality-versus-cost study (8.3 #11); drop line count or bands in High if needed |
| AAX approval and signing delays | High | Apply to Avid early (M0); allocate buffer in M6 |
| Zero-latency convolution complexity | Medium | Use proven partition schemes; fall back to labeled low-latency mode |
| Pitch artifacts from smooth size changes | Medium | Dual-engine crossfade fallback |
| Denormal or NaN faults in feedback loops | High | Fuzz testing, FTZ/DAZ, guards, NaN watchdog that resets state safely |
| Differences between platforms (float rounding, SIMD) | Medium | Test tolerance bands; avoid bit-exact claims across platforms |
| IR licensing for factory content | Medium | Create own captures or synthesized IRs; legal review |
| Scope growth (surround, extra engines) | Medium | Hold v1 scope; design N-channel graph but ship stereo |

## 14. Licensing and legal

- JUCE: personal use fits the free tier. Check terms again if you ever distribute.
- AAX SDK: Avid developer agreement required. Pro Tools also needs signed plugins (or a developer Pro Tools build) to load AAX.
- Third-party libraries: prefer permissive licenses (MIT, BSD, Apache). KFR is GPL; avoid it unless you accept that.
- pyroomacoustics is MIT licensed. Keep the notice in tools.
- Factory IRs: use your own captures or synthesized IRs.
- No copy protection.

## 15. Open questions

1. Which reference reverbs to use in your listening tests?
2. Is CLAP support worth the effort?
3. Inclusion of a learned component in a later version (CPU-only inference)?

## 16. Success criteria

- All Section 7 gates pass.
- Section 8 CPU budgets met on your machines.
- You use it in your own Pro Tools sessions and prefer it to your current reverbs.
- Your host matrix (the hosts you actually use) passes.
- No telemetry, accounts, or data collection of any kind.
