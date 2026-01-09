# DEV_TASKS-0001: Competitive Biotope Mode (Red vs Blue)

**Status:** Planned
**Date:** 2026-01-08

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality.

**Briefing Documents:**
*   [ADR-0001: Competitive Biotope Mode (Red vs Blue)](../../docs/ADR-0001-competitive-biotope-mode.md)
*   [DEV_SPEC-0001: Requirements Specification](../../docs/DEV_SPEC-0001-competitive-biotope-mode.md)
*   [DEV_TECH_DESIGN-0001: Technical Design](../../docs/DEV_TECH_DESIGN-0001-competitive-biotope-mode.md)

---

## Phase 1: Core Data & Visualization Setup

*Goal: Update the data model to support teams and adapt the visualization to render them distinctively. The logic will temporarily remain "random" or broken until Phase 2.*

- [ ] **Step 1.1: Define Constants and Update Grid Initialization**
    - [ ] **Action:** In `main.c`, add the following preprocessor definitions at the top:
        ```c
        #define DEAD 0
        #define TEAM_RED 1
        #define TEAM_BLUE 2
        ```
    - [ ] **Action:** Modify `init_world` to populate the grid with `DEAD`, `TEAM_RED`, or `TEAM_BLUE` values.
        - *Logic:* ~80% DEAD, ~10% TEAM_RED, ~10% TEAM_BLUE.
        - *Code Hint:* `int val = rand() % 100; if (val < 10) grid[i] = TEAM_RED; else if (val < 20) grid[i] = TEAM_BLUE; else grid[i] = DEAD;`
    - [ ] **Verification (Interactive Test):**
        1.  Compile the program: `gcc main.c -o game` (or your platform equivalent).
        2.  Run with a small grid: `./game 10 10 100`.
        3.  **Expected Result:** The program runs, but the output (visualization) might look wrong or confusing (characters might shift) because `print_world` isn't updated yet. *Confirm it doesn't crash.*

