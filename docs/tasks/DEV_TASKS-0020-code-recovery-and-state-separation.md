# DEV_TASKS-0020: Surgical Code Recovery and Logical State Separation

> **Status: ABGESCHLOSSEN** — ADR-0020 wurde implementiert (zwischen ADR-0019 und ADR-0021). Verifiziert durch Code-Analyse: `SessionOrigin`, `reset_simulation_context()`, `cleanup_interactive_session()`, session-origin-aware Navigation und `[P]`/`[K]`-Shortcuts sind alle im Source-Code vorhanden. Die Checkboxen wurden während der Implementierung nicht verfolgt.

This task plan implements the restoration of the interactive single-player flow and the logical separation between Kiosk mode and Interactive mode. The plan is derived from ADR-0020, DEV_SPEC-0020, and DEV_TECH_DESIGN-0020.

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality.

**Coding Rules Reminder:**
- `snake_case` for variables and functions. `PascalCase` for structs/types. `UPPER_SNAKE_CASE` for constants/macros.
- Mark all AI-generated or significantly modified code with `// KI-Agent unterstützt`.
- Before every code change: Identify → Global Search → Analyze references → Implement atomically → Verify (CODING_STYLE.md, Rule 10).

**Briefing Documents:**
*   [ADR-0020: Surgical Code Recovery](../adr/ADR-0020-surgical-code-recovery.md)
*   [DEV_SPEC-0020: Requirements Specification](../specs/DEV_SPEC-0020-surgical-code-recovery.md)
*   [DEV_TECH_DESIGN-0020: Technical Design](../tech_design/DEV_TECH_DESIGN-0020-surgical-code-recovery.md)

---

## Phase 1: Infrastructure — New Types and Timer Consolidation

*Goal: Establish the foundational types and eliminate variable conflicts before touching any logic. The application MUST still compile and behave identically after this phase (no functional changes).*

- [ ] **Step 1.1: Add `SessionOrigin` enum to `core_types.h`**
    - [ ] **Action (Rule 10 — Identify):** The new enum `SessionOrigin` will be added to `src/core/core_types.h`. Search the project for any existing type named `SessionOrigin` to confirm it does not already exist.
        ```bash
        grep -rn "SessionOrigin" src/
        ```
    - [ ] **Action (Implement):** Open `src/core/core_types.h`. After the closing `} AppState;` line (line 34), add:
        ```c
        // KI-Agent unterstützt: Session origin tracking for back-navigation
        typedef enum {
            ORIGIN_NONE,           // No active session (Kiosk or Config idle)
            ORIGIN_INTERACTIVE,    // User entered via [P] → Config → Edit → Ignition
            ORIGIN_KIOSK_REPLAY    // User clicked a Multicam quadrant
        } SessionOrigin;
        ```
    - [ ] **Verification:** Run `make clean && make`. The build MUST succeed with zero warnings. The new enum is unused at this point — that is expected.

- [ ] **Step 1.2: Add `reset_simulation_context()` to `game_logic.h` and `game_logic.c`**
    - [ ] **Action (Rule 10 — Identify):** Search for all occurrences of `init_simulation_context` and `free_simulation_context` to understand the existing API surface:
        ```bash
        grep -rn "simulation_context" src/
        ```
    - [ ] **Action (Implement — Header):** Open `src/core/game_logic.h`. After the line declaring `void init_world(...)` (around line 11), add:
        ```c
        // KI-Agent unterstützt: Reset context for a new interactive session (fast path if dimensions match)
        void reset_simulation_context(SimulationContext *ctx, int rows, int cols);
        ```
    - [ ] **Action (Implement — Source):** Open `src/core/game_logic.c`. After the `free_simulation_context()` function (after line ~297), add the full implementation as specified in DEV_TECH_DESIGN-0020 §4.1:
        ```c
        // KI-Agent unterstützt: Reset simulation context for a new interactive session
        void reset_simulation_context(SimulationContext *ctx, int rows, int cols) {
            if (!ctx) return;

            // If dimensions match, just clear the grids (fast path)
            if (ctx->world_a && ctx->world_a->rows == rows && ctx->world_a->cols == cols) {
                int stride = cols + 2;
                int total = (rows + 2) * stride;
                memset(ctx->world_a->grid, 0, total * sizeof(int));
                memset(ctx->world_b->grid, 0, total * sizeof(int));
                if (ctx->world_a->chunk_map)
                    memset(ctx->world_a->chunk_map, 0,
                           ctx->world_a->chunk_rows * ctx->world_a->chunk_cols);
                if (ctx->world_b->chunk_map)
                    memset(ctx->world_b->chunk_map, 0,
                           ctx->world_b->chunk_rows * ctx->world_b->chunk_cols);
            } else {
                // Dimensions changed: full teardown + rebuild
                if (ctx->world_a) free_world(ctx->world_a);
                if (ctx->world_b) free_world(ctx->world_b);
                ctx->world_a = create_world(rows, cols);
                ctx->world_b = create_world(rows, cols);
            }

            ctx->rows = rows;
            ctx->cols = cols;
            ctx->current_generation = 0;
            ctx->max_generations = MAX_ROUNDS;
            ctx->current_world = ctx->world_a;
            ctx->next_world = ctx->world_b;
            ctx->is_active = false;
        }
        ```
    - [ ] **Verification:** Run `make clean && make`. Zero warnings expected. The new function is unused — that is expected.

