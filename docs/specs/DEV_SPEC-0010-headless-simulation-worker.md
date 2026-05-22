# Requirements Analysis & Specification: Headless Simulation Worker

This document details the requirements for the Headless Simulation Worker, as described in **ADR-0010**.

---

### 1. Detailed Requirements Specification

The Headless Simulation Worker is a core component of the massive-parallel multiplayer ecosystem. It decouples the `game_logic.c` from the graphical user interface (`gui.c` and Raylib) to allow ultra-fast, command-line driven execution of Game of Life matches on backend servers.

#### 1.1 Input and Execution
- **CLI Interface:** The worker must be executable via the command line.
- **Input Parameters:** It must accept exactly two arguments specifying the paths to two JSON configuration files. The first file represents Team Red (left side), and the second represents Team Blue (right side). Both inputs are uniform 8x8 grids.
- **Environment:** The worker must run without requiring a virtual display (Xvfb) or GPU resources.

#### 1.2 Simulation Constraints
- **Grid Initialization:** The worker must load both 8x8 JSON configurations and place them into the appropriate positions within the overarching simulation arena.
- **Metadata Support:** The worker must extract player metadata (`player_id`, `nickname`) from the input files and include it in the final result.
- **Fixed Duration:** The simulation must execute for exactly 100 generations.
- **Logic Consistency:** The simulation must use the exact same `game_logic.c` functions as the GUI version to guarantee consistent results.

#### 1.3 Output and Result Persistence
- **Result Determination:** After 100 generations, the worker must count the surviving cells for both Team Red and Team Blue.
- **Result Output:** The worker must determine the winner (the team with the most living cells, or a draw).
- **Result Persistence:** The results of the match MUST be saved. The worker must generate a result JSON file (e.g., `result.json` or appending to a specified path) that contains the final cell counts for both teams and the determined winner. This is critical for the backend to update Elo ratings, statistics, and leaderboards.

#### 1.4 Build System
- **Makefile Update:** The build system must be updated to include a specific target (e.g., `make headless`) that compiles the codebase without linking Raylib or other GUI-specific libraries.

---

### 2. User Stories & Acceptance Criteria

**Epic: Automated Match Simulation for Multiplayer Backend**

*   **User Story 1: Headless CLI Execution**
    *   **As a backend matchmaking service,** I want to invoke the simulation worker via the command line with two JSON files as arguments, **so that** I can trigger automated matches without graphical overhead.
    *   **Acceptance Criteria:**
        *   The executable accepts two file paths as arguments (e.g., `./biotope_headless red.json blue.json`).
        *   The executable fails gracefully with an appropriate error message and exit code if the arguments are missing or invalid.
        *   The executable runs successfully in a purely terminal-based environment without Raylib or OpenGL dependencies.

*   **User Story 2: Fixed-Duration Simulation**
    *   **As a competitive player,** I want matches to be simulated fairly and predictably for exactly 100 generations, **so that** my 8x8 configuration is evaluated correctly against my opponent's.
    *   **Acceptance Criteria:**
        *   The worker initializes the global grid and places the Red and Blue 8x8 configurations correctly.
        *   The simulation logic runs exactly 100 times.
        *   The logic uses the exact same `update_generation` implementation as the visual game.

*   **User Story 3: Result Calculation and Persistence**
    *   **As a system administrator,** I want the worker to save the match results into a structured format (JSON), **so that** the backend can parse the outcome and update player rankings and statistics.
    *   **Acceptance Criteria:**
        *   After generation 100, the worker calculates the final population for both teams.
        *   The worker outputs a JSON string or file containing `winner` (red, blue, or draw), `red_population`, and `blue_population`.
        *   The backend can reliably read this output without parsing complex debug logs.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Adapt `main.c` or create a new entry point (`main_headless.c`) for CLI execution without Raylib.
        *   Accept two JSON files as input arguments.
        *   Run exactly 100 generations using `game_logic.c`.
        *   Output the match results (winner and final populations) in JSON format.
        *   Update Makefile with a headless build target.
    *   **Should-Have:**
        *   Detailed error logging to `stderr` if JSON parsing fails.
    *   **Could-Have:**
        *   Configurable max generations via a third CLI argument (defaulting to 100).
    *   **Won't-Have (in this increment):**
        *   Saving the entire final grid state (only the population counts and winner are needed for rankings).

*   **Dependencies:**
    1.  **Topic:** JSON Payload Specification (ADR-0009). The headless worker depends on the JSON structures defined and parsed via `cJSON`.
    2.  **Topic:** Matchmaking Backend. The backend service depends on the headless worker's executable and its standardized result output.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| 001 | Automated Match Simulation | Refactor build system (Makefile) to support a `headless` target. | Must |
| 002 | Automated Match Simulation | Implement CLI argument parsing in the headless entry point. | Must |
| 003 | Automated Match Simulation | Integrate `cJSON` to load and place two 8x8 configurations onto the simulation grid. | Must |
| 004 | Automated Match Simulation | Implement the strict 100-generation simulation loop without any rendering calls. | Must |
| 005 | Automated Match Simulation | Implement logic to count final populations and generate a JSON result output. | Must |
| 006 | Automated Match Simulation | Add graceful error handling for missing files or malformed JSON payloads. | Should |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The code follows the guidelines in `docs/CODING_STYLE.md` (snake_case, PascalCase, AI attribution).
*   **Compilation:** The code compiles cleanly with `make headless` without warnings (`-Wall -Wextra`) and without linking Raylib.
*   **Tests:**
    *   The headless binary can be executed from the terminal with test JSON files and completes successfully.
    *   The result output matches the expected JSON structure and accurately reflects the simulation outcome.
*   **Acceptance Criteria:** All acceptance criteria defined for the story have been met.
*   **Documentation:** All new functions are commented with an educational focus for 1st-semester computer science students, as specified in the project briefings.