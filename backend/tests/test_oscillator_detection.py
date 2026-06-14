"""
Unit tests for the oscillator detection functions in worker.py.
Run with: python3 backend/tests/test_oscillator_detection.py
"""

# KI-Agent unterstützt
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "backend"))

import numpy as np
from app.worker import (
    _step_numpy,
    filter_highlights_by_oscillation,
    is_oscillating_match,
)

# ---------------------------------------------------------------------------
# _step_numpy tests
# ---------------------------------------------------------------------------


def test_step_blinker_rotates():
    """A horizontal blinker must become a vertical blinker after one step."""
    import numpy as np
    from app.worker import _TEAM_RED

    g = np.zeros((10, 10), dtype=np.int8)
    g[5, 4] = g[5, 5] = g[5, 6] = _TEAM_RED  # horizontal blinker
    g2 = _step_numpy(g)
    assert g2[4, 5] == _TEAM_RED, "cell (4,5) should be born"
    assert g2[5, 5] == _TEAM_RED, "cell (5,5) should survive"
    assert g2[6, 5] == _TEAM_RED, "cell (6,5) should be born"
    assert g2[5, 4] == 0, "cell (5,4) should die"
    assert g2[5, 6] == 0, "cell (5,6) should die"
    print("PASS: test_step_blinker_rotates")


def test_step_block_stable():
    """A 2x2 block is a still-life: one step must leave it unchanged."""
    from app.worker import _TEAM_RED

    g = np.zeros((6, 6), dtype=np.int8)
    g[2, 2] = g[2, 3] = g[3, 2] = g[3, 3] = _TEAM_RED
    g2 = _step_numpy(g)
    assert np.array_equal(g, g2), "2x2 block must be a still-life"
    print("PASS: test_step_block_stable")


def test_step_birth_majority_red():
    """Dead cell with 2 red + 1 blue neighbors must be born as RED."""
    from app.worker import _TEAM_BLUE, _TEAM_RED

    g = np.zeros((5, 5), dtype=np.int8)
    # Dead center cell (2,2): neighbors (1,1)=RED, (1,2)=RED, (1,3)=BLUE
    g[1, 1] = _TEAM_RED
    g[1, 2] = _TEAM_RED
    g[1, 3] = _TEAM_BLUE
    g2 = _step_numpy(g)
    assert g2[2, 2] == _TEAM_RED, f"Expected RED birth at (2,2), got {g2[2,2]}"
    print("PASS: test_step_birth_majority_red")


def test_step_birth_majority_blue():
    """Dead cell with 1 red + 2 blue neighbors must be born as BLUE."""
    from app.worker import _TEAM_BLUE, _TEAM_RED

    g = np.zeros((5, 5), dtype=np.int8)
    g[1, 1] = _TEAM_RED
    g[1, 2] = _TEAM_BLUE
    g[1, 3] = _TEAM_BLUE
    g2 = _step_numpy(g)
    assert g2[2, 2] == _TEAM_BLUE, f"Expected BLUE birth at (2,2), got {g2[2,2]}"
    print("PASS: test_step_birth_majority_blue")


# ---------------------------------------------------------------------------
# is_oscillating_match tests
# ---------------------------------------------------------------------------


def test_period2_blinker_red():
    """Horizontal blinker in red seed → period-2 oscillator → True."""
    red_seed = [0] * 64
    red_seed[3 * 8 + 2] = red_seed[3 * 8 + 3] = red_seed[3 * 8 + 4] = (
        1  # row 3, cols 2-4
    )
    blue_seed = [0] * 64
    result = is_oscillating_match(red_seed, blue_seed, 200)
    assert result is True, f"Expected True for period-2 blinker, got {result}"
    print("PASS: test_period2_blinker_red")


def test_period2_blinker_blue():
    """Horizontal blinker in blue seed → period-2 oscillator → True."""
    red_seed = [0] * 64
    blue_seed = [0] * 64
    blue_seed[3 * 8 + 2] = blue_seed[3 * 8 + 3] = blue_seed[3 * 8 + 4] = 1
    result = is_oscillating_match(red_seed, blue_seed, 200)
    assert result is True, f"Expected True for blue blinker, got {result}"
    print("PASS: test_period2_blinker_blue")


def test_dead_grid():
    """All-dead grid is static from generation 0 → False."""
    result = is_oscillating_match([0] * 64, [0] * 64, 200)
    assert result is False, f"Expected False for dead grid, got {result}"
    print("PASS: test_dead_grid")