- [ ] **Step 1.3: Consolidate `ignitionStartTime` — Add getter/setter in `app_state_manager`**
    - [ ] **Action (Rule 10 — Identify):** Search for all occurrences of `ignitionStartTime` across the project:
        ```bash
        grep -rn "ignitionStartTime" src/
        ```
        Expected: two `static double ignitionStartTime` declarations — one in `renderer.c` (around line 365) and one in `app_state_manager.c` (around line 9).
    - [ ] **Action (Implement — Header):** Open `src/gui/app_state_manager.h`. After the `void reset_kiosk_timers(void);` declaration, add:
        ```c
        // KI-Agent unterstützt: Ignition time accessors (single source of truth)
        double get_ignition_start_time(void);
        void set_ignition_start_time(double t);
        ```
    - [ ] **Action (Implement — Source):** Open `src/gui/app_state_manager.c`. After the `static double ignitionStartTime = 0.0;` declaration (line 9), add:
        ```c
        // KI-Agent unterstützt: Ignition time accessors
        double get_ignition_start_time(void) { return ignitionStartTime; }
        void set_ignition_start_time(double t) { ignitionStartTime = t; }
        ```
    - [ ] **Action (Implement — Remove duplicate):** Open `src/gui/renderer.c`. **Delete** the line `static double ignitionStartTime = 0.0;` (around line 365).
    - [ ] **Action (Implement — Update references in renderer.c):** Replace all occurrences of the raw `ignitionStartTime` variable in `renderer.c` with calls to `get_ignition_start_time()` or `set_ignition_start_time()`. Search for them:
        ```bash
        grep -n "ignitionStartTime" src/gui/renderer.c
        ```
        Typical replacements:
        - `if (ignitionStartTime == 0.0) ignitionStartTime = GetTime();` → `if (get_ignition_start_time() == 0.0) set_ignition_start_time(GetTime());`
        - `double elapsed = GetTime() - ignitionStartTime;` → `double elapsed = GetTime() - get_ignition_start_time();`
        - `ignitionStartTime = 0.0;` → `set_ignition_start_time(0.0);`
    - [ ] **Verification:** Run `make clean && make`. Zero warnings expected. Run the application and confirm:
        1.  The Kiosk mode starts normally.
        2.  The Kiosk cycles between Leaderboard and Multicam.
        3.  **Expected Result:** Identical behavior to before. No functional change.

- [ ] **Step 1.4: Scope the `timeAccumulator` in `app_state_manager.c`**
    - [ ] **Action (Rule 10 — Identify):** Search for `timeAccumulator` across the project:
        ```bash
        grep -rn "timeAccumulator" src/
        ```
    - [ ] **Action (Implement):** In `src/gui/app_state_manager.c`, rename `timeAccumulator` (line 10) to `kiosk_time_accumulator`. Update all references within the file (expect ~3 occurrences: the declaration, the MULTICAM tick block, and the RUNNING tick block).
        - **Important:** The `timeAccumulator` used in the `STATE_RUNNING`/`STATE_OBSERVER` block for interactive sim ticking should be renamed to `interactive_time_accumulator` (a new `static float` at the same location).
    - [ ] **Verification:** Run `make clean && make`. Zero warnings. Run the app. Confirm Kiosk cycling and Multicam simulation ticking still work.

