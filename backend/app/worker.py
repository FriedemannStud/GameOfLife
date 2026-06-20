import asyncio
import hashlib
import json
import logging
import os
import sys
import tempfile
from collections import deque
from datetime import datetime

import numpy as np
from bson import ObjectId
from pymongo import UpdateOne

# Path management MUST be first
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import get_db

# KI-Agent unterstützt: Epoch-based Tournament Worker

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("epoch_worker")


# ---------------------------------------------------------------------------
# Incremental Tournament Computation (ADR-0032)
# ---------------------------------------------------------------------------
# KI-Agent unterstützt: Content hash of an 8x8 seed pattern. Keying by content
# (not player_id) gives edit-invalidation and duplicate-pattern dedup for free.
def seed_hash(cells) -> str:
    canonical = json.dumps(cells, separators=(",", ":"))
    return hashlib.sha256(canonical.encode("utf-8")).hexdigest()[:16]


# KI-Agent unterstützt: Stable fingerprint of the active roster. Captures
# additions, removals, deactivations, and edits (an edited seed hashes
# differently). Order-independent via sorting.
def roster_fingerprint(submissions) -> str:
    parts = sorted(
        f"{str(s['_id'])}:{seed_hash(s['config']['cells'])}" for s in submissions
    )
    return hashlib.sha256("\n".join(parts).encode("utf-8")).hexdigest()


# KI-Agent unterstützt: Canonical (unordered) cache key for a pair of seeds.
# The orientation in which a pair is played (which seed is RED/left) does not
# change the winning seed or per-seed populations on the torus — see ADR-0026 —
# so the cached result is a well-defined function of the unordered pair.
# A self-pair (h_a == h_b, two identical patterns) is a valid mirror match.
def pair_key(h_a: str, h_b: str) -> str:
    lo, hi = (h_a, h_b) if h_a <= h_b else (h_b, h_a)
    return f"{lo}:{hi}"


# KI-Agent unterstützt: Map every distinct unordered seed-pair to one representative
# pair of competitor indices (ADR-0032). Identical patterns (equal hash) collapse
# to a single pair_key via setdefault, so a duplicated pattern is computed once.
def enumerate_needed_pairs(hashes) -> dict:
    needed = {}
    n = len(hashes)
    for i in range(n):
        for j in range(i + 1, n):
            needed.setdefault(pair_key(hashes[i], hashes[j]), (i, j))
    return needed


# KI-Agent unterstützt: Reproduce the C highlight seed layout (ADR-0032). The C
# hyper-worker stores a flat 64-entry seed where bit (r*8 + c) is set; its parser
# fills cells[y][x] from a sparse [x, y] pair, so the flat index is y*8 + x.
# Mirrors grid_to_bitboard() in game_logic.c.
def cells_to_seed64(cells) -> list:
    seed = [0] * 64
    for pair in cells:
        x, y = pair[0], pair[1]
        if 0 <= x < 8 and 0 <= y < 8:
            seed[y * 8 + x] = 1
    return seed


# KI-Agent unterstützt: Locate and run biotope_hyper_worker on a batch dict,
# returning the parsed results (or None on failure). Centralises temp-file and
# binary-path handling shared by full and incremental (pairings) runs.
async def run_hyper_worker(batch_input: dict):
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f_in:
        json.dump(batch_input, f_in)
        input_path = f_in.name
    output_path = input_path + ".out.json"

    possible_paths = [
        "./build/biotope_hyper_worker",
        "/app/build/biotope_hyper_worker",
        "./biotope_hyper_worker",
    ]
    binary_path = next((p for p in possible_paths if os.path.exists(p)), None)
    if not binary_path:
        logger.error(
            f"Binary not found. Checked: {possible_paths}. "
            f"Did you run 'make' before starting docker-compose?"
        )
        _safe_remove(input_path, output_path)
        return None

    try:
        process = await asyncio.create_subprocess_exec(
            binary_path,
            input_path,
            output_path,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )
        _stdout, stderr = await process.communicate()
        if process.returncode != 0:
            logger.error(f"Hyper-Worker failed: {stderr.decode()}")
            return None
        if not os.path.exists(output_path):
            logger.error("Output file from Hyper-Worker missing.")
            return None
        with open(output_path, "r") as f_out:
            return json.load(f_out)
    finally:
        _safe_remove(input_path, output_path)


