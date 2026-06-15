# DEV_TASKS-0029: Prevent Accidental Duplicate Submissions

> **INVALID / WITHDRAWN (2026-06-15).** This task plan is void. The underlying feature was a
> mistake; see [ADR-0029](../adr/ADR-0029-prevent-duplicate-submissions.md), which has been
> declared invalid. Do **not** implement the steps below. Retained for historical reference only.

Implementation plan for the per-player species-name uniqueness rule that stops repeated
"Ab in die Arena" presses from creating duplicate submissions.

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps
to allow for interruptions and ensure stability. After each "Verification" step, report the outcome.
This iterative process is crucial for maintaining quality.

**Briefing Document:**
*   [ADR-0029: Prevent Accidental Duplicate Submissions via Per-Player Name Uniqueness](../adr/ADR-0029-prevent-duplicate-submissions.md)

**Conventions (binding):** All code/comments in **English**. Every AI-authored function/block carries
`# KI-Agent unterstützt` (Python) / `// KI-Agent unterstützt` (JS). Python formatted with `black .`
and linted with `ruff check .`. Each phase is a safe stopping point. Interactive tests (Phase 2) are
run by the developer per the Team Principle.

---

## Phase 1: Server backstop — reject same-name re-submits

*Goal: Make branch 2b (silent owner) deterministically reject a duplicate, and persist a derived
`nickname_normalized` on submissions for the check and for later analysis. No frontend change yet.*

- [ ] **Step 1.1: Persist `nickname_normalized` on each submission**
    - [ ] **Action:** In `backend/app/main.py` `_persist_submission`, after building the
        `DBSubmission`, add the derived key to the inserted document:
        `doc = db_submission.model_dump(); doc["nickname_normalized"] = normalize_nickname(submission.metadata.nickname)`
        (mirrors how the worker adds `win_rate` etc. dynamically). Insert `doc`. Add
        `# KI-Agent unterstützt`.
    - [ ] **Verification:**
        1.  Restart the backend; submit a fresh name via `curl` (claim path):
            `curl -s -X POST localhost:8000/api/v1/submit_config -H 'Content-Type: application/json' -d '{"metadata":{"player_id":"devDup","nickname":"  Dup  Test "},"config":{"cells":[[0,0]]}}'`
        2.  In Mongo, inspect the new `submissions` doc.
        3.  **Expected Result:** the document has `nickname_normalized: "dup test"` (trimmed,
            lowercased, whitespace collapsed) alongside `metadata`/`config`.

- [ ] **Step 1.2: Reject duplicates in branch 2b (silent owner)**
    - [ ] **Action:** In `submit_config`, inside the silent-owner branch
        (`player["player_id"] == submission.metadata.player_id`, `main.py:150-157`), **before**
        persisting, run:
        `existing = await db.submissions.find_one({"metadata.player_id": submission.metadata.player_id, "nickname_normalized": norm, "status": "active"})`
        and if `existing is not None`, `raise HTTPException(status_code=409, detail={"error": "duplicate_submission"})`.
        Only persist when no active duplicate exists. Add `# KI-Agent unterstützt`.
    - [ ] **Action:** Confirm the existing `except HTTPException: raise` (`main.py:182-184`) lets the
        new `409` propagate unchanged (do **not** let it fall into the catch-all `500`).
    - [ ] **Verification:**
        1.  Restart the backend.
        2.  **Claim:** `curl -s -i -X POST localhost:8000/api/v1/submit_config -H 'Content-Type: application/json' -d '{"metadata":{"player_id":"devC","nickname":"DupGuard"},"config":{"cells":[[0,0]]}}'` → `201` (with `recovery_code`).
        3.  **Duplicate (the accidental case):** repeat the **exact same** command (same `player_id`, same name) → now `409 {"detail":{"error":"duplicate_submission"}}` (previously this returned `201`).
        4.  **Different name, same player:** same `player_id`, `"nickname":"DupGuard2"` → `201` (a new species is fine).
        5.  **Cross-player (no regression):** `"player_id":"devD"`, `"nickname":"DupGuard"`, no auth → still `409 {"detail":{"error":"name_taken"}}` (ADR-0027 path unchanged).
        6.  **Expected Result:** all four responses match exactly; the two `409`s carry **different** `error` values.

