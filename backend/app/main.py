from fastapi import FastAPI, HTTPException
from datetime import datetime
from .models import Submission, DBSubmission, Player
from .validators import validate_biotope_rules
from .database import get_db

# KI-Agent unterstützt: Biotope Backend with MongoDB integration for Matchmaking

app = FastAPI(title="Biotope API")


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
        await db.players.update_one(
            {"player_id": player_data.player_id},
            {
                "$set": {"nickname": player_data.nickname},
                "$setOnInsert": {
                    "elo_rating": 1200,
                    "matches_played": 0,
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
