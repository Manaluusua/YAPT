# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

YAPT ("Yet Another Path Tracer") is a C++17 real-time/offline GPU path tracer with a bidirectional path tracing (BDPT) integrator, supporting both DirectX 12 and Vulkan backends behind a common abstraction layer. The C++ core is driven and inspected through a Python/Qt front-end via pybind11 bindings — `main.cpp` itself does almost nothing except boot an embedded Python interpreter and hand off to a bootstrap script.

## Build

Windows + MSVC only (the CMake build hard-errors with `message(FATAL_ERROR "not implemented")` for non-MSVC compilers).

```
cmake -S . -B Build -G "Visual Studio 17 2022"
cmake --build Build --config Debug
```

- vcpkg is auto-bootstrapped via `FetchContent` if not already on `PATH`; it provides Vulkan/Ktx/etc. dependencies. First configure can take a while.
- Graphics backend is chosen via the `GRAPHICS_API` cache variable in `Source/Gfx/CMakeLists.txt`: `DirectX12` (default) or `Vulkan`. Set with `-DGRAPHICS_API=Vulkan` at configure time; this defines `RENDERER_DX12`/`RENDERER_VK` for the whole `Gfx` target.
- `Source/Python/CMakeLists.txt` requires Python3 + pip and will `pip install` `pybind11`, `pyside6`, `pygltflib`, `pyktx` into the environment during configure.
- All binaries/DLLs land in `Build/Bin/<Config>/`; shaders are copied to `Build/Bin/<Config>/shaders/` and Python scripts run from `Source/Python/pythonScripts/` (see `Source/Python/pythonScripts/.vscode/launch.json` for a debug entry point).
- There are no unit tests or lint targets in this repo currently.

### Running

The `YAPT` executable loads `Source/Python/pythonScripts/default_bootstrap.py` (falling back to a relative path from `Build/Bin/<Config>/`), which launches a PySide6 `QApplication` (`yapt.app.Application`). Example scenes to load from the Python console/scripts live in `Source/Python/pythonScripts/examples/`.

## Architecture

### Module dependency graph

```
Common  →  Gfx  →  Renderer  →  Scene ─┐
                                AssetLoader ─┤
                                             Python (embed) → py_yapt (pybind11 module) → pythonScripts (PySide6 GUI)
```

Each module is its own CMake target/subdirectory under `Source/` with its own `CMakeLists.txt`, public headers under `inc/<Module>/`, and implementation under `src/`:

- **Common** — static lib. Allocators (`ArenaAllocator`, `ArrayIndexAllocator`, `RangeAllocator`, ...), containers (`TightlyPackedArray`, `BubbleArray`, `StructureOfArrays`, `RingBuffer`), threading (`ThreadPool`, `JobSystem`), the intrusive refcounting base (`RCObject`/`RCObjectPtr`), logging (`Logger.h`), and `Math/` (GLM-based vector/matrix types, BRDF math, Sobol sampling tables). No dependency on any other YAPT module.
- **Gfx** — static lib. The graphics-API abstraction layer: a handle-based C-style API (`GfxApi.h`, opaque `*Handle` types) plus a `RenderGraph` (nodes for compute/raytrace/swapchain/custom passes) built on top of it. Backend implementations live in parallel `Dx12/` and `Vk/` subfolders (headers in `inc/Gfx/Dx12`, `inc/Gfx/Vk`; sources in `src/Dx12`, `src/Vk`) and only one is compiled in per `GRAPHICS_API` setting.
- **Renderer** (shared/dynamic lib) — the actual path tracer built on `Gfx`. Public interface in `inc/Renderer/*.h` (`Renderer`, `Mesh`, `Texture`, `Material`, `RenderObject`); implementation under `Shared/` (backend-agnostic, e.g. `CRenderer`, `MeshManager`, `MaterialManager`, `RenderObjectManager`, bindless resource managers, `LightManager`). Render passes are composed as a `RenderPipeline` of `RenderStage`s (see `Shared/RenderPipeline/`), with the main integrator in `Shared/RenderPipeline/PathTracer/` (`PathTracerPipeline`, forward/backward path integrator substages, BDPT). HLSL shaders live in `Renderer/shaders/` (`raytracing/`, `postprocess/`, `materials/`, `common/`), compiled via DXC; pipeline layouts/shader groupings are declared in the `*PipelineDefinitions.json` files in that folder.
- **Scene** (shared/dynamic lib) — scene graph: `Scene`, `SceneObject`/`RenderableObject`, `Transform`, `Camera`. Depends on `Renderer` (renderable objects wrap `Renderer` resources).
- **AssetLoader** (shared/dynamic lib) — glTF (via `tinygltf`) and texture (via `Ktx`) loading, plus a default disk-based `RendererCacheProvider` implementation. Depends on `Renderer`.
- **Python** — an embedded-Python host module (`Python` target) plus a separate pybind11 extension module `py_yapt` (`incBindings`/`srcBindings`) that exposes `Renderer`, `Scene`, `AssetLoader` etc. to Python. `Source/Python/pythonScripts/yapt/` is the actual application (Qt main window, camera controller, material/inspector views); `default_bootstrap.py` is the entry point invoked by `main.cpp`.

