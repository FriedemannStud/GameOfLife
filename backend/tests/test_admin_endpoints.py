"""
Auth + endpoint smoke tests for the admin config-deletion API (ADR-0035, Phase 3).

Drives the live HTTP surface and seeds its own scoped data, so it must run inside
an environment that can reach both the backend and MongoDB (e.g. the backend
container), with ADMIN_PASSWORD set:

    docker compose exec backend python tests/test_admin_endpoints.py

It uses a unique nickname per run (mode B is nickname-scoped, so no other data is
touched) and deletes its own documents afterwards.
"""

# KI-Agent unterstützt
import asyncio
import json
import os
import sys
import urllib.error
import urllib.request
import uuid
from datetime import datetime

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.database import get_db  # noqa: E402

API = os.getenv("BIOTOPE_API_URL", "http://localhost:8000")
KEY = os.getenv("ADMIN_PASSWORD")
NICK = f"itest-admin-{uuid.uuid4().hex[:8]}"


def _req(method, path, payload=None, key=None):
    headers = {"Content-Type": "application/json"}
    if key is not None:
        headers["X-Admin-Key"] = key
    data = json.dumps(payload).encode() if payload is not None else None
    req = urllib.request.Request(API + path, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req) as r:
            return r.status, json.loads(r.read().decode())
    except urllib.error.HTTPError as e:
        return e.code, json.loads(e.read().decode())


def _seed_doc(nickname):
    return {
        "metadata": {"nickname": nickname, "player_id": "itest", "league": "local"},
        "config": {"bounding_box_x": 8, "bounding_box_y": 8, "cells": [[0, 0], [1, 1]]},
        "status": "active",
        "elo_rating": 1200,
        "matches_played": 5,
        "win_rate": 0.25,
        "avg_stable_generation": 60.0,
        "created_at": datetime.utcnow(),
    }


async def _statuses(db, ids):
    docs = await db.submissions.find({"_id": {"$in": ids}}).to_list(length=None)
    return {str(d["_id"]): d["status"] for d in docs}


async def run():
    if not KEY:
        raise SystemExit("ADMIN_PASSWORD must be set in this environment")

    db = get_db()
    r1 = await db.submissions.insert_one(_seed_doc(NICK))
    r2 = await db.submissions.insert_one(_seed_doc(NICK))
    ids = [r1.inserted_id, r2.inserted_id]

    try:
        body = {"mode": "B", "nickname": NICK, "dry_run": True}

        # --- Auth: missing key -> 401, no mutation ---
        st, _ = _req("POST", "/api/admin/delete", body, key=None)
        assert st == 401, f"missing key expected 401, got {st}"
        st, _ = _req("POST", "/api/admin/delete", body, key="wrong-key")
        assert st == 401, f"wrong key expected 401, got {st}"
        states = await _statuses(db, ids)
        assert all(v == "active" for v in states.values()), states
        print("PASS: missing/invalid key -> 401, DB unchanged")

        # --- Correct key + dry_run -> 200, preview, DB unchanged ---
        st, resp = _req("POST", "/api/admin/delete", body, key=KEY)
        assert st == 200, f"dry-run expected 200, got {st}: {resp}"
        assert resp["dry_run"] is True
        assert resp["removed_count"] == 2, resp
        assert len(resp["affected"]) == 2, resp
        states = await _statuses(db, ids)
        assert all(v == "active" for v in states.values()), "dry-run must not mutate"
        print("PASS: dry-run returns affected count, DB unchanged")

        # --- Execute -> rows transition to removed ---
        st, resp = _req(
            "POST",
            "/api/admin/delete",
            {"mode": "B", "nickname": NICK, "dry_run": False},
            key=KEY,
        )
        assert st == 200 and resp["removed_count"] == 2, resp
        states = await _statuses(db, ids)
        assert all(v == "removed" for v in states.values()), states
        print("PASS: execute transitions rows to removed")

        # --- Trash listing includes our rows ---
        st, resp = _req("GET", "/api/admin/removed", key=KEY)
        assert st == 200, resp
        removed_ids = {row["submission_id"] for row in resp["removed"]}
        assert {str(i) for i in ids} <= removed_ids, "removed rows missing from trash"
        print("PASS: trash listing includes removed rows")

        # --- Restore round-trips one row; 404 on unknown id ---
        st, resp = _req(
            "POST", "/api/admin/restore", {"submission_id": str(ids[0])}, key=KEY
        )
        assert st == 200 and resp["status"] == "restored", resp
        states = await _statuses(db, ids)
        assert states[str(ids[0])] == "active", "restored row must be active"
        assert states[str(ids[1])] == "removed", "other row must stay removed"

        st, _ = _req(
            "POST",
            "/api/admin/restore",
            {"submission_id": "0" * 24},
            key=KEY,
        )
        assert st == 404, f"unknown restore expected 404, got {st}"
        print("PASS: restore round-trips; unknown id -> 404")

        print("\nAll admin endpoint tests passed.")
    finally:
        await db.submissions.delete_many({"_id": {"$in": ids}})


if __name__ == "__main__":
    asyncio.run(run())