- [ ] **Step 1.5: Phase 1 Checkpoint — Commit**
    - [ ] **Action:** Commit all changes with message: `refactor: Phase 1 — Add SessionOrigin, reset_simulation_context, consolidate timers (ADR-0020)`
    - [ ] **Verification:** `git status` shows a clean working tree.

---

## Phase 2: Refactor `process_ui_events()` Signature

*Goal: Change `process_ui_events()` to accept `SimulationContext*` instead of `World**`. This is a breaking API change — both the declaration, implementation, and call site must be updated atomically.*

- [ ] **Step 2.1: Rule 10 Analysis — Identify all references to `process_ui_events`**
    - [ ] **Action:** Perform a global search:
        ```bash
        grep -rn "process_ui_events" src/
        ```
        Create a checklist of all locations:
        - `renderer.h` (declaration)
        - `renderer.c` (implementation)
        - `main.c` (call site)

- [ ] **Step 2.2: Update the declaration in `renderer.h`**
    - [ ] **Action:** Open `src/gui/renderer.h`. Change line 36 from:
        ```c
        AppState process_ui_events(AppState current_state, GameConfig* config, World** p_current_world, World** p_swap_world, RenderContext *r_ctx);
        ```
        To:
        ```c
        // KI-Agent unterstützt: Refactored to use SimulationContext instead of raw World**
        AppState process_ui_events(AppState current_state, GameConfig* config, SimulationContext *sim_ctx, RenderContext *r_ctx, SessionOrigin *session_origin);
        ```

- [ ] **Step 2.3: Update the implementation signature in `renderer.c`**
    - [ ] **Action:** Open `src/gui/renderer.c`. Change the function signature (around line 412) to match the new declaration.
    - [ ] **Action:** Replace the local pointer aliases at the top of the function:
        ```c
        // BEFORE:
        World* gui_world = *p_current_world;
        World* swap_world = *p_swap_world;

        // AFTER:
        World* gui_world = sim_ctx->current_world;
        ```
    - [ ] **Action:** **Remove** the write-back lines at the end of the function (around lines 828-829):
        ```c
        // DELETE these lines:
        *p_current_world = gui_world;
        *p_swap_world = swap_world;
        ```
    - [ ] **Action:** At the very end of the function, before `return state;`, update `gui_world` back into the context for any code that reassigned `gui_world` locally:
        - Search for all `gui_world = ` assignments in the function. These are the critical points where the local pointer diverges from the context.
        - For each assignment (e.g., `gui_world = create_world(...)` or `gui_world = NULL`), the logic will be refactored in Phase 3. For now, keep the assignments but ensure they also update `sim_ctx->current_world`.

- [ ] **Step 2.4: Update the call site in `main.c`**
    - [ ] **Action:** Open `src/apps/gui/main.c`. Add a new global variable after `static RenderContext global_render;` (line 33):
        ```c
        static SessionOrigin session_origin = ORIGIN_NONE;
        ```
    - [ ] **Action:** Update `MainLoopStep()` (line 38) from:
        ```c
        state = process_ui_events(state, &config, &global_sim.current_world, &global_sim.next_world, &global_render);
        ```
        To:
        ```c
        state = process_ui_events(state, &config, &global_sim, &global_render, &session_origin);
        ```

- [ ] **Step 2.5: Compile and quick-test**
    - [ ] **Verification:** Run `make clean && make`. Fix any compiler errors. Zero warnings required.
    - [ ] **Verification (Interactive Test):**
        1.  Start the application. The Kiosk mode must launch.
        2.  Wait for the Multicam view. Click a quadrant.
        3.  **Expected Result:** The Click-to-Replay ignition countdown and simulation still work (because the `sim_ctx->current_world` pointer is set by the Kiosk replay code in `app_state_manager.c` before this function is called).

- [ ] **Step 2.6: Phase 2 Checkpoint — Commit**
    - [ ] **Action:** Commit: `refactor: Phase 2 — process_ui_events accepts SimulationContext* (ADR-0020)`

---

## Phase 3: Fix World Lifecycle in Interactive States

*Goal: Ensure CONFIG → EDIT_RED → EDIT_BLUE → IGNITION flow correctly allocates and uses worlds through `SimulationContext`, not through disconnected local pointers.*

