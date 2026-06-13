# DEV_TASKS-0028: Shoulder-Duel Mode

Step-by-step implementation plan for the on-demand, server-authoritative 1:1 duel between two
co-located phones.

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps
to allow for interruptions and ensure stability. After each "Verification" step, report the outcome.
This iterative process is crucial for maintaining quality. Per the Team Principle, **interactive
tests are run by you on the phone/browser**; I define exactly what to do and what to expect.

**Binding rules for every step:** English code/comments; 4-space indent, no trailing whitespace;
Python passes `black .` + `ruff check .`; any C change compiles via `make` with **zero warnings**;
all AI-authored blocks carry `// KI-Agent unterstützt` (or `# KI-Agent unterstützt` in Python).

**Briefing Documents:**
*   [ADR-0028: Shoulder-Duel Mode](../adr/ADR-0028-shoulder-duel-mode.md)
*   [DEV_SPEC-0028: Requirements](../specs/DEV_SPEC-0028-shoulder-duel-mode.md)
*   [DEV_TECH_DESIGN-0028: Technical Design](../tech_design/DEV_TECH_DESIGN-0028-shoulder-duel-mode.md)

> **Build order rationale:** Phases 1–5 deliver the **MVP** (Must-Have): pair → blind choice →
> referee computes once → WIN/LOSE → rematch → personal record. Phase 6 adds the **Should-Have**
> polish (WASM replay, soundtrack, guest-claim). Each phase is a safe stopping point.

---

## Phase 1: Scaffolding & Static Assets

*Goal: Stand up the duel page shell and the self-hosted soundtrack so later phases have a home.*

- [ ] **Step 1.1: Create the duel page skeleton**
    - [ ] **Action:** Create `web/duel/duel.html` as a minimal mobile-first page with a single
        "Start Duel" / "Scan to Join" home screen and the `biotope_player_id` localStorage handling
        copied from `editor.html:320-321`. No backend calls yet.
    - [ ] **Action:** In `backend/app/main.py`, mount it next to the editor:
        `app.mount("/duel", StaticFiles(directory="web/duel"), name="duel")`. Mark with
        `# KI-Agent unterstützt`.
    - [ ] **Verification:** Run `uvicorn app.main:app --reload --port 8000` from `backend/`, open
        `http://localhost:8000/duel/duel.html` on a phone-sized viewport.
        **Expected Result:** The home screen renders; DevTools shows a `biotope_player_id` value in
        localStorage.

- [ ] **Step 1.2: Self-host the Suno soundtrack**
    - [ ] **Action:** Download the project's Suno track ("Open Flow") as MP3 and place it at
        `web/assets/duel_theme.mp3`. Add a short README note in `web/assets/` recording the source
        URL (`https://suno.com/s/82OAUnbHAxUlSSla`).
    - [ ] **Verification:** Open `http://localhost:8000/assets/duel_theme.mp3` (add an `/assets`
        static mount if not already served).
        **Expected Result:** The browser plays the track. (Mounting the file is enough here; in-game
        playback comes in Phase 6.)

---

## Phase 2: Data Model & Duel-Room Lifecycle (Backend)

*Goal: Two phones can create and join a room and observe each other's presence. Backlog B-01, B-03.*

- [ ] **Step 2.1: Add duel models**
    - [ ] **Action:** In `backend/app/models.py`, add `DuelParticipant`, `DuelRoom`, and the
        request/response schemas (`CreateRoomRequest`, `JoinRoomRequest`, `LockRequest`,
        `RematchRequest`) per DEV_TECH_DESIGN §3.1. Each with `# KI-Agent unterstützt`.
    - [ ] **Verification:** `cd backend && python -c "from app.models import DuelRoom, DuelParticipant; print('ok')"`.
        **Expected Result:** Prints `ok`, no import errors.

- [ ] **Step 2.2: Duel service — create & join with TTL**
    - [ ] **Action:** Create `backend/app/duel_service.py` with `create_room` and `join_room` per
        DEV_TECH_DESIGN §4.2. Generate a URL-safe `room_id` and a 4-char `room_code` (reuse the
        Crockford alphabet style from `auth_utils`). Set `expires_at = now + ROOM_TTL` (e.g. 30 min).
        `is_guest` is derived by checking `players` for a claimed normalized name. `join_room` is
        idempotent for the same `player_id` and rejects a full room.
    - [ ] **Action:** In `main.py` startup, create a **TTL index** on `duel_rooms.expires_at`
        (`expireAfterSeconds=0`) and a unique index on `duel_rooms.room_id`.
    - [ ] **Verification:** Add `backend/tests/test_duel_room.py` (self-running, unique ids,
        self-cleanup, following the `test_name_claiming.py` style) covering: create → join →
        second join rejected (`room_full`); join non-existent → `room_not_found`. Run
        `python backend/tests/test_duel_room.py`.
        **Expected Result:** All assertions pass; no leftover docs in `duel_rooms`.

