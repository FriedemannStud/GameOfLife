# DEV_TASKS-0007: Laser Focus on the "Biotope" Competitive USP

This document breaks down the implementation of the competitive E-sport features ("Blind Draft", "Ignition", "Catalyst", "Telemetry") into actionable, verifiable steps.

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality and adhering to `docs/CODING_STYLE.md`.

**Briefing Documents:**
*   [ADR-0007: Laser Focus on the "Biotope" Competitive USP](../adr/ADR-0007-laser-focus-competitive-usp.md)
*   [DEV_SPEC-0007: Laser Focus on the "Biotope" Competitive USP](../specs/DEV_SPEC-0007-competitive-usp-focus.md)
*   [DEV_TECH_DESIGN-0007: Laser Focus on the "Biotope" Competitive USP](../tech_design/DEV_TECH_DESIGN-0007-competitive-usp-focus.md)

---

## Phase 1: The "Blind Draft" (State Machine Expansion)

*Goal: Replace the single `STATE_EDIT` with sequential turn-based editing states that obscure the opponent's territory using a "Fog of War".*

- [ ] **Step 1.1: State Enum Expansion**
    - [ ] **Action:** Open `gui.h`.
    - [ ] **Action:** Replace `STATE_EDIT` in the `AppState` enum with `STATE_EDIT_RED` and `STATE_EDIT_BLUE`.
    - [ ] **Verification:** Run `make clean && make`.
        - **Expected Result:** Compilation will fail with errors pointing to `gui.c` where `STATE_EDIT` is still used. This is expected. Report the number of errors.

- [ ] **Step 1.2: Refactor State Transitions**
    - [ ] **Action:** Open `gui.c`.
    - [ ] **Action:** Replace the `case STATE_EDIT:` block with two identical blocks for `case STATE_EDIT_RED:` and `case STATE_EDIT_BLUE:`.
    - [ ] **Action:** In `STATE_CONFIG`, change the transition trigger (usually pressing Enter or a Start button) to transition to `STATE_EDIT_RED` instead of the old edit state.
    - [ ] **Action:** In `STATE_EDIT_RED`, change the "Done/Next" trigger (e.g., pressing Enter) to transition to `STATE_EDIT_BLUE`.
    - [ ] **Action:** In `STATE_EDIT_BLUE`, change the "Done/Next" trigger to transition to `STATE_RUNNING` (we will change this to `STATE_IGNITION` in Phase 2).
    - [ ] **Action:** Enforce placement restrictions: In `STATE_EDIT_RED`, `gui_world->grid` updates should only be allowed if `col >= midCol`. In `STATE_EDIT_BLUE`, only if `col < midCol`.
    - [ ] **Verification (Interactive Test):** Run `make` and `./biotope`.
        - **Expected Result:** You enter Config, then Red Edit (can only place on the right), press Enter, then Blue Edit (can only place on the left), press Enter, then the simulation runs. Please confirm the flow works.

- [ ] **Step 1.3: Fog of War Rendering**
    - [ ] **Action:** Still in `gui.c`, locate the rendering section (either inside the state cases or in `DrawGridAndCells` if you pass the state to it).
    - [ ] **Action:** Implement the visual overlay as designed in DEV_TECH_DESIGN Section 5.1. Draw a semi-transparent black rectangle over the left half during `STATE_EDIT_RED`, and over the right half during `STATE_EDIT_BLUE`.
    - [ ] **Verification (Interactive Test):** Run `make` and `./biotope`.
        - **Expected Result:** During Red's turn, the Blue side is dark/obscured. During Blue's turn, the Red side is dark. Confirm visual parity.

---

## Phase 2: The "Ignition" Sequence

*Goal: Add a dramatic 3-second pause where the full grid is revealed before the simulation actually begins calculating.*

- [ ] **Step 2.1: Add `STATE_IGNITION`**
    - [ ] **Action:** Open `gui.h` and add `STATE_IGNITION` to `AppState`.
    - [ ] **Action:** In `gui.c`, change the transition at the end of `STATE_EDIT_BLUE` to go to `STATE_IGNITION` instead of `STATE_RUNNING`.

