# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

"Biotop" is an extended Conway's Game of Life where two teams (Red vs. Blue) compete for cellular dominance. It is a university project written in C (Raylib GUI, headless simulation, OpenMP hyper-worker) with a FastAPI/MongoDB backend and a web-based pattern editor.

The main branch is `biotop`.

## Build Commands

```bash
# Build all three native binaries into build/
make

# Clean build artifacts
make clean

# Build WebAssembly (requires Emscripten / emcc in PATH or /opt/emsdk/)
make -f Makefile.wasm
```

The three produced binaries are:
- `build/biotope` — interactive GUI (Raylib, requires display)
- `build/biotope_headless` — single match CLI: `./build/biotope_headless <red.json> <blue.json> [out.json]`
- `build/biotope_hyper_worker` — batch tournament CLI: `./build/biotope_hyper_worker <input_batch.json> <output_results.json>`

Compiler flags: `gcc -Wall -Wextra -std=c99 -O3 -fopenmp`. The build **must** produce zero warnings.

## Backend & Services

```bash
# Start the full stack
docker-compose up

# Run backend standalone (from /backend)
pip install -r requirements.txt
uvicorn app.main:app --host 0.0.0.0 --port 8000 --reload
```

`docker-compose up` brings up four services:
- `mongo` — local MongoDB (`mongo:4.4`), exposed on `127.0.0.1:27018`. This replaced the earlier MongoDB Atlas setup; older CHANGELOG/ADR entries that mention Atlas are historical.
- `backend` — FastAPI on port 8000 (`uvicorn app.main:app`), depends on `mongo` being healthy. Mounts `./build` as `/app/build`.
- `matchmaker` — the Python tournament worker (`app/worker.py`). It mounts `./worker_bin` (not `./build`) as `/app/build`, so the C binaries it invokes must be present in `worker_bin/`.
- `c-dev` — C build/dev container with X11 forwarding (`DISPLAY=host.docker.internal:0`).

Backend environment variables (`MONGO_USER`, `MONGO_PASSWORD`, etc.) are in `.env` — do not commit changes to it. See `.env.example` for the expected keys.

### MongoDB Backup

To preserve data before the server is decommissioned, run the backup script **on the server** (outside Docker, with the stack running):

```bash
python scripts/backup_mongo.py
```

It reads credentials from `.env`, calls `mongodump` against `127.0.0.1:27018`, and writes the dump to `backup/mongodump_<timestamp>/`. Restore with `mongorestore` using the same credentials.

**Do not commit dumps.** `backup/` is git-ignored: the `players` collection contains visitor nicknames and short recovery-code hashes (4-char codes — trivially brute-forceable from their SHA-256), which must not appear in the public repository, and `epoch_highlights` is ~96 MB of derived data that is fully reproducible from `submissions` (matches are deterministic, ADR-0032). Instead, pack the dump (`tar -czf biotope-mongodump-<date>.tar.gz -C backup mongodump_<timestamp>`) and store it privately outside the repo. A dump from 2026-06-27 is preserved at `~/dev/biotope-mongodump-20260627.tar.gz`.

## Testing

**C unit tests** — compiled and run directly:
```bash
# Example: run core types test
gcc tests/test_core_types.c src/core/game_logic.c src/core/core_types.c \
    src/vendor/cJSON/cJSON.c -Isrc/core -Isrc/vendor/cJSON -o tests/test_core_types && \
    ./tests/test_core_types
```
Test sources live in `tests/test_*.c`. Compiled test binaries (no extension) are pre-built there too.

**Python backend tests** — run directly (no test runner needed). Each `backend/tests/test_*.py` file is self-running, e.g.:
```bash
python backend/tests/test_ranking.py
python backend/tests/test_validators.py
python backend/tests/test_duel_room.py
python backend/tests/test_admin_service.py
python backend/tests/test_system_integration.py  # requires built binaries
```
See `backend/tests/` for the full list (duel, admin, epoch-guard, match-cache, and name-claiming suites are also covered).

