# Requirements Analysis & Specification: Multicam Render Context Architecture

This document details the requirements for the systematic refactoring of the C-Application to support multiple concurrent simulation viewports, as described in **ADR-0018**.

---

### 1. Detailed Requirements Specification

The primary objective is to eliminate the usage of global variables for simulation state and rendering resources, allowing the application to run and display multiple "Game of Life" simulations side-by-side (the "Wusel-Multicam" mode).

**1.1 Data Encapsulation (Context Structs):**
*   **SimulationContext:** All data pertaining to a single running match (grid double buffers, active populations, generation counters, current ruleset) must be encapsulated into a `SimulationContext` struct (or by extending the existing `World` struct).
*   **RenderContext:** All Raylib GPU resources and viewport configurations for a single view (GPU Texture ID, target screen `Rectangle` bounds, camera offset/zoom, shader uniform locations specific to this view) must be encapsulated into a `RenderContext` struct.

**1.2 Signature Refactoring:**
*   All functions in `src/core/game_logic.c` that modify the game state must take a pointer to `SimulationContext` (or `World*`) instead of operating on implied global arrays.
*   All functions in `src/gui/renderer.c` must take a pointer to `RenderContext` (and optionally `SimulationContext` for data mapping) to determine *what* to draw and *where* on the screen to draw it.
*   Functions responsible for uploading data to the GPU (e.g., `UpdateTexture`) must operate strictly on the texture ID provided by the passed `RenderContext`.

**1.3 Multicam Manager (Kiosk Implementation):**
*   The application must support initializing an array of these contexts (e.g., `RenderContext contexts[4]` and `SimulationContext sims[4]` for a 2x2 grid).
*   The main rendering loop must iterate over these arrays, calling the refactored update and draw functions sequentially for each active viewport.

---

### 2. User Stories & Acceptance Criteria

**Epic: Refactor for Wusel-Multicam Mode**

*   **User Story 1: Eliminate Global Simulation States**
    *   **As a C developer,** I want to encapsulate all game logic data into a struct, **so that** I can instantiate multiple independent games simultaneously in memory without cross-contamination.
    *   **Acceptance Criteria:**
        *   No global or `static` arrays for the cell grid exist in `game_logic.c` or `core_types.h`.
        *   A `SimulationContext` (or `World`) struct contains the double buffers.
        *   Running the existing unit tests (`test_core_types.c` or similar) passes after modifying them to instantiate local structs.

*   **User Story 2: Eliminate Global Rendering States**
    *   **As a C developer,** I want to encapsulate Raylib textures and viewport data into a `RenderContext`, **so that** the GPU can render different grids to different areas of the screen.
    *   **Acceptance Criteria:**
        *   The global `gridTex` (and related GPU buffers) in `renderer.c` is removed.
        *   A `RenderContext` struct holds the `Texture2D` and a target `Rectangle` (x, y, width, height).
        *   The `draw_grid` function (or equivalent) accepts a `RenderContext*` and renders exactly within the bounds defined by that context's `Rectangle`.

*   **User Story 3: Render a 2x2 Grid (Proof of Concept)**
    *   **As a Kiosk observer,** I want to see four distinct simulations running on the screen at the same time, **so that** the "Wusel-Multicam" visual effect is achieved.
    *   **Acceptance Criteria:**
        *   The application loop initializes an array of 4 `RenderContext` and 4 `SimulationContext` instances.
        *   The screen is divided into 4 quadrants (top-left, top-right, bottom-left, bottom-right).
        *   Each quadrant runs independently (e.g., setting a pixel in quadrant 1 does not affect quadrant 2).
        *   The application maintains a high frame rate (>= 60 FPS) while updating 4 textures per frame.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Definition of `SimulationContext` and `RenderContext` structs.
        *   Removal of global state from `game_logic.c` and `renderer.c`.
        *   Refactoring of all function signatures to pass context pointers.
        *   Successful compilation (`make`) without warnings.
        *   Ability to render at least two independent viewports side-by-side.
    *   **Should-Have:**
        *   Optimized GPU upload (only upload pixel data for contexts that actually changed their generation).
        *   Dynamic resizing of viewports (handling window resize events gracefully).
    *   **Could-Have:**
        *   Independent shader parameterization per `RenderContext` (e.g., different color palettes for different quadrants).
    *   **Won't-Have (in this increment):**
        *   The actual fetching of live seeds from the backend (This is handled by WP2 / ADR-0017).
        *   The overarching Kiosk State Machine timer (handled by WP4 / ADR-0019).

*   **Dependencies:**
    1.  **Codebase Stability:** This refactoring is highly invasive. It MUST be executed on a clean, working branch. No other feature development (like Networking or UI) should modify `renderer.c` or `game_logic.c` concurrently to avoid massive merge conflicts.
    2.  **Kiosk Mode (ADR-0019):** The final Kiosk state relies completely on the successful implementation of this Multicam architecture.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| 1 | Multicam | Define `SimulationContext` / update `World` struct in `core_types.h`. | Must-Have |
| 2 | Multicam | Remove global grid arrays and refactor `game_logic.c` signatures. | Must-Have |
| 3 | Multicam | Define `RenderContext` struct in `renderer.h` (Texture2D, Rectangle). | Must-Have |
| 4 | Multicam | Remove global `gridTex` and refactor `renderer.c` signatures. | Must-Have |
| 5 | Multicam | Update `main.c` / `app_state_manager.c` to instantiate the new structs and pass pointers. | Must-Have |
| 6 | Multicam | Implement a 2x2 grid manager (allocate 4 contexts, calculate screen bounds for each). | Must-Have |
| 7 | Multicam | Verify memory management (ensure all 4 textures are unloaded during cleanup). | Must-Have |
| 8 | Multicam | Optimize GPU `UpdateTexture` to trigger only on generation changes per context. | Should-Have |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The code is written and formatted according to the guidelines in `docs/CODING_STYLE.md` (snake_case, no global variables for simulation state).
*   **Tests:**
    *   All C unit tests (`test_core_types.c`, `test_renderer.c`, etc.) have been updated to use the new structs and pass successfully.
*   **Acceptance Criteria:** All acceptance criteria defined for the story have been met.
*   **Interactive Verification:** A manual launch of the application confirms that multiple simulations can run on the screen simultaneously without visual bleeding or crashes.
*   **Memory Safety:** Valgrind analysis confirms that allocating and freeing multiple `RenderContext` instances does not leak Raylib GPU textures or host memory.
*   **Documentation:** Technical documentation (e.g., `DEV_TASKS-0018.md`) is updated to reflect progress.
