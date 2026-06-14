import asyncio
import json
import os
import secrets
import tempfile
from datetime import datetime, timedelta

from bson import ObjectId
from bson.errors import InvalidId
from pymongo import ReturnDocument

from .database import get_db
from .auth_utils import normalize_nickname, hash_recovery_code
from .grid_utils import cells_to_grid
from .validators import validate_biotope_rules
from .models import (
    Config,
    DuelParticipant,
    DuelResultEmbed,
    DuelRoom,
    Metadata,
    Submission,
)

# KI-Agent unterstützt: Shoulder-Duel service layer (ADR-0028 / DEV_TECH_DESIGN §4.2).
# Room lifecycle (create / join), with later phases adding lock-in, the headless
# referee, rematch, and the personal record aggregation.

# A duel room lives at most this long before the MongoDB TTL index reaps it.
ROOM_TTL = timedelta(minutes=30)

# Mirrors the Crockford Base32 alphabet from auth_utils (no confusable I, L, O, U)
# so the 4-char fallback code is transcribable by hand at a booth.
_ROOM_CODE_ALPHABET = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"

# The deterministic referee. Checked in several locations for Docker/local
# flexibility, mirroring worker.py's lookup of the hyper-worker binary.
_HEADLESS_PATHS = [
    "./build/biotope_headless",
    "/app/build/biotope_headless",
    "./biotope_headless",
]


# KI-Agent unterstützt: Domain error carrying the deterministic HTTP contract
# (ADR-0027): the router maps `.status` to 404 / 409 / 410 without a catch-all 500.
class DuelError(Exception):
    def __init__(self, error: str, status: int):
        super().__init__(error)
        self.error = error
        self.status = status


# KI-Agent unterstützt
def generate_room_id() -> str:
    """URL-safe room id used as the QR payload."""
    return secrets.token_urlsafe(8)


# KI-Agent unterstützt
def generate_room_code() -> str:
    """Short, human-transcribable fallback code (Crockford-style)."""
    return "".join(secrets.choice(_ROOM_CODE_ALPHABET) for _ in range(4))


# KI-Agent unterstützt
def _strip_id(doc: dict) -> dict:
    """Drop the Mongo ObjectId so the room is JSON-serializable for the API."""
    if doc is not None:
        doc.pop("_id", None)
    return doc


# KI-Agent unterstützt
def project_room(room: dict) -> dict:
    """Hidden-choice projection (DEV_TECH_DESIGN §6 — the critical security rule).

    The room-state poll must never expose a participant's chosen `config`/
    `config_id`, or an opponent could read the API to peek before locking. The
    canonical seeds are revealed only inside `result` once the match is computed.
    Applied server-side, not client-side.
    """
    safe = dict(room)
    safe.pop("_id", None)
    # The replay frames are large; they are fetched once via the frames endpoint,
    # never on the short-poll. Strip them here just like the hidden configs.
    safe.pop("frames", None)
    for slot in ("red", "blue"):
        participant = safe.get(slot)
        if participant:
            participant = dict(participant)
            participant.pop("config", None)
            participant.pop("config_id", None)
            safe[slot] = participant
    return safe


# KI-Agent unterstützt
async def get_room(room_id: str) -> dict:
    """Fetch a room for the short-poll, applying the hidden-choice projection."""
    db = get_db()
    room = await db.duel_rooms.find_one({"room_id": room_id})
    if room is None:
        raise DuelError("room_not_found", 404)
    if room["status"] == "expired" or room["expires_at"] < datetime.utcnow():
        raise DuelError("room_expired", 410)
    return project_room(room)


# KI-Agent unterstützt
async def get_room_frames(room_id: str) -> dict:
    """Return the one-shot deterministic replay payload {rows, cols, frames:[...]}.

    Fetched once by each phone when the VS splash starts, kept off the short-poll
    (project_room strips it). 404 until a match has produced frames (ADR-0028 §2.2).
    """
    db = get_db()
    room = await db.duel_rooms.find_one({"room_id": room_id})
    if room is None:
        raise DuelError("room_not_found", 404)
    frames = room.get("frames")
    if not frames:
        raise DuelError("frames_not_ready", 404)
    return frames


