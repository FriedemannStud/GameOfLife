#!/usr/bin/env python3
# KI-Agent unterstützt: Faithful reproduction of the (uncommitted) "exhaust_engine"
# generator that filled the `biotope_study` database. The original tool is not in
# the repo/history; this rebuilds its document schema from the existing data:
#   metadata.player_id = "exhaust_engine"
#   metadata.nickname  = "exhaust#NNNNNNNN"   (8-digit, continued after current max)
#   metadata.league    = "study"
#   config.{bounding_box_x, bounding_box_y, cell_count, cells, cells_int,
#           next_gen_cells_int}
#   status = "active", elo_rating = 1200, created_at = now
# Each generated pattern is a random distinct-cell 8x8 seed. Bit index = y*8 + x
# (same convention as backend/app/grid_utils.py and the C grid_to_bitboard).
#
# Usage (host):
#   python3 backend/scripts/generate_study_configs.py 3000      # fill UP TO 3000 total
#   python3 backend/scripts/generate_study_configs.py 3000 --per-cellcount
#
# Reads MONGODB_URI / MONGODB_DB from repo-root .env (rewrites docker host -> 27018).

import os
import random
import sys
from datetime import datetime, timezone

from dotenv import load_dotenv
from pymongo import MongoClient

GRID = 8
MIN_CELLS = 2
MAX_CELLS = 24  # 38% of 64, matches the spread seen in the existing study data


def _connect():
    here = os.path.dirname(os.path.abspath(__file__))
    load_dotenv(os.path.join(here, "..", "..", ".env"))
    uri = os.getenv("TOP_PATTERNS_URI") or os.getenv("MONGODB_URI")
    if not uri:
        sys.exit("MONGODB_URI not set in repo-root .env")
    uri = uri.replace("@mongo:27017", "@127.0.0.1:27018")
    db_name = os.getenv("MONGODB_DB", "biotope_db")
    return MongoClient(uri, serverSelectionTimeoutMS=4000)[db_name], db_name


def cells_to_int(cells):
    # Signed 64-bit bitboard, bit index = y*8 + x (matches existing study docs)
    v = 0
    for x, y in cells:
        v |= 1 << (y * GRID + x)
    if v >= 1 << 63:  # emulate signed 64-bit storage as seen in the data
        v -= 1 << 64
    return v


def next_gen_int(cells):
    # Standard Conway step on the isolated 8x8 board (dead outside, no wrap).
    # Used only as metadata, mirroring the original next_gen_cells_int field.
    alive = set((x, y) for x, y in cells)
    nxt = []
    for y in range(GRID):
        for x in range(GRID):
            n = sum(
                (nx, ny) in alive
                for nx in (x - 1, x, x + 1)
                for ny in (y - 1, y, y + 1)
                if not (nx == x and ny == y)
            )
            if ((x, y) in alive and n in (2, 3)) or ((x, y) not in alive and n == 3):
                nxt.append((x, y))
    return cells_to_int(nxt)


def random_pattern():
    cell_count = random.randint(MIN_CELLS, MAX_CELLS)
    coords = [(x, y) for x in range(GRID) for y in range(GRID)]
    return random.sample(coords, cell_count)


def make_doc(index, seen):
    # config.cells_int has a UNIQUE index in biotope_study — keep generating
    # until we find a pattern whose bitboard is not already used.
    for _ in range(200):
        cells = random_pattern()
        cint = cells_to_int(cells)
        if cint in seen:
            continue
        seen.add(cint)
        return {
            "metadata": {
                "player_id": "exhaust_engine",
                "nickname": f"exhaust#{index:08d}",
                "league": "study",
            },
            "config": {
                "bounding_box_x": GRID,
                "bounding_box_y": GRID,
                "cell_count": len(cells),
                "cells": [[x, y] for x, y in cells],
                "cells_int": cint,
                "next_gen_cells_int": next_gen_int(cells),
            },
            "status": "active",
            "elo_rating": 1200,
            "created_at": datetime.now(timezone.utc),
        }
    raise RuntimeError("Could not find a unique pattern after 200 tries.")


def main():
    if len(sys.argv) < 2:
        sys.exit("Usage: generate_study_configs.py <target_total> [--batch N]")
    target = int(sys.argv[1])

    db, db_name = _connect()
    if db_name != "biotope_study":
        # Guard: this tool is meant for the study dataset only.
        print(f"WARNING: active MONGODB_DB is '{db_name}', not 'biotope_study'.")
        if input("Proceed anyway? [y/N]: ").strip().lower() != "y":
            sys.exit("Aborted.")

    col = db.submissions
    current = col.count_documents({})
    need = target - current
    print(f"Dataset '{db_name}': {current} configs present, target {target}.")
    if need <= 0:
        print("Target already reached — nothing to generate.")
        return

    # Continue nickname numbering after the current maximum to avoid confusion.
    start_idx = 0
    last = list(
        col.find(
            {"metadata.nickname": {"$regex": r"^exhaust#"}}, {"metadata.nickname": 1}
        )
        .sort("metadata.nickname", -1)
        .limit(1)
    )
    if last:
        start_idx = int(last[0]["metadata"]["nickname"].split("#")[1]) + 1

    print(f"Generating {need} new configs (exhaust#{start_idx:08d} ...).")
    # Preload existing bitboards so we never collide with the UNIQUE cells_int index.
    seen = set(
        d["config"]["cells_int"]
        for d in col.find({}, {"config.cells_int": 1})
        if d.get("config", {}).get("cells_int") is not None
    )
    BATCH = 1000
    made = 0
    while made < need:
        chunk = min(BATCH, need - made)
        docs = [make_doc(start_idx + made + i, seen) for i in range(chunk)]
        col.insert_many(docs, ordered=False)
        made += chunk
        print(f"  inserted {made}/{need} (total now {current + made})")

    print(f"Done. '{db_name}' now holds {col.count_documents({})} configs.")


if __name__ == "__main__":
    main()
