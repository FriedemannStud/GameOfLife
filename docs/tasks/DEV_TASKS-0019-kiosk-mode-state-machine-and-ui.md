# DEV_TASKS-0019: Kiosk Mode State Machine and UI

**Developer:** Please follow these steps precisely. This is the final integration phase for the Uni-Messe Kiosk. Ensure strict adherence to `CODING_STYLE.md`. The plan is broken into manageable phases. After each "Verification" step, report the outcome to confirm stability before proceeding.

**Briefing Documents:**
*   [ADR-0019: Kiosk Mode State Machine and UI](../adr/ADR-0019-kiosk-mode-state-machine-and-ui.md)
*   [DEV_SPEC-0019: Kiosk Mode State Machine and UI](../specs/DEV_SPEC-0019-kiosk-mode-state-machine-and-ui.md)
*   [DEV_TECH_DESIGN-0019: Technical Design: Kiosk Mode State Machine and UI](../tech_design/DEV_TECH_DESIGN-0019-kiosk-mode-state-machine-and-ui.md)

---

## Phase 1: State Machine Foundation & Leaderboard UI

*Goal: Establish the new Kiosk state and render the fetched Leaderboard data on screen.*

- [ ] **Step 1.1: Add `STATE_KIOSK_MODE`**
    - [ ] **Action:** Open `src/gui/app_state_manager.h`.
    - [ ] **Action:** Add `STATE_KIOSK_MODE` to the `AppState` enum.
    - [ ] **Action:** Open `src/gui/app_state_manager.c`. Add a `case STATE_KIOSK_MODE:` to the main update and draw switch statements.

- [ ] **Step 1.2: Implement Leaderboard Rendering**
    - [ ] **Action:** In `src/gui/app_state_manager.c` (or a dedicated UI file), create a function `void draw_leaderboard_ui(void)`.
    - [ ] **Action:** Inside this function, call `network_get_leaderboard(&local_data)`.
    - [ ] **Action:** Use Raylib's `DrawText` or `DrawTextEx` to loop through `local_data.entries` and print the Rank, Name, Elo, and Win Rate in a tabular format on screen. Add a stylized background rectangle.

- [ ] **Step 1.3: Default Startup State**
    - [ ] **Action:** In `src/apps/gui/main.c`, change the initial state from `STATE_CONFIG` (or equivalent) to `STATE_KIOSK_MODE`.
    - [ ] **Action:** In `STATE_KIOSK_MODE`'s update loop, call `network_fetch_leaderboard_async()` exactly ONCE when entering the state.
    - [ ] **Verification (Interactive Test):**
        1.  Start the Python backend.
        2.  Run: `make clean && make` and launch the C-Application.
        3.  **Expected Result:** The application opens directly into a screen displaying the live leaderboard data fetched from the backend.

---

## Phase 2: Autonomous Cycling (Leaderboard <-> Multicam)

*Goal: Implement the delta-time logic to cycle between the Leaderboard and the Multicam views automatically.*

- [ ] **Step 2.1: Define Kiosk Control Struct**
    - [ ] **Action:** In `src/gui/app_state_manager.c`, define the `KioskSubState` enum and `KioskController` struct as specified in `DEV_TECH_DESIGN-0019` (Section 3.1).
    - [ ] **Action:** Initialize a static instance: `static KioskController kiosk_ctrl;`. Set its initial state to `KIOSK_SUB_LEADERBOARD` and timer to `0.0`.

- [ ] **Step 2.2: Implement Timer Logic**
    - [ ] **Action:** In the update loop for `STATE_KIOSK_MODE`, accumulate `GetFrameTime()` into `kiosk_ctrl.state_timer`.
    - [ ] **Action:** If `state_timer > 15.0f` and current state is `LEADERBOARD`, switch to `MULTICAM`, reset timer to 0, and trigger `network_fetch_highlights_async()`.
    - [ ] **Action:** If `state_timer > 30.0f` and current state is `MULTICAM`, switch to `LEADERBOARD`, reset timer to 0, and trigger `network_fetch_leaderboard_async()`.

