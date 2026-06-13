# LLM Log — Shoulder-Duel Mode: Backend Implementation (Phases 1–4)

**Date:** 2026-06-13
**Branch:** `biotop`
**Type:** `/run-dev` Step 4 (implementation), following `DEV_TASKS-0028`
**Outcome:** The entire backend of Shoulder-Duel Mode (Phases 1–4 of the task plan)
is implemented and verified. The frontend (Phase 5) has **not** been started.

---

## 1. Context

This session continues the Shoulder-Duel feature planned in the prior session
(see `260613_llm_log-shoulder-duel-mode-design.md`). All four briefing documents
already existed, so the work entered `/run-dev` **Step 4: Implementation**,
proceeding phase by phase through
[`DEV_TASKS-0028`](../tasks/DEV_TASKS-0028-shoulder-duel-mode.md).

Briefing documents (read before coding):
- [ADR-0028](../adr/ADR-0028-shoulder-duel-mode.md) — server-authoritative,
  deterministic compute-once, QR-connect, short polling, local-first record,
  self-hosted sound.
- [DEV_SPEC-0028](../specs/DEV_SPEC-0028-shoulder-duel-mode.md) — FR/NFR, epics,
  backlog.
- [DEV_TECH_DESIGN-0028](../tech_design/DEV_TECH_DESIGN-0028-shoulder-duel-mode.md)
  — data model, endpoints, service logic, the critical hidden-choice rule.

**Working rhythm (Team Principle):** backend verifications (tests / HTTP calls)
were executed by the agent; interactive UI tests are reserved for the developer.

---

## 2. Environment Notes (important for continuation)

- The stack runs under Docker Compose. Relevant running containers:
  `gameoflife-backend-1`, `gameoflife-matchmaker-1`, `gameoflife-mongo-1`,
  `gameoflife-c-dev-1`.
- The backend (`python:3.11-slim`) **volume-mounts** `./backend → /app`,
  `./build → /app/build`, `./web → /app/web`, and runs uvicorn with `--reload`,
  so source edits hot-reload.
- `black` / `ruff` are **not installed** locally or in the container; the agent
  kept all new code within black's 88-char limit by hand. Pre-existing lines in
  `main.py` (leaderboard section) already exceed 88 and were left untouched.
- Backend tests were run inside the container, e.g.
  `docker exec gameoflife-backend-1 python tests/test_duel_room.py`.
  They use the live `MONGODB_DB` from `.env` but use unique ids and self-clean.
- Host→container `curl localhost:8000` did not route in this WSL shell; HTTP
  verifications were driven via `urllib` **inside** the container instead.

### ⚠️ Runtime-library gotcha (action required)
The duel referee invokes `biotope_headless` **inside the backend process**. That
binary links `libgomp.so.1` and `libcurl.so.4`, absent from `python:3.11-slim`
→ `error while loading shared libraries: libcurl.so.4`.

Fix applied in `docker-compose.yml`: the `backend` service `command` now runs
`apt-get install -y libgomp1 libcurl4` before uvicorn (mirroring `matchmaker`).
The libs were also hot-installed into the running container so verifications
could pass. **To persist across a recreate, run `docker compose up -d backend`.**

---

## 3. The Headless Referee Contract (reverse-engineered)

Confirmed empirically by running `build/biotope_headless` on the golden fixtures
(`tests/tc6_e2e_headless_red.json` / `_blue.json`) and on synthetic 0–7 configs:

- **Input** (`red.json`, `blue.json`): the editor/submission shape, **no
  bounding box needed** —
  `{"metadata": {"player_id", "nickname"}, "config": {"cells": [[x,y], ...]}}`.
- **Team assignment is by file**: cells in `red.json` become RED, cells in
  `blue.json` become BLUE (the positional fallback does not override this).
- **Output JSON** (`save_headless_results` in `src/io/file_io.c`):
  `{winner: "red"|"blue"|"draw", generations: 100, timestamp, red:{player_id,
  nickname, population}, blue:{...}}`.
