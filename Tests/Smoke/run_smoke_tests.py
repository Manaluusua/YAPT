"""Builds YAPT for both graphics backends and runs smoke_test.py against each build.

Each backend gets its own build tree (Build/SmokeTest/<api>), so switching backends does not
rebuild everything and the main Build folder is left alone. The smoke test runs in its own
process per backend; a crash, failed assert, hang (timeout) or failed check is a failure.

    python Tests/Smoke/run_smoke_tests.py                       # build + test DirectX12 and Vulkan
    python Tests/Smoke/run_smoke_tests.py --backends Vulkan     # only one backend
    python Tests/Smoke/run_smoke_tests.py --skip-build          # test the existing builds
    python Tests/Smoke/run_smoke_tests.py --config Release

Captures and the renderer's debug log (OutputDebugString, see dbgview.py) land in
Tests/Smoke/results/<api>/.
"""

import argparse
import os
import subprocess
import sys
import time
from pathlib import Path

TEST_DIR = Path(__file__).resolve().parent
REPO_ROOT = TEST_DIR.parents[1]
DBGVIEW = REPO_ROOT / "Tests" / "IntegratorCompare" / "dbgview.py"
BACKENDS = ["DirectX12", "Vulkan"]
GENERATOR = "Visual Studio 17 2022"


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--backends", nargs="+", choices=BACKENDS, default=BACKENDS)
    parser.add_argument("--config", default="Debug", choices=["Debug", "Release"])
    parser.add_argument("--build-root", default=str(REPO_ROOT / "Build" / "SmokeTest"))
    parser.add_argument("--skip-build", action="store_true", help="test the existing builds as they are")
    parser.add_argument("--timeout", type=float, default=600.0, help="seconds per backend before the test counts as hung")
    return parser.parse_args()


def run(command, **kwargs):
    print(f"> {' '.join(str(c) for c in command)}", flush=True)
    return subprocess.run([str(c) for c in command], **kwargs).returncode


def build(backend, build_dir, config):
    if not (build_dir / "CMakeCache.txt").is_file():
        configure = ["cmake", "-S", REPO_ROOT, "-B", build_dir, "-G", GENERATOR, f"-DGRAPHICS_API={backend}"]
        #reuse the vcpkg checkout of the main build tree instead of cloning it once per backend
        vcpkg_src = REPO_ROOT / "Build" / "_deps" / "vcpkg-src"
        if (vcpkg_src / "bootstrap-vcpkg.bat").is_file():
            configure.append(f"-DFETCHCONTENT_SOURCE_DIR_VCPKG={vcpkg_src.as_posix()}")
        if run(configure) != 0:
            return "configure failed"
    if run(["cmake", "--build", build_dir, "--config", config]) != 0:
        return "build failed"
    return None


def start_debug_log_listener(timeout_s, raw_log_path):
    """Best effort: only one OutputDebugString listener can run at a time (DebugView, a debugger)."""
    try:
        return subprocess.Popen(
            [sys.executable, DBGVIEW, str(timeout_s)],
            stdout=open(raw_log_path, "w", encoding="utf-8", errors="replace"),
            stderr=subprocess.STDOUT,
        )
    except OSError as error:
        print(f"  no debug log: {error}", flush=True)
        return None


def keep_lines_of_process(raw_log_path, log_path, pid):
    prefix = f"[{pid}]"
    try:
        lines = raw_log_path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return []
    lines = [line[len(prefix):].strip() for line in lines if line.startswith(prefix)]
    lines = [line for line in lines if line]
    log_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    raw_log_path.unlink(missing_ok=True)
    return lines


def describe_exit_code(code):
    if code is None:
        return "timed out"
    known = {3: "abort / failed assert", 0x80000003: "breakpoint (YAPT_LOG_FATAL_ERROR or __debugbreak)",
             0xC0000005: "access violation", 0xC0000409: "stack buffer overrun / fast fail",
             0xC00000FD: "stack overflow"}
    unsigned = code & 0xFFFFFFFF
    return f"exit code {code} (0x{unsigned:08X}{', ' + known[unsigned] if unsigned in known else ''})"


def smoke_test(backend, build_dir, config, timeout_s):
    bin_dir = build_dir / "Bin" / config
    out_dir = TEST_DIR / "results" / backend
    out_dir.mkdir(parents=True, exist_ok=True)
    for old_capture in out_dir.glob("*.png"):
        old_capture.unlink()

    raw_log_path = out_dir / "debug_log_raw.txt"
    listener = start_debug_log_listener(timeout_s + 30, raw_log_path)
    time.sleep(0.5)

    command = [sys.executable, TEST_DIR / "smoke_test.py", "--bin-dir", bin_dir, "--out", out_dir]
    print(f"> {' '.join(str(c) for c in command)}", flush=True)
    process = subprocess.Popen([str(c) for c in command])
    try:
        code = process.wait(timeout=timeout_s)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()
        code = None

    if listener is not None:
        time.sleep(0.5)
        listener.terminate()
        listener.wait()
    log_lines = keep_lines_of_process(raw_log_path, out_dir / "debug_log.txt", process.pid)

    if code == 0:
        return None
    if log_lines:
        print(f"  last renderer log lines ({out_dir / 'debug_log.txt'}):")
        for line in log_lines[-25:]:
            print(f"    {line}")
    return describe_exit_code(code)


def main():
    args = parse_args()
    build_root = Path(args.build_root).resolve()
    results = {}

    for backend in args.backends:
        print(f"\n==== {backend} ({args.config}) ====", flush=True)
        build_dir = build_root / backend
        error = None if args.skip_build else build(backend, build_dir, args.config)
        if error is None:
            error = smoke_test(backend, build_dir, args.config, args.timeout)
        results[backend] = error

    print("\n==== smoke test summary ====")
    for backend, error in results.items():
        print(f"  {backend:10s} {'PASSED' if error is None else 'FAILED: ' + error}")
    return 0 if all(error is None for error in results.values()) else 1


if __name__ == "__main__":
    sys.exit(main())
