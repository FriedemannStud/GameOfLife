# DEV_TASKS-0027: Name-Claiming with Recovery Code

> **Status (2026-06-12): COMPLETE.** All six phases implemented and verified.
> Backend (Phases 1–4) verified automatically — unit tests, full curl/HTTP matrix, secret-isolation
> inspection, reset script, and an end-to-end integration test (run live against `biotope_db`).
> Frontend (Phase 5) verified interactively by the developer (claim → recovery card + PNG → silent
> return → cross-device `409` → wrong/correct code reclaim). Finalization (Phase 6): CHANGELOG
> updated, ADR-0027 → Implemented, `black`/`ruff` clean on all feature files, full test sweep green.
> Deferred follow-up tracked as DEV_SPEC-0027 backlog B-13.

Implementation plan for unique, ownable species names with a server-generated recovery code in the
web editor.

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps
to allow for interruptions and ensure stability. After each "Verification" step, report the outcome.
This iterative process is crucial for maintaining quality.

**Briefing Documents:**
*   [ADR-0027: Name-Claiming with Server-Generated Recovery Code](../adr/ADR-0027-name-claiming-with-recovery-code.md)
*   [DEV_SPEC-0027: Name-Claiming with Recovery Code](../specs/DEV_SPEC-0027-name-claiming-with-recovery-code.md)
*   [DEV_TECH_DESIGN-0027: Technical Design](../tech_design/DEV_TECH_DESIGN-0027-name-claiming-with-recovery-code.md)

**Conventions (binding):** All code/comments in **English**. Every AI-authored function/block carries
`# KI-Agent unterstützt`. Python formatted with `black .` and linted with `ruff check .`. Each phase
is a safe stopping point.

---

## Phase 1: Auth utilities (pure, DB-free)

*Goal: Create the testable building blocks — normalization, code generation, hashing — before any
DB or endpoint logic depends on them.*

- [ ] **Step 1.1: Create `backend/app/auth_utils.py`**
    - [ ] **Action:** Add `normalize_nickname(nickname)` → `" ".join(nickname.strip().lower().split())`.
    - [ ] **Action:** Add `generate_recovery_code(n_chars=4, prefix="BIOTOP")` using `secrets.choice`
        over the Crockford Base32 alphabet `"0123456789ABCDEFGHJKMNPQRSTVWXYZ"`, returns e.g.
        `BIOTOP-7F3A`.
    - [ ] **Action:** Add `hash_recovery_code(code)` → `hashlib.sha256(code.strip().upper().encode()).hexdigest()`.
    - [ ] **Action:** Mark each function with `# KI-Agent unterstützt`.
    - [ ] **Verification:**
        1.  Run `python -c "from backend.app.auth_utils import normalize_nickname, generate_recovery_code, hash_recovery_code; print(normalize_nickname('  Vibe  Master '), generate_recovery_code(), hash_recovery_code('biotop-7f3a')[:12])"` from the repo root.
        2.  **Expected Result:** prints `vibe master`, a `BIOTOP-XXXX` code, and a 12-hex-char hash prefix; no exceptions.

- [ ] **Step 1.2: Unit tests for auth utilities**
    - [ ] **Action:** Create `backend/tests/test_auth_utils.py` (same style as `test_validators.py`,
        path-append + `if __name__ == "__main__"`).
    - [ ] **Action:** Assert: `normalize_nickname` lowercases, trims, collapses internal whitespace;
        `"VibeMaster" / "vibemaster" / "  VibeMaster "` all normalize equal.
    - [ ] **Action:** Assert: `generate_recovery_code()` matches `^BIOTOP-[0-9A-HJ-NP-TV-Z]{4}$`, has
        no `I/L/O/U`, and two calls differ.
    - [ ] **Action:** Assert: `hash_recovery_code` is case/space-insensitive on input (`"biotop-7f3a"`
        == `" BIOTOP-7F3A "`) and never equals the plaintext.
    - [ ] **Verification:**
        1.  Run `python backend/tests/test_auth_utils.py`.
        2.  **Expected Result:** all assertions pass, "OK" lines printed, exit code 0.

---

## Phase 2: Data model & payload schema

*Goal: Extend the Pydantic models so the `auth` block and the new `players` fields exist, without yet
changing endpoint behavior.*

- [ ] **Step 2.1: Add the `Auth` model and `Submission.auth`**
    - [ ] **Action:** In `backend/app/models.py` add `class Auth(BaseModel): recovery_code: str | None = Field(None, ...)`.
    - [ ] **Action:** Add `auth: Auth = Field(default_factory=Auth)` to `Submission`.
    - [ ] **Action:** Ensure `DBSubmission` does **not** inherit/serialize `auth` (build it explicitly
        from `metadata`+`config`, or override so `auth` is excluded). Add `# KI-Agent unterstützt`.
    - [ ] **Verification:**
        1.  Run `python -c "from backend.app.models import Submission, DBSubmission; s=Submission(metadata={'player_id':'a','nickname':'n'},config={'cells':[[0,0]]}); print('auth' in s.model_dump()); print('auth' in DBSubmission(metadata=s.metadata, config=s.config).model_dump())"`.
        2.  **Expected Result:** prints `True` then `False` — `auth` exists on the payload but is absent from the persisted submission.