- [ ] **Step 1.3: Test coverage for the duplicate path**
    - [ ] **Action:** Extend `backend/tests/test_name_claiming.py` with assertions: a second submit of
        the same `(player_id, name)` returns `409` with `error == "duplicate_submission"`; a submit of
        a **different** name by the same `player_id` returns `201`; the persisted submission carries
        `nickname_normalized`. Reset the relevant docs at test start. Add `# KI-Agent unterstützt`.
    - [ ] **Verification:**
        1.  Run `python backend/tests/test_name_claiming.py` (backend DB reachable).
        2.  Run `python backend/tests/test_validators.py` and `python backend/tests/test_ranking.py`.
        3.  **Expected Result:** all assertions pass; no regressions in the existing claim/reclaim matrix.

---

## Phase 2: Frontend — browser guard + split the 409 handler

*Goal: Disable resubmission until the species name changes, and present a distinct message for the
duplicate case. (Interactive tests are run by the developer.)*

- [ ] **Step 2.1: Remember the last-submitted name and gate the submit button**
    - [ ] **Action:** In `web/editor/editor.html`, add a client normalizer mirroring the server:
        `const normName = s => s.trim().toLowerCase().replace(/\s+/g, ' ')`. Add a module-level
        `let lastSubmittedName = null`. On a successful `201`, set
        `lastSubmittedName = normName(speciesName)`. Comment `// KI-Agent unterstützt`.
    - [ ] **Action:** In `updateUI()`, extend the submit-disable condition so the button is also
        disabled when `lastSubmittedName !== null && normName(nicknameInput.value) === lastSubmittedName`.
        Add a short hint near the button (e.g. in the status line) prompting a rename.
    - [ ] **Verification (Interactive Test):**
        1.  Serve the editor (backend running), draw a pattern, enter a fresh name, submit.
        2.  Without changing anything, observe the "Ab in die Arena" button.
        3.  Change one character of the species name.
        4.  **Expected Result:** after the successful submit the button is **disabled** and a rename
            hint is visible; editing the name to a new value **re-enables** it; restoring the exact
            old name disables it again.

- [ ] **Step 2.2: Split the 409 handler (`name_taken` vs `duplicate_submission`)**
    - [ ] **Action:** In `submitBtn.onclick`, change the `res.status === 409` branch to read
        `data.detail.error`. For `"name_taken"` keep the existing recovery-code reveal
        (`showRecoveryGroup()`, ADR-0027). For `"duplicate_submission"` show a message such as
        "Diese Spezies hast du schon eingereicht — wähle einen neuen Namen." and ensure the button
        stays disabled until the name changes (set `lastSubmittedName = normName(speciesName)` so the
        Step 2.1 gate engages). Comment `// KI-Agent unterstützt`.
    - [ ] **Verification (Interactive Test):**
        1.  Submit a brand-new name → success.
        2.  Press submit again without renaming (if reachable, e.g. via a stale tab) → observe the
            duplicate message, **not** the recovery-code card.
        3.  In a different browser/profile, submit that **same** name → observe the recovery-code
            reveal (ADR-0027 path) still works.
        4.  **Expected Result:** the two 409 situations show clearly different UI; the duplicate path
            never reveals the recovery input.

---

## Phase 3: Finalization

*Goal: Documentation and sign-off.*

- [ ] **Step 3.1: Update CHANGELOG and ADR status**
    - [ ] **Action:** Add an entry to `docs/CHANGELOG.md` summarizing the duplicate-submission guard.
        Change ADR-0029 **Status** from `Proposed` to `Implemented`.
    - [ ] **Verification:** Confirm both files reflect the completed feature.

- [ ] **Step 3.2: Final formatting & test sweep**
    - [ ] **Action:** Run `black .` and `ruff check .` in `backend/`.
    - [ ] **Verification:**
        1.  Run `python backend/tests/test_name_claiming.py`, `python backend/tests/test_validators.py`, `python backend/tests/test_ranking.py`.
        2.  **Expected Result:** formatting clean, lint clean, all tests pass.
