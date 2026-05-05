# Requirements Analysis & Specification: "Epic Scale" Tournament Architecture

This document details the requirements for supporting massive-scale biotope simulations and the broadcasting tools required for an E-sport environment, as described in **ADR-0008**.

---

### 1. Detailed Requirements Specification

The objective is to leverage the native C/OpenMP environment to push the boundaries of grid sizes (e.g., 5000x5000) for "spectacle generation" and provide content creators with the necessary tools to cast these algorithmic battles.

**Key Functional Requirements:**
1.  **Algorithmic Heuristics (Sparse/Chunking):** The current `update_generation` loop iterates over every cell. For a 5000x5000 grid, this is 25 million iterations per frame. We must implement a heuristic (e.g., dividing the grid into "chunks" and tracking which chunks contain living cells) to bypass processing for completely dead sectors.
2.  **Observer UI State (`STATE_OBSERVER`):** A dedicated state machine branch focused purely on spectating, not interacting with the simulation rules.
3.  **Free-Cam 2D Camera:** The observer must be able to pan across the massive grid and zoom in/out smoothly using the mouse or keyboard, as the entire grid will no longer fit legibly on a single screen.
4.  **Broadcast Overlays:** Real-time, non-intrusive UI elements displaying current populations, dominance percentages, and a minimap (if zoomed in) to provide context to viewers.

---

### 2. User Stories & Acceptance Criteria

**Epic: Algorithmic Supremacy (Massive Grids)**

*   **User Story 1: Epic Scale Initialization**
    *   **As a power user,** I want to be able to configure grid sizes up to 5000x5000 in the native build, **so that** I can simulate galactic-scale biotope wars.
    *   **Acceptance Criteria:**
        *   The `GameConfig` limits are increased for native builds to allow 5000x5000 grids.
        *   Memory allocation (`create_world`) successfully handles the massive RAM requirement (Double Buffering 25M `int`s) without crashing, assuming sufficient system RAM.

*   **User Story 2: Active Chunk Heuristic**
    *   **As an E-sport caster,** I want the simulation to maintain a playable framerate (e.g., >30 FPS) even on massive grids, **so that** the broadcast remains visually smooth.
    *   **Acceptance Criteria:**
        *   The `update_generation` logic implements a spatial partitioning system (e.g., dividing the grid into 64x64 chunks).
        *   Only chunks containing living cells (or adjacent to living cells) are processed by the OpenMP loop.
        *   FPS on a sparse 5000x5000 grid is significantly higher than a naive brute-force loop.

**Epic: The E-Sport Broadcast Tools**

*   **User Story 3: Free-Cam Navigation**
    *   **As an observer,** I want to pan and zoom around the massive grid using my mouse and scroll wheel, **so that** I can focus on specific skirmishes or view the entire macro-structure.
    *   **Acceptance Criteria:**
        *   In `STATE_OBSERVER`, a `Camera2D` object (Raylib) is implemented.
        *   Right-click and drag pans the camera.
        *   Mouse wheel zooms in and out, centered on the cursor.
        *   Grid rendering (`DrawGridAndCells`) respects the camera transform.

*   **User Story 4: Broadcast Overlays & Minimap**
    *   **As an observer,** I want to see a live minimap and population statistics overlay, **so that** my viewers always understand the context of the battle even when zoomed in.
    *   **Acceptance Criteria:**
        *   A minimalist UI overlay displays the current Red and Blue population counts and the generation number.
        *   When zoomed in, a small minimap in the corner shows the full grid state (highly downscaled) and a rectangle indicating the current camera viewport.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Increase maximum grid size constraints in `STATE_CONFIG`.
        *   Implementation of the 2D Camera (`Camera2D`) for panning and zooming in `STATE_OBSERVER`.
    *   **Should-Have:**
        *   Active Chunk/Sparse Matrix heuristic in `game_logic.c` to salvage performance on massive grids.
        *   Live population overlay in `STATE_OBSERVER`.
    *   **Could-Have:**
        *   Interactive Minimap (click minimap to jump camera).
        *   "Follow Frontline" automatic camera mode.
    *   **Won't-Have (in this increment):**
        *   Hashlife implementation (too complex for the current C architecture, we will rely on chunking + OpenMP).

*   **Dependencies:**
    1.  **Topic: GPU Aesthetic Overhaul:** The camera scaling and panning logic must interact seamlessly with the GLSL shaders and Ping-Pong RenderTextures established in DEV_SPEC-0006.
    2.  **Topic: Memory Management:** The heuristic chunking data structures must be carefully allocated alongside the `World` grids to prevent memory leaks.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| TSK-01 | Architecture | Increase UI configuration limits to allow up to 5000x5000 grids | High |
| TSK-02 | Broadcast | Add `STATE_OBSERVER` to `AppState` enum and basic state transition logic | High |
| TSK-03 | Broadcast | Implement Raylib `Camera2D` logic (Pan with Right-Click, Zoom with Scroll) | High |
| TSK-04 | Broadcast | Update `DrawGridAndCells` to render through `BeginMode2D(camera)` | High |
| TSK-05 | Supremacy | Design `Chunk` data structure (e.g., `bool is_active`) in `game_logic.h` | High |
| TSK-06 | Supremacy | Refactor `update_generation` to only iterate over active chunks and their neighbors | High |
| TSK-07 | Supremacy | Implement logic to mark chunks as active/inactive at the end of a generation | High |
| TSK-08 | Broadcast | Create live population UI overlay (text rendering) | Medium |
| TSK-09 | Broadcast | Render downscaled Minimap using `DrawTexturePro` when camera zoom > 1.5x | Low |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** Code is formatted according to `docs/CODING_STYLE.md`.
*   **Compilation:** Project compiles without warnings.
*   **Execution:** 
    *   A 5000x5000 grid can be initialized without crashing.
    *   The Observer Camera pans and zooms smoothly without visual tearing.
    *   The simulation correctly follows Red vs. Blue rules even when using the Chunk heuristic.
*   **Tests:**
    *   Performance Test: Compare the execution time of 100 generations on a 2000x2000 grid with and without the Chunk heuristic. Document the speedup.
    *   Regression Test: Ensure the WASM build (which limits grid size) is not broken by the new camera or chunking logic.
*   **Acceptance Criteria:** All acceptance criteria defined for the story have been met.
*   **Documentation:** Updates to `CHANGELOG.md`.