# Requirements Analysis & Specification: Matchmaking Service and Tournament Architecture

This document details the requirements for the Matchmaking Service and Job-Queue, as described in **ADR-0012**.

---

### 1. Detailed Requirements Specification

The Matchmaking Service transforms Biotope from a single-player submission tool into a competitive multiplayer platform. It orchestrates the automated execution of matches between user-submitted patterns and manages the competitive ranking system.

#### 1.1 Database Integration
- **MongoDB Atlas:** The system must connect to the provided MongoDB Atlas cluster.
- **Data Collections:** The database must maintain three core collections:
    - `players`: Tracks player identity and global Elo rating.
    - `submissions`: Stores the validated 8x8 JSON patterns submitted via the REST API.
    - `matches`: Records the outcome of every simulation run by the headless worker.
- **Asynchronous Driver:** The backend must use an asynchronous MongoDB driver (like `motor` for Python) to prevent blocking the event loop during database operations.

#### 1.2 Matchmaking Algorithm ("Proximity Swiss")
- **Rating Range:** The algorithm must prioritize pairing players whose current Elo ratings are within a ±150 point bracket.
- **Placement Matches:** New submissions (or players with 0 matches) must be paired quickly to establish a baseline rating.
- **Frequency Constraints:** A single submission should not play against the exact same opponent consecutively to ensure diverse data gathering.

#### 1.3 Job Queue & Execution Worker
- **Database Polling:** A dedicated Python worker process (the `matchmaker`) must poll the MongoDB `submissions` collection at regular intervals to identify candidates for new matches.
- **Atomic Operations:** The worker must use atomic operations (e.g., `find_one_and_update` with a specific status flag) to claim matchmaking tasks, ensuring that if we scale to multiple workers, they don't schedule identical matches simultaneously.
- **Headless Invocation:** Once a pair is determined, the worker must:
    1. Write the patterns to temporary files.
    2. Invoke the `biotope_headless` binary as a subprocess.
    3. Parse the resulting JSON output.
    4. Clean up the temporary files.

#### 1.4 Ranking System (Elo)
- **Calculation:** After a match, the `RankingService` must calculate the Elo delta based on the winner/draw outcome.
- **Dynamic K-Factor:** The system should implement a K-factor that is higher (e.g., 40) for the first 10 matches of a submission, and lowers (e.g., 20) as the submission's rating stabilizes.

---

### 2. User Stories & Acceptance Criteria

**Epic: Automated Matchmaking and Ranking Ecosystem**

*   **User Story 1: Database Setup and Connection**
    *   **As a system administrator,** I want the backend to connect to MongoDB Atlas securely, **so that** user data and match history can be stored persistently.
    *   **Acceptance Criteria:**
        *   The backend connects to the database using the `MONGODB_URI` environment variable.
        *   The connection utilizes an asynchronous driver (e.g., `motor`).
        *   The backend logs a successful connection upon startup or fails gracefully if the URI is invalid.

*   **User Story 2: Elo-Based Matchmaking Selection**
    *   **As a competitive player,** I want to be matched against opponents of similar skill, **so that** the leaderboard accurately reflects pattern quality.
    *   **Acceptance Criteria:**
        *   The algorithm selects two distinct submissions for a match.
        *   The absolute difference in Elo ratings between the paired submissions is minimized (ideally <= 150).
        *   New submissions are guaranteed to be placed into a match within a reasonable timeframe (e.g., next polling cycle).

*   **User Story 3: Automated Headless Execution**
    *   **As a backend service,** I want to automatically trigger the C-based simulation worker with the paired submissions, **so that** match outcomes are determined without human intervention.
    *   **Acceptance Criteria:**
        *   The Python worker successfully spawns the `biotope_headless` process.
        *   The worker passes the correct JSON files to the binary.
        *   The worker accurately parses the output JSON from the headless simulation.
        *   Temporary input/output files are securely deleted after parsing.

*   **User Story 4: Rating Updates**
    *   **As a competitive player,** I want my Elo rating to update immediately after a match concludes, **so that** I can track my progress.
    *   **Acceptance Criteria:**
        *   The `matches` collection is updated with a new document containing the result.
        *   The `players` (or `submissions`) collection is updated with the newly calculated Elo ratings for both participants.
        *   The Elo calculation correctly handles wins, losses, and draws.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   MongoDB Atlas connection and schema definition.
        *   Worker process that can execute `biotope_headless` as a subprocess.
        *   Basic random pairing algorithm (if Elo is too complex for MVP, establish random pairing first).
        *   Saving match results back to the database.
    *   **Should-Have:**
        *   Proximity-based Elo matchmaking logic.
        *   Dynamic K-Factor implementation.
        *   Atomic task claiming to prevent race conditions.
    *   **Could-Have:**
        *   A REST API endpoint to query the current Leaderboard.
        *   A REST API endpoint to view the match history of a specific player.
    *   **Won't-Have (in this increment):**
        *   A real-time WebSocket connection to stream live matches to spectators (This belongs to Issue #7).

*   **Dependencies:**
    1.  **Topic:** Headless Worker (Issue #3). The Matchmaker depends entirely on the robust execution of `biotope_headless`.
    2.  **Topic:** API Submission (Issue #4). The Matchmaker consumes patterns submitted via the REST API.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| 001 | Matchmaking Ecosystem | Setup `motor` dependency and establish MongoDB connection in FastAPI. | Must |
| 002 | Matchmaking Ecosystem | Define Pydantic models / DB schemas for `Player`, `Submission`, and `Match`. | Must |
| 003 | Matchmaking Ecosystem | Refactor the `submit_config` endpoint to save patterns to MongoDB instead of local files. | Must |
| 004 | Matchmaking Ecosystem | Create the standalone `matchmaker.py` polling worker script. | Must |
| 005 | Matchmaking Ecosystem | Implement the subprocess execution of `biotope_headless` inside the worker. | Must |
| 006 | Matchmaking Ecosystem | Implement the Elo rating calculation function. | Should |
| 007 | Matchmaking Ecosystem | Implement the "Proximity Swiss" pairing logic. | Should |
| 008 | Matchmaking Ecosystem | Add a `GET /api/v1/leaderboard` endpoint to retrieve top players. | Could |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The code is written and formatted according to the guidelines in `docs/CODING_STYLE.md` (`black .`, `ruff check .`). Python scripts include AI attribution comments where applicable.
*   **Database:** MongoDB operations handle asynchronous contexts correctly.
*   **Tests:**
    *   The Elo calculation logic is verified by unit tests.
    *   The worker execution logic is verified by integration tests (mocking the headless binary if necessary).
*   **Acceptance Criteria:** All acceptance criteria defined for the story have been met.
*   **Documentation:** Technical documentation (ADR, Specs) is up-to-date.