#!/usr/bin/env python3
# KI-Agent unterstützt: Variant of generate_study_configs.py for a SECOND,
# independent study dataset. Differences from the original exhaust# study:
#   * cell_count is drawn uniformly over the FULL range 2..64 (not 2..24),
#   * documents carry their own identifying nickname prefix "uni64#" and
#     player_id "uniform_engine",
#   * everything lands in a SEPARATE database (default "biotope_study_uniform")
#     so its round-robin tournament never mixes with the existing study.
# The document schema is otherwise identical to the exhaust# study, so the same
# analysis / tournament tooling works unchanged once MONGODB_DB points here.
#
# Usage (host):
#   python3 backend/scripts/generate_study_configs_uniform.py 10000
#   python3 backend/scripts/generate_study_configs_uniform.py 10000 \
#       --db biotope_study_uniform
#
# Reads MONGODB_URI from repo-root .env (rewrites docker host -> 27018). The
# MONGODB_DB env var is IGNORED here; the target DB is fixed/overridable via --db
# so this tool can never accidentally write into the live or exhaust# datasets.

import os
import random
import sys
from datetime import datetime, timezone

from dotenv import load_dotenv
from pymongo import MongoClient

GRID = 8
MIN_CELLS = 2
MAX_CELLS = 64  # full board: uniform over the entire density range
NICK_PREFIX = "uni64"
PLAYER_ID = "uniform_engine"
DEFAULT_DB = "biotope_study_uniform"


def _connect(db_name):
    here = os.path.dirname(os.path.abspath(__file__))
    load_dotenv(os.path.join(here, "..", "..", ".env"))
    uri = os.getenv("TOP_PATTERNS_URI") or os.getenv("MONGODB_URI")
    if not uri:
        sys.exit("MONGODB_URI not set in repo-root .env")
    uri = uri.replace("@mongo:27017", "@127.0.0.1:27018")
    return MongoClient(uri, serverSelectionTimeoutMS=4000)[db_name]


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
    # Uniform over cell_count in [MIN_CELLS, MAX_CELLS], then a uniform random
    # placement of that many distinct cells on the 8x8 board.
    cell_count = random.randint(MIN_CELLS, MAX_CELLS)
    coords = [(x, y) for x in range(GRID) for y in range(GRID)]
    return random.sample(coords, cell_count)


def make_doc(index, seen):
    # config.cells_int has a UNIQUE index — keep generating until we find a
    # pattern whose bitboard is not already used.
    for _ in range(200):
        cells = random_pattern()
        cint = cells_to_int(cells)
        if cint in seen:
            continue
        seen.add(cint)
        return {
            "metadata": {
                "player_id": PLAYER_ID,
                "nickname": f"{NICK_PREFIX}#{index:08d}",
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
    args = sys.argv[1:]
    if not args:
        sys.exit("Usage: generate_study_configs_uniform.py <target_total> [--db NAME]")
    target = int(args[0])
    db_name = DEFAULT_DB
    if "--db" in args:
        db_name = args[args.index("--db") + 1]

    db = _connect(db_name)
    col = db.submissions
    # Mirror the exhaust# study schema: enforce unique bitboards.
    col.create_index("config.cells_int", unique=True)

    current = col.count_documents({})
    need = target - current
    print(f"Dataset '{db_name}': {current} configs present, target {target}.")
    if need <= 0:
        print("Target already reached — nothing to generate.")
        return

    # Continue nickname numbering after the current maximum.
    start_idx = 0
    last = list(
        col.find(
            {"metadata.nickname": {"$regex": rf"^{NICK_PREFIX}#"}},
            {"metadata.nickname": 1},
        )
        .sort("metadata.nickname", -1)
        .limit(1)
    )
    if last:
        start_idx = int(last[0]["metadata"]["nickname"].split("#")[1]) + 1

    print(
        f"Generating {need} new configs "
        f"({NICK_PREFIX}#{start_idx:08d} ...), cell_count uniform "
        f"{MIN_CELLS}..{MAX_CELLS}."
    )
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