### Public interface / implementation split

Nearly every module follows the same pattern: an abstract interface class with no prefix (`Renderer`, `Scene`, `AssetLoader`, `Mesh`, `Texture`, ...) declares the public API and lives in the module's top-level `inc/<Module>/` folder. A concrete implementation class prefixed with `C` (`CRenderer`, `CScene`, `CAssetLoader`, ...) lives under `inc/<Module>/Shared/` or `inc/<Module>/Impl/` + `src/`, inherits the interface, and marks its overrides `final`. Interfaces expose module-scoped `createX()`/`destroyX()` factory functions (not `new`/`delete`) annotated with a per-module `<MODULE>_MODULE_INTERFACE` macro (`DLL_EXPORT` when building that module, `DLL_IMPORT` for consumers — see each module's `CommonDefines.h`). Destructors are typically `protected` with the `destroyX()` free function declared as a `friend`, so callers cannot `delete` the interface pointer directly.

GPU/engine objects that need shared ownership derive from `Common/RCObject.h` (intrusive refcount, `AddRef`/`Release`) and are held via `RCObjectPtr<T>` (aliased `RCPtr<T>`), not `std::shared_ptr`.

`Gfx`'s own API (`GfxApi.h`) is a free-function, opaque-handle C-style API rather than an object interface (`createTexture(GfxApiHandle h, ...)`, `destroyTexture(GfxApiHandle h, TextureHandle)`, etc.) — this is intentional since it must be trivially bridgeable across the Dx12/Vk backend split.

## Coding conventions

- Everything lives in namespace `YAPT` (nested `YAPT::Gfx` inside the `Gfx` module).
- Indentation is **tabs**, brace style is **Allman** (opening brace on its own line).
- Types are `PascalCase`; functions and methods are `camelCase`; private/protected member variables are prefixed `m_` (e.g. `m_frameIndex`, `m_renderPipelineMngr`).
- Headers use `#pragma once`, except the per-module `CommonDefines.h` files which use classic `#ifndef YAPT_<MODULE>_COMMONDEFINES_H` include guards.
- `.inl` files are used for template/inline definitions split out of a header (e.g. `Camera.inl`, `Transform.inl`, `JobSystem.inl`).
- Logging goes through the `YAPT_LOG_DEBUG/WARNING/ERROR/FATAL_ERROR(fmt, ...)` printf-style macros in `Common/Logger.h`, not `std::cout`/`printf` directly.
- CMake source lists (`HEADERS`/`SOURCE`) are maintained by hand per target and fed through `generateVSFilters()` (from `CMakeCommon.txt`) to mirror the folder layout as Visual Studio filters — new files must be added to the relevant `CMakeLists.txt` explicitly (no globbing), except shaders and Python scripts which are globbed for IDE visibility only.
- Python code (`pythonScripts/yapt/`) uses `snake_case` functions/methods and a leading-underscore convention for "private" instance attributes (e.g. `self._renderer`, `self._scene`), following typical PySide6/Qt-application style rather than the C++ side's conventions.