**Backend integration tests** require `make` to have been run first (they invoke the C binaries).

**Python lint** — config lives in the root `pyproject.toml` (black, line-length 88; ruff with `E`/`F`/`W`/`I` rules), scoped to `backend/`:
```bash
black --check backend/
ruff check backend/
```

## CI (`.github/workflows/ci.yml`)

Runs on push/PR to `biotop`. Four jobs, all of which reflect the canonical dev commands above:
- `build-c` — `make build/biotope_headless build/biotope_hyper_worker` (the GUI binary is skipped in CI; it needs a display).
- `test-c` — compiles and runs `tests/test_core_types.c` and `tests/test_file_io.c` directly with `gcc` (not via `make`).
- `test-backend` — runs `test_ranking.py` and `test_validators.py`.
- `lint-python` — `black --check backend/` and `ruff check backend/`.

## Architecture

### C Application (Single-player / Interactive)

The app is a **state machine** driven by `AppState` (defined in `src/core/core_types.h`). The main loop in `src/apps/gui/main.c` calls three functions each frame: poll input → `update_app_state` → render.

Key states: `STATE_CONFIG` → `STATE_EDIT_RED` → `STATE_EDIT_BLUE` → `STATE_IGNITION` → `STATE_RUNNING` → `STATE_FINISHED`. `STATE_KIOSK_MODE` is a parallel exhibition mode.

| Layer | Files | Responsibility |
|-------|-------|----------------|
| Core | `src/core/game_logic.c`, `core_types.h`, `config.h` | GOL rules, `World` struct, double-buffered grid |
| GUI | `src/gui/renderer.c`, `app_state_manager.c` | Raylib rendering (GPU texture upload), state transitions |
| I/O | `src/io/file_io.c`, `network_io.c` | JSON persistence (cJSON), async libcurl + pthread API calls |
| Apps | `src/apps/gui/`, `headless/`, `hyper/` | Entry points for each binary variant |
| Vendor | `src/vendor/cJSON/` | Embedded cJSON library |

**Grid representation:** `World.grid` is a flat `int*` in row-major order with a 1-cell ghost border. Cells are `DEAD(0)`, `TEAM_RED(1)`, or `TEAM_BLUE(2)`. Two `World` instances are ping-ponged each generation (double buffering). `World.chunk_map` is used for spatial partitioning to skip dead regions.

**Kiosk mode** (`STATE_KIOSK_MODE`): `KioskController` (in `app_state_manager.h`) drives a 15 s/30 s display cycle between a leaderboard view and a 2×2 Multicam grid showing four simultaneous live matches. It holds four `SimulationContext` + four `RenderContext` instances. API data is fetched asynchronously via `network_io.c`.

**Render budget tuning (ADR-0033):** `KIOSK_TARGET_FPS` in `src/core/config.h` is the single per-device knob for the GUI frame cap (committed default **30** for cool office-laptop operation; raise to 60 for smoother motion). The fossil-trail fade is frame-rate-independent (`pow(0.95, dt·120)`), so trail length is identical at any cap. Per-quadrant ping-pong FBOs are capped at ≤1080p (`KIOSK_MAX_FBO_HEIGHT`) and upscaled — a no-op on ≤1080p panels.

**`SessionOrigin`** tracks how the current session started (`ORIGIN_INTERACTIVE` vs. `ORIGIN_KIOSK_REPLAY`) to enable correct back-navigation (ADR-0020).

### Backend (Multiplayer Tournament)

