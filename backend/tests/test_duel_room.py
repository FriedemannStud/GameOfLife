import sys
import os
import uuid
import asyncio

# Add app to path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import get_db
from app.models import Config
from app.duel_service import (
    create_room,
    join_room,
    lock_choice,
    get_room,
    request_rematch,
    DuelError,
)

# KI-Agent unterstützt: Duel-room lifecycle test (ADR-0028, Step 2.2). Drives the
# service layer directly against the configured MONGODB_DB. Uses unique player ids
# per run and deletes its own room afterwards (no leftover docs in duel_rooms).
#
#   MONGODB_DB=biotope_test python backend/tests/test_duel_room.py

RUN = uuid.uuid4().hex[:8]
RED = f"red-{RUN}"
BLUE = f"blue-{RUN}"
INTRUDER = f"intruder-{RUN}"


async def run_lifecycle():
    db = get_db()
    room_id = None
    try:
        # 1. Create -> RED slot, status waiting, has room_id + 4-char room_code.
        room = await create_room(RED, "RedBob")
        room_id = room["room_id"]
        assert room["status"] == "waiting", room["status"]
        assert room["red"]["player_id"] == RED, room["red"]
        assert len(room["room_code"]) == 4, room["room_code"]
        assert "_id" not in room, "ObjectId leaked into room dict"
        print(f"OK: Created room {room_id} (code {room['room_code']}), RED set.")

        # 2. Join as BLUE -> status choosing, both participants present.
        joined = await join_room(room_id, BLUE, "BlueBob")
        assert joined["status"] == "choosing", joined["status"]
        assert joined["blue"]["player_id"] == BLUE, joined["blue"]
        print("OK: BLUE joined; status -> choosing.")

        # 3. Idempotent re-join by the same BLUE -> no error, still choosing.
        again = await join_room(room_id, BLUE, "BlueBob")
        assert again["status"] == "choosing", again["status"]
        print("OK: Same-player re-join is idempotent.")

        # 4. Third participant -> room_full (409).
        try:
            await join_room(room_id, INTRUDER, "Intruder")
            raise AssertionError("expected room_full DuelError")
        except DuelError as e:
            assert e.status == 409 and e.error == "room_full", (e.status, e.error)
        print("OK: Third join rejected (room_full / 409).")

        # 5. Join a non-existent room -> room_not_found (404).
        try:
            await join_room("does-not-exist", BLUE, "BlueBob")
            raise AssertionError("expected room_not_found DuelError")
        except DuelError as e:
            assert e.status == 404 and e.error == "room_not_found", (e.status, e.error)
        print("OK: Join non-existent room rejected (room_not_found / 404).")

        # 6. Single lock (RED) -> still choosing, choice stays hidden via GET.
        await lock_choice(room_id, RED, config=Config(cells=[[0, 0], [1, 1]]))
        view = await get_room(room_id)
        assert view["status"] == "choosing", view["status"]
        assert "config" not in view["red"], "RED config leaked after single lock!"
        assert "config" not in view["blue"], "BLUE config leaked!"
        print("OK: First lock keeps status=choosing; configs hidden via GET.")

        # 7. Second lock (BLUE) -> match computed exactly once -> status result.
        await lock_choice(room_id, BLUE, config=Config(cells=[[6, 6], [7, 7]]))
        view = await get_room(room_id)
        assert view["status"] == "result", view["status"]
        res = view["result"]
        assert res["winner"] in ("red", "blue", "draw"), res["winner"]
        assert len(res["red_seed"]) == 64 and len(res["blue_seed"]) == 64
        assert res["generations"] == 100, res["generations"]
        print(f"OK: Second lock computed result (winner={res['winner']}).")

        # 8. Exactly one permanent duels document was written for this match.
        n = await db.duels.count_documents({"room_id": room_id})
        assert n == 1, f"expected exactly one duels doc, found {n}"
        duel = await db.duels.find_one({"room_id": room_id})
        assert duel["red"]["player_id"] == RED, duel["red"]
        assert duel["blue"]["player_id"] == BLUE, duel["blue"]
        print("OK: Exactly one duels record written with both identities.")

        # 9. Rematch: first request stays in result; both requests reset to choosing.
        r1 = await request_rematch(room_id, RED)
        assert r1["status"] == "result", r1["status"]
        assert r1["rematch"]["red"] is True, r1["rematch"]
        view = await get_room(room_id)
        assert view["status"] == "result", "single rematch must not reset the room"
        r2 = await request_rematch(room_id, BLUE)
        assert r2["status"] == "choosing", r2["status"]
        view = await get_room(room_id)
        assert view["status"] == "choosing", view["status"]
        assert view["red"]["locked"] is False and view["blue"]["locked"] is False
        assert view["result"] is None, "result must be cleared for the new round"
        assert "config" not in view["red"] and "config" not in view["blue"]
        # A second match should still write exactly one *more* duels doc.
        await lock_choice(room_id, RED, config=Config(cells=[[0, 0], [1, 1]]))
        await lock_choice(room_id, BLUE, config=Config(cells=[[6, 6], [7, 7]]))
        view = await get_room(room_id)
        assert view["status"] == "result", view["status"]
        n2 = await db.duels.count_documents({"room_id": room_id})
        assert n2 == 2, f"expected two duels docs after rematch, found {n2}"
        print("OK: Rematch resets to choosing and replays (two duels records).")
    finally:
        if room_id is not None:
            await db.duel_rooms.delete_many({"room_id": room_id})
            await db.duels.delete_many({"room_id": room_id})
        # Defensive: ensure nothing from this run survives.
        leftover = await db.duel_rooms.count_documents(
            {"red.player_id": RED}
        )
        assert leftover == 0, f"leftover rooms for {RED}: {leftover}"
        print("OK: Cleaned up test rooms.")


def test_duel_room():
    print(f"Running duel-room lifecycle test (run={RUN})...")
    asyncio.run(run_lifecycle())
    print("All duel-room assertions passed.")


if __name__ == "__main__":
    test_duel_room()
