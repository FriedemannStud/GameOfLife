# Requirements Analysis & Specification: C-Networking and API Integration

This document details the requirements for the C-Networking and API Integration, as described in **ADR-0017**.

---

### 1. Detailed Requirements Specification

The primary objective is to enable the Raylib C-Application to dynamically fetch tournament data (leaderboards and match highlights) from the FastAPI backend. This requires introducing HTTP networking capabilities to the C-App.

**1.1 Backend Extensions (Python/FastAPI):**
*   **Database:** Create a new MongoDB collection named `epoch_highlights` to store metadata about the best matches from each tournament epoch (seed configurations, participant names, metrics).
*   **API Endpoints:**
    *   `GET /api/leaderboard`: Returns a JSON array containing the current top rankings (e.g., Name, Elo, Win Rate).
    *   `GET /api/epoch/highlights`: Returns a JSON object containing the 8x8 seed configurations and metadata for the "Greatest Hits" of the most recent epoch.

**1.2 C-Application Data Layer (C/libcurl):**
*   **Library Integration:** Integrate `libcurl` for HTTP requests and utilize the existing `cJSON` library for parsing responses.
*   **Module Encapsulation:** Create a new file (e.g., `src/io/network_io.c`) to encapsulate all network logic, ensuring the `src/core/` and `src/gui/` modules remain oblivious to the networking implementation.
*   **Non-Blocking Behavior:** The network requests must not stall the Raylib rendering loop (60 FPS). This requires either careful timeout configuration on synchronous calls or, preferably, executing the `libcurl` requests on a background thread.
*   **Error Handling:** The C-App must robustly handle network failures (timeouts, connection refused, 404/500 errors) and malformed JSON payloads. In the event of a failure, the Kiosk Mode should log the error and continue rendering previous or default data without crashing.

---

### 2. User Stories & Acceptance Criteria

**Epic: Kiosk Mode Data Synchronization**

*   **User Story 1: Provide Tournament Highlights via API**
    *   **As a backend developer,** I want to provide an API endpoint for tournament highlights, **so that** the Kiosk application can fetch the data needed to render replay simulations.
    *   **Acceptance Criteria:**
        *   A new MongoDB collection `epoch_highlights` exists.
        *   The tournament worker writes highlight match metadata (seeds, names, metric types) to this collection at the end of every epoch.
        *   The endpoint `GET /api/epoch/highlights` returns a valid JSON payload containing the latest highlights.
        *   The endpoint handles requests efficiently and returns appropriate HTTP status codes (e.g., 200 OK, 404 Not Found if no data exists yet).

*   **User Story 2: Provide Live Leaderboard via API**
    *   **As a backend developer,** I want to provide an API endpoint for the current leaderboard, **so that** the Kiosk application can display the latest rankings.
    *   **Acceptance Criteria:**
        *   The endpoint `GET /api/leaderboard` returns a valid JSON array of the top participants sorted by Elo/Wins.
        *   The payload format is consistent and documented.

*   **User Story 3: Fetch Data without Blocking UI**
    *   **As a Kiosk Mode observer,** I want the visual simulation to run smoothly without stuttering, **even when** the application is requesting new data from the backend.
    *   **Acceptance Criteria:**
        *   The C-Application uses `libcurl` to fetch data from `/api/leaderboard` and `/api/epoch/highlights`.
        *   The HTTP requests do not block the main Raylib thread; the application maintains ~60 FPS during network operations.
        *   The application gracefully handles network timeouts or connection drops without crashing or displaying empty/broken views.

*   **User Story 4: Parse JSON Responses**
    *   **As a C developer,** I need to parse the JSON responses from the API, **so that** I can extract the leaderboard data and the 8x8 seed configurations into C data structures.
    *   **Acceptance Criteria:**
        *   The C-Application successfully parses the JSON payloads using `cJSON`.
        *   Leaderboard data is populated into an array of `Participant` structs.
        *   Highlight data is populated into `MatchConfig` or equivalent structs containing the 8x8 grids.
        *   Memory allocated by `cJSON` is correctly freed after parsing to prevent memory leaks.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Backend: `GET /api/leaderboard` and `GET /api/epoch/highlights` endpoints.
        *   Backend: Logic to identify and store highlights in `epoch_highlights`.
        *   C-App: Basic HTTP GET functionality using `libcurl`.
        *   C-App: JSON parsing using `cJSON`.
        *   C-App: Non-blocking or sufficiently fast polling to avoid UI stutters.
    *   **Should-Have:**
        *   Robust C-level error handling (retry logic, fallback data).
        *   Background threading for `libcurl` calls to guarantee UI fluidity.
    *   **Could-Have:**
        *   Caching of responses to reduce backend load.
    *   **Won't-Have (in this increment):**
        *   WebSocket implementation (as per ADR-0017).
        *   Rendering of the UI components (handled in WP 4/ADR-0019).

*   **Dependencies:**
    1.  **Backend Data:** The C-Networking layer (WP 2) depends entirely on the existence and stability of the FastAPI endpoints (WP 1). Backend work must be completed first or mocked.
    2.  **Multicam Rendering:** The rendering of highlights (WP 3/ADR-0018) depends on the C-Networking layer successfully providing the seed configurations.
    3.  **Kiosk UI:** The final Kiosk State Machine (WP 4/ADR-0019) depends on both the networking layer and the rendering refactoring.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| 1 | Kiosk Data | Backend: Create `epoch_highlights` MongoDB collection and update Worker logic to populate it. | Must-Have |
| 2 | Kiosk Data | Backend: Implement `GET /api/leaderboard` endpoint. | Must-Have |
| 3 | Kiosk Data | Backend: Implement `GET /api/epoch/highlights` endpoint. | Must-Have |
| 4 | Kiosk Data | C-App: Update `Makefile` to link `libcurl`. | Must-Have |
| 5 | Kiosk Data | C-App: Implement `network_io.c` with a basic `libcurl` GET function. | Must-Have |
| 6 | Kiosk Data | C-App: Implement background threading or asynchronous handling for `libcurl` requests to prevent UI blocking. | Must-Have |
| 7 | Kiosk Data | C-App: Implement `cJSON` parsing logic for the leaderboard payload. | Must-Have |
| 8 | Kiosk Data | C-App: Implement `cJSON` parsing logic for the highlights payload (extracting 8x8 seeds). | Must-Have |
| 9 | Kiosk Data | C-App: Add robust error handling (timeouts, invalid JSON, memory cleanup) to `network_io.c`. | Should-Have |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:**
    *   Python code follows PEP 8 / project standards (`ruff check .`, `black .` if applicable).
    *   C code complies strictly with `docs/CODING_STYLE.md` (snake_case formatting, AI attribution, memory management).
*   **Tests:**
    *   New backend endpoints are covered by tests in `backend/tests/`.
    *   C-application logic is tested via isolated test drivers (e.g., `tests/test_network_io.c`) or robust interactive verification without crashing.
    *   All existing tests (`make clean && make`, `pytest`) continue to pass.
*   **Acceptance Criteria:** All acceptance criteria defined for the story have been met and manually verified.
*   **Execution Verification:** The C-Application compiles successfully (`make`) without warnings related to the new code and links `libcurl` correctly.
*   **Memory Safety:** Valgrind or similar manual analysis confirms no memory leaks exist in the C-Networking parsing logic (`cJSON` cleanup).
*   **Documentation:** Technical documentation (e.g., `DEV_TASKS-0017.md`) is updated to reflect progress.
