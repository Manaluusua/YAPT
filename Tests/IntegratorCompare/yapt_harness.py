"""Shared plumbing for the integrator comparison tests.

Boots the PySide6 application the same way default_bootstrap.py does, drives it from a
QTimer, and captures the render area.

Capturing needs explaining: the render area is a native HWND with a flip model DXGI
swapchain, and DWM composites that surface directly. Ordinary GDI screen capture
(QScreen.grabWindow) therefore returns whatever is *behind* the window - typically the
desktop wallpaper - and QWidget.grab() returns the flat Qt background, since Qt never
paints the area itself. PrintWindow with PW_RENDERFULLCONTENT is the one path that reads
the composited DirectX content, so that is what capture_render_area() uses.
"""

import ctypes
import ctypes.wintypes as wt
import gc
import hashlib
import os
import sys
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


# ---------------------------------------------------------------- window capture

user32 = ctypes.WinDLL("user32", use_last_error=True)
gdi32 = ctypes.WinDLL("gdi32", use_last_error=True)

PW_RENDERFULLCONTENT = 0x00000002
BI_RGB = 0
DIB_RGB_COLORS = 0


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG),
        ("biPlanes", wt.WORD), ("biBitCount", wt.WORD), ("biCompression", wt.DWORD),
        ("biSizeImage", wt.DWORD), ("biXPelsPerMeter", wt.LONG),
        ("biYPelsPerMeter", wt.LONG), ("biClrUsed", wt.DWORD), ("biClrImportant", wt.DWORD),
    ]


class BITMAPINFO(ctypes.Structure):
    _fields_ = [("bmiHeader", BITMAPINFOHEADER), ("bmiColors", wt.DWORD * 3)]


def capture_window(hwnd):
    """Capture a window's client area as an RGB uint8 array, or None if it has no area."""
    rect = wt.RECT()
    user32.GetClientRect(wt.HWND(hwnd), ctypes.byref(rect))
    width, height = rect.right - rect.left, rect.bottom - rect.top
    if width <= 0 or height <= 0:
        return None

    hdc_window = user32.GetDC(wt.HWND(hwnd))
    hdc_mem = gdi32.CreateCompatibleDC(hdc_window)
    hbitmap = gdi32.CreateCompatibleBitmap(hdc_window, width, height)
    gdi32.SelectObject(hdc_mem, hbitmap)

    user32.PrintWindow(wt.HWND(hwnd), hdc_mem, PW_RENDERFULLCONTENT)

    info = BITMAPINFO()
    info.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    info.bmiHeader.biWidth = width
    info.bmiHeader.biHeight = -height  #negative means top down
    info.bmiHeader.biPlanes = 1
    info.bmiHeader.biBitCount = 32
    info.bmiHeader.biCompression = BI_RGB

    buffer = ctypes.create_string_buffer(width * height * 4)
    gdi32.GetDIBits(hdc_mem, hbitmap, 0, height, buffer, ctypes.byref(info), DIB_RGB_COLORS)

    gdi32.DeleteObject(hbitmap)
    gdi32.DeleteDC(hdc_mem)
    user32.ReleaseDC(wt.HWND(hwnd), hdc_window)

    bgra = np.frombuffer(bytes(buffer), np.uint8).reshape(height, width, 4)
    return np.ascontiguousarray(bgra[:, :, 2::-1])  #BGRA -> RGB


# ---------------------------------------------------------------- test driver

class HarnessError(Exception):
    pass


class TestHarness:
    """Boots the app, runs a list of steps on a timer, and captures frames.

    Every capture is hashed and checked against the ones already taken. Two identical
    captures mean the frame never updated - a covered window, a dead render loop, a broken
    capture path - and the run is not trustworthy, so it is reported loudly rather than
    silently producing plausible looking numbers.
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

    def capture(self, name):
        image = capture_window(int(self._window.get_render_area_widget().winId()))
        if image is None:
            raise HarnessError("render area has no client area to capture")

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
        try:
            if self._step_index >= len(self._steps):
                self._app.get_qt_app().quit()
                return
            action, wait_s = self._steps[self._step_index]
            self._step_index += 1
            action()
            self._timer.setInterval(int(wait_s * 1000))
        except Exception:
            self._failed = True
            traceback.print_exc()
            self._app.get_qt_app().quit()