def _safe_remove(*paths):
    for p in paths:
        if p and os.path.exists(p):
            os.remove(p)


# ---------------------------------------------------------------------------
# Oscillator Detection (ADR-0023)
# World config mirrors run_isolated_match() in game_logic.c and
# the kiosk seed placement in app_state_manager.c (KIOSK_SIM_ROWS/COLS).
# ---------------------------------------------------------------------------
# KI-Agent unterstützt
_KIOSK_ROWS = 8  # LOCAL_GRID_SIZE
_KIOSK_COLS = 16  # LOCAL_GRID_SIZE * 2
_SEED_SIZE = 8
_RED_ORIGIN = (0, 0)  # left half:  rows 0-7, cols 0-7
_BLUE_ORIGIN = (0, 8)  # right half: rows 0-7, cols 8-15
_TEAM_RED = 1
_TEAM_BLUE = 2
_DEAD = 0
_MAX_PERIOD = 5
_CONFIRM_CYCLES = 2


def _step_numpy(grid: np.ndarray) -> np.ndarray:
    """One generation of two-team Conway rules with toroidal (wrap-around) boundary."""
    # KI-Agent unterstützt: Translated from update_generation() in game_logic.c
    red = (grid == _TEAM_RED).astype(np.int16)
    blue = (grid == _TEAM_BLUE).astype(np.int16)
    red_n = np.zeros(grid.shape, dtype=np.int16)
    blue_n = np.zeros(grid.shape, dtype=np.int16)

    for dr in (-1, 0, 1):
        for dc in (-1, 0, 1):
            if dr == 0 and dc == 0:
                continue
            red_n += np.roll(np.roll(red, dr, axis=0), dc, axis=1)
            blue_n += np.roll(np.roll(blue, dr, axis=0), dc, axis=1)

    total_n = red_n + blue_n
    alive = grid != _DEAD
    survives = alive & ((total_n == 2) | (total_n == 3))
    born = ~alive & (total_n == 3)

    new_grid = np.zeros_like(grid)
    new_grid[survives] = grid[survives]
    new_grid[born] = np.where(red_n[born] > blue_n[born], _TEAM_RED, _TEAM_BLUE)
    return new_grid


def is_oscillating_match(red_seed: list, blue_seed: list, max_generations: int) -> bool:
    """
    Returns True if the match converges to a period-2..5 oscillator in the kiosk world.
    Simulates an 8x16 grid with same seed placement as app_state_manager.c (ADR-0023).
    """
    # KI-Agent unterstützt
    grid = np.zeros((_KIOSK_ROWS, _KIOSK_COLS), dtype=np.int8)

    r0_r, c0_r = _RED_ORIGIN
    r0_b, c0_b = _BLUE_ORIGIN
    seed_r = np.array(red_seed, dtype=np.int8).reshape(_SEED_SIZE, _SEED_SIZE)
    seed_b = np.array(blue_seed, dtype=np.int8).reshape(_SEED_SIZE, _SEED_SIZE)
    grid[r0_r : r0_r + _SEED_SIZE, c0_r : c0_r + _SEED_SIZE] = seed_r * _TEAM_RED
    grid[r0_b : r0_b + _SEED_SIZE, c0_b : c0_b + _SEED_SIZE] = seed_b * _TEAM_BLUE

    buf_size = _MAX_PERIOD * _CONFIRM_CYCLES + 1  # 11 state snapshots
    state_buf = deque(maxlen=buf_size)
    prev_state: bytes = b""

    for _ in range(max_generations):
        grid = _step_numpy(grid)
        curr_state = grid.tobytes()
        if curr_state == prev_state:  # Truly static (period-1): not an oscillator
            return False
        state_buf.append(curr_state)
        prev_state = curr_state

    buf = list(state_buf)
    n = len(buf)
    for p in range(2, _MAX_PERIOD + 1):
        if n >= 2 * p + 1:
            if buf[-1] == buf[-1 - p] == buf[-1 - 2 * p]:
                return True
    return False


