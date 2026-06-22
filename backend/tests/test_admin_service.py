"""
Unit tests for the admin config-deletion service layer (ADR-0035, Phase 2).
Self-running per CLAUDE.md:

    python backend/tests/test_admin_service.py

Uses a tiny in-memory async fake of the motor `submissions` collection (mongomock
is not a project dependency), exercising only the query/update operators the
service actually relies on.
"""

# KI-Agent unterstützt
import asyncio
import os
import sys
from datetime import datetime, timedelta

from bson import ObjectId

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.admin_service import (  # noqa: E402
    REASON_MODE_A,
    list_removed,
    restore_id,
    select_mode_a,
    select_mode_b,
    select_mode_c,
    soft_delete_ids,
)

# ---------------------------------------------------------------------------
# Minimal in-memory async fake of a motor collection
# ---------------------------------------------------------------------------


def _match_op(actual, expected) -> bool:
    """Evaluate a single field constraint (scalar equality or an operator dict)."""
    if isinstance(expected, dict):
        for op, val in expected.items():
            if op == "$ne" and actual == val:
                return False
            if op == "$gt" and not (actual is not None and actual > val):
                return False
            if op == "$in" and actual not in val:
                return False
        return True
    return actual == expected


def _matches(doc: dict, query: dict) -> bool:
    return all(_match_op(doc.get(field), cond) for field, cond in query.items())


class _Cursor:
    def __init__(self, docs):
        self._docs = docs

    def sort(self, spec):
        # Apply each (field, direction) pair from least to most significant.
        for field, direction in reversed(spec):
            self._docs.sort(key=lambda d: d.get(field), reverse=(direction == -1))
        return self

    async def to_list(self, length=None):
        return list(self._docs if length is None else self._docs[:length])


class _Result:
    def __init__(self, modified_count):
        self.modified_count = modified_count


class _FakeCollection:
    def __init__(self, docs):
        self._docs = docs

    def find(self, query):
        return _Cursor([d for d in self._docs if _matches(d, query)])

    async def update_many(self, query, update):
        count = 0
        for doc in self._docs:
            if _matches(doc, query):
                self._apply(doc, update)
                count += 1
        return _Result(count)

    async def update_one(self, query, update):
        for doc in self._docs:
            if _matches(doc, query):
                self._apply(doc, update)
                return _Result(1)
        return _Result(0)

    @staticmethod
    def _apply(doc, update):
        for key, value in update.get("$set", {}).items():
            doc[key] = value
        for key in update.get("$unset", {}):
            doc.pop(key, None)


class _FakeDB:
    def __init__(self, docs):
        self.submissions = _FakeCollection(docs)


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

_BASE = datetime(2026, 6, 1, 12, 0, 0)


def _doc(
    nickname, win_rate=0.0, matches=0, avg_stable=0.0, age_days=0, status="active"
):
    return {
        "_id": ObjectId(),
        "metadata": {"nickname": nickname},
        "config": {"cells": [[0, 0], [1, 1]]},
        "status": status,
        "win_rate": win_rate,
        "matches_played": matches,
        "avg_stable_generation": avg_stable,
        "created_at": _BASE + timedelta(days=age_days),
    }


# ---------------------------------------------------------------------------
# Mode A
# ---------------------------------------------------------------------------


async def _mode_a_best_per_nickname():
    strong = _doc("VibeMaster", win_rate=0.31, matches=40, avg_stable=80, age_days=1)
    weak = _doc("VibeMaster", win_rate=0.12, matches=40, avg_stable=90, age_days=2)
    unrated = _doc("  VIBEMASTER  ", win_rate=0.0, matches=0, age_days=3)
    db = _FakeDB([weak, unrated, strong])

    kept, affected = await select_mode_a(db)
    assert len(kept) == 1, kept
    assert kept[0]["submission_id"] == str(strong["_id"]), "rated/best must be kept"
    affected_ids = {a["submission_id"] for a in affected}
    assert affected_ids == {str(weak["_id"]), str(unrated["_id"])}, affected_ids
    print("PASS: mode A keeps best per nickname (rated beats unrated, normalized)")


async def _mode_a_pure_tie_newest_wins():
    older = _doc("Tie", win_rate=0.2, matches=10, avg_stable=50, age_days=1)
    newer = _doc("Tie", win_rate=0.2, matches=10, avg_stable=50, age_days=5)
    db = _FakeDB([older, newer])
    kept, affected = await select_mode_a(db)
    assert kept[0]["submission_id"] == str(newer["_id"]), "newest wins on pure tie"
    assert affected[0]["submission_id"] == str(older["_id"])
    print("PASS: mode A pure-tie keeps newest")