- Invocation: `biotope_headless <red.json> <blue.json> <out.json>`.

---

## 4. What Was Implemented

### Phase 1 — Scaffolding & Static Assets
- **`web/duel/duel.html`** — mobile-first dark "SHOULDER DUEL" home screen with
  "Start Duel" / "Scan to Join", a minimal screen-router stub, and the
  `biotope_player_id` localStorage identity copied from `editor.html:313-321`.
- **`backend/app/main.py`** — added static mounts `app.mount("/duel", ...)` and
  `app.mount("/assets", ...)`.
- **`web/assets/duel_theme.mp3`** — the project's Suno track "Open Flow"
  (copied from the repo-root `OpenFlow.mp3`); source URL recorded in
  `web/assets/README.md`.
- *Verified interactively by the developer (page renders, localStorage id set,
  mp3 plays).*

### Phase 2 — Data Model & Duel-Room Lifecycle
- **`models.py`** — `DuelParticipant`, `DuelResultEmbed`, `DuelRoom`, and request
  schemas `CreateRoomRequest`, `JoinRoomRequest`, `LockRequest`, `RematchRequest`.
- **`duel_service.py`** — `create_room` (creator = RED, `status=waiting`,
  `expires_at = now + 30 min`), `join_room` (→ `choosing`; idempotent re-join;
  rejects full/expired/missing room via a `DuelError(error, status)` carrying the
  404/409/410 contract). Helpers: `generate_room_id` (token_urlsafe),
  `generate_room_code` (4-char Crockford), `_resolve_is_guest`, `_strip_id`.
- **`duel_router.py`** — `APIRouter(prefix="/api/v1")`; room routes under
  `/duel/rooms`. `POST /duel/rooms`, `POST /duel/rooms/{id}/join`,
  `GET /duel/rooms/{id}`. Included in `main.py` via `include_router`.
- **`main.py` startup** — `_ensure_duel_indexes`: TTL index on
  `duel_rooms.expires_at` (`expireAfterSeconds=0`), unique index on
  `duel_rooms.room_id`, and per-player indexes `duels.red.player_id` /
  `duels.blue.player_id`.
- *Verified:* `tests/test_duel_room.py` (create/join/idempotent/full/not-found);
  index introspection; in-container HTTP flow showing the hidden-choice
  projection (`GET` exposes only `nickname/is_guest/locked/player_id`).

### Phase 3 — Blind Choice & The Referee
- **`GET /api/v1/my_configs?player_id=`** → `get_my_configs`: the player's active
  `submissions` as `{config_id, nickname, seed}` (seed via `cells_to_grid`).
- **`POST /duel/rooms/{id}/lock`** → `lock_choice`: resolves `config_id` → seed
  from `submissions`, or validates an inline `config` with
  `validate_biotope_rules` (guest quick-draw). Stores the **hidden** config behind
  a conditional `find_one_and_update` guarding `{slot}.locked`; the
  `choosing → computing` transition is itself a guarded update so only the
  **second** lock triggers exactly one `run_match` (no double computation).
- **`run_match`** → `_invoke_headless` (temp dir, binary lookup among
  `["./build/biotope_headless", "/app/build/biotope_headless",
  "./biotope_headless"]`, `asyncio.create_subprocess_exec`, parse, always clean
  up temp files), then stores the `DuelResultEmbed` (winner, generations, both
  populations, **both 64-int seeds**) on the room (`status=result`) and inserts a
  **`duels`** document keyed by player identity.
- **`project_room`** — the critical hidden-choice projection: strips
  `config`/`config_id` from both slots server-side; canonical seeds appear only in
  `result`.
- *Verified:* extended `test_duel_room.py` (single lock stays hidden, double lock
  → `result`, one `duels` doc); in-container HTTP end-to-end run.

### Phase 4 — Personal Duel Record
- **`GET /api/v1/duels?player_id=` (+ optional `recovery_code`)** →
  `get_my_record`: aggregates `duels` into `head_to_head` (per opponent nickname:
  wins/losses/draws), `best_weapon` (most-winning own `config_id`, with seed), and
  an overall `tally {w,l,d}`. A supplied `recovery_code` is verified against
  `players.recovery_code_hash` via `hash_recovery_code` (→ `403
  invalid_recovery_code` on mismatch).
