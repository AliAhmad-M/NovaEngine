# Nova Engine

A C++ game engine with a working 2D toolset and an editor. 3D support is planned.

> Work in progress.

## Architecture

| Module | Responsibility |
|---|---|
| `Core` | ECS, math utilities, helpers, scripting |
| `Editor` | Editor application and project creation |
| `Graphics` | Rendering |
| `Physics` | Physics integration and simulation |
| `Platform` | Windowing and input |
| `Runtime` | Main loops and runtime execution |
| `World` | World systems, such as tilemaps |

## Technology

| Technology | Purpose |
|---|---|
| C++ | Engine and game logic |
| OpenGL | Rendering |
| Dear ImGui | Editor UI |
| Box2D | 2D physics |
| Jolt | 3D physics |
| CMake | Game project builds |

## Roadmap

- 3D rendering and physics
- Finish the editor workflows that still need manual setup
