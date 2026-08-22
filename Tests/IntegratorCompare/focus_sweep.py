"""Sweeps EmissionFocus on one integrator and shows how the emission profile redistributes.

The cosine power profile is normalised by (n+2)/2, which keeps the total emitted power
fixed as the lobe narrows. The signature of that being wired up correctly is: the overall
brightness stays roughly flat while the peak climbs and the periphery falls below where it
started. If the normalisation were missing anywhere the whole image would instead dim by
2/(n+2) - a factor of 17 at n = 32, impossible to miss.

Difference images are written against the first focus value in the sweep, so the
redistribution is visible directly rather than having to be read out of the numbers.

    python focus_sweep.py
    FOCI=0,4,16,64 ACCUM_S=30 python focus_sweep.py

Environment:
    FOCI        comma separated exponents to sweep (default 0,2,8,32)
    INTEGRATOR  0 = BPT, 1 = BPT_WITH_NEE, 2 = BDPT (default 1)
    TWO_SIDED   1 to make the emitter two sided and move it mid room (default 0)
    ACCUM_S     seconds of accumulation per step (default 20)
    OUT_DIR     output folder name (default results/sweep)
    YAPT_CONFIG Debug or Release (default Debug)
"""

import os
import sys

import numpy as np
from PIL import Image

import yapt_harness
from yapt_harness import TestHarness, TEST_DIR, HarnessError
from report import build_montage, blockwise_difference_image, difference_stats

FOCI = [float(v) for v in os.environ.get("FOCI", "0,2,8,32").split(",")]
INTEGRATOR = int(os.environ.get("INTEGRATOR", "1"))
TWO_SIDED = os.environ.get("TWO_SIDED", "0") == "1"
ACCUM_S = float(os.environ.get("ACCUM_S", "20"))
OUT_DIR = TEST_DIR / "results" / os.environ.get("OUT_DIR", "sweep")

INTEGRATOR_NAMES = {0: "BPT", 1: "BPT_WITH_NEE", 2: "BDPT"}


def print_profile(captures):
    """Horizontal brightness profile across the floor, as the lobe narrows."""
    print()
    print("  floor brightness profile (row 78% down the image, 12 buckets across):")
    for focus, image in captures.items():
        luma = np.asarray(Image.fromarray(image).convert("L"), np.float64)
        row = luma[int(luma.shape[0] * 0.78)]
        buckets = row[: len(row) // 12 * 12].reshape(12, -1).mean(axis=1)
        print(f"    n={focus:>4g}: " + " ".join(f"{v:6.1f}" for v in buckets))
    print()
    print("  peak should climb while the edges fall below the n=0 row: that crossover is")
    print("  energy being redistributed rather than gained.")


def main():
    os.environ["TWO_SIDED"] = "1" if TWO_SIDED else "0"

    harness = TestHarness(OUT_DIR)
    captures = {}

    #only the emitter material is kept, and it is dropped after the last step. holding python
    #references to renderer owned objects past the run makes Renderer.shutdown() hang.
    light = {"mat": None}

    def load():
        context = harness.load_scene(TEST_DIR / "focused_emission_scene.py")
        light["mat"] = context["light_mat"]
        harness.set_pipeline(INTEGRATOR)
        light["mat"].setEmissionFocus(FOCI[0])
        print(f"  integrator={INTEGRATOR_NAMES.get(INTEGRATOR, INTEGRATOR)} "
              f"two_sided={TWO_SIDED} accumulating {ACCUM_S:g}s per step", flush=True)

    def make_step(index):
        def step():
            focus = FOCI[index]
            captures[focus] = harness.capture(f"focus_{focus:g}")
            if index + 1 < len(FOCI):
                light["mat"].setEmissionFocus(FOCI[index + 1])
            else:
                light["mat"] = None
        return step

    steps = [(load, ACCUM_S)]
    steps += [(make_step(i), ACCUM_S) for i in range(len(FOCI))]
    harness.run(steps)

    print()
    print(f"  {'focus':>6s} {'mean':>8s} {'p99':>7s} {'vs first: rel|d| 16x':>22s} {'gap':>8s}")
    print("  " + "-" * 56)
    base_focus = FOCI[0]
    base = captures[base_focus]
    for focus in FOCI:
        image = captures[focus]
        stats = difference_stats(base, image)
        p99 = np.percentile(image, 99)
        print(f"  {focus:6g} {image.mean():8.3f} {p99:7.1f} {stats['rel_down']:21.2f}% "
              f"{stats['mean_gap']:+7.2f}%")

    print_profile(captures)

    #renders on the top row, difference against the first focus value underneath
    top = [(captures[f], f"EmissionFocus = {f:g}", f"mean {captures[f].mean():.1f}") for f in FOCI]
    bottom = []
    for focus in FOCI:
        diff = blockwise_difference_image(base, captures[focus], amplify=6.0)
        Image.fromarray(diff).save(OUT_DIR / f"diff_focus{base_focus:g}_vs_focus{focus:g}.png")
        stats = difference_stats(base, captures[focus])
        bottom.append((diff, f"vs n = {base_focus:g}  16px blocks, signed (x6)",
                       f"rel|d| 16x = {stats['rel_down']:.2f}%   gap {stats['mean_gap']:+.2f}%"
                       f"   red = brighter than n={base_focus:g}, blue = darker"))

    montage_path = OUT_DIR / "sweep.png"
    build_montage([top, bottom], path=montage_path)
    print(f"  wrote {montage_path}")


if __name__ == "__main__":
    try:
        main()
    except HarnessError as error:
        print(f"\n  FAILED: {error}", file=sys.stderr)
        sys.exit(1)