- [ ] **Step 2.3: Duel router — room endpoints + state poll**
    - [ ] **Action:** Create `backend/app/duel_router.py` (an `APIRouter`, prefix `/api/v1/duel`)
        with `POST /rooms`, `POST /rooms/{room_id}/join`, and `GET /rooms/{room_id}`. Include it in
        `main.py` via `app.include_router(...)`. The `GET` response **must apply the hidden-choice
        projection** (DEV_TECH_DESIGN §6): never include the opponent's `config`/`config_id` while
        status is `choosing`/`computing`.
    - [ ] **Action:** Follow the ADR-0027 error contract: deterministic `404`/`409`/`410`, no
        catch-all `500` swallowing the conflict paths.
    - [ ] **Verification:** With the backend running:
        ```
        curl -s -X POST localhost:8000/api/v1/duel/rooms -H 'Content-Type: application/json' \
             -d '{"player_id":"a","nickname":"Bob"}'
        ```
        then join with a second `player_id`, then `GET` the room.
        **Expected Result:** Create returns `room_id`/`room_code`/`slot:"red"`; join returns
        `slot:"blue"` + both names; GET shows `status:"choosing"` and **no** opponent config field.

---

## Phase 3: Blind Choice & The Referee (Backend)

*Goal: Both lock in hidden configs; the server computes the match exactly once. Backlog B-04, B-05, B-06, B-08.*

- [ ] **Step 3.1: Own-configs endpoint**
    - [ ] **Action:** Add `GET /api/v1/my_configs?player_id=` (in `duel_router.py`), returning the
        player's active `submissions` (`metadata.player_id == player_id`) as
        `{config_id, nickname, seed}` using `cells_to_grid` for the seed icon.
    - [ ] **Verification:** Submit a config via the editor for a known `player_id`, then
        `curl "localhost:8000/api/v1/my_configs?player_id=<id>"`.
        **Expected Result:** JSON lists that player's config(s) with an 8×8 seed array.

- [ ] **Step 3.2: Lock-in with hidden choice + race guard**
    - [ ] **Action:** Add `POST /rooms/{room_id}/lock` accepting `{player_id, config | config_id}`.
        Resolve `config_id` → seed from `submissions`. Validate an inline `config` with
        `validate_biotope_rules` before storing. Store the participant's hidden `config`, set
        `locked=true`. Use a conditional `find_one_and_update` so only the update observing the
        **second** lock transitions `choosing → computing` and triggers the match.
    - [ ] **Verification:** Extend `test_duel_room.py`: lock A (status stays `choosing`, opponent
        still hidden via GET), then lock B (status flips to `computing`/`result`). Run it.
        **Expected Result:** Single-lock keeps the choice hidden; double lock triggers exactly one
        transition. No double computation.

- [ ] **Step 3.3: Run the headless referee**
    - [ ] **Action:** In `duel_service.py`, add `run_match(room)` mirroring `worker.py:160-185`:
        write `red.json`/`blue.json` to a temp dir, locate the binary among
        `["./build/biotope_headless", "/app/build/biotope_headless", "./biotope_headless"]`, invoke
        via `asyncio.create_subprocess_exec` with an output path, parse the result JSON
        (`winner`, `generations`, `red.population`, `blue.population`), embed it (with both seeds)
        into the room as `result`, set `status=result`. Always clean up temp files (try/finally).
    - [ ] **Verification:** Ensure binaries are built (`make`). Drive a full create→join→lock→lock
        via curl/test, then GET the room.
        **Expected Result:** GET returns `status:"result"` with `winner`, `generations`, both
        populations, and both seeds present.

