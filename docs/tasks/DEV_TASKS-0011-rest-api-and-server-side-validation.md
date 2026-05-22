# DEV_TASKS-0011: REST API and Server-Side Validation

Implementation of a secure, FastAPI-based backend to handle player submissions and enforce fair-play rules (38% biomass, 8x8 grid).

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality.

**Briefing Documents:**
*   [ADR-0011: REST API and Server-Side Validation](../adr/ADR-0011-rest-api-and-server-side-validation.md)
*   [DEV_SPEC-0011: REST API and Server-Side Validation](../specs/DEV_SPEC-0011-rest-api-and-server-side-validation.md)
*   [DEV_TECH_DESIGN-0011: REST API and Server-Side Validation](../tech_design/DEV_TECH_DESIGN-0011-rest-api-and-server-side-validation.md)

---

## Phase 1: Environment and Scaffolding

*Goal: Set up the Python/FastAPI environment within the Docker ecosystem.*

- [x] **Step 1.1: Create Backend Directory Structure**
    - [x] **Action:** Create a `backend/` directory in the root.
    - [x] **Action:** Create an empty `backend/app/` subdirectory for the source code.
    - [x] **Verification:** Run `ls -R backend/` and confirm the structure.

- [x] **Step 1.2: Define Dependencies**
    - [x] **Action:** Create `backend/requirements.txt` with:
        ```text
        fastapi==0.109.0
        uvicorn==0.27.0
        pydantic==2.5.3
        python-multipart==0.0.6
        ```
    - [x] **Verification:** Confirm file content with `cat backend/requirements.txt`.

- [x] **Step 1.3: Update Docker-Compose**
    - [x] **Action:** Add the `backend` service to `docker-compose.yml`.
    - [x] **Action:** Map port `8000:8000` and mount `./backend:/app`.
    - [x] **Action:** Mount the shared volume `./biotope_results:/app/results` for shared access with the C-worker.
    - [x] **Verification:** Run `docker-compose config` to check for syntax errors.

- [x] **Step 1.4: Skeleton Implementation**
    - [x] **Action:** Create a minimalist `backend/app/main.py` that returns a "Hello Biotope" JSON on `/`.
    - [x] **Action:** Run `docker-compose up backend` (or equivalent) to start the service.
    - [x] **Verification (Interactive Test):**
        1. Open a browser or use `curl`.
        2. Access `http://localhost:8000/`.
        3. **Expected Result:** `{"message": "Hello Biotope"}` is returned.

---

## Phase 2: Data Models and Structural Validation

*Goal: Implement the Pydantic models to enforce JSON schema integrity.*

- [x] **Step 2.1: Define Pydantic Models**
    - [x] **Action:** Create `backend/app/models.py`.
    - [x] **Action:** Implement `Metadata`, `Config`, and `Submission` classes as per Technical Design.
    - [x] **Verification:** Confirm the file adheres to `snake_case` for fields and `PascalCase` for classes.

- [x] **Step 2.2: Test Structural Validation**
    - [x] **Action:** Temporarily update `main.py` to accept a `Submission` object in a POST request.
    - [x] **Verification (Interactive Test):**
        1. Send a malformed JSON (missing `player_id`) to the endpoint.
        2. **Expected Result:** FastAPI returns a `422 Unprocessable Entity` automatically.

---

## Phase 3: Business Logic Validation (Fair Play)

*Goal: Enforce the 38% biomass and 8x8 grid constraints.*

- [x] **Step 3.1: Implement Validation Service**
    - [x] **Action:** Create `backend/app/validators.py`.
    - [x] **Action:** Implement `validate_biotope_rules(submission)` as per Tech Design.
    - [x] **Verification:** Ensure it checks `len(cells) <= 24` and coordinate ranges `[0..7]`.

- [x] **Step 3.2: Unit Testing Validation Logic**
    - [x] **Action:** Create `backend/tests/test_validators.py`.
    - [x] **Action:** Add tests for: Valid pattern, Too many cells (25), Out-of-bounds coordinate (8, 0).
    - [x] **Verification:** Run `pytest` (after adding it to requirements) or a simple script to verify logic.

---

## Phase 4: Persistence Layer

*Goal: Save valid submissions for consumption by the C-Worker.*

- [x] **Step 4.1: Implement File Storage**
    - [x] **Action:** Create `backend/app/storage.py`.
    - [x] **Action:** Implement logic to save a `Submission` as a JSON file in the `/app/results` directory.
    - [x] **Action:** Use a unique ID or timestamp for the filename (e.g., `match_20260522_1234.json`).
    - [x] **Verification:** Confirm the directory exists and is writeable by the backend service.

---

## Phase 5: Final API Integration

*Goal: Tie everything together in the main endpoint.*

- [x] **Step 5.1: Implement Submission Endpoint**
    - [x] **Action:** Complete `POST /api/v1/submit_config` in `main.py`.
    - [x] **Action:** Integrate the validator and storage service.
    - [x] **Verification (Interactive Test):**
        1. Submit a valid 8x8 pattern via Swagger (`/docs`).
        2. **Expected Result:** Receive `201 Created` and see the file appear in `biotope_results/`.

- [x] **Step 5.2: Interoperability Check**
    - [x] **Action:** Manually trigger `biotope_headless` using the file generated by the API.
    - [x] **Verification:** Run `./biotope_headless biotope_results/[new_file].json test_blue.json`.
    - [x] **Expected Result:** The C-worker parses and simulates the API-generated file correctly.

---

## Phase 6: Documentation and Cleanup

*Goal: Ensure the system is developer-friendly and clean.*

- [x] **Step 6.1: Refine Swagger Docs**
    - [x] **Action:** Add descriptions and examples to the Pydantic models.
    - [x] **Verification:** View `/docs` and ensure it is self-explanatory.

- [x] **Step 6.2: Final Code Review**
    - [x] **Action:** Check all files for `// KI-Agent unterstützt` comments.
    - [x] **Action:** Run a linter (e.g., `ruff`) on the Python code.
    - [x] **Verification:** No linting errors or missing attributions.
