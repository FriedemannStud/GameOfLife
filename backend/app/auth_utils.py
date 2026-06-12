import hashlib
import secrets

# KI-Agent unterstützt: Pure helpers for name-claiming with recovery codes (ADR-0027).
# No DB, no FastAPI — unit-testable in isolation.

# Crockford Base32 alphabet (no confusable I, L, O, U)
_ALPHABET = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"


# KI-Agent unterstützt
def normalize_nickname(nickname: str) -> str:
    """Trim + lowercase + collapse internal whitespace runs to a single space.

    This is the identity key for name ownership; it blocks trivial
    case/whitespace impersonation (e.g. "VibeMaster" vs "  vibe  master ").
    """
    return " ".join(nickname.strip().lower().split())


# KI-Agent unterstützt
def generate_recovery_code(n_chars: int = 4, prefix: str = "BIOTOP") -> str:
    """Generate a short, human-readable recovery code, e.g. "BIOTOP-7F3A".

    Uses ``secrets.choice`` over the Crockford Base32 alphabet so the code is
    transcribable by hand at a booth and free of confusable characters.
    """
    body = "".join(secrets.choice(_ALPHABET) for _ in range(n_chars))
    return f"{prefix}-{body}"


# KI-Agent unterstützt
def hash_recovery_code(code: str) -> str:
    """Return the SHA-256 hex digest of a recovery code.

    Input is normalized (strip + uppercase) so user transcription is forgiving;
    the plaintext code is never persisted, only this hash.
    """
    return hashlib.sha256(code.strip().upper().encode("utf-8")).hexdigest()