- [ ] **Step 2.2: Extend the `Player` model**
    - [ ] **Action:** Add `nickname_normalized: str` and `recovery_code_hash: str | None = None` to
        `Player` in `models.py`.
    - [ ] **Verification:**
        1.  Run `python -c "from backend.app.models import Player; print(Player(player_id='a', nickname='N', nickname_normalized='n').model_dump().keys())"`.
        2.  **Expected Result:** key set includes `nickname_normalized` and `recovery_code_hash`; no error.

---

## Phase 3: Backend claim/reclaim logic

*Goal: Rewrite `submit_config` to enforce claiming, perform reclaim/re-binding, and return the
machine-readable error contract — with the index in place.*

- [ ] **Step 3.1: Ensure the unique index exists at startup**
    - [ ] **Action:** In `backend/app/main.py` `startup_db_client`, after a successful connection,
        call `await get_db().players.create_index("nickname_normalized", unique=True)`. Add
        `# KI-Agent unterstützt`.
    - [ ] **Verification:**
        1.  Start the backend (`uvicorn app.main:app --reload` from `backend/`, or `docker-compose up`).
        2.  In a Mongo shell or Compass, run `db.players.getIndexes()`.
        3.  **Expected Result:** an index on `nickname_normalized` with `unique: true` is present; backend log shows the MongoDB connection succeeded.

- [ ] **Step 3.2: Rewrite `submit_config` decision logic**
    - [ ] **Action:** After `validate_biotope_rules`, compute `norm = normalize_nickname(submission.metadata.nickname)` and `player = await db.players.find_one({"nickname_normalized": norm})`.
    - [ ] **Action:** **Free name (claim):** generate code, `insert_one` player with
        `player_id`, `nickname`, `nickname_normalized`, `recovery_code_hash`, and the existing
        defaults (`elo_rating`, `matches_played`, `win_rate`, `created_at`); wrap insert in
        `try/except DuplicateKeyError → raise HTTPException(409, {"error":"name_taken"})` to handle
        the race. Return `201` with `recovery_code` in the body.
    - [ ] **Action:** **Silent owner** (`player["player_id"] == metadata.player_id`): persist
        submission, return `201` (no code).
    - [ ] **Action:** **Taken by someone else:** read `submission.auth.recovery_code`; if `None` →
        `HTTPException(409, {"error":"name_taken"})`; if `hash_recovery_code(code) != player["recovery_code_hash"]`
        → `HTTPException(403, {"error":"invalid_recovery_code"})`; else `update_one` the player's
        `player_id` to the new device (re-binding), persist submission, return `201` with
        `{"reclaimed": True}`.
    - [ ] **Action:** Persist submission via `DBSubmission(metadata=..., config=...)` (no `auth`).
        Remove the old nickname-keyed upsert block (`main.py:51-69`). Add `# KI-Agent unterstützt`.
    - [ ] **Action:** Fix exception handling: keep `except ValueError → 400`, let `HTTPException`
        propagate (add `except HTTPException: raise` before the catch-all), keep `except Exception → 500`.
    - [ ] **Verification:**
        1.  Restart the backend.
        2.  **Claim:** `curl -s -i -X POST localhost:8000/api/v1/submit_config -H 'Content-Type: application/json' -d '{"metadata":{"player_id":"devA","nickname":"GrillTest"},"config":{"cells":[[0,0]]}}'` → `201` with a `recovery_code` field.
        3.  **Silent return:** repeat the exact same command (same `player_id`) → `201`, **no** `recovery_code`.
        4.  **Blocked:** same name, `"player_id":"devB"`, no auth → `409 {"error":"name_taken"}`.
        5.  **Wrong code:** add `"auth":{"recovery_code":"BIOTOP-XXXX"}` (wrong) with `devB` → `403 {"error":"invalid_recovery_code"}`.
        6.  **Reclaim:** repeat with the **correct** code from step 2 and `devB` → `201` with `"reclaimed":true`.
        7.  **Expected Result:** all six responses match exactly as stated.

- [ ] **Step 3.3: Confirm no plaintext code is persisted**
    - [ ] **Action:** None (inspection only).
    - [ ] **Verification:**
        1.  After the Step 3.2 reclaim, in Mongo inspect the last `submissions` doc and the
            `GrillTest` `players` doc.
        2.  **Expected Result:** the submission has **no** `auth`/`recovery_code` field; the player has
            `recovery_code_hash` (a 64-hex string) and **no** plaintext code; `player_id` is now `devB`.

---

## Phase 4: Reset script & integration test

*Goal: Provide the clean-slate operational tool and an automated end-to-end check.*

