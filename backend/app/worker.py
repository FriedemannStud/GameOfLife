import asyncio
import logging
import os
import sys
import json
import tempfile
from datetime import datetime
from bson import ObjectId

# Path management MUST be first
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import get_db

# KI-Agent unterstützt: Epoch-based Tournament Worker

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("epoch_worker")

async def execute_epoch(db):
    """
    Executes a full tournament epoch using the C-Hyper-Worker.
    """
    logger.info("Starting Tournament Epoch...")
    
    # 1. Fetch all active submissions
    # We use a large length limit for the university exhibition (up to 1000)
    submissions = await db.submissions.find({"status": "active"}).to_list(length=1000)
    
    if len(submissions) < 2:
        logger.info(f"Not enough submissions for a tournament (Found: {len(submissions)}). Skipping.")
        return

    # 2. Prepare input batch
    batch_input = {
        "max_generations": 1000,
        "competitors": []
    }
    
    for s in submissions:
        batch_input["competitors"].append({
            "player_id": str(s["_id"]),
            "cells": s["config"]["cells"]
        })

    # Create temp files
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f_in:
        json.dump(batch_input, f_in)
        input_path = f_in.name
    
    output_path = input_path + ".out.json"

    try:
        # 3. Run biotope_hyper_worker
        # KI-Agent unterstützt: Check in multiple locations for Docker/Local flexibility
        possible_paths = ["./build/biotope_hyper_worker", "/app/build/biotope_hyper_worker", "./biotope_hyper_worker"]
        binary_path = next((p for p in possible_paths if os.path.exists(p)), None)
        
        if not binary_path:
            logger.error(f"Binary not found. Checked: {possible_paths}. Did you run 'make build/biotope_hyper_worker'?")
            return

        process = await asyncio.create_subprocess_exec(
            binary_path,
            input_path,
            output_path,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )

        stdout, stderr = await process.communicate()

        if process.returncode != 0:
            logger.error(f"Hyper-Worker failed: {stderr.decode()}")
            return

        # 4. Parse results and Update Database
        if not os.path.exists(output_path):
            logger.error("Output file from Hyper-Worker missing.")
            return

        with open(output_path, "r") as f_out:
            results = json.load(f_out)

        logger.info(f"Epoch finished. Matches played: {results['total_matches_played']}")

        # Store Highlights
        epoch_id = f"epoch_{int(datetime.utcnow().timestamp())}"
        highlights_data = []
        for h in results.get("highlights", []):
            highlights_data.append({
                "metric_type": "activity_sum",
                "red_name": h["red_name"],
                "blue_name": h["blue_name"],
                "red_seed": h["red_seed"],
                "blue_seed": h["blue_seed"],
                "metric_value": h["metric_value"]
            })
        
        if highlights_data:
            await db.epoch_highlights.insert_one({
                "epoch_id": epoch_id,
                "timestamp": datetime.utcnow(),
                "highlights": highlights_data
            })
            logger.info(f"Stored {len(highlights_data)} highlights for {epoch_id}")

        # KI-Agent unterstützt: Epoch-fresh ranking — no historical Elo, full round-robin data resets each epoch
        for position, rank in enumerate(results["rankings"], start=1):
            submission = next((s for s in submissions if str(s["_id"]) == rank["player_id"]), None)

            await db.submissions.update_one(
                {"_id": ObjectId(rank["player_id"])},
                {
                    "$set": {
                        "rank": position,
                        "win_rate": rank["win_rate"],
                        "wins": rank["wins"],
                        "draws": rank["draws"],
                        "losses": rank["losses"],
                        "total_score": rank["total_score"],
                        "matches_played": rank["matches_played"],
                        "avg_stable_generation": rank["avg_stable_generation"],
                        "last_epoch_at": datetime.utcnow()
                    }
                }
            )

            if submission:
                await db.players.update_one(
                    {"nickname": submission["metadata"]["nickname"]},
                    {
                        "$set": {
                            "rank": position,
                            "win_rate": rank["win_rate"],
                            "wins": rank["wins"],
                            "draws": rank["draws"],
                            "losses": rank["losses"],
                            "avg_stable_generation": rank["avg_stable_generation"],
                            "matches_played": rank["matches_played"],
                        }
                    }
                )

        logger.info("Database updated with Epoch results.")

    except Exception as e:
        logger.error(f"Error during Epoch execution: {e}")
    finally:
        # Cleanup
        if os.path.exists(input_path):
            os.remove(input_path)
        if os.path.exists(output_path):
            os.remove(output_path)

async def worker_loop():
    logger.info("Biotope Epoch Worker started.")
    from app.database import get_db, check_connection
    if not await check_connection():
        logger.error("Could not connect to MongoDB. Worker exiting.")
        return
    db = get_db()
    while True:
        try:
            await execute_epoch(db)
            # Sleep for 60 seconds until the next epoch
            await asyncio.sleep(60)
        except Exception as e:
            logger.error(f"Fatal error in worker loop: {e}")
            await asyncio.sleep(10)

if __name__ == "__main__":
    asyncio.run(worker_loop())
