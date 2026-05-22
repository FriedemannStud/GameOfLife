import random
import requests
import asyncio
import os
import sys
from datetime import datetime

# Path management
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from app.database import get_db
from app.worker import find_match_pair, execute_match

# KI-Agent unterstützt: Automated test for diverse match outcomes

API_URL = "http://localhost:8000/api/v1/submit_config"

def generate_random_pattern():
    # 38% of 64 is 24.32 -> Max 24 cells
    cell_count = random.randint(5, 24)
    all_coords = [(x, y) for x in range(8) for y in range(8)]
    cells = random.sample(all_coords, cell_count)
    return cells

async def run_automation():
    db = get_db()
    wins = {"red": 0, "blue": 0, "draw": 0}
    
    print("Starting automated test for diverse outcomes...")
    
    iteration = 1
    while wins["red"] == 0 or wins["blue"] == 0:
        print(f"\n--- Iteration {iteration} ---")
        
        # 1. Submit for Player A
        p1 = {
            "metadata": {"player_id": "auto_test_a", "nickname": "Bot_Alpha"},
            "config": {"bounding_box_x": 8, "bounding_box_y": 8, "cells": generate_random_pattern()}
        }
        r1 = requests.post(API_URL, json=p1)
        
        # 2. Submit for Player B
        p2 = {
            "metadata": {"player_id": "auto_test_b", "nickname": "Bot_Beta"},
            "config": {"bounding_box_x": 8, "bounding_box_y": 8, "cells": generate_random_pattern()}
        }
        r2 = requests.post(API_URL, json=p2)
        
        if r1.status_code != 201 or r2.status_code != 201:
            print(f"Error submitting: {r1.text} | {r2.text}")
            break
            
        print("Submissions successful. Finding match...")
        
        # 3. Run Matchmaker logic
        a, b = await find_match_pair(db)
        if a and b:
            # We wrap execute_match logic to see the result here
            # (In worker.py, execute_match prints to logger)
            await execute_match(db, a, b)
            
            # Check last match result
            last_match = await db.matches.find().sort("timestamp", -1).limit(1).to_list(1)
            if last_match:
                winner = last_match[0]["winner"]
                wins[winner] += 1
                print(f"Iteration Result: {winner.upper()} won!")
        else:
            print("No pair found by matchmaker.")
            
        iteration += 1
        if iteration > 20: # Safety break
            print("Reached max iterations without diverse results.")
            break
            
        await asyncio.sleep(1)

    print("\n--- Final Statistics ---")
    print(f"Red Wins:  {wins['red']}")
    print(f"Blue Wins: {wins['blue']}")
    print(f"Draws:     {wins['draw']}")
    print("Test finished successfully!")

if __name__ == "__main__":
    asyncio.run(run_automation())