- [ ] **Step 2.3: Integrate Multicam Draw Logic**
    - [ ] **Action:** In the draw loop for `STATE_KIOSK_MODE`, use a `switch (kiosk_ctrl.current_sub_state)` statement.
    - [ ] **Action:** If `LEADERBOARD`, call `draw_leaderboard_ui()`.
    - [ ] **Action:** If `MULTICAM`, loop through `kiosk_ctrl.renders[4]` and call `draw_grid()` for each, as developed in DEV_TASKS-0018.
    - [ ] **Verification (Interactive Test):**
        1.  Compile and run the C-Application. Do not touch the mouse or keyboard.
        2.  **Expected Result:** The screen shows the leaderboard for 15 seconds, then automatically switches to a 4-way split screen (Multicam) for 30 seconds, and repeats this cycle endlessly.

---

## Phase 3: The Interrupts (Global Failsafe & Replay)

*Goal: Allow users to break out of the passive loop into an interactive simulation, and ensure the app heals itself if abandoned.*

- [ ] **Step 3.1: Global Inactivity Timer**
    - [ ] **Action:** In `src/gui/app_state_manager.c` (or `main.c`), add a `static double time_since_last_input = 0.0;`.
    - [ ] **Action:** Create `update_global_input(void)` as designed in `DEV_TECH_DESIGN-0019` (Section 3.2). Call it at the very top of the main loop, before the state switch.
    - [ ] **Action:** Add logic: `if (time_since_last_input > 60.0 && current_app_state != STATE_KIOSK_MODE) { current_app_state = STATE_KIOSK_MODE; reset_kiosk_timers(); }`.

- [ ] **Step 3.2: Verify Failsafe**
    - [ ] **Verification (Interactive Test):**
        1.  Run the application.
        2.  Force the app into an interactive state (e.g., press a key to go to `STATE_CONFIG` or `STATE_RUNNING`).
        3.  Wait exactly 60 seconds without touching the mouse or keyboard.
        4.  **Expected Result:** The application abruptly transitions back to the autonomous Kiosk Leaderboard.

- [ ] **Step 3.3: Click-to-Replay Mechanism**
    - [ ] **Action:** Inside the update loop for `STATE_KIOSK_MODE` -> `KIOSK_SUB_MULTICAM`, check for `IsMouseButtonPressed(MOUSE_LEFT_BUTTON)`.
    - [ ] **Action:** Iterate through the 4 `kiosk_ctrl.renders[i].viewport_bounds` using `CheckCollisionPointRec(GetMousePosition(), bounds)`.
    - [ ] **Action:** If a collision is found:
        - Copy the seed data from `kiosk_ctrl.sims[i]` into the global single-player `SimulationContext`.
        - Set `current_app_state = STATE_RUNNING`.
        - Reset `time_since_last_input = 0.0`.

- [ ] **Step 3.4: Final End-to-End Verification**
    - [ ] **Verification (Interactive Test):**
        1.  Run the application. Wait for it to cycle to the Multicam view.
        2.  Click on the bottom-right quadrant.
        3.  **Expected Result:** The application immediately jumps to a fullscreen view (`STATE_RUNNING`) displaying the exact simulation that was just in that bottom-right quadrant.
        4.  Wait 60 seconds without interacting.
        5.  **Expected Result:** The application heals back to the Kiosk Mode.

---

## Phase 4: Polish and Cleanup

*Goal: Finalize code quality before calling the Uni-Messe integration complete.*

- [ ] **Step 4.1: UI Button for Manual Return**
    - [ ] **Action:** In the standard `STATE_RUNNING` HUD, ensure there is a clear "Return to Kiosk / Main Menu" button so users don't have to wait 60 seconds to exit their replay.

- [ ] **Step 4.2: Code Quality Check**
    - [ ] **Action:** Run a final pass over `app_state_manager.c`. Ensure all AI-assisted code is tagged with `// KI-Agent unterstützt`.
    - [ ] **Action:** Run `make clean && make`. Ensure no warnings exist.

---
**Completion:** Once Phase 4 is verified, the Uni-Messe Kiosk Mode is fully implemented! Update `docs/CHANGELOG.md` to reflect the completion of WP4 and WP5.
