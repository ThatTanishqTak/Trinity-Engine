# Trinity Engine

Trinity Engine is a C++23 real‑time rendering and tooling project built around two main targets:

- **TrinityEngine** is a static engine library that provides the core application lifecycle, an ECS scene model, asset loading, event and input handling, and an abstraction layer over the renderer【810741037388778†L15-L20】.
- **TrinityForge** is an editor‑style executable that links the engine and exposes a docked ImGui workflow for scene viewing and basic entity/component editing【810741037388778†L15-L20】.

Trinity is not intended to be a turnkey game engine; rather, it serves as a foundation for experiments in graphics and tooling.  It emphasises modularity and tries to stay close to modern C++ practices, with custom layers for application logic, an EnTT‑based ECS, Vulkan rendering back‑end, YAML serialization and an asset system built around UUIDs.

## Current state

At the time of writing, the engine is functional on Windows using MSVC and Ninja.  Key features include:

- A layered application architecture (`Application`, `Layer`, `LayerStack`) for modular updates and rendering【810741037388778†L15-L20】.
- An ECS scene model with `Scene` and `Entity` types, common components (transform, camera, mesh renderer, etc.), and YAML‑based serialization/deserialization.
- A Vulkan rendering pipeline with runtime shader compilation (`glslc`), G‑buffer based geometry pass, lighting pass, and basic shadow support.  Additional back‑ends (MoltenVK, DirectX) have scaffolding but are not yet implemented.
- Platform/window/input/event systems built on SDL3 and an event queue.
- An integrated editor (TrinityForge) that demonstrates a dockspace layout, viewport rendering, an entity hierarchy panel and basic properties editing for tag/transform/mesh renderer data【810741037388778†L69-L78】.
- Built‑in primitive meshes and a starter scene to verify that the pipeline works end to end.

## Roadmap

Work is ongoing to broaden Trinity’s capabilities.  Areas under active development or planned for the future include:

- **Cross‑platform support** beyond Windows.  Linux and macOS builds via Clang/GCC are being investigated.
- **Additional renderer back‑ends** such as DirectX 12 on Windows and Metal via MoltenVK on macOS.
- **Editor improvements**, including prefab support, asset browser panels, undo/redo, scene persistence and expanded component editors.
- **More rendering features** (PBR shading, post‑processing, multiple shadow cascades) and a more flexible frame graph system.
- **Audio and physics** integrations to support richer scenes.
- **Scripting** support to allow gameplay logic in a higher‑level language.

These goals reflect the direction the project is heading in; contributions and feedback are welcome.

## Build requirements

The project targets **MSVC‑based Windows development** and uses CMake and Ninja to drive the build【810741037388778†L7-L8】.  To build the engine on Windows you will need:

1. **Visual Studio 2022** with the MSVC C++ toolset【810741037388778†L44-L52】.
2. **CMake 3.30** or newer【810741037388778†L31-L33】.
3. **Ninja** (or another compatible generator)【810741037388778†L44-L52】.
4. **Vulkan SDK** installed and available to CMake, including the `glslc` shader compiler【810741037388778†L44-L52】.
5. A **Git** client with submodule support; the engine vendors are pulled in as submodules【810741037388778†L44-L53】.

Linux and macOS builds are not yet officially supported, but the codebase uses SDL and CMake throughout, so cross‑platform support is a future goal.

### Getting started

Clone the repository and initialize submodules:

```bash
git clone --recurse-submodules <repo-url>
cd Trinity-Engine
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

On a successful build, binaries are emitted under `bin/<Config>-<System>-<Arch>/Trinity/`【810741037388778†L63-L67】.  The `TrinityForge` binary is the runnable editor target【810741037388778†L63-L67】.

Once the editor launches, you will see a dockspace layout with a scene viewport, entity hierarchy, and property panels.  Loading and saving scenes is currently manual; the initial startup scene loads a few primitive meshes【810741037388778†L69-L78】.

## License

Trinity Engine is licensed under the **Apache License 2.0**.  See the [LICENSE](LICENSE) file for the full license text【246190959861404†L65-L71】.