- [ ] **Step 2.2: Implement Timer Logic**
    - [ ] **Action:** In `gui.c`, add the `case STATE_IGNITION:` block.
    - [ ] **Action:** Implement the timer logic using `GetTime()` as detailed in DEV_TECH_DESIGN Section 5.2.
    - [ ] **Action:** Ensure `DrawGridAndCells(...)` is called inside this state so the full grid (without fog) is visible.
    - [ ] **Action:** Draw the countdown text (3... 2... 1...) in the center of the screen.
    - [ ] **Verification (Interactive Test):** Run `make` and `./biotope`. Finish the draft phase.
        - **Expected Result:** The game transitions to Ignition, the fog lifts showing both sides, a 3-second countdown displays, and then it automatically transitions to Running. Confirm.

---

## Phase 3: The "Catalyst" Intervention

*Goal: Allow players a single, tactical strike during the simulation to alter the deterministic outcome.*

- [ ] **Step 3.1: Core Logic in `game_logic.c`**
    - [ ] **Action:** Open `game_logic.h` and declare `void apply_catalyst(World *w, int center_r, int center_c);`.
    - [ ] **Action:** Open `game_logic.c` and implement the function. Ensure strict bounds checking to prevent writing outside the `r >= 1 && r <= w->rows` visible area (see DEV_TECH_DESIGN Section 4.1).
    - [ ] **Verification:** Run `make`. Ensure it compiles without errors.

- [ ] **Step 3.2: State Tracking & Input**
    - [ ] **Action:** Open `gui.h` and add `bool red_catalyst_used;` and `bool blue_catalyst_used;` to `GameConfig`.
    - [ ] **Action:** In `gui.c` (`STATE_CONFIG`), initialize both to `false`.
    - [ ] **Action:** In `gui.c` (`STATE_RUNNING`), add mouse input detection. E.g., if `IsMouseButtonPressed(MOUSE_LEFT_BUTTON)`:
        - Calculate the grid `row` and `col` from the mouse position.
        - If `col >= midCol` and `!config.red_catalyst_used`, call `apply_catalyst(gui_world, row, col)` and set `red_catalyst_used = true`.
        - Do the equivalent for Blue on the left side.
    - [ ] **Verification (Interactive Test):** Run the game. During the simulation, click the mouse on the red side, then again on the red side.
        - **Expected Result:** The first click instantly creates a 10x10 dead square. The second click does nothing (charge used). Please confirm.

---

## Phase 4: Telemetry & Post-Match Autopsy

*Goal: Record population sizes per round to render a graph at the end of the match.*

- [ ] **Step 4.1: Data Allocation**
    - [ ] **Action:** Open `gui.h` and add `int *history_red_pop;`, `int *history_blue_pop;`, and `int history_count;` to `GameConfig`.
    - [ ] **Action:** In `gui.c` during `STATE_IGNITION` (or right before entering RUNNING), allocate the arrays: `malloc(config.max_rounds * sizeof(int))`. Set `history_count = 0`.
    - [ ] **Action:** IMPORTANT: In the transition *out* of `STATE_FINISHED` (e.g., returning to menu), ensure you `free()` these arrays to prevent memory leaks.

- [ ] **Step 4.2: Data Recording**
    - [ ] **Action:** In `gui.c` (`STATE_RUNNING`), right after `update_generation` returns, record the current populations into the arrays and increment `history_count` (see DEV_TECH_DESIGN Section 4.2).

- [ ] **Step 4.3: Graph Rendering**
    - [ ] **Action:** In `gui.c` (`STATE_FINISHED` or `STATE_GAME_OVER`), write a loop that iterates from `0` to `history_count`.
    - [ ] **Action:** Use Raylib's `DrawLine()` or `DrawLineStrip()` to draw two graphs (one Red, one Blue) mapping the X-axis to rounds and the Y-axis to population size.
    - [ ] **Verification (Interactive Test):** Run a match with a low `max_rounds` (e.g., 500). Wait for it to finish.
        - **Expected Result:** The Game Over screen displays a rudimentary line graph showing the population trends. Close the app and check for memory leaks (valgrind/Task Manager). Please confirm functionality.

---

*Developer: Upon completing these phases, the game has transformed into a structured competitive E-sport. Ensure all changes are committed and the DoD from DEV_SPEC is met.*