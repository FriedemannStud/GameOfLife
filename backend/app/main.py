from datetime import datetime

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from pymongo.errors import DuplicateKeyError, OperationFailure

from .auth_utils import (
    generate_recovery_code,
    hash_recovery_code,
    normalize_nickname,
)
from .database import get_db
from .duel_router import router as duel_router
from .grid_utils import cells_to_grid
from .models import DBSubmission, Player, Submission
from .validators import validate_biotope_rules

# KI-Agent unterstützt: Biotope Backend with MongoDB integration for Matchmaking

# KI-Agent unterstützt: Max ranked rows returned by /api/leaderboard. Must match
# the C client's MAX_LEADERBOARD_ENTRIES so paging capacity agrees end-to-end.
# `total_count` stays an independent server-side count, so the honest "+N more"
# overflow indicator remains correct beyond this cap (ADR-0030, ADR-0031).
LEADERBOARD_MAX_ROWS = 50

app = FastAPI(title="Biotope API")

# KI-Agent unterstützt: Enable CORS for Mobile Web Editor
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # Allow all origins for MVP / Mobile access
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.on_event("startup")
async def startup_db_client():
    from .database import check_connection

    if await check_connection():
        print("Successfully connected to MongoDB!")
        await _ensure_nickname_index(get_db().players)
        await _ensure_duel_indexes(get_db())
        await _ensure_match_cache_indexes(get_db())
    else:
        print("CRITICAL: Could not connect to MongoDB. Check your .env file.")


# KI-Agent unterstützt: Match-cache index (ADR-0032 Stage 2). The unique index on
# `pair_key` backs the content-addressed lookup of deterministic match outcomes
# and prevents duplicate cache rows under concurrent upserts.
async def _ensure_match_cache_indexes(db):
    await db.match_results.create_index("pair_key", unique=True)


# KI-Agent unterstützt: Shoulder-Duel indexes (ADR-0028). The TTL index on
# `expires_at` (expireAfterSeconds=0) lets MongoDB reap ephemeral rooms without a
# sweeper job; the unique `room_id` index backs the QR/room-code lookup. Per-player
# `duels` indexes keep the "My Duels" aggregation fast.
async def _ensure_duel_indexes(db):
    await db.duel_rooms.create_index("expires_at", expireAfterSeconds=0)
    await db.duel_rooms.create_index("room_id", unique=True)
    await db.duels.create_index("red.player_id")
    await db.duels.create_index("blue.player_id")


# KI-Agent unterstützt: Partial unique index is the authoritative ownership guard
# for normalized names, including under concurrent claims (ADR-0027). The
# partialFilterExpression scopes uniqueness to real string names, so legacy/null
# player docs (pre-claim schema) cannot break the index build. If an older plain
# unique index of the same name already exists, migrate it in place (IndexOptions-
# Conflict, code 85) by dropping and recreating it as the partial variant.
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
        if e.code == 85:
            await players.drop_index("nickname_normalized_1")
            await _create()
        else:
            raise


# KI-Agent unterstützt: serve the smartphone landing / title screen at the root,
# which links onward to the editor and the shoulder-duel page.
@app.get("/")
async def root():
    return FileResponse("web/landing/index.html")


# KI-Agent unterstützt: Serve web editor as static files
app.mount("/editor", StaticFiles(directory="web/editor"), name="editor")

# KI-Agent unterstützt: Serve the Mission Statement (rules of Biotop) static page.
app.mount("/mission", StaticFiles(directory="web/mission"), name="mission")

# KI-Agent unterstützt: Serve the Shoulder-Duel page and its self-hosted assets
# (soundtrack) as static files (ADR-0028).
app.mount("/duel", StaticFiles(directory="web/duel"), name="duel")
app.mount("/assets", StaticFiles(directory="web/assets"), name="assets")
# KI-Agent unterstützt: Vendored client libs (QR generator) for the duel page.
app.mount("/vendor", StaticFiles(directory="web/vendor"), name="vendor")

# KI-Agent unterstützt: Shoulder-Duel REST endpoints (ADR-0028)
app.include_router(duel_router)


# KI-Agent unterstützt: Persist metadata + config only — the `auth` block (recovery
# code) is structurally excluded so no model_dump() path leaks it (ADR-0027, FR-7).
async def _persist_submission(db, submission: Submission) -> str:
    db_submission = DBSubmission(metadata=submission.metadata, config=submission.config)
    result = await db.submissions.insert_one(db_submission.model_dump())
    return str(result.inserted_id)