- [ ] **Step 3.4: Persist the duel record**
    - [ ] **Action:** In `run_match`, after computing, insert a `duels` document per
        DEV_TECH_DESIGN §3.2 (both `player_id`/`nickname`/`config_id`/`seed`, winner, generations,
        populations). Create per-player indexes on `red.player_id` and `blue.player_id` at startup.
    - [ ] **Verification:** Run a full duel, then inspect Mongo:
        `docker-compose exec mongo mongo biotope_db --eval 'db.duels.find().pretty()'` (adjust db
        name/port).
        **Expected Result:** Exactly one `duels` document for the match, with both identities and the
        winner.

---

## Phase 4: Personal Duel Record (Backend)

*Goal: "My Duels" data — head-to-head + best weapon, with cross-device recovery. Backlog B-09 (backend), B-12, B-13.*

- [ ] **Step 4.1: Record aggregation endpoint**
    - [ ] **Action:** Add `GET /api/v1/duels?player_id=` (optional `recovery_code`) calling
        `get_my_record` in `duel_service.py`: aggregate `duels` for the identity into
        `head_to_head` (grouped by opponent nickname with W/L), `best_weapon` (most-winning own
        `config_id`/seed), and an overall `tally`. If `recovery_code` is provided, verify it against
        `players.recovery_code_hash` via `auth_utils.hash_recovery_code` before returning.
    - [ ] **Verification:** Add `backend/tests/test_duel_record.py` seeding a few `duels` docs and
        asserting head-to-head counts, best-weapon selection, and that a wrong recovery code is
        rejected. Run it.
        **Expected Result:** All assertions pass; self-cleanup leaves no test docs.

---

## Phase 5: Frontend MVP Flow

*Goal: A playable end-to-end duel on two phones (no fancy animation yet). Backlog B-02, B-04 (UI), B-05 (UI), B-07, B-09 (UI).*

- [ ] **Step 5.1: Pairing UI (QR + room code)**
    - [ ] **Action:** In `duel.html`, wire "Start Duel" → `POST /rooms`, render a **QR code** of the
        `room_id` (vendored QR generator) plus the `room_code` text. Wire "Scan to Join" → camera QR
        scan (with manual code entry fallback) → `POST /rooms/{id}/join`. Start the ~1.5 s poll loop
        on `GET /rooms/{id}`.
    - [ ] **Verification (Interactive Test):**
        1. Open `/duel/duel.html` on **two** phones (or two browser tabs).
        2. On phone A tap "Start Duel"; on phone B tap "Scan to Join" and scan A's QR (or type the
           4-char code).
        3. **Expected Result:** Both phones leave the lobby and show **both player names**; status
           becomes "Choose your configuration".

- [ ] **Step 5.2: Choice UI (own configs + quick-draw)**
    - [ ] **Action:** Fetch `GET /api/v1/my_configs`, render the player's configs as selectable
        seed icons, plus a quick-draw 8×8 grid (reuse the editor cell model). "Lock in" →
        `POST /rooms/{id}/lock`; then show "Waiting for opponent…" with the opponent's pick **hidden**.
    - [ ] **Verification (Interactive Test):**
        1. With both phones paired, each picks a config and taps "Lock in".
        2. After the **first** lock, check the still-waiting phone.
        3. **Expected Result:** A locked phone cannot see the opponent's choice until both have
           locked; once both lock, both advance to the result.

