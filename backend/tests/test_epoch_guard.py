"""
Unit tests for the roster-change early-exit guard helpers in worker.py
(ADR-0032 Stage 1).
Run with: python3 backend/tests/test_epoch_guard.py
"""

# KI-Agent unterstützt
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "backend"))

from app.worker import roster_fingerprint, seed_hash


def _cells(fill=0):
    """An 8x8 grid; set fill to vary one cell."""
    grid = [[0] * 8 for _ in range(8)]
    if fill:
        grid[0][0] = 1
    return grid


def _sub(_id, cells):
    return {"_id": _id, "config": {"cells": cells}}


# ---------------------------------------------------------------------------
# seed_hash tests
# ---------------------------------------------------------------------------


def test_seed_hash_deterministic():
    assert seed_hash(_cells()) == seed_hash(_cells())
    print("PASS: test_seed_hash_deterministic")


def test_seed_hash_differs_on_edit():
    assert seed_hash(_cells(fill=0)) != seed_hash(_cells(fill=1))
    print("PASS: test_seed_hash_differs_on_edit")


# ---------------------------------------------------------------------------
# roster_fingerprint tests
# ---------------------------------------------------------------------------


def test_identical_rosters_equal():
    a = [_sub("1", _cells()), _sub("2", _cells(fill=1))]
    b = [_sub("1", _cells()), _sub("2", _cells(fill=1))]
    assert roster_fingerprint(a) == roster_fingerprint(b)
    print("PASS: test_identical_rosters_equal")


def test_reordered_roster_equal():
    a = [_sub("1", _cells()), _sub("2", _cells(fill=1))]
    b = [_sub("2", _cells(fill=1)), _sub("1", _cells())]
    assert roster_fingerprint(a) == roster_fingerprint(b)
    print("PASS: test_reordered_roster_equal")


def test_edited_cell_changes_fingerprint():
    a = [_sub("1", _cells()), _sub("2", _cells())]
    b = [_sub("1", _cells()), _sub("2", _cells(fill=1))]
    assert roster_fingerprint(a) != roster_fingerprint(b)
    print("PASS: test_edited_cell_changes_fingerprint")


def test_added_submission_changes_fingerprint():
    a = [_sub("1", _cells()), _sub("2", _cells(fill=1))]
    b = a + [_sub("3", _cells())]
    assert roster_fingerprint(a) != roster_fingerprint(b)
    print("PASS: test_added_submission_changes_fingerprint")


def test_removed_submission_changes_fingerprint():
    a = [_sub("1", _cells()), _sub("2", _cells(fill=1))]
    b = [_sub("1", _cells())]
    assert roster_fingerprint(a) != roster_fingerprint(b)
    print("PASS: test_removed_submission_changes_fingerprint")


if __name__ == "__main__":
    print("=== seed_hash tests ===")
    test_seed_hash_deterministic()
    test_seed_hash_differs_on_edit()

    print("\n=== roster_fingerprint tests ===")
    test_identical_rosters_equal()
    test_reordered_roster_equal()
    test_edited_cell_changes_fingerprint()
    test_added_submission_changes_fingerprint()
    test_removed_submission_changes_fingerprint()

    print("\nAll epoch guard tests passed.")
