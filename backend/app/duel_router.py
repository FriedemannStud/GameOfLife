from fastapi import APIRouter, HTTPException

from .models import CreateRoomRequest, JoinRoomRequest, LockRequest, RematchRequest
from . import duel_service
from .duel_service import DuelError

# KI-Agent unterstützt: Shoulder-Duel REST surface (ADR-0028 / DEV_TECH_DESIGN §4.1).
# A single router under /api/v1: room lifecycle lives beneath /duel/rooms, while the
# player-facing helpers (my_configs, duels) sit directly under /api/v1. Errors follow
# the ADR-0027 deterministic contract (404 / 409 / 410), never a catch-all 500.

router = APIRouter(prefix="/api/v1", tags=["duel"])


# KI-Agent unterstützt
def _http(error: DuelError) -> HTTPException:
    """Translate a domain DuelError into its deterministic HTTP response."""
    return HTTPException(status_code=error.status, detail={"error": error.error})


@router.post("/duel/rooms", status_code=201)
async def create_room(req: CreateRoomRequest):
    room = await duel_service.create_room(req.player_id, req.nickname)
    return {
        "room_id": room["room_id"],
        "room_code": room["room_code"],
        "slot": "red",
    }


@router.post("/duel/rooms/{room_id}/join")
async def join_room(room_id: str, req: JoinRoomRequest):
    try:
        room = await duel_service.join_room(room_id, req.player_id, req.nickname)
    except DuelError as e:
        raise _http(e)
    return {
        "slot": "blue",
        "red_nickname": room["red"]["nickname"],
        "blue_nickname": room["blue"]["nickname"],
    }


@router.get("/duel/rooms/by-code/{room_code}")
async def resolve_code(room_code: str):
    # KI-Agent unterstützt: resolve a typed 4-char code to its room_id so the
    # manual-entry fallback can join (the QR path already carries the room_id).
    try:
        return await duel_service.resolve_room_code(room_code)
    except DuelError as e:
        raise _http(e)


@router.get("/duel/rooms/{room_id}")
async def get_room(room_id: str):
    # KI-Agent unterstützt: short-poll state; hidden-choice projection applied in
    # the service so a chosen config is never exposed before the result.
    try:
        return await duel_service.get_room(room_id)
    except DuelError as e:
        raise _http(e)


@router.get("/duel/rooms/{room_id}/frames")
async def room_frames(room_id: str):
    # KI-Agent unterstützt: one-shot deterministic replay payload (kept off the
    # short-poll); each phone fetches it once when the VS splash starts.
    try:
        return await duel_service.get_room_frames(room_id)
    except DuelError as e:
        raise _http(e)


@router.post("/duel/rooms/{room_id}/lock")
async def lock(room_id: str, req: LockRequest):
    # KI-Agent unterstützt: hidden lock-in; the second lock triggers exactly one
    # server-side computation. 400 on a malformed/oversized inline config.
    try:
        return await duel_service.lock_choice(
            room_id, req.player_id, config=req.config, config_id=req.config_id
        )
    except DuelError as e:
        raise _http(e)
    except ValueError as e:
        raise HTTPException(status_code=400, detail=str(e))


@router.post("/duel/rooms/{room_id}/rematch")
async def rematch(room_id: str, req: RematchRequest):
    # KI-Agent unterstützt: per-slot rematch flag; both slots set -> the server
    # resets the room to `choosing` for a new round with the same participants.
    try:
        return await duel_service.request_rematch(room_id, req.player_id)
    except DuelError as e:
        raise _http(e)


@router.get("/my_configs")
async def my_configs(player_id: str):
    # KI-Agent unterstützt: a player's choosable configs are their own active
    # submissions (DEV_TECH_DESIGN §4.1), returned as seed icons for the choice UI.
    configs = await duel_service.get_my_configs(player_id)
    return {"configs": configs}


@router.get("/duels")
async def duels(player_id: str, recovery_code: str = None):
    # KI-Agent unterstützt: personal record (head-to-head, best weapon, tally).
    # An optional recovery_code authenticates cross-device retrieval (FR-19).
    try:
        return await duel_service.get_my_record(player_id, recovery_code)
    except DuelError as e:
        raise _http(e)
