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

    # 2. Prepare input batch
    batch_input = {"max_generations": 1000, "competitors": []}

    for s in submissions:
        batch_input["competitors"].append(
            {"player_id": str(s["_id"]), "cells": s["config"]["cells"]}
        )

    # Create temp files
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f_in:
        json.dump(batch_input, f_in)
        input_path = f_in.name

    output_path = input_path + ".out.json"

    try:
        # 3. Run biotope_hyper_worker
        # KI-Agent unterstützt: Check in multiple locations for Docker/Local flexibility
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

        logger.info(
            f"Epoch finished. Matches played: {results['total_matches_played']}"
        )

        # Store Highlights
        epoch_id = f"epoch_{int(datetime.utcnow().timestamp())}"
        # KI-Agent unterstützt: Resolve UUID player_ids to human-readable
        # nicknames (ADR-0021 bugfix)
        id_to_nickname = {str(s["_id"]): s["metadata"]["nickname"] for s in submissions}
        highlights_data = []
        # KI-Agent unterstützt: Filter oscillating patterns before storing
        # highlights (ADR-0023)
        _raw_highlights = results.get("highlights", [])
        _filtered_highlights = filter_highlights_by_oscillation(
            _raw_highlights, batch_input["max_generations"]
        )
        for h in _filtered_highlights:
            highlights_data.append(
                {
                    "metric_type": "activity_sum",
                    "red_name": id_to_nickname.get(h["red_name"], h["red_name"]),
                    "blue_name": id_to_nickname.get(h["blue_name"], h["blue_name"]),
                    "red_seed": h["red_seed"],
                    "blue_seed": h["blue_seed"],
                    "metric_value": h["metric_value"],
                }
            )

        if highlights_data:
            await db.epoch_highlights.insert_one(
                {
                    "epoch_id": epoch_id,
                    "timestamp": datetime.utcnow(),
                    "highlights": highlights_data,
                }
            )
            logger.info(f"Stored {len(highlights_data)} highlights for {epoch_id}")

        # KI-Agent unterstützt: Epoch-fresh ranking — no historical Elo, full
        # round-robin data resets each epoch
        for position, rank in enumerate(results["rankings"], start=1):
            submission = next(
                (s for s in submissions if str(s["_id"]) == rank["player_id"]), None
            )

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
                        "last_epoch_at": datetime.utcnow(),
                    }
                },
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
                    },
                )

        logger.info("Database updated with Epoch results.")

        # KI-Agent unterstützt: Record the roster fingerprint only after a
        # successful epoch, so a failed epoch retries on the next tick
        # (ADR-0032 Stage 1).
        await db.worker_state.update_one(
            {"_id": "epoch_guard"},
            {"$set": {"fingerprint": current_fp, "updated_at": datetime.utcnow()}},
            upsert=True,
        )

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
