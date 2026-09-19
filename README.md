# Trinity Engine

A **C++23 game engine and scene editor** built around Vulkan rendering, modular engine systems and an entity/component scene model.

The main application is **Trinity-Forge**, an editor for assembling scenes, inspecting entities, adjusting materials and running simulations.

**Status:** Active development. The engine and Forge contain the main implementation. Several additional product targets are still application skeletons or generated placeholders.

## Current Features

### Rendering

* Vulkan rendering backend with a graphics abstraction layer.
* Slang shader compilation.
* Physically based material shading.
* Environment lighting with irradiance, prefiltered reflections and a BRDF lookup texture.
* Directional shadow rendering.
* A render graph for organising passes and resource transitions.
* Skybox rendering, tone mapping and depth visualisation.
* Debug geometry and rendering statistics.

### Scenes and Runtime Systems

* EnTT-based entity/component organisation.
* Entity hierarchy and transform management.
* Mesh importing through Assimp.
* Asset metadata and an asset database.
* YAML scene and material serialisation.
* Audio integration through miniaudio.
* Box2D-backed 2D physics.
* Fixed-step simulation controls and physics debugging.

### Forge Editor

* Scene viewport with transform gizmos.
* Hierarchy, inspector and content-browser panels.
* Console, render-graph and statistics panels.
* Command-based undo and redo.
* Entity creation, duplication and deletion.
* Scene opening and saving.
* Play, pause and single-step controls.

## Product Status

| Target                             | Current state                              |
| ---------------------------------- | ------------------------------------------ |
| `Trinity-Engine`                   | Core static engine library                 |
| `Trinity-Forge`                    | Implemented scene editor                   |
| `Trinity-Runtime`                  | Basic application skeleton                 |
| `Trinity-Server`                   | Generated placeholder                      |
| `Trinity-Hub`                      | Generated placeholder                      |
| Build, cooking and packaging tools | Primarily generated placeholders           |
| `Trinity-PhysicsSmoke`             | Implemented physics smoke-check executable |

A complete project creation, cooking, packaging and standalone-player workflow is still being developed.

## Requirements

* CMake 3.30 or newer.
* A C++23 compiler.
* Vulkan SDK with Slang headers, libraries and runtime components.
* A compatible Vulkan GPU and driver.
* Git with submodule support.

Windows builds can use Visual Studio 2022.

## Build and Run Forge

```powershell
git clone --recurse-submodules https://github.com/ThatTanishqTak/Trinity-Engine.git
cd Trinity-Engine

cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target Trinity-Forge

cd bin/Release-Windows-x86_64/Trinity/Trinity-Forge
.\Trinity-Forge.exe
```

Keep the staged assets, shaders and runtime libraries beside the executable.

Build outputs follow:

```text
bin/<Configuration>-<System>-<Architecture>/Trinity/<Target>/
```

## Editor Workflow

1. Select an entity in the hierarchy.
2. Edit its components in the inspector.
3. Position it using the viewport gizmos.
4. Use the content browser to inspect available assets.
5. Save the scene.
6. Use Play, Pause and Step to inspect runtime behaviour.

Common shortcuts include `Ctrl+S` for saving, `Ctrl+O` for opening a scene, and `Ctrl+Z` / `Ctrl+Y` for undo and redo.

## Repository Layout

* `Trinity-Engine/`: Engine systems, shaders and shared assets.
* `Trinity-Forge/`: Editor application and panels.
* `Trinity-Runtime/`: Runtime application foundation.
* `Trinity-Server/` and `Trinity-Hub/`: Future product targets.
* `Trinity-Tools/`: Tool targets and physics checks.
* `cmake/`: Shared target configuration.
* `Vendor/`: Third-party submodules.

## Current Boundaries

* Vulkan is the implemented graphics backend.
* Metal and DirectX 12 options do not represent completed backends.
* Box2D is the implemented physics backend; the planned PhysX integration is incomplete.
* Some editor menu entries are disabled placeholders.
* Project management, cooking and distribution workflows remain unfinished.

## License

Licensed under the [Apache License 2.0](LICENSE). Third-party dependencies retain their respective licences.
