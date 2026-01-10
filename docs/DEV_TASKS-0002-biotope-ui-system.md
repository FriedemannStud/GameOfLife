# DEV_TASKS-0002: Biotope UI System Implementation

This task list details the implementation steps for the interactive "Biotope" game mode using Raylib, as defined in **DEV_SPEC-0002** and **DEV_TECH_DESIGN-0002**.

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality.

**Important:** Every single code change (new lines or modifications) must be commented with `// KI-Agent unterstützt`.

**Briefing Documents:**
*   [ADR-0002: Interactive UI System](../../docs/ADR-0002-biotope-ui-system.md)
*   [DEV_SPEC-0002: Requirements Specification](../../docs/DEV_SPEC-0002-biotope-ui-system.md)
*   [DEV_TECH_DESIGN-0002: Technical Specification](../../docs/DEV_TECH_DESIGN-0002-biotope-ui-system.md)

---

## Phase 1: Infrastructure & Core Refactoring

*Goal: Decouple the existing simulation logic from `main.c` into a reusable module (`game_logic.c`), paving the way for the GUI integration.*

- [x] **Step 1.1: Create Logic Module Headers**
    - [x] **Action:** Create `game_logic.h`.
    - [x] **Action:** Define the constants `DEAD`, `TEAM_RED`, `TEAM_BLUE` in `game_logic.h`.
    - [x] **Action:** Define the `World` struct in `game_logic.h` (move it from `main.c`).
    - [x] **Action:** Declare function prototypes: `create_world`, `free_world`, `init_world`, `update_generation`, `count_neighbors` (if needed publicly).
    - [x] **Verification:** Header exists and is correctly structured.

- [x] **Step 1.2: Implement Logic Module Source**
    - [x] **Action:** Create `game_logic.c`.
    - [x] **Action:** Move the implementations of `init_world` and `update_generation` (and the `update_generation_colored` logic from ADR-0001) from `main.c` to `game_logic.c`.
    - [x] **Action:** Implement `create_world` (mallocs) and `free_world` (frees) in `game_logic.c`.
    - [x] **Action:** Ensure all functions are commented with `// KI-Agent unterstützt`.
    - [x] **Verification:** Created `test_logic.c`. All tests passed.

- [x] **Step 1.3: Clean up `main.c` (CLI Legacy)**
    - [x] **Action:** Modify `main.c` to include `game_logic.h`.
    - [x] **Action:** Remove the struct definitions and helper functions that were moved.
    - [x] **Action:** Update `main` to use `create_world` and `free_world`.
    - [x] **Verification (Interactive Test):**
        1.  Run the existing CLI build command.
        2.  Execute `./game 20 20 100`.
        3.  **Expected Result:** CLI version still works. (Confirmed)

---

## Phase 2: Raylib Setup & State Machine

*Goal: Initialize the graphical window and establish the basic application flow (State Machine).*

- [x] **Step 2.1: Raylib Environment Setup**
    - [x] **Action:** Update `Makefile` (or build script) to link against `raylib`, `gdi32`, `winmm` (for Windows).
    - [x] **Action:** Create `gui.c` and `gui.h`.
    - [x] **Action:** In `gui.c`, create a function `void run_gui_app()`.
    - [x] **Action:** Inside `run_gui_app`, add `InitWindow(800, 600, "Biotope");` and a basic `while (!WindowShouldClose())` loop with `BeginDrawing(); ClearBackground(RAYWHITE); EndDrawing();`.
    - [x] **Verification (Interactive Test):**
        1.  Modify `main.c` to call `run_gui_app()`.
        2.  Compile and run.
        3.  **Expected Result:** Blank white window appears. (Confirmed)