- [ ] **Step 3.1: Fix STATE_CONFIG → STATE_EDIT_RED world allocation**
    - [ ] **Action (Rule 10 — Identify):** Search for `create_world` calls in `renderer.c`:
        ```bash
        grep -n "create_world" src/gui/renderer.c
        ```
    - [ ] **Action (Implement):** In `renderer.c`, in the `STATE_CONFIG` case, replace the `KEY_ENTER` block (around lines 512-530) as specified in DEV_TECH_DESIGN-0020 §5.5C:
        - Replace `gui_world = create_world(...)` + `swap_world = create_world(...)` + manual grid clearing with:
            ```c
            // KI-Agent unterstützt: Allocate via SimulationContext
            reset_simulation_context(sim_ctx, config->rows, config->cols);
            gui_world = sim_ctx->current_world;
            ```
        - Add: `*session_origin = ORIGIN_INTERACTIVE;`
        - Keep the existing population/round resets.
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 3.2: Fix STATE_EDIT_BLUE → STATE_IGNITION save call**
    - [ ] **Action:** In `renderer.c`, in the `STATE_EDIT_BLUE` branch of the `KEY_ENTER` handler (around line 668), replace:
        ```c
        save_grid(autoFilename, gui_world, config);
        ```
        With:
        ```c
        save_grid(autoFilename, sim_ctx->current_world, config);
        ```
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 3.3: Fix STATE_LOAD file loading to use SimulationContext**
    - [ ] **Action:** In `renderer.c`, in the `STATE_LOAD` `KEY_ENTER` handler (around lines 682-694), replace as specified in DEV_TECH_DESIGN-0020 §5.5F:
        - Replace `load_grid(..., gui_world, ...)` with `load_grid(..., sim_ctx->current_world, ...)`
        - Replace `swap_world = create_world(...)` with:
            ```c
            // KI-Agent unterstützt: Rebuild swap world via context
            if (sim_ctx->world_b) free_world(sim_ctx->world_b);
            sim_ctx->world_b = create_world(config->rows, config->cols);
            sim_ctx->next_world = sim_ctx->world_b;
            ```
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 3.4: Remove remaining `swap_world` local variable usage**
    - [ ] **Action:** Search for any remaining uses of the local `swap_world` variable in `renderer.c`:
        ```bash
        grep -n "swap_world" src/gui/renderer.c
        ```
    - [ ] **Action:** Ensure no code path reads or writes `swap_world` as a disconnected local variable. All swap-world access must go through `sim_ctx->next_world`.
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 3.5: Interactive Verification — Full Interactive Flow**
    - [ ] **Verification (Interactive Test):**
        1.  Start the application. You will be in Kiosk mode.
        2.  Press `[K]` on the keyboard (this is the existing CONFIG shortcut from the KIOSK_MODE case in `process_ui_events`). You should enter `STATE_CONFIG`.
        3.  In Config, press `[ENTER]`. You should enter `STATE_EDIT_RED`.
        4.  Draw some red cells by clicking on the right half of the grid.
        5.  Press `[ENTER]`. You should enter `STATE_EDIT_BLUE`.
        6.  Draw some blue cells on the left half.
        7.  Press `[ENTER]`. The Ignition countdown should appear.
        8.  After 3 seconds, the simulation should start running.
        9.  **Expected Result:** The simulation runs correctly. Red and blue cells evolve. Population counters update. No crash.
        10. Press `[Q]`. You should return to `STATE_CONFIG`.

- [ ] **Step 3.6: Phase 3 Checkpoint — Commit**
    - [ ] **Action:** Commit: `fix: Phase 3 — World lifecycle via SimulationContext in interactive states (ADR-0020)`

---

## Phase 4: Implement `cleanup_interactive_session()` and Safe Exit Paths

*Goal: Create a centralized cleanup function and integrate it into every exit path to prevent memory leaks.*

- [ ] **Step 4.1: Implement `cleanup_interactive_session()` in `app_state_manager`**
    - [ ] **Action (Implement — Header):** Open `src/gui/app_state_manager.h`. Add after the existing declarations:
        ```c
        // KI-Agent unterstützt: Centralized cleanup for interactive sessions
        void cleanup_interactive_session(SimulationContext *sim, GameConfig *config,
                                         RenderContext *r_ctx);
        ```
    - [ ] **Action (Implement — Source):** Open `src/gui/app_state_manager.c`. Add the full implementation as specified in DEV_TECH_DESIGN-0020 §4.2 (the function frees worlds, telemetry, resets counters, resets camera).
    - [ ] **Verification:** Run `make clean && make`. Zero warnings. Function is unused at this point — expected.

