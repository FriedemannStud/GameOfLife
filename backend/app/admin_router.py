from fastapi import APIRouter, Depends, HTTPException

from . import admin_service
from .admin_auth import require_admin
from .database import get_db
from .models import AdminDeleteRequest, AdminRestoreRequest

# KI-Agent unterstützt: Admin config-deletion REST surface (ADR-0035 §4.1).
# The whole router is gated by `require_admin`, so every endpoint demands a valid
# X-Admin-Key. Selection + mutation logic lives in admin_service; this layer only
# dispatches on the mode, validates mode-specific parameters, and shapes the JSON.

router = APIRouter(
    prefix="/api/admin",
    tags=["admin"],
    dependencies=[Depends(require_admin)],
)


@router.post("/delete")
async def delete_configs(req: AdminDeleteRequest):
    # KI-Agent unterstützt: unified delete (dry-run or execute). Selection is run
    # for both paths so the preview can never disagree with what is deleted (NFR-3).
    db = get_db()
    mode = (req.mode or "").upper()
    kept = None

    if mode == "A":
        kept, affected = await admin_service.select_mode_a(db)
        reason = admin_service.REASON_MODE_A
    elif mode == "B":
        if not req.nickname or not req.nickname.strip():
            raise HTTPException(status_code=400, detail={"error": "nickname_required"})
        affected = await admin_service.select_mode_b(db, req.nickname)
        reason = admin_service.REASON_MODE_B
    elif mode == "C":
        if req.rank is None:
            raise HTTPException(status_code=400, detail={"error": "rank_required"})
        try:
            affected = await admin_service.select_mode_c(db, req.rank)
        except ValueError:
            raise HTTPException(status_code=400, detail={"error": "rank_out_of_range"})
        reason = admin_service.REASON_MODE_C
    else:
        raise HTTPException(status_code=400, detail={"error": "invalid_mode"})

    if req.dry_run:
        removed_count = len(affected)
    else:
        ids = [a["submission_id"] for a in affected]
        removed_count = await admin_service.soft_delete_ids(db, ids, reason)

    response = {
        "mode": mode,
        "dry_run": req.dry_run,
        "removed_count": removed_count,
        "affected": affected,
    }
    if mode == "A":
        response["kept"] = kept
    return response


@router.get("/removed")
async def list_removed():
    # KI-Agent unterstützt: trash listing, newest first (FR-8).
    db = get_db()
    return {"removed": await admin_service.list_removed(db)}


@router.post("/restore")
async def restore_config(req: AdminRestoreRequest):
    # KI-Agent unterstützt: flip a removed submission back to active (FR-9).
    db = get_db()
    ok = await admin_service.restore_id(db, req.submission_id)
    if not ok:
        raise HTTPException(status_code=404, detail={"error": "not_found"})
    return {"status": "restored", "submission_id": req.submission_id}
