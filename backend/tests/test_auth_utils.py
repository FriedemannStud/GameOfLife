import os
import re
import sys

# Add app to path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app.auth_utils import (
    generate_recovery_code,
    hash_recovery_code,
    normalize_nickname,
)

# KI-Agent unterstützt: Unit tests for the name-claiming auth helpers (ADR-0027)


def test_normalize_nickname():
    # Lowercase, trim, collapse internal whitespace
    assert normalize_nickname("VibeMaster") == "vibemaster"
    assert normalize_nickname("  VibeMaster ") == "vibemaster"
    assert normalize_nickname("vibemaster") == "vibemaster"
    # All three variants normalize to the same identity
    variants = {
        normalize_nickname("VibeMaster"),
        normalize_nickname("vibemaster"),
        normalize_nickname("  VibeMaster "),
    }
    assert len(variants) == 1
    # Collapse internal whitespace runs
    assert normalize_nickname("  Vibe   Master ") == "vibe master"
    print("OK: normalize_nickname trims, lowercases, collapses whitespace.")


def test_generate_recovery_code():
    pattern = re.compile(r"^BIOTOP-[0-9A-HJ-NP-TV-Z]{4}$")
    code = generate_recovery_code()
    assert pattern.match(code), f"Unexpected code format: {code}"
    # No confusable characters in the body
    body = code.split("-", 1)[1]
    assert not any(c in body for c in "ILOU"), f"Confusable char in {code}"
    # Two calls should (overwhelmingly likely) differ
    assert generate_recovery_code() != generate_recovery_code()
    print(f"OK: generate_recovery_code format/charset valid (e.g. {code}).")


def test_hash_recovery_code():
    # Case- and space-insensitive on input
    assert hash_recovery_code("biotop-7f3a") == hash_recovery_code(" BIOTOP-7F3A ")
    # Never equals the plaintext, and is a 64-hex digest
    h = hash_recovery_code("BIOTOP-7F3A")
    assert h != "BIOTOP-7F3A"
    assert re.match(r"^[0-9a-f]{64}$", h)
    print("OK: hash_recovery_code is case/space-insensitive and not plaintext.")


if __name__ == "__main__":
    test_normalize_nickname()
    test_generate_recovery_code()
    test_hash_recovery_code()
    print("All auth_utils tests passed.")