- [ ] **Step 1.2: Update Visualization (`print_world`)**
    - [ ] **Action:** Modify `print_world` in `main.c`.
    - [ ] **Action:** Replace the `printf("%c", current_gen->grid[i] + 32);` logic with a switch/case or if/else block:
        - If `TEAM_RED`: Print `'X'` (Red ANSI optional: `\033[31mX\033[0m`).
        - If `TEAM_BLUE`: Print `'O'` (Blue ANSI optional: `\033[34mO\033[0m`).
        - If `DEAD`: Print `'.'` or `' '`.
    - [ ] **Verification (Interactive Test):**
        1.  Compile: `gcc main.c -o game`.
        2.  Run: `./game 20 20 100`.
        3.  **Expected Result:** You should see a grid with distinct 'X', 'O', and empty spaces/dots. The simulation "moves" (logic is still Conway's binary logic applied to integers, so colors might swap strangely, but display works).

- [ ] **Step 1.3: Input Validation and Cleanup**
    - [ ] **Action:** In `main` (argument parsing section), add checks:
        - If `rows > 2000` or `cols > 2000`, print an error and exit (prevents massive memory allocation crashes).
    - [ ] **Verification (Interactive Test):**
        1.  Run `./game 3000 10 100`.
        2.  **Expected Result:** Program exits with "Error: Grid too large".

---

## Phase 2: Logic Implementation (The "Biotope" Engine)

*Goal: Implement the core rules: Neighbor counting by team, Survival, and Competitive Birth.*

- [ ] **Step 2.1: Implement Team-Aware Neighbor Counting Helper**
    - [ ] **Action:** This is a complex refactor. In `update_generation`, the current neighbor counting logic (summing 8 neighbors) is purely numeric.
    - [ ] **Action:** Create a helper function (or inline logic if preferred for performance) that takes `index`, `cols`, `rows`, and `grid` and returns `red_count` and `blue_count`.
        - *Note:* Since the current code unrolls the loop for edge cases, you must replicate this structure. Instead of `sum = n1 + n2...`, you need to check each neighbor: `if (n1 == TEAM_RED) r++; else if (n1 == TEAM_BLUE) b++;`.
    - [ ] **Action:** *CRITICAL:* Refactor the neighbor access. The current code has massive blocks for "Top Left", "Top Right", etc. You will need to insert the "check value and increment specific counter" logic into *each* of those blocks.
        - *Recommendation:* Define a macro or inline function `CHECK_NEIGHBOR(idx)` that increments local `r` or `b` variables to avoid copying 10 lines of code 8 times.
    - [ ] **Verification (Code Review only):**
        1.  Verify that *every* neighbor access (8 per cell) is now checking for team identity, not just adding the integer value.

- [ ] **Step 2.2: Implement Evolution Rules**
    - [ ] **Action:** In `update_generation`, inside the main loop, after calculating `red_neighbors` and `blue_neighbors`:
        - `int total_neighbors = red_neighbors + blue_neighbors;`
        - **Survival Rule:** `if (current_cell != DEAD && (total_neighbors == 2 || total_neighbors == 3))` -> `next_cell = current_cell;` (Keep color).
        - **Death Rule:** `else if (current_cell != DEAD)` -> `next_cell = DEAD;`
        - **Birth Rule:** `else if (current_cell == DEAD && total_neighbors == 3)`:
            - `if (red_neighbors > blue_neighbors) next_cell = TEAM_RED;`
            - `else next_cell = TEAM_BLUE;`
    - [ ] **Verification (Interactive Test):**
        1.  Compile and run.
        2.  Observe closely.
        3.  **Expected Result:**
            - Stable blocks (2x2) of one color should persist.
            - "Gliders" should move.
            - Collisions between Red and Blue should produce new cells based on the majority rule (hard to verify visually in fast motion, but ensure colors are propagating).

---

## Phase 3: Game Management & HUD

*Goal: Add the game loop constraints, scoring, and end-game conditions.*

- [ ] **Step 3.1: Implement Population Counting**
    - [ ] **Action:** Inside the main `while` loop (or within `print_world` to save a pass), iterate the grid to count `total_red` and `total_blue`.
    - [ ] **Action:** Print these stats above or below the grid: `"Turn: %d | Red: %d | Blue: %d"`.
    - [ ] **Verification (Interactive Test):**
        1.  Run the game.
        2.  **Expected Result:** The numbers should update dynamically every frame.

- [ ] **Step 3.2: Implement MAX_ROUNDS and Win Condition**
    - [ ] **Action:** Define `#define MAX_ROUNDS 1000` (or passed as arg).
    - [ ] **Action:** In `main`, after the `while` loop finishes (ensure `turns < MAX_ROUNDS` is the condition), add logic to compare final populations.
    - [ ] **Action:** Print "GAME OVER".
    - [ ] **Action:** Print "Winner: RED" (if red > blue), "Winner: BLUE", or "DRAW".
    - [ ] **Verification (Interactive Test):**
        1.  Set `MAX_ROUNDS` to a small number (e.g., 50) temporarily or pass it as an argument.
        2.  Run.
        3.  **Expected Result:** Game stops automatically after 50 turns and announces a winner.

---

## Phase 4: Final Polish & Cleanup

*Goal: Code cleanup, standard compliance, and final manual verification.*

- [ ] **Step 4.1: Code Cleanup**
    - [ ] **Action:** Remove any debug `printf` statements.
    - [ ] **Action:** Ensure variable names are consistent (e.g., `current_gen`, `next_gen`).
    - [ ] **Action:** Check for memory leaks (ensure `free()` is called for grids).

- [ ] **Step 4.2: Final "Glider" Test**
    - [ ] **Action:** Hardcode a specific pattern in `init_world` *temporarily* for testing (e.g., a Red Glider crashing into a Blue Block).
    - [ ] **Verification:** Watch the interaction. Does the glider "infect" the block or get destroyed? (Just confirm interaction occurs).
    - [ ] **Action:** Revert to random initialization.

- [ ] **Step 4.3: Documentation**
    - [ ] **Action:** Update the header comment in `main.c` to describe the new rules.

