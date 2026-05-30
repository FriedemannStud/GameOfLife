# Requirements Analysis & Specification: Surgical Code Recovery and Logical State Separation

This document details the requirements for restoring the interactive single-player features and implementing clean logical separation between Kiosk mode and Interactive mode, as described in **ADR-0020**.

**Related Documents:**
- [ADR-0020: Surgical Code Recovery](../adr/ADR-0020-surgical-code-recovery.md)
- [ADR-0019: Kiosk Mode State Machine and UI](../adr/ADR-0019-kiosk-mode-state-machine-and-ui.md)
- [ADR-0018: Multicam Render Context Architecture](../adr/ADR-0018-multicam-render-context-architecture.md)
- [DEV_SPEC-0019: Kiosk Mode State Machine](DEV_SPEC-0019-kiosk-mode-state-machine-and-ui.md)

---

### 1. Detailed Requirements Specification

#### 1.1 Problem Analysis — Current State

During the implementation of ADR-0018 (Multicam Render Contexts) and ADR-0019 (Kiosk Mode), the following regressions were introduced:

| Issue | Impact | Affected Code |
|:------|:-------|:-------------|
| Entry point hardcoded to `STATE_KIOSK_MODE` | Users cannot access the interactive single-player flow | `main.c:18` |
| Interactive states (`STATE_CONFIG`, `STATE_EDIT_RED/BLUE`) use **local `gui_world` pointers** instead of the `global_sim` SimulationContext | World allocations in `process_ui_events()` write to stack-local variables; the `SimulationContext`'s `current_world`/`next_world` pointers are never updated, causing a disconnect between the edit phase and the simulation phase | `renderer.c:412-831` |
| `process_ui_events()` passes `World**` pointers that reference `SimulationContext` fields, but the edit/config logic allocates new worlds via `create_world()` without updating the SimulationContext's double-buffer lifecycle | Memory is allocated for `gui_world` and `swap_world` but the `SimulationContext` fields (`world_a`, `world_b`, `current_world`, `next_world`) remain stale or NULL | `renderer.c:514-516` |
| `update_global_input()` 60s timeout forces transition to `STATE_KIOSK_MODE` without freeing dynamically allocated grids from an in-progress single-player session | Memory leak: worlds allocated during `STATE_CONFIG` → `STATE_EDIT_*` are orphaned when the timeout fires | `app_state_manager.c:39-42` |
| Kiosk and Interactive code share `timeAccumulator` and `ignitionStartTime` as `static` globals in different files | `renderer.c:365` has its own `ignitionStartTime`; `app_state_manager.c:9-10` has another pair. These can conflict during state transitions | Both files |
| No `[P]lay` shortcut hint in Kiosk mode to enter Config mode interactively | Users at the exhibition have no visible indication of how to start a new game | `renderer.c:1166-1224` |
| No "Back" button in Observer/Running mode to return to Kiosk | The only exit from a replay is the keyboard shortcut Q/Backspace, which goes to `STATE_CONFIG` instead of `STATE_KIOSK_MODE` | `renderer.c:707-724` |

#### 1.2 Requirements

**FR-01: Restore Interactive Entry Point**
- The application MUST start in `STATE_KIOSK_MODE` (exhibition default).
- The application MUST display a visible `[P]lay` keyboard shortcut hint during `STATE_KIOSK_MODE` to indicate the entry into interactive mode.
- Pressing `[P]` during Kiosk mode MUST trigger the transition to `STATE_CONFIG`.

**FR-02: Context-Aware World Lifecycle in Interactive States**
- When transitioning from `STATE_CONFIG` → `STATE_EDIT_RED`, the system MUST allocate worlds via `init_simulation_context()` or directly assign to `global_sim.current_world` and `global_sim.next_world`.
- The `process_ui_events()` function MUST operate on the `SimulationContext` pointers, NOT on disconnected local `World*` variables.
- All grid editing operations (`STATE_EDIT_RED`, `STATE_EDIT_BLUE`) MUST read/write from `global_sim.current_world->grid`.

