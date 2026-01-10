# Technical Design: Biotope UI System

**Version:** 1.0
**Date:** 2026-01-09
**Author:** Gemini
**Related Documents:** [ADR-0002](docs/ADR-0002-biotope-ui-system.md), [DEV_SPEC-0002](docs/DEV_SPEC-0002-biotope-ui-system.md)

---

### 1. Introduction

This document provides a detailed technical design for the "Biotope UI System" feature. It translates the requirements defined in DEV_SPEC-0002 into a concrete implementation plan, specifying the architecture, components, data models, and APIs. The goal is to create a robust, secure, and performant solution that integrates seamlessly with the existing application structure, leveraging **Raylib** for the graphical interface.

---

### 2. System Architecture and Components

The architecture follows a modular C approach, separating the simulation logic (Model) from the visualization and user interaction (View/Controller).

#### 2.1. Component Overview

*   **Logic Layer (`game_logic.c/h`):**
    *   **Responsibility:** Handles the core "Game of Life" rules, grid state management, and memory allocation. It is completely decoupled from Raylib, ensuring the simulation remains testable and portable.
    *   **Key Functions:** `update_generation`, `init_world_struct`, `count_neighbors`.

*   **UI Layer (`main.c` + `gui.c/h`):**
    *   **Responsibility:** Manages the Raylib window, the main application loop, input handling, and rendering.
    *   **State Machine:** Orchestrates transitions between Configuration, Editor, Simulation, and Game Over states.
    *   **Modules:**
        *   `ui_config`: Renders text boxes and sliders for settings.
        *   `ui_editor`: Handles mouse clicks for initial cell placement.
        *   `ui_simulation`: Renders the grid loop and handles timing.

*   **Data Persistence (`file_io.c/h`):**
    *   **Responsibility:** Handles saving/loading initial patterns and exporting game statistics to Markdown files.

#### 2.2. Component Interaction Diagram

```mermaid
graph TD
    User((User)) -->|Input (Mouse/Keyboard)| RaylibApp[Raylib Application]
    
    subgraph UI_Layer
        RaylibApp --> StateMachine[State Machine]
        StateMachine -->|Render/Input| UI_Config[Config Screen]
        StateMachine -->|Render/Input| UI_Editor[Editor Screen]
        StateMachine -->|Render| UI_Sim[Simulation View]
    end
    
    subgraph Logic_Layer
        UI_Sim -->|Get Grid Data| WorldData[World Struct]
        UI_Sim -->|Update| GameLogic[Game Logic Engine]
        UI_Editor -->|Modify| WorldData
        GameLogic -->|Read/Write| WorldData
    end

    subgraph Storage
        UI_Editor -->|Save/Load| FileIO[File I/O]
        StateMachine -->|Export Stats| FileIO
    end
```

---

### 3. Data Model Specification

The core data structures are defined in `game_logic.h` (refactored from `main.c`).

#### 3.1. World Structure
Reused from `main.c` but formalized in a header.

```c
// game_logic.h

#define DEAD 0
#define TEAM_RED 1
#define TEAM_BLUE 2

typedef struct {
    int *grid; // Pointer to flat array: row-major order
    int rows;
    int cols;
} World;
```

#### 3.2. Game Configuration Structure
Holds the settings for the current session.

```c
typedef struct {
    int rows;           // Default: 100
    int cols;           // Default: 100
    int delay_ms;       // Default: 500
    int max_population; // Default: 100 per team
    int max_rounds;     // Default: 1000
    int current_red_pop;
    int current_blue_pop;
} GameConfig;
```

#### 3.3. Application State Enum

```c
typedef enum {
    STATE_CONFIG,
    STATE_EDIT,
    STATE_RUNNING,
    STATE_GAME_OVER
} AppState;
```

---

### 4. Logic Layer Specification (`game_logic.c`)

This module encapsulates the core algorithms.

#### 4.1. API Functions

*   `World* create_world(int rows, int cols);`
    *   Allocates memory for a new World struct and its grid.
*   `void free_world(World *w);`
    *   Frees memory.
*   `void update_generation_colored(World *current, World *next);`
    *   Implementation of the Biotope rules (Red/Blue birth logic).
*   `int count_team_population(World *w, int team_id);`
    *   Helper to count cells for victory conditions/limits.

---

### 5. UI Layer Specification

#### 5.1. Main Loop (`main.c`)
The `main` function initializes Raylib and enters the `while (!WindowShouldClose())` loop. It acts as the context switcher.

```c
// Pseudo-code
void main() {
    InitWindow(800, 600, "Biotope Game of Life");
    AppState state = STATE_CONFIG;
    GameConfig config = {100, 100, 500, 100, 1000};
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        switch (state) {
            case STATE_CONFIG: draw_config_screen(&state, &config); break;
            case STATE_EDIT:   draw_editor_screen(&state, &config, world); break;
            case STATE_RUNNING: draw_simulation_screen(&state, &config, world); break;
            case STATE_GAME_OVER: draw_game_over_screen(&state, &config, world); break;
        }
        
        EndDrawing();
    }
    CloseWindow();
}
```

#### 5.2. Editor Screen Logic
*   **Coordinate Mapping:** Mouse `(x, y)` pixels -> Grid `(r, c)`.
    *   `cell_size = screen_width / cols;`
    *   `r = mouse_y / cell_size;`
    *   `c = mouse_x / cell_size;`
*   **Hemisphere Check:**
    *   `mid_col = cols / 2;`
    *   If `c < mid_col` -> Left Hemisphere (Blue).
    *   If `c >= mid_col` -> Right Hemisphere (Red).
*   **Population Check:**
    *   Before setting a cell to alive, check `if (current_team_pop < max_pop)`.

#### 5.3. Simulation Screen Logic
*   **Timer:** Uses `GetTime()` from Raylib to track delta time. Only calls `update_generation_colored` when `timer >= delay_ms`.
*   **Rendering:** Iterates `World->grid`.
    *   `value == TEAM_RED`: `DrawRectangle(..., RED)`
    *   `value == TEAM_BLUE`: `DrawRectangle(..., BLUE)`
    *   `value == DEAD`: `DrawRectangleLines(...)` (Grid lines)

---

### 6. File I/O Specification (`file_io.c`)

#### 6.1. Save/Load Format (Custom .bio)
Simple text format to store configuration and active cells.

```text
# Header
ROWS 100
COLS 100
MAX_POP 100

# Data (r, c, team)
10 5 2
10 6 2
50 80 1
...
```

#### 6.2. Statistics Export (Markdown)
Writes to `biotope_results_YYYYMMDD_HHMMSS.md`.

---

### 7. Performance Considerations

*   **Rendering Optimization:** Drawing 10,000 rectangles (100x100) per frame in Raylib is generally fast enough (batching is handled internally). If performance drops on large grids (e.g., 500x500), we will switch to drawing a single `Image` (texture manipulation) instead of thousands of `DrawRectangle` calls.
*   **Memory:** 1000x1000 integers is ~4MB, which is negligible.
*   **Update Logic:** The `O(N)` update complexity is acceptable for N=10,000 to 1,000,000.

---

### 8. Security Considerations

*   **Input Validation:** The Configuration screen must prevent allocating excessively large grids (e.g., >2000x2000) to avoid DoS (memory exhaustion) on the user's machine.
*   **File I/O:** The file loader should be robust against malformed `.bio` files to prevent buffer overflows or crashes (standard `fscanf` safety). It only writes to the local execution directory.
