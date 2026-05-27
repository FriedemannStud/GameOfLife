# Requirements Analysis & Specification: Kiosk Mode State Machine and UI

This document details the requirements for implementing the autonomous Kiosk Mode and its interactive interrupts, as described in **ADR-0019**.

---

### 1. Detailed Requirements Specification

The objective is to finalize the Uni-Messe Kiosk experience by integrating the networking layer (ADR-0017) and the Multicam renderer (ADR-0018) into a cohesive, timer-driven state machine that seamlessly transitions between passive observation and active user engagement.

**1.1 State Machine Integration (`app_state_manager.c`):**
*   **`STATE_KIOSK_MODE`:** A new top-level application state must be added.
*   **Sub-State Cycling:** Inside `STATE_KIOSK_MODE`, the application must maintain a delta-time counter to switch between two sub-views:
    *   `UI_LEADERBOARD`: Displays the fetched `LeaderboardData` using Raylib text/shapes.
    *   `UI_MULTICAM`: Displays the fetched `HighlightData` utilizing the 2x2 Multicam Render Contexts.
*   **Data Polling:** The Kiosk state should trigger `network_fetch_..._async()` when entering the respective sub-states to ensure data is fresh.

**1.2 Leaderboard UI:**
*   A clean, highly legible tabular display using Raylib's `DrawText` or `DrawTextEx` functions.
*   Must display the Top N players, showing Rank, Name, Elo, and Win Rate.

**1.3 Global Inactivity Failsafe:**
*   The application must track global idle time (`time_since_last_input`).
*   Any registered mouse movement, mouse click, or keyboard press must reset this timer to 0.0.
*   If `time_since_last_input > 60.0` seconds AND the application is *not* currently in `STATE_KIOSK_MODE`, it must force a state transition back to `STATE_KIOSK_MODE`.

**1.4 Click-to-Replay Interrupt:**
*   While in the `UI_MULTICAM` sub-state, the application must listen for `IsMouseButtonPressed(MOUSE_LEFT_BUTTON)`.
*   If a click occurs, the application must detect which of the 4 Multicam quadrants was clicked (based on `viewport_bounds`).
*   The application must transition to `STATE_RUNNING`.
*   The application must inject the `seed_red` and `seed_blue` from that specific quadrant's `HighlightData` into the primary single-player `SimulationContext`.

---

### 2. User Stories & Acceptance Criteria

**Epic: Uni-Messe Kiosk Experience**

*   **User Story 1: Autonomous Cycling**
    *   **As an exhibition observer,** I want the screen to automatically switch between the Leaderboard and the Live Matches, **so that** I can see both the current standings and the action without touching the mouse.
    *   **Acceptance Criteria:**
        *   The application starts in `STATE_KIOSK_MODE`.
        *   It shows the Leaderboard for exactly 15 seconds.
        *   It transitions to the 2x2 Multicam for exactly 30 seconds.
        *   It repeats this cycle indefinitely.

*   **User Story 2: Live Leaderboard Display**
    *   **As a tournament participant,** I want to see my name and Elo score clearly on the screen, **so that** I know my current ranking.
    *   **Acceptance Criteria:**
        *   The `UI_LEADERBOARD` view accurately renders the data fetched via `network_io.c`.
        *   The text is large, legible, and formatted nicely in columns (Rank, Name, Elo).
        *   The view handles the "loading" state gracefully (e.g., showing a spinner if `is_ready` is false).

*   **User Story 3: High-Fidelity Replay Interrupt**
    *   **As an interested user,** I want to click on one of the four small running simulations, **so that** it expands to fullscreen and lets me inspect it closely.
    *   **Acceptance Criteria:**
        *   Clicking a quadrant during the Multicam view transitions the app to `STATE_RUNNING`.
        *   The fullscreen simulation correctly displays the exact 8x8 seeds from the clicked quadrant.
        *   Standard `STATE_RUNNING` UI (Play, Pause, Speed controls) is available.

*   **User Story 4: Global Inactivity Failsafe**
    *   **As the exhibition host,** I want the application to reset itself if a user walks away, **so that** the screen always returns to the eye-catching Kiosk loop for the next group of people.
    *   **Acceptance Criteria:**
        *   If the user is in `STATE_RUNNING` or `STATE_CONFIG` and touches nothing for 60 seconds, the application automatically jumps back to `STATE_KIOSK_MODE`.
        *   Mouse movements or keypresses successfully reset the 60-second timer.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Implementation of `STATE_KIOSK_MODE` and the 15s/30s cyclic timer.
        *   Implementation of the Leaderboard UI rendering.
        *   Integration of the 2x2 Multicam (from ADR-0018) into the Kiosk loop.
        *   Global inactivity timer (60s failsafe).
    *   **Should-Have:**
        *   The Click-to-Replay interrupt logic (calculating mouse intersections with bounds).
    *   **Could-Have:**
        *   Smooth visual fade transitions between the Leaderboard and Multicam views.
    *   **Won't-Have (in this increment):**
        *   Interactive sorting or scrolling of the Leaderboard (it's a passive display).

*   **Dependencies:**
    1.  **C-Networking (ADR-0017):** The Leaderboard UI depends completely on `network_io.c` providing valid `LeaderboardData`.
    2.  **Multicam Architecture (ADR-0018):** The `UI_MULTICAM` view depends completely on the `SimulationContext` and `RenderContext` refactoring being finished and stable.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| 1 | Kiosk UI | Add `STATE_KIOSK_MODE` to `AppState` enum and basic `case` in `app_state_manager.c`. | Must-Have |
| 2 | Kiosk UI | Implement `draw_leaderboard_ui()` using Raylib text drawing and the `g_leaderboard` struct. | Must-Have |
| 3 | Kiosk UI | Implement the delta-time based cyclic state machine (15s Leaderboard -> 30s Multicam -> Repeat). | Must-Have |
| 4 | Kiosk UI | Hook `network_fetch_..._async()` triggers to the state transitions so data updates every cycle. | Must-Have |
| 5 | Kiosk UI | Implement the global `time_since_last_input` variable at the top of the main application loop. | Must-Have |
| 6 | Kiosk UI | Add logic to detect inactivity > 60s and force transition to `STATE_KIOSK_MODE`. | Must-Have |
| 7 | Kiosk UI | Implement Click-to-Replay: Detect mouse clicks in Multicam, identify quadrant, and load seeds into `STATE_RUNNING`. | Should-Have |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The code is written and formatted according to the guidelines in `docs/CODING_STYLE.md`. State machine logic is contained cleanly within `app_state_manager.c`.
*   **Interactive Verification:** 
    *   The application launches into the Kiosk loop automatically.
    *   The Leaderboard renders correctly without crashing.
    *   Leaving the PC idle for 60 seconds during manual configuration successfully forces a return to the Kiosk loop.
    *   Clicking a Multicam quadrant successfully launches the fullscreen replay of that specific match.
*   **Acceptance Criteria:** All acceptance criteria defined for the stories have been met.
*   **Documentation:** Technical documentation (e.g., `DEV_TASKS-0019.md`) is updated to reflect progress.
