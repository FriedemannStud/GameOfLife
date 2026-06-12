# Docker Services – Biotop

This page describes each Docker container in the project: what it does, what it depends on, and when it runs.

---

## Overview

```
                    ┌─────────────────────────────────────┐
                    │         Docker network               │
                    │                                      │
  Browser ──────────┤──► backend (Port 8000) ──────────► mongo
  (Editor/API)      │         FastAPI                   MongoDB
                    │            │                         ▲
                    │            │ writes Elo              │
                    │            ▼                         │
                    │       matchmaker ────────────────────┘
                    │   (tournaments every 60 s)
                    │
                    │       c-dev  (local only)
                    │   (build environment for C code)
                    └─────────────────────────────────────┘

  build/biotope  ──────────────────────────────────────────►  GUI display
  (Raylib, runs directly                                       (kiosk, requires
  on host/laptop)                                              display/OpenGL)
```

---

## The Four Services

### `mongo` – Database

| Property | Value |
|----------|-------|
| Image | `mongo:4.4` |
| Port (internal) | `27017` (Docker network only) |
| Port (host) | `127.0.0.1:27018` → accessible from local machine only |
| Data | Volume `mongo_data` (persists even when the container stops) |
| Required by | `backend`, `matchmaker` |

**What it does:** Stores all persistent project data:
- submitted player patterns (`submissions`)
- Elo ratings and player profiles (`players`)
- highlight replays of past tournament epochs (`epoch_highlights`)

**Dependencies:** None — it starts first. `backend` and `matchmaker` wait via `healthcheck` until MongoDB is ready before starting.

**When needed:** Always, when the app is running (locally and on the server).

---

### `backend` – API Server

| Property | Value |
|----------|-------|
| Image | `python:3.11-slim` |
| Port | `8000` (exposed externally) |
| Volumes | `./backend` → `/app`, `./build` → `/app/build`, `./web` → `/app/web` |
| Starts after | `mongo` (waits for `service_healthy`) |

**What it does:**
- Provides the REST API: `POST /api/v1/submit_config`, `GET /api/leaderboard`, etc.
- Serves the web editor as a static file (`/editor/editor.html`)
- Connects to MongoDB for all read and write operations

**Why is `./build` mounted?**  
The backend volume `./build:/app/build` is a remnant for potential direct binary calls and integration tests (e.g. `backend/tests/test_system_integration.py`) that call `./build/biotope_hyper_worker` directly.

**When needed:** Always — it is the heart of the multiplayer mode.

---

### `matchmaker` – Tournament Worker

| Property | Value |
|----------|-------|
| Image | `python:3.11-slim` |
| Volumes | `./backend` → `/app`, `./worker_bin` → `/app/build` |
| Starts after | `mongo` (waits for `service_healthy`) |
| Runtime | Infinite loop, one tournament epoch every 60 seconds |

**What it does:**
1. Fetches all active player submissions from MongoDB
2. Serialises them into a temporary JSON file
3. Calls `/app/build/biotope_hyper_worker` (the C program for the simulation)
4. Reads the results and updates Elo ratings in MongoDB

**Why `./worker_bin` instead of `./build`?**  
`build/` is created by the `c-dev` container as `root`. If Docker creates the directory before `make`, the user can no longer write to it afterwards. `worker_bin/` is always created with the correct owner (`fried`) — either via `git clone` (through `.gitkeep`) or via `make`.  
Once `make build/biotope_hyper_worker` is run, the Makefile automatically copies the binary to `worker_bin/`.

**When needed:** Only in multiplayer mode (locally for testing or on the server).

---

### `c-dev` – Build Container

| Property | Value |
|----------|-------|
| Image | Built from local `Dockerfile` |
| Volumes | `.` → `/app` (complete repo) |
| Port | none |
| Display | `DISPLAY=host.docker.internal:0` (X11 forwarding) |

**What it does:**  
Provides a complete C development environment (gcc, make, Raylib, Emscripten) without needing to install these tools locally. You can work in it interactively:

```bash
docker compose exec c-dev bash
# You are now inside the container:
make
./build/biotope
```

**When needed:** Only locally during development, when you have no Raylib installation on the host. On the server it does nothing and can be ignored.

---

## Dependencies at a Glance

```
mongo ◄─── backend    (backend only starts when mongo is healthy)
mongo ◄─── matchmaker (matchmaker only starts when mongo is healthy)

backend    ──► ./build/         (reads binaries for integration tests)
matchmaker ──► ./worker_bin/    (reads biotope_hyper_worker for simulations)
c-dev      ──► ./               (reads + writes the entire repo)
```

---

## Usage Scenarios

| Scenario | mongo | backend | matchmaker | c-dev |
|----------|-------|---------|------------|-------|
| Local development (testing simulation) | ✓ | ✓ | ✓ | optional |
| Server deployment (university kiosk) | ✓ | ✓ | ✓ | — |
| Build C code only (no backend needed) | — | — | — | ✓ |
| Try editor in browser | ✓ | ✓ | — | — |

---

## Important Startup Order

```bash
# 1. Build binaries — ALWAYS first
make build/biotope_hyper_worker   # server (no Raylib required)
make                               # local (with Raylib, builds everything)

# 2. Start containers — AFTERWARDS
docker compose up -d
```

**Why this order?**  
If `docker compose up` is started before `make`, Docker creates the `worker_bin/` directory as `root` (if it does not already exist), and `make` can no longer write to it afterwards. The `.gitkeep` file in the repo ensures that `worker_bin/` already exists as a user-owned directory after `git clone` — making this order robust, but it does not hurt to follow it anyway.

---

## Useful Commands

```bash
docker compose ps                   # status of all containers
docker compose logs -f matchmaker   # live logs of the tournament worker
docker compose logs -f backend      # live logs of the API server
docker compose restart matchmaker   # restart a single service
docker compose down                 # stop everything (data is preserved)
docker compose down -v              # stop everything + delete database
```
