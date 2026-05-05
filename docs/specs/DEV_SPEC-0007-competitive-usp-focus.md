# Requirements Analysis & Specification: Laser Focus on the "Biotope" Competitive USP

This document details the requirements for structuring the gameplay mechanics and UI around the core 1v1 competitive loop, as described in **ADR-0007**.

---

### 1. Detailed Requirements Specification

The primary objective is to solidify the "Red vs. Blue" algorithmic battle as the central pillar of the game. This requires transforming the current basic "Sandbox with two colors" into a structured, tense, and analyzable competitive match format.

**Key Functional Requirements:**
1.  **The "Blind Draft" System:** The setup phase (`STATE_CONFIG` / `STATE_EDIT`) must be modified to prevent players from seeing each other's initial cell placements. A "Fog of War" must obscure the opposing hemisphere during drafting.
2.  **The "Ignition Sequence":** The transition from `STATE_EDIT` to `STATE_RUNNING` must be a distinct, highly polished event. It must reveal the opponent's draft, pause briefly for strategic evaluation, and then commence the simulation.
3.  **The "Catalyst" Intervention:** A singular, high-impact tactical intervention allowed once per match per player. This breaks pure determinism slightly to add tension and agency during the simulation phase.
4.  **Granular Telemetry & Analytics:** The `game_logic.c` must track not just total population, but frontline shifts, territory control, and momentum. This data must be saved via `file_io.c` and displayed in a new `STATE_FINISHED` post-match screen.

---

### 2. User Stories & Acceptance Criteria

**Epic: The Draft & Ignition (Pre-Match Tension)**

*   **User Story 1: Blind Drafting (Fog of War)**
    *   **As a player drafting my cells,** I want the opposing half of the grid to be obscured, **so that** my opponent cannot perfectly counter my starting formation before the match begins.
    *   **Acceptance Criteria:**
        *   During `STATE_EDIT`, a visual "Fog of War" (e.g., solid color or blurred overlay) covers the hemisphere not currently being edited.
        *   If playing locally (hotseat), the UI prompts "Player 1 (Red) Turn", hides Blue, then switches to "Player 2 (Blue) Turn" hiding Red.

*   **User Story 2: The Ignition Reveal**
    *   **As a player,** I want a dramatic reveal of the opponent's draft right before the simulation starts, **so that** I can experience a moment of realization ("Did my strategy work?") before the chaos begins.
    *   **Acceptance Criteria:**
        *   Pressing ENTER to start the simulation triggers the "Ignition Sequence".
        *   The Fog of War is removed.
        *   The game pauses for exactly 3 seconds, allowing players to view the complete initial state (`generation 0`).
        *   After 3 seconds, `update_generation` begins execution.

**Epic: Mid-Match Agency**

*   **User Story 3: The Catalyst Strike**
    *   **As a player watching my cells lose,** I want exactly one opportunity to destroy a small section of the grid, **so that** I can attempt a desperate tactical save.
    *   **Acceptance Criteria:**
        *   During `STATE_RUNNING`, each team has a UI indicator showing their "Catalyst" is ready (1 charge).
        *   Pressing a specific key + clicking the mouse consumes the charge and instantly sets a 10x10 area around the cursor to `DEAD`.
        *   Once used, the UI indicator shows the Catalyst is depleted for that team for the remainder of the match.

**Epic: Post-Match Analytics**

*   **User Story 4: The Post-Match Autopsy**
    *   **As a competitive player,** I want detailed statistics after the match ends, **so that** I can analyze why I won or lost and improve my starting formations.
    *   **Acceptance Criteria:**
        *   `STATE_FINISHED` (or `STATE_GAME_OVER`) displays a line graph showing the population of Red vs. Blue over time (generations).
        *   The system highlights the "Turn of Maximum Volatility" (the generation with the highest number of cell deaths/births).
        *   This data is serialized and saved into the `.bio` file by `file_io.c`.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   The "Blind Draft" UI logic (Fog of War for hotseat play).
        *   The "Ignition Sequence" (3-second reveal pause before running).
        *   Hardcoding the 3-state logic (Dead, Red, Blue) to prevent feature creep.
    *   **Should-Have:**
        *   The single-use "Catalyst" intervention mechanic.
        *   Basic line graph of population over time in `STATE_FINISHED`.
    *   **Could-Have:**
        *   Advanced telemetry (territory control mapping).
        *   Replay mode (stepping backward through the saved generations).
    *   **Won't-Have (in this increment):**
        *   Online multiplayer matchmaking (Blind Draft will be implemented for local Hotseat/AI first).

*   **Dependencies:**
    1.  **Topic: UI Architecture:** The Fog of War and Ignition sequence depend on modifications to the rendering logic in `gui.c`, specifically `DrawGridAndCells`.
    2.  **Topic: Memory Management:** Tracking telemetry data over 1000+ rounds requires allocating arrays to store historical data in the `GameConfig` or `World` struct, which must be carefully managed to avoid memory leaks.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| TSK-01 | Draft | Implement Hotseat Turn Logic in `STATE_EDIT` (Player 1 vs Player 2 modes) | High |
| TSK-02 | Draft | Render "Fog of War" overlay over the inactive hemisphere during drafting | High |
| TSK-03 | Ignition | Implement 3-second delay timer state before switching to `STATE_RUNNING` | High |
| TSK-04 | Core | Audit `game_logic.c` to ensure logic is strictly hardcoded to Red/Blue and strip any generic/sandbox remnants | High |
| TSK-05 | Agency | Add Catalyst state variables (bool `red_catalyst_used`, `blue_catalyst_used`) to `GameConfig` | Medium |
| TSK-06 | Agency | Implement mouse-click logic in `STATE_RUNNING` to apply Catalyst (kill 10x10 area) | Medium |
| TSK-07 | Analytics| Create `int* population_history_red` and `blue` arrays to store round-by-round counts | Medium |
| TSK-08 | Analytics| Build `STATE_FINISHED` UI to draw a line graph using the historical population arrays | Medium |
| TSK-09 | Analytics| Update `file_io.c` to append telemetry arrays to the `.bio` save format | Low |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The C code is formatted properly and adheres to `docs/CODING_STYLE.md`.
*   **Compilation:** The project compiles natively without warnings.
*   **Execution:** 
    *   The Blind Draft effectively hides the opponent's side during hotseat play.
    *   The Ignition pause works correctly without freezing the OS thread.
    *   The Catalyst can only be used strictly once per team.
*   **Tests:**
    *   Run a full match to ensure telemetry arrays do not overflow (`MAX_ROUNDS` limits are respected).
*   **Acceptance Criteria:** All acceptance criteria defined for the story have been met.
*   **Documentation:** `CHANGELOG.md` is updated to reflect the new competitive mechanics.