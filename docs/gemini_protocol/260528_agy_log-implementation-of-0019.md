# Implementation Log: Kiosk Mode State Machine and UI (DEV_TASKS-0019)

## Overview
This document summarizes the development steps, technical decisions, and bug fixes taken during the implementation of the Uni-Messe Kiosk Mode (`ADR-0019`) for the Biotope/Game of Life application. 

## Development Phases

### Phase 1: State Machine Foundation & Leaderboard UI
- **Goal:** Introduce `STATE_KIOSK_MODE` and render dynamic leaderboard data.
- **Implementation:**
  - Added `STATE_KIOSK_MODE` to the `AppState` enum.
  - Set the application's default startup state in `main.c` to `STATE_KIOSK_MODE`.
  - Implemented tabular rendering of the Global Leaderboard in `renderer.c` by consuming network data cached by `app_state_manager.c`.

### Phase 2: Autonomous Cycling (Leaderboard <-> Multicam)
- **Goal:** Automate the rotation between Leaderboard and Multicam 2x2 splitscreen views.
- **Implementation:**
  - Introduced `KioskController` inside `app_state_manager.c` to track `kiosk_ctrl.state_timer` and `kiosk_ctrl.current_sub_state` (`KIOSK_SUB_LEADERBOARD` vs `KIOSK_SUB_MULTICAM`).
  - Added cycle logic: The app stays in the Leaderboard state for 15 seconds, then switches to the Multicam simulation for 30 seconds.
  - Network requests (`network_fetch_leaderboard_async` and `network_fetch_highlights_async`) are seamlessly triggered upon state transitions to ensure fresh data.
  - Network responses are explicitly cached in `kiosk_ctrl` to avoid re-fetching or dropping data during the draw loop.

### Phase 3: Interactive Interrupts & Failsafe
- **Goal:** Enable user interaction (Replay) and prevent the app from getting stuck if abandoned by users.
- **Implementation:**
  - **Click-to-Replay:** Added logic inside `app_state_manager.c` so that clicking on any of the 4 Multicam quadrants copies its seed data into the global single-player `SimulationContext`. The game then immediately transitions to `STATE_IGNITION` to begin a fullscreen interactive replay.
  - **Global Inactivity Failsafe:** Implemented `update_global_input()`. It monitors mouse clicks, mouse movement, and keyboard activity. If 60 seconds pass with zero user input, it forcefully resets the app state back to `STATE_KIOSK_MODE` and calls `reset_kiosk_timers()`.

## Debugging and Bug Fixes
During interactive QA testing, two critical bugs were discovered and successfully fixed:

### Bug 1: Static "Frozen" Cells in Multicam
- **Symptom:** Cells injected into the Kiosk 2x2 grids from backend highlight data failed to simulate ("wusel") and remained frozen.
- **Root Cause:** The Game of Life engine utilizes Spatial Partitioning (`chunk_map`) for performance optimization. Manually injecting cells bypassed the chunk activation, causing the engine to skip the "dead" sectors.
- **Fix:** Added `activate_chunk_at(w, row, col)` calls whenever seed data is injected into the grids from the `HighlightData`. 

### Bug 2: Segmentation Fault on Replay and Manual Kiosk Entry
- **Symptom:** The app crashed exactly when a user clicked a Multicam quadrant if they had previously visited the configuration menu.
- **Root Cause:** 
  1. Transitioning to the `STATE_CONFIG` menu (e.g. by pressing `[Q]`) properly freed the global `gui_world` and set the pointers to `NULL` to prevent memory leaks.
  2. Entering Kiosk Mode manually via `[K]` reused the existing `sim_ctx` (which had `current_world == NULL`).
  3. Clicking a quadrant triggered an attempt to clear the single-player grid (`sim_ctx->current_world->grid[k] = DEAD`), directly dereferencing a `NULL` pointer.
  4. Another segfault vector existed when jumping directly from Kiosk into `STATE_RUNNING`, as the telemetry arrays (`history_red_pop`) were bypassed (these are normally allocated in `STATE_IGNITION`).
- **Fix:** 
  - Refactored the click-to-replay transition to jump to `STATE_IGNITION` instead of `STATE_RUNNING`. This restores the dramatic 3-second countdown and guarantees the telemetry arrays are allocated.
  - Added on-demand allocation (`create_world()`) within the quadrant click-handler if `sim_ctx->current_world` is detected as `NULL`.
  - Enforced rigorous calls to `reset_kiosk_timers()` on all paths entering Kiosk Mode to prevent erratic timer behaviors.

## Status
- `DEV_TASKS-0019` is marked as 100% complete.
- The Kiosk Mode feature has passed the QA stress-test inside the Docker development environment.
