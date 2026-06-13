import sys
import os
import uuid
import asyncio

# Add app to path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import get_db
from app.auth_utils import hash_recovery_code
from app.duel_service import get_my_record, DuelError

# KI-Agent unterstützt: Personal duel-record aggregation test (ADR-0028, Step 4.1).
# Seeds a handful of duels docs for one identity, asserts head-to-head, best-weapon
# selection, the tally, and recovery-code verification. Self-cleaning.
#
#   MONGODB_DB=biotope_test python backend/tests/test_duel_record.py

RUN = uuid.uuid4().hex[:8]
ME = f"me-{RUN}"
ROOM_TAG = f"rec-{RUN}"  # marks all docs from this run for cleanup
WEAPON_A = f"weaponA-{RUN}"
WEAPON_B = f"weaponB-{RUN}"


def _seed(seed_val):
    return [seed_val] + [0] * 63


def _duel(idx, my_slot, opponent_nick, winner, my_config_id):
    """Build a duels doc placing ME in `my_slot` with a given outcome/weapon."""
    me = {
        "player_id": ME,
        "nickname": "MeBob",
        "config_id": my_config_id,
        "seed": _seed(1),
    }
    opp = {
        "player_id": f"opp-{opponent_nick}-{RUN}",
        "nickname": opponent_nick,
        "config_id": None,
        "seed": _seed(0),
    }
    red, blue = (me, opp) if my_slot == "red" else (opp, me)
    return {
        "room_id": f"{ROOM_TAG}-{idx}",
        "red": red,
        "blue": blue,
        "winner": winner,
        "generations": 100,
        "red_population": 1,
        "blue_population": 1,
    }


async def run_record():
    db = get_db()
    try:
        # vs FRANK: 2 wins (weapon A), 1 loss (weapon B). vs ANNA: 1 draw.
        docs = [
            _duel(1, "red", "FRANK", "red", WEAPON_A),  # win, A
            _duel(2, "blue", "FRANK", "blue", WEAPON_A),  # win, A
            _duel(3, "red", "FRANK", "blue", WEAPON_B),  # loss, B
            _duel(4, "red", "ANNA", "draw", WEAPON_A),  # draw, A
        ]
        await db.duels.insert_many(docs)

        rec = await get_my_record(ME)

        # Tally: 2 wins, 1 loss, 1 draw.
        assert rec["tally"] == {"w": 2, "l": 1, "d": 1}, rec["tally"]
        print("OK: Tally is 2W / 1L / 1D.")

        # Head-to-head: vs FRANK 2:1 (+0 draws), vs ANNA 0:0 (+1 draw).
        hh = {row["opponent"]: row for row in rec["head_to_head"]}
        assert hh["FRANK"]["wins"] == 2 and hh["FRANK"]["losses"] == 1, hh["FRANK"]
        assert hh["ANNA"]["draws"] == 1, hh["ANNA"]
        print("OK: Head-to-head vs FRANK 2:1, vs ANNA has 1 draw.")

        # Best weapon: WEAPON_A has 2 wins, WEAPON_B has 0 -> A wins.
        assert rec["best_weapon"] is not None, "expected a best weapon"
        assert rec["best_weapon"]["config_id"] == WEAPON_A, rec["best_weapon"]
        assert rec["best_weapon"]["wins"] == 2, rec["best_weapon"]
        print("OK: Best weapon is WEAPON_A (2 wins).")

        # Recovery-code verification: wrong code rejected (403).
        await db.players.insert_one(
            {
                "player_id": ME,
                "nickname": "MeBob",
                "nickname_normalized": "mebob",
                "recovery_code_hash": hash_recovery_code("BIOTOP-GOOD"),
            }
        )
        try:
            await get_my_record(ME, recovery_code="BIOTOP-WRONG")
            raise AssertionError("expected invalid_recovery_code DuelError")
        except DuelError as e:
            assert e.status == 403 and e.error == "invalid_recovery_code", e.error
        print("OK: Wrong recovery code rejected (403).")

        # Correct code passes through.
        rec2 = await get_my_record(ME, recovery_code="BIOTOP-GOOD")
        assert rec2["tally"] == {"w": 2, "l": 1, "d": 1}, rec2["tally"]
        print("OK: Correct recovery code returns the record.")
    finally:
        await db.duels.delete_many({"room_id": {"$regex": f"^{ROOM_TAG}-"}})
        await db.players.delete_many({"player_id": ME})
        leftover = await db.duels.count_documents(
            {"room_id": {"$regex": f"^{ROOM_TAG}-"}}
        )
        assert leftover == 0, f"leftover duels: {leftover}"
        print("OK: Cleaned up test documents.")


def test_duel_record():
    print(f"Running duel-record aggregation test (run={RUN})...")
    asyncio.run(run_record())
    print("All duel-record assertions passed.")


if __name__ == "__main__":
    test_duel_record()
