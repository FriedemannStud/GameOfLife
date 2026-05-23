# Requirements Analysis & Specification: Global Tournament Epoch Architecture

This document details the requirements for the periodic, high-performance Round-Robin tournament architecture, as described in **ADR-0014**.

---

### 1. Detailed Requirements Specification

The primary objective is to replace the inefficient, continuous Elo-based 1v1 matchmaking with a highly scalable, epoch-based batch processing system. This system must accurately evaluate the performance of all active Game of Life patterns submitted by participants in a "Round-Robin" (everybody vs. everybody) format.

**Key Functional Requirements:**

1.  **Epoch Orchestration (Python Backend):**
    *   The backend must run a scheduled job (e.g., every 60 seconds) representing a tournament "Epoch".
    *   During an Epoch, the backend must query the database for all active pattern submissions.
    *   The backend must serialize all retrieved submissions into a single, structured format (e.g., a massive JSON array) and save it to a temporary batch file on disk.
    *   The backend must then execute the `biotope_hyper_worker` C-binary, passing the batch file as an argument.
    *   Upon completion of the C-binary, the backend must read the resulting output file containing the updated rankings and write these results back to the MongoDB.

2.  **Hyper-Worker Execution (C-Core):**
    *   A new C executable (`biotope_hyper_worker`) must be created, replacing the behavior of the current `biotope_headless`.
    *   The worker must parse the provided JSON batch file containing $N$ patterns.
    *   It must execute a double `for` loop to match every pattern against every other pattern ($O(N^2)$).
    *   **Home/Away Symmetry:** Within the loop, every pair (Pattern A and Pattern B) must play two distinct matches:
        *   Match 1: Pattern A starts on the Left, Pattern B on the Right.
        *   Match 2: Pattern B starts on the Left, Pattern A on the Right.
    *   **Match Logic:** Each individual match must run for a maximum fixed number of generations (e.g., 1000). A match concludes by comparing the final living cell count of both teams. The winner receives 1 point, a draw yields 0.5 points for each, and a loss yields 0 points.
    *   **Early Termination:** The simulation must detect if a grid reaches a static state (Still-Life) early. If no cell changes state between generation $N$ and $N+1$, the match halts, and $N$ is recorded as `stable_at_generation` in the match statistics.
    *   **Optimization:** The $O(N^2)$ loop must be parallelized using OpenMP (`#pragma omp parallel for`) to distribute the workload across all available CPU cores.
    *   The worker must output a JSON file containing an array of `player_id`s, sorted by their total accumulated score (Win-Rate).

3.  **Ranking Metric:**
    *   The database must store a "Win Percentage" or "Total Score" metric instead of an Elo rating. The leaderboard is derived purely from this score, calculated fresh during each Epoch.

---

### 2. User Stories & Acceptance Criteria

**Epic: Fair and Scalable Live Tournament**

*   **User Story 1: The Batch Matchmaker (Backend)**
    *   **As the tournament orchestrator (backend),** I want to bundle all active player submissions into a single file every minute, **so that** the C-worker can process them without the overhead of starting thousands of individual processes.
    *   **Acceptance Criteria:**
        *   A scheduled task runs every $X$ seconds.
        *   The task queries MongoDB for all documents where `status == 'active'`.
        *   It generates a single, valid JSON file containing all pattern data.
        *   It successfully triggers the `biotope_hyper_worker` via `subprocess.run` with the batch file path.

*   **User Story 2: The Hyper-Worker Round-Robin (C-Core)**
    *   **As the simulation engine,** I want to load a batch file and execute a complete Round-Robin tournament internally using all CPU cores, **so that** I can calculate the results of thousands of matches in mere seconds.
    *   **Acceptance Criteria:**
        *   The `biotope_hyper_worker` successfully parses a JSON array of patterns using `cJSON`.
        *   It executes exactly $N \times (N-1)$ matches (due to the Home/Away requirement).
        *   The match loop is parallelized with OpenMP.
        *   It outputs a JSON array containing the `player_id` and their final `score`.

*   **User Story 3: Fair "Home and Away" Matches**
    *   **As a player,** I want my pattern to play on both the left and right side of the grid against every opponent, **so that** positional advantages or asymmetry in the grid rules do not unfairly impact my score.
    *   **Acceptance Criteria:**
        *   For any given pair (A, B), the worker simulates A(Left) vs B(Right) and B(Left) vs A(Right).
        *   The scores from both matches are added to the respective totals.

*   **User Story 4: Live Leaderboard Updates**
    *   **As a player at the exhibition,** I want the leaderboard to update predictably every minute reflecting the latest submissions, **so that** I can instantly see how my new pattern performs against the entire university.
    *   **Acceptance Criteria:**
        *   The backend parses the output JSON from the Hyper-Worker.
        *   It updates the "score" and "rank" fields in the MongoDB for all active players in a single bulk operation.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   C-Hyper-Worker capable of parsing a batch JSON and running an $O(N^2)$ loop.
        *   Python scheduler to trigger the worker and read the result.
        *   Home/Away match logic in C.
    *   **Should-Have:**
        *   OpenMP parallelization in the C loop (critical for scaling beyond ~50 players).
        *   Early termination in C (stop simulation if a stable Still-Life state is reached before 1000 generations).
    *   **Could-Have:**
        *   "Asteroids" (static dead blocks) placed in the center of the grid to break purely deterministic head-on collisions.
    *   **Won't-Have (in this increment):**
        *   Elo calculations (completely replaced by the Epoch scoring).
        *   Historical persistence of every single 1v1 match (only the final epoch scores are saved to avoid database bloat).

*   **Dependencies:**
    1.  **Topic:** The `biotope_hyper_worker` C code must be written and tested before the Python `worker.py` can be refactored to use it.
    2.  **Topic:** The database schema (`models.py`) must be updated to remove Elo and support `current_epoch_score` before the backend orchestrator is deployed.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| TSK-01 | Tournament | Create `biotope_hyper_worker.c` skeleton and basic JSON batch parsing. | Must-Have |
| TSK-02 | Tournament | Implement the $O(N^2)$ loop and the "Home/Away" initialization logic. | Must-Have |
| TSK-03 | Tournament | Implement the scoring system and JSON result generation in C. | Must-Have |
| TSK-04 | Tournament | Add OpenMP `#pragma` directives to the C loop and verify thread safety. | Should-Have |
| TSK-05 | Tournament | Refactor MongoDB schema (remove Elo, add epoch score). | Must-Have |
| TSK-06 | Tournament | Rewrite `worker.py` to act as an Epoch Scheduler instead of an asynchronous queue consumer. | Must-Have |
| TSK-07 | Tournament | Implement Early Termination logic (Still-Life detection) in `game_logic.c`. | Should-Have |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The code is written and formatted according to the guidelines in `docs/CODING_STYLE.md` (`black .`, `ruff check .` for Python; standard C formatting for native code).
*   **Tests:**
    *   All new backend functions (especially the JSON batch generation) are covered by unit tests.
    *   The C-Hyper-Worker is verified by an integration test (feeding it a known JSON batch and verifying the output ranking).
    *   All existing tests continue to pass (no regressions).
*   **Acceptance Criteria:** All acceptance criteria defined for the story have been met and manually verified.
*   **Code Review:** The code has been reviewed by at least one other team member (or is in a reviewable state in a pull request).
*   **Merge:** The code has been successfully merged into the main development branch (e.g., `main` or `develop`).
*   **Documentation:** Necessary changes to technical documentation have been made.