# KI-Agent unterstützt
async def resolve_room_code(room_code: str) -> dict:
    """Map a human-entered 4-char code to its room_id (manual join fallback).

    The QR carries the full room_id, but a player can also type the short code
    at a booth. Codes only need to be unique among *live* rooms (the TTL index
    reaps old ones), so this returns the newest non-expired room for the code.
    """
    db = get_db()
    code = (room_code or "").strip().upper()
    room = await db.duel_rooms.find_one(
        {
            "room_code": code,
            "status": {"$ne": "expired"},
            "expires_at": {"$gt": datetime.utcnow()},
        },
        sort=[("created_at", -1)],
    )
    if room is None:
        raise DuelError("room_not_found", 404)
    return {"room_id": room["room_id"]}


# KI-Agent unterstützt
async def get_my_configs(player_id: str) -> list:
    """Return a player's own active submissions as choosable duel configs.

    Each entry is {config_id, nickname, seed} where seed is the dense 8x8 grid
    (cells_to_grid) used to render the selectable icon in the choice UI.
    """
    db = get_db()
    cursor = db.submissions.find({"metadata.player_id": player_id, "status": "active"})
    subs = await cursor.to_list(length=100)
    configs = []
    for s in subs:
        configs.append(
            {
                "config_id": str(s["_id"]),
                "nickname": s["metadata"]["nickname"],
                "seed": cells_to_grid(s.get("config", {}).get("cells", [])),
            }
        )
    return configs


# KI-Agent unterstützt
async def _resolve_is_guest(db, nickname: str) -> bool:
    """A nickname is a guest unless it is backed by a claimed player (ADR-0027)."""
    norm = normalize_nickname(nickname)
    player = await db.players.find_one({"nickname_normalized": norm})
    return player is None


# KI-Agent unterstützt
async def create_room(player_id: str, nickname: str) -> dict:
    """Create a new duel room; the creator takes the RED slot (status=waiting)."""
    db = get_db()
    is_guest = await _resolve_is_guest(db, nickname)
    red = DuelParticipant(player_id=player_id, nickname=nickname, is_guest=is_guest)
    room = DuelRoom(
        room_id=generate_room_id(),
        room_code=generate_room_code(),
        status="waiting",
        red=red,
        expires_at=datetime.utcnow() + ROOM_TTL,
    )
    doc = room.model_dump()
    await db.duel_rooms.insert_one(doc)
    return _strip_id(doc)


# KI-Agent unterstützt
async def join_room(room_id: str, player_id: str, nickname: str) -> dict:
    """Join a room as BLUE and transition waiting -> choosing.

    Idempotent for a player already in the room (a re-join after a dropped poll
    must not 409). Rejects a full room and a missing/expired room.
    """
    db = get_db()
    room = await db.duel_rooms.find_one({"room_id": room_id})
    if room is None:
        raise DuelError("room_not_found", 404)
    if room["status"] == "expired" or room["expires_at"] < datetime.utcnow():
        raise DuelError("room_expired", 410)

    # Idempotent re-join: the creator or an already-joined blue just gets the room.
    if room["red"]["player_id"] == player_id:
        return _strip_id(room)
    if room.get("blue") is not None:
        if room["blue"]["player_id"] == player_id:
            return _strip_id(room)
        raise DuelError("room_full", 409)

    is_guest = await _resolve_is_guest(db, nickname)
    blue = DuelParticipant(player_id=player_id, nickname=nickname, is_guest=is_guest)

    # Conditional update guards against a concurrent join filling the slot first.
    updated = await db.duel_rooms.find_one_and_update(
        {"room_id": room_id, "blue": None},
        {"$set": {"blue": blue.model_dump(), "status": "choosing"}},
        return_document=ReturnDocument.AFTER,
    )
    if updated is None:
        # Lost the race: re-fetch to decide between idempotent re-join and full.
        current = await db.duel_rooms.find_one({"room_id": room_id})
        if current and current.get("blue", {}).get("player_id") == player_id:
            return _strip_id(current)
        raise DuelError("room_full", 409)
    return _strip_id(updated)


# KI-Agent unterstützt
def _slot_for(room: dict, player_id: str) -> str:
    """Resolve which slot a player occupies, or reject if they are not in the room."""
    if room["red"]["player_id"] == player_id:
        return "red"
    blue = room.get("blue")
    if blue and blue["player_id"] == player_id:
        return "blue"
    raise DuelError("not_in_room", 403)


# KI-Agent unterstützt
async def _resolve_config(db, player_id, nickname, config, config_id) -> tuple:
    """Resolve a lock-in to (stored_config_dict, config_id).

    A `config_id` references one of the player's submissions (seed read from
    `submissions`); an inline `config` is validated with the existing fair-play
    rules before storing (the guest quick-draw path, FR-4). Exactly one is needed.
    """
    if config_id:
        try:
            oid = ObjectId(config_id)
        except (InvalidId, TypeError):
            raise DuelError("config_not_found", 404)
        sub = await db.submissions.find_one({"_id": oid})
        if sub is None:
            raise DuelError("config_not_found", 404)
        cells = sub.get("config", {}).get("cells", [])
        return ({"cells": cells}, config_id)
    if config is not None:
        # Validate the inline config exactly as the submission path does (400 on bad).
        submission = Submission(
            metadata=Metadata(player_id=player_id, nickname=nickname), config=config
        )
        validate_biotope_rules(submission)
        return (config.model_dump(), None)
    raise ValueError("Either config or config_id is required")


# KI-Agent unterstützt
async def lock_choice(
    room_id: str, player_id: str, config=None, config_id=None
) -> dict:
    """Store a participant's hidden choice; the second lock triggers the match.

    The hidden config is written behind a conditional update guarding this slot's
    `locked` flag, and the `choosing -> computing` transition is itself a guarded
    update so only the lock that observes the *second* commit runs `run_match`
    (no double computation, DEV_TECH_DESIGN §4.2).
    """
    db = get_db()
    room = await db.duel_rooms.find_one({"room_id": room_id})
    if room is None:
        raise DuelError("room_not_found", 404)
    if room["status"] == "expired" or room["expires_at"] < datetime.utcnow():
        raise DuelError("room_expired", 410)
    slot = _slot_for(room, player_id)
    if room["status"] != "choosing":
        raise DuelError("not_choosing", 409)

    stored_config, resolved_id = await _resolve_config(
        db, player_id, room[slot]["nickname"], config, config_id
    )

    # Commit the hidden choice only if this slot has not already locked this round.
    updated = await db.duel_rooms.find_one_and_update(
        {"room_id": room_id, "status": "choosing", f"{slot}.locked": False},
        {
            "$set": {
                f"{slot}.config": stored_config,
                f"{slot}.config_id": resolved_id,
                f"{slot}.locked": True,
            }
        },
        return_document=ReturnDocument.AFTER,
    )
    if updated is None:
        # Already locked or status moved on; treat as idempotent.
        updated = await db.duel_rooms.find_one({"room_id": room_id})

    blue = updated.get("blue")
    if updated["red"]["locked"] and blue and blue["locked"]:
        # Guarded transition: exactly one caller flips choosing -> computing.
        owner = await db.duel_rooms.find_one_and_update(
            {"room_id": room_id, "status": "choosing"},
            {"$set": {"status": "computing"}},
            return_document=ReturnDocument.AFTER,
        )
        if owner is not None:
            await run_match(owner)

    return {"locked": True, "slot": slot}


# KI-Agent unterstützt
async def _invoke_headless(red_cells: list, blue_cells: list) -> dict:
    """Run the deterministic referee once and return its parsed result JSON.

    Mirrors worker.py: write red.json/blue.json to a temp dir, locate the binary,
    invoke via create_subprocess_exec with an output path, parse the result, and
    always clean up the temp files.
    """
    binary = next((p for p in _HEADLESS_PATHS if os.path.exists(p)), None)
    if binary is None:
        raise RuntimeError(
            f"biotope_headless not found in {_HEADLESS_PATHS}; run 'make' first"
        )

    tmpdir = tempfile.mkdtemp(prefix="duel_")
    red_path = os.path.join(tmpdir, "red.json")
    blue_path = os.path.join(tmpdir, "blue.json")
    out_path = os.path.join(tmpdir, "out.json")
    frames_path = os.path.join(tmpdir, "frames.json")
    try:
        with open(red_path, "w") as f:
            json.dump({"metadata": {"player_id": "red", "nickname": "red"},
                       "config": {"cells": red_cells}}, f)
        with open(blue_path, "w") as f:
            json.dump({"metadata": {"player_id": "blue", "nickname": "blue"},
                       "config": {"cells": blue_cells}}, f)

        # KI-Agent unterstützt: the extra frames_path argument makes headless emit a
        # per-generation replay payload alongside the result (ADR-0028 §2.2).
        process = await asyncio.create_subprocess_exec(
            binary,
            red_path,
            blue_path,
            out_path,
            frames_path,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )
        _, stderr = await process.communicate()
        if process.returncode != 0:
            raise RuntimeError(f"headless failed: {stderr.decode()}")
        with open(out_path) as f:
            result = json.load(f)
        # Frames are best-effort: a result without them still shows the verdict.
        try:
            with open(frames_path) as f:
                result["_frames"] = json.load(f)
        except (OSError, json.JSONDecodeError):
            result["_frames"] = None
        return result
    finally:
        for path in (red_path, blue_path, out_path, frames_path):
            try:
                os.remove(path)
            except OSError:
                pass
        try:
            os.rmdir(tmpdir)
        except OSError:
            pass


# KI-Agent unterstützt
async def run_match(room: dict) -> None:
    """Compute the match once, store the result on the room, and record the duel.

    The seeds are embedded in the room so each phone can run its local replay
    without a second round-trip; a permanent `duels` document (keyed by player
    identity) backs the "My Duels" record (DEV_TECH_DESIGN §3.2 / §4.2).
    """
    db = get_db()
    red, blue = room["red"], room["blue"]
    red_cells = red["config"]["cells"]
    blue_cells = blue["config"]["cells"]

    result = await _invoke_headless(red_cells, blue_cells)
    red_seed = cells_to_grid(red_cells)
    blue_seed = cells_to_grid(blue_cells)

    embed = DuelResultEmbed(
        winner=result["winner"],
        generations=result["generations"],
        red_population=result["red"]["population"],
        blue_population=result["blue"]["population"],
        red_seed=red_seed,
        blue_seed=blue_seed,
    ).model_dump()

    # KI-Agent unterstützt: the replay frames live at the room top-level (not inside
    # `result`), so the 1.5s poll projection can strip them and serve them only once
    # via GET /rooms/{id}/frames. The permanent `duels` record stays lean (no frames).
    frames = result.get("_frames")
    await db.duel_rooms.update_one(
        {"room_id": room["room_id"]},
        {"$set": {"status": "result", "result": embed, "frames": frames}},
    )

    # Permanent per-player record (keyed by identity, not submission id).
    duel_doc = {
        "room_id": room["room_id"],
        "timestamp": datetime.utcnow(),
        "red": {
            "player_id": red["player_id"],
            "nickname": red["nickname"],
            "config_id": red.get("config_id"),
            "seed": red_seed,
        },
        "blue": {
            "player_id": blue["player_id"],
            "nickname": blue["nickname"],
            "config_id": blue.get("config_id"),
            "seed": blue_seed,
        },
        "winner": embed["winner"],
        "generations": embed["generations"],
        "red_population": embed["red_population"],
        "blue_population": embed["blue_population"],
    }
    await db.duels.insert_one(duel_doc)