- *Verified:* `tests/test_duel_record.py` (tally, head-to-head, best-weapon
  selection, wrong/right recovery code), all self-cleaning.

### Regression check
`test_validators`, `test_auth_utils`, `test_ranking`,
`test_oscillator_detection` (the last needs `PYTHONPATH=/app` due to a
pre-existing hardcoded path) all pass.

---

## 5. Files Touched

**New:**
- `web/duel/duel.html`
- `web/assets/duel_theme.mp3`, `web/assets/README.md`
- `backend/app/duel_service.py`
- `backend/app/duel_router.py`
- `backend/tests/test_duel_room.py`
- `backend/tests/test_duel_record.py`

**Modified:**
- `backend/app/models.py` (duel models + request schemas)
- `backend/app/main.py` (static mounts, router include, duel indexes)
- `docker-compose.yml` (backend installs `libgomp1 libcurl4`)

---

## 6. API Surface (as built)

| Method | Path | Purpose |
| :-- | :-- | :-- |
| POST | `/api/v1/duel/rooms` | Create room (creator = RED) → `{room_id, room_code, slot}` |
| POST | `/api/v1/duel/rooms/{id}/join` | Join (BLUE); 404/409/410 |
| GET | `/api/v1/duel/rooms/{id}` | Short-poll state (hidden-choice projection) |
| POST | `/api/v1/duel/rooms/{id}/lock` | Hidden lock-in; 2nd lock computes once |
| GET | `/api/v1/my_configs?player_id=` | Player's own configs to choose from |
| GET | `/api/v1/duels?player_id=` | Personal record (head-to-head/best-weapon/tally) |

Not yet built: `POST /duel/rooms/{id}/rematch` (planned in Step 5.3 backend).

---

## 7. How to Continue

1. **Persist the infra fix:** `docker compose up -d backend` (bakes in
   `libgomp1`/`libcurl4`).
2. **Phase 5 — Frontend MVP** (interactive, developer-run on two phones/tabs):
   - 5.1 Pairing UI: `POST /rooms` + render **QR** of `room_id` + 4-char code;
     "Scan to Join" (camera QR + manual fallback) → `join`; start ~1.5 s poll.
     *Open decision:* QR library approach — vendor minimal JS locally
     (recommended, network-independent), CDN tags, or ship room-code-only first.
   - 5.2 Choice UI: `my_configs` seed icons + quick-draw grid → `lock`; show
     "waiting for opponent" with the pick hidden.
   - 5.3 Result + Rematch: WIN/LOSE from server `winner` vs this phone's slot;
     **add backend `POST /duel/rooms/{id}/rematch`** (`request_rematch`: both flags
     → reset `locked`/`config`/`result` → `choosing`).
   - 5.4 "My Duels" local-first (`localStorage['biotope_duels']`), reconciled with
     `GET /api/v1/duels`.
3. **Phase 6 — Polish:** VS splash + soundtrack (`duel_theme.mp3`, gated behind
   first tap) + Suno credit; deterministic **WASM replay** (server verdict stays
   canonical); guest fallback + post-match claim prompt.
4. **Phase 7 — DoD:** test sweep + `black --check`/`ruff`; end-to-end
   `test_duel_integration.py`; update `docs/CHANGELOG.md`; flip ADR-0028 →
   `implemented`; PR onto `biotop`.

---

## 8. Binding Conventions (reminder)

- English code/comments; `snake_case`/`PascalCase`/`UPPER_SNAKE_CASE`; 4-space
  indent, no trailing whitespace; black (88) + ruff clean.
- Every AI-authored block carries `# KI-Agent unterstützt` (Python) /
  `// KI-Agent unterstützt` (C).
- Backend tests are self-running `backend/tests/test_*.py`, unique ids,
  self-cleanup; integration tests need built binaries (`make`).
