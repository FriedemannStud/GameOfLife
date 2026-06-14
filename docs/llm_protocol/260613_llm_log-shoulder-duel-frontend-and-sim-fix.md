# LLM Log — Shoulder-Duel Mode: Frontend MVP (Phase 5) & Core Simulation Fix

**Date:** 2026-06-13
**Branch:** `biotop`
**Type:** `/run-dev` Step 4 (implementation), following `DEV_TASKS-0028`
**Outcome:** Phase 5 (frontend MVP) of Shoulder-Duel Mode is fully implemented and
interactively verified. A pre-existing **core simulation bug** (frozen headless
match) was discovered during verification and fixed. Two commits landed on `biotop`.

---

## 1. Context

This session continues the Shoulder-Duel feature. Prior sessions produced the four
briefing documents and the backend (Phases 1–4). See:
- `260613_llm_log-shoulder-duel-mode-design.md` — design & decisions.
- `260613_llm_log-shoulder-duel-mode-backend-implementation.md` — backend Phases 1–4.
- Briefings: [ADR-0028](../adr/ADR-0028-shoulder-duel-mode.md),
  [DEV_SPEC-0028](../specs/DEV_SPEC-0028-shoulder-duel-mode.md),
  [DEV_TECH_DESIGN-0028](../tech_design/DEV_TECH_DESIGN-0028-shoulder-duel-mode.md),
  [DEV_TASKS-0028](../tasks/DEV_TASKS-0028-shoulder-duel-mode.md).

Working rhythm (Team Principle): backend checks were run by the agent; the
interactive UI tests (two browser tabs / two phones) were run by the developer,
who reported the outcome before each next step.

---

## 2. Key Decision: QR Pairing Approach

The "open decision" flagged in the prior log was resolved with the developer:

> **QR encodes a deep-link URL** `…/duel/duel.html?join=ROOM_ID`. Phone B scans it
> with its **native camera app**, which opens the link; `duel.html` reads `?join=`
> and auto-joins. A typed **4-character room code** is the manual fallback.

Rationale: no in-app camera-scanner library is needed — only a small QR *generator*.
Robust and network-independent. The generator is vendored at `web/vendor/qrcode.js`
(qrcode-generator by Kazuhiko Arase, MIT), served via a new `/vendor` static mount.

---

## 3. Backend Additions (Phase 5)

All verified in-container (`docker exec gameoflife-backend-1 …`); host→container
`curl` does not route in this WSL shell, so HTTP checks use `urllib` inside the
container, and service-level tests run directly.

- **`POST /api/v1/duel/rooms/{room_id}/rematch`** (`request_rematch` in
  `duel_service.py`): per-slot rematch flag; the request that observes **both**
  flags performs a guarded `result → choosing` reset (clears
  `locked`/`config`/`config_id`/`result`, resets rematch flags) so the same two
  players replay in the same room. Idempotent; the reset is guarded so it fires once.
- **`GET /api/v1/duel/rooms/by-code/{room_code}`** (`resolve_room_code`): maps a
  typed 4-char code to its `room_id` (newest non-expired room) so the manual-entry
  fallback can join. The QR path already carries the full `room_id`.
- **`/vendor` static mount** in `main.py` for the vendored QR library.
- **Test:** `tests/test_duel_room.py` extended with Step 9 (rematch: single request
  stays in `result`; both requests reset to `choosing`; a second match writes a
  second `duels` doc). Green.

---

## 4. Frontend MVP (`web/duel/duel.html`, rewritten)

A single mobile-first page, poll-driven (~1.5 s), whose screens mirror the server
room status: **home → lobby/join → choice → result → my-duels**.

- **5.1 Pairing:** "Start Duel" → `POST /rooms` → render a QR of the deep-link join
  URL + the 4-char code; "Scan to Join" → manual code entry → `by-code` →
  `POST …/join`; boot-time `?join=ROOM_ID` auto-join. Poll loop started on entry.
- **5.2 Choice:** `GET /my_configs` rendered as selectable seed icons, plus an 8×8
  quick-draw grid (reuses the editor cell model, max 24). "Lock in" sends
  `{config_id}` or inline `{config:{cells}}`. After locking, controls hide and a
  "waiting for opponent" state shows; the opponent's pick stays hidden until both
  lock (server-side projection).
- **5.3 Result + Rematch:** `WIN/LOSE/DRAW` derived from the server `winner` vs this
  phone's slot; both seeds + both populations shown; "Rematch" → `POST …/rematch`.
- **5.4 My Duels:** every result is appended to `localStorage['biotope_duels']`
  (dedup key = `room_id|computed_at`); the view renders head-to-head + W/L/D tally
  **local-first**, then reconciles with `GET /api/v1/duels` (server authoritative).

**Identity:** reuses `biotope_player_id` (localStorage, ADR-0027); nickname is a
claimed `biotope_nickname` if present, else a `Gast-XXXX` guest tag.

### Layout standardisation (from developer feedback)
The result/choice screens initially mixed framings (seeds egocentric "me-left",
populations absolute "Red-left"), which was inconsistent on the BLUE phone. They
were standardised to **absolute framing: RED always left, BLUE always right** on
both phones; `(you)` marks the viewer's side. `YOU WIN/LOSE` stays egocentric.