- [ ] **Step 4.2: Update `update_global_input()` signature and add cleanup**
    - [ ] **Action (Rule 10 — Identify):** Search for all references:
        ```bash
        grep -rn "update_global_input" src/
        ```
    - [ ] **Action (Implement — Header):** In `app_state_manager.h`, update the declaration:
        ```c
        void update_global_input(AppState* current_app_state, SimulationContext *sim,
                                 GameConfig *config, RenderContext *r_ctx);
        ```
    - [ ] **Action (Implement — Source):** In `app_state_manager.c`, update the function signature. Inside the 60s timeout block, add `cleanup_interactive_session(sim, config, r_ctx);` **before** `*current_app_state = STATE_KIOSK_MODE;`.
    - [ ] **Action (Implement — Call site):** In `main.c`, update the `MainLoopStep()` call:
        ```c
        update_global_input(&state, &global_sim, &config, &global_render);
        ```
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 4.3: Replace manual cleanup in `STATE_RUNNING` [Q] exit with `cleanup_interactive_session()`**
    - [ ] **Action:** In `renderer.c`, in the `STATE_RUNNING` case `KEY_Q`/`KEY_BACKSPACE` handler (around lines 708-724), replace the manual `free_world`, `free(history_*)`, camera reset lines with:
        ```c
        // KI-Agent unterstützt: Route based on session origin
        cleanup_interactive_session(sim_ctx, config, r_ctx);
        set_ignition_start_time(0.0);
        state = STATE_CONFIG; // Will be updated in Phase 5 for session-origin routing
        break;
        ```
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 4.4: Replace manual cleanup in `STATE_FINISHED` [Q] exit**
    - [ ] **Action:** In `renderer.c`, in the `STATE_FINISHED` case `KEY_Q` handler (around lines 790-804), replace all manual cleanup with:
        ```c
        cleanup_interactive_session(sim_ctx, config, r_ctx);
        set_ignition_start_time(0.0);
        state = STATE_CONFIG;
        ```
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 4.5: Replace manual cleanup in `STATE_GAME_OVER` [1] exit**
    - [ ] **Action:** In `renderer.c`, in the `STATE_GAME_OVER` case `KEY_ONE` handler (around lines 808-822), replace all manual cleanup with:
        ```c
        cleanup_interactive_session(sim_ctx, config, r_ctx);
        set_ignition_start_time(0.0);
        state = STATE_CONFIG;
        ```
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 4.6: Interactive Verification — Exit Paths**
    - [ ] **Verification (Interactive Test):**
        1.  Start app → Enter Config (press K from Kiosk) → Edit Red → Edit Blue → Ignition → Running.
        2.  Press `[Q]` during Running. You should return to Config. **No crash**.
        3.  Repeat steps 1-2 three times in a row. **No crash, no visual glitches**.
        4.  Start a game, let it run to completion (STATE_FINISHED). Press `[ENTER]` to see Game Over. Press `[1]`.
        5.  **Expected Result:** You return to Config cleanly each time. No memory errors.

- [ ] **Step 4.7: Phase 4 Checkpoint — Commit**
    - [ ] **Action:** Commit: `fix: Phase 4 — Centralized cleanup_interactive_session for all exit paths (ADR-0020)`

---

## Phase 5: Session-Origin-Aware Navigation

*Goal: [Q] routes back to CONFIG for interactive sessions, and back to KIOSK_MODE for replay sessions. [K] always returns to Kiosk.*

