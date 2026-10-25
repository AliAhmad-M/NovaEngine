# Nova Engine

Nova Engine is a work-in-progress game engine written in C++ that aims to support both 2D and 3D game development, with functional 2D capabilities and 3D development planned as part of its ongoing evolution. The engine includes a custom Entity Component System (ECS), an OpenGL-based graphics subsystem, physics integration, and a graphical editor built with Dear ImGui. The 2D functionality along with the editor is operational, but the engine is not yet a stable, plug-and-play release and requires manual setup and configuration.

## Features

- **ECS:** Entity Component System implemented using paginated sparse sets and system bitset signatures.
- **2D Rendering:** Rendering support and animation for 2D game objects and sprites.
- **2D Physics:** Box2D integration for physics simulation and collision handling.
- **Tilemaps:** Tilemaps created with [Tiled](https://www.mapeditor.org/) can be imported into Nova.
- **Camera:** 2D camera support with bounds.
- **Scene Management:** Create, save, load, and manage game scenes.
- **Entity and Component Editing:** Create entities, attach or remove components, and edit component properties through the inspector.
- **Asset Management:** Import and manage game assets.
- **C++ Game Logic:** Implement game logic in C++ using custom behavior scripts that get compiled into the game executable.
- **Input Handling:** Configure input mappings for game controls.
- **Project Management:** Create projects, open existing projects, and access recent projects.
- **Build and Run:** Build and launch game projects through the editor.

Some editor workflows and engine systems remain incomplete or require manual configuration.

## Architecture

Nova is organized into several major subsystems:

| Module | Responsibility |
|---|---|
| `Core` | ECS, mathematics utilities, helpers, and scripting logic |
| `Editor` | Graphical editor application and project creation workflow |
| `Graphics` | Rendering and graphics functionality |
| `Physics` | Physics integration and simulation |
| `Platform` | Windowing and input handling |
| `Runtime` | Main loops and runtime execution |
| `World` | Game world-related functionality, such as tilemaps |

The engine uses a custom ECS implementation based on paginated sparse sets and system bitset signatures to manage entities, components, and systems.

## Technology Stack

| Technology | Purpose |
|---|---|
| C++ | Engine and game logic |
| OpenGL | Graphics rendering |
| Dear ImGui | Editor interface |
| Box2D | 2D physics |
| CMake | Game project build system |

## Current Status

Nova Engine is under active development. Its 2D functionality supports creating and running games through the editor, including scene editing, component configuration, physics, camera functionality, and imported tilemaps. The broader goal is to develop a more complete 2D and 3D game engine. **3D functionality remains a work in progress** and should not be considered available in a stable form. The current project is primarily intended for experimentation, learning, and continued development of engine architecture. Getting it running on another machine may require additional manual configuration.
