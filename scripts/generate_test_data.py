import asyncio
import os
import random
from datetime import datetime
from dotenv import load_dotenv
from motor.motor_asyncio import AsyncIOMotorClient

# KI-Agent unterstützt: Generate test data for MongoDB Atlas

load_dotenv()

MONGODB_URI = os.getenv("MONGODB_URI")
DB_NAME = os.getenv("MONGODB_DB", "biotope_db")

async def generate_data():
    if not MONGODB_URI:
        print("Error: MONGODB_URI not set in .env")
        return

    print(f"Connecting to MongoDB Atlas...")
    try:
        # Use tlsAllowInvalidCertificates if necessary for some environments, 
        # but the main issue is usually IP Whitelist.
        client = AsyncIOMotorClient(MONGODB_URI, serverSelectionTimeoutMS=5000)
        db = client[DB_NAME]
        
        # Ping check
        await db.command("ping")
        print(f"Connection to '{DB_NAME}' successful!")
    except Exception as e:
        print(f"CRITICAL: Connection failed: {e}")
        print("\nPRO-TIP: Check your Atlas IP Access List (Whitelist) and ensure 0.0.0.0/0 is allowed for testing.")
        return
    
    # 1. Clear existing test data (optional, but good for clean state)
    # await db.players.delete_many({})
    # await db.submissions.delete_many({})
    # await db.epoch_highlights.delete_many({})

    # 2. Create mock players
    player_names = ["VibeMaster", "PixelWizard", "NeoConway", "LifeHacker", "GridLord"]
    players = []
    
    for name in player_names:
        player_id = f"user_{name.lower()}"
        player = {
            "player_id": player_id,
            "nickname": name,
            "elo_rating": random.randint(1100, 1600),
            "matches_played": random.randint(5, 50),
            "win_count": random.randint(2, 25),
            "created_at": datetime.utcnow()
        }
        await db.players.update_one({"player_id": player_id}, {"$set": player}, upsert=True)
        players.append(player)
        print(f"Upserted player: {name}")

    # 3. Create mock submissions (8x8 seeds)
    for p in players:
        # Create a simple random 8x8 pattern
        cells = []
        for r in range(8):
            for c in range(8):
                if random.random() < 0.3:
                    cells.append((c, r))
        
        submission = {
            "metadata": {
                "player_id": p["player_id"],
                "nickname": p["nickname"],
                "league": "local"
            },
            "config": {
                "bounding_box_x": 8,
                "bounding_box_y": 8,
                "cells": cells
            },
            "status": "active",
            "elo_rating": p["elo_rating"],
            "matches_played": p["matches_played"],
            "created_at": datetime.utcnow()
        }
        await db.submissions.insert_one(submission)
        print(f"Created submission for {p['nickname']}")

    # 4. Create a mock highlight
    epoch_id = f"epoch_mock_{int(datetime.utcnow().timestamp())}"
    highlights = []
    for i in range(2):
        h = {
            "metric_type": "activity_sum",
            "red_name": random.choice(player_names),
            "blue_name": random.choice(player_names),
            "red_seed": [random.randint(0, 1) for _ in range(64)],
            "blue_seed": [random.randint(0, 1) for _ in range(64)],
            "metric_value": random.randint(500, 5000)
        }
        highlights.append(h)
    
    await db.epoch_highlights.insert_one({
        "epoch_id": epoch_id,
        "timestamp": datetime.utcnow(),
        "highlights": highlights
    })
    print(f"Created mock highlights for {epoch_id}")

    print("--- Test Data Generation Complete ---")

if __name__ == "__main__":
    asyncio.run(generate_data())