- [ ] **Step 5.1: Update `update_app_state()` to accept `SessionOrigin*`**
    - [ ] **Action (Rule 10 — Identify):** Search:
        ```bash
        grep -rn "update_app_state" src/
        ```
    - [ ] **Action (Implement — Header):** In `app_state_manager.h`, update the declaration:
        ```c
        AppState update_app_state(AppState current_state, GameConfig* config,
                                  SimulationContext *sim_ctx, float delta_time,
                                  double current_time, SessionOrigin *session_origin);
        ```
    - [ ] **Action (Implement — Source):** In `app_state_manager.c`, update the function signature. In the Click-to-Replay block (around line 215, before `return STATE_IGNITION;`), add:
        ```c
        *session_origin = ORIGIN_KIOSK_REPLAY;
        ```
    - [ ] **Action (Implement — Call site):** In `main.c`, update the call:
        ```c
        state = update_app_state(state, &config, &global_sim, GetFrameTime(),
                                 GetTime(), &session_origin);
        ```
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 5.2: Implement session-origin-aware routing in `STATE_RUNNING` [Q] exit**
    - [ ] **Action:** In `renderer.c`, in the `STATE_RUNNING` `KEY_Q`/`KEY_BACKSPACE` handler, replace the fixed `state = STATE_CONFIG;` with the session-origin routing:
        ```c
        cleanup_interactive_session(sim_ctx, config, r_ctx);
        set_ignition_start_time(0.0);
        if (*session_origin == ORIGIN_KIOSK_REPLAY) {
            state = STATE_KIOSK_MODE;
            reset_kiosk_timers();
        } else {
            state = STATE_CONFIG;
        }
        *session_origin = ORIGIN_NONE;
        break;
        ```
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 5.3: Add `[K]` shortcut to always return to Kiosk from RUNNING/OBSERVER**
    - [ ] **Action:** In `renderer.c`, in the `STATE_RUNNING` case, **after** the `KEY_Q` block and **before** the `KEY_SPACE` toggle, add:
        ```c
        // KI-Agent unterstützt: [K] always returns to Kiosk
        if (IsKeyPressed(KEY_K)) {
            cleanup_interactive_session(sim_ctx, config, r_ctx);
            set_ignition_start_time(0.0);
            state = STATE_KIOSK_MODE;
            reset_kiosk_timers();
            *session_origin = ORIGIN_NONE;
            break;
        }
        ```
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 5.4: Interactive Verification — Navigation Routing**
    - [ ] **Verification (Interactive Test — Replay Flow):**
        1.  Start app in Kiosk mode. Wait for Multicam view.
        2.  Click a Multicam quadrant. Ignition countdown → Simulation runs.
        3.  Press `[Q]`. **Expected: Returns to Kiosk mode** (NOT to Config).
        4.  Verify the Kiosk loop resumes cycling (Leaderboard → Multicam).
    - [ ] **Verification (Interactive Test — Interactive Flow):**
        1.  From Kiosk, press `[K]` to enter Config.
        2.  Configure and run a full game (Config → Edit Red → Edit Blue → Ignition → Running).
        3.  Press `[Q]`. **Expected: Returns to Config** (NOT to Kiosk).
    - [ ] **Verification (Interactive Test — [K] shortcut):**
        1.  From Config, start a game and reach Running.
        2.  Press `[K]`. **Expected: Returns to Kiosk mode**.

- [ ] **Step 5.5: Phase 5 Checkpoint — Commit**
    - [ ] **Action:** Commit: `feat: Phase 5 — Session-origin-aware back-navigation (ADR-0020)`

---

## Phase 6: `[P]lay` Shortcut Hint and Kiosk Entry

*Goal: Add visible `PRESS [P] TO PLAY` text in Kiosk mode and wire the `[P]` key to enter CONFIG.*

- [ ] **Step 6.1: Add `[P]` input handling in `process_ui_events`**
    - [ ] **Action:** In `renderer.c`, in the `STATE_KIOSK_MODE` case of `process_ui_events()` (currently just `break;` around line 824-825), replace with:
        ```c
        case STATE_KIOSK_MODE:
            // KI-Agent unterstützt: [P] keyboard shortcut to enter interactive mode
            if (IsKeyPressed(KEY_P)) {
                state = STATE_CONFIG;
                *session_origin = ORIGIN_NONE;
            }
            break;
        ```
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 6.2: Add `PRESS [P] TO PLAY` text hint in `draw_current_state`**
    - [ ] **Action:** In `renderer.c`, in the `STATE_KIOSK_MODE` case of `draw_current_state()`:
        - At the **end** of the `KIOSK_SUB_LEADERBOARD` draw block (just before the `else if` for MULTICAM), add:
            ```c
            // KI-Agent unterstützt: Play shortcut hint for interactive entry
            DrawText("PRESS [P] TO PLAY",
                     screenWidth / 2 - MeasureText("PRESS [P] TO PLAY", 20) / 2,
                     screenHeight - 100, 20, THEME_ACCENT);
            ```
        - Add the **same** `DrawText` call at the **end** of the `KIOSK_SUB_MULTICAM` draw block (just before the closing `}`).
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 6.3: Add `[K] BACK TO KIOSK` hint in Running/Observer footer**
    - [ ] **Action:** In `renderer.c`, in the `draw_current_state()` `STATE_RUNNING`/`STATE_OBSERVER` case, locate the footer `DrawText(simFooter, ...)` call (around line 1096). The footer format strings already exist. Append ` | [K] KIOSK` to each format string. For example:
        ```c
        sprintf(simFooter, "RUNNING | [SPACE] PAUSE SIM | [O] OBSERVER | [Q] ABORT | [K] KIOSK");
        ```
        Do this for all 4 format string variants (paused/running × running/observer).
    - [ ] **Verification:** Run `make clean && make`. Zero warnings.