**FR-03: Safe State Transitions with Memory Cleanup**
- When the 60s inactivity timeout fires in `update_global_input()`, the system MUST:
  1. Free any dynamically allocated `World` grids belonging to an active single-player session (`global_sim`).
  2. Free any telemetry history arrays (`config->history_red_pop`, `config->history_blue_pop`).
  3. Reset the `global_sim` to a clean state (NULL worlds, generation counter = 0).
  4. Reset `GameConfig` counters (current populations, current round).
  5. Only then transition to `STATE_KIOSK_MODE`.
- When the user presses `[Q]`/`[Backspace]` during `STATE_RUNNING` after a Kiosk replay (Click-to-Replay), the system MUST return to `STATE_KIOSK_MODE`, NOT to `STATE_CONFIG`.

**FR-04: Logical Separation of Kiosk and Interactive Code**
- The Kiosk sub-state logic (leaderboard cycling, multicam simulation ticking, click-to-replay) MUST remain encapsulated in `app_state_manager.c` (or a dedicated `kiosk_controller.c` translation unit).
- The Interactive state logic (config UI interaction, grid editing, pattern placement, file loading) MUST remain in `renderer.c` (or a dedicated `interactive_ui.c` translation unit).
- Shared state transitions (timeout, explicit mode switches) MUST use a well-defined transition function that handles cleanup.

**FR-05: "Back to Kiosk" Navigation from Replay**
- A visible `[K] BACK TO KIOSK` shortcut hint MUST be rendered during `STATE_RUNNING` and `STATE_OBSERVER` when the session originated from a Kiosk replay (Click-to-Replay).
- Pressing this button MUST:
  1. Free the single-player `SimulationContext` worlds.
  2. Free any telemetry arrays.
  3. Transition to `STATE_KIOSK_MODE`.
  4. Reset kiosk timers.

**FR-06: Kiosk Context Preservation**
- When transitioning from `STATE_KIOSK_MODE` to `STATE_CONFIG` (via `[P]` shortcut), the Kiosk contexts (`kiosk_sims[4]`, `kiosk_renders[4]`) MUST remain allocated but idle (not ticking).
- When returning to `STATE_KIOSK_MODE`, the Kiosk contexts MUST resume cycling without re-initialization (unless the window was resized).

**FR-07: Unified Timer Variables**
- The duplicate `ignitionStartTime` declarations in `renderer.c` and `app_state_manager.c` MUST be consolidated into a single authoritative location.
- The `timeAccumulator` in `app_state_manager.c` MUST be scoped to avoid interference between Kiosk simulation ticking and Interactive simulation ticking.

**NFR-01: Zero Compilation Warnings**
- All changes MUST compile without warnings under `gcc -Wall -Wextra -std=c99 -O3 -fopenmp`.

**NFR-02: No Memory Leaks on State Transitions**
- Every state transition path (timeout, manual exit, replay completion) MUST be verified to have no orphaned `malloc`/`calloc` allocations.
- Valgrind verification SHOULD be performed on the final implementation.

**NFR-03: Kiosk Uptime Stability**
- The application MUST be able to run continuously for at least 8 hours in Kiosk mode without memory growth or crashes (exhibition day scenario).

---

### 2. User Stories & Acceptance Criteria

**Epic: Restore Interactive Single-Player Flow Alongside Kiosk Mode**

*   **User Story 1: Start an Interactive Game from Kiosk Mode**
    *   **As a Uni-Messe visitor,** I want to see a clear `[P]lay` hint on the Kiosk screen and press `[P]` to start, **so that** I can configure and start my own Red vs. Blue match.
    *   **Acceptance Criteria:**
        *   A `PRESS [P] TO PLAY` text hint is visible at the bottom of both Kiosk sub-views (Leaderboard and Multicam).
        *   Pressing `[P]` transitions to `STATE_CONFIG`.
        *   The Config screen renders correctly with all settings (grid size, delay, max rounds, max population, presets).
        *   The Kiosk contexts remain in memory but stop ticking.

