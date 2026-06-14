# LLM Log — Shoulder-Duel Mode: Replay Polish (Music-Synced) & Phase 7 DoD

**Date:** 2026-06-14
**Branch:** `biotop`
**Type:** `/run-dev` Step 4 (implementation), continuing `DEV_TASKS-0028`
**Status at end of session:** Phases 1–6 complete and interactively verified; Phase 7
(DoD) done except the formatter/linter run, the final two-phone acceptance loop (7.3),
and the commit/PR. **Nothing committed this session** (developer handles the commit).

---

## 1. Context

Continuation of the Shoulder-Duel feature (ADR-0028). Entering this session the MVP
(Phases 1–5) and the Phase-6 polish (6.1 soundtrack, 6.2 deterministic replay, 6.3 guest
claim) were already implemented; 6.1/6.3 were verified, 6.2 still needed the HTTP
end-to-end check and the interactive two-phone test.

Briefings: [ADR-0028](../adr/ADR-0028-shoulder-duel-mode.md),
[DEV_SPEC-0028](../specs/DEV_SPEC-0028-shoulder-duel-mode.md),
[DEV_TECH_DESIGN-0028](../tech_design/DEV_TECH_DESIGN-0028-shoulder-duel-mode.md),
[DEV_TASKS-0028](../tasks/DEV_TASKS-0028-shoulder-duel-mode.md).
Prior logs: `260613_llm_log-shoulder-duel-frontend-and-sim-fix.md`,
`260613_llm_log-shoulder-duel-phase6-polish-and-replay.md`.

---

## 2. Replay (6.2) — HTTP End-to-End Verification

The interrupted HTTP check of the new frames endpoint was re-run inside the backend
container (host→container `curl` does not route in this WSL shell; used `urllib` against
`localhost:8000`):

- Drove `create → join → lock(red) → lock(blue) → result`.
- **Asserted:** first lock keeps `choosing` and hides the opponent config; the poll
  response has **no `frames` key** (frames must not ride the ~1.5 s poll); the dedicated
  `GET /api/v1/duel/rooms/{id}/frames` returns `rows=8, cols=16, 101` frames of 128 chars.
- All assertions passed. Test room + `duels` doc cleaned up afterwards.

---

## 3. Frontend Polish Round (all in `web/duel/duel.html` unless noted)

The developer ran each change as an interactive test and reported back before the next.

### 3.1 Bug fix — claimed name never reached the duel page (`web/editor/editor.html`)
- **Symptom:** after claiming a name in the editor, the duel page still showed
  `Gast-XXXX` (player tag and the name above the simulations), even though the claimed
  config was offered in the chooser.
- **Root cause:** the duel page decides guest-vs-claimed **solely** from
  `localStorage['biotope_nickname']` (`duel.html` identity block), but the editor never
  wrote that key — it only registered the name server-side (in `players`/`submissions`,
  keyed by `player_id`). The chooser still worked because configs come from the server
  (`my_configs` by `player_id`), independent of the local name key.
- **Fix:** in the editor's `201` success branch, persist
  `localStorage.setItem('biotope_nickname', speciesName)`. The same-browser duel page
  then reuses the claimed name after a reload (and sends it on the next create/join).
- **Note:** `nickname` in `duel.html` is computed once at page load, so an already-open
  duel tab must be **reloaded** for the new name to take effect; the name above the
  simulations updates on the **next** match (it is sent at create/join time).

### 3.2 Replay tempo + music synchronisation
- The 45 ms/frame default felt too fast; first bumped to 75 ms, then the developer asked
  to **sync the frame rate to the music beats**.
- **Tempo measured locally:** decoded `web/assets/duel_theme.mp3` with `ffmpeg` to mono
  PCM, computed an onset envelope (energy flux) and autocorrelation → **~120 BPM**
  (`librosa`/`aubio` not installed; `ffmpeg`+`numpy` were).
- **Implementation:**
  - `TRACK_BPM = 120` (hard-wired); `BEAT_MS = 60000 / TRACK_BPM`;
    `REPLAY_FRAME_MS = BEAT_MS / 4` → **one generation per 1/16 note = 125 ms** (release
    value; ~12.6 s for 101 frames).
  - `animateReplay` was rewritten to **drive the frame index off the audio clock**
    (`duelAudio.currentTime`) instead of a fixed `setTimeout` counter, so the board stays
    on the beat and never drifts over the replay. A **wall-clock fallback**
    (`performance.now()`) keeps the replay running at the same tempo when the sound is
    muted or autoplay is blocked. The loop polls every ~16 ms and redraws only on a
    frame change.
- **Caveat (documented for the developer):** because `TRACK_BPM` is hard-wired, swapping
  the MP3 for a track with a different tempo de-syncs the animation; it also assumes the
  downbeat is at `t=0` and a constant tempo. Swapping the track means re-measuring BPM and
  updating `TRACK_BPM`. An optional sidecar config (`{bpm, offset_ms}`) was discussed and
  deferred. Decision: keep the hard-wired constant (single fixed track).