- [ ] **Step 5.3: Result screen + Rematch**
    - [ ] **Action:** When the poll sees `status:"result"`, show a **WIN/LOSE** screen (compare the
        server `winner` to this phone's slot) with both final populations. Add a **Rematch** button →
        `POST /rooms/{id}/rematch`; when both request, the poll sees `status:"choosing"` and both
        return to the choice screen. Add backend `POST /rooms/{id}/rematch` if not yet present
        (reset `locked`/`config`/`result` for the new round).
    - [ ] **Verification (Interactive Test):**
        1. Finish a duel on both phones.
        2. Confirm the winner shows WIN and the loser shows LOSE (consistent across both).
        3. Both tap "Rematch".
        4. **Expected Result:** Both phones return to the choice screen in the **same** room and can
           play again.

- [ ] **Step 5.4: "My Duels" view (local-first)**
    - [ ] **Action:** After each result, append to `localStorage['biotope_duels']`. Add a "My Duels"
        screen rendering per-opponent head-to-head and overall tally from local data; when a claimed
        identity exists, reconcile with `GET /api/v1/duels` (server authoritative).
    - [ ] **Verification (Interactive Test):**
        1. Play 2–3 duels against the same opponent, mixing wins/losses.
        2. Open "My Duels".
        3. **Expected Result:** Shows e.g. "vs. <Name> 2:1" immediately (before any reload), and the
           overall tally matches what you played.

---

## Phase 6: Polish Layer (Should-Have)

*Goal: Make it feel like a game. Backlog B-10, B-11, B-14.*

- [ ] **Step 6.1: VS splash + soundtrack + Suno credit**
    - [ ] **Action:** Add a VS splash (both seeds + names) shown before the result, with the
        `web/assets/duel_theme.mp3` playing a build-up (gated behind the first user tap for mobile
        autoplay) and a payoff cue on the result screen. Add an unobtrusive footer credit:
        "♪ Music: 'Open Flow' — made with Suno" linking to the Suno URL.
    - [ ] **Verification (Interactive Test):**
        1. Play a duel with phone volume up.
        2. **Expected Result:** Music builds on the VS splash and resolves on WIN/LOSE; the Suno
           credit link is visible and opens the track. Music keeps playing if the network drops
           mid-match (self-hosted).

- [ ] **Step 6.2: Deterministic WASM replay**
    - [ ] **Action:** Load the existing WASM core (`Makefile.wasm` output) in `duel.html`; between
        the VS splash and the result, animate the match by re-running the deterministic simulation
        from both seeds, with a live red/blue population bar. **The displayed winner stays the server
        verdict.** If WASM integration proves impractical, fall back to a compact server frame
        payload (ADR-0028 §2.2 / alternatives) — note the decision in the CHANGELOG.
    - [ ] **Verification (Interactive Test):**
        1. Play the same pair of configs twice.
        2. **Expected Result:** The animation looks identical on both phones and across both runs;
           the final on-screen winner equals the server `winner`. (If they ever disagree, the server
           value is shown — report it so we can check WASM/headless rule parity.)

- [ ] **Step 6.3: Guest fallback & post-match claim prompt**
    - [ ] **Action:** Ensure a phone with no claimed name plays as `Gast-XXXX` (FR-4). After a guest's
        first match, show a gentle, non-blocking prompt to claim a name (links to the editor's
        claim flow); on claim, associate the local guest record with the claimed identity.
    - [ ] **Verification (Interactive Test):**
        1. In a fresh browser profile (no claimed name), play one duel.
        2. **Expected Result:** You appear as `Gast-XXXX`, the match completes, and a non-blocking
           "claim your name" prompt appears afterward (and does not block the Rematch).

---

## Phase 7: Tests, DoD & Documentation

*Goal: Lock in quality and finish per the Definition of Done.*

- [ ] **Step 7.1: Backend test sweep**
    - [ ] **Action:** Ensure `test_duel_room.py` and `test_duel_record.py` cover the happy path and
        the key edge cases (full room, hidden choice, double-lock race, wrong recovery code). Confirm
        existing tests still pass.
    - [ ] **Verification:** Run each `backend/tests/test_*.py` and confirm no regressions; run
        `black --check .` and `ruff check .` in `backend/`.
        **Expected Result:** All tests pass; formatter/linter clean.

- [ ] **Step 7.2: End-to-end integration test**
    - [ ] **Action:** Add a `test_duel_integration.py` that, against a running backend with built
        binaries, drives create→join→lock→lock and asserts a `result` plus one `duels` doc (style of
        `test_system_integration.py`).
    - [ ] **Verification:** `make` first, then run the integration test.
        **Expected Result:** The full duel completes and the record is written.

- [ ] **Step 7.3: Final interactive acceptance**
    - [ ] **Verification (Interactive Test):**
        1. On two phones, run the full loop: scan → choose → watch → WIN/LOSE → rematch, then open
           "My Duels".
        2. **Expected Result:** Under ~10 s from "Start Duel" to the VS splash; head-to-head reflects
           the session; soundtrack and (if enabled) animation play correctly.

- [ ] **Step 7.4: Documentation & merge**
    - [ ] **Action:** Update `docs/CHANGELOG.md` with the feature and any deltas (e.g. WASM-vs-frame
        replay decision). Flip ADR-0028 status `proposed → implemented`. Ensure all new C (if any)
        compiles via `make` with zero warnings and AI attribution is present throughout.
    - [ ] **Verification:** `make` clean build; `git status` review; open a PR onto `biotop`.
        **Expected Result:** Zero-warning build, CHANGELOG updated, PR ready for review.
