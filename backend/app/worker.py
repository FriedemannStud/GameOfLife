import asyncio
import logging
import os
import sys
import json
import tempfile
from datetime import datetime

# Path management MUST be first
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import get_db
from app.ranking import calculate_elo

# KI-Agent unterstützt: Background worker with headless simulation integration

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("matchmaker")


async def execute_match(db, a, b):
    """
    Executes a match between two submissions using the headless C-worker.
    """
    logger.info(
        f"Executing match: {a['metadata']['nickname']} vs {b['metadata']['nickname']}"
    )

    # Prepare JSON serializable copies (convert ObjectId to string)
    a_serializable = json.loads(json.dumps(a, default=str))
    b_serializable = json.loads(json.dumps(b, default=str))

    # Create temp files for input
    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".json", delete=False
    ) as f_a, tempfile.NamedTemporaryFile(
        mode="w", suffix=".json", delete=False
    ) as f_b:

        json.dump(a_serializable, f_a)
        json.dump(b_serializable, f_b)
        path_a = f_a.name
        path_b = f_b.name

    try:
        # Run biotope_headless
        # Assuming the binary is in the root directory (one level up from app/)
        binary_path = "./biotope_headless"

        process = await asyncio.create_subprocess_exec(
            binary_path,
            path_a,
            path_b,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )

        stdout, stderr = await process.communicate()

        if process.returncode != 0:
            logger.error(f"Simulation failed: {stderr.decode()}")
            return

        # Parse output JSON (it's printed to stdout by biotope_headless)
        result_data = json.loads(stdout.decode())
        logger.info(f"Match Result: Winner={result_data['winner']}")

        # 1. Update Elo Ratings
        score_a = (
            1.0
            if result_data["winner"] == "red"
            else (0.5 if result_data["winner"] == "draw" else 0.0)
        )
        score_b = 1.0 - score_a

        new_elo_a = calculate_elo(
            a["elo_rating"], b["elo_rating"], score_a, a["matches_played"]
        )
        new_elo_b = calculate_elo(
            b["elo_rating"], a["elo_rating"], score_b, b["matches_played"]
        )

        elo_delta_a = new_elo_a - a["elo_rating"]

        # 2. Update Database (Atomic updates)
        # Update Submissions
        await db.submissions.update_one(
            {"_id": a["_id"]},
            {
                "$set": {"status": "active", "elo_rating": new_elo_a},
                "$inc": {"matches_played": 1},
            },
        )
        await db.submissions.update_one(
            {"_id": b["_id"]},
            {
                "$set": {"status": "active", "elo_rating": new_elo_b},
                "$inc": {"matches_played": 1},
            },
        )

        # Update Global Player Ratings (Simplified: just update)
        await db.players.update_one(
            {"player_id": a["metadata"]["player_id"]},
            {"$set": {"elo_rating": new_elo_a}, "$inc": {"matches_played": 1}},
        )
        await db.players.update_one(
            {"player_id": b["metadata"]["player_id"]},
            {"$set": {"elo_rating": new_elo_b}, "$inc": {"matches_played": 1}},
        )

        # 3. Log Match
        match_log = {
            "timestamp": datetime.utcnow(),
            "red_submission_id": a["_id"],
            "blue_submission_id": b["_id"],
            "winner": result_data["winner"],
            "red_population": result_data["red"]["population"],
            "blue_population": result_data["blue"]["population"],
            "elo_delta": elo_delta_a,
        }
        await db.matches.insert_one(match_log)
        logger.info(
            f"Match finalized. Elo A: {new_elo_a} ({elo_delta_a:+}), Elo B: {new_elo_b}"
        )

    except Exception as e:
        logger.error(f"Error during match execution: {e}")
        # Rollback status
        await db.submissions.update_many(
            {"_id": {"$in": [a["_id"], b["_id"]]}}, {"$set": {"status": "active"}}
        )
    finally:
        # Cleanup temp files
        if os.path.exists(path_a):
            os.remove(path_a)
        if os.path.exists(path_b):
            os.remove(path_b)


async def find_match_pair(db):
    target_a = await db.submissions.find_one_and_update(
        {"status": "active"},
        {"$set": {"status": "in_match"}},
        sort=[("matches_played", 1), ("created_at", 1)],
        return_document=True,
    )
    if not target_a:
        return None, None

    elo_a = target_a["elo_rating"]
    player_id_a = target_a["metadata"]["player_id"]

    target_b = await db.submissions.find_one_and_update(
        {
            "status": "active",
            "metadata.player_id": {"$ne": player_id_a},
            "elo_rating": {"$gte": elo_a - 150, "$lte": elo_a + 150},
        },
        {"$set": {"status": "in_match"}},
        sort=[("matches_played", 1)],
        return_document=True,
    )

    if not target_b:
        await db.submissions.update_one(
            {"_id": target_a["_id"]}, {"$set": {"status": "active"}}
        )
        return None, None

    return target_a, target_b


async def matchmaking_loop():
    logger.info("Matchmaker Service started.")
    db = get_db()
    while True:
        try:
            a, b = await find_match_pair(db)
            if a and b:
                await execute_match(db, a, b)
            else:
                await asyncio.sleep(5)
        except Exception as e:
            logger.error(f"Fatal error in loop: {e}")
            await asyncio.sleep(10)


if __name__ == "__main__":
    asyncio.run(matchmaking_loop())
