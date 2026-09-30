"""Shared plumbing for the integrator comparison tests.

Boots the PySide6 application the same way default_bootstrap.py does, drives it from a
QTimer, and captures frames through the renderer's readback of the final color target
(the same image the swapchain presents, at render resolution).
"""

import gc
import hashlib
import os
import sys
import time
import traceback
from pathlib import Path

import numpy as np
from PIL import Image


# ---------------------------------------------------------------- module locations

def find_repo_root():
    """Walk up from this file (or use YAPT_ROOT) until the python scripts folder shows up."""
    env_root = os.environ.get("YAPT_ROOT")
    candidates = [Path(env_root)] if env_root else []
    candidates += list(Path(__file__).resolve().parents)

    for candidate in candidates:
        if (candidate / "Source/Python/pythonScripts/yapt/app.py").is_file():
            return candidate

    raise RuntimeError(
        "could not locate the YAPT repository root. Set YAPT_ROOT to the folder that "
        "contains Source/Python/pythonScripts."
    )


REPO_ROOT = find_repo_root()
CONFIG = os.environ.get("YAPT_CONFIG", "Debug")
BIN_DIR = REPO_ROOT / "Build" / "Bin" / CONFIG
SCRIPTS_DIR = REPO_ROOT / "Source" / "Python" / "pythonScripts"
TEST_DIR = Path(__file__).resolve().parent


def setup_import_paths():
    """Make py_yapt and the yapt package importable regardless of the working directory."""
    if not (BIN_DIR / "py_yapt.cp313-win_amd64.pyd").is_file():
        matches = list(BIN_DIR.glob("py_yapt*.pyd"))
        if not matches:
            raise RuntimeError(
                f"no py_yapt module in {BIN_DIR}. Build the {CONFIG} configuration first, "
                f"or set YAPT_CONFIG."
            )

    for path in (BIN_DIR, SCRIPTS_DIR, SCRIPTS_DIR / "yapt", TEST_DIR):
        path_str = str(path)
        if path_str not in sys.path:
            sys.path.insert(0, path_str)

    #the dependent dlls (Renderer.dll and friends) sit next to the pyd
    if hasattr(os, "add_dll_directory"):
        os.add_dll_directory(str(BIN_DIR))


setup_import_paths()


# ---------------------------------------------------------------- frame capture

from py_yapt import ReadbackTarget, ReadbackState
from yapt.screenshot_writer import FORMAT_LAYOUTS, _read_pixels


def readback_to_rgb(data):
    """Unpack a ready FinalColor readback into an RGB uint8 array.

    FinalColor is display encoded 8 bit (RGBA8_SRGB), so the bytes are used as is. Float
    formats are clamped to [0, 1] the same way screenshot_writer does."""
    layout = FORMAT_LAYOUTS.get(data.getFormat())
    if layout is None:
        raise HarnessError(f"no conversion for readback format {data.getFormat()}")
    _, _, order, encoding = layout

    pixels = _read_pixels(data, layout)
    if encoding == "unorm16":
        pixels = (pixels >> 8).astype(np.uint8)
    elif encoding == "float":
        pixels = np.rint(np.clip(pixels.astype(np.float32), 0.0, 1.0) * 255.0).astype(np.uint8)

    if len(order) == 1:
        return np.ascontiguousarray(np.repeat(pixels, 3, axis=2))
    return np.ascontiguousarray(pixels[:, :, [order.index(c) for c in "RGB"]])


# ---------------------------------------------------------------- test driver

class HarnessError(Exception):
    pass


READBACK_TIMEOUT_S = 10.0


