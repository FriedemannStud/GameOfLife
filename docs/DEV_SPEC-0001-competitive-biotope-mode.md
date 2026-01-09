# Requirements Analysis & Specification: Competitive Biotope Mode (Red vs Blue)

This document details the requirements for the "Biotope" game mode, as described in **ADR-0001: Competitive Biotope Mode (Red vs Blue)**.

---

### 1. Detailed Requirements Specification

The objective is to transform the existing single-state Conway's Game of Life into a two-player zero-sum game called "Biotope".

#### 1.1. Core Mechanics
- **Grid State:** The grid must support three distinct cell states: `DEAD` (0), `TEAM_RED` (1), and `TEAM_BLUE` (2).
- **Initialization:**
  - The application must support initializing the grid with distinct configurations for Red and Blue teams.
  - While an interactive setup is planned for a future phase (Vibe Coding Prompt), the core engine must essentially support a programmatic setup or predefined pattern loading that distinguishes teams.
- **Evolution Logic (The "Biotope" Rules):**
  - **Survival:** A living cell (Red or Blue) survives if it has exactly 2 or 3 living neighbors (regardless of neighbor color). It retains its original color.
  - **Death:** A living cell dies if it has < 2 or > 3 living neighbors.
  - **Birth (The Competitive Twist):** A dead cell becomes alive if it has exactly 3 living neighbors.
    - **Color Determination:** The new cell's color is determined by the majority color of the 3 parents.
      - 2 Red, 1 Blue -> New cell is Red.
      - 3 Red, 0 Blue -> New cell is Red.
      - 2 Blue, 1 Red -> New cell is Blue.
      - 3 Blue, 0 Red -> New cell is Blue.
- **Game Loop Constraints:**
  - The simulation must run for a finite number of generations (e.g., `MAX_ROUNDS`).
  - The simulation must detect a "Game Over" state based on the round limit.

#### 1.2. Visualization
- The CLI output must visually distinguish between Red cells, Blue cells, and Dead cells.
- Suggested visual mapping:
  - `DEAD`: ` ` (Space) or `.`
  - `TEAM_RED`: `X` (Optionally Red ANSI color)
  - `TEAM_BLUE`: `O` (Optionally Blue ANSI color)
- The current generation count and population statistics (Red vs. Blue count) should be displayed each frame.

#### 1.3. Winning Condition
- At the end of `MAX_ROUNDS`, the game must calculate and display the total population for each team.
- The winner is the team with the highest number of living cells.

---

### 2. User Stories & Acceptance Criteria

**Epic: Core Engine Adaptation**

*   **User Story 1: Multi-State Grid Support**
    *   **As a** developer,
    *   **I want** the grid data structure to support identifying Red and Blue cells distinct from Dead cells,
    *   **So that** the simulation can track team populations.
    *   **Acceptance Criteria:**
        *   `World` struct grid can store values 0, 1, and 2.
        *   Memory usage remains efficient (using `int`).
        *   Constants `DEAD`, `TEAM_RED`, `TEAM_BLUE` are defined.

*   **User Story 2: Competitive Evolution Logic**
    *   **As a** player,
    *   **I want** new cells to inherit the color of the majority of their parents,
    *   **So that** my team can expand its territory based on the "Biotope" rules.
    *   **Acceptance Criteria:**
        *   A dead cell with 3 neighbors becomes alive.
        *   If parents are 2 Red/1 Blue, new cell is Red.
        *   If parents are 2 Blue/1 Red, new cell is Blue.
        *   Existing cells survive/die based on total neighbor count (standard rules), retaining their color.
        *   Unit tests verify specific "inheritance" scenarios.

**Epic: Game Management & UI**

*   **User Story 3: Initial Team Setup**
    *   **As a** player,
    *   **I want** the game to start with a mixed population of Red and Blue cells,
    *   **So that** the competition can begin immediately.
    *   **Acceptance Criteria:**
        *   `init_world` is updated to populate the grid with `TEAM_RED` and `TEAM_BLUE` cells.
        *   Initialization strategy ensures a roughly fair or interesting starting distribution (e.g., split screen or random mix).

*   **User Story 4: Visual Distinction and HUD**
    *   **As a** spectator,
    *   **I want** to see Red cells as 'X' and Blue cells as 'O' (or colored), and a score counter,
    *   **So that** I can track who is winning.
    *   **Acceptance Criteria:**
        *   Console output renders different characters/colors for Red vs Blue.
        *   Current turn number is displayed.
        *   Current population count for Red and Blue is displayed each turn.

*   **User Story 5: Win Condition & Game Over**
    *   **As a** competitive player,
    *   **I want** the game to end after a fixed number of turns and declare a winner,
    *   **So that** the match has a definitive conclusion.
    *   **Acceptance Criteria:**
        *   Simulation stops automatically after `MAX_ROUNDS`.
        *   Final message announces "Red Wins", "Blue Wins", or "Draw".
        *   Final scores are displayed.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Constants for cell states.
        *   Updated `update_generation` logic (counting neighbors by color, inheritance rules).
        *   Updated `init_world` for 2 teams.
        *   Visual distinction in `print_world`.
    *   **Should-Have:**
        *   ANSI Color codes for terminal output (Red/Blue text).
        *   Real-time score display (HUD).
        *   Fixed round limit logic.
    *   **Could-Have:**
        *   User input to place specific patterns for Red/Blue (Interactive Setup).
        *   "Pause" functionality.
    *   **Won't-Have (in this increment):**
        *   Graphical User Interface (GUI).
        *   Networked multiplayer.

*   **Dependencies:**
    1.  **Grid State Refactoring:** Must happen before Evolution Logic can be implemented.
    2.  **Evolution Logic:** Required before meaningful Visualization can be tested.
    3.  **Visualization:** Needed to verify the Initial Setup visually.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| **PB-01** | Core | Define constants (`DEAD`, `TEAM_RED`, `TEAM_BLUE`) and refactor `init_world` to seed both teams. | High |
| **PB-02** | Core | Implement `count_neighbors_by_team` helper logic to count Red vs Blue neighbors specifically. | High |
| **PB-03** | Core | Update `update_generation` to implement the "Majority Parent" birth rule and standard survival. | High |
| **PB-04** | UI | Update `print_world` to render 'X' and 'O' (and optional colors) based on cell value. | High |
| **PB-05** | UI | Add HUD: Display current Turn count and Population (Red vs Blue) in the loop. | Medium |
| **PB-06** | Core | Implement `MAX_ROUNDS` check and "Game Over" win calculation logic in `main`. | Medium |
| **PB-07** | Core | Create basic unit/verification test cases (e.g., small known patterns) to verify inheritance rules. | High |

---

### 5. Definition of Done (DoD)

A Product Backlog Item is considered "Done" when:

*   **Code Quality:**
    *   Code compiles without warnings (`gcc -Wall`).
    *   Follows the existing C style/formatting (minimalist, readable).
    *   No memory leaks (verified by code review or tool if available).
*   **Functionality:**
    *   The feature works as specified in the User Story.
    *   The simulation runs stable for at least 1000 generations or the defined limit.
*   **Verification:**
    *   Manual verification via CLI output shows correct behavior (e.g., a "Red" block doesn't randomly turn "Blue" without cause).
    *   The "Biotope" logic (2 Red + 1 Blue parent = Red child) is confirmed working.