- **FastAPI** (`backend/app/main.py`): Accepts pattern submissions at `POST /api/v1/submit_config`, serves `/api/leaderboard` and `/api/epoch/highlights`. Mounts `duel_router` and `admin_router` (see below).
- **MongoDB** (`backend/app/database.py`): Collections `submissions`, `players`, `epoch_highlights`, plus `match_results` (deterministic match cache) and `worker_state` (epoch guard) — see ADR-0032.
- **Python worker** (`backend/app/worker.py`): Runs every 60 s. **Incremental** (ADR-0032): it skips entirely when the active roster is unchanged (fingerprint guard), otherwise computes only the *uncached* seed-pairs via the C worker's pairings mode, upserts them into `match_results`, and aggregates the ranking + highlights from the cache. The optimisation relies on matches being deterministic (determinism tripwire at the cache boundary).
- **C Hyper-Worker** (`src/apps/hyper/main_hyper.c`): O(N²) round-robin tournament with OpenMP parallelisation. Reads a batch JSON, runs all pairings, outputs rankings + highlight seeds. When the batch includes an optional `pairings` array (ADR-0032), it instead computes only those pairs and emits a per-pair `match_results` array.
- **Shoulder-Duel mode** (`backend/app/duel_router.py` + `duel_service.py`, ADR-0028): two players share a screen, each editing one half of an 8×8 grid; `POST /duel/rooms` creates a room, `/duel/rooms/{id}/join`, `/lock`, and `/rematch` drive the room state machine, and the referee match is run via `biotope_headless` (needs `libgomp1`+`libcurl4` in the container — see ADR-0028 runtime notes).
- **Admin config deletion** (`backend/app/admin_router.py` + `admin_service.py`, ADR-0035): password-gated `POST /admin/delete` soft-removes submissions, `/admin/removed` lists them, `/admin/restore` undoes a deletion.
- Supporting modules: `validators.py` (submission validation), `ranking.py` (leaderboard aggregation), `grid_utils.py` (8×8 pattern helpers), `auth_utils.py` / `admin_auth.py` (recovery-code and admin-password auth), `storage.py` (misc persistence helpers).

### Web Editor

`web/editor/editor.html` — standalone HTML/JS page for participants to draw their 8×8 starting pattern and POST it to the backend.

## Coding Style (binding rules from `docs/CODING_STYLE.md`)

- **Language:** All code, comments, and variable names in **English**.
- **AI attribution:** Every function or block written/significantly modified by an AI agent **must** include `// KI-Agent unterstützt` as a comment.
- **Naming:** `snake_case` for variables/functions, `PascalCase` for structs/types, `UPPER_SNAKE_CASE` for constants and macros.
- **Indentation:** 4 spaces, no trailing whitespace.
- **Memory:** Always pair `malloc`/`calloc` with `free`. No dynamic allocation inside the hot simulation loop. Use `create_world` / `free_world` constructor/destructor pattern.
- **Refactoring workflow:** Before renaming any symbol, grep project-wide for all references and update atomically. Recompile with `make` after every change.

## Collaborative Working Mode (Team Principle)

Claude and the developer work as a team. This has concrete consequences for how verification is done:

- **"Interactive Test" means exactly that:** When a `DEV_TASKS` step is marked `Verification (Interactive Test)`, Claude defines precisely *what* to look for and *how* to navigate there. The developer then runs the app, observes the result with their own eyes, and reports back. Claude does **not** attempt to capture the screen autonomously.
- **Claude's role in interactive tests:** Formulate clear, numbered observation instructions ("Start the app, press [K], wait 3 seconds, report what you see in the top bar of each quadrant"). Then wait for the report.
- **Developer's role:** Run the app, follow the navigation steps, report honestly what is visible — including unexpected behaviour.
- **Autonomous screenshot attempts are a last resort**, not a first instinct. The Raylib GUI runs with OpenGL on a local display; the developer is sitting in front of it. Trust that.

## Documentation Conventions (`docs/DEVELOPMENT_GUIDELINES.md`)

For any non-trivial change, the expected workflow is:
1. Create or update an **ADR** in `docs/adr/` for architectural decisions.
2. Create a **DEV_TASKS** checklist in `docs/tasks/` (Markdown checkboxes).
3. Update `docs/CHANGELOG.md` upon completion.

ADRs are numbered sequentially (`ADR-0021-...`). Tech design docs live in `docs/tech_design/`.

