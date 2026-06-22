# DEV_TASKS-0035: Password-Protected Admin Configuration Deletion

This plan implements the admin soft-delete capability (modes A/B/C + restore) defined in
the briefing documents below.

**Developer:** Please follow these steps precisely. The plan is broken into phases and
small steps to allow for interruptions and ensure stability. After each "Verification"
step, report the outcome. This iterative process is crucial for maintaining quality.

**Briefing Documents:**
*   [ADR-0035: Password-Protected Admin Access for Configuration Deletion](../adr/ADR-0035-admin-config-deletion.md)
*   [DEV_SPEC-0035: Requirements Analysis & Specification](../specs/DEV_SPEC-0035-admin-config-deletion.md)
*   [DEV_TECH_DESIGN-0035: Technical Design](../tech_design/DEV_TECH_DESIGN-0035-admin-config-deletion.md)

**Coding rules (from `docs/CODING_STYLE.md`):** English code/comments; `snake_case`
functions; AI-written blocks carry `# KI-Agent unterstützt`; pair every resource; format
Python with `black .` and lint with `ruff check .`. Python backend tests are self-running
(`python backend/tests/test_*.py`).

---

## Phase 1: Authentication Foundation

*Goal: A constant-time, fail-closed admin password gate, with the secret in `.env`.*

- [ ] **Step 1.1: Add the `ADMIN_PASSWORD` config key**
    - [ ] **Action:** Add `ADMIN_PASSWORD=` (placeholder, no real value) to `.env.example`
        with a comment: ≥16 random chars, required for `/api/admin/*`.
    - [ ] **Action:** Add a real strong value (≥16 random chars) to your local `.env`
        (do **not** commit `.env`).
    - [ ] **Verification:** `grep ADMIN_PASSWORD .env.example` shows the placeholder key;
        confirm `.env` has a real value locally. Report both.

- [ ] **Step 1.2: Create the auth dependency `backend/app/admin_auth.py`**
    - [ ] **Action:** Implement `require_admin(x_admin_key: str = Header(default=""))`
        reading `ADMIN_PASSWORD` from env, comparing with `secrets.compare_digest`, raising
        `HTTPException(401, {"error":"unauthorized"})` when the var is unset/empty or the
        key mismatches. Mark the block `# KI-Agent unterstützt`.
    - [ ] **Verification:** `python -c "import backend.app.admin_auth"` (from repo root,
        with backend deps installed) imports without error. Run `ruff check
        backend/app/admin_auth.py` and `black --check backend/app/admin_auth.py`. Report.

## Phase 2: Service Layer (selection + mutation)

*Goal: Pure, unit-testable selection logic shared by dry-run and execute, plus the
soft-delete/restore mutations.*

- [ ] **Step 2.1: Create `backend/app/admin_service.py` view helper + mode selectors**
    - [ ] **Action:** Implement `_to_view(doc)` mapping a submission doc to
        `{submission_id, nickname, win_rate (percent, 1dp), matches_played, seed}` using
        `cells_to_grid(doc["config"]["cells"])`.
    - [ ] **Action:** Implement `select_mode_a(db) -> (kept, affected)`: fetch all
        `status != "removed"` submissions; group by `normalize_nickname(metadata.nickname)`;
        sort each group by `win_rate` desc, `avg_stable_generation` asc, `created_at` desc;
        first = kept, rest = affected; singleton groups yield only a kept entry.
    - [ ] **Action:** Implement `select_mode_b(db, nickname) -> affected`: all `status !=
        "removed"` submissions whose normalized nickname matches.
    - [ ] **Action:** Implement `select_mode_c(db, rank) -> affected`: query
        `status:"active", matches_played>0` sorted `win_rate` desc, `avg_stable_generation`
        asc; pick the 1-based `rank`-th; raise `ValueError` if out of range.
    - [ ] **Verification:** `python -c "import backend.app.admin_service"` imports clean;
        `ruff`/`black --check` pass. Report.

- [ ] **Step 2.2: Add mutation helpers**
    - [ ] **Action:** Implement `soft_delete_ids(db, ids, reason)` →
        `update_many({_id $in ids, status:"active"}, {$set:{status:"removed",
        removed_at:utcnow, removed_reason:reason}})`, returning the modified count.
    - [ ] **Action:** Implement `list_removed(db)` (status removed, sorted `removed_at`
        desc, mapped via `_to_view` + `removed_at`/`removed_reason`) and
        `restore_id(db, submission_id)` (`update_one` removed→active, `$unset removed_at,
        removed_reason`; return whether a row matched). Parse ids to `ObjectId` defensively.
    - [ ] **Verification:** imports clean; `ruff`/`black --check` pass. Report.

- [ ] **Step 2.3: Unit tests for the service layer**
    - [ ] **Action:** Create `backend/tests/test_admin_service.py` (self-running, per
        CLAUDE.md) exercising: mode A keeps the best per nickname + correct tie-break
        (rated beats unrated; newest on pure tie); mode A no-op for singleton nicknames;
        mode B normalized matching incl. unrated; mode C rank resolution + out-of-range
        `ValueError`; `soft_delete_ids` idempotency (already-removed not re-touched). Use a
        fake/in-memory db or `mongomock` consistent with existing backend tests.
    - [ ] **Verification:** `python backend/tests/test_admin_service.py` runs and all
        assertions pass. Report the output.

