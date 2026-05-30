from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from datetime import datetime
from .models import Submission, DBSubmission, Player
from .validators import validate_biotope_rules
from .database import get_db

# KI-Agent unterstützt: Biotope Backend with MongoDB integration for Matchmaking

app = FastAPI(title="Biotope API")

# KI-Agent unterstützt: Enable CORS for Mobile Web Editor
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"], # Allow all origins for MVP / Mobile access
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.on_event("startup")
async def startup_db_client():
    from .database import check_connection
    if await check_connection():
        print("Successfully connected to MongoDB Atlas!")
    else:
        print("CRITICAL: Could not connect to MongoDB Atlas. Check your .env file.")


@app.get("/")
async def root():
    return {"message": "Hello Biotope"}


@app.post("/api/v1/submit_config", status_code=201)
async def submit_config(submission: Submission):
    try:
        # 1. Validate Business Rules (Fair Play)
        validate_biotope_rules(submission)

        # 2. Get DB connection
        db = get_db()

        # 3. Upsert Player (Ensure player exists)
        player_data = Player(
            player_id=submission.metadata.player_id,
            nickname=submission.metadata.nickname,
        )
        # KI-Agent unterstützt: Keyed by nickname for test phase; swap to player_id once email auth is added
        await db.players.update_one(
            {"nickname": player_data.nickname},
            {
                "$set": {"player_id": player_data.player_id},
                "$setOnInsert": {
                    "nickname": player_data.nickname,
                    "elo_rating": 1200,
                    "matches_played": 0,
                    "win_rate": 0.0,
                    "created_at": datetime.utcnow(),
                },
            },
            upsert=True,
        )

        # 4. Insert Submission
        db_submission = DBSubmission(**submission.model_dump())
        result = await db.submissions.insert_one(db_submission.model_dump())

        return {
            "status": "success",
            "submission_id": str(result.inserted_id),
            "timestamp": datetime.utcnow().isoformat(),
        }
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))
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
        # Fetch top 20 players by Elo
        players = (
            await db.players.find({})
            .sort("elo_rating", -1)
            .limit(20)
            .to_list(length=20)
        )

        leaderboard = []
        for p in players:
            leaderboard.append(
                {"name": p["nickname"], "elo": p["elo_rating"], "win_rate": p.get("win_rate", 0.0)}
            )

        return {"leaderboard": leaderboard}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Leaderboard error: {str(e)}")


@app.get("/api/epoch/highlights")
async def get_epoch_highlights():
    try:
        db = get_db()
        # Fetch the most recent epoch highlight document
        highlight_doc = (
            await db.epoch_highlights.find({}).sort("timestamp", -1).limit(1).to_list(length=1)
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