### 3.3 Proportional population meter
- Replaced the plain "Red: N  Blue: N" text on the VS splash with a real **proportional
  bar** (`.pop-meter` with red/blue segments, 90 ms width transition) above the numeric
  readout. `drawReplayFrame` sets segment widths from each frame's live-cell share
  (`rp/(rp+bp)`); empty board → both at 0. Reset on entering the VS splash.

### 3.4 Generation counter + timeline
- Added a **"Gen N / 100"** readout plus a thin **timeline progress bar** (`.gen-track` /
  `.gen-fill`), both fed from the **same clock-driven `target` index** that drives the
  board (frame *i* == generation *i*; frame 0 = the seeds). `tabular-nums` prevents digit
  jitter; the counter holds at "100 / 100" during the 600 ms final-frame hold.
- During testing the developer requested one tempo step slower; the release value remains
  1/16 (125 ms), with 1/8 (250 ms) and 1/4 (500 ms) noted as easy slower options.

---

## 4. Phase 7 — Tests, DoD & Documentation

### 4.1 Backend test sweep (7.1)
Run inside `gameoflife-backend-1` (mounts host `./build`):
- **Pass:** `test_validators`, `test_auth_utils`, `test_ranking`, `test_duel_room`,
  `test_duel_record`, `test_system_integration`, and `test_oscillator_detection`
  (the last needs `PYTHONPATH=/app`).
- **Not a regression:** `test_name_claiming` targets a separate `:8001` test server
  (default `BIOTOPE_API_URL`) → connection refused without that server.
- ⚠️ **`black` / `ruff` are not installed** (host or container) — the formatter/linter DoD
  step is **outstanding** and must be run elsewhere. New code written black-style.

### 4.2 Integration test (7.2)
- Added **`backend/tests/test_duel_integration.py`** (self-running, self-cleaning, style
  of `test_duel_room.py`): full `create→join→lock→lock` through the **real C binary**,
  asserting `status=result`, a lean poll (no `frames` key), exactly **one** `duels` doc,
  and a non-empty, well-formed **8×16 / 101-frame** replay payload (board actually
  evolves). Passes in-container.

### 4.3 Documentation & build (7.4)
- `docs/CHANGELOG.md`: new dated entry (2026-06-14) covering the whole ADR-0028 feature
  (backend lifecycle, referee, replay frame-payload, frontend flow, soundtrack, the
  claim-name fix) **and** the earlier core-sim chunk-skip fix.
- `docs/adr/ADR-0028-shoulder-duel-mode.md`: status flipped **`proposed → implemented`**.
- `docs/tasks/DEV_TASKS-0028-shoulder-duel-mode.md`: checkboxes set to the real state
  (Phases 1–6 `[x]`; 7.1/7.2 `[x]` with the black/ruff caveat; 7.3 and the 7.4 commit/PR
  left `[ ]`), plus a status block.
- **`make clean && make` → zero warnings**, all three binaries produced. (No C source was
  changed this session; the rebuild only confirms the prior `main_headless.c` frame-capture
  change compiles cleanly.)

---

## 5. Files Touched (uncommitted at session end)

**Modified this session:**
- `web/duel/duel.html` — audio-clock-driven replay, BPM constants, population meter,
  generation counter + timeline.
- `web/editor/editor.html` — persist `biotope_nickname` on successful claim.
- `docs/CHANGELOG.md` — ADR-0028 entry.
- `docs/adr/ADR-0028-shoulder-duel-mode.md` — status → implemented.
- `docs/tasks/DEV_TASKS-0028-shoulder-duel-mode.md` — checkbox status.

**New this session:**
- `backend/tests/test_duel_integration.py`.

**Also modified earlier (prior sessions, still uncommitted):**
- `backend/app/duel_service.py`, `backend/app/duel_router.py`,
  `src/apps/headless/main_headless.c` (frame capture).

**Rebuilt (gitignored):** `build/biotope`, `build/biotope_headless`,
`build/biotope_hyper_worker`. The duel referee uses the backend's `./build` mount, so no
copy to `worker_bin/` is needed (that only serves the tournament `hyper_worker`).

---

## 6. How to Continue

1. **Developer:** run `black --check .` / `ruff check .` in `backend/` once available, and
   the **full two-phone acceptance loop (7.3)**: scan → choose → watch (music-synced
   replay, population meter, Gen counter) → WIN/LOSE → rematch → My Duels.
2. **Commit & PR onto `biotop`** (developer): the modified + new files above; include the
   core-sim fix note already in the CHANGELOG.
3. Optional backlog: move `TRACK_BPM`/offset into a sidecar config beside the MP3 so the
   track can be swapped without a code change.

---

## 7. Binding Conventions (reminder)

- English code/comments; `snake_case` / `PascalCase` / `UPPER_SNAKE_CASE`; 4-space indent,
  no trailing whitespace; Python within black's 88 cols.
- Every AI-authored block carries `// KI-Agent unterstützt` (C/JS) /
  `# KI-Agent unterstützt` (Python).
- C must compile via `make` with **zero warnings**.
- **Team Principle:** interactive UI tests are run by the developer; the agent defines
  exactly what to do and what to expect, then waits for the report.
