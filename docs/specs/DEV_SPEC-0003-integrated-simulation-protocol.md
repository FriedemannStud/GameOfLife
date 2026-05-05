# Requirements Analysis & Specification: Integrated Simulation Protocol and Replay System

This document details the requirements for the "Integrated Simulation Protocol and Replay System", as described in **ADR-0003**.

---

### 1. Detailed Requirements Specification

The objective is to transform the existing save/load functionality into a robust auditing and replay system. This involves changes to the data persistence layer, the simulation lifecycle, and the user interface.

#### 1.1 Extended Protocol Format
The existing `.bio` file format must be extended to capture the full context of a simulation.
*   **Legacy Format:** `Rows Cols MaxPop` (Header) + List of Cells.
*   **New Protocol Format:**
    *   **Header:** Must include `Rows`, `Cols`, `MaxPop`, `MaxRounds`, `DelayMS`, and a `Timestamp`.
    *   **Body:** List of active cells (Team Red/Blue).
    *   **Footer (Optional):** If a simulation completes, results (Winner, Final Population Counts) should be appended to the file.
    *   **Compatibility:** The loader must distinguish between legacy files (3 integer header) and protocol files (extended header) to prevent crashes.

#### 1.2 "Save-on-Start" Automation
*   **Trigger:** Transition from `STATE_EDIT` to `STATE_RUNNING` (when user presses `ENTER`).
*   **Action:** Automatically generate a filename using the pattern `biotope_results/run_YYYYMMDD_HHMMSS.bio`.
*   **Content:** Serialize the *current* state of the grid and the *current* configuration settings immediately before the first simulation tick.
*   **Error Handling:** If file creation fails (e.g., directory missing), the simulation should still proceed, but an error message should be displayed.

#### 1.3 In-App File Browser (UI)
A new UI state `STATE_LOAD` will replace the simple "Load" toggle.
*   **Directory Scanning:** The system must list all valid `.bio` files in the `biotope_results/` and root directories.
*   **Navigation:**
    *   **Up/Down Arrows:** Select file.
    *   **Enter:** Load selected file and transition to `STATE_EDIT`.
    *   **Esc/Q:** Return to `STATE_CONFIG` or `STATE_EDIT` without loading.
*   **Preview:** When a file is selected (highlighted), display its metadata (Date, Grid Size, Max Rounds) on the side or bottom panel before loading.
*   **Sorting:** Files should ideally be sorted by date (newest first).

---

### 2. User Stories & Acceptance Criteria

**Epic: Simulation Audit and Replayability**

*   **User Story 1: Automatic Protocol Generation**
    *   **As a** Researcher/Player,
    *   **I want** my simulation setup to be saved automatically when I start the game,
    *   **So that** I don't lose my configuration if I forget to save manually or if the program crashes.
    *   **Acceptance Criteria:**
        *   Pressing `ENTER` in Editor Mode creates a new file in `biotope_results/`.
        *   The filename contains the current timestamp.
        *   The file contains `MaxRounds`, `Delay`, and `MaxPopulation` in addition to grid dimensions.
        *   The simulation starts immediately after saving.

*   **User Story 2: Browse Protocol History**
    *   **As a** Researcher/Player,
    *   **I want** to browse a list of past simulation runs within the application,
    *   **So that** I can find a specific experiment without leaving the game window.
    *   **Acceptance Criteria:**
        *   Pressing `[L]` opens a file browser view (`STATE_LOAD`).
        *   The view lists files from the `biotope_results` folder.
        *   The user can navigate the list using keyboard arrows.
        *   The currently selected filename is visually highlighted.

*   **User Story 3: Load and Replay**
    *   **As a** Researcher/Player,
    *   **I want** to load a selected protocol file,
    *   **So that** the application restores the exact Grid, Max Rounds, and Delay settings from that run.
    *   **Acceptance Criteria:**
        *   Selecting a file and pressing `ENTER` loads the state.
        *   The application transitions to `STATE_EDIT`.
        *   Grid dimensions resize automatically if the file differs from current settings.
        *   Configuration counters (Rounds, Delay) are updated to match the file.
        *   Legacy files (old format) can still be loaded without errors (using default defaults for missing values).

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Extension of `save_grid` / `load_grid` functions to handle new metadata.
        *   Automatic "Save-on-Start" logic in `gui.c`.
        *   Basic List UI in `STATE_LOAD` (rendering file names).
        *   Keyboard navigation logic for the list.
    *   **Should-Have:**
        *   Metadata preview (parsing header without loading full grid) in the UI.
        *   Sorting files by date (Newest First).
        *   Appending results (Winner) to the file end upon `STATE_GAME_OVER`.
    *   **Could-Have:**
        *   Deleting files from the UI.
        *   Sub-directory navigation.
    *   **Won't-Have (in this increment):**
        *   JSON formatting.
        *   Mouse support for file selection (Keyboard only for MVP).

*   **Dependencies:**
    1.  **File I/O Update:** The `file_io.c` module must be updated to support the new format *before* the UI can display metadata or load correctly.
    2.  **Directory Access:** Platform-specific (Windows `dirent.h` or WinAPI) logic is needed to list files.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority | Est. Effort |
| :-- | :--- | :--- | :--- | :--- |
| **PB-01** | Protocol | **Task:** Update `file_io.c` `save_grid` to include `max_rounds`, `delay`, `timestamp`. | High | 1h |
| **PB-02** | Protocol | **Task:** Update `file_io.c` `load_grid` to parse new header (with backward compatibility). | High | 1.5h |
| **PB-03** | Protocol | **Task:** Implement `list_files` helper function (using `dirent.h` or equivalent for Win32). | High | 1h |
| **PB-04** | Automation | **Story 1:** Integrate `save_grid` call into `STATE_EDIT` -> `STATE_RUNNING` transition. | High | 0.5h |
| **PB-05** | UI | **Task:** Create `STATE_LOAD` structure and render loop in `gui.c`. | High | 1h |
| **PB-06** | UI | **Story 2:** Implement scrolling/selection logic for file list. | High | 1.5h |
| **PB-07** | UI | **Story 3:** Connect `STATE_LOAD` selection to `load_grid` function. | High | 0.5h |
| **PB-08** | Protocol | **Task:** Append simulation results to file in `STATE_GAME_OVER`. | Medium | 0.5h |

---

### 5. Definition of Done (DoD)

A Product Backlog Item is considered "Done" when:

*   **Compilability:** The code compiles with `gcc` without errors or new warnings.
*   **Tests:**
    *   Manual verification: Saving a grid produces the expected file content.
    *   Manual verification: Loading the file restores the exact state.
    *   Manual verification: The "Save-on-Start" creates a file every time the simulation runs.
*   **Acceptance Criteria:** All ACs defined in the User Story are met.
*   **No Regressions:** Existing `.bio` files (if any) can still be loaded (backward compatibility).
*   **Code Style:** Code follows the existing indentation and naming conventions of the project (K&R style, clear variable names).
*   **Documentation:** Functions in `file_io.h` are documented with comments explaining the new format.
