import sys
import os
import json
import uuid
import asyncio
import urllib.request
import urllib.error

# Add app to path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import get_db
from app.auth_utils import normalize_nickname

# KI-Agent unterstützt: End-to-end integration test for name-claiming (ADR-0027).
# Requires a reachable backend (default http://localhost:8001) whose MONGODB_DB
# matches this process's MONGODB_DB, so DB inspection sees the same data.
#
#   MONGODB_DB=biotope_test BIOTOPE_API_URL=http://localhost:8001 \
#       python backend/tests/test_name_claiming.py
#
# Uses a unique nickname per run and cleans up its own docs afterwards.

API_URL = (
    os.getenv("BIOTOPE_API_URL", "http://localhost:8001") + "/api/v1/submit_config"
)
NICKNAME = f"itest-{uuid.uuid4().hex[:8]}"


def _post(payload):
    data = json.dumps(payload).encode()
    req = urllib.request.Request(
        API_URL, data=data, headers={"Content-Type": "application/json"}, method="POST"
    )
    try:
        with urllib.request.urlopen(req) as r:
            return r.status, json.loads(r.read().decode())
    except urllib.error.HTTPError as e:
        return e.code, json.loads(e.read().decode())


def _payload(player_id, recovery_code=None):
    p = {
        "metadata": {"player_id": player_id, "nickname": NICKNAME},
        "config": {"cells": [[0, 0]]},
    }
    if recovery_code is not None:
        p["auth"] = {"recovery_code": recovery_code}
    return p


def run_matrix():
    # 1. Claim a fresh name -> 201 + recovery_code
    status, body = _post(_payload("devA"))
    assert status == 201, f"claim status {status}: {body}"
    code = body.get("recovery_code")
    assert code, f"no recovery_code in claim response: {body}"
    print(f"OK: Claim returned 201 + recovery_code ({code}).")

    # 2. Silent return (same device) -> 201, no code
    status, body = _post(_payload("devA"))
    assert status == 201, f"silent status {status}: {body}"
    assert "recovery_code" not in body, f"unexpected code on silent return: {body}"
    print("OK: Silent same-device return 201, no code.")

    # 3. Blocked (other device, no code) -> 409 name_taken
    status, body = _post(_payload("devB"))
    assert status == 409, f"blocked status {status}: {body}"
    assert body.get("detail") == {"error": "name_taken"}, body
    print("OK: Other device without code blocked (409 name_taken).")

    # 4. Wrong code -> 403 invalid_recovery_code
    status, body = _post(_payload("devB", "BIOTOP-XXXX"))
    assert status == 403, f"wrong-code status {status}: {body}"
    assert body.get("detail") == {"error": "invalid_recovery_code"}, body
    print("OK: Wrong code rejected (403 invalid_recovery_code).")

    # 5. Reclaim with correct code -> 201 + reclaimed
    status, body = _post(_payload("devB", code))
    assert status == 201, f"reclaim status {status}: {body}"
    assert body.get("reclaimed") is True, f"missing reclaimed flag: {body}"
    print("OK: Reclaim with correct code 201 + reclaimed.")

    return code


async def inspect_and_cleanup():
    db = get_db()
    norm = normalize_nickname(NICKNAME)

    player = await db.players.find_one({"nickname_normalized": norm})
    assert player is not None, "player not found after claim"
    assert player["player_id"] == "devB", f"re-binding failed: {player['player_id']}"
    h = player.get("recovery_code_hash")
    assert h and len(h) == 64, f"bad recovery_code_hash: {h}"
    assert "recovery_code" not in player, "plaintext code leaked into player doc"
    print("OK: Player re-bound to devB, stores only a 64-hex hash (no plaintext).")

    sub = await db.submissions.find_one(
        {"metadata.nickname": NICKNAME}, sort=[("_id", -1)]
    )
    assert sub is not None, "submission not persisted"
    assert (
        "auth" not in sub and "recovery_code" not in sub
    ), f"recovery code leaked into submission: {sub.keys()}"
    print("OK: Stored submission contains no auth / recovery_code.")

    # Cleanup this run's docs.
    await db.players.delete_many({"nickname_normalized": norm})
    await db.submissions.delete_many({"metadata.nickname": NICKNAME})
    print("OK: Cleaned up test documents.")


def test_name_claiming():
    print(f"Running name-claiming integration test (nickname={NICKNAME})...")
    run_matrix()
    asyncio.run(inspect_and_cleanup())
    print("All name-claiming integration assertions passed.")


if __name__ == "__main__":
    test_name_claiming()