- [ ] **Step 6.4: Interactive Verification — Play Shortcut**
    - [ ] **Verification (Interactive Test):**
        1.  Start app. You are in Kiosk mode.
        2.  Verify `PRESS [P] TO PLAY` text is visible at the bottom of the Leaderboard view.
        3.  Wait for the Multicam view. Verify the same text is visible.
        4.  Press `[P]`. **Expected: Transitions to CONFIG screen**.
        5.  Verify the Config screen renders correctly with all parameters.
        6.  Press `[K]` from Config. **Expected: Returns to Kiosk**.

- [ ] **Step 6.5: Phase 6 Checkpoint — Commit**
    - [ ] **Action:** Commit: `feat: Phase 6 — [P]lay shortcut hint and Kiosk entry/exit (ADR-0020)`

---

## Phase 7: Kiosk Context Preservation

*Goal: Ensure Kiosk simulation contexts remain allocated when entering Interactive mode and resume seamlessly on return.*

- [ ] **Step 7.1: Verify Kiosk contexts persist during Interactive mode**
    - [ ] **Action:** Review the code in `app_state_manager.c` `STATE_KIOSK_MODE` initialization block (lines 125-142). It checks `if (!kiosk_ctrl.initialized)` and only allocates on first entry. Confirm this guard is present and correct.
    - [ ] **Action:** Ensure that `cleanup_interactive_session()` does **NOT** touch `kiosk_ctrl`, `kiosk_sims`, or `kiosk_renders`. Search:
        ```bash
        grep -n "kiosk" src/gui/app_state_manager.c | grep -i "cleanup"
        ```
        There should be zero results.
    - [ ] **Verification:** Confirmed by code inspection — no action needed if the guard is already correct.

- [ ] **Step 7.2: Interactive Verification — Seamless Kiosk Resume**
    - [ ] **Verification (Interactive Test):**
        1.  Start app → Kiosk runs → wait for Multicam → observe simulations ticking.
        2.  Press `[P]` → Config → Edit Red → Edit Blue → Ignition → Running.
        3.  Press `[K]` to return to Kiosk.
        4.  **Expected Result:** Kiosk resumes immediately. The Multicam simulations continue from where they left off (or are refreshed with new data). No crash, no blank screen.
        5.  Repeat the cycle 3 times.

- [ ] **Step 7.3: Phase 7 Checkpoint — Commit (if any code changes were needed)**
    - [ ] **Action:** Commit if changes were made: `fix: Phase 7 — Kiosk context preservation verified (ADR-0020)`

---

## Phase 8: Comprehensive Verification and Hardening

*Goal: Systematically test every state transition path and verify zero memory leaks.*

- [ ] **Step 8.1: Full Interactive Flow Test**
    - [ ] **Verification (Interactive Test):**
        1.  Start app → `[P]` → Config → `[ENTER]` → Edit Red → draw cells → `[ENTER]` → Edit Blue → draw cells → `[ENTER]` → Ignition (3s) → Running.
        2.  Watch the simulation run to completion (STATE_FINISHED).
        3.  Press `[ENTER]` → Game Over screen with telemetry graph.
        4.  Press `[1]` → back to Config.
        5.  **Expected Result:** Full cycle completes without crash. Populations counted correctly. Graph displays.

- [ ] **Step 8.2: Replay Flow Test**
    - [ ] **Verification (Interactive Test):**
        1.  Start app → Kiosk → wait for Multicam → click a quadrant.
        2.  Ignition → Running. Observe simulation.
        3.  Press `[Q]`. **Expected:** Returns to Kiosk.
        4.  Click another quadrant. Ignition → Running.
        5.  Press `[K]`. **Expected:** Returns to Kiosk.
        6.  Repeat 3 times.
        7.  **Expected Result:** All transitions clean, no crash, Kiosk resumes each time.

