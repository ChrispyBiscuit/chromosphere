# Optimization guide for the plug-in suite

This guide comes from the Chromosphere | EXO performance work. It applies to any plug-in in the
suite that is built with JUCE 8 and runs as VST3/AAX on Windows. The target is a session with
20 or more instances open in FL Studio or Pro Tools, with no hang and no UI lag.

Use it as a checklist. Each section gives the symptom, the cause, the fix and a way to verify
the fix.

## 1. What went wrong in Chromosphere

| Symptom the user saw | Cause | Fix |
|---|---|---|
| Whole system froze (DAW, other apps; only Task Manager worked) | OpenGL contexts inside the DAW window | OpenGL is off by default (opt-in preference). Animated views render on the CPU, on a low-priority thread. |
| DAW lagged with the editor open, while the plug-in showed low CPU | UI thread work: per-pixel access to GPU bitmaps (Direct2D), full repaints at display rate, waits on vsync | Software images for per-pixel work. Cached static layers. Low frame rates. Small repaint areas. |
| Hang a few seconds after loading a session with many instances | Each instance had its own updater. All of them checked the network, downloaded the same file and wrote the settings file at the same time. | One updater per process (`SharedResourcePointer`) behind a cross-process lock |
| CPU used by hidden or minimised editors | Timers and render threads ran with no window on screen | All animation stops while the window is not visible |
| CPU used by silent instances | DSP ran on silence. A DC offset in a feedback loop kept one mode "active" for ever. | Sleep on silence. DC blocker in feedback paths. |

Measured results (Linux, software renderer, one core = 100%):

| Item | Before | After |
|---|---|---|
| Open editor, idle | 140% | 10% |
| Open editor, signal playing | — | about 30% |
| One instance playing, mean of 21 modes | 2.10% | 1.19% |
| One silent instance | same as playing | 0.045% |

## 2. Measure first

Do not change code before you have a number. Record each number before and after each change.

1. **DSP cost per instance.** Write a small benchmark that builds the processor and calls
   `processBlock` with 256-sample blocks at 48 kHz for about 10 s of audio, for each mode. Report
   the time as a percentage of real time. Measure with signal and with silence. Reference:
   `tools/plugin_bench.cpp`.
2. **Editor cost.** Open the editor in a real window (on Linux: under `xvfb-run`). Read the
   process CPU time from `/proc/<pid>/stat` (fields 14 and 15) over 15 s or more. Measure idle
   and with signal.
3. **Many instances.** Load 20 instances in a DAW session. Open and close editors. Minimise the
   DAW. Watch for UI stalls, not only CPU percentage. A UI stall shows as low CPU with an
   unresponsive window.
4. **Validation.** Run `pluginval` at strictness 10 after each change.
5. **Block-size independence.** Render the same input with block sizes 1, 7, 64, 256 and 4096.
   The outputs must be identical (or within 1e-6). Optimizations in chunked code often break
   this.

## 3. UI thread rules

The DAW's UI and every plug-in editor share one message thread. If one editor blocks it, the
whole DAW stops responding, even when CPU use is low.

### 3.1 Images on Windows (Direct2D)

JUCE 8 draws with Direct2D on Windows. A plain `juce::Image` is then a GPU bitmap. Each
`BitmapData` access, `getPixelAt` or `setPixelAt` copies data between GPU and CPU.

- For any image that you read or write per pixel (noise, blur, generated textures, frames),
  use a software image:

  ```cpp
  juce::Image softwareImage(juce::Image::PixelFormat fmt, int w, int h, bool clear) {
      return juce::Image(juce::SoftwareImageType().create(fmt, std::max(1, w), std::max(1, h), clear));
  }
  ```

- Never create a native (Direct2D) image off the message thread. A background thread makes
  software images only. The message thread converts the newest frame once
  (`juce::NativeImageType().convert(img)`) and draws the converted copy.
- Do blurs as a separable Gaussian on a software image, not with a 2-D convolution kernel.

### 3.2 Repaint less

- Draw static parts (textures, panels, shadows, labels, dividers) once into a cached image.
  Rebuild that image only on resize or on a layout change.