- [x] **Step 2.2: State Machine Implementation**
    - [x] **Action:** Define `AppState` enum (`STATE_CONFIG`, `STATE_EDIT`, `STATE_RUNNING`, `STATE_GAME_OVER`) in `gui.h`.
    - [x] **Action:** Define `GameConfig` struct in `gui.h` (rows, cols, delay, etc.).
    - [x] **Action:** In `run_gui_app`, declare a variable `AppState state = STATE_CONFIG;`.
    - [x] **Action:** Implement a `switch(state)` inside the drawing loop.
    - [x] **Action:** Add temporary key presses to switch states.
    - [x] **Verification (Interactive Test):**
        1.  Run the app.
        2.  Press keys to switch states.
        3.  **Expected Result:** Text on screen changes. (Confirmed)

---

## Phase 3: Configuration & Editor (Interactive Setup)

*Goal: Allow users to define the grid and place initial cells.*

- [x] **Step 3.1: Configuration Screen UI**
    - [x] **Action:** In `STATE_CONFIG`, implement a simple UI to set grid dimensions and delay.
    - [x] **Action:** Add support for German keyboard layout (`KEY_RIGHT_BRACKET`, `KEY_SLASH`) and Page Keys.
    - [x] **Action:** Add a "START SETUP" (ENTER) button.
    - [x] **Verification (Interactive Test):** Configuration values change correctly. (Confirmed)

- [x] **Step 3.2: Grid Rendering (Static)**
    - [x] **Action:** In `STATE_EDIT`, implement grid rendering and hemisphere separator.
    - [x] **Action:** Enable `FLAG_WINDOW_RESIZABLE` and use dynamic screen dimensions.
    - [x] **Verification:** Grid is drawn and resizes correctly. (Confirmed)

- [x] **Step 3.3: Mouse Interaction (Cell Placement)**
    - [x] **Action:** In `STATE_EDIT`, detect mouse clicks to toggle cells.
    - [x] **Action:** Implement hemisphere constraints (Blue left, Red right).
    - [x] **Verification (Interactive Test):** Cell placement works as intended. (Confirmed)

- [x] **Step 3.4: Max Population Constraint**
    - [x] **Action:** Add population counters and enforce limits.
    - [x] **Action:** Display counters on screen.
    - [x] **Verification (Interactive Test):** Counters update and limit population. (Confirmed)

---

## Phase 4: Simulation & Game Loop

*Goal: Run the actual game logic and visualize the battle.*

- [x] **Step 4.1: Simulation State Logic**
    - [x] **Action:** In `STATE_RUNNING`, track time and update generations based on `delay_ms`.
    - [x] **Action:** Render living cells with colors.
    - [x] **Verification (Interactive Test):** Simulation runs smoothly. (Confirmed)

- [x] **Step 4.2: Game Over & Intermediate Finish State**
    - [x] **Action:** Implement `STATE_FINISHED` to freeze the final grid before showing results.
    - [x] **Action:** Check victory conditions and switch to `STATE_FINISHED`.
    - [x] **Action:** Transition to `STATE_GAME_OVER` after ENTER.
    - [x] **Verification (Interactive Test):** Full game cycle works with intermediate view. (Confirmed)

---

## Phase 5: Polish, Persistence & Data

*Goal: Save/Load functionality and nice-to-have features.*

- [x] **Step 5.1: File I/O (Save/Load Setup)**
    - [x] **Action:** Create `file_io.c` and `file_io.h`.
    - [x] **Action:** Implement `save_grid` and `load_grid` with status feedback.
    - [x] **Action:** Add 'S' and 'L' key handling in Editor.
    - [x] **Verification (Interactive Test):** Patterns are saved/loaded correctly with visual status messages. (Confirmed)

- [x] **Step 5.2: Game Over Screen & Export**
    - [x] **Action:** Display final winner and scores in `STATE_GAME_OVER`.
    - [x] **Action:** Automatically export statistics to Markdown on finish.
    - [x] **Verification:** Markdown files are created with correct data. (Confirmed)

- [x] **Step 5.3: Cleanup**
    - [x] **Action:** Ensure memory is freed and window is closed properly.
    - [x] **Action:** Final code review for `// KI-Agent unterstützt` comments.
    - [x] **Verification:** Final playthrough successful. (Confirmed)