# KI-Agent unterstützt
async def request_rematch(room_id: str, player_id: str) -> dict:
    """Flag a rematch for one slot; when both agree, reset the room to choosing.

    A rematch is only meaningful once a result exists. The first request sets
    that slot's flag; the request that observes *both* flags set performs the
    guarded reset (clear locked/config/config_id/result, status -> choosing) so
    the same two participants replay in the same room. Idempotent per slot, and
    the result -> choosing transition is guarded so it happens exactly once.
    """
    db = get_db()
    room = await db.duel_rooms.find_one({"room_id": room_id})
    if room is None:
        raise DuelError("room_not_found", 404)
    if room["status"] == "expired" or room["expires_at"] < datetime.utcnow():
        raise DuelError("room_expired", 410)
    slot = _slot_for(room, player_id)
    if room["status"] != "result":
        raise DuelError("not_in_result", 409)

    updated = await db.duel_rooms.find_one_and_update(
        {"room_id": room_id, "status": "result"},
        {"$set": {f"rematch.{slot}": True}},
        return_document=ReturnDocument.AFTER,
    )
    if updated is None:
        # Status moved on (e.g. expired); re-fetch for an idempotent response.
        updated = await db.duel_rooms.find_one({"room_id": room_id})

    rematch = updated.get("rematch") or {"red": False, "blue": False}
    if rematch.get("red") and rematch.get("blue"):
        # Guarded reset: exactly one caller flips result -> choosing for round N+1.
        reset = await db.duel_rooms.find_one_and_update(
            {"room_id": room_id, "status": "result"},
            {
                "$set": {
                    "status": "choosing",
                    "result": None,
                    "frames": None,
                    "rematch": {"red": False, "blue": False},
                    "red.locked": False,
                    "red.config": None,
                    "red.config_id": None,
                    "blue.locked": False,
                    "blue.config": None,
                    "blue.config_id": None,
                }
            },
            return_document=ReturnDocument.AFTER,
        )
        if reset is not None:
            updated = reset

    return {
        "rematch": updated.get("rematch") or {"red": False, "blue": False},
        "status": updated["status"],
    }


# KI-Agent unterstützt
async def get_my_record(player_id: str, recovery_code: str = None) -> dict:
    """Aggregate a player's duels into head-to-head, best weapon, and a tally.

    With a `recovery_code`, the identity is verified against the ADR-0027
    `players.recovery_code_hash` before the record is returned (cross-device
    recovery, FR-19). The code is compared only as a SHA-256 hash.
    """
    db = get_db()
    if recovery_code:
        player = await db.players.find_one({"player_id": player_id})
        if player and player.get("recovery_code_hash"):
            if hash_recovery_code(recovery_code) != player["recovery_code_hash"]:
                raise DuelError("invalid_recovery_code", 403)

    cursor = db.duels.find(
        {"$or": [{"red.player_id": player_id}, {"blue.player_id": player_id}]}
    )
    duels = await cursor.to_list(length=1000)

    head_to_head = {}  # opponent nickname -> {wins, losses, draws}
    weapons = {}  # config_id (or quick-draw) -> {config_id, seed, wins}
    wins = losses = draws = 0

    for duel in duels:
        if duel["red"]["player_id"] == player_id:
            me, opponent, my_team = duel["red"], duel["blue"], "red"
        else:
            me, opponent, my_team = duel["blue"], duel["red"], "blue"

        hh = head_to_head.setdefault(
            opponent["nickname"], {"wins": 0, "losses": 0, "draws": 0}
        )
        won = False
        if duel["winner"] == "draw":
            draws += 1
            hh["draws"] += 1
        elif duel["winner"] == my_team:
            wins += 1
            hh["wins"] += 1
            won = True
        else:
            losses += 1
            hh["losses"] += 1

        # Best-weapon attribution by source config_id (inline picks share one key).
        key = me.get("config_id") or "quick-draw"
        weapon = weapons.setdefault(
            key, {"config_id": me.get("config_id"), "seed": me.get("seed"), "wins": 0}
        )
        if won:
            weapon["wins"] += 1

    best_weapon = None
    winning = [w for w in weapons.values() if w["wins"] > 0]
    if winning:
        best_weapon = max(winning, key=lambda w: w["wins"])

    head = [{"opponent": name, **rec} for name, rec in head_to_head.items()]
    return {
        "head_to_head": head,
        "best_weapon": best_weapon,
        "tally": {"w": wins, "l": losses, "d": draws},
    }