- [ ] **Step 4.1: Reset script**
    - [ ] **Action:** Create `backend/scripts/reset_db.py` (async, uses `get_db()`): `delete_many({})`
        on `players` and `submissions`, then `create_index("nickname_normalized", unique=True)`,
        print a summary. Guard with `if __name__ == "__main__": asyncio.run(main())`. Add
        `# KI-Agent unterstützt`.
    - [ ] **Verification:**
        1.  Run `python backend/scripts/reset_db.py`.
        2.  In Mongo: `db.players.countDocuments({})`, `db.submissions.countDocuments({})`, `db.players.getIndexes()`.
        3.  **Expected Result:** both counts `0`; unique index on `nickname_normalized` present; script prints a clear summary.

- [ ] **Step 4.2: Integration test for claim → block → reclaim**
    - [ ] **Action:** Extend `backend/tests/test_system_integration.py` (or add
        `test_name_claiming.py`) with a flow using FastAPI `TestClient` (or `httpx`) that asserts the
        full matrix: claim `201`+code, silent `201` no code, `409`, `403`, reclaim `201`+`reclaimed`,
        and that the stored submission contains no recovery code. Reset the relevant docs at test
        start. Add `# KI-Agent unterstützt`.
    - [ ] **Verification:**
        1.  Run `python backend/tests/test_name_claiming.py` (with backend DB reachable).
        2.  **Expected Result:** all assertions pass; exit code 0.

- [ ] **Step 4.3: Regression check**
    - [ ] **Action:** None.
    - [ ] **Verification:**
        1.  Run `python backend/tests/test_validators.py`, `python backend/tests/test_ranking.py`.
        2.  **Expected Result:** both pass — no regressions from the model/endpoint changes.

---

## Phase 5: Frontend — submit, reclaim, recovery card

*Goal: Wire the web editor to the new contract and present/collect the recovery code. (Interactive
tests are run by the developer, per the Team Principle.)*

- [ ] **Step 5.1: Send the `auth` block**
    - [ ] **Action:** In `web/editor/editor.html` `submitBtn.onclick`, add `auth: { recovery_code: <reclaim input value or null> }` to the payload. Add a hidden `#recovery-input` group in the markup. Comment `// KI-Agent unterstützt`.
    - [ ] **Verification (Interactive Test):**
        1.  Serve the editor (backend running), open it, draw a pattern, enter a fresh name, submit.
        2.  In DevTools → Network, inspect the request payload.
        3.  **Expected Result:** the payload contains `auth: {recovery_code: null}` alongside `metadata` and `config`.

- [ ] **Step 5.2: Handle the response matrix**
    - [ ] **Action:** Branch on status/body: `201`+`recovery_code` → open recovery overlay; `201`
        plain → success; `201`+`reclaimed` → success + hide reclaim input; `409` → reveal
        `#recovery-input` with hint; `403` → keep input + "Code falsch"; `400` → show `detail`.
        Comment `// KI-Agent unterstützt`.
    - [ ] **Verification (Interactive Test):**
        1.  Submit a brand-new name → observe the recovery overlay appears with a code.
        2.  In a different browser/profile (new `player_id`), submit the **same** name → observe the recovery input appears ("Name vergeben").
        3.  Enter a wrong code, submit → observe "Code falsch", input stays.
        4.  Enter the correct code, submit → observe success ("zurückgeholt").
        5.  **Expected Result:** each of the four cases behaves exactly as described.

- [ ] **Step 5.3: Recovery card overlay + PNG download**
    - [ ] **Action:** Add `#recovery-overlay` modal with `#recovery-card` (species name + big code +
        date), a "Verstanden / Schließen" button (hides overlay), and a "Als Bild speichern" button
        that renders the card to a `<canvas>` and triggers `canvas.toBlob` → `<a download>`. If
        `canvas`/`toBlob` is unavailable, hide the save button. Comment `// KI-Agent unterstützt`.
    - [ ] **Verification (Interactive Test):**
        1.  Claim a new name; on the overlay press "Als Bild speichern".
        2.  Open the downloaded PNG; press "Verstanden".
        3.  **Expected Result:** the PNG shows name + code legibly; "Verstanden" closes the overlay; re-submitting the same name on the same browser still returns success silently (code remained valid).

---

## Phase 6: Finalization

*Goal: Documentation and sign-off.*

- [ ] **Step 6.1: Update CHANGELOG and ADR status**
    - [ ] **Action:** Add an entry to `docs/CHANGELOG.md` summarizing the feature. Change ADR-0027
        **Status** from `proposed` to `Implemented`.
    - [ ] **Verification:** Confirm both files reflect the completed feature.

- [ ] **Step 6.2: Final formatting & test sweep**
    - [ ] **Action:** Run `black .` and `ruff check .` in `backend/`.
    - [ ] **Verification:**
        1.  Run all backend tests (`test_auth_utils.py`, `test_name_claiming.py`, `test_validators.py`, `test_ranking.py`).
        2.  **Expected Result:** formatting clean, lint clean, all tests pass.
