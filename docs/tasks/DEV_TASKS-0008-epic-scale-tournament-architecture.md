# DEV_TASKS-0008: "Epic Scale" Tournament Architecture

This document breaks down the implementation of the massive-grid optimization ("Active Chunk" heuristic) and the E-sport broadcasting tools (`STATE_OBSERVER`, Camera) into actionable, verifiable steps for a C developer. Quality precedes speed.

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality and adhering to `docs/CODING_STYLE.md`.

**Briefing Documents:**
*   [ADR-0008: "Epic Scale" Tournament Architecture](../adr/ADR-0008-epic-scale-tournament-architecture.md)
*   [DEV_SPEC-0008: "Epic Scale" Tournament Architecture](../specs/DEV_SPEC-0008-epic-scale-tournament-architecture.md)
*   [DEV_TECH_DESIGN-0008: "Epic Scale" Tournament Architecture](../tech_design/DEV_TECH_DESIGN-0008-epic-scale-tournament-architecture.md)

---

## Phase 1: Configuration Expansion

*Goal: Safely increase the grid size limits for the native build to support "Epic Scale" without breaking the WASM web constraints.*

- [x] **Step 1.1: Limit Adjustments**
    - [x] **Action:** Open `gui.c`.
    - [x] **Action:** Locate the `STATE_CONFIG` state where the `rows` and `cols` variables are manipulated by the UI buttons (e.g., +/- 10).
    - [x] **Action:** Change the upper boundary condition. If compiling natively (`#ifndef PLATFORM_WEB`), allow the max size to reach 5000. If compiling for the web (`#ifdef PLATFORM_WEB`), clamp the max size to 500 (or the previously established safe limit).
    - [x] **Verification:** Run `make clean && make`, then `./biotope`. Go into config and try to increase the grid size past 1000.
        - **Expected Result:** The UI allows you to set sizes up to 5000. Do not press Start yet, as the brute-force loop will likely hang the CPU. Please report if the UI changes worked.

---

## Phase 2: Algorithmic Supremacy (Active Chunking)

*Goal: Implement the spatial partitioning heuristic in `game_logic.c` to bypass dead sectors of the grid, saving millions of CPU cycles.*

- [x] **Step 2.1: Extend `World` Struct**
    - [x] **Action:** Open `game_logic.h`.
    - [x] **Action:** Add the `chunk_map` pointer and dimensions to the `World` struct (see DEV_TECH_DESIGN Section 3.1). Add `#define CHUNK_SIZE 64`.
    - [x] **Verification:** Compile (`make`). Expected to pass.

- [x] **Step 2.2: Memory Allocation for Chunks**
    - [x] **Action:** Open `game_logic.c`.
    - [x] **Action:** In `create_world()`, calculate `chunk_rows` and `chunk_cols` (remember to account for remainders, e.g., `(rows + CHUNK_SIZE - 1) / CHUNK_SIZE`).
    - [x] **Action:** Allocate memory: `w->chunk_map = calloc(w->chunk_rows * w->chunk_cols, sizeof(unsigned char));`.
    - [x] **Action:** In `free_world()`, add `if(w->chunk_map) free(w->chunk_map);`.
    - [x] **Verification:** Run `make` and `./biotope`. Ensure no segmentation faults during startup or shutdown.

- [x] **Step 2.3: Initializing the Chunk Map**
    - [x] **Action:** In `game_logic.c`, inside `init_world()` (where random cells are placed) AND whenever a cell is manually placed/removed via the UI in `gui.c`, the corresponding chunk in `chunk_map` MUST be set to `1` (ACTIVE).
    - [x] **Action:** Write a small helper function `void activate_chunk_at(World *w, int r, int c)` to handle the math `chunk_idx = (r / CHUNK_SIZE) * w->chunk_cols + (c / CHUNK_SIZE); w->chunk_map[chunk_idx] = 1;`.

- [x] **Step 2.4: Refactor `update_generation` (The Core Heuristic)**
    - [x] **Action:** Open `game_logic.c` and rewrite `update_generation`.
    - [x] **Action:** Follow the structure outlined in DEV_TECH_DESIGN Section 4.1.
        1. Clear `next_gen->chunk_map`.
        2. Create the outer loop over chunks (`cr` and `cc`).
        3. Write `is_chunk_or_neighbors_active` helper. If false, `continue`.
        4. If true, run the standard inner loop (`r` and `c`) *only* for the boundaries of that specific chunk.
        5. If any cell becomes ALIVE inside that inner loop, set `next_gen->chunk_map[chunk_idx] = 1`.
    - [x] **Verification (Performance Test):** Set the grid size to 2000x2000 in `STATE_CONFIG`. Place a single Glider pattern. Start the simulation.
        - **Expected Result:** The game should run smoothly at 60fps (or very high CPU speed) because 99% of the 2000x2000 grid is marked DEAD and bypassed. Compare this mentally to how slow a 2000x2000 grid ran before this change. Report your findings.

---

## Phase 3: The E-Sport Broadcast Tools

*Goal: Add the `STATE_OBSERVER` and implement free-cam navigation to view the massive grids.*

- [x] **Step 3.1: Initialize `Camera2D`**
    - [x] **Action:** Open `gui.c`. At the top (or inside `run_gui_app`), declare `Camera2D observer_camera = { 0 };`.
    - [x] **Action:** Initialize it: `observer_camera.zoom = 1.0f; observer_camera.rotation = 0.0f;`.

- [x] **Step 3.2: Implement `STATE_OBSERVER`**
    - [x] **Action:** Open `gui.h` and add `STATE_OBSERVER` to `AppState`.
    - [x] **Action:** In `gui.c`, add the state to the main switch. (For now, just copy the logic from `STATE_RUNNING` so the simulation continues).
    - [x] **Action:** Add a hotkey in `STATE_RUNNING` (e.g., 'O') to toggle into `STATE_OBSERVER`.

- [x] **Step 3.3: Camera Input Logic**
    - [x] **Action:** Inside the `STATE_OBSERVER` logic block in `gui.c`, add input handling:
        *   **Pan:** `if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) { Vector2 delta = GetMouseDelta(); observer_camera.target.x -= delta.x / observer_camera.zoom; observer_camera.target.y -= delta.y / observer_camera.zoom; }`
        *   **Zoom:** `float wheel = GetMouseWheelMove(); if (wheel != 0) { /* calculate zoom to center on mouse */ }` (Raylib has standard patterns for zoom-to-mouse).

- [x] **Step 3.4: Connect Camera to Renderer**
    - [x] **Action:** In `DrawGridAndCells` (or wherever the main grid drawing occurs), wrap the grid drawing functions with `BeginMode2D(observer_camera);` and `EndMode2D();`.
    - [x] **Action:** Ensure the UI overlay (population counters) is drawn *after* `EndMode2D()` so it stays fixed to the screen.
    - [x] **Verification (Interactive Test):** Run a simulation. Press 'O' to enter observer mode.
        - **Expected Result:** Right-clicking and dragging pans the view over the grid. Scrolling zooms in and out. The UI remains stationary. Please confirm the camera controls work.

---

*Developer: Upon completing these phases, the architecture supports "Epic Scale" simulations and provides the necessary tools for E-sport broadcasting. Ensure all changes are committed and the DoD from DEV_SPEC is met.*