---

## 5. Core Simulation Bug (found during verification, now fixed)

### Symptom (developer, evidence-based)
Across several duels, the final populations always equalled the **initial** number
of live cells the players drew — i.e. the "winner" was just "who drew more cells".

### Investigation
Running `build/biotope_headless` directly on discriminating Game-of-Life patterns:
- A single isolated cell stayed at population 1 (must die — no neighbours).
- An L-tromino stayed 3 (must grow to a 2×2 block = 4).
- 6 scattered isolated cells stayed 6 (must all die to 0).

→ The board was **frozen**: the population never changed from the seed.

### Root cause (two latent bugs in the chunk-skip optimization)
`update_generation` (`src/core/game_logic.c`) skips "dead" chunks via `chunk_map`
for speed. Two defects made it unsound on the headless path:

1. **`initialize_world_from_file` (`src/io/file_io.c`)** wrote seed cells into the
   grid but **never called `activate_chunk_at`**, so all chunks were inactive and
   every chunk was skipped. With double buffering this ping-ponged the grid between
   "seeds" and "empty"; after 100 (even) generations the original seeds remained.
   The tournament path (`run_isolated_match`) *does* activate chunks and was
   unaffected — only the headless single-match referee was broken.
2. **`update_generation`** left skipped chunks **untouched** in `next_gen` instead
   of writing them `DEAD`, so stale cells from two generations earlier
   "resurrected" once a region emptied (a lone cell survived only when the opposite
   half was empty).

### Fix
- `file_io.c`: after placing a seed cell, call
  `activate_chunk_at(world, y, x + offset_x)` (mirrors `run_isolated_match`).
- `game_logic.c`: in the chunk-skip branch, write the chunk's region as `DEAD` in
  `next_gen` before `continue` (localised; safe under the OpenMP parallel-for since
  chunks write disjoint regions).

### Verification (after fix)
Lone cell → 0, L-tromino → 4, blinker → oscillates at 3, block → stable 4,
scattered → 0. **Warning-free `make` build.** No regression: `test_system_integration`
(tournament path), `test_oscillator_detection`, `test_duel_room`, `test_duel_record`
all pass. Binaries rebuilt into `build/` and `worker_bin/`.

---

## 6. Files Touched

**Modified:**
- `src/io/file_io.c`, `src/core/game_logic.c` — the core simulation fix.
- `backend/app/duel_router.py` — rematch + by-code endpoints.
- `backend/app/duel_service.py` — `request_rematch`, `resolve_room_code`.
- `backend/app/main.py` — `/vendor` static mount.
- `backend/tests/test_duel_room.py` — rematch test (Step 9).

**New:**
- `web/duel/duel.html` — full Phase-5 frontend (rewritten from the skeleton).
- `web/vendor/qrcode.js` — vendored QR generator (MIT).

---

## 7. Commits (on `biotop`)

- `4acc9b9` **fix:** headless simulation froze due to chunk-map skip optimization
  (the two `.c` files; kept separate as it also affects GUI/tournament).
- `a0c4430` **feat:** implement Shoulder-Duel Mode (1:1 duel, ADR-0028) — backend
  Phases 1–5, frontend, assets, tests, `docker-compose` runtime libs.

Not committed: repo-root `OpenFlow.mp3` (1.6 MB duplicate of
`web/assets/duel_theme.mp3` — left untracked; delete or gitignore). `build/` and
`worker_bin/` are gitignored. No push was performed.

---

## 8. How to Continue

**Phase 6 — Polish (Should-Have):**
1. **VS splash + soundtrack** (`web/assets/duel_theme.mp3`, gated behind first tap)
   + Suno credit link.
2. **Deterministic WASM replay** — the animation of the match between "Lock in" and
   the WIN/LOSE screen (developer's explicit request "A"). The server verdict stays
   canonical. Now meaningful since the simulation actually evolves.
3. **Guest fallback & post-match claim prompt.**
4. **Open clarification "B":** finalise the team-colour assignment and the
   left/right layout of the two sides on the VS/replay/result screens (interim:
   absolute RED-left / BLUE-right).

**Phase 7 — DoD:** add `test_duel_integration.py`; full test sweep + `black`/`ruff`;
update `docs/CHANGELOG.md` (include the **core simulation fix** note); flip ADR-0028
`proposed → implemented`; open a PR onto `biotop`.

**Infra reminder:** the duel referee links `libgomp1`/`libcurl4`; the `backend`
service installs them on start (mirrors `matchmaker`). `docker compose up -d backend`
persists this across a recreate.

---

## 9. Binding Conventions (reminder)

- English code/comments; `snake_case` / `PascalCase` / `UPPER_SNAKE_CASE`; 4-space
  indent, no trailing whitespace; Python within black's 88 cols.
- Every AI-authored block carries `# KI-Agent unterstützt` (Python) /
  `// KI-Agent unterstützt` (C).
- C must compile via `make` with **zero warnings**.
- Backend tests are self-running `backend/tests/test_*.py`, unique ids, self-clean;
  integration tests need built binaries (`make`).