*   **User Story 2: Configure and Edit a Game**
    *   **As a player,** I want to configure the grid size and rules, then draw my Red and Blue patterns, **so that** I can set up a competitive match.
    *   **Acceptance Criteria:**
        *   Arrow keys change grid size; `+/-` changes delay; `PageUp/Down` changes max rounds; `Insert/Delete` changes max population.
        *   Pressing `[ENTER]` in Config allocates a new `World` via `global_sim` and transitions to `STATE_EDIT_RED`.
        *   Mouse drawing places/removes cells on the correct team's side.
        *   Pattern hotkeys (`[G]` Glider, `[T]` Traveler, `[B]` Blaster) work correctly.
        *   `[R]` randomizes the current team's side.
        *   `[ENTER]` in `STATE_EDIT_RED` transitions to `STATE_EDIT_BLUE`; `[ENTER]` in `STATE_EDIT_BLUE` auto-saves and starts Ignition.
        *   The grid renders correctly using the `global_render` RenderContext throughout the edit flow.

*   **User Story 3: Run and Observe a Simulation**
    *   **As a player,** I want to watch my configured match play out with real-time population counters, **so that** I can see which team wins.
    *   **Acceptance Criteria:**
        *   After Ignition countdown, the simulation runs at the configured delay.
        *   Population counters (Blue/Red), round counter, and the grid all render correctly.
        *   `[SPACE]` toggles pause.
        *   `[O]` enters Observer mode with pan/zoom.
        *   The simulation ends at max rounds or when a team is eliminated, transitioning to `STATE_FINISHED`.
        *   `[ENTER]` on `STATE_FINISHED` shows `STATE_GAME_OVER` with the telemetry graph.

*   **User Story 4: Return to Kiosk After Inactivity**
    *   **As the exhibition host,** I want the application to automatically return to Kiosk mode if a visitor walks away mid-game, **so that** the screen always shows the eye-catching display.
    *   **Acceptance Criteria:**
        *   If no mouse or keyboard input is detected for 60 seconds in any interactive state, the app transitions to `STATE_KIOSK_MODE`.
        *   All single-player `World` allocations are freed before the transition.
        *   All telemetry history arrays are freed.
        *   The Kiosk loop resumes immediately with the Leaderboard sub-state.
        *   No memory leak occurs (verifiable via valgrind).

*   **User Story 5: Return to Kiosk from a Replay**
    *   **As a visitor,** after I click a Multicam quadrant and watch the fullscreen replay, I want to press `[K]` to go back to the Kiosk loop, **so that** I don't get stuck in a dead-end state.
    *   **Acceptance Criteria:**
        *   During `STATE_RUNNING` or `STATE_OBSERVER` (when entered via Kiosk Click-to-Replay), a `[K] BACK TO KIOSK` shortcut hint is visible in the footer.
        *   Pressing `[K]` or `[Q]` returns to `STATE_KIOSK_MODE`.
        *   The replay worlds are freed, kiosk timers are reset, and the kiosk loop resumes.

*   **User Story 6: Return to Config from a Manual Game**
    *   **As a player,** after I manually configure and run a game, I want pressing `[Q]` to return me to the Config screen (not the Kiosk), **so that** I can start another game without going through the passive display.
    *   **Acceptance Criteria:**
        *   When `STATE_RUNNING` was reached via `STATE_CONFIG` → `STATE_EDIT_*` → `STATE_IGNITION`, pressing `[Q]` returns to `STATE_CONFIG`.
        *   Worlds and telemetry are freed.
        *   The Config screen renders correctly and is fully interactive.

