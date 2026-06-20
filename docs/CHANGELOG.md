2026-06-20 — Web frontend: German UI localization pass

- All four served web pages (`web/landing/index.html`, `web/editor/editor.html`, `web/duel/duel.html`, `web/mission/mission.html`) had their remaining user-visible English strings translated to German, for a consistent German interface. Only display text was touched — code comments, identifiers, `console.*` messages, JSON field names, `localStorage` keys, CSS classes/ids and other wire values were left unchanged (the `'red'`/`'blue'` slot keys and `room.status` values in particular stay English so the duel polling protocol is unaffected).
- `web/duel/duel.html`: the bulk of the work — `lang="en"` → `lang="de"`, `<title>`, all seven screens (home/join/lobby/choice/vs/result/my-duels), the footer, and ~25 JS-facing strings (alerts, verdicts, status/sync messages, fallback labels). Notable term choices: `Shoulder Duel` → **Schulter-Duell**, `FIGHT` → **KAMPF**, `Lock in`/`Locked in` → **Festlegen**/**Festgelegt**, `Quick-draw` → **Schnellzeichnung**, `Best bio` → **Beste Spezies**, `YOU WIN`/`YOU LOSE`/`DRAW` → **DU GEWINNST**/**DU VERLIERST**/**UNENTSCHIEDEN**, head-to-head draw suffix `(Nd)` → `(Nu)`. The `(you)` self-marker → `(du)`.
- `web/landing/index.html`: `Conway Lab` → **Conway-Labor** (title + kicker), `Shoulder Duel` → **Schulter-Duell**, `Phone gegen Phone` → **Handy gegen Handy**, `Genom designen` → **Genom gestalten**.
- `web/editor/editor.html`: `Clear` → **Leeren**, `Recovery-Code` → **Wiederherstellungs-Code** (input placeholder, hint, overlay title, and the downloadable PNG label — the `biotop-recovery-*.png` filename slug stays ASCII), `« Zurück zum Lab` → **« Zurück zum Labor**.
- `web/mission/mission.html`: `« Zurück zum Lab` → **« Zurück zum Labor**.
- Kept English by decision (established loanwords / proper names / titles): `Mission Statement`, `Mission Briefing`, `Sound`, `Reset`, `Scan`, `Limit`, `Code`, `Arena`, `Codex`, `VS`, `Gen`, plus brand/track names (`Biotop`, `Open Flow`, `Suno`). The C/Raylib GUI and the `docs/Präsentation/` report HTML were out of scope.

2026-06-19 — ADR-0032: Incremental tournament computation — Stage 2 (deterministic match cache + delta computation)

- Optimization: the tournament worker now computes only the *uncached* seed-pairs each epoch instead of the full O(N²) round-robin. Adding one player to a field of N costs ~N new matches instead of N(N−1)/2 (e.g. 49 vs 1225 at N=50). The expensive part (simulation) is incremental; ranking aggregation is cheap arithmetic.
- New collections: `match_results` (content-addressed cache keyed by the unordered pair of seed hashes, unique index on `pair_key`) and `worker_state` (the Stage 1 epoch guard). The unordered key is justified by the ADR-0026 torus colour-swap symmetry; valid only while matches stay deterministic (determinism tripwire documented at the cache boundary).
- `src/core/core_types.h`: added `Pairing {a,b}` and `PairOutcome {idx_a, idx_b, winner, pop_a, pop_b, activity_sum, stable_at_generation}`.
- `src/io/file_io.c`/`.h`: `parse_batch_file` accepts an optional top-level `pairings` array (absent → `NULL`/`-1` = full round-robin; out-of-range indices skipped); `save_batch_results` emits a per-pair `match_results` array in pairings mode only, so full-mode output is byte-unchanged. Per-pair results are keyed by **competitor index** — Python owns `seed_hash` and maps indices → `pair_key`, keeping all hashing in Python.
- `src/apps/hyper/main_hyper.c`: factored the per-match scoring+highlight body into `play_pairing(...)`; pairings mode loops over supplied pairs (OpenMP retained), full mode keeps the `i<j` loop. ADR-0026 tripwire comment extended to cover the cache.
- `backend/app/worker.py`: `execute_epoch` rewritten as a delta pipeline — `enumerate_needed_pairs` → cache lookup → compute missing pairs via `run_hyper_worker` (canonical hash order) → `bulk_write` upsert → Python ranking aggregation → highlights from cache (`cells_to_seed64` mirrors `grid_to_bitboard`) with the unchanged oscillation filter (ADR-0023). `database.py` adds `match_results_col`/`worker_state_col` + schema docs; `main.py` adds `_ensure_match_cache_indexes` (unique `pair_key`).
- `backend/tests/test_match_cache.py` (new): pair-enumeration / missing-set / dedup / `cells_to_seed64` tests. Aggregation cross-validated autonomously against a C full round-robin (6 competitors incl. a duplicated pattern): delta computed 11 pairs vs 15 and reproduced the full ranking exactly. C `tests/test_hyper_pairings.c` covers the parser. Zero-warning build retained.
- Hardening: `execute_epoch` now verifies the C worker returned exactly one result per requested pairing (`len(match_results) == len(pairings)`). On mismatch — the signature of a stale `biotope_hyper_worker` that ignores the `pairings` array and emits no `match_results` — the epoch aborts **without** overwriting the leaderboard and **without** writing the roster fingerprint, so it retries on the next tick once the correct binary is in place. Previously such a mismatch silently zeroed every player's stats and froze that state via the guard. (Tip: rebuild just the worker with `make build/biotope_hyper_worker` — no Raylib needed — then recreate the matchmaker container.)

2026-06-19 — ADR-0032: Incremental tournament computation — Stage 1 (roster-change early-exit guard)

- Optimization: the tournament worker no longer re-runs the full O(N²) round-robin every 60 s when nothing changed. Matches are deterministic (`run_isolated_match`, `src/core/game_logic.c`; the only `rand()` is in `init_world`, GUI-only), so an unchanged active roster yields a bit-for-bit identical ranking — recomputing is pure waste.
- `backend/app/worker.py`: new pure helpers `seed_hash(cells)` (SHA-256/16 over canonical JSON of the 8×8 pattern) and `roster_fingerprint(submissions)` (order-independent hash over `id:seed_hash` pairs — detects additions, removals, deactivations, and edits). `execute_epoch` computes the current fingerprint after loading active submissions and early-returns ("Roster unchanged since last epoch — skipping.") when it matches the last successful epoch's fingerprint stored in the new `worker_state` singleton (`_id: "epoch_guard"`). The fingerprint is written only after a successful epoch, so a failed epoch retries next tick.
- `backend/tests/test_epoch_guard.py` (new): unit tests for `seed_hash` (deterministic, edit-sensitive) and `roster_fingerprint` (identical/reordered rosters equal; edit/add/remove differ). All pass.
- Foundation for Stage 2 (deterministic match cache with delta computation); see ADR-0032 and DEV_TASKS-0032.

2026-06-15 — ADR-0031: Auto-paging rotation for the Kiosk Global Leaderboard

- Feature: the Kiosk "Global Leaderboard" no longer shows only the top ~9 players. Within its slot it now auto-pages through all ranked players at full row size (ranks 1–9, 10–18, …) and then hands off to Multicam as before. Page index is derived purely from `state_timer`, so no new mutable controller state is introduced.
- End-to-end capacity raised 20 → 50: `backend/app/main.py` adds a named `LEADERBOARD_MAX_ROWS = 50` behind the `.limit(...)`/`.to_list(...)` (verified live: the endpoint returned `rows=27` instead of the former 20-cap); `src/io/network_io.h` raises `MAX_LEADERBOARD_ENTRIES` 20 → 50 to match. `total_count` stays an independent server-side count, so the honest "+N more" line (ADR-0030) is now shown only when more than 50 eligible players exist server-side.
- `src/gui/renderer.h`: shared timing constants `KIOSK_LB_SECONDS_PER_PAGE` (6 s), `KIOSK_LB_MIN_DURATION` (15 s), `KIOSK_LB_MAX_DURATION` (48 s).
- `src/gui/renderer.c`: new helpers `kiosk_lb_max_fit`/`kiosk_lb_rows_per_page` (single source of the rows-per-page math, 28 px row floor unchanged) and public `kiosk_leaderboard_page_count` / `kiosk_leaderboard_duration`. `draw_kiosk_leaderboard` renders the current page window using **global** rank numbering, keeps the red/blue/accent podium styling on the true top-3 only (page 0), draws a right-aligned `PAGE x / y` indicator when `num_pages > 1`, and emits "+N more" on the last page only. The footer progress bar now spans the adaptive slot duration instead of a fixed 15 s.
- `src/gui/app_state_manager.c`: the `KIOSK_SUB_LEADERBOARD` → Multicam transition now uses `kiosk_leaderboard_duration(...)` (= `num_pages × 6 s`, clamped to [15, 48] s) instead of the hard-coded 15 s. A single page keeps the unchanged 15 s behaviour.
- Verified interactively (27 ranked players → 3 pages / ~18 s): pages rotate every ~6 s, ranks run continuously across pages, podium appears on page 1 only, `PAGE x / 3` tracks the table, and the progress bar fills exactly to the Multicam hand-off. The "+N more" path was confirmed by temporarily lowering the cap to 5 (`+ 22 more` shown on a single page). Build stays warning-free; `test_ranking.py` and `test_system_integration.py` pass.

2026-06-15 — ADR-0030: Honest "+N more" leaderboard overflow count

- Fix: the Kiosk "Global Leaderboard" `+N more` line froze (observed `+ 11 more`) once ≥ 20 configurations were ranked, because it was computed from the 20-row cap rather than the true total. It now reports the real number of additional ranked configurations.
- `backend/app/main.py`: `get_leaderboard` factors its filter into one shared `query`, adds `total_count = count_documents(query)` over the same predicate, and returns it alongside `leaderboard` (verified live: `rows=20, total_count=27`).
- `src/io/network_io.h`/`.c`: `LeaderboardData` gains `total_count` (distinct from the parsed-row `count`); the parser reads `total_count` with a fallback to `count` for older backends.
- `src/gui/renderer.c`: `draw_kiosk_leaderboard` derives `hidden_count` from the true total and also emits the line when all parsed rows fit but more eligible rows exist server-side (`true_total >= parsed_entries` clamp guards the non-atomic count/list read). The geometric row cap and footer protection are unchanged.
- Both 20-record caps are retained by design — only the count becomes honest. See ADR-0030; follow-up to ADR-0025.

2026-06-15 — Mobile landing / title screen + cross-page navigation

- `web/landing/index.html` (new): smartphone landing / title screen in the shared Modern-Retro "Digital Lab" theme (cyan/red palette, Orbitron/Rajdhani/Share-Tech-Mono, scanline veil, HUD bracket corners). Carries the project masthead (`// Conway Lab`, "Biotop: Wenn Zellkulturen den Kampf aufnehmen", "> von Friedemann Decker") and routes onward to the editor and the shoulder-duel page via two "mission-select" portal cards. A live red-vs-blue Game of Life runs as an ambient `<canvas>` backdrop (toroidal, majority-team birth color, reseeds on stagnation); each card carries its own tiny live-Conway preview; footer shows the global species counter (`/api/v1/stats/count`). Respects `prefers-reduced-motion` (disables backdrop + reveals).
- `backend/app/main.py`: `GET /` now serves the landing page (`FileResponse("web/landing/index.html")`) instead of the JSON stub.
- `web/editor/editor.html`: masthead trio removed (the landing now carries the branding); added a `« Zurück zum Lab` footer link back to `/`; dead masthead CSS (`.kicker`, `h1`, `.author`) pruned.
- `web/duel/duel.html`: added a `« Back to Lab` footer link back to `/`. During the match replay the big banner now reads a pulsing amber **FIGHT** (new `.verdict.fight`, reusing the win-verdict `verdictPulse`) instead of "VS"; the small "Fight!" status line below the board was removed (element + both JS writes). The "Red VS Blue" matchup separator is unchanged.

2026-06-14 — ADR-0028: Shoulder-Duel Mode (on-demand server-authoritative 1:1 duel)

- New feature: a 1:1 duel between two co-located phones — pair (QR deep-link + 4-char room code) → blind config choice → the server runs the referee exactly once → WIN/LOSE → rematch → personal record. Built on the silent identity from ADR-0027.
- `backend/app/models.py`: `DuelParticipant`, `DuelRoom`, and the request schemas (`CreateRoomRequest`, `JoinRoomRequest`, `LockRequest`, `RematchRequest`).
- `backend/app/duel_service.py` (new): room lifecycle (`create_room`/`join_room` with a TTL on `expires_at`, idempotent same-player join, `room_full`), hidden-choice lock-in (`lock_choice`; only the second lock transitions `choosing → computing` and triggers exactly one match), the headless referee (`run_match`, mirroring `worker.py`, embeds `result` and persists exactly one `duels` doc), `request_rematch` (per-slot flag → guarded `result → choosing` reset), `resolve_room_code`, `get_my_configs`, `get_my_record` (head-to-head, best weapon, tally; optional recovery-code auth), and `get_room_frames`.
- `backend/app/duel_router.py` (new): `/api/v1/duel` room endpoints + `GET /my_configs` + `GET /duels`, following the ADR-0027 deterministic error contract (404/409/410, no catch-all 500). `GET /rooms/{id}` applies the hidden-choice projection (the opponent's config is never exposed before the result).
- `backend/app/main.py`: mounts `/duel`, `/assets`, `/vendor`; startup creates the duel TTL/unique and per-player `duels` indexes.
- Deterministic replay (Phase 6.2): `src/apps/headless/main_headless.c` gained opt-in per-generation frame capture (extra positional argv path) — 101 frames (gen 0 seeds + gens 1..100) over the 8×16 playfield, each a compact row-major "0/1/2" string; backward compatible (no path → unchanged behaviour). `duel_service` stores frames at the room top level, strips them from the short-poll projection, and serves them once via `GET /rooms/{id}/frames`.
- `web/duel/duel.html` (new): mobile-first poll-driven flow (home → lobby/join → choice → VS splash + replay → result → My Duels). QR deep-link join (`web/vendor/qrcode.js`, MIT) with a manual room-code fallback. Self-hosted Suno soundtrack (`web/assets/duel_theme.mp3`, "Open Flow") with a VS-splash build-up / result payoff and a Suno credit. The canvas replay animates the match frame by frame, clock-locked to the soundtrack (~120 BPM → one generation per 1/16 note, driven off the audio clock so it never drifts), with a proportional red/blue population meter and a "Gen N / 100" counter + timeline. Non-blocking guest post-match claim prompt.
- `web/editor/editor.html`: on a successful claim the species name is now persisted to `localStorage['biotope_nickname']`, so the same-browser duel page reuses the claimed name instead of the `Gast-XXXX` fallback.
- Core simulation fix (found during duel verification): the chunk-skip optimization froze the headless single-match board. `src/io/file_io.c` now calls `activate_chunk_at` for seed cells, and `src/core/game_logic.c` writes skipped chunks as `DEAD` into `next_gen`. The tournament path was unaffected. Verified: lone cell → 0, L-tromino → 4, blinker oscillates, block stable.
- Tests: `backend/tests/test_duel_room.py`, `test_duel_record.py`, and `test_duel_integration.py` (new — full create→join→lock→lock asserting a result, exactly one `duels` doc, and a non-empty 8×16 / 101-frame replay payload via the real C binary). The `backend`/`matchmaker` containers install `libgomp1`/`libcurl4` so the referee binary runs.

2026-06-12 — ADR-0027: Name-Claiming with Server-Generated Recovery Code

- `backend/app/auth_utils.py` (new): pure helpers `normalize_nickname()` (trim+lowercase+collapse whitespace), `generate_recovery_code()` (Crockford Base32, e.g. `BIOTOP-7F3A`), `hash_recovery_code()` (SHA-256). Unit-tested in `backend/tests/test_auth_utils.py`.
- `backend/app/models.py`: new `Auth` model + `Submission.auth` (top-level, optional, default empty). `DBSubmission` now inherits a `SubmissionBase` (metadata+config only) so the recovery code can never leak into the `submissions` collection. `Player` gains `nickname_normalized` and `recovery_code_hash`.
- `backend/app/main.py`: `submit_config` rewritten as name-claiming with two-factor ownership (silent `player_id` OR recovery code). First claim issues a code; same-device returns are silent; a different device is blocked unless it presents the code (then `player_id` is re-bound). Startup hook creates a unique index on `players.nickname_normalized`. Exception handling fixed so `409`/`403`/`400` no longer collapse into `500`.
- Error contract: `201` (+ `recovery_code` on first claim, `reclaimed: true` on reclaim), `409 {"error":"name_taken"}`, `403 {"error":"invalid_recovery_code"}`, `400` (domain rule). Concurrent claims race on the unique index → `DuplicateKeyError` → `409`.
- `backend/scripts/reset_db.py` (new): destructive clean-slate reset (clears `players` + `submissions`, ensures the unique index) for pre-fair setup.
- `web/editor/editor.html`: sends the `auth` block; branches on the response matrix; reveals a recovery-code input on `409`; shows a screenshot-friendly recovery card overlay with a `<canvas>`-rendered "save as PNG" download (graceful degradation if unavailable).
- Tests: `backend/tests/test_name_claiming.py` (new) — end-to-end claim → silent → block → wrong code → reclaim, asserting no plaintext code is ever persisted. Existing validator/ranking tests still pass.
- C application unchanged (`network_io.c` is GET-only; never POSTs submissions).
- B-13 (DEV_SPEC-0027): on `409` the editor now shows a two-case hint — "not your species → pick another name" vs. "submitted before on another device → enter your recovery code" — so an unaware first-time visitor who picks a taken name is no longer confused by a code prompt.

2026-06-08 — Deployment Guide for Server Operation

- `docs/DEPLOYMENT.md` (new): Step-by-step guide to starting the stack on a server (VM, SSH). Separation between headless server stack (Mongo, backend, editor, matchmaker via `docker compose`) and graphical display (`build/biotope`, requires a display).
- Documented: only `build/biotope_hyper_worker` + `build/biotope_headless` are required for the matchmaker (no Raylib dependency), `.env` must be recreated on the server (gitignored), password must be identical in `MONGO_PASSWORD` and `MONGODB_URI`.
- Three display options described; recommended: Option C (local GUI + SSH tunnel `-L 8000:localhost:8000`), since the GUI hardcodes `http://localhost:8000`.
- `README.md`: reference to the new deployment guide added in the intro block.

2026-06-01 — ADR-0025: Per-Config Leaderboard + 8x8 Start-Config Icon

- `backend/app/grid_utils.py` (new): `cells_to_grid()` — sparse `[x,y]` → dense 64-int grid (`index = y*8 + x`), defensive against empty/malformed/out-of-bounds cells.
- `backend/app/main.py`: `get_leaderboard` now reads per-configuration from `submissions` (`status=active`, `matches_played>0`) instead of aggregated per-nickname from `players`. Fixes the last-write-wins distortion (weakest submission overwrote the display). New field `seed` (8x8 start configuration) per entry.
- `src/io/network_io.h`: `int seed[GRID_SIZE_8X8]` added to `LeaderboardEntry`.
- `src/io/network_io.c`: leaderboard parser reads `seed` array (default 0, 64-element bound guard).
- `src/gui/renderer.c`: `draw_kiosk_leaderboard` shows a CONFIG icon column between RANK and PLAYER; column layout redistributed; 8x8 icon monochrome (`THEME_ACCENT`), size coupled to `rowHeight`.
- Effect: each configuration is its own leaderboard row with its own icon and correct stats; the same player name can appear multiple times and is distinguishable by the icon.

2026-06-01 — ADR-0024: Client-Side Highlight Rotation (Round-Robin) + Replay Bugfixes

- `src/io/network_io.h`: `MatchHighlight matches[4]` → `matches[10]` (pool capacity).
- `src/io/network_io.c`: parse loop `i < 4` → `i < 10`.
- `src/gui/app_state_manager.h`: `highlight_pool_index`, `highlight_pool_size` added to `KioskController`; `restore_main_game_context` declared; `update_app_state` signature extended with `RenderContext *r_ctx`.
- `src/gui/app_state_manager.c`: static helper `load_kiosk_sims_from_pool()` extracted; pool index rotates by `match_count` on MULTICAM→LB transition; on LB→MULTICAM transition sims are loaded from the new pool window. `highlight_pool_index` is only reset on the first data fetch (pool_size==0), not on every refresh — this was the root cause of the rotation bug.
- `src/core/config.h`: `DEFAULT_GRID_ROWS` / `DEFAULT_GRID_COLS` (50) added.
- Replay fix: KEY_1..4 now starts fullscreen replay on the correct 8×16 grid (identical to the tournament). `restore_main_game_context()` restores 50×50 on exit. `r_ctx` is correctly re-initialised.
- Renderer fix: player names visible in replay header (replaces "SIMULATION ACTIVE").
- `src/gui/renderer.c`: `draw_current_state` extended with `SimulationContext *sim_ctx`; bonus fix: thumbnails and `metric_reason` badge use `render_slot` formula.
- Effect: kiosk shows different matches on each 30 s cycle (round-robin over up to 10 highlights). Fullscreen replay shows the correct match on the correct board.

2026-05-31 — ADR-0023: Oscillator Filtering & Kiosk World Alignment

- Kiosk simulation world changed from 50×50 to 8×16 (app_state_manager.h/.c), identical to run_isolated_match() in the hyper-worker. Leaderboard result and kiosk display now use the same arena.
- Seed placement in kiosk corrected: Red on the left (like the hyper-worker), Blue on the right.
- Oscillator filtering implemented in backend/app/worker.py (ADR-0023): _step_numpy(), is_oscillating_match(), filter_highlights_by_oscillation(). Oscillating patterns (period 2–5) are moved to the end of epoch_highlights; non-oscillating matches appear first. Soft fallback when 0 clean candidates are found.
- numpy>=1.24.0 added to backend/requirements.txt.
- 13 unit tests in backend/tests/test_oscillator_detection.py, all passing.
- End-to-end verified: worker detects and logs oscillators, MongoDB contains correctly resolved nicknames and 10 ordered highlights.

26.12.2025: GitHub account set up, GitHub repository set up, Dockerfile created, docker-compose created, dev container created, C environment set up, hello script created and tested.

28.12.2025: README.md created.

05.01.2026: Matrix initialisation created, first version of current matrix (World) output in development environment created.

07.01.2026: Cell calculation in update_generation() modelled. Bottom-left and bottom-right corners not yet modelled. README.md extended with description of the grid layout.

08.01.2026: Cell calculation in update_generation() completed. Grid size and display delay integrated as command-line parameters.

09.01.2026: New branch (biotop) created for separate version management of "manual" programming and "vibe-coding". Vibe-coding concept for extended functionality "Interactive Red vs Blue game concept" created. Interactive Red vs Blue game concept implemented using Gemini-CLI-Agent. After implementing the game logic, UI refactoring performed.

10.01.2026: UI fine-tuning.

16.01.2026:
feat(gui): Key repeat for configuration parameters implemented — Helper function `IsActionTriggered` added for handling initial key press and continuous repeat. Continuous input processing activated for Grid Size, Delay, Max Rounds, and Max Population. Initial delay set to 500 ms and repeat interval to 50 ms for smooth value changes.

feat(gui): Drag-to-Paint function added in editor mode — Enables activating or deactivating multiple cells by dragging with the mouse button held. Status (placing or deleting) is automatically detected on first click. Team hemisphere and population limit checks remain active while dragging.

perf(gui): Rendering bottleneck fixed via texture-based drawing — `DrawGridAndCells` switched to texture rendering to minimise VcXsrv/X11 lag. Replaces thousands of `DrawRectangle` calls with a single texture update per frame. Implements resource management (lazy init) for texture and pixel buffer. Uses point filtering for pixel-accurate display when scaling.

17.01.2026:
feat ADR-0003-integrated-simulation-protocol: Implementation of the integrated simulation protocol and replay system — Introduction of protocol format v2 (includes timestamp, round count, and delay). Implementation of "save-on-start" logic: every simulation is automatically archived in 'biotope_results/' on start. New in-app file browser (STATE_LOAD) for browsing and loading past simulations. Extension of protocols with result data (winner, final score) after simulation completion. Integration of metadata preview in the browser (date, grid size, result). Backwards compatibility ensured for existing .bio files. Removal of now-redundant Markdown export (export_stats_md), as results are stored directly in the protocol. Optimisation of UI layouts to avoid text overlap in archive mode.

05.05.2026 (Anniversary Edition):
feat ADR-0005: Frictionless WASM Onboarding — Porting the entire C application to WebAssembly via Emscripten. Introduction of an `UpdateDrawFrame`-based main loop for browser compatibility. Implementation of persistent storage via IndexedDB (FS.syncfs) for saving .bio protocols in the browser. Addition of phase-based onboarding (Tutorial/Puzzle state).

feat ADR-0006: GPU Aesthetic Overhaul — Migration of rendering from CPU-based pixel arrays to a GPU shader pipeline. Implementation of temporal trails ("Fossils") via ping-pong framebuffer technique. Support for GLSL 330 (desktop) and GLSL 100 (Web/WASM). Efficient transfer of the grid state as a 1-byte grayscale texture to minimise bus bandwidth. Complete resource management and VRAM cleanup integrated.

06.05.2026:
feat ADR-0007: Competitive 1v1 USP Focus — Implementation of the "Catalyst Strike": a one-time 10x10 destruction action per team per match. Introduction of the "Ignition Sequence": a dramatic 3-second countdown before simulation start. Blind-Draft Phase: turn-based editor (Red vs. Blue) with the opponent's side hidden. Dynamic telemetry: automatically scaling population graphs based on peak value. Navbar scoreboard: centred real-time statistics for better competitive overview.

feat ADR-0008: Epic Scale Tournament Architecture — Extension of grid limits to 5000x5000 for native desktop builds. Implementation of an "Active Chunk" heuristic for dramatic performance improvement on large grids (skipping dead sectors). New `STATE_OBSERVER` (observer mode) with specialised broadcast tools. Integration of a 2D camera (Raylib `Camera2D`) for smooth zoom and free panning. Optimised randomisation logic: uniform 3% distribution across the entire hemisphere.

20.05.2026:
feat(gui): Enhanced Random Population Density — Increased the random start cell density from 3% to 37.5% per team for the [R]andom key in editor mode.

feat(wasm): Persistent Browser Storage (IDBFS) — Added automatic creation and mounting of the 'biotope_results/' directory to IndexedDB (IDBFS). Ensured filesystem persistence so saved .bio files are retained after browser refreshes. Added -lidbfs.js and -s FORCE_FILESYSTEM=1 to the WASM build flags.

fix(load): Simulation Logic After Load — Resolved a critical bug where loaded configurations failed to simulate (black screen). Fixed the 'load_grid' function to properly initialise and update the 'chunk_map' (spatial partitioning) for loaded cells.

perf(build): Robust Docker Build Environment — Renamed 'Makefile.web' to 'Makefile.wasm' to avoid GNU Make naming conflicts with legacy 'tangle' rules. Implemented automatic 'emcc' path detection in 'Makefile.wasm' to ensure the compiler is found even in non-interactive Docker shells.

21.05.2026:
feat ADR-0009: Multiplayer JSON Ecosystem (Hard Cut) — Complete replacement of the obsolete `.bio` text format with a web-compatible `.json` format for client-server communication. Specification of leagues (Beginner, Rookie, Champions) with individual bounding boxes. Introduction of relative coordinates to decouple patterns from grid positions. Integration of the open-source `cJSON` library into the C codebase and adaptation of build systems. Complete refactoring of `file_io.c` (`save_grid`, `load_grid`) to prevent format fragmentation (single source of truth).

22.05.2026:
feat ADR-0010: Headless Simulation Worker — Implementation of a CLI-based C worker (`biotope_headless`) for automated match simulation on servers. Decoupling of simulation logic from the GUI (Raylib). Support for rich metadata (`player_id`, `nickname`) and flexible cell formats. Generation of structured result JSONs for backend matchmaking.

feat ADR-0011: REST API & Server-Side Validation — Development of a robust backend using Python/FastAPI. Implementation of the `POST /api/v1/submit_config` endpoint for player submissions. Introduction of strict server-side validation (fair play): max. 38% biomass (24 cells) and 8x8 bounding box. Automated API documentation via Swagger/OpenAPI.

22.05.2026 (Evening):
feat ADR-0012: Automated Matchmaking & Elo System — Full integration of MongoDB Atlas as persistent storage for players, patterns, and matches. Implementation of the "Proximity Swiss" algorithm for fair opponent pairing based on Elo rating. Development of an autonomous background worker (`matchmaker`) that orchestrates simulations via `biotope_headless`. Introduction of a dynamic Elo ranking system with variable K-factor for fast convergence. Containerisation of the matchmaker as a dedicated Docker service with automated dependency management.

feat ADR-0013: Mobile-First Web Draft Editor — Implementation of a standalone web editor (`editor.html`) for creating and submitting cell configurations via smartphone. Optimised touch interface with 8x8 grid and real-time biomass validation (max. 24 cells). Persistent player identity (UUID) via browser LocalStorage. Integration into the main app (`gui.c`) via a direct link button ("MOBILE EDITOR"). CORS configuration in the FastAPI backend to allow cross-origin submissions from mobile devices.

23.05.2026:
feat ADR-0015: Architectural Consolidation and Modularization — Complete refactoring of the C codebase to eliminate monolithic structures and circular dependencies. Separated `gui.c` into specialised modules: `renderer.c` (Raylib presentation) and `app_state_manager.c` (core logic and state machine). Centralised magic numbers and parameters into a new `config.h`. Consolidated all core data structures (`World`, `GameConfig`, `Team`, `AppState`) into a pure C header `core_types.h`. Migrated all I/O and JSON parsing routines out of entry points and strictly into `file_io.c` with robust boundary checks. Implemented dedicated C-based state-of-the-art `DEV_TEST` suites for every refactoring phase, including an end-to-end golden snapshot integration test to guarantee zero regressions.

24.05.2026:
refactor(gui): UI Clean-up and Mobile Editor Discovery — Removed redundant manual save button ([S] key) from editor mode as auto-save is performed on simulation start. Removed non-functional 'MOBILE EDITOR' button from the main simulation HUD. Implemented a new 'Mobile Editor Discovery' section in the tutorial screen (STATE_PUZZLE) featuring a QR-code placeholder and short-link (biotope.io/editor). Updated test checklists and debugging reports to reflect the streamlined UI architecture.
fix(gui): Ignition Countdown State Leak — Resolved an issue where the ignition countdown would only display '1' in subsequent simulation runs. Fixed by ensuring `ignitionStartTime` is reset to `0.0` when returning to `STATE_CONFIG` from either `STATE_FINISHED` or `STATE_GAME_OVER`.
feat(gui): Independent Pause and Observer Modes — Introduced a global `is_paused` flag in `GameConfig` to decouple simulation state from camera state. Assigned `[SPACE]` to toggle simulation pause across all active modes with clean visual feedback in the dynamic footer. Updated `[O]` to toggle the observer (free-camera) mode independently of the simulation's run state. Removed redundant green status messages that overlapped with the scoreboard in the header. Implemented a dynamic 'Status-First' footer showing "PAUSED" vs "RUNNING" and context-aware controls. Ensured `is_paused` state and camera state are reset when returning to the configuration screen.
fix(gui): Observer Camera State Leak — Resolved an issue where camera zoom and pan settings persisted across different simulation runs. Fixed by ensuring the `observer_camera` is reset to default values when returning to the configuration screen.
refactor(gameplay): Catalyst Feature Removal — Completely removed the 'Catalyst' mechanic (`apply_catalyst`) from the simulation logic and UI. This change eliminates redundant interactions, prevents UI clutter (overlapping texts in the scoreboard), and simplifies the codebase without affecting the core competitive loop.

28.05.2026:
feat ADR-0017: C-Networking and API Integration — Implementation of an asynchronous network layer in C using `libcurl` and `pthread` to connect to the backend. Introduction of `network_io.c` to encapsulate all HTTP calls per clean architecture. Implementation of background threads for leaderboard and highlight fetching to prevent UI stutter (60 FPS guarantee). Extension of the hyper-worker with the metric "activity sum" (sum of all cell changes) for identifying visually dynamic highlights. Memory optimisation: internal storage of 8x8 highlight seeds as `uint64_t` bitboards. Backend update: full support for MongoDB Atlas with configurable credentials via `.env` and automatic connection check on startup. Integration of test triggers (`[L]` for leaderboard, `[H]` for highlights) to verify data transfer.

feat ADR-0018: Multicam Render Context Architecture
- Refactored core simulation and renderer logic to decouple from global state, introducing `SimulationContext` and `RenderContext`.
- Enabled simultaneous multi-viewport rendering and parallel simulations.
- Implemented `BeginScissorMode` clipping to prevent quadrant rendering bleed.
- Added a 2x2 splitscreen "Wusel-Multicam" PoC (Kiosk Mode) triggered via the `[K]` key.
- Verified memory safety (0 definitely lost leaks from our new logic) and successful texture management.

feat ADR-0019: Kiosk Mode State Machine and UI
- Implementation of the `STATE_KIOSK_MODE` state machine for autonomous app operation.
- Added leaderboard rendering and 2x2 multicam splitscreen rendering including seamless switching every 15/30 seconds.
- Click-to-Replay functionality added: clicking a multicam quadrant starts a full-screen replay of the simulation (including ignition countdown).
- Global failsafe inactivity timer: returns to kiosk mode after 60 seconds of inactivity from any menu or replay.
- Memory safety on simulation teardown and kiosk transitions (fixing segfaults and memory leaks on world allocations).

31.05.2026:
feat ADR-0021: Kiosk Mode Engagement Enhancement
- P1: Player names per multicam quadrant — `MatchHighlight.participant_red/blue` is now copied to `SimulationContext.participant_red/blue` (previously discarded) and rendered as a colour-coded name header (`THEME_RED` / `THEME_BLUE`) above each quadrant.
- P2: Live population bar per quadrant — `KioskController` extended with `quad_red_pop[4]` / `quad_blue_pop[4]`; the previously discarded `dummy_red/dummy_blue` outputs of `update_generation_ctx` are now persistently stored and rendered as a proportional red/blue bar at the bottom of each quadrant.
- P3: Metric-reason label — `MatchHighlight.metric_reason` (e.g. "LONGEST MATCH") is displayed right-aligned in the header strip of each quadrant.
- P4: 8×8 start pattern thumbnail — shows the original seed pattern as a small colour grid in the upper-left corner of each quadrant, making the contrast between simple input and complex output visible.
- P5: QR code placeholder + CTA on the leaderboard screen — stylised QR code and "Submit YOUR strategy!" invitation text for exhibition visitors.
- P6: Visual progress bar replaces the text timer "SWITCHING IN X SECONDS" in both kiosk substates.

feat ADR-0022: Kiosk UI Architecture Refactor & UX Enhancement (Phase B)
- Architecture: render blocks extracted from monolithic switch into `draw_kiosk_leaderboard()` and `draw_kiosk_multicam()`.
- Scaling: `viewport_bounds` is now recalculated every frame from `compute_kiosk_layout()` — quadrants scale correctly on window resize and fullscreen.
- Interaction: mouse click trigger removed; keys `[1]`–`[4]` start replay of the selected match from the initial state (seed), not from the running state.
- Leaderboard: footer panel (fixed bar behind CTA + progress bar), proportional column layout (80% screen width, 10% margin), clip-to-fit with `+N more` indicator, top-3 row highlights, column "STAMINA" → "ENDURANCE", QR panel removed.
- Multicam: title "WUSEL-MULTICAM KIOSK MODE" → "LIVE BATTLES", proportional score bar (≥ 20 px), quadrant separator lines, metric_reason as centred badge below player names, thumbnail cell size proportional (≈ 20% quadrant height), footer panel.
- Cleanup (Boy Scout Rule): unused KioskLayout fields `font_cta`, `font_name`, `font_score` removed.
