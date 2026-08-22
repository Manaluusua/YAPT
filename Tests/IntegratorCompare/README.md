# IntegratorCompare

Cross-checks the three path integrators against each other, and sweeps the emissive
`EmissionFocus` parameter. Written while adding the cosine power emission profile, but the
comparison itself is not specific to that feature - any change to light transport can be
checked this way.

## Why comparing integrators is a useful test

All three integrators are unbiased estimators of the same image, so once converged they
must agree. They reach the light by different routes, which is what makes the comparison
sharp:

| pipeline | index | how it finds an emitter |
| --- | --- | --- |
| `BPT` | 0 | BSDF sampling only - it has to stumble onto the emitter |
| `BPT_WITH_NEE` | 1 | samples a *position* on the emitter, direction fixed by geometry |
| `BDPT` | 2 | additionally samples emitted *directions* from the emitter |

An emission change touches all three differently: a cosine power profile enters `evaluate`
alone for NEE, but enters `sample`, `pdf` *and* `evaluate` for BDPT's light subpath. If any
of those disagree, one integrator drifts away from the other two.

**Always run a `EMISSION_FOCUS=0` comparison first.** That is the plain Lambertian path,
untouched by the focus feature, so its numbers are this scene's noise floor. A focused run
is fine as long as it stays in the same band - not because it hits some absolute threshold.

## Running

Needs a built `Debug` (or `YAPT_CONFIG=Release`) configuration and the plane asset the
scene loads. Working directory does not matter; the harness locates the repo itself.

```bash
python compare_integrators.py                      # focus 8, one sided
EMISSION_FOCUS=0 python compare_integrators.py     # the control
EMISSION_FOCUS=32 TWO_SIDED=1 python compare_integrators.py
python focus_sweep.py                              # focus 0, 2, 8, 32
```

Results land in `results/<run>/`: the individual captures, per pair difference images, and
a `comparison.png` / `sweep.png` montage. The pairwise table is printed to stdout.

Shared environment variables: `EMISSION_FOCUS`, `TWO_SIDED`, `ACCUM_S` (seconds of
accumulation per capture), `OUT_DIR`, `YAPT_CONFIG`, `YAPT_ROOT`, `YAPT_ASSET_ROOT`.
`focus_sweep.py` also takes `FOCI` and `INTEGRATOR`.

## Reading the output

```
  pair                       mean A   mean B      gap  rel|d| full  rel|d| 16x    p99   max
  bpt vs bpt_nee             56.674   56.854   +0.32%        8.43%       0.35%   16.0    30
  bpt vs bdpt                56.674   58.967   +4.05%        8.83%       4.04%   17.0    33
  bpt_nee vs bdpt            56.854   58.967   +3.72%        3.66%       3.70%    4.0     7
```

`rel|d| full` is dominated by per-pixel Monte Carlo noise and stays high however long you
accumulate - **ignore it for judging agreement**. `rel|d| 16x` block averages first, which
cancels zero mean noise and leaves only systematic disagreement. That is the number to
read.

The montage says the same thing visually, in three rows:

1. the renders
2. per-pixel difference (heatmap) - mostly noise, useful for spotting *structured*
   differences like a lit region only one integrator finds
3. 16px block averaged difference, **signed** - blue where the second image is darker, red
   where it is brighter, near black where they agree

Row 3 is the one to look at. A uniform wash of one colour means a flat brightness offset; a
localised patch means a transport difference in that part of the scene.

## Known result at the time of writing

BDPT sits about 3-4% brighter than both unidirectional integrators, and it does so at
`EMISSION_FOCUS=0` too - on the Lambertian path that predates the focus feature. Row 3 of
`results/compare_focus0/comparison.png` shows it as a flat, uniform field rather than a
localised patch. It is a pre-existing BDPT discrepancy, unrelated to the emission profile,
but it is real. Treat ~4% as this scene's BDPT baseline until it is chased down.

BPT vs BPT_WITH_NEE agree to well under 1%, and the spread grows only mildly with focus
(0.35% at n=0, 0.68% at n=8, ~1.7% at n=32) - the expected variance cost of BSDF sampling a
narrowing lobe.

## What focus_sweep checks

The profile is `Le = emissive * (n+2)/2 * |cos|^n`. The `(n+2)/2` factor keeps total
emitted power fixed as the lobe narrows, so raising focus should *redistribute* light, not
create or destroy it. Signature of that working:

- overall mean stays flat (56.9, 64.0, 67.5, 65.2 for n = 0, 2, 8, 32)
- the peak climbs hard (p99 goes 68 -> 192)
- the periphery drops **below** where it started

That last crossover is the important one - the printed floor profile shows the edges
falling from 51.6 at n=0 to 39.6 at n=32 while the centre rises from 64.5 to 163.5. If the
normalisation were missing anywhere the whole image would instead dim by `2/(n+2)`, a
factor of 17 at n=32.

## Capturing, and why it is fiddly

The render area is a native HWND with a flip model DXGI swapchain that DWM composites
directly. That defeats the obvious capture routes:

- `QScreen.grabWindow(0, ...)` returns whatever is *behind* the window, usually the desktop
  wallpaper, and looks entirely plausible until you notice the render never changes
- `QWidget.grab()` returns the flat Qt background, since Qt never paints that area
- `QScreen.grabWindow(winId)` likewise

`PrintWindow` with `PW_RENDERFULLCONTENT` is the one path that reads the composited
DirectX content, and is what `yapt_harness.capture_window()` uses.

Because a broken capture produces *convincing* numbers rather than an obvious failure,
every capture is md5 hashed and compared against the ones already taken in that run. Two
identical captures mean the frame never updated, and the run aborts with `FAILED: stale
captures` instead of reporting the results.

## Files

| file | |
| --- | --- |
| `yapt_harness.py` | boots the app, drives it from a timer, captures the render area |
| `report.py` | difference images, montages, the printed table |
| `focused_emission_scene.py` | closed diffuse box with one focused emitter |
| `compare_integrators.py` | renders under each integrator, reports disagreement |
| `focus_sweep.py` | sweeps `EmissionFocus` on one integrator |
| `dbgview.py` | standalone `OutputDebugString` listener, see below |

## dbgview.py

`YAPT_LOG_*` writes to `OutputDebugStringA` only, and `YAPT_LOG_FATAL_ERROR` calls
`__debugbreak()`. Outside a debugger a shader compile failure therefore kills the app with
no message at all and no stdout. Run this in another terminal to see the log:

```bash
python dbgview.py 300      # listen for 300 seconds
```

This is how the `denoise.comp` syntax error that broke startup was found.

## Gotchas

- The harness `chdir`s to `Build/Bin`; the renderer resolves shaders, LUTs and its cache
  relative to the working directory. Output paths are made absolute first.
- Do not hold Python references to renderer owned objects (materials, render objects) past
  the end of a run - `Renderer.shutdown()` hangs. `focus_sweep.py` drops its material
  reference after the last step for exactly this reason.
- The scene pins `Tonemap.UseAutoExposure` to off. Auto exposure would normalise away the
  brightness differences these tests exist to measure.
