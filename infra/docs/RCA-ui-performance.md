# RCA: Chromosphere | INFRA interface instability and slowdowns

| | |
|---|---|
| **Product** | Chromosphere \| INFRA (VST3 plug-in and standalone app), Windows |
| **Affected builds** | 0.1.40 and earlier (`infra-nightly`) |
| **Fixed in** | The next `infra-nightly` build from branch `claude/awesome-bell-rehevx`. Installed copies get it through the updater. |
| **Symptom** | Severe slowdowns and instability while the INFRA window is open, in a DAW or the standalone app |
| **Severity** | High: the instrument is hard to use with its window open |

## Summary

INFRA's interface was redrawing far more than it needed to, and drawing it the wrong way for Windows.

1. **Too much work on the UI thread.** Several animated elements repainted continuously at 20 to 30 frames a
   second. Each repaint also redrew the large static parts behind them, including two expensive blurred drop
   shadows, with nothing cached. With no audio playing, the standalone app used **121% of a CPU core**.
2. **Images on the wrong side of the GPU on Windows.** JUCE 8 draws with Direct2D on Windows, so every
   `juce::Image` is a GPU bitmap by default. Five effects touched those images pixel by pixel every frame: the
   live sun, the scope glow, the wordmark lens, the LED readout bloom and the plate grain. Each read made the CPU
   wait for the GPU to finish and copy the image back, and each write sent it to the GPU again, many times a
   second. These round trips stall the DAW's UI thread and do not show up on Linux, where the build was tested.
3. **GPU images made off the UI thread.** The live sun renders on its own background thread. On Windows each of
   its frames created and wrote a new Direct2D bitmap from that thread, about 40 a second for the two suns.
   JUCE's Direct2D images are not safe to create or use outside the message thread. This is the most likely
   cause of the instability: crashes, hangs and graphics errors.
4. **GPU images destroyed at plug-in unload.** Shared image caches were function-level statics. In a plug-in,
   statics are destroyed while the host unloads the DLL, which is too late to release Direct2D resources
   safely. This could crash or hang the DAW when it closes or removes the plug-in.

## Measurements

The standalone app at the default 1280 x 900 window, idle (no notes playing), CPU time over 15 to 20 s after a
6 s warm-up. Measured on Linux with an X server (Xvfb) and JUCE's software renderer on a cloud VM, because
there is no Windows machine here (see **Verification**).

| Step | CPU (% of one core) |
|---|---|
| Before (0.1.40) | **121%** |
| Static background cached; header, scope and VFD repaints reduced; sun images moved to the CPU | 73% |
| Scope sun at a quarter resolution, 15 fps, idle when hidden | 55% |
| Bilinear sun upscale; scope repaints once per sun frame | 47% |
| Separable blur; wordmark lens only redraws its changing columns | **46%** |

The remaining 46% is mostly the software renderer scaling and blending images onto the window. On Windows,
Direct2D does that on the GPU. The Windows-only stalls in causes 2 and 3 are not in this table at all, so the
improvement on Windows should be larger than these numbers show.

## How it was found

