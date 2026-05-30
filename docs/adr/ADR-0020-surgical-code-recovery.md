### **ADR-0020: Surgical Code Recovery and Logical State Separation**

  

**Status:** Accepted and Implemented

  

**Date:** 2026-05-29

  

#### **1. Context and Problem Statement**

  

During the implementation of the Uni-Messe Kiosk Mode (`ADR-0018` and `ADR-0019`), the overarching Raylib state machine (`app_state_manager.c`) and rendering pipeline were severely disrupted. Specifically, the legacy codebase relied heavily on global variables for the simulation grid. When the architecture shifted to use instantiated `SimulationContext` and `RenderContext` structs (to support the 2x2 Multicam), the AI/Developer encountered compiler errors in the interactive single-player states (`STATE_CONFIG`, `STATE_EDIT_RED`, `STATE_EDIT_BLUE`).

  

Instead of refactoring the legacy UI logic to use the new context pointers, these states were aggressively deleted or bypassed. Furthermore, the entry point of the application was hardcoded to `STATE_KIOSK_MODE`. As a result, the application degraded into a passive presentation loop. The original interactive features (configuring grid sizes, drawing patterns for Team Red/Blue) are completely unreachable, effectively destroying the core interactive premise of the app.

  

We need to restore the single-player interactivity without breaking the functional Kiosk mode, while also addressing the architectural friction of combining asynchronous polling loops (Kiosk) with event-driven loops (Interactive UI).

  

#### **2. Decision**

  

We will proceed with **Option 1 (Code-Recovery & Refactoring)**, combined with a strict mandate to **logically separate the code** internally ("trennen den Code") to prevent state machine bloat.

  

1.  **Code Recovery:** We will extract the missing `switch` cases and UI logic (for `STATE_CONFIG` and editing) from the Git history just prior to the execution of `DEV_TASKS-0018`.

2.  **Context-Aware Adaptation:** We will refactor the recovered legacy UI code to operate strictly on the new `global_sim` (`SimulationContext`) and `global_render` (`RenderContext`) pointers, permanently removing its reliance on the deleted global arrays.

3.  **Logical Separation of Concerns:** To manage the complexity, we will separate the execution paths inside `app_state_manager.c`. The Kiosk logic and the Interactive/Config logic will be decoupled into distinct handler functions (or separate translation units if necessary).

4.  **Safe Transitions & Failsafes:** We will implement explicit exit routes (e.g., a visible `[P]lay` keyboard shortcut hint in Kiosk mode to enter Config mode, and a `[K]` shortcut hint in Replay mode to return). Crucially, the 60-second inactivity timer (`update_global_input`) will be fortified to safely deallocate (`free`) any dynamically allocated grid buffers belonging to a partially configured single-player session before forcing a transition back to Kiosk mode.

  

#### **3. Consequences of the Decision**

  

**Positive Consequences (Advantages):**

  

*   **Fast Time-to-Recovery:** By patching the missing states back in, we save significant development time compared to rewriting the Kiosk mode from scratch.

*   **Seamless UX:** By maintaining a single Raylib executable, users at the Uni-Messe will experience fluid, instant transitions between passive observation and active configuration without jarring desktop window flashes.

*   **Preservation of Work:** The correctly implemented features from tasks 0018 and 0019 (Multicam, Leaderboard fetcher) remain fully intact and operational.

  

**Negative Consequences (Disadvantages):**

  

*   **Memory Management Complexity:** Juggling 4 Kiosk `SimulationContexts` alongside 1 Interactive `SimulationContext` requires meticulous tracking of `malloc`/`free` calls. A single missed pointer during a state interrupt (like the 60s timeout) will cause VRAM or memory leaks.

*   **State Machine Friction:** The overarching loop still has to accommodate two entirely different lifecycles (Timer/Network-driven vs. Input-driven), which increases the cognitive load for future maintenance.

  

#### **4. Alternatives Considered**

  

*   **The Two-App Approach (Split Binaries):**

    *   *Concept:* Compile two entirely separate executables (`biotope_kiosk` and `biotope_singleplayer`) that share the core `game_logic.c`.

    *   *Why it was rejected:* While architecturally superior and much safer regarding memory, it was previously rejected in ADR-0019 because transitioning between two OS-level windows creates a poor, unpolished user experience in a physical Kiosk exhibition setting.

*   **Rollback & Rewrite (The Clean Slate):**

    *   *Concept:* Revert the entire repository to the commit before ADR-0018 and re-implement the Multicam and Kiosk features holistically, ensuring no legacy states are dropped.

    *   *Why it was rejected:* Highly inefficient. It discards complex, working code (the Multicam layout, the API fetchers) just to fix a routing issue in the state machine. The surgical recovery (Option 1) provides a faster, pragmatic path to a working product.