def filter_highlights_by_oscillation(highlights: list, max_generations: int) -> list:
    """
    Reorders highlight candidates: non-oscillating first, oscillating as soft fallback.
    Preserves descending metric_value order within each group.
    """
    # KI-Agent unterstützt
    non_osc = []
    osc = []
    for h in highlights:
        if is_oscillating_match(h["red_seed"], h["blue_seed"], max_generations):
            logger.info(
                "Oscillator detected: %s vs %s (metric_value=%.0f)",
                h.get("red_name", "?"),
                h.get("blue_name", "?"),
                h.get("metric_value", 0),
            )
            osc.append(h)
        else:
            non_osc.append(h)
    logger.info(
        "Highlight filter: %d non-oscillating, %d oscillating (of %d candidates)",
        len(non_osc),
        len(osc),
        len(highlights),
    )
    return non_osc + osc


async def execute_epoch(db):
    """
    Executes a full tournament epoch using the C-Hyper-Worker.
    """
    logger.info("Starting Tournament Epoch...")

    # 1. Fetch all active submissions
    # We use a large length limit for the university exhibition (up to 1000)
    submissions = await db.submissions.find({"status": "active"}).to_list(length=1000)

    if len(submissions) < 2:
        logger.info(
            f"Not enough submissions for a tournament "
            f"(Found: {len(submissions)}). Skipping."
        )
        return

    # KI-Agent unterstützt: Roster-change early-exit guard (ADR-0032 Stage 1).
    # Matches are deterministic (see run_isolated_match in game_logic.c), so an
    # unchanged roster yields a bit-for-bit identical ranking — recomputing is
    # pure waste. Skip unless the active set changed since the last successful
    # epoch.
    current_fp = roster_fingerprint(submissions)
    guard = await db.worker_state.find_one({"_id": "epoch_guard"})
    if guard and guard.get("fingerprint") == current_fp:
        logger.info("Roster unchanged since last epoch — skipping.")
        return

    # KI-Agent unterstützt: Build the competitor list in a stable index order and
    # compute each seed's content hash (ADR-0032 Stage 2).
    competitors = []
    for s in submissions:
        cells = s["config"]["cells"]
        competitors.append(
            {
                "id": str(s["_id"]),
                "cells": cells,
                "hash": seed_hash(cells),
                "nickname": s["metadata"]["nickname"],
            }
        )
    n = len(competitors)
    max_generations = 1000

    # KI-Agent unterstützt: DETERMINISM TRIPWIRE (ADR-0032 / ADR-0026). The match
    # cache below is valid ONLY because run_isolated_match is deterministic and the
    # torus colour-swap symmetry makes a match result a function of the unordered
    # seed pair. If randomness or a topology/birth-rule change is ever introduced,
    # drop the match_results collection and re-evaluate this optimisation.
    try:
        # 1. Enumerate every distinct unordered seed-pair needed for the ranking.
        needed = enumerate_needed_pairs([c["hash"] for c in competitors])

        # 2. Look up the cache; compute only the missing pairs.
        cached = {}
        async for doc in db.match_results.find({"pair_key": {"$in": list(needed)}}):
            cached[doc["pair_key"]] = doc

        missing = [(k, ij) for k, ij in needed.items() if k not in cached]
        logger.info(
            "Tournament: %d competitors, %d distinct pairs, %d cached, %d to compute.",
            n,
            len(needed),
            len(cached),
            len(missing),
        )

        if missing:
            # Send pairings in canonical hash order so C's idx_a is always the
            # lower-hash seed → the stored 'a'/'b' winner needs no remapping.
            pairings = []
            for _k, (i, j) in missing:
                if competitors[i]["hash"] <= competitors[j]["hash"]:
                    pairings.append([i, j])
                else:
                    pairings.append([j, i])
            batch_input = {
                "max_generations": max_generations,
                "competitors": [
                    {"player_id": c["id"], "cells": c["cells"]} for c in competitors
                ],
                "pairings": pairings,
            }
            results = await run_hyper_worker(batch_input)
            if results is None:
                # Error already logged; do NOT update the fingerprint so the
                # epoch retries on the next tick.
                return

            # KI-Agent unterstützt: Integrity guard (ADR-0032). The C worker must
            # return exactly one result per requested pairing. A mismatch means the
            # invoked binary does not support pairings mode (e.g. a stale
            # biotope_hyper_worker that ignores the "pairings" array and emits no
            # "match_results"). Abort WITHOUT aggregating — otherwise every pair
            # would be uncached and the leaderboard would be silently overwritten
            # with zeros — and WITHOUT updating the fingerprint, so the epoch
            # retries once the correct binary is in place.
            computed = results.get("match_results", [])
            if len(computed) != len(pairings):
                logger.error(
                    "Hyper-Worker returned %d match results for %d requested "
                    "pairings. The binary likely does not support pairings mode "
                    "(stale build?). Aborting epoch without touching the leaderboard.",
                    len(computed),
                    len(pairings),
                )
                return

            # 3. Upsert newly computed results into the cache.
            ops = []
            for m in computed:
                ia, ib = m["idx_a"], m["idx_b"]
                ha, hb = competitors[ia]["hash"], competitors[ib]["hash"]
                doc = {
                    "pair_key": pair_key(ha, hb),
                    "hash_a": ha,  # lower hash (canonical send order)
                    "hash_b": hb,
                    "winner": m["winner"],
                    "pop_a": m["pop_a"],
                    "pop_b": m["pop_b"],
                    "activity_sum": m["activity_sum"],
                    "stable_at_generation": m["stable_at_generation"],
                    "computed_at": datetime.utcnow(),
                }
                cached[doc["pair_key"]] = doc
                ops.append(
                    UpdateOne({"pair_key": doc["pair_key"]}, {"$set": doc}, upsert=True)
                )
            if ops:
                await db.match_results.bulk_write(ops, ordered=False)
            logger.info("Cached %d newly computed matches.", len(ops))
        else:
            logger.info("All pairs cached — aggregating from cache only.")

        # 4. Aggregate the ranking from the cache over ALL active competitor pairs.
        #    (Cheap arithmetic; the expensive simulations were the cached part.)
        stats = [
            {
                "score": 0.0,
                "wins": 0,
                "draws": 0,
                "losses": 0,
                "played": 0,
                "stable_sum": 0,
            }
            for _ in range(n)
        ]
        for i in range(n):
            for j in range(i + 1, n):
                hi, hj = competitors[i]["hash"], competitors[j]["hash"]
                doc = cached.get(pair_key(hi, hj))
                if doc is None:
                    continue  # defensive; should not happen
                stable = doc["stable_at_generation"] or max_generations
                stats[i]["played"] += 1
                stats[j]["played"] += 1
                stats[i]["stable_sum"] += stable
                stats[j]["stable_sum"] += stable
                if doc["winner"] == "draw":
                    stats[i]["draws"] += 1
                    stats[j]["draws"] += 1
                    stats[i]["score"] += 0.5
                    stats[j]["score"] += 0.5
                else:
                    # doc 'a' == lower hash; i is the 'a' side iff hi <= hj.
                    i_won = (doc["winner"] == "a") == (hi <= hj)
                    if i_won:
                        stats[i]["wins"] += 1
                        stats[i]["score"] += 1.0
                        stats[j]["losses"] += 1
                    else:
                        stats[j]["wins"] += 1
                        stats[j]["score"] += 1.0
                        stats[i]["losses"] += 1

        # Rank by score desc, then wins, then id for a deterministic order.
        ranked = sorted(
            range(n),
            key=lambda idx: (
                -stats[idx]["score"],
                -stats[idx]["wins"],
                competitors[idx]["id"],
            ),
        )

        now = datetime.utcnow()
        for position, idx in enumerate(ranked, start=1):
            c = competitors[idx]
            st = stats[idx]
            played = st["played"]
            win_rate = st["score"] / played if played else 0
            avg_stable = st["stable_sum"] / played if played else 0
            await db.submissions.update_one(
                {"_id": ObjectId(c["id"])},
                {
                    "$set": {
                        "rank": position,
                        "win_rate": win_rate,
                        "wins": st["wins"],
                        "draws": st["draws"],
                        "losses": st["losses"],
                        "total_score": st["score"],
                        "matches_played": played,
                        "avg_stable_generation": avg_stable,
                        "last_epoch_at": now,
                    }
                },
            )
            await db.players.update_one(
                {"nickname": c["nickname"]},
                {
                    "$set": {
                        "rank": position,
                        "win_rate": win_rate,
                        "wins": st["wins"],
                        "draws": st["draws"],
                        "losses": st["losses"],
                        "avg_stable_generation": avg_stable,
                        "matches_played": played,
                    }
                },
            )
        logger.info("Database updated with Epoch results (%d players).", n)

        # 5. Highlights: top activity_sum pairs from the active set's cache.
        #    Seeds are reconstructed from the active submissions (hash_a -> red,
        #    hash_b -> blue; orientation is irrelevant by the ADR-0026 symmetry).
        hash_to_comp = {}
        for c in competitors:
            hash_to_comp.setdefault(c["hash"], c)
        candidates = []
        for k, _ij in needed.items():
            doc = cached.get(k)
            if doc is None:
                continue
            ca = hash_to_comp.get(doc["hash_a"])
            cb = hash_to_comp.get(doc["hash_b"])
            if not ca or not cb:
                continue
            candidates.append(
                {
                    "red_name": ca["nickname"],
                    "blue_name": cb["nickname"],
                    "red_seed": cells_to_seed64(ca["cells"]),
                    "blue_seed": cells_to_seed64(cb["cells"]),
                    "metric_value": doc["activity_sum"],
                }
            )
        candidates.sort(key=lambda h: h["metric_value"], reverse=True)
        # KI-Agent unterstützt: Filter oscillating patterns before storing (ADR-0023)
        filtered = filter_highlights_by_oscillation(candidates[:10], max_generations)
        highlights_data = [
            {
                "metric_type": "activity_sum",
                "red_name": h["red_name"],
                "blue_name": h["blue_name"],
                "red_seed": h["red_seed"],
                "blue_seed": h["blue_seed"],
                "metric_value": h["metric_value"],
            }
            for h in filtered
        ]
        if highlights_data:
            epoch_id = f"epoch_{int(datetime.utcnow().timestamp())}"
            await db.epoch_highlights.insert_one(
                {
                    "epoch_id": epoch_id,
                    "timestamp": datetime.utcnow(),
                    "highlights": highlights_data,
                }
            )
            logger.info("Stored %d highlights for %s", len(highlights_data), epoch_id)

        # 6. Record the roster fingerprint only after a fully successful epoch, so
        #    a failed epoch retries on the next tick (ADR-0032 Stage 1).
        await db.worker_state.update_one(
            {"_id": "epoch_guard"},
            {"$set": {"fingerprint": current_fp, "updated_at": datetime.utcnow()}},
            upsert=True,
        )

    except Exception as e:
        logger.error(f"Error during Epoch execution: {e}")


async def worker_loop():
    logger.info("Biotope Epoch Worker started.")
    from app.database import check_connection

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