- Repaint only the area that changed (`repaint(rect)`), for example a cursor or a meter.
- Repaint a component only when its content changes. Compare the new state to the last drawn
  state and skip the repaint if they are equal.
- Do not repaint at the display refresh rate. Typical rates that look good:
  - Meters and glows: 15 Hz, and only when the value changes by more than a small step.
  - Text effects and cursors: 15 Hz.
  - Animated backgrounds: 6–8 fps when quiet, up to 15–20 fps with signal.
- Stop a component's timer when its animation is settled (for example, a display whose dots
  have stopped fading).

### 3.3 Stop all animation when the window is not visible

```cpp
bool Editor::windowVisible() const {
    if (!isShowing()) return false;
    if (auto* peer = getPeer()) return !peer->isMinimised();
    return true;
}

void Editor::timerCallback() {
    const bool vis = windowVisible();
    if (vis != animating_) {
        animating_ = vis;
        for (auto* a : animatedChildren_) a->setAnimating(vis);   // stops their timers and threads
    }
    if (!vis) return;
    // ...normal work
}
```

Each animated component must have `setAnimating(bool)`. When false, it stops its timer, and a
render thread waits on an event instead of running.

### 3.4 Render threads

- One render thread per heavy animated view, at low priority.
- The thread renders at reduced resolution (0.25x–0.6x) and the message thread scales the image
  up. Soft visuals do not show the difference.
- The thread hands over the newest frame with a lock that is held only for a pointer swap.
  The message thread never waits for a frame.
- Use integer hashes for noise, not `sin()` per sample point.

### 3.5 OpenGL

Do not turn OpenGL on by default. Each `OpenGLContext` adds a thread that takes the message
manager lock every frame and can wait on vsync. Inside a DAW window this froze the user's whole
system. If a plug-in needs OpenGL, put it behind a preference that is off by default, set the
swap interval to 0 and drive frames from a timer.

## 4. Process-wide resources

All instances of a plug-in in one DAW run in one process. Anything "per instance" is multiplied
by the instance count.

- **Shared caches.** Use `juce::SharedResourcePointer<T>` for image caches, fonts, look-up
  tables and loaded assets. Do not use function-local statics for JUCE objects: they are
  destroyed at DLL unload, after JUCE has shut down, which can crash on exit.
- **One updater per process.** The updater is a `SharedResourcePointer`. Each editor listens to
  it with a `ChangeListener`; do not use a single `std::function` callback that a second editor
  or panel can overwrite.
- **Cross-process lock.** Several DAWs or scan processes can run at once. Guard network
  checks, downloads and settings writes with `juce::InterProcessLock("<Product>Updater")`.
- **No load-time storm.** Do not start network, disk or heavy work in the processor
  constructor or in `prepareToPlay`. Start it a few seconds later, from the shared object, once.
- **Settings files.** Reload before you read (`PropertiesFile::reload()`) and write only on a
  real change.
- **Worker threads.** A background thread that polls must sleep between polls (25 ms is enough
  for parameter-driven rebuilds). Better: wait on an event that the audio or UI side signals.

## 5. Audio thread rules

### 5.1 Sleep on silence

Most instances in a session are silent most of the time. A silent instance must cost almost
nothing.

```cpp
float peak = 0.f;
for (int i = 0; i < n; ++i) peak = std::max(peak, std::max(std::fabs(L[i]), std::fabs(R[i])));
if (peak > kSleepLevel || frozen) { silentSamples_ = 0; sleeping_ = false; }
else {
    silentSamples_ += n;
    if (!sleeping_ && silentSamples_ > (juce::int64)(0.25 * getSampleRate()) && lastWetPeak_ < kSleepLevel)
        sleeping_ = true;
}
if (sleeping_) { /* output dry only (or silence); do not run the engine */ }
```

- `kSleepLevel` = 1e-6 (−120 dBFS).
- Any input above the level wakes the engine on the same block. No click: the engine state was
  already below hearing.
- Never sleep while a "freeze", "hold" or infinite mode is on.
- Track the wet output peak (`lastWetPeak_`) so that a long tail is not cut.
- Test it: feed silence, check that the instance sleeps, then feed a click and check that the
  output is correct on the first block.

### 5.2 Remove things that keep the engine awake

