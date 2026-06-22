import os
import secrets

from fastapi import Header, HTTPException

# KI-Agent unterstützt: Static-password admin gate (ADR-0035 §4.3 / FR-1).
# Every /api/admin/* request must carry the X-Admin-Key header matching the
# ADMIN_PASSWORD env var. The comparison is constant-time (no timing oracle) and
# fails closed: an unset/empty ADMIN_PASSWORD rejects every request.


# KI-Agent unterstützt
async def require_admin(x_admin_key: str = Header(default="")):
    """FastAPI dependency enforcing the admin password.

    Reads ADMIN_PASSWORD from the environment on every call so a redeployed
    secret takes effect without a code change. Raises 401 when the secret is
    unset/empty (fail closed) or the supplied key does not match.
    """
    admin_password = os.getenv("ADMIN_PASSWORD")
    if not admin_password or not secrets.compare_digest(x_admin_key, admin_password):
        raise HTTPException(status_code=401, detail={"error": "unauthorized"})
