import asyncio
import os
import sys
import uuid

# Add app to path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import get_db
from app.duel_service import (
    create_room,
    get_room,
    get_room_frames,
    join_room,
    lock_choice,
)
from app.models import Config

# KI-Agent unterstützt: Duel end-to-end integration test (ADR-0028, Step 7.2). Unlike
# the service-only test_duel_room, this drives the full referee path through the C
# binary (build/biotope_headless) and asserts the deterministic replay payload that
# Phase 6.2 added: a result, exactly one permanent duels doc, and a non-empty frames
# payload on the room. Requires built binaries (make) and a reachable MongoDB.
#
#   MONGODB_DB=biotope_test python backend/tests/test_duel_integration.py

RUN = uuid.uuid4().hex[:8]
RED = f"red-{RUN}"
BLUE = f"blue-{RUN}"

# A red blinker (oscillates) vs a blue 2x2 block (stable) -> deterministic outcome.
RED_CELLS = [[1, 0], [1, 1], [1, 2]]
BLUE_CELLS = [[2, 2], [3, 2], [2, 3], [3, 3]]


async def run_integration():
    db = get_db()
    room_id = None
    try:
        # 1. Full pairing + blind choice driving the headless referee.
        room = await create_room(RED, "RedBob")
        room_id = room["room_id"]
        await join_room(room_id, BLUE, "BlueBob")
        await lock_choice(room_id, RED, config=Config(cells=RED_CELLS))
        await lock_choice(room_id, BLUE, config=Config(cells=BLUE_CELLS))
        print(f"OK: Drove create->join->lock->lock on room {room_id}.")

        # 2. The match was computed exactly once and produced a result.
        view = await get_room(room_id)
        assert view["status"] == "result", view["status"]
        res = view["result"]
        assert res["winner"] in ("red", "blue", "draw"), res["winner"]
        assert res["generations"] == 100, res["generations"]
        # The short-poll projection must never carry the heavy frames payload.
        assert "frames" not in view, "frames leaked onto the poll projection!"
        print(f"OK: Referee produced result (winner={res['winner']}); poll lean.")

        # 3. Exactly one permanent duels document for this match.
        n = await db.duels.count_documents({"room_id": room_id})
        assert n == 1, f"expected exactly one duels doc, found {n}"
        print("OK: Exactly one duels record written.")

        # 4. A non-empty, well-formed deterministic replay payload is available.
        frames = await get_room_frames(room_id)
        assert frames["rows"] == 8, frames["rows"]
        assert frames["cols"] == 16, frames["cols"]
        assert len(frames["frames"]) == 101, len(frames["frames"])
        assert all(len(f) == 128 for f in frames["frames"]), "bad frame length"
        # Gen 0 is the seeds (>= 7 live cells); the board must actually evolve.
        assert frames["frames"][0].count("0") < 128, "gen 0 is empty"
        assert frames["frames"][0] != frames["frames"][1], "board never changed"
        print("OK: Frames payload = 8x16, 101 frames, board evolves.")
    finally:
        if room_id is not None:
            await db.duel_rooms.delete_many({"room_id": room_id})
            await db.duels.delete_many({"room_id": room_id})
        leftover = await db.duel_rooms.count_documents({"red.player_id": RED})
        assert leftover == 0, f"leftover rooms for {RED}: {leftover}"
        print("OK: Cleaned up test documents.")


def test_duel_integration():
    print(f"Running duel integration test (run={RUN})...")
    asyncio.run(run_integration())
    print("All duel-integration assertions passed.")


if __name__ == "__main__":
    test_duel_integration()
