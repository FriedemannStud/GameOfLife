# Requirements Analysis & Specification: Biotope UI System

This document details the requirements for the "Competitive Biotope Mode" interactive user interface, as described in **ADR-0002**.

---

### 1. Detailed Requirements Specification

The system requires a graphical user interface (GUI) to facilitate the "Biotope" game mode, where two players (Red vs. Blue) compete on a shared Conway's Game of Life grid. The application must transition from a pure CLI-based simulation to an interactive Raylib-based application.

**Functional Requirements:**
1.  **Configuration Screen:**
    *   Allow users to input grid dimensions (Rows, Columns). Default: 100x100.
    *   Allow users to set the update delay (in ms). Default: 500ms.
    *   Allow users to set the maximum initial population per team. Default: 100.
    *   Allow users to set the maximum number of rounds. Default: 1000.
    *   Validate inputs (e.g., max grid size, non-negative delay).
2.  **Grid Editor (Setup Phase):**
    *   Display an empty grid based on configured dimensions.
    *   Divide the screen visually into two hemispheres: Left (Blue Team) and Right (Red Team).
    *   Allow players to toggle cells by clicking.
    *   **Constraint:** Blue cells can only be placed in the left hemisphere; Red cells only in the right.
    *   **Constraint:** Enforce the maximum initial population limit per team.
    *   Provide a "Save Configuration" button to save the current pattern to a file.
    *   Provide a "Load Configuration" button to load a pattern from a file.
    *   Provide a "Start Game" button.
3.  **Simulation View (Running Phase):**
    *   Render the grid in real-time.
    *   Differentiate cells by color: Red (Team 1), Blue (Team 2).
    *   Display current statistics: Round number, Red Count, Blue Count.
    *   Handle game loop timing based on the configured delay.
    *   Automatically stop when `MAX_ROUNDS` is reached or one team is eliminated.
4.  **Game Over Screen:**
    *   Display the winner and final scores.
    *   Automatically export statistics to a Markdown file (`.md`).
    *   Allow restarting the game or exiting.

**Non-Functional Requirements:**
*   **Performance:** The grid rendering must be smooth (60 FPS) for grids up to 200x200.
*   **Usability:** UI controls (sliders, buttons) must be intuitive and clearly labeled.
*   **Portability:** Code must compile on Windows (primary) and Linux.

---

### 2. User Stories & Acceptance Criteria

**Epic: Game Configuration**

*   **User Story 1: Configure Simulation Parameters**
    *   **As a** player,
    *   **I want to** adjust the grid size, speed, and turn limit before the game starts,
    *   **so that** I can customize the match duration and difficulty.
    *   **Acceptance Criteria:**
        *   The app launches into a "Configuration" screen.
        *   Input fields exist for Rows, Cols, Delay, Max Population, and Max Rounds.
        *   Default values (100x100, 500ms, 100 pop, 1000 rounds) are pre-filled.
        *   Clicking "Next" validates inputs (e.g., prevent negative numbers) and proceeds to the Grid Editor.

**Epic: Interactive Setup (Grid Editor)**

*   **User Story 2: Place Initial Cells**
    *   **As a** player (Red or Blue),
    *   **I want to** click on grid cells to set my starting population,
    *   **so that** I can strategically position my colony.
    *   **Acceptance Criteria:**
        *   Clicking a dead cell toggles it to alive (and vice-versa).
        *   Clicking on the left half creates a BLUE cell.
        *   Clicking on the right half creates a RED cell.
        *   Attempting to place a cell in the opponent's hemisphere is blocked (no action).
        *   A counter shows "Remaining Cells" for each team. Placing a cell decreases this count.

*   **User Story 3: Save/Load Setup**
    *   **As a** player,
    *   **I want to** save my meticulously created pattern to a file and load it later,
    *   **so that** I can replay specific scenarios or pause setup.
    *   **Acceptance Criteria:**
        *   "Save" button writes the current grid state (positions and colors) to a file.
        *   "Load" button reads a file and restores the grid state.
        *   Loading overwrites the current grid.

**Epic: Simulation & Visualization**

*   **User Story 4: View Live Match**
    *   **As a** spectator,
    *   **I want to** see the Red and Blue cells evolve and fight in real-time,
    *   **so that** I can follow the progress of the game.
    *   **Acceptance Criteria:**
        *   The grid updates automatically according to the "Delay" setting.
        *   Red cells are rendered as Red squares, Blue as Blue squares.
        *   A visible counter shows the current Round Number.

**Epic: Game Over & Analysis**

*   **User Story 5: Result Export**
    *   **As a** data analyst,
    *   **I want** the game results (winner, final counts) to be saved to a file,
    *   **so that** I can keep a history of match outcomes.
    *   **Acceptance Criteria:**
        *   When the game ends, a `biotope_results_<timestamp>.md` file is created.
        *   The file contains the date, settings used, final scores, and the winner.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Refactoring core logic to be separate from CLI `main`.
        *   Raylib window initialization.
        *   State machine implementation (Config -> Edit -> Run -> End).
        *   Mouse interaction for cell placement (Red/Blue split).
        *   Visual rendering of the grid.
        *   Winning logic and Game Over screen.
    *   **Should-Have:**
        *   `raygui` widgets for inputs (can use keyboard shortcuts or simple text rendering initially if GUI is complex).
        *   File Save/Load functionality.
        *   Detailed statistics export.
    *   **Could-Have:**
        *   Zoom/Pan controls for the grid (Camera2D).
        *   Fancy particle effects for cell death/birth.
    *   **Won't-Have (in this increment):**
        *   Networked multiplayer (playing over LAN/Internet).

*   **Dependencies:**
    1.  **Raylib Integration:** All UI stories depend on successfully linking and running Raylib.
    2.  **Core Refactoring:** The interactive loop depends on `update_generation` being callable from the Raylib loop, not a `while(true)` sleep loop.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| PB-001 | Infrastructure | **Task:** Refactor `main.c` core logic into `game_logic.c/h` (Simulation State separate from UI). | High |
| PB-002 | UI Framework | **Task:** Set up Raylib build environment (Makefile/Task) and verify "Hello World" window. | High |
| PB-003 | Configuration | **User Story 1:** Implement "Config State" with input handling for Rows/Cols/Delay. | Medium |
| PB-004 | Editor | **User Story 2:** Implement "Edit State" with mouse clicking and hemisphere constraints. | High |
| PB-005 | Editor | **Task:** Implement "Max Population" check in Edit State. | Medium |
| PB-006 | Simulation | **User Story 4:** Implement "Run State" connecting the Game Loop to `update_generation`. | High |
| PB-007 | Simulation | **Task:** Visualize Red/Blue cells with distinct colors. | High |
| PB-008 | Data | **User Story 3:** Implement File Save/Load for initial grid patterns. | Medium |
| PB-009 | Analysis | **User Story 5:** Implement Game Over logic and Markdown export. | Low |

---

### 5. Definition of Done (DoD)

A Product Backlog Item is considered "Done" when:

*   **Code Quality:** The code compiles without warnings. Naming conventions match existing C style.
*   **Functionality:** The feature works as described in the Acceptance Criteria within the Raylib window.
*   **Stability:** The application does not crash on invalid inputs (e.g., negative grid size) or boundary clicks.
*   **Verification:** Verified manually by running the application on the local Windows machine.
*   **Documentation:** Functions are commented. Changes are reflected in the project `README.md` if user-facing instructions change.
