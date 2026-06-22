from datetime import datetime

from bson import ObjectId
from bson.errors import InvalidId

from .auth_utils import normalize_nickname
from .grid_utils import cells_to_grid

# KI-Agent unterstützt: Admin config-deletion service layer (ADR-0035 §4.2).
# Pure-ish selection + mutation helpers over the `submissions` collection. The
# selection functions are the single source of truth shared by the dry-run preview
# and the live execution (NFR-3), so the preview can never disagree with what is
# deleted. All functions take the motor db as an argument and never import the
# global connection, keeping them unit-testable against an in-memory fake.

# KI-Agent unterstützt: reason tags persisted on removed rows, one per mode.
REASON_MODE_A = "mode_a_dedup"
REASON_MODE_B = "mode_b_nickname"
REASON_MODE_C = "mode_c_rank"


# KI-Agent unterstützt
def _to_view(doc: dict) -> dict:
    """Map a submission document to the public admin response row.

    `win_rate` is stored as a 0..1 fraction; it is surfaced here as a percentage
    rounded to one decimal, matching the /api/leaderboard convention.
    """
    return {
        "submission_id": str(doc["_id"]),
        "nickname": doc.get("metadata", {}).get("nickname", ""),
        "win_rate": round(doc.get("win_rate", 0.0) * 100, 1),
        "matches_played": doc.get("matches_played", 0),
        "seed": cells_to_grid(doc.get("config", {}).get("cells", [])),
    }


# KI-Agent unterstützt
def _best_first_key(doc: dict):
    """Sort key implementing the "best" ordering (FR-3).

    Best = win_rate desc, then avg_stable_generation asc, then created_at desc
    (newest wins on a pure tie). Returned as a tuple where every component sorts
    ascending, so a plain ``sorted`` puts the best document first. A rated
    submission therefore always outranks an unrated one (win_rate == 0).
    """
    created_at = doc.get("created_at")
    created_ts = created_at.timestamp() if isinstance(created_at, datetime) else 0.0
    return (
        -doc.get("win_rate", 0.0),
        doc.get("avg_stable_generation", 0.0),
        -created_ts,
    )


# KI-Agent unterstützt
async def _fetch_non_removed(db) -> list:
    """All submissions that are not soft-deleted (active or any non-removed state)."""
    return await db.submissions.find({"status": {"$ne": "removed"}}).to_list(
        length=None
    )


# KI-Agent unterstützt
async def select_mode_a(db):
    """Mode A — global per-nickname de-duplication (FR-3).

    Returns ``(kept, affected)`` view lists. Within each normalized nickname the
    best document is kept and the rest are marked affected. Singleton nicknames
    contribute only a kept entry, never an affected one.
    """
    docs = await _fetch_non_removed(db)
    groups: dict = {}
    for doc in docs:
        nickname = doc.get("metadata", {}).get("nickname", "")
        groups.setdefault(normalize_nickname(nickname), []).append(doc)

    kept = []
    affected = []
    for members in groups.values():
        members.sort(key=_best_first_key)
        kept.append(_to_view(members[0]))
        affected.extend(_to_view(d) for d in members[1:])
    return kept, affected


# KI-Agent unterstützt
async def select_mode_b(db, nickname: str):
    """Mode B — every non-removed submission of one normalized nickname (FR-4)."""
    target = normalize_nickname(nickname)
    docs = await _fetch_non_removed(db)
    return [
        _to_view(d)
        for d in docs
        if normalize_nickname(d.get("metadata", {}).get("nickname", "")) == target
    ]


# KI-Agent unterstützt
async def select_mode_c(db, rank: int):
    """Mode C — resolve a 1-based leaderboard rank to a single submission (FR-5).

    Uses the exact query + sort of /api/leaderboard so the rank the operator sees
    maps to the same document. Raises ValueError when the rank is out of range so
    the router can return a 400. Returns a one-element affected list.
    """
    if rank < 1:
        raise ValueError("rank must be >= 1")
    query = {"status": "active", "matches_played": {"$gt": 0}}
    ranked = (
        await db.submissions.find(query)
        .sort([("win_rate", -1), ("avg_stable_generation", 1)])
        .to_list(length=None)
    )
    if rank > len(ranked):
        raise ValueError(f"rank {rank} out of range (only {len(ranked)} ranked)")
    return [_to_view(ranked[rank - 1])]


# KI-Agent unterstützt
def _to_object_ids(ids) -> list:
    """Defensively parse stringified ids to ObjectId, skipping malformed ones."""
    object_ids = []
    for sid in ids:
        try:
            object_ids.append(ObjectId(sid))
        except (InvalidId, TypeError):
            continue
    return object_ids


# KI-Agent unterstützt
async def soft_delete_ids(db, ids, reason: str) -> int:
    """Soft-delete the given submissions, returning the modified count.

    The ``status: "active"`` guard makes this idempotent: rows already removed
    (or in any non-active state) are never re-touched, so re-running a delete is
    a no-op and `removed_at` is never overwritten.
    """
    object_ids = _to_object_ids(ids)
    if not object_ids:
        return 0
    result = await db.submissions.update_many(
        {"_id": {"$in": object_ids}, "status": "active"},
        {
            "$set": {
                "status": "removed",
                "removed_at": datetime.utcnow(),
                "removed_reason": reason,
            }
        },
    )
    return result.modified_count


# KI-Agent unterstützt
async def list_removed(db) -> list:
    """Trash listing — all removed submissions, newest first (FR-8)."""
    docs = (
        await db.submissions.find({"status": "removed"})
        .sort([("removed_at", -1)])
        .to_list(length=None)
    )
    rows = []
    for doc in docs:
        view = _to_view(doc)
        view["removed_at"] = doc.get("removed_at")
        view["removed_reason"] = doc.get("removed_reason")
        rows.append(view)
    return rows


# KI-Agent unterstützt
async def restore_id(db, submission_id: str) -> bool:
    """Restore one removed submission to active (FR-9).

    Flips status removed -> active and unsets the removal metadata. Returns True
    only when a removed row with that id actually matched.
    """
    try:
        oid = ObjectId(submission_id)
    except (InvalidId, TypeError):
        return False
    result = await db.submissions.update_one(
        {"_id": oid, "status": "removed"},
        {
            "$set": {"status": "active"},
            "$unset": {"removed_at": "", "removed_reason": ""},
        },
    )
    return result.modified_count > 0
