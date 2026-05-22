# DEV_TASKS-0012: Matchmaking Service and Tournament Architecture

This document breaks down the implementation of the Matchmaking Service and Job-Queue (Issue #5) into actionable, verifiable steps for a Full-Stack developer. Quality precedes speed.

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality and adhering to `docs/CODING_STYLE.md`. Remember to add `// KI-Agent unterstützt` to generated code.

**Briefing Documents:**
*   [ADR-0012: Matchmaking Service and Tournament Architecture](../adr/ADR-0012-matchmaking-and-tournament-architecture.md)
*   [DEV_SPEC-0012: Matchmaking Service and Tournament Architecture](../specs/DEV_SPEC-0012-matchmaking-and-tournament-architecture.md)
*   [DEV_TECH_DESIGN-0012: Matchmaking Service and Tournament Architecture](../tech_design/DEV_TECH_DESIGN-0012-matchmaking-and-tournament-architecture.md)

---

## Phase 1: Database Foundation

*Goal: Establish a secure, asynchronous connection to MongoDB Atlas and prepare the environment.*

- [ ] **Step 1.1: Environment & Dependencies**
    - [ ] **Action:** Add `motor==3.3.2` and `python-dotenv==1.0.1` to `backend/requirements.txt`.
    - [ ] **Action:** Rebuild the backend container or install locally: `docker-compose build backend` or `pip install -r backend/requirements.txt`.
    - [ ] **Verification:** Run `docker-compose run --rm backend pip show motor` and confirm it is installed.

- [ ] **Step 1.2: Database Connection Client**
    - [ ] **Action:** Create `backend/app/database.py`.
    - [ ] **Action:** Use `motor.motor_asyncio.AsyncIOMotorClient` to establish a connection.
    - [ ] **Action:** Read the `MONGODB_URI` from environment variables using `os.getenv`. If not set, raise an explicit error.
    - [ ] **Action:** Provide a helper function `get_db()` that returns the specific database instance (e.g., `biotope_db`).
    - [ ] **Verification (Interactive Test):**
        1. Create a temporary script `test_db.py` in the backend root:
           ```python
           import asyncio
           from app.database import get_db
           async def test():
               db = get_db()
               info = await db.command("serverStatus")
               print("OK: MongoDB Connected", info.get("version"))
           asyncio.run(test())
           ```
        2. Execute `MONGODB_URI="mongodb+srv://<user>:<password>@<cluster>.mongodb.net/" python3 test_db.py`.
        3. **Expected Result:** Console prints "OK: MongoDB Connected" along with the version.
        4. Delete `test_db.py` after success.

---

## Phase 2: Schema Migration and API Update

*Goal: Update data models to support database fields and refactor the submission endpoint to save to MongoDB instead of the local filesystem.*

- [ ] **Step 2.1: Extended Pydantic Models**
    - [ ] **Action:** Open `backend/app/models.py`.
    - [ ] **Action:** Add `status: str = "active"`, `elo_rating: int = 1200`, and `matches_played: int = 0` to the `Submission` model (or create a `DBSubmission` subclass).
    - [ ] **Action:** Create a `Player` model (fields: `player_id`, `nickname`, `elo_rating`, `matches_played`).
    - [ ] **Action:** Create a `MatchResult` model (fields: `red_submission_id`, `blue_submission_id`, `winner`, `red_population`, `blue_population`, `elo_delta`).

- [ ] **Step 2.2: Refactor API Endpoint**
    - [ ] **Action:** Open `backend/app/main.py`.
    - [ ] **Action:** Modify `submit_config`. Remove the call to `save_submission(submission)` from `storage.py`.
    - [ ] **Action:** Inject the DB client into the endpoint.
    - [ ] **Action:** Upsert the Player (insert if not exists based on `player_id`).
    - [ ] **Action:** Insert the validated `Submission` into the `submissions` collection. Convert the Pydantic model to a dict using `.model_dump()`.
    - [ ] **Verification (Interactive Test):**
        1. Start the backend: `docker-compose up -d backend`.
        2. Send a valid JSON payload using `curl` to `POST /api/v1/submit_config`.
        3. **Expected Result:** API returns `201 Created` or `200 OK`.
        4. Check MongoDB Atlas (via UI or a mongo CLI) to verify that the `submissions` and `players` collections contain the new document.

---

## Phase 3: Ranking Engine

*Goal: Implement the Elo calculation logic independent of the database.*

- [ ] **Step 3.1: Elo Calculation Logic**
    - [ ] **Action:** Create `backend/app/ranking.py`.
    - [ ] **Action:** Implement `calculate_elo(rating_a: int, rating_b: int, score_a: float, matches_played_a: int) -> int` as defined in `DEV_TECH_DESIGN-0012`.
    - [ ] **Action:** Document the parameters thoroughly.

- [ ] **Step 3.2: Unit Testing Elo**
    - [ ] **Action:** Create `backend/tests/test_ranking.py`.
    - [ ] **Action:** Write test cases for: 
        * Win, Loss, and Draw scenarios.
        * New player (high K-factor) vs Established player (low K-factor).
    - [ ] **Verification:** Run `docker-compose exec backend pytest tests/test_ranking.py`. 
    - [ ] **Expected Result:** All tests pass.

---

## Phase 4: Matchmaker Worker (Proximity Swiss)

*Goal: Build the background process that selects opponents securely using atomic database operations.*

- [ ] **Step 4.1: The Polling Loop Skeleton**
    - [ ] **Action:** Create `backend/app/worker.py`.
    - [ ] **Action:** Create an `async def matchmaking_loop()` that runs `while True:` with an `asyncio.sleep(5)`.
    - [ ] **Action:** Add `if __name__ == "__main__": asyncio.run(matchmaking_loop())`.

- [ ] **Step 4.2: Atomic Opponent Selection**
    - [ ] **Action:** In `worker.py`, implement `find_match_pair(db)`.
    - [ ] **Action:** Use `db.submissions.find_one_and_update` to find ONE document where `status == "active"`, sorted by `matches_played` ASC. Set its status to `in_match`. This is Target A.
    - [ ] **Action:** If Target A is found, run a second `find_one_and_update` to find Target B where `status == "active"`, `player_id != A.player_id`, and `elo_rating` is between `A.elo - 150` and `A.elo + 150`. Set its status to `in_match`.
    - [ ] **Action:** If Target B is NOT found, revert Target A's status back to `active`.
    - [ ] **Verification (Interactive Test):**
        1. Insert 3 mock submissions into the DB directly or via the API (Player 1, Player 2, Player 3).
        2. Add a `print` statement inside `find_match_pair` showing the IDs found.
        3. Run `python3 backend/app/worker.py` manually.
        4. **Expected Result:** The console prints that it paired Player 1 and Player 2. If you stop and restart, it shouldn't pair them again if they are marked `in_match`.

---

## Phase 5: Headless Subprocess Integration

*Goal: Execute the C-binary with the matched pairs and process the outcome.*

- [ ] **Step 5.1: Secure Execution Logic**
    - [ ] **Action:** In `worker.py`, implement `execute_match(submission_a, submission_b)`.
    - [ ] **Action:** Use Python's `tempfile` to create two temporary JSON files securely in `/app/results/` (or `/tmp/`). Write the `config` of A and B into them.
    - [ ] **Action:** Use `asyncio.create_subprocess_exec("./biotope_headless", path_a, path_b)` to run the simulation.
    - [ ] **Action:** Capture stdout using `stdout=asyncio.subprocess.PIPE`.
    - [ ] **Action:** Delete the temporary files immediately in a `finally` block to prevent disk space leaks.

- [ ] **Step 5.2: Parsing and Database Updates**
    - [ ] **Action:** Parse the JSON output from `biotope_headless` stdout.
    - [ ] **Action:** Determine the winner and calculate the new Elo using `ranking.py`.
    - [ ] **Action:** Update the `submissions` collection: set `status` back to `active`, increment `matches_played`, and update `elo_rating` for both A and B.
    - [ ] **Action:** Insert a new document into the `matches` collection logging the result.
    - [ ] **Verification (Interactive Test):**
        1. Ensure the `biotope_headless` binary exists in the backend container (or paths are correctly mapped).
        2. Run `worker.py` manually.
        3. **Expected Result:** Worker pairs submissions, calls binary, calculates Elo, updates DB. Verify via MongoDB Atlas that Elo values changed.

---

## Phase 6: Orchestration and Cleanup

*Goal: Integrate the worker seamlessly into the Docker environment and finalize code quality.*

- [ ] **Step 6.1: Docker Compose Integration**
    - [ ] **Action:** Update `docker-compose.yml`. Add a new service named `matchmaker`.
    - [ ] **Action:** Use the same `build/image` and `volumes` as the backend.
    - [ ] **Action:** Set the `command` to `python3 -m app.worker`.
    - [ ] **Verification:** Run `docker-compose up -d`. Check logs with `docker-compose logs -f matchmaker`. Expected: Worker loops gracefully.

- [ ] **Step 6.2: Final Code Review**
    - [ ] **Action:** Run `black backend/` and `ruff check backend/`. Fix any linting errors.
    - [ ] **Action:** Verify that all new files have the `// KI-Agent unterstützt` (or `# KI-Agent unterstützt`) attribution.
    - [ ] **Action:** Complete all task boxes in this document.
