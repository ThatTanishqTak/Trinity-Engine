# Trinity

Trinity is a custom C++23 game engine ecosystem: a modular runtime engine, a
professional editor (**Forge**), a standalone runtime player, a dedicated
headless server, and a project/version launcher (**Trinity-Hub**).

The project is built through working vertical slices rather than upfront
abstraction. The first objective is a complete Engine → Forge → Runtime loop:
create a project, edit a scene, render it through Vulkan, save and reload it,
cook the project, and launch it in Trinity-Runtime.

## Products

| Target          | Description                                      |
|-----------------|--------------------------------------------------|
| Trinity-Engine  | Core C++ engine library used by all products     |
| Trinity-Forge   | Editor                                           |
| Trinity-Runtime | Standalone player for cooked builds              |
| Trinity-Server  | Dedicated headless multiplayer runtime           |
| Trinity-Hub     | Launcher, project and engine-version manager     |
| Trinity-Tools   | BuildTool, Cooker, ShaderCompiler, PackageTool, ProjectGenerator |

## Prerequisites

- CMake 3.30 or newer
- A C++23 compiler (Visual Studio 2022 / MSVC on Windows)

Additional dependencies (Vulkan SDK, SDL3, and others) are introduced in later
milestones as they are needed.

## Building

```
cmake -S . -B out/build
cmake --build out/build --config Debug
```

Build output is written to:

```
bin/<Config>-<System>-<Arch>/<Project>/<Target>/
```

## Repository Layout

```
Trinity-Engine/    Core engine library (Source/, Include/)
Trinity-Forge/     Editor
Trinity-Runtime/   Standalone player
Trinity-Server/    Headless server
Trinity-Hub/       Launcher
Trinity-Tools/     Command-line and build pipeline tools
Templates/         Project templates
Examples/          Example projects
Docs/              Documentation (see Docs/Architecture)
Vendor/            Third-party dependencies (git submodules)
Build/             Build support scripts
cmake/             Shared CMake modules
```

## Documentation

Architecture rules and module boundaries live in `Docs/Architecture/`.

## License

Apache License 2.0. See `LICENSE`.