async def _mode_a_singleton_noop():
    solo = _doc("Solo", win_rate=0.5, matches=5)
    db = _FakeDB([solo])
    kept, affected = await select_mode_a(db)
    assert len(kept) == 1 and affected == [], (kept, affected)
    print("PASS: mode A no-op for singleton nickname")


async def _mode_a_ignores_removed():
    a = _doc("Dup", win_rate=0.3, matches=10)
    b = _doc("Dup", win_rate=0.1, matches=10)
    gone = _doc("Dup", win_rate=0.9, matches=10, status="removed")
    db = _FakeDB([a, b, gone])
    kept, affected = await select_mode_a(db)
    ids = {kept[0]["submission_id"], *(x["submission_id"] for x in affected)}
    assert str(gone["_id"]) not in ids, "removed rows must be excluded from selection"
    print("PASS: mode A excludes already-removed rows")


# ---------------------------------------------------------------------------
# Mode B
# ---------------------------------------------------------------------------


async def _mode_b_normalized_incl_unrated():
    r1 = _doc("Spammer", win_rate=0.4, matches=20)
    r2 = _doc(" spammer ", win_rate=0.0, matches=0)  # normalized match, unrated
    other = _doc("Innocent", win_rate=0.5, matches=20)
    db = _FakeDB([r1, r2, other])
    affected = await select_mode_b(db, "SPAMMER")
    ids = {a["submission_id"] for a in affected}
    assert ids == {str(r1["_id"]), str(r2["_id"])}, ids
    print("PASS: mode B normalized matching includes unrated, excludes others")


# ---------------------------------------------------------------------------
# Mode C
# ---------------------------------------------------------------------------


async def _mode_c_rank_resolution_and_bounds():
    top = _doc("First", win_rate=0.9, matches=30, avg_stable=40)
    mid = _doc("Second", win_rate=0.5, matches=30, avg_stable=40)
    low = _doc("Third", win_rate=0.1, matches=30, avg_stable=40)
    unranked = _doc("NoMatches", win_rate=0.0, matches=0)
    db = _FakeDB([mid, low, top, unranked])

    rank2 = await select_mode_c(db, 2)
    assert rank2[0]["submission_id"] == str(mid["_id"]), "rank 2 must resolve to Second"

    for bad in (0, 4):
        try:
            await select_mode_c(db, bad)
            assert False, f"rank {bad} should raise ValueError"
        except ValueError:
            pass
    print("PASS: mode C resolves rank and rejects out-of-range")


# ---------------------------------------------------------------------------
# Mutations
# ---------------------------------------------------------------------------


async def _soft_delete_idempotent_and_restore():
    a = _doc("X", win_rate=0.2, matches=5)
    b = _doc("X", win_rate=0.1, matches=5)
    db = _FakeDB([a, b])

    n1 = await soft_delete_ids(db, [str(a["_id"]), str(b["_id"])], REASON_MODE_A)
    assert n1 == 2, n1
    assert a["status"] == "removed" and a["removed_reason"] == REASON_MODE_A
    first_removed_at = a["removed_at"]

    # Re-running must not re-touch already-removed rows (idempotency).
    n2 = await soft_delete_ids(db, [str(a["_id"]), str(b["_id"])], REASON_MODE_A)
    assert n2 == 0, n2
    assert a["removed_at"] == first_removed_at, "removed_at must not be overwritten"

    removed = await list_removed(db)
    assert len(removed) == 2 and "removed_reason" in removed[0]

    ok = await restore_id(db, str(a["_id"]))
    assert ok and a["status"] == "active"
    assert "removed_at" not in a and "removed_reason" not in a
    assert await restore_id(db, str(a["_id"])) is False, "restore is idempotent"
    print("PASS: soft_delete idempotency + restore round-trip")


async def _main():
    await _mode_a_best_per_nickname()
    await _mode_a_pure_tie_newest_wins()
    await _mode_a_singleton_noop()
    await _mode_a_ignores_removed()
    await _mode_b_normalized_incl_unrated()
    await _mode_c_rank_resolution_and_bounds()
    await _soft_delete_idempotent_and_restore()
    print("\nAll admin_service tests passed.")


if __name__ == "__main__":
    asyncio.run(_main())
