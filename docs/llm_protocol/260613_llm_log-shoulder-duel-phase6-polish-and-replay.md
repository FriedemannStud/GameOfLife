# LLM Log — Shoulder-Duel Mode: Phase 6 (Polish) — Soundtrack, Guest Claim & Deterministic Replay

**Date:** 2026-06-13
**Branch:** `biotop`
**Type:** `/run-dev` Step 4 (implementation), continuing `DEV_TASKS-0028`
**Status at end of session:** 6.1 + 6.3 implemented and interactively verified by the
developer; 6.2 (deterministic match replay) implemented across C + backend + frontend
and verified at the unit/integration level. **Not yet committed.** The final HTTP
end-to-end check of the new frames endpoint was interrupted before running; the
interactive (two-phone) test of the replay animation is still pending.

---

## 1. Context

Continuation of the Shoulder-Duel feature (ADR-0028). The MVP (Phases 1–5) was already
implemented, verified and committed in a prior session (`a0c4430` feat, `4acc9b9` core
sim fix). This session picks up at **Phase 6 (Polish, Should-Have)**.

Briefings: [ADR-0028](../adr/ADR-0028-shoulder-duel-mode.md),
[DEV_SPEC-0028](../specs/DEV_SPEC-0028-shoulder-duel-mode.md),
[DEV_TECH_DESIGN-0028](../tech_design/DEV_TECH_DESIGN-0028-shoulder-duel-mode.md),
[DEV_TASKS-0028](../tasks/DEV_TASKS-0028-shoulder-duel-mode.md).
Prior session log: `260613_llm_log-shoulder-duel-frontend-and-sim-fix.md`.

Phase 6 steps: **6.1** VS splash + soundtrack + Suno credit; **6.2** deterministic
match replay animation; **6.3** guest fallback + post-match claim prompt.

---

## 2. Key Decisions

### 2.1 Replay mechanism (6.2): server frame-payload, not WASM
The Game-of-Life rules live **only in C**; ADR-0028 forbids re-implementing them in JS
(divergence risk). Two ADR-compliant options exist:
- **A) Core-only WASM module** — run the C rules in the browser. Needs `emcc`/the emsdk
  container (not available in the dev shell) plus JS glue. The existing `Makefile.wasm`
  builds the *full Raylib GUI* (`biotope.html`), **not** a callable core.
- **B) Server frame-payload** (ADR-0028 §2.2 named fallback) — `biotope_headless` emits
  the per-generation board; the server passes it to `duel.html`, which plays it on a
  canvas. One engine (the C core), fully deterministic, server verdict stays canonical.

Because `emcc` is unavailable and `gcc` is, **option B was chosen**.

### 2.2 6.2 was first deferred, then reactivated
At the start of the session the developer appeared to cancel 6.2, so 6.1 + 6.3 were done
first. The developer then clarified it was a misunderstanding: **a match simulation
should play after both opponents lock in.** 6.2 was reactivated and implemented.

### 2.3 Frames must not ride the short-poll
The room state is polled every ~1.5 s. The replay payload (~13 KB) must **not** be
returned on every poll. Mirroring the existing hidden-choice projection, frames are
stored on the room, **stripped from the poll projection**, and served **once** via a
dedicated `GET …/frames` endpoint that each phone fetches when the VS splash starts.

---

## 3. Phase 6.1 — VS Splash + Soundtrack + Suno Credit (frontend only)

All in `web/duel/duel.html` (no C/Python). Verified interactively by the developer (OK).

- New **`screen-vs`** section, shown between lock-in and the verdict.
- Self-hosted **`/assets/duel_theme.mp3`** (`<audio>` element, looped): build-up on the
  splash (`playBuildup`), payoff cue on the result (`playPayoff`), stop on Back-to-Home.
- Mobile autoplay handled by unlocking audio on the **first `pointerdown`**
  (`unlockAudio`). A **`♪ Sound: on/off`** toggle and a footer **"made with Suno"**
  credit link (`https://suno.com/s/82OAUnbHAxUlSSla`) were added.
- Poll routing: `status:"result"` now enters the VS splash first (guarded by
  `currentScreen !== 'result' && !== 'vs'`).

## 4. Phase 6.3 — Guest Fallback + Post-Match Claim Prompt (frontend only)

In `web/duel/duel.html`. Verified interactively by the developer (OK).

- A phone with **no claimed `biotope_nickname`** already plays as `Gast-XXXX` (FR-4).
- After a result, `maybeShowClaimPrompt()` shows a **non-blocking dashed banner** on the
  result screen inviting the guest to claim a name (link opens `/editor/editor.html` in a
  new tab). It never blocks **Rematch**. "Maybe later" dismisses it for the session
  (`sessionStorage['biotope_claim_dismissed']`). A claimed phone never sees the banner.
- `player_id` is stable (ADR-0027) and local duel records do not store the player's own
  name, so a later claim keeps the existing local record valid (noted in code comment).

## 5. Phase 6.2 — Deterministic Match Replay (C + backend + frontend)

### 5.1 C — `src/apps/headless/main_headless.c`
The headless playfield is **8 rows × 16 cols** (red left 8×8, blue right 8×8), so frames
are cheap. `biotope_headless` is now used **only** by the duel (`duel_service.py`); the
tournament uses `biotope_hyper_worker`, and no test invokes headless directly — so
extending headless is safe.

- Added `#include "cJSON.h"` and a `capture_frame()` helper that serialises the board to
  a compact **row-major "0/1/2" string** (128 chars: `0`=dead, `1`=red, `2`=blue).
