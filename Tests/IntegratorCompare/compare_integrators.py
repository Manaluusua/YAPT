"""Renders the same scene with each integrator and reports where they disagree.

All three integrators are unbiased estimators of the same image, so once converged they
must agree. They reach the light in different ways, which is what makes this a useful
test of an emission change: BPT only finds the emitter by BSDF sampling, BPT_WITH_NEE
samples a position on it, and BDPT additionally samples emitted *directions* from it. A
mistake in one of the sampling / pdf / evaluate sites shows up as one integrator drifting
away from the other two.

Run a focus 0 comparison first: that is the plain lambertian path, so its numbers are this
scene's noise floor. A focused run is fine as long as it stays in the same band.

    python compare_integrators.py                 # focus 8, one sided
    EMISSION_FOCUS=32 python compare_integrators.py
    TWO_SIDED=1 EMISSION_FOCUS=0 python compare_integrators.py

Environment:
    EMISSION_FOCUS  cosine power exponent for the emitter (default 8)
    TWO_SIDED       1 to make the emitter two sided and move it mid room (default 0)
    ACCUM_S         seconds of accumulation per integrator (default 25)
    OUT_DIR         output folder name (default results/compare_focus<n>[_twosided])
    YAPT_CONFIG     Debug or Release (default Debug)
"""

import os
import sys

import yapt_harness
from yapt_harness import TestHarness, TEST_DIR, HarnessError
from report import write_comparison

FOCUS = float(os.environ.get("EMISSION_FOCUS", "8"))
TWO_SIDED = os.environ.get("TWO_SIDED", "0") == "1"
ACCUM_S = float(os.environ.get("ACCUM_S", "25"))

DEFAULT_OUT = f"compare_focus{FOCUS:g}" + ("_twosided" if TWO_SIDED else "")
OUT_DIR = TEST_DIR / "results" / os.environ.get("OUT_DIR", DEFAULT_OUT)

#(renderer variable option index, label)
INTEGRATORS = [(0, "bpt"), (1, "bpt_nee"), (2, "bdpt")]


def main():
    os.environ["TWO_SIDED"] = "1" if TWO_SIDED else "0"

    harness = TestHarness(OUT_DIR)
    captures = {}

    def load():
        harness.load_scene(
            TEST_DIR / "focused_emission_scene.py",
            {"EMISSION_FOCUS = 8.0": f"EMISSION_FOCUS = {FOCUS}"},
        )
        harness.set_pipeline(INTEGRATORS[0][0])
        print(f"  focus={FOCUS:g} two_sided={TWO_SIDED} accumulating {ACCUM_S:g}s per integrator",
              flush=True)

    def make_step(index):
        def step():
            label = INTEGRATORS[index][1]
            captures[label] = harness.capture(label)
            if index + 1 < len(INTEGRATORS):
                harness.set_pipeline(INTEGRATORS[index + 1][0])
        return step

    steps = [(load, ACCUM_S)]
    steps += [(make_step(i), ACCUM_S) for i in range(len(INTEGRATORS))]

    harness.run(steps)
    write_comparison(captures, OUT_DIR)


if __name__ == "__main__":
    try:
        main()
    except HarnessError as error:
        print(f"\n  FAILED: {error}", file=sys.stderr)
        sys.exit(1)
