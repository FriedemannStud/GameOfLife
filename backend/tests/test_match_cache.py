"""
Unit tests for the incremental match-cache pair enumeration / delta logic in
worker.py (ADR-0032 Stage 2, Phase 4).
Run with: python3 backend/tests/test_match_cache.py
"""

# KI-Agent unterstützt
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "backend"))

from app.worker import cells_to_seed64, enumerate_needed_pairs, pair_key


def _missing(needed, cached_keys):
    """Mirror the delta computation in execute_epoch."""
    return [k for k in needed if k not in cached_keys]


# ---------------------------------------------------------------------------
# enumerate_needed_pairs / missing-set tests
# ---------------------------------------------------------------------------


def test_all_distinct_pairs_for_n():
    hashes = ["a", "b", "c", "d"]
    needed = enumerate_needed_pairs(hashes)
    assert len(needed) == 6  # 4*3/2
    print("PASS: test_all_distinct_pairs_for_n")


def test_empty_cache_all_missing():
    hashes = ["a", "b", "c"]
    needed = enumerate_needed_pairs(hashes)
    assert len(_missing(needed, set())) == len(needed) == 3
    print("PASS: test_empty_cache_all_missing")


def test_full_cache_none_missing():
    hashes = ["a", "b", "c"]
    needed = enumerate_needed_pairs(hashes)
    assert _missing(needed, set(needed.keys())) == []
    print("PASS: test_full_cache_none_missing")


def test_one_new_competitor_adds_n_pairs():
    # Field of 5 fully cached, add a 6th new competitor.
    old = ["a", "b", "c", "d", "e"]
    cached = set(enumerate_needed_pairs(old).keys())
    new = old + ["f"]
    needed = enumerate_needed_pairs(new)
    missing = _missing(needed, cached)
    # The newcomer 'f' plays each of the 5 existing => exactly 5 new pairs.
    assert len(missing) == 5
    assert all("f" in k for k in missing)
    print("PASS: test_one_new_competitor_adds_n_pairs")


def test_duplicate_pattern_dedups_pairs():
    # Two players share the same pattern hash 'a'.
    hashes = ["a", "a", "b"]
    needed = enumerate_needed_pairs(hashes)
    # distinct pair_keys: {a:a}, {a:b}  -> 2, not 3
    assert len(needed) == 2
    assert pair_key("a", "a") in needed
    assert pair_key("a", "b") in needed
    print("PASS: test_duplicate_pattern_dedups_pairs")


# ---------------------------------------------------------------------------
# cells_to_seed64 tests (must mirror grid_to_bitboard in game_logic.c)
# ---------------------------------------------------------------------------


def test_cells_to_seed64_layout():
    # sparse [x, y] -> flat index y*8 + x
    seed = cells_to_seed64([[0, 0], [7, 0], [0, 1], [3, 2]])
    assert seed[0] == 1  # (x=0,y=0)
    assert seed[7] == 1  # (x=7,y=0)
    assert seed[8] == 1  # (x=0,y=1)
    assert seed[2 * 8 + 3] == 1  # (x=3,y=2)
    assert sum(seed) == 4
    assert len(seed) == 64
    print("PASS: test_cells_to_seed64_layout")


def test_cells_to_seed64_ignores_out_of_range():
    seed = cells_to_seed64([[8, 0], [0, 8], [-1, 0]])
    assert sum(seed) == 0
    print("PASS: test_cells_to_seed64_ignores_out_of_range")


if __name__ == "__main__":
    print("=== enumerate_needed_pairs / missing-set tests ===")
    test_all_distinct_pairs_for_n()
    test_empty_cache_all_missing()
    test_full_cache_none_missing()
    test_one_new_competitor_adds_n_pairs()
    test_duplicate_pattern_dedups_pairs()

    print("\n=== cells_to_seed64 tests ===")
    test_cells_to_seed64_layout()
    test_cells_to_seed64_ignores_out_of_range()

    print("\nAll match cache tests passed.")
