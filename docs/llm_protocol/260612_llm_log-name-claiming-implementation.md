# LLM Session Log — Name-Claiming with Recovery Code: Implementation (ADR-0027)

**Date:** 2026-06-12
**Branch:** `biotop`
**Agent:** Claude (Opus 4.8)
**Mode:** `/run-dev` Step 4 (Implementation) — resumed from the prior design session, then B-13 follow-up.
**Companion doc:** `260612_claude_log-name-claiming-with-recovery-code.md` (the preceding design/spec session).

---

## 1. Objective

Implement the feature designed in the prior session: make a species **name in the web editor
ownable**. A claimed name cannot be reused by another person; the legitimate owner can return
silently on the same browser (via `localStorage` `player_id`) or reclaim from a new device using a
**server-generated recovery code**. No email/account auth (single-day university fair).

All design artifacts (ADR-0027, DEV_SPEC-0027, DEV_TECH_DESIGN-0027, DEV_TASKS-0027) already existed.
This session executed **DEV_TASKS-0027 Phases 1–6** and an additional follow-up (B-13).

---

## 2. Environment Facts (important for continuation)

- Backend runs in Docker: container **`gameoflife-backend-1`** (`/app` is a bind-mount of `backend/`),
  Mongo in **`gameoflife-mongo-1`**, worker in **`gameoflife-matchmaker-1`**.
- The container has **no `python` (only `python3` on host), no `curl`, no `httpx`/`requests`,
  no `black`/`ruff`** by default. `uvicorn` and Python stdlib (`urllib`) are available.
- Backend command uses **`uvicorn ... --reload`**: editing `.py` files triggers an automatic process
  restart (re-runs startup events). Static files (`editor.html`) are served fresh per request → no
  restart needed for frontend edits.
- **DB name comes from `MONGODB_DB`**, injected by Docker Compose `env_file: .env` **at container
  creation**. `--reload` and `docker restart` do **not** re-read `.env`. To switch DB you must
  **recreate** the container: `docker compose up -d --force-recreate backend` (and `matchmaker`).
  There is no `/app/.env` inside the container, so `load_dotenv()` is a no-op there.
- DBs seen this session: `biotope_study` (script-generated test data, 1647 submissions, 0 players)
  and `biotope_db` (the chosen fair DB; was empty). A throwaway `biotope_test` was used for
  isolated verification and then dropped.

---

## 3. What Was Implemented (by phase)

### Phase 1 — Auth utilities (`backend/app/auth_utils.py`, new)
Pure, DB-free helpers (all marked `# KI-Agent unterstützt`):
- `normalize_nickname(s)` → `" ".join(s.strip().lower().split())` (trim + lowercase + collapse
  internal whitespace) — the identity key.
- `generate_recovery_code(n_chars=4, prefix="BIOTOP")` → `secrets.choice` over Crockford Base32
  `0123456789ABCDEFGHJKMNPQRSTVWXYZ` (no `I/L/O/U`), e.g. `BIOTOP-7F3A`.
- `hash_recovery_code(code)` → SHA-256 hex of `code.strip().upper()` (forgiving input).
Unit tests: `backend/tests/test_auth_utils.py`.

### Phase 2 — Models (`backend/app/models.py`)
- New `Auth` model: `recovery_code: Optional[str] = None`.
- Refactor: extracted `SubmissionBase` (metadata + config). `Submission(SubmissionBase)` adds
  `auth: Auth = Field(default_factory=Auth)`. **`DBSubmission(SubmissionBase)`** — i.e. it does
  **not** inherit `auth`, so the recovery code can never leak into the `submissions` collection
  (FR-7), structurally rather than by a fragile `exclude=`.
- `Player` gained `nickname_normalized: str` and `recovery_code_hash: Optional[str] = None`.

### Phase 3 — Endpoint (`backend/app/main.py`)
- Startup hook now also runs
  `await get_db().players.create_index("nickname_normalized", unique=True)`.
- `submit_config` rewritten into claim/silent/reclaim decision logic:
  - **Free name → CLAIM:** generate code, insert player (`player_id`, `nickname`,
    `nickname_normalized`, `recovery_code_hash`, elo defaults, `win_rate=0.0`); `DuplicateKeyError`
    (concurrency race) → `409`. Returns `201` + `recovery_code`.
  - **Silent owner** (`player_id` matches): persist, `201`, no code.
  - **Taken by other:** read `auth.recovery_code`; missing → `409`; hash mismatch → `403`; match →
    re-bind `player_id` to the requesting device, persist, `201` + `reclaimed: true`.
- Helper `_persist_submission()` builds `DBSubmission(metadata=..., config=...)` (no `auth`).
- Exception handling fixed: `except HTTPException: raise` added **before** the catch-all so
  `409/403/400` are not swallowed into `500`.

### Phase 4 — Ops & tests
- `backend/scripts/reset_db.py` (new): destructive clean-slate — `delete_many({})` on `players` +
  `submissions`, then ensure the unique index. Bound to `MONGODB_DB`. Guarded with `__main__`.
- `backend/tests/test_name_claiming.py` (new): end-to-end integration test using stdlib `urllib`
  against a running backend (`BIOTOPE_API_URL`, default `:8001`) + `motor` for DB inspection. Uses a
  unique per-run nickname, asserts the full matrix, asserts **no plaintext code in submissions**,
  and cleans up its own docs. (TestClient/httpx were unavailable, hence the urllib approach.)

### Phase 5 — Frontend (`web/editor/editor.html`)
- Submit payload now includes a top-level `auth: { recovery_code: <reclaim input or null> }`.
- Response-matrix branching on **`res.status`**: `201`+`recovery_code` → recovery-card overlay;
  `201`+`reclaimed` → success + hide reclaim input; `201` plain → success; `409` → reveal reclaim
  input; `403` → keep input, "Code falsch"; else show `detail`.