class TestHarness:
    """Boots the app, runs a list of steps on a timer, and captures frames.

    Every capture is hashed and checked against the ones already taken. Two identical
    captures mean the frame never updated - a dead render loop, accumulation that stopped -
    and the run is not trustworthy, so it is reported loudly rather than silently producing
    plausible looking numbers.
    """

    def __init__(self, output_dir, width=900, height=700):
        from PySide6.QtCore import QTimer
        from yapt.app import Application

        self._output_dir = Path(output_dir).resolve()
        self._output_dir.mkdir(parents=True, exist_ok=True)

        #the renderer resolves its shaders, LUTs and cache relative to the working
        #directory, so it has to be Build/Bin the same way default_bootstrap.py runs it.
        #output paths are resolved to absolute above, so this does not affect them.
        os.chdir(BIN_DIR.parent)

        self._app = Application()
        self._window = self._app._main_window
        self._window.resize(width, height)
        self._window.show()
        self._window.raise_()
        self._window.activateWindow()

        self._renderer = self._app.get_renderer()
        self._hashes = {}
        self._stale = []
        self._steps = []
        self._step_index = 0
        self._failed = False
        self._timer = QTimer()
        self._timer.timeout.connect(self._tick)

    @property
    def renderer(self):
        return self._renderer

    @property
    def output_dir(self):
        return self._output_dir

    def load_scene(self, scene_file, replacements=None):
        """Exec a scene script the way the app's Run Script menu does."""
        source = Path(scene_file).read_text(encoding="utf-8")
        for old, new in (replacements or {}).items():
            source = source.replace(old, new)
        context = {"yapt_instance": self._app}
        exec(source, context, context)
        return context

    def set_pipeline(self, index):
        """0 = BPT, 1 = BPT_WITH_NEE, 2 = BDPT, 3.. = debug pipelines."""
        self._renderer.getRendererVariable("RenderPipeline").setSelectedOption(index)

    # ------------------------------------------------ renderer variables

    def _variable(self, name):
        var = self._renderer.getRendererVariable(name)
        if var is None:
            raise HarnessError(f"no renderer variable {name}")
        return var

    def set_option(self, name, option):
        """Select an options renderer variable by its label, e.g. set_option("Denoise.Mode", "OFF")."""
        var = self._variable(name)
        options = list(var.getOptions())
        if option not in options:
            raise HarnessError(f"{name} has no option {option}, options are {options}")
        var.setSelectedOption(options.index(option))

    def set_float(self, name, *values):
        self._variable(name).setFromFloatArray([float(v) for v in values])

    def set_int(self, name, *values):
        self._variable(name).setFromIntArray([int(v) for v in values])

    def set_output(self, denoise=True, tonemap=True, exposure=None, per_sample=False):
        """Pick what the captured frame shows.

        denoise     False shows the accumulated result without the denoiser
        tonemap     False skips the tonemapping operator (exposure is still applied)
        exposure    None keeps auto exposure, a number switches to that manual exposure
        per_sample  True shows only the latest frame's samples (no accumulation, no denoiser)

        set_output(denoise=False, tonemap=False, exposure=1.0) gives the plain accumulated
        radiance, clamped to [0, 1] by the 8 bit final color target."""
        if per_sample:
            self.set_option("Denoise.Mode", "DEBUG_SHOW_SAMPLE_SOURCES")
        else:
            self.set_option("Denoise.Mode", "ON" if denoise else "OFF")
        self.set_option("Tonemap.Enable", "ON" if tonemap else "OFF")
        if exposure is None:
            self.set_option("Tonemap.UseAutoExposure", "ON")
        else:
            self.set_option("Tonemap.UseAutoExposure", "OFF")
            self.set_float("Tonemap.ManualExposure", exposure)

    # ------------------------------------------------ capturing

    def _wait_for_readback(self, readback):
        """Pump the Qt event loop (which drives the render loop) until the readback is done."""
        qt_app = self._app.get_qt_app()
        start = time.monotonic()
        while True:
            state = readback.getState()
            if state == ReadbackState.Ready:
                return
            if state in (ReadbackState.Failed, ReadbackState.Freed):
                raise HarnessError(f"readback ended in state {state}")
            if time.monotonic() - start > READBACK_TIMEOUT_S:
                raise HarnessError(f"readback not ready after {READBACK_TIMEOUT_S:g}s (state {state})")
            qt_app.processEvents()
            time.sleep(0.002)

    def grab_frame(self):
        """Read back the next rendered frame as an RGB uint8 array."""
        readback = self._renderer.readback(ReadbackTarget.FinalColor)
        try:
            self._wait_for_readback(readback)
            data = readback.getData()
            if data is None:
                raise HarnessError("readback is ready but has no data")
            return readback_to_rgb(data)
        finally:
            #the readback references renderer owned resources and has to go before the renderer does
            readback.release()

    def capture(self, name):
        image = self.grab_frame()

        Image.fromarray(image).save(self._output_dir / f"{name}.png")
        digest = hashlib.md5(image.tobytes()).hexdigest()[:10]
        duplicate = next((k for k, v in self._hashes.items() if v == digest), None)
        self._hashes[name] = digest

        note = ""
        if duplicate is not None:
            self._stale.append((name, duplicate))
            note = f"   !! IDENTICAL TO {duplicate} - frame never updated"
        print(f"  captured {name:10s} mean={image.mean():7.3f} std={image.std():6.2f} "
              f"md5={digest}{note}", flush=True)
        return image

    def run(self, steps, first_delay_s=3):
        """steps is a list of (callable, seconds_to_wait_before_the_next_step)."""
        self._steps = steps
        self._timer.start(int(first_delay_s * 1000))
        self._app.execute()

        #any surviving python reference to a renderer owned object makes shutdown hang,
        #so drop the steps (they close over scene objects) and collect before shutting down
        self._steps = []
        gc.collect()
        self._app.shutdown()

        if self._failed:
            raise HarnessError("harness step raised, see traceback above")
        if self._stale:
            pairs = ", ".join(f"{a}=={b}" for a, b in self._stale)
            raise HarnessError(f"stale captures ({pairs}); results are not trustworthy")

    def _tick(self):
        #captures pump the event loop while waiting for the readback, so keep this timer from
        #firing again in the middle of a step
        self._timer.stop()
        try:
            if self._step_index >= len(self._steps):
                self._app.get_qt_app().quit()
                return
            action, wait_s = self._steps[self._step_index]
            self._step_index += 1
            action()
            self._timer.start(int(wait_s * 1000))
        except Exception:
            self._failed = True
            traceback.print_exc()
            self._app.get_qt_app().quit()