## Phase 3: Router & Wiring

*Goal: The `/api/admin` REST surface, gated by `require_admin`, delegating to the service.*

- [ ] **Step 3.1: Create `backend/app/admin_router.py`**
    - [ ] **Action:** `APIRouter(prefix="/api/admin", tags=["admin"],
        dependencies=[Depends(require_admin)])`. Implement `POST /delete` (dispatch on
        `mode`, validate mode-B nickname / mode-C rank → 400, run `select_*`; if not
        `dry_run`, run `soft_delete_ids` with the mode's reason tag), `GET /removed`,
        `POST /restore` (404 if id not in removed). Add request models to `models.py`
        (`AdminDeleteRequest{mode,dry_run,nickname?,rank?}`, `AdminRestoreRequest`).
    - [ ] **Action:** In `main.py`, `from .admin_router import router as admin_router` and
        `app.include_router(admin_router)` next to the duel router include.
    - [ ] **Verification:** Start the backend (`uvicorn app.main:app --reload` from
        `backend/`). Open `/docs` and confirm the three `/api/admin/*` endpoints appear.
        Report.

- [ ] **Step 3.2: Auth + endpoint smoke tests**
    - [ ] **Action:** Create `backend/tests/test_admin_endpoints.py` (or extend) covering:
        missing key → 401, wrong key → 401, correct key + `dry_run` → 200 with `affected`
        present and DB unchanged; execute path transitions rows; restore round-trips.
    - [ ] **Verification:** `python backend/tests/test_admin_endpoints.py` passes. Then
        manual curl: a dry-run with the correct `X-Admin-Key` returns counts; the same
        without the header returns 401. Report both outputs.

## Phase 4: Admin Web Page

*Goal: A phone-usable page with login gate, three preview→confirm modes, and a trash tab.*

- [ ] **Step 4.1: Page scaffold + login gate**
    - [ ] **Action:** Create `web/admin/admin.html` (standalone, mirrors
        `web/editor/editor.html`). Password input → store in
        `localStorage["biotop_admin_key"]`; reveal tabs; a Logout button clears it. All
        fetches attach `X-Admin-Key`; a `401` clears the key and returns to the gate.
    - [ ] **Verification (Interactive Test):**
        1. Serve `web/` and open `web/admin/admin.html` on a phone/desktop browser.
        2. Enter the wrong password, trigger any action → expect return to the login gate.
        3. Enter the correct password → expect the tabs to appear.
        4. **Expected Result:** Auth gate works; correct key unlocks the UI.

- [ ] **Step 4.2: Modes A and B panels (preview → confirm)**
    - [ ] **Action:** Tab A: "Preview" → table of affected rows (seed thumbnail via
        `<canvas>`, nickname, win-rate) + kept-per-nickname annotation + headline count;
        "Confirm" sends `dry_run:false`. Tab B: nickname input + same preview→confirm.
    - [ ] **Verification (Interactive Test):**
        1. With seeded test data containing duplicates, open Tab A, click Preview.
        2. Confirm the count and the kept/removed rows look correct; click Confirm.
        3. Reload `/api/leaderboard` (or the kiosk) and check the removed configs are gone.
        4. **Expected Result:** Preview matches what gets removed; leaderboard updates
           (allow ≤60 s for the worker).

- [ ] **Step 4.3: Mode C panel (rank selection)**
    - [ ] **Action:** Tab C fetches `/api/leaderboard`, renders the ranked rows; selecting
        a row previews that submission; Confirm sends `{mode:"C", rank:N, dry_run:false}`.
    - [ ] **Verification (Interactive Test):**
        1. Open Tab C, select a known rank, confirm the preview shows that exact config.
        2. Confirm deletion; verify that config leaves the leaderboard.
        3. **Expected Result:** The previewed config (not a shifted rank-N) is removed.

- [ ] **Step 4.4: Trash tab + restore**
    - [ ] **Action:** Tab Trash loads `/api/admin/removed`, renders rows with
        `removed_at`/reason + seed + a per-row "Restore" button calling
        `/api/admin/restore`.
    - [ ] **Verification (Interactive Test):**
        1. Open Trash; confirm previously removed configs appear newest-first with reason.
        2. Click Restore on one; confirm it disappears from Trash.
        3. Wait ≤60 s; confirm it reappears on the leaderboard.
        4. **Expected Result:** Restore returns the config to active and the ranking.

## Phase 5: Finalisation

*Goal: Tests green, docs updated, ADR finalised.*

- [ ] **Step 5.1: Full regression + formatting**
    - [ ] **Action:** Run all backend tests listed in CLAUDE.md plus the two new test files;
        run `black .` and `ruff check .` in `backend/`.
    - [ ] **Verification:** All tests pass, formatting/lint clean. Report the summary.

- [ ] **Step 5.2: Documentation**
    - [ ] **Action:** Set ADR-0035 status to `accepted`; add a `docs/CHANGELOG.md` entry;
        confirm `.env.example` documents `ADMIN_PASSWORD`.
    - [ ] **Verification:** `git status` shows the doc changes; review the CHANGELOG entry.
        Report.
