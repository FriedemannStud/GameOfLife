### **ADR-0018: Multicam Render Context Architecture**

**Status:** Proposed

**Date:** 2026-05-27

#### **1. Context and Problem Statement**

The current C-Application was originally designed as a single-player, full-screen experience. Because of this, the core rendering logic (`src/gui/renderer.c`) and simulation logic (`src/core/game_logic.c`) heavily rely on global and static variables. For instance, there is a single global grid buffer, a single global GPU texture (`gridTex`), and a single shader instance. 

As part of the Uni-Messe Kiosk Mode, we must implement "Modul 2: Wusel-Multicam" (Observer Engagement). This feature requires the application to simulate and render multiple independent "Greatest Hits" matches simultaneously on the same screen, arranged in a grid layout (e.g., a 2x2 split-screen). Due to the hardcoded global states, it is currently impossible to instantiate multiple distinct simulation worlds and render them concurrently in different viewports. 

#### **2. Decision**

We will systematically refactor the C-Application to eliminate global simulation and rendering states, adopting an object-oriented approach in C.

*   **Render Context:** We will introduce a `RenderContext` struct in `renderer.h`. This struct will encapsulate all data required to render a single viewport, including its specific GPU texture, target screen rectangle (bounds), camera position, and specific shader uniform locations.
*   **Simulation Context:** We will introduce a `SimulationContext` (or expand the existing `World` struct) in `core_types.h`/`game_logic.h` to hold all logical data for a single match instance, including the grid state (double buffers), generation counter, and current rules/configuration.
*   **Function Signatures:** All functions in `renderer.c` (e.g., `draw_grid`, `update_gpu_texture`) and `game_logic.c` (e.g., `update_generation`) will be modified to accept pointers to these context structs (e.g., `void draw_grid(RenderContext* ctx, SimulationContext* sim)`).
*   **Multicam Manager:** We will introduce a controller that allocates arrays of these contexts to manage the 2x2 Kiosk grid.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

*   **Multicam Capability:** Directly satisfies the requirement to render multiple concurrent simulations independently, allowing the "Wusel-Multicam" mode to function.
*   **Testability:** By removing global states, the simulation and rendering functions become pure (or purely dependent on their inputs). This vastly improves unit testability, as isolated contexts can be created for tests without cross-contamination.
*   **Architectural Cleanliness:** Aligns with Clean Architecture principles and modern C best practices by explicitly defining data dependencies.
*   **Independent Lifecycles:** Each quadrant in the Multicam can run at a different simulation speed or reset independently without affecting the others.

**Negative Consequences (Disadvantages):**

*   **High Refactoring Effort:** This is a highly invasive refactoring. Almost every function signature in the rendering and logic layers will need to change, breaking the current build until fully resolved.
*   **Regression Risk:** Touching the core simulation and GPU texture upload pipelines introduces a high risk of visual bugs or memory leaks if the new context lifecycle (initialization/destruction) is not perfectly managed.
*   **Increased Memory Usage:** Allocating multiple `RenderContext` structures implies multiple independent VRAM textures, increasing the overall GPU memory footprint (though still well within modern limits).

#### **4. Alternatives Considered**

*   **Rapid Context Switching (Data Swapping):** 
    *   *Concept:* Keep the global variables, but swap the data inside them rapidly right before the `UpdateTexture` and `DrawTexture` calls for each viewport quadrant.
    *   *Rejected:* Highly error-prone and "hacky". It creates a bottleneck and makes the render loop convoluted. It also scales poorly if different simulations need different shader parameters.
*   **Multiple OS Processes (Multi-Processing):** 
    *   *Concept:* Run 4 separate instances of the `biotope` executable, relying on a window manager to tile them into a 2x2 grid.
    *   *Rejected:* Destroys the cohesive Kiosk experience. We would lose the ability to have a unified HUD overlay, transitions between Kiosk modes (like fullscreen replay to grid), and it complicates the networking logic (each process fetching data independently).
*   **Geometry Instancing within Shader:**
    *   *Concept:* Send 4 separate grid datasets to a single shader invocation and use UV manipulation to place them.
    *   *Rejected:* Overly complex shader logic that tightly couples the number of simultaneous views to the shader code. A struct-based Context approach in C provides much greater flexibility.
