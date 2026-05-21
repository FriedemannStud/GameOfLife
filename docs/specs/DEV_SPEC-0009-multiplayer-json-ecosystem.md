# Requirements Analysis & Specification: Multiplayer Ecosystem (Biotope)

This document details the requirements for the massive-parallel multiplayer ecosystem, as described in **ADR-0009**.

---

### 1. Detailed Requirements Specification

The primary goal is to transform the local Game of Life simulation into a distributed, asynchronous competition platform for university fairs. The system consists of three main components: a WASM-based Web Editor, a Backend API, and a Headless Simulation Worker.

#### 1.1 Data Format & Communication
- **Format:** All communication between the Web Editor and the Backend MUST use the JSON format defined in ADR-0009.
- **Protocol:** Communication happens over HTTPS using RESTful patterns (primarily `POST` for submissions).
- **Coordinate System:** Cells MUST be stored as relative `[x, y]` integer pairs. The origin `[0,0]` represents the top-left corner of the player's assigned bounding box.

#### 1.2 League Tier Constraints
The system MUST enforce strict limits based on the selected league to ensure fairness and performance:

| League | Bounding Box | Total Cells | Biomass Limit (19%) |
| :--- | :--- | :--- | :--- |
| **Einsteiger** | 8x8 | 64 | Max 12 living cells |
| **Rookie** | 50x50 | 2,500 | Max 475 living cells |
| **Champions** | 100x100 | 10,000 | Max 1,900 living cells |

#### 1.3 Validation Logic (Server-Side)
The backend MUST validate every submission:
1.  **Coordinate Check:** All cell coordinates must be within `[0, 0]` and `[limit-1, limit-1]`.
2.  **Biomass Check:** Total count of living cells must not exceed the league's 19% limit.
3.  **Schema Check:** All mandatory metadata (player_id, nickname, league) must be present.

#### 1.4 Evolutionary Features (Forking)
- The system MUST track the lineage of patterns.
- If a user starts from an existing pattern (Forking), the `parent_config_id` MUST be preserved in the submission to allow ancestry tree generation.

---

### 2. User Stories & Acceptance Criteria

**Epic: Multiplayer Participation & Competition**

*   **User Story 1: Submit Pattern via Web-Editor**
    *   **As a visitor,** I want to design a pattern on my smartphone and submit it to the competition, **so that** I can see how my creation performs against others.
    *   **Acceptance Criteria:**
        *   User can select a league (Einsteiger, Rookie, Champions).
        *   The editor restricts drawing to the league-specific bounding box.
        *   The editor provides real-time feedback on the biomass limit (e.g., "10/12 cells used").
        *   Successful submission triggers a "Success" message and provides a link to the live stream.

*   **User Story 2: Server-Side Validation**
    *   **As a system administrator,** I want the server to strictly validate all incoming JSON payloads, **so that** malicious or invalid entries do not corrupt the competition or bypass limits.
    *   **Acceptance Criteria:**
        *   Server rejects any payload exceeding the 19% biomass limit with a 400 Bad Request.
        *   Server rejects any cells outside the specified bounding box.
        *   Server assigns a unique `config_id` to every valid submission and saves it to the database.

*   **User Story 3: Evolutionary Forking**
    *   **As a competitive player,** I want to load a successful pattern from the leaderboard and modify it, **so that** I can iterate on successful strategies.
    *   **Acceptance Criteria:**
        *   "Fork" button on the leaderboard opens the editor with the selected pattern pre-loaded.
        *   The new submission correctly includes the `parent_config_id`.
        *   The metadata (nickname) can be changed by the new user.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   JSON Payload implementation (ADR-0009).
        *   Backend API for submission and storage.
        *   Server-side validation logic for the three leagues.
        *   Basic Web-Editor (WASM) with bounding box enforcement.
    *   **Should-Have:**
        *   Elo-based Matchmaking system.
        *   Headless C-Worker for automated background simulation.
        *   Live Leaderboard UI.
    *   **Could-Have:**
        *   Ancestry/Evolution Tree visualization.
        *   Custom player avatars based on their patterns.
    *   **Won't-Have (in this increment):**
        *   In-game chat for viewers.
        *   Real-time 3D rendering for the spectator view.

*   **Dependencies:**
    1.  **Data Format:** The JSON schema (ADR-0009) is the foundation for both Client and Server.
    2.  **API before Editor:** The backend must be ready to receive data before the Web-Editor can be fully tested.
    3.  **Headless Mode:** The `game_logic.c` must be decoupled from `gui.c` to allow the Headless Worker to run matches.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| 001 | Infrastructure | Implement JSON Schema and validation library in C/Backend | Must |
| 002 | Backend | Develop REST API endpoint for pattern submission | Must |
| 003 | Client | Update WASM Editor to support relative coordinates and league boxes | Must |
| 004 | Worker | Create Headless Simulation Wrapper for `game_logic.c` | Should |
| 005 | Backend | Implement Elo-Rating calculation logic | Should |
| 006 | UX | Design and implement the Live Leaderboard | Should |
| 007 | Evolution | Implement 'Forking' metadata tracking | Could |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The code follows `docs/CODING_STYLE.md` (snake_case, PascalCase, AI attribution).
*   **Validation:** Server-side validation is implemented and tested with edge cases (e.g., exactly 19% vs 19.1%).
*   **Documentation:** Every function includes educational comments for 1st-semester students as per `gemini.md`.
*   **Tests:**
    *   JSON parsing and validation are verified by unit tests.
    *   End-to-end flow (Editor -> API -> DB) is manually verified.
*   **Interoperability:** The C-based worker can successfully "stamp" the relative JSON pattern into its internal grid.

// KI-Agent unterstützt: Comprehensive specification for the Biotope Multiplayer Ecosystem.