- Frame capture is **opt-in via an extra positional arg** (`argv[4]` = frames output
  path); absent → behaviour unchanged (backward compatible).
- Captures **gen 0** (the seeds) plus gens 1–100 → **101 frames**, written to the frames
  file as `{rows, cols, frames:[str,...]}` (separate file → the stable result schema is
  untouched).
- **`make` rebuilds with zero warnings.** Smoke test confirmed: 101×128 frames, a red
  blinker oscillates, a blue 2×2 block stays stable, payload ≈ 13 KB.

### 5.2 Backend — `backend/app/duel_service.py` + `duel_router.py`
- `_invoke_headless`: passes a 5th `frames_path` arg; parses `frames.json` into
  `result["_frames"]` (best-effort; `None` on failure); cleans up the extra temp file.
- `run_match`: stores frames at the **room top level** (`$set: {..., "frames": frames}`),
  **not** inside the polled `result` embed; the permanent `duels` doc stays lean.
- `project_room`: now also `safe.pop("frames", None)` — frames never ride the poll.
- Rematch reset: also clears `"frames": None` for round N+1.
- New `get_room_frames(room_id)` service (404 `frames_not_ready` until computed) +
  new route **`GET /api/v1/duel/rooms/{room_id}/frames`**.

### 5.3 Frontend — `web/duel/duel.html`
- `screen-vs` now hosts a **`<canvas id="replay-canvas">`** (320×160, pixelated) and a
  **live red/blue population bar** instead of static seed thumbnails.
- `enterVsSplash` → `startReplay(room)`: fetches `…/frames` once; on success
  `animateReplay` draws each frame at `REPLAY_FRAME_MS` (45 ms, ~4.5 s total), updates
  the live population counts, holds the final frame ~600 ms, then `enterResult`.
- **Fallback:** if the frames fetch fails/returns nothing, a timed splash
  (`VS_SPLASH_MS` = 3500 ms) still reaches the result. The **displayed winner is always
  the server verdict** (canvas is cosmetic).
- Aborts cleanly if the user leaves the screen mid-animation (`currentScreen !== 'vs'`
  guards; `vsTimer` cleared in `enterResult`/`leaveToHome`).

---

## 6. Verification Performed

- **C:** `make` → zero warnings; headless frames smoke-tested directly (blinker
  oscillates, block stable, winner correct, ~13 KB payload).
- **Backend (in `gameoflife-backend-1`, which mounts host `./build`):**
  `tests/test_duel_room.py` and `tests/test_duel_record.py` both **pass** (the
  `run_match`-with-frames path works end-to-end in-container). No lines exceed black's
  88 cols.
- **6.1 / 6.3:** interactively verified OK by the developer (two tabs/phones).

### Not yet done
- The final **HTTP end-to-end check** of `GET …/frames` (assert poll strips `frames`,
  endpoint returns 101 frames) was **interrupted before running** — re-run it next.
- **Interactive two-phone test of the 6.2 replay animation** is pending (developer).
- **`black` / `ruff` are not installed** in this environment (host or backend
  container) — the DoD formatter/linter step must be run elsewhere. Code was written to
  black style (4-space, double quotes, ≤88 cols).

---

## 7. Files Touched (uncommitted)

**Modified:**
- `src/apps/headless/main_headless.c` — opt-in per-generation frame capture (cJSON).
- `backend/app/duel_service.py` — frames through `_invoke_headless`/`run_match`, poll
  strip, rematch clear, `get_room_frames`.
- `backend/app/duel_router.py` — `GET /duel/rooms/{room_id}/frames`.
- `web/duel/duel.html` — VS splash + soundtrack + Suno credit (6.1), guest claim prompt
  (6.3), canvas replay animation + frames fetch (6.2).

**Rebuilt (gitignored):** `build/biotope_headless` (mounted into the backend container).
*Reminder:* if the duel referee runs from `worker_bin/`, copy the rebuilt binary there
too; the duel backend itself uses `./build` (mounted as `/app/build`).

---

## 8. How to Continue

1. **Re-run the interrupted HTTP check** (inside `gameoflife-backend-1`, via `urllib` to
   `localhost:8000` — host→container curl does not route in this WSL shell): drive
   create→join→lock→lock, assert the poll response has **no** `frames` key, then `GET
   …/frames` returns `rows=8, cols=16, 101` frames.
2. **Interactive test (developer, two phones):** lock in on both → expect a VS splash
   with the **board animating** (cells evolving) and the **live population bar moving**,
   then the WIN/LOSE verdict matching the server. Same configs twice → identical
   animation on both phones.
3. **Phase 7 — DoD:** add `test_duel_integration.py` (create→join→lock→lock asserting a
   `result` + one `duels` doc + a non-empty frames payload); run the full backend test
   sweep + `black --check .` / `ruff check .`; update `docs/CHANGELOG.md` (Phase 6
   polish **and** the earlier core-sim fix); flip ADR-0028 `proposed → implemented`;
   open a PR onto `biotop`.

---

## 9. Binding Conventions (reminder)

- English code/comments; `snake_case`/`PascalCase`/`UPPER_SNAKE_CASE`; 4-space indent,
  no trailing whitespace; Python within black's 88 cols.
- Every AI-authored block carries `// KI-Agent unterstützt` (C) /
  `# KI-Agent unterstützt` (Python).
- C must compile via `make` with **zero warnings**.
- Backend tests are self-running `backend/tests/test_*.py`; integration tests need built
  binaries (`make`) and a running Mongo.
- **Team Principle:** interactive UI tests are run by the developer; the agent defines
  exactly what to do and what to expect, then waits for the report.
