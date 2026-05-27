### **ADR-0019: Kiosk Mode State Machine and UI**

**Status:** Proposed

**Date:** 2026-05-27

#### **1. Context and Problem Statement**

To serve the Uni-Messe requirement, the Game of Life application must operate autonomously as an eye-catching exhibition piece while still allowing for deep interactive exploration. The system currently possesses the ability to fetch remote data (ADR-0017) and the architecture to render multiple simulations concurrently (ADR-0018). However, the overarching control flow—the cycling between showcasing the Leaderboard and the Wusel-Multicam, and transitioning to a fullscreen replay upon user interaction—is missing. 

We need to decide how to integrate an autonomous idle loop (Kiosk Mode) into the existing Raylib state machine (`app_state_manager.c`) without permanently locking the user out of the standard interactive features. The application must smoothly handle transitions from "idle autonomous display" to "active user exploration" and back to "idle".

#### **2. Decision**

We will extend the existing `AppState` enum and `app_state_manager.c` to implement a robust, timer-driven Kiosk loop with a global interrupt mechanism.

*   **New Main State (`STATE_KIOSK_MODE`):** We will introduce a new overarching application state. When in this state, the application ignores standard game configuration logic and operates autonomously.
*   **Sub-State Cycling (WP 4):** Within `STATE_KIOSK_MODE`, we will implement a sub-state machine driven by delta-time timers. It will automatically cycle between rendering the `UI_LEADERBOARD` view (e.g., for 15 seconds) and the `UI_MULTICAM` view (e.g., for 30 seconds).
*   **User Interrupt Mechanism (WP 5):**
    1.  **Click-to-Replay:** While in the `UI_MULTICAM` sub-state, if a user clicks on one of the 4 quadrants, the application will immediately transition out of `STATE_KIOSK_MODE` into `STATE_RUNNING`. It will load the specific 8x8 seed of that quadrant into the primary fullscreen `SimulationContext`, acting as a High-Fidelity replay.
    2.  **Global Inactivity Timer:** The main loop will track a global `time_since_last_input` variable (reset by any mouse movement or keystroke). If this timer exceeds a defined threshold (e.g., 60 seconds) while in *any* interactive state (like `STATE_CONFIG` or `STATE_RUNNING`), the application will automatically force a transition back to `STATE_KIOSK_MODE`.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

*   **Seamless Transitions:** Leveraging the existing `app_state_manager.c` ensures that transitions between Kiosk mode and interactive mode use the same clean memory and context setup/teardown paths as the rest of the application.
*   **Code Reusability:** The High-Fidelity replay simply re-uses the existing `STATE_RUNNING` view, meaning we do not need to build a second "fullscreen replay" renderer.
*   **Self-Healing:** The global inactivity timer acts as a failsafe. If a Uni-Messe attendee starts configuring a game but walks away halfway through, the system will automatically "heal" itself by returning to the eye-catching Kiosk loop.

**Negative Consequences (Disadvantages):**

*   **State Machine Complexity:** The state machine becomes non-linear. The application can jump from `STATE_CONFIG` directly back to `STATE_KIOSK_MODE` at any unpredictable moment due to the inactivity timeout.
*   **Input Hooking:** The global inactivity timer requires hooking mouse and keyboard inputs at the very top of the application loop, preceding the state-specific update functions.

#### **4. Alternatives Considered**

*   **Separate Executables (Game App vs. Kiosk App):**
    *   *Concept:* Build two separate binaries. A launcher script kills the Game App and starts the Kiosk App after 60 seconds of inactivity.
    *   *Rejected:* Results in a jarring OS-level window transition (flashing desktop/terminal). It also duplicates networking and rendering code across two codebases, violating DRY principles.
*   **External Script Automation (Simulated Keystrokes):**
    *   *Concept:* A Python or bash script monitors the system and sends simulated keyboard events (e.g., pressing `Right Arrow`) to cycle through views.
    *   *Rejected:* Extremely fragile. If the Raylib window loses OS focus, the keystrokes fail. It also struggles to handle graceful transitions from a user's active session back to the idle loop.
*   **Hardcoded Replay UI:**
    *   *Concept:* Build a completely new rendering state specifically for Kiosk replays, independent of the normal `STATE_RUNNING`.
    *   *Rejected:* Unnecessary duplication of effort. The existing `STATE_RUNNING` logic already handles fullscreen grids, HUDs, and colors perfectly.
