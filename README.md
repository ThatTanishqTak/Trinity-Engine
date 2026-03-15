# Trinity Engine

Trinity is a C++23 real-time rendering and tooling project built around two targets:

- **TrinityEngine**: a static engine library that provides application lifecycle, ECS, asset loading, event/input handling, and a renderer abstraction.
- **TrinityForge**: an editor-style executable that links the engine and provides a docked ImGui workflow for scene viewing and basic entity/component editing.

The project is configured for **MSVC-based Windows development** with CMake and Ninja.

## What this project is

At a high level, Trinity is a foundation for building graphics applications and editor tooling.

It includes:

- A layered application architecture (`Application`, `Layer`, `LayerStack`).
- An ECS scene model (`Scene`, `Entity`, and common components).
- Renderer backends and infrastructure (Vulkan path with shader compilation, plus additional backend scaffolding).
- Platform/window/input/event systems.
- Serialization and asset systems for scene and mesh workflows.
- An integrated editor app (`TrinityForge`) used as the primary sandbox for engine development.

## Repository layout

- `Trinity-Engine/` — core engine source and third-party dependencies.
- `Trinity-Forge/` — editor application that consumes the engine.
- `CMakeLists.txt` — root build orchestration.
- `CMakeSettings.json` — Visual Studio CMake presets using MSVC x64 environments.

## Core technologies

- **Language standard**: C++23
- **Build system**: CMake 3.30+
- **Primary toolchain**: MSVC (x64)
- **Windowing/input**: SDL3
- **Rendering**: Vulkan (with runtime shader compilation via `glslc`)
- **UI**: ImGui
- **ECS**: EnTT
- **Math**: GLM
- **Logging**: spdlog
- **Serialization**: yaml-cpp
- **Asset import**: assimp
- **GPU allocation**: VulkanMemoryAllocator

## Build requirements

To build on Windows:

1. Visual Studio 2022 with MSVC C++ toolset.
2. CMake 3.30 or newer.
3. Ninja (or a compatible generator).
4. Vulkan SDK installed and available to CMake (`find_package(Vulkan REQUIRED)`), including `glslc`.
5. Git submodules initialized (engine vendors are expected as subdirectories).

## Getting started (MSVC + CMake)

```bash
git clone --recurse-submodules <repo-url>
cd Trinity-Engine
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

On successful build, binaries are emitted under:

- `bin/<Config>-<System>-<Arch>/Trinity/`

`TrinityForge` is the runnable editor target.

## Current editor workflow in TrinityForge

The Forge app currently demonstrates:

- Dockspace-based editor layout.
- Scene viewport rendering.
- Entity hierarchy panel.
- Basic properties editing for tag/transform/mesh renderer data.
- Sample startup scene with primitive meshes.

## Notes

- Shader assets are compiled to SPIR-V during build and copied to output directories.
- The root project enforces C++23 and applies MSVC compile parallelization (`/MP`) when using Visual Studio toolchains.
