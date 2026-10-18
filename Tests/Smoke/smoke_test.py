"""Smoke test for one built backend: loads a scene, switches between the three integrators
and resizes the window, checking after every step that frames are still being rendered.

Normally run through run_smoke_tests.py, which builds both backends and runs this once per
build in its own process, so a crash shows up as a failed exit code.

    python smoke_test.py --bin-dir Build/SmokeTest/DirectX12/Bin/Debug --out results/DirectX12
"""

import argparse
import os
import sys
from pathlib import Path

TEST_DIR = Path(__file__).resolve().parent

INTEGRATORS = ["BPT", "BPT_WITH_NEE", "BDPT"]
START_SIZE = (1280, 720)
RESIZES = [(640, 480), (1600, 900), (801, 603), (400, 300), (1024, 1024), START_SIZE]
STORM_SIZES = [(700, 500), (1200, 800), (500, 400), (1300, 700), (900, 900), (600, 650)] * 2
SCENE_SEED = "1234"
SETTLE_S = 2.0


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--bin-dir", required=True, help="Bin/<Config> folder of the build to test")
    parser.add_argument("--out", default=str(TEST_DIR / "results" / "default"), help="where captures are written")
    return parser.parse_args()


def main():
    args = parse_args()

    #the harness picks the build up at import time
    os.environ["YAPT_BIN_DIR"] = str(Path(args.bin_dir).resolve())
    os.environ["YAPT_SCENE_SEED"] = SCENE_SEED
    sys.path.insert(0, str(TEST_DIR.parent / "IntegratorCompare"))

    from yapt_harness import TestHarness, HarnessError, SCRIPTS_DIR

    harness = TestHarness(args.out, *START_SIZE)
    capture_count = [0]

    def check_frame(label):
        capture_count[0] += 1
        width, height = harness.render_size()
        image = harness.capture(f"{capture_count[0]:02d}_{label}_{width}x{height}")
        if image.shape[:2] != (height, width):
            raise HarnessError(f"{label}: frame is {image.shape[1]}x{image.shape[0]}, render area is {width}x{height}")
        if image.max() == 0:
            raise HarnessError(f"{label}: frame is completely black")

    def load():
        harness.load_scene(SCRIPTS_DIR / "examples" / "load_random_shapes_scene.py")
        harness.set_option("RenderPipeline", INTEGRATORS[0])
        print(f"  backend bin dir {os.environ['YAPT_BIN_DIR']}", flush=True)

    def switch(integrator):
        def step():
            harness.set_option("RenderPipeline", integrator)
        return step

    def resize(size, integrator):
        def step():
            harness.resize(*size)
            harness.set_option("RenderPipeline", integrator)
        return step

    def storm():
        for size in STORM_SIZES:
            harness.resize(*size)
            harness.process_events(0.05)

    def checked(name):
        return lambda: check_frame(name)

    steps = [(load, 5.0), (checked("loaded"), 0.5)]

    for integrator in INTEGRATORS[1:] + INTEGRATORS[:1]:
        steps += [(switch(integrator), SETTLE_S), (checked(integrator), 0.5)]

    for i, size in enumerate(RESIZES):
        integrator = INTEGRATORS[i % len(INTEGRATORS)]
        steps += [(resize(size, integrator), SETTLE_S), (checked(integrator), 0.5)]

    steps += [(storm, SETTLE_S), (checked("after_resize_storm"), 0.5)]

    for integrator in INTEGRATORS:
        steps += [(switch(integrator), SETTLE_S), (checked(f"final_{integrator}"), 0.5)]

    try:
        harness.run(steps)
    except HarnessError as error:
        print(f"\n  FAILED: {error}", file=sys.stderr, flush=True)
        return 1

    print(f"\n  PASSED: {capture_count[0]} frames checked", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