*   **User Story 7: Load a Protocol File**
    *   **As a player,** I want to load a previously saved `.json` protocol file and replay it, **so that** I can re-examine past matches.
    *   **Acceptance Criteria:**
        *   Pressing `[L]` during `STATE_EDIT_RED` or `STATE_EDIT_BLUE` opens the Protocol Archive browser (`STATE_LOAD`).
        *   The file list renders correctly with preview metadata.
        *   Selecting a file and pressing `[ENTER]` loads the world and transitions back to the edit state.
        *   `[Q]`/`[ESC]` cancels and returns to the edit state.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP — required for Uni-Messe):**
        *   FR-02: Context-aware world lifecycle (fix the broken `SimulationContext` wiring)
        *   FR-03: Safe state transitions with memory cleanup (prevent leaks on timeout)
        *   FR-01: Restore interactive entry point (`[P]lay` hint in Kiosk)
        *   FR-05: "Back to Kiosk" navigation from replay
        *   FR-07: Unified timer variables (prevent state interference bugs)
        *   NFR-01: Zero compilation warnings
    *   **Should-Have:**
        *   FR-04: Logical separation of Kiosk and Interactive code (improves maintainability)
        *   FR-06: Kiosk context preservation (avoids re-initialization cost)
        *   NFR-02: No memory leaks (valgrind-verified)
    *   **Could-Have:**
        *   NFR-03: 8-hour kiosk uptime stress test
        *   Visual transition effects (fade/slide) between Kiosk → Config
    *   **Won't-Have (in this increment):**
        *   Full code extraction into separate translation units (`kiosk_controller.c`, `interactive_ui.c`)
        *   Puzzle/Tutorial mode recovery (`STATE_PUZZLE` remains non-essential)

*   **Dependencies:**

    ```mermaid
    graph TD
        A[FR-07: Unify Timer Variables] --> B[FR-02: Fix SimulationContext Wiring]
        B --> C[FR-03: Safe State Transitions]
        C --> D["FR-01: [P]lay Hint in Kiosk"]
        C --> E[FR-05: Back to Kiosk from Replay]
        D --> F[FR-06: Kiosk Context Preservation]
        B --> G[FR-04: Logical Code Separation]
        E --> G
    ```

    1.  **FR-07 → FR-02:** Timer consolidation must happen before fixing the simulation context, because both share `timeAccumulator` and `ignitionStartTime`.
    2.  **FR-02 → FR-03:** The SimulationContext wiring must be correct before safe cleanup can be implemented, since cleanup needs to know which pointers to free.
    3.  **FR-03 → FR-01, FR-05:** The cleanup/transition logic must be solid before adding new entry/exit points, to ensure every new transition path is safe.
    4.  **FR-02 → FR-04:** Code separation requires a clear understanding of which code touches which context.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority | Dependency |
