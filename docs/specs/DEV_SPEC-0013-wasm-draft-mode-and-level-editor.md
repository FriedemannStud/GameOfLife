# Requirements Analysis & Specification: WASM Draft Mode and Level Editor

This document details the requirements for the interactive pattern editor and submission system, as described in **ADR-0013**.

---

### 1. Detailed Requirements Specification

The **Draft Mode** (Level Editor) is an extension of the existing Biotope UI aimed at enabling user-generated content directly within the browser. 

#### **Core Functionality:**
- **Interactive Grid Editing:** Users must be able to toggle cell states (Alive/Dead) by clicking on an 8x8 area of the grid.
- **Visual Constraints:** An 8x8 "Bounding Box" must be rendered over the arena (which is 16x8) to guide the user.
- **Biomass Validation:** The UI must display a real-time counter of placed cells (e.g., "Cells: 12/24"). Placements beyond 24 cells must be blocked.
- **Identity Management:**
    - **Nickname:** A text input field for the player's display name.
    - **Automatic Player ID:** The system must automatically generate a unique `player_id` (UUID format). This ID should be persisted in the browser's `LocalStorage` so the user maintains their identity across sessions.
- **Submission Lifecycle:**
    - A "Submit" button that becomes active only if the pattern is valid (1-24 cells).
    - Asynchronous transmission of the JSON configuration to the FastAPI backend.
    - Visual feedback (Success/Error messages) following the submission attempt.

#### **Technical Constraints:**
- Implemented in `gui.c` using Raylib.
- Asynchronous networking via Emscripten's fetch API or JS injection.
- JSON construction using `cJSON`.

---

### 2. User Stories & Acceptance Criteria

**Epic: Pattern Design and Submission**

*   **User Story 1: Interactive Editing**
    *   **As a player,** I want to click on the grid to place or remove cells, **so that** I can design my competitive pattern.
    *   **Acceptance Criteria:**
        *   Clicking an empty cell within the 8x8 box makes it alive.
        *   Clicking a living cell removes it.
        *   Cell placement is restricted to the defined 8x8 drafting area.

*   **User Story 2: Real-time Validation**
    *   **As a player,** I want to see how many cells I have placed, **so that** I don't exceed the biomass limit.
    *   **Acceptance Criteria:**
        *   The UI displays a counter (Current/Max).
        *   The system prevents adding more than 24 cells.
        *   The "Submit" button is disabled if 0 cells are placed.

*   **User Story 3: Automatic Identity**
    *   **As a player,** I want the system to remember me without forcing me to manage an ID, **so that** I can focus on designing patterns.
    *   **Acceptance Criteria:**
        *   On first launch, a unique UUID is generated as `player_id`.
        *   The `player_id` is stored in the browser's `LocalStorage`.
        *   Subsequent visits retrieve the existing `player_id`.

*   **User Story 4: Pattern Submission**
    *   **As a player,** I want to submit my pattern to the tournament, **so that** I can compete against others.
    *   **Acceptance Criteria:**
        *   Pressing "Submit" sends a valid JSON (Metadata + Config) to `/api/v1/submit_config`.
        *   The UI displays a "Sending..." state.
        *   A success message is shown upon `201 Created` response.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Toggle between "Simulation" and "Draft" modes.
        *   8x8 Grid interaction (Place/Remove).
        *   Automatic UUID generation and persistence (LocalStorage).
        *   Submit button with API integration.
    *   **Should-Have:**
        *   Visual Bounding Box overlay.
        *   Nickname text input field.
        *   Success/Error toast notifications.
    *   **Could-Have:**
        *   "Clear Grid" button.
        *   "Test Simulation" button (local preview before submit).
    *   **Won't-Have (in this increment):**
        *   Multi-layer patterns.
        *   Advanced brush tools.

*   **Dependencies:**
    1.  **Emscripten/WASM Build:** The project must be buildable via `Makefile.wasm`.
    2.  **FastAPI Backend:** The `/api/v1/submit_config` endpoint must be accessible (CORS configured).
    3.  **cJSON Integration:** Required for generating the submission payload in C.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| 1 | UI/UX | Implement State Machine (Sim/Draft) | Must |
| 2 | Interaction | Mouse-based grid cell toggling | Must |
| 3 | Identity | Automatic UUID generation & LocalStorage persistence | Must |
| 4 | Networking | Emscripten-based JSON POST submission | Must |
| 5 | Validation | Real-time cell counter and 24-cell limit | Must |
| 6 | UI/UX | Nickname input field | Should |
| 7 | UI/UX | Bounding Box visual overlay | Should |
| 8 | Feedback | Status notifications (Success/Fail) | Should |

---

### 5. Definition of Done (DoD)

A Product Backlog Item is considered "Done" when:

*   **Code Quality:** C code adheres to `docs/CODING_STYLE.md` (snake_case, AI attribution).
*   **Validation:** 
    *   The editor correctly prevents >24 cell placements.
    *   The generated JSON matches the backend's expected schema.
*   **Networking:** Submissions are successfully received and stored in MongoDB Atlas via the API.
*   **Persistence:** `player_id` survives a browser refresh.
*   **WASM Compatibility:** The feature works seamlessly in Chrome/Firefox/Safari via the `biotope.html` interface.
*   **Documentation:** Updates to `CHANGELOG.md` and relevant technical docs are completed.