- **Recovery-card overlay**: species name + big code + date; **"Als Bild speichern"** renders the
  card to a `<canvas>` and downloads a PNG (graceful degradation: button hidden if
  `canvas`/`toBlob` unavailable); **"Verstanden"** closes the overlay.
- Reclaim input group (`#recovery-group`), hidden until a `409`.

### Phase 6 — Finalization
- `black`: applied to the **new** files only; new-code regions in `main.py` hand-formatted to be
  black-compliant. `black .` was deliberately **not** run project-wide (pre-existing handlers like
  `get_leaderboard` were never black-formatted; a full run would bury the feature in unrelated diff).
- `ruff check` clean on all feature files.
- `docs/CHANGELOG.md` entry added; **ADR-0027 status → Implemented**; DEV_TASKS-0027 got a
  completion banner.

### Follow-up — B-13 (frontend-only)
The `409` hint previously assumed the user was the owner, confusing a first-time visitor who picks a
taken name (they have no code). Rewrote `#recovery-hint` to cover both cases ("not your species →
pick another name" vs. "submitted before on another device → enter recovery code") and shortened the
status line ("Name bereits vergeben — siehe Hinweis.") because `.status-msg` is only 20px tall.
DEV_SPEC B-13 marked done; CHANGELOG and memory note updated.

---

## 4. Error Contract (as implemented — note the `detail` nesting)

FastAPI `HTTPException(detail={"error": ...})` **nests the body under `detail`**. So the actual
wire bodies are:

| Condition | Status | Body |
|---|---|---|
| Domain rule violation | `400` | `{"detail": "<reason>"}` |
| Free name → claim | `201` | `{"status":"success","submission_id":...,"recovery_code":"BIOTOP-XXXX"}` |
| Silent owner | `201` | `{"status":"success", ...}` (no code) |
| Reclaim | `201` | `{"status":"success", ...,"reclaimed":true}` |
| Taken, no code | `409` | `{"detail":{"error":"name_taken"}}` |
| Taken, wrong code | `403` | `{"detail":{"error":"invalid_recovery_code"}}` |

**The frontend branches on `res.status`** (not on body shape), so the `detail` wrapper is harmless;
the generic-error path reads `data.detail.error` as a fallback.

---

## 5. Verification Performed

- **Phase 1/2:** unit + model checks (in-container `python`), all pass.
- **Phase 3/4 (isolated):** a second uvicorn launched in-container on **port 8001** with
  `MONGODB_DB=biotope_test`; ran the 6-case matrix and DB inspection via urllib; confirmed unique
  index, re-binding, and **no plaintext in `submissions`**. Server killed, `biotope_test` dropped.
- **Live on the fair DB:** after the developer switched `.env` to `biotope_db` and recreated the
  containers, the integration test ran against `:8000`/`biotope_db` — all green, self-cleaned.
- **Phase 5 (interactive, developer-run):** claim → recovery card + PNG → silent return →
  cross-device `409` → wrong/correct code reclaim. All confirmed OK by the developer.
- **B-13 (interactive, developer-run):** pending at time of writing (code served and confirmed
  present in the delivered HTML).
- Regression: `test_validators.py`, `test_ranking.py` still pass.

---

## 6. Database State at End of Session

- **`biotope_db`** (fair DB): **reset** via `reset_db.py` → 0 players, 0 submissions, unique index
  present. Clean generalprobe baseline.
- **`biotope_study`**: the stray unique index (accidentally created earlier via `--reload`) was
  **dropped**; only `_id_` remains. Its 1647 submissions untouched. (Caveat: if a generator inserts
  `players` directly without `nickname_normalized`, a unique index would cause a null collision —
  reason it was removed there.)
- **`biotope_test`**: dropped.

---

## 7. Files Changed / Added (not yet committed — developer commits)

**Code**
- `backend/app/auth_utils.py` *(new)*
- `backend/app/models.py`
- `backend/app/main.py`
- `backend/scripts/reset_db.py` *(new)*
- `backend/tests/test_auth_utils.py` *(new)*
- `backend/tests/test_name_claiming.py` *(new)*
- `web/editor/editor.html`

**Docs**
- `docs/CHANGELOG.md`
- `docs/adr/ADR-0027-...md` (status → Implemented)
- `docs/specs/DEV_SPEC-0027-...md` (B-13 → done)
- `docs/tasks/DEV_TASKS-0027-...md` (completion banner)

The C application was **not** changed (`network_io.c` is GET-only; never POSTs).

---

## 8. How to Continue

- **Run the tests** (in-container, against a running backend whose `MONGODB_DB` matches):
  - Unit: `docker exec gameoflife-backend-1 python /app/tests/test_auth_utils.py`
  - Integration: `docker exec -e MONGODB_DB=<db> -e BIOTOPE_API_URL=http://localhost:8000 \
    gameoflife-backend-1 python /app/tests/test_name_claiming.py`
- **Before the fair:** run `docker exec gameoflife-backend-1 python /app/scripts/reset_db.py`
  against the final fair DB (destructive; ensures clean data + unique index).
- **Switching DB** requires `docker compose up -d --force-recreate backend` (not just restart).
- **Open item:** finish the B-13 interactive check in the browser (cross-device `409` shows the new
  two-case hint, lines wrap cleanly).
- **Potential future work:** recovery-code regeneration is still out of scope (ADR-0027); the data
  model supports it but no endpoint logic exists.
- **Dev-tool note:** `black`/`ruff` are not in `requirements.txt`; they were installed ephemerally
  in the container for the lint pass. Consider adding them as dev dependencies if a CI gate is wanted
  (and decide whether to normalize the older non-black-formatted handlers project-wide).
