# LLM Log — Shoulder-Duel Mode: Design & Spec-Driven Planning

**Date:** 2026-06-13
**Branch:** `biotop`
**Participants:** Developer + Claude (KI-Agent)
**Type:** Feature ideation → full spec-driven planning (no implementation yet)
**Outcome:** Four briefing documents produced (ADR, DEV_SPEC, DEV_TECH_DESIGN, DEV_TASKS) for a new
1:1 duel feature. Implementation (Step 4 of `/run-dev`) not yet started.

---

## 1. Purpose of This Session

Brainstorm and then fully specify a new **mini-game mode**: two students at a university fair (or
later, spontaneously in the cafeteria) play a direct **1:1 duel** of their Game-of-Life start
configurations on their phones. The session moved from creative ideation into the project's
structured `/run-dev` workflow, producing one reviewed document per stage.

The guiding persona throughout was **"Bob"** — an enthusiastic gamer who tests indie games at
gamescom — used as a lens to keep the design fun, immediate, and brag-worthy.

---

## 2. Feature Concept (the agreed vision)

**Shoulder-Duel Mode** — an on-demand, server-authoritative, location-independent 1:1 duel.

The complete game loop:

> QR-connect (carries identity) → blind simultaneous config choice → both lock in → server computes
> the match **once** (referee) → synchronized local replay with VS-splash & soundtrack → WIN/LOSE
> payoff → **Rematch** — every result written to the player's personal duel record.

---

## 3. Key Decisions (and the reasoning behind them)

These were resolved interactively with the developer; the reasoning matters for anyone continuing.

| Topic | Decision | Why |
| :-- | :-- | :-- |
| **Location** | Works anywhere (booth **and** cafeteria), shoulder-to-shoulder | Driven by Bob's "let's play right now in the cafeteria" scenario; must not depend on the kiosk big screen |
| **Who computes** | **Server is the referee**, runs `biotope_headless` **once** per pairing | GoL is deterministic → the whole match is fixed the moment both seeds are known; no live frame streaming, no desync, no dispute |
| **Replay** | Phones re-run the match **locally & deterministically** via the existing **WASM core**; server verdict is canonical | Tiny seed-based payload, robust over flaky WiFi; no second rule implementation to keep in sync |
| **Pairing** | **QR-code scan** (4-char room code as fallback); the QR also carries identity | Sub-10s, frictionless "let's go" moment; co-location only needed for the scan |
| **State transport** | **Short polling**, not websockets | Few transitions, resilient to dropouts, consistent with existing GET-based stack |
| **Identity** | Reuse **ADR-0027** `player_id` + `nickname` + recovery code; **guest fallback** (`Gast-XXXX`) with post-match claim prompt | No account hurdle at "let's go"; named record needed for bragging |
| **Personal record** | New `duels` collection; **local-first** (instant, no login) **+ server backup** keyed to identity; server authoritative on conflict | Bob's explicit requirement: "I want to show who I beat" — survives device loss via recovery code |
| **Sound** | Self-host the project's **Suno track** ("Open Flow") as a static asset; **not** live-streamed; unobtrusive Suno credit link | Suno share page exposes no embeddable stream; self-hosting is reliable (cached), legal (own track), controllable; credit link is a discovery funnel |

**"My configs" gap resolved:** the editor does not store named local configs. A player's
configurations are defined as their own active `submissions` (filtered by `player_id`), served by a
new `GET /api/v1/my_configs`; plus an inline quick-draw option so guests can play immediately.

---

## 4. Scope Boundaries

**In scope (this increment):** QR pairing + identity, blind simultaneous choice, server-once
computation, WIN/LOSE + rematch, `duels` record with head-to-head + "best weapon", local-first +
server backup, self-hosted soundtrack, WASM replay (Should-Have).

**Out of scope (deferred, structure prepared):** remote/asynchronous duels between non-co-located
players; tournaments of 3+ players in duel mode; in-match interaction; kiosk big-screen as an
optional spectator showcase; emotes; sudden-death/shrinking-field tension; highlight-seed sharing.

**MVP cut:** Phases 1–5 of the task plan deliver a playable game **without** animation or sound
(connect → choose → result → rematch → personal record). Phase 6 adds the polish.

---

## 5. Documents Produced

All under `docs/`, numbered **0028** to stay in sync with the ADR.

1. **`docs/adr/ADR-0028-shoulder-duel-mode.md`** — the architectural decision (server-authoritative,
   deterministic compute-once, QR-connect, short polling, local-first record, self-hosted sound).
   Status: `proposed`. Section 4 records all rejected alternatives (live streaming, full frame
   history, JS rule re-implementation, P2P/Bluetooth, typed room codes, websockets, Suno live-stream,
   reusing the async pool).
2. **`docs/specs/DEV_SPEC-0028-shoulder-duel-mode.md`** — 20 functional (FR-1..20) + 8 non-functional
   (NFR-1..8) requirements; 3 epics (Connect / Play / Bragging) with testable acceptance criteria;
   MoSCoW prioritization; 18-item product backlog (B-01..B-18); Definition of Done.