| :-- | :--- | :--- | :--- | :--- |
| T-01 | Infrastructure | Consolidate `ignitionStartTime` and `timeAccumulator` into a single authoritative location. Add separate `kiosk_time_accumulator` for Kiosk sim ticking. | Must-Have | — |
| T-02 | Infrastructure | Add a `session_origin` field (enum: `ORIGIN_KIOSK_REPLAY`, `ORIGIN_INTERACTIVE`) to track how `STATE_RUNNING` was entered, enabling correct back-navigation. | Must-Have | — |
| T-03 | Core Recovery | Refactor `process_ui_events()` `STATE_CONFIG` → `STATE_EDIT_RED` transition to allocate worlds via `SimulationContext` (`global_sim`) instead of bare `create_world()` calls to local pointers. | Must-Have | T-01 |
| T-04 | Core Recovery | Refactor `process_ui_events()` to pass `SimulationContext*` instead of `World**` — all grid editing in `STATE_EDIT_RED/BLUE` reads/writes `sim_ctx->current_world->grid`. | Must-Have | T-03 |
| T-05 | Core Recovery | Verify that `STATE_LOAD` (Protocol Archive) correctly loads into the `SimulationContext` world and resizes the swap world accordingly. | Must-Have | T-04 |
| T-06 | Memory Safety | Implement `cleanup_interactive_session()` helper: frees `global_sim` worlds, frees telemetry arrays, resets config counters. | Must-Have | T-04 |
| T-07 | Memory Safety | Integrate `cleanup_interactive_session()` into all exit paths: (a) `update_global_input()` 60s timeout, (b) `[Q]` from `STATE_RUNNING`, (c) `[Q]` from `STATE_FINISHED`, (d) `[1]` from `STATE_GAME_OVER`. | Must-Have | T-06 |
| T-08 | Kiosk UI | Add `PRESS [P] TO PLAY` text hint to both Kiosk sub-views (Leaderboard and Multicam). Wire `[P]` keypress to transition to `STATE_CONFIG`. | Must-Have | T-06 |
| T-09 | Navigation | Use `session_origin` (T-02) to route `[Q]`/`[Backspace]` from `STATE_RUNNING`: if origin is `ORIGIN_KIOSK_REPLAY` → `STATE_KIOSK_MODE` + `reset_kiosk_timers()`; if origin is `ORIGIN_INTERACTIVE` → `STATE_CONFIG`. | Must-Have | T-02, T-07 |
| T-10 | Navigation | Add "BACK TO KIOSK `[K]`" hint in footer during `STATE_RUNNING`/`STATE_OBSERVER`. Wire `[K]` to always return to `STATE_KIOSK_MODE` with cleanup. | Should-Have | T-09 |
| T-11 | Kiosk | Ensure Kiosk contexts (`kiosk_sims`, `kiosk_renders`) remain allocated but idle when entering Interactive mode. Resume ticking without re-initialization on return. | Should-Have | T-08 |
| T-12 | Code Quality | Extract shared cleanup logic into `state_transitions.c` or inline helper functions to reduce duplication across state exit paths. | Should-Have | T-07 |
| T-13 | Verification | Compile with `make clean && make`. Ensure zero warnings under `-Wall -Wextra`. | Must-Have | T-09 |
| T-14 | Verification | Manual interactive test: full flow Config → Edit → Ignition → Running → Finished → Game Over → Config. | Must-Have | T-13 |
| T-15 | Verification | Manual Kiosk test: Kiosk cycling → Click-to-Replay → Back to Kiosk → Play → Config → Running → 60s timeout → Kiosk. | Must-Have | T-13 |
| T-16 | Verification | Valgrind memory leak check on state transition loops. | Should-Have | T-14, T-15 |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The code is written and formatted according to `docs/CODING_STYLE.md`:
    *   `snake_case` for variables and functions.
    *   `PascalCase` for structs and types.
    *   `UPPER_SNAKE_CASE` for constants and macros.
    *   All AI-generated or significantly modified code is marked with `// KI-Agent unterstützt`.
    *   Functions do not exceed ~50 lines; functions with >4 arguments use struct pointers.
*   **Compilation:** `make clean && make` produces zero warnings under `gcc -Wall -Wextra -std=c99 -O3 -fopenmp`.
*   **Context Analysis (Rule 10):** Before every code change, the developer has performed:
    1.  Identification of the element being changed.
    2.  Global search for all occurrences.
    3.  Analysis of every reference.
    4.  Atomic implementation of the change + all references.
    5.  Recompilation and verification.
*   **Interactive Verification:**
    *   The full interactive flow (Config → Edit Red → Edit Blue → Ignition → Running → Finished → Game Over → Config) works without crashes.
    *   The Kiosk loop (Leaderboard ↔ Multicam cycling) runs stably.
    *   The `[P]` shortcut in Kiosk mode correctly enters Config.
    *   The "Back to Kiosk" path from a replay works correctly.
    *   The 60-second inactivity timeout correctly returns to Kiosk from any interactive state without memory leaks.
*   **Memory Safety:** No orphaned `malloc`/`calloc` allocations on any state transition path. Valgrind run shows no definite leaks (if available).
*   **Acceptance Criteria:** All acceptance criteria for the relevant User Story have been met.
*   **Documentation:** `CHANGELOG.md` is updated. `DEV_TASKS-0020` checkboxes reflect progress.