- [ ] **Step 8.3: Inactivity Timeout Test**
    - [ ] **Verification (Interactive Test):**
        1.  Press `[P]` → Config. **Do not touch keyboard or mouse for 60+ seconds**.
        2.  **Expected:** App automatically returns to Kiosk mode.
        3.  Press `[P]` → Config → Edit Red → draw some cells → **wait 60+ seconds**.
        4.  **Expected:** App returns to Kiosk mode. Previously drawn cells are cleaned up.
        5.  Verify Kiosk resumes normally.

- [ ] **Step 8.4: Protocol Archive (Load) Test**
    - [ ] **Verification (Interactive Test):**
        1.  Run a complete game to generate a protocol file in `biotope_results/`.
        2.  Start a new game: Config → Edit Red.
        3.  Press `[L]` to open Protocol Archive.
        4.  Select the file, press `[ENTER]`.
        5.  **Expected:** World loads correctly. Edit mode shows loaded pattern.
        6.  Continue to Edit Blue → Ignition → Running.
        7.  **Expected Result:** Simulation runs with the loaded pattern.

- [ ] **Step 8.5: Rapid State Switching Stress Test**
    - [ ] **Verification (Interactive Test):**
        1.  Rapidly press `[P]` → `[K]` → `[P]` → `[K]` (10 times fast).
        2.  Press `[P]` → `[ENTER]` → `[Q]` → `[P]` → `[ENTER]` → `[Q]` (5 times fast).
        3.  **Expected Result:** No crash, no freeze, no visual corruption.

- [ ] **Step 8.6: Compilation Final Check**
    - [ ] **Verification:** Run `make clean && make`. Confirm output shows zero warnings. All three targets (`biotope`, `biotope_headless`, `biotope_hyper`) build successfully.

- [ ] **Step 8.7: Valgrind Memory Check (if available)**
    - [ ] **Action:** If Valgrind is installed, run:
        ```bash
        valgrind --tool=memcheck --leak-check=full --show-leak-kinds=definite \
            ./build/biotope 2>&1 | tee valgrind.log
        ```
    - [ ] **Action:** During the Valgrind run, exercise the following cycle:
        - Kiosk → `[P]` → Config → Edit → Running → `[Q]` → Config → `[K]` → Kiosk → Close window.
    - [ ] **Verification:** Check `valgrind.log` for "definitely lost" blocks. The count should be 0 for allocations made by our code (Raylib/OpenGL internal allocations are expected and can be ignored).

- [ ] **Step 8.8: Phase 8 Checkpoint — Final Commit**
    - [ ] **Action:** Commit: `test: Phase 8 — Comprehensive verification passed (ADR-0020)`

---

## Phase 9: Documentation Update

*Goal: Update CHANGELOG and mark tasks as complete.*

- [ ] **Step 9.1: Update CHANGELOG.md**
    - [ ] **Action:** Open `docs/CHANGELOG.md`. Add a new entry at the top:
        ```markdown
        ## [ADR-0020] Surgical Code Recovery and State Separation — 2026-05-29

        ### Fixed
        - Restored interactive single-player flow (Config → Edit → Running → Game Over)
        - Fixed `SimulationContext` world lifecycle: worlds now allocated/freed through
          context ownership model instead of disconnected local pointers
        - Fixed memory leak on 60-second inactivity timeout (worlds and telemetry now freed)
        - Consolidated duplicate `ignitionStartTime` and `timeAccumulator` variables
        - Session-origin-aware `[Q]` routing: returns to Config (interactive) or Kiosk (replay)

        ### Added
        - `SessionOrigin` enum for tracking how STATE_RUNNING was entered
        - `reset_simulation_context()` for efficient context reuse
        - `cleanup_interactive_session()` centralized cleanup function
        - `PRESS [P] TO PLAY` hint in Kiosk mode for interactive entry
        - `[K]` shortcut to return to Kiosk from any running state
        - `[K] KIOSK` hint in Running/Observer footer

        ### Related
        - ADR-0020, DEV_SPEC-0020, DEV_TECH_DESIGN-0020
        ```

- [ ] **Step 9.2: Mark DEV_TASKS-0020 as complete**
    - [ ] **Action:** Go through this file and mark all completed checkboxes with `[x]`.

- [ ] **Step 9.3: Final Commit**
    - [ ] **Action:** Commit: `docs: Phase 9 — CHANGELOG and task completion (ADR-0020)`
    - [ ] **Action:** Push the `kiosk` branch:
        ```bash
        git push origin kiosk
        ```
