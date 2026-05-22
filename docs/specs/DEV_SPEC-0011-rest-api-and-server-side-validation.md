# Requirements Analysis & Specification: REST API and Server-Side Validation

This document details the requirements for the centralized backend and validation engine, as described in **ADR-0011**.

---

### 1. Detailed Requirements Specification

The primary objective is to provide a secure, robust, and scalable entry point for player-submitted patterns in the Biotope multiplayer ecosystem.

#### 1.1 API Architecture
- **Framework:** Python 3.x with FastAPI.
- **Protocol:** RESTful HTTPS.
- **Input Format:** JSON (standardized Biotope format).
- **Core Principle:** The "EVA" Principle (Input - Process - Output).

#### 1.2 Functional Requirements
- **Endpoint Implementation:** Provide a `POST /api/v1/submit_config` endpoint.
- **Structural Validation:** Verify that the incoming JSON matches the required schema (presence of `metadata`, `config`, and `cells` array).
- **Business Logic Validation (Fair Play):**
    - **Bounding Box Check:** All `[x, y]` pairs in the `cells` array must be within the range `0` to `7` (inclusive).
    - **Biomass Check:** The total number of living cells in the `cells` array must not exceed **24** (38% of the 64 available slots).
    - **Metadata Validation:** Ensure `player_id` and `nickname` are non-empty strings.
- **Persistence:** Valid submissions must be saved to a persistent storage layer (filesystem or database) in a format ready for the `biotope_headless` worker.
- **Response Handling:**
    - `201 Created` for successful submissions, including a unique `submission_id`.
    - `400 Bad Request` for validation failures, providing specific error messages (e.g., "Biomass limit exceeded").
    - `422 Unprocessable Entity` for schema/type mismatches (handled by FastAPI/Pydantic).

---

### 2. User Stories & Acceptance Criteria

**Epic: Secure Pattern Submission & Tournament Integrity**

*   **User Story 1: Submit Pattern via API**
    *   **As a player (via the Web Editor),** I want to submit my 8x8 configuration to the server, **so that** I can participate in the automated tournament.
    *   **Acceptance Criteria:**
        *   The endpoint accepts a JSON payload with valid 8x8 coordinates.
        *   The server returns a `201 Created` status code upon success.
        *   The submission is persistently stored on the server.

*   **User Story 2: Enforce Fair Play Rules**
    *   **As a system administrator,** I want the server to reject any configuration that violates the competition rules, **so that** the tournament remains fair and stable.
    *   **Acceptance Criteria:**
        *   Submissions with more than 24 cells are rejected with a clear error message.
        *   Submissions with coordinates outside the 0-7 range are rejected.
        *   Submissions missing mandatory player metadata are rejected.

*   **User Story 3: Developer Documentation (Swagger)**
    *   **As a frontend developer,** I want to see an interactive documentation of the API, **so that** I can easily integrate the Web Editor with the backend.
    *   **Acceptance Criteria:**
        *   The Swagger UI is accessible at `/docs`.
        *   The `submit_config` endpoint is fully documented with schema definitions and example payloads.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   FastAPI project setup within Docker.
        *   `POST /api/v1/submit_config` endpoint.
        *   Pydantic models for structural validation.
        *   Custom validation logic for 38% biomass and 8x8 bounding box.
        *   File-based persistence (storing JSON files for the worker).
    *   **Should-Have:**
        *   Detailed error responses in the JSON body.
        *   Automatic Swagger/OpenAPI documentation.
        *   Unit tests for validation logic.
    *   **Could-Have:**
        *   Rate limiting to prevent submission spam.
        *   Structured logging for audit trails.
    *   **Won't-Have (in this increment):**
        *   User authentication (OAuth2/JWT).
        *   Live matchmaking (Issue #5).

*   **Dependencies:**
    1.  **Topic:** JSON Format Definition (Issue #1). The API depends on the finalized JSON schema.
    2.  **Topic:** Headless Worker (Issue #3). The API must produce files compatible with the C-worker's input.
    3.  **Infrastructure:** Docker environment must support Python/FastAPI alongside the C-build system.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| 001 | Setup | Initialize FastAPI project and update `docker-compose.yml`. | Must |
| 002 | Schema | Define Pydantic models for the Biotope JSON format. | Must |
| 003 | Validation | Implement server-side business logic validation (Biomass/Grid). | Must |
| 004 | Endpoint | Create the `POST /api/v1/submit_config` endpoint. | Must |
| 005 | Persistence | Implement a simple file-based storage layer for submissions. | Must |
| 006 | Testing | Write unit tests for the validation engine. | Should |
| 007 | Docs | Refine Swagger documentation with examples and descriptions. | Should |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The code is written and formatted according to Python standards (`black`, `ruff`).
*   **AI Attribution:** Every new source file includes an AI attribution comment.
*   **Tests:**
    *   All validation rules are covered by unit tests (Positive/Negative/Edge-cases).
    *   API functionality is verified using an automated test client (FastAPI `TestClient`).
*   **Acceptance Criteria:** All acceptance criteria defined for the story have been met.
*   **Documentation:** The `/docs` endpoint correctly reflects all changes and rules.
*   **Interoperability:** The JSON files generated by the API can be successfully processed by `biotope_headless`.

// AI-attributed: Requirement specification for the Biotope REST API and Validation Engine.