1. The idle CPU of the standalone app was measured, as above.
2. Thread stacks were sampled about 40 times with `gdb` (a poor man's profiler; `perf` is not available here),
   and each sample was attributed to a function.
3. **Before:** most UI-thread samples were inside `InfraEditor::paint`, then `look::drawPlate`, then
   `juce::DropShadow::drawForPath`, then the box blur. That is the two plate shadows being blurred again on
   every repaint. The sun render threads spent most of their samples in `sun::renderCpu`.
4. The rendering code was then reviewed for how it uses images, and JUCE 8's Direct2D code was checked
   (`juce_Direct2DImage_windows.cpp`, `juce_Direct2DGraphicsContextImpl_windows.cpp`):
   - `NativeImageType::create` returns a `Direct2DPixelData`, which is a GPU bitmap.
   - `Image::BitmapData` on such an image forces a backup, a GPU-to-CPU copy.
   - Drawing a software image into a Direct2D context converts it with `NativeImageType{}.convert(...)`, an
     upload, every time it is drawn.
   - Every `Direct2DPixelData` registers itself as a listener on the shared DirectX adapter list, and
     unregisters in its destructor.
5. **After each fix:** the build was profiled again until the UI thread no longer showed any one hot spot
   from INFRA's own code.

## Root causes and fixes

### 1. Static background redrawn on every repaint

`InfraEditor::paint` drew the background texture and both instrument plates every time. Every plate draw ran
`juce::DropShadow::drawForPath`, which renders the plate's whole outline into a mask and box-blurs it. The
dials, displays and suns in front are transparent, so each of their repaints, 20 to 30 times a second, ran the
editor's paint for the area behind them. In practice that is the whole window.

**Fix:**
- The texture and plates are drawn once per window size into one cached image (`chrome_`). Each repaint is
  now a single image copy.
- The header status line ("Voices · bpm · kHz · version") used to repaint the whole brand area, both
  animated suns included, 30 times a second. It now repaints only its own text, and only when the text
  changes.

### 2. Duplicate and unnecessary repaint sources

| Source | Before | After |
|---|---|---|
| Scope (with its blur) | Repainted by the editor timer (30 Hz) *and* the sun timer (24 Hz) | Repainted only when a new sun frame arrives (15 Hz) |
| VFD dot-matrix displays (preset name, every dropdown, mod matrix) | Each repainted 30 times a second for a 1% brightness flicker: hundreds of dot sprites per display | Repaint only when their dots change; the flicker shows on those repaints. Hidden pages do nothing. |
| Mod matrix rows (8 rows, each with two VFD dropdowns) | Repainted every frame | Repaint only when a slot turns on or off |
| Wordmark lens | Recomputed every pixel 30 times a second | 20 Hz, and only the columns around the breathing cursor between glitches; the whole word only during a glitch |
| Sun render threads | Kept rendering when the window was hidden or minimised | Idle while the view is not showing |

### 3. Pixel access on GPU images (Windows)

These images are read or written on the CPU every frame. They were native images (Direct2D bitmaps) and are
now `juce::SoftwareImageType()` images:

| Image | What touched it |
|---|---|
| `sun::renderCpu` frames | Written pixel by pixel on the sun's render thread |
| Scope trace layer and its glow | Drawn on the CPU, scaled down, then blurred |
| Wordmark source and output | Read and written by the lens pass |
| Seven-segment bloom layer | Blurred |
| Plate grain | Written once as a software image, then converted to a native image for drawing |

Images that are only ever drawn stay native, so Direct2D keeps them on the GPU: the cached background and
plates, dial bodies, VFD sprites and glass layers, and the cached seven-segment readouts.

`juce::ImageConvolutionKernel`, a generic 2D convolution, was replaced for the two glow effects by
`look::gaussianBlur`, a separable Gaussian with the same weights. It is much cheaper per frame. The old blur
was applied in place, so it read pixels it had already blurred. The new one is a true Gaussian; the glow looks
right in the editor snapshots.

### 4. Direct2D images created on a background thread (Windows)

The sun's render thread created a new native image for each frame. These frames are now software images, so
that thread never touches Direct2D. The UI thread draws the finished frame, which uploads it once.

### 5. Shared GPU images destroyed during DLL unload (Windows)

The caches for asset images, background textures, plate grain and dial bodies were function-level statics. They
are now emptied by `look::releaseImageCaches()` when the last INFRA editor closes. When the host unloads the
plug-in, no Direct2D image is left to destroy.

### 6. Sun cost

- **Scope sun:** a quarter of the display's resolution (was 35%), at 15 fps (was 20). It sits dimmed behind
  the traces, and the soft upscale hides the difference.
- **Emblem sun:** 20 fps (was 24).
- **Upscaling:** bilinear (was high-quality). The source frame is soft by design.

## Verification

- All INFRA tests pass: DSP, plugin smoke, all 107 presets rendered, updater.
- Editor snapshots at 1180 x 820, 1280 x 900 and 1700 x 1150, on every page, render correctly: the
  scope glow, LED bloom, VFDs, wordmark and both suns.
- Idle CPU fell from 121% to 46% of a core on the Linux software renderer, as in the table above.
- **Not yet verified on Windows hardware:** the Direct2D stalls and the off-thread image creation can only be
  observed on Windows. The fixes follow from JUCE's Direct2D code (cited above), but the improvement still
  needs confirming in a DAW on Windows. Watch the DAW's CPU and UI responsiveness with the INFRA window open,
  and check that no crash or hang happens when closing the window or the DAW.

## Prevention

- **Images:** any image whose pixels are read or written (`Image::BitmapData`, blurs, `rescaled`), or that
  is touched off the message thread, is created with `juce::SoftwareImageType()`. Images that are only
  drawn stay native. The comment at each image's creation says which applies and why.
- **Static content:** expensive static drawing (shadows, textures, plates) is cached per size. It is never
  redrawn in a `paint` that runs every frame.
- **Animation:** each animated area has one repaint source, and nothing animates while it is hidden.
- **CI:** an idle-CPU check for the standalone app on Linux would catch regressions of the repaint and
  caching kind. It cannot catch the Windows-only GPU issues, which need a Windows test with a real GPU.

## Related: Chromosphere EXO

INFRA's sun, VFD, seven-segment and wordmark code was ported from EXO (branch `prd-draft-2`). EXO very likely
has the same Windows image issues (causes 3 to 5), and possibly the repaint issues too. Its code has not been
changed here. The same fixes apply to it.