@app.post("/api/v1/submit_config", status_code=201)
async def submit_config(submission: Submission):
    # KI-Agent unterstützt: Name-claiming with recovery code (ADR-0027). Ownership of a
    # normalized name is proven by either the silent player_id or the recovery code;
    # first claim wins, later devices reclaim via re-binding (password-reset semantics).
    try:
        # 1. Validate Business Rules (Fair Play) -> 400 on ValueError
        validate_biotope_rules(submission)

        db = get_db()
        norm = normalize_nickname(submission.metadata.nickname)
        player = await db.players.find_one({"nickname_normalized": norm})

        # 2a. Free name -> CLAIM: bind name to this device, issue a recovery code.
        if player is None:
            code = generate_recovery_code()
            new_player = Player(
                player_id=submission.metadata.player_id,
                nickname=submission.metadata.nickname,
                nickname_normalized=norm,
                recovery_code_hash=hash_recovery_code(code),
            )
            doc = new_player.model_dump()
            doc["win_rate"] = 0.0
            try:
                await db.players.insert_one(doc)
            except DuplicateKeyError:
                # Lost a concurrent claim race -> treat as taken by someone else.
                raise HTTPException(status_code=409, detail={"error": "name_taken"})
            submission_id = await _persist_submission(db, submission)
            return {
                "status": "success",
                "submission_id": submission_id,
                "timestamp": datetime.utcnow().isoformat(),
                "recovery_code": code,
            }

        # 2b. Silent owner -> same device, zero friction.
        if player["player_id"] == submission.metadata.player_id:
            submission_id = await _persist_submission(db, submission)
            return {
                "status": "success",
                "submission_id": submission_id,
                "timestamp": datetime.utcnow().isoformat(),
            }

        # 2c. Taken by someone else -> require the recovery code.
        code = submission.auth.recovery_code if submission.auth else None
        if not code:
            raise HTTPException(status_code=409, detail={"error": "name_taken"})
        if hash_recovery_code(code) != player.get("recovery_code_hash"):
            raise HTTPException(
                status_code=403, detail={"error": "invalid_recovery_code"}
            )

        # RECLAIM -> re-bind the name to the requesting device.
        await db.players.update_one(
            {"_id": player["_id"]},
            {"$set": {"player_id": submission.metadata.player_id}},
        )
        submission_id = await _persist_submission(db, submission)
        return {
            "status": "success",
            "submission_id": submission_id,
            "timestamp": datetime.utcnow().isoformat(),
            "reclaimed": True,
        }
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))
    except HTTPException:
        # Let the deterministic 409/403/400 contract propagate unchanged.
        raise
    except Exception:
        # In a real app, log the error 'e'
        raise HTTPException(status_code=500, detail="Internal server error")


@app.get("/api/v1/stats/count")
async def get_submission_count():
    try:
        db = get_db()
        count = await db.submissions.count_documents({})
        return {"count": count}
    except Exception:
        raise HTTPException(status_code=500, detail="Could not fetch count")


@app.get("/api/leaderboard")
async def get_leaderboard():
    try:
        db = get_db()
        # KI-Agent unterstützt: Per-configuration leaderboard read from
        # `submissions` (ADR-0025). Each active, already-ranked submission is its
        # own row with its own stats and seed icon. This avoids the per-nickname
        # last-write-wins aggregation on `players`, where a player's weakest
        # submission would overwrite the displayed stats.
        # KI-Agent unterstützt: count and list must share one filter so the
        # honest "+N more" total can never disagree with the returned rows (ADR-0030)
        query = {"status": "active", "matches_played": {"$gt": 0}}

        total_count = await db.submissions.count_documents(query)

        submissions = (
            await db.submissions.find(query)
            .sort([("win_rate", -1), ("avg_stable_generation", 1)])
            .limit(LEADERBOARD_MAX_ROWS)
            .to_list(length=LEADERBOARD_MAX_ROWS)
        )

        leaderboard = []
        for position, s in enumerate(submissions, start=1):
            leaderboard.append(
                {
                    "rank": position,
                    "name": s["metadata"]["nickname"],
                    "win_rate": round(s.get("win_rate", 0.0) * 100, 1),
                    "wins": s.get("wins", 0),
                    "draws": s.get("draws", 0),
                    "losses": s.get("losses", 0),
                    "avg_stable_generation": round(
                        s.get("avg_stable_generation", 0.0), 1
                    ),
                    # KI-Agent unterstützt: Dense 8x8 start configuration for
                    # the leaderboard icon (ADR-0025)
                    "seed": cells_to_grid(s.get("config", {}).get("cells", [])),
                }
            )

        # KI-Agent unterstützt: true server-side total for the honest "+N more"
        # overflow indicator; may exceed the LEADERBOARD_MAX_ROWS rows
        # returned (ADR-0030)
        return {"leaderboard": leaderboard, "total_count": total_count}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Leaderboard error: {str(e)}")


@app.get("/api/epoch/highlights")
async def get_epoch_highlights():
    try:
        db = get_db()
        # Fetch the most recent epoch highlight document
        highlight_doc = (
            await db.epoch_highlights.find({})
            .sort("timestamp", -1)
            .limit(1)
            .to_list(length=1)
        )

        if not highlight_doc:
            raise HTTPException(status_code=404, detail="No highlights found")

        doc = highlight_doc[0]
        return {
            "epoch_id": doc["epoch_id"],
            "timestamp": doc["timestamp"].isoformat(),
            "highlights": doc["highlights"],
        }
    except HTTPException:
        raise
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Highlights error: {str(e)}")


# KI-Agent unterstützt: Performance monitoring feed (ADR-0032). Serves the most
# recent tournament-performance metrics written by backend/scripts/perf_tournament.py
# into the `performance_metrics` collection. Powers the live ops dashboard during
# the fair (is any submitted pattern dragging match throughput down?) and doubles
# as a queryable history for later analysis.
@app.get("/api/performance")
async def get_performance(limit: int = 50):
    try:
        db = get_db()
        limit = max(1, min(limit, 500))
        docs = (
            await db.performance_metrics.find({})
            .sort("timestamp", -1)
            .limit(limit)
            .to_list(length=limit)
        )
        runs = []
        for d in docs:
            d.pop("_id", None)
            if isinstance(d.get("timestamp"), datetime):
                d["timestamp"] = d["timestamp"].isoformat()
            runs.append(d)
        latest = runs[0] if runs else None
        return {"latest": latest, "runs": runs, "count": len(runs)}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Performance error: {str(e)}")