- **DC in feedback loops.** Pitch shifters, saturators and asymmetric non-linear stages make DC.
  A loop with DC gain above 1 keeps an offset for ever, so the instance never sleeps. Put a
  20 Hz one-pole DC blocker in every feedback path.
- **Dither on silence.** Do not add dither or noise when the input is digital silence.
- **Denormals.** Use `juce::ScopedNoDenormals` at the top of `processBlock`.

### 5.3 Control rate and chunking

- Compute modulation, smoothing and coefficients at a control rate (every 16 samples), not per
  sample.
- Compute control values once per control period at a fixed point in that period. Do not
  compute them per host block: the result then depends on the host block size.
- Split the host block into fixed chunks. Keep each chunk inside the range the delay buffers
  allow (for example: block length ≤ buffer size − maximum delay).

### 5.4 Inner loops

- Template the hot loop on sizes that are fixed at run time (number of delay lines, channels).
  The compiler then unrolls and vectorises it.
- Mark buffer pointers `__restrict` where they do not alias.
- Fuse loops that read the same data.
- Avoid a wrap test per sample in delay lines. Two methods:
  - Process contiguous runs up to the wrap point, then continue from the start.
  - Keep a small mirror (guard) of the start of the buffer after its end, so a read of up to
    the guard length never wraps.
- Process parallel units (for example, several springs, left and right channels) as SIMD lanes
  (`juce::dsp::SIMDRegister` or plain arrays of 4 or 8 that the compiler vectorises).
- No allocation, no locks, no logging and no file access on the audio thread.

### 5.5 Compiler settings

- MSVC: build the DSP library with `/O2 /arch:AVX2 /fp:contract` (the default is SSE2).
  Keep `/fp:precise`. Check that the users' machines support AVX2 (every Intel and AMD desktop
  CPU since about 2013 does).
- Clang/GCC on Linux: `-O3 -ffp-contract=fast`; use function multi-versioning if you need
  baseline x86-64 builds.

## 6. Updater and installer (if the plug-in updates itself)

- The installer needs admin rights. Do not start it from the DAW: Windows shows the UAC prompt
  behind the DAW window and it is easy to miss.
- Instead, start a hidden, unprivileged waiter process
  (`CreateProcessW` with `CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP`, and
  `CREATE_BREAKAWAY_FROM_JOB` with a fallback without it). The waiter waits for the DAW process
  to exit, then starts the installer with `/VERYSILENT /SUPPRESSMSGBOXES /NORESTART` and
  `-Verb RunAs`.
- Remember the pending install. On the next load, if the version did not change, drop the
  pending flag and try again.
- Stamp the build number into both the plug-in (`CMake -DCS_VERSION:STRING=0.1.<run>`) and the
  installer (`ISCC /DAppVersion=...`). If these differ, the updater offers the same build for
  ever.

## 7. Checklist

Copy this list into the plug-in's issue tracker and tick each item.

- [ ] Benchmarks exist for DSP cost (playing and silent) and editor cost (idle and playing).
- [ ] Every per-pixel image is a software image; no native image is made off the message thread.
- [ ] Static drawing is cached in a layer.
- [ ] No component repaints at display rate; repaint areas are small; timers stop when settled.
- [ ] All animation and render threads stop when the window is hidden or minimised.
- [ ] OpenGL is off by default.
- [ ] Caches and the updater are `SharedResourcePointer`s; no JUCE objects in function statics.
- [ ] Network, download and settings writes are guarded by an `InterProcessLock`.
- [ ] No heavy work in the constructor or `prepareToPlay`.
- [ ] Worker threads sleep or wait between polls.
- [ ] Sleep on silence, with a wake test.
- [ ] DC blockers in feedback paths; no dither on silence; `ScopedNoDenormals`.
- [ ] Control-rate computations; output identical for block sizes 1, 7, 64, 256, 4096.
- [ ] DSP library built with AVX2 on Windows.
- [ ] `pluginval` strictness 10 passes.
- [ ] 20-instance session in FL Studio: no stall when loading, opening and closing editors, or
      minimising.

Targets per instance (one core = 100%): silent < 0.1%, playing < 1.5% for a typical mode,
editor idle < 10%, editor closed adds nothing.