3. **`docs/tech_design/DEV_TECH_DESIGN-0028-shoulder-duel-mode.md`** — component overview + Mermaid
   diagrams; data model (`duel_rooms` ephemeral w/ TTL index, `duels` permanent); 7 API endpoints;
   `duel_service.py` logic; frontend screen states + sequence diagram; security (hidden-choice
   server-side projection is the critical rule); performance.
4. **`docs/tasks/DEV_TASKS-0028-shoulder-duel-mode.md`** — 7 phases, small steps, each ending in a
   Verification (backend = curl/test command; frontend = developer-run interactive test).

---

## 6. Technical Design Highlights (for the implementer)

**New backend files:** `duel_router.py` (APIRouter, prefix `/api/v1/duel`), `duel_service.py`
(room lifecycle + headless invocation + record aggregation); extend `models.py`.

**New collections:**
- `duel_rooms` — ephemeral; fields include `room_id` (QR payload), `room_code` (4-char fallback),
  `status` (`waiting`→`choosing`→`computing`→`result`→`choosing`/`expired`), `red`/`blue`
  participants (each with a **hidden** `config`), `result`, `rematch` flags, `expires_at`
  (**TTL index** for auto-cleanup).
- `duels` — permanent record keyed by player identity; per-player indexes on `red.player_id` and
  `blue.player_id`.

**API endpoints:**
- `POST /api/v1/duel/rooms` — create (creator = RED)
- `POST /api/v1/duel/rooms/{id}/join` — join (BLUE); `409 room_full`, `404`, `410 expired`
- `GET /api/v1/duel/rooms/{id}` — **short-poll** state (with hidden-choice projection)
- `POST /api/v1/duel/rooms/{id}/lock` — blind lock-in; second lock triggers compute
- `POST /api/v1/duel/rooms/{id}/rematch` — both → reset to `choosing`
- `GET /api/v1/my_configs?player_id=` — player's own submissions to choose from
- `GET /api/v1/duels?player_id=` (+ optional `recovery_code`) — head-to-head, best weapon, tally

**Referee invocation (`run_match`):** mirrors the existing `worker.py` pattern — write
`red.json`/`blue.json` to a temp dir, locate the binary among
`["./build/biotope_headless", "/app/build/biotope_headless", "./biotope_headless"]`, invoke via
`asyncio.create_subprocess_exec` with an output path, parse the result JSON
(`{winner, generations, red.population, blue.population}` — see `save_headless_results` in
`src/io/file_io.c`), persist a `duels` doc, clean up temp files.

**Concurrency:** the match is triggered only by the conditional `find_one_and_update` that observes
the **second** lock-in → no double computation.

**Critical security rule:** the room-state GET response **must strip the opponent's config
server-side** while status is `choosing`/`computing`, or a player could poll the API to peek before
locking. `player_id` is advisory/untrusted; the *outcome* is server-computed and name *ownership*
rests on the ADR-0027 recovery code.

---

## 7. How to Continue (next actions)

1. **Manual prerequisite:** download the Suno track ("Open Flow",
   `https://suno.com/s/82OAUnbHAxUlSSla`) as MP3 → `web/assets/duel_theme.mp3` (Task Step 1.2).
2. **Optional:** commit the four planning documents before coding.
3. **Implementation** = Step 4 of `/run-dev`, following `DEV_TASKS-0028` phase by phase:
   - Phase 1 — scaffolding (`web/duel/duel.html`, static mount, audio asset)
   - Phase 2 — duel-room lifecycle backend
   - Phase 3 — blind choice + headless referee + `duels` write
   - Phase 4 — personal record aggregation backend
   - Phase 5 — frontend MVP flow (playable end-to-end)
   - Phase 6 — polish (WASM replay, soundtrack, guest-claim)
   - Phase 7 — tests, DoD, CHANGELOG, flip ADR-0028 to `implemented`, PR onto `biotop`
4. After each Verification step, the developer reports the outcome before proceeding (Team
   Principle; interactive UI tests run by the developer, not the agent).

---

## 8. Binding Conventions (reminder for implementation)

- English code/comments; `snake_case` / `PascalCase` / `UPPER_SNAKE_CASE`; 4-space indent, no
  trailing whitespace.
- Every AI-authored block: `// KI-Agent unterstützt` (or `# KI-Agent unterstützt` in Python).
- C must compile via `make` with **zero warnings**; Python passes `black .` + `ruff check .`.
- Backend tests are self-running `backend/tests/test_*.py` with unique ids and self-cleanup
  (`MONGODB_DB` / `BIOTOPE_API_URL` env), integration tests require built binaries.
- Workflow docs: ADR for decisions, DEV_SPEC/TECH_DESIGN/TASKS per stage, update `docs/CHANGELOG.md`
  on completion.
