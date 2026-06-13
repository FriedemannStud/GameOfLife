# LLM Session Log — Backend Startup Crash Fix (MongoDB Index)

**Date:** 2026-06-13
**Branch:** `biotop`
**Scope:** Diagnose and fix a server-side backend startup crash that made the API unreachable (`curl` → connection reset / empty reply), plus a deployment robustness improvement.

---

## Symptom

After redeploying the server stack per `docs/DEPLOYMENT.md` (Part 5 verification), the backend was unreachable:

```bash
docker compose up -d
curl http://localhost:8000/
# curl: (56) Recv failure: Connection reset by peer
```

`docker compose ps` reported all containers `Up`, MongoDB `healthy`. The failure looked like a healthy stack — but the API never answered.

---

## Diagnostic Path

The investigation isolated the fault layer by layer:

1. **Code change review** — The recent codebase cleanup had removed dead DB variables (`results_col`, `epoch_highlights_col`) from `database.py`. Confirmed via `grep` that nothing referenced them; `database.py` was clean. Not the cause.
2. **Meaning of the error** — `Connection reset by peer` (not `Connection refused`): Docker accepts the TCP connection on port 8000, but **no process inside the container is listening** → the connection is reset. Analogy: the mailbox is mounted, but nobody sits behind the door.
3. **Local reproduction** — Started the same stack locally. The app responded correctly **from inside the container** (`python urllib` → `{"message":"Hello Biotope"}`, `200 OK` in the access log). This proved the **application code and the MongoDB connection were healthy**.
4. **Port-forwarding surprise** — On the local WSL host, `curl localhost:8000` returned empty. `ss -tlnp` revealed port 8000 was held by **`ssh -L 8000:localhost:8000 uni`** (the deployment tunnel from Part 7, Option C), not Docker. All local `curl`s were silently forwarded to the **server**, never reaching the local container.
5. **Server logs (decisive)** — `docker compose logs backend --tail=30` on the server showed the real cause:

```
pymongo.errors.DuplicateKeyError: Index build failed ...
  E11000 duplicate key error collection: biotope_db.players
  index: nickname_normalized_1  dup key: { nickname_normalized: null }
ERROR:    Application startup failed. Exiting.
```

---

## Root Cause

The FastAPI startup event creates a **unique** index on `players.nickname_normalized` (introduced for name-claiming, ADR-0027):

```python
await get_db().players.create_index("nickname_normalized", unique=True)
```

The server's `players` collection still contained **legacy documents from before the name-claiming feature** — documents with no `nickname_normalized` field. When MongoDB builds a unique index, a missing field is treated as `null`. With **two or more** such legacy documents, the `null` values collide → `E11000` → the startup event raises → **uvicorn exits**. The container stays "up" (the shell/`--reload` parent lives), but nothing serves port 8000 → connection reset / empty reply.

This is why the symptom looked environmental (timing) but was deterministic: every restart crashed at the same point.

---

## Changes Applied

All changes were verified locally by reproducing the server scenario (injecting two legacy docs without `nickname_normalized`).

### 1. Partial Unique Index (`backend/app/main.py`)

Scoped uniqueness to **real string names** via `partialFilterExpression`, so legacy/null documents are excluded from the index and can no longer break the build. The ownership guarantee for actually claimed names is unchanged.

```python
await players.create_index(
    "nickname_normalized",
    unique=True,
    partialFilterExpression={"nickname_normalized": {"$type": "string"}},
)
```

### 2. In-Place Index Migration (`backend/app/main.py`)

Local testing surfaced a second edge case: an environment that had **already built the old plain unique index** would fail the upgrade with `IndexOptionsConflict` (code 85). The startup logic now drops and recreates the index as the partial variant in that case. Extracted into `_ensure_nickname_index(players)`; added `OperationFailure` to the `pymongo.errors` import.

```python
async def _ensure_nickname_index(players):
    def _create():
        return players.create_index(
            "nickname_normalized",
            unique=True,
            partialFilterExpression={"nickname_normalized": {"$type": "string"}},
        )
    try:
        await _create()
    except OperationFailure as e:
        if e.code == 85:  # IndexOptionsConflict: legacy plain unique index exists
            await players.drop_index("nickname_normalized_1")
            await _create()
        else:
            raise
```

### 3. Backend Healthcheck (`docker-compose.yml`)

Added a healthcheck so `docker compose ps` reports `Up (healthy)` only once uvicorn actually answers — closing the "Started ≠ serving" gap that caused the original confusion. The `python:3.11-slim` image has no `curl`, so the probe uses `urllib`; `start_period: 120s` covers the one-off `pip install` window.

```yaml
healthcheck:
  test: ["CMD", "python", "-c", "import urllib.request,sys; sys.exit(0 if urllib.request.urlopen('http://localhost:8000/').status == 200 else 1)"]
  interval: 10s
  timeout: 5s
  retries: 5
  start_period: 120s
```

### 4. Cosmetic Log Cleanup (`backend/app/main.py`, `database.py`)

Replaced stale `MongoDB Atlas` strings/comments with `MongoDB` (the stack uses a local Mongo container, not Atlas). The misleading log text had initially caused concern that the backend was talking to the cloud.

---

## Verification

Reproduced the exact server failure locally, then confirmed the fix:

- Injected 2 legacy docs without `nickname_normalized` → backend now starts cleanly (`Application startup complete`).
- Confirmed final index: `unique: True` **and** `partialFilterExpression: {nickname_normalized: {$type: string}}`.
- Confirmed plain→partial migration path runs without crashing.
- `docker compose ps` → backend `Up (healthy)`.
- Cleaned up injected test data.

**Server resolution:** The developer chose a hard reset of the (test-only) database for a clean start:

```bash
git pull
docker compose down -v     # drops the mongo_data volume — destroys all data
docker compose up -d
docker compose ps          # wait for backend "Up (healthy)"
curl http://localhost:8000/            # -> {"message":"Hello Biotope"}
curl http://localhost:8000/api/leaderboard  # -> []
```

Confirmed working: backend `Up (healthy)`, `curl` returns `Hello Biotope`.

---

## Lessons / Notes for Future Work

- **"Container Started" ≠ "app serving."** The healthcheck now makes this distinction visible. On any future odd `curl` failure, the first move is `docker compose logs <service>` — the stack trace (here the `E11000`) points straight at the cause.
- **Tunnel vs. local port.** On the WSL dev machine, `localhost:8000` is occupied by the SSH deployment tunnel (`ssh -L 8000:localhost:8000`), so local `curl`s hit the **server**, not a local container. To test a local stack, close the tunnel or remap the local port.
- **Schema migrations on existing collections.** Adding a unique index to a collection with pre-existing data must account for missing/null fields (partial filter) and for an index that may already exist with different options (code 85).

---

## Modified Files

- `backend/app/main.py` — partial unique index, `_ensure_nickname_index` migration helper, `OperationFailure` import, Atlas→MongoDB log text.
- `backend/app/database.py` — Atlas→MongoDB comment.
- `docker-compose.yml` — backend healthcheck.

(Commit handled by the developer.)
