#!/usr/bin/env python3
# KI-Agent unterstützt: One-shot poster helper — pull the top winning start
# patterns from MongoDB and render their 8x8 seeds as ASCII art (F2.3 / poster).
#
# Usage (host, after `docker-compose up`):
#     python3 backend/scripts/top_patterns.py            # top 5
#     python3 backend/scripts/top_patterns.py 10         # top 10
#
# Reads MONGODB_URI / MONGODB_DB from the repo-root .env. The URI in .env points at
# the docker-internal host `mongo:27017`; from the host MongoDB is published on
# 127.0.0.1:27018, so we rewrite that automatically unless TOP_PATTERNS_URI is
# set to override the connection string explicitly.

import os
import sys

from dotenv import load_dotenv
from pymongo import MongoClient


# Same sparse-cells -> dense 8x8 convention as backend/app/grid_utils.py
def cells_to_grid(cells):
    grid = [0] * 64
    for cell in cells or []:
        if not cell or len(cell) < 2:
            continue
        x, y = int(cell[0]), int(cell[1])
        if 0 <= x < 8 and 0 <= y < 8:
            grid[y * 8 + x] = 1
    return grid


def render_ascii(grid):
    rows = []
    for r in range(8):
        rows.append("".join("█" if grid[r * 8 + c] else "·" for c in range(8)))
    return rows


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.join(here, "..", "..")
    load_dotenv(os.path.join(repo_root, ".env"))

    limit = int(sys.argv[1]) if len(sys.argv) > 1 else 5

    uri = os.getenv("TOP_PATTERNS_URI") or os.getenv("MONGODB_URI")
    if not uri:
        sys.exit("MONGODB_URI not set in backend/.env")
    # Rewrite docker-internal host -> host-published port
    uri = uri.replace("@mongo:27017", "@127.0.0.1:27018")
    db_name = os.getenv("MONGODB_DB", "biotope_db")

    client = MongoClient(uri, serverSelectionTimeoutMS=4000)
    db = client[db_name]

    # Same ranking filter/sort as GET /api/leaderboard (main.py)
    query = {"status": "active", "matches_played": {"$gt": 0}}
    total = db.submissions.count_documents(query)
    cursor = (
        db.submissions.find(query)
        .sort([("win_rate", -1), ("avg_stable_generation", 1)])
        .limit(limit)
    )

    print(f"\nTop {limit} winning start patterns (of {total} ranked submissions)\n")
    for rank, s in enumerate(cursor, start=1):
        name = s.get("metadata", {}).get("nickname", "?")
        wr = round(s.get("win_rate", 0.0) * 100, 1)
        w, d, lo = s.get("wins", 0), s.get("draws", 0), s.get("losses", 0)
        mp = s.get("matches_played", 0)
        stab = round(s.get("avg_stable_generation", 0.0), 1)
        grid = cells_to_grid(s.get("config", {}).get("cells", []))
        live = sum(grid)

        print(f"#{rank}  {name}")
        print(
            f"     win-rate {wr}%  (W{w}/D{d}/L{lo}, {mp} matches)  "
            f"avg-stable-gen {stab}  live-cells {live}/64"
        )
        for row in render_ascii(grid):
            print(f"     {row}")
        print()


if __name__ == "__main__":
    main()