def test_single_cell_dies():
    """A single cell has 0 neighbors, dies at gen 1, grid static → False."""
    red_seed = [0] * 64
    red_seed[0] = 1
    result = is_oscillating_match(red_seed, [0] * 64, 200)
    assert result is False, f"Expected False for single cell, got {result}"
    print("PASS: test_single_cell_dies")


def test_stable_block_not_oscillator():
    """A 2x2 still-life block is static (period-1), not a period-2..5
    oscillator → False."""
    red_seed = [0] * 64
    red_seed[0 * 8 + 0] = red_seed[0 * 8 + 1] = 1
    red_seed[1 * 8 + 0] = red_seed[1 * 8 + 1] = 1
    result = is_oscillating_match(red_seed, [0] * 64, 200)
    assert result is False, f"Expected False for stable 2x2 block, got {result}"
    print("PASS: test_stable_block_not_oscillator")


# ---------------------------------------------------------------------------
# filter_highlights_by_oscillation tests
# ---------------------------------------------------------------------------


def _make_highlight(
    blinker_in_red: bool, blinker_in_blue: bool, name: str, score: float
) -> dict:
    rs = [0] * 64
    bs = [0] * 64
    if blinker_in_red:
        rs[3 * 8 + 2] = rs[3 * 8 + 3] = rs[3 * 8 + 4] = 1
    if blinker_in_blue:
        bs[3 * 8 + 2] = bs[3 * 8 + 3] = bs[3 * 8 + 4] = 1
    return {
        "red_seed": rs,
        "blue_seed": bs,
        "red_name": name,
        "blue_name": "Opponent",
        "metric_value": score,
    }


def test_filter_non_oscillating_first():
    """Non-oscillating match must appear before oscillating match in output."""
    highlights = [
        _make_highlight(True, False, "oscillator", 9000.0),  # oscillator, high score
        _make_highlight(False, False, "dynamic", 5000.0),  # non-oscillator, lower score
    ]
    result = filter_highlights_by_oscillation(highlights, 200)
    assert (
        result[0]["red_name"] == "dynamic"
    ), f"Expected 'dynamic' first, got {result[0]['red_name']}"
    assert (
        result[1]["red_name"] == "oscillator"
    ), f"Expected 'oscillator' second, got {result[1]['red_name']}"
    print("PASS: test_filter_non_oscillating_first")


def test_filter_preserves_order_within_groups():
    """Within each group, higher metric_value must come first (input order
    preserved)."""
    highlights = [
        _make_highlight(True, False, "osc_high", 9000.0),
        _make_highlight(True, False, "osc_low", 7000.0),
        _make_highlight(False, False, "dyn_high", 6000.0),
        _make_highlight(False, False, "dyn_low", 4000.0),
    ]
    result = filter_highlights_by_oscillation(highlights, 200)
    assert (
        result[0]["red_name"] == "dyn_high"
    ), f"Expected 'dyn_high' at [0], got {result[0]['red_name']}"
    assert (
        result[1]["red_name"] == "dyn_low"
    ), f"Expected 'dyn_low'  at [1], got {result[1]['red_name']}"
    assert (
        result[2]["red_name"] == "osc_high"
    ), f"Expected 'osc_high' at [2], got {result[2]['red_name']}"
    assert (
        result[3]["red_name"] == "osc_low"
    ), f"Expected 'osc_low'  at [3], got {result[3]['red_name']}"
    print("PASS: test_filter_preserves_order_within_groups")


def test_filter_all_oscillating_returns_all():
    """If all candidates oscillate, all must still be returned (soft fallback)."""
    highlights = [
        _make_highlight(True, False, f"osc_{i}", float(9000 - i * 100))
        for i in range(4)
    ]
    result = filter_highlights_by_oscillation(highlights, 200)
    assert len(result) == 4, f"Expected 4 results, got {len(result)}"
    print("PASS: test_filter_all_oscillating_returns_all")


def test_filter_empty_input():
    """Empty highlight list must return an empty list."""
    result = filter_highlights_by_oscillation([], 200)
    assert result == [], f"Expected [], got {result}"
    print("PASS: test_filter_empty_input")


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    print("=== _step_numpy tests ===")
    test_step_blinker_rotates()
    test_step_block_stable()
    test_step_birth_majority_red()
    test_step_birth_majority_blue()

    print("\n=== is_oscillating_match tests ===")
    test_period2_blinker_red()
    test_period2_blinker_blue()
    test_dead_grid()
    test_single_cell_dies()
    test_stable_block_not_oscillator()

    print("\n=== filter_highlights_by_oscillation tests ===")
    test_filter_non_oscillating_first()
    test_filter_preserves_order_within_groups()
    test_filter_all_oscillating_returns_all()
    test_filter_empty_input()

    print("\nAll oscillator detection tests passed.")
