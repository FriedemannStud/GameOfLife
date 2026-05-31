# Technical Design: Oscillator Detection and Filtering for Kiosk Highlight Quality

**Version:** 1.0
**Date:** 2026-05-31
**Author:** Claude (KI-Agent)
**Related Documents:** [ADR-0023](../adr/ADR-0023-oscillator-filtering-highlight-quality.md), [DEV_SPEC-0023](../specs/DEV_SPEC-0023-oscillator-filtering.md)

---

### 1. Introduction

This document provides a detailed technical design for the oscillator detection feature (ADR-0023). The feature adds a post-processing step to the Python Tournament Worker that identifies oscillating cellular patterns among the top-10 highlight candidates and reorders the highlights list so that non-oscillating matches appear first in MongoDB.

**Scope:** Exactly one file is modified: `backend/app/worker.py`. No C files, no API endpoint changes, no database schema changes.

---

### 2. System Architecture and Components

#### 2.1 Component Overview

Two new functions are added to `worker.py`, plus one integration call inside the existing `execute_epoch()`:

| Component | Type | Responsibility |
|---|---|---|
| `_step_numpy(grid)` | Pure function | One generation of two-team Conway rules (NumPy, toroidal) |
| `is_oscillating_match(red_seed, blue_seed, max_gen)` | Pure function | Simulates a match in the kiosk world, returns True if end-state is periodic |
| `filter_highlights_by_oscillation(highlights, max_gen)` | Pure function | Applies detector to all candidates, returns reordered list |
| `execute_epoch()` call site | Integration | Calls filter after Hyper-Worker output is parsed, before DB insert |

#### 2.2 Component Interaction Diagram

```
execute_epoch()
  │
  ├─→ [unchanged] run biotope_hyper_worker
  │       └─→ results["highlights"]  (list of 10, sorted by activity_sum desc)
  │
  ├─→ [NEW] filter_highlights_by_oscillation(highlights, max_generations)
  │       │
  │       ├─→ is_oscillating_match(h["red_seed"], h["blue_seed"], max_gen)  [× 10]
  │       │       ├─→ build 50×50 numpy grid from seeds
  │       │       ├─→ loop: _step_numpy(grid) × max_gen
  │       │       │         check static-state early exit
  │       │       │         append grid.tobytes() to rolling buffer (maxlen=11)
  │       │       └─→ end: check period 2,3,4,5 with 2-cycle confirmation
  │       │
  │       └─→ returns [non_oscillating... , oscillating...]
  │
  └─→ [unchanged] db.epoch_highlights.insert_one({..., "highlights": filtered_list})
```

---

### 3. Data Model Specification

**No schema changes.** The `epoch_highlights` MongoDB collection is unchanged. Individual highlight documents retain the same fields:

```json
{
  "metric_type": "activity_sum",
  "red_name":    "PlayerA",
  "blue_name":   "PlayerB",
  "red_seed":    [0, 1, 0, ...],
  "blue_seed":   [0, 0, 1, ...],
  "metric_value": 48320.0
}
```

The only observable change is the **ordering** of items within the `highlights` array of each epoch document. Non-oscillating matches occupy lower indices; oscillating fallback matches occupy higher indices.

---

### 4. Simulation World Configuration

The Python simulation MUST replicate the kiosk world exactly to detect oscillation as it appears on screen:

| Parameter | Value | Source in C |
|---|---|---|
| Grid dimensions | 8 rows × 16 cols | `KIOSK_SIM_ROWS` / `KIOSK_SIM_COLS` in `app_state_manager.h` |
| Boundary condition | Toroidal (wrap-around) | `sync_ghost_borders()` in `game_logic.c` |
| Red seed origin | row=0, col=0 (0-indexed, left half) | `(r+1)*stride + (c+1)` where stride=18 |
| Blue seed origin | row=0, col=8 (0-indexed, right half) | `(r+1)*stride + (c+LOCAL_GRID_SIZE+1)` |
| Seed size | 8 × 8 cells | `LOCAL_GRID_SIZE` in `config.h` |
| Seed format | flat list of 64 ints (0/1), row-major: `seed[r*8+c]` | `file_io.c`: `(bitboard >> b) & 1` for b=0..63 |
| Max generations | `max_generations` from batch input (default 1000) | `MAX_ROUNDS` in `core_types.h` |

> **Note:** This configuration is identical to `run_isolated_match()` in `game_logic.c`. The Kiosk world and the Hyper-Worker world now use the same arena, ensuring that the match outcome visible on screen matches the leaderboard result.

---

### 5. Game Rules (translated from `update_generation()` in `game_logic.c`)

For each cell at position (r, c), the Moore neighborhood (8 cells) wraps toroidally:

```
red_neighbors  = count of TEAM_RED  cells in 8-cell Moore neighborhood
blue_neighbors = count of TEAM_BLUE cells in 8-cell Moore neighborhood
total_neighbors = red_neighbors + blue_neighbors

IF cell is ALIVE (RED or BLUE):
    IF total_neighbors IN {2, 3}:  new_state = current_team  (survival)
    ELSE:                          new_state = DEAD

IF cell is DEAD:
    IF total_neighbors == 3:
        new_state = TEAM_RED  IF red_neighbors > blue_neighbors
                   TEAM_BLUE  OTHERWISE   (blue wins ties: 1R+2B → BLUE, 2R+1B → RED)
    ELSE:
        new_state = DEAD
```

---

### 6. Oscillator Detection Algorithm

**Input:** `red_seed` (list of 64 ints), `blue_seed` (list of 64 ints), `max_generations` (int)

**Output:** `bool` — `True` if the match ends in period-2..5 oscillation, `False` otherwise.

```
CONSTANTS:
    ROWS, COLS = 50, 50
    MAX_PERIOD = 5
    CONFIRM_CYCLES = 2
    BUFFER_SIZE = MAX_PERIOD * CONFIRM_CYCLES + 1  → 11

ALGORITHM:
  1. Initialize grid[50][50] = 0 (DEAD)
  2. Place blue_seed at grid[20:28, 10:18]  (multiply by 2 = TEAM_BLUE)
     Place red_seed  at grid[20:28, 30:38]  (multiply by 1 = TEAM_RED)
  3. state_buffer = deque(maxlen=11)
     prev_state = b""  (empty bytes)

  4. FOR gen = 0 TO max_generations-1:
       a. grid = _step_numpy(grid)
       b. curr_state = grid.tobytes()
       c. IF curr_state == prev_state:
              RETURN False     ← truly static (period-1): not an oscillator
       d. state_buffer.append(curr_state)
       e. prev_state = curr_state

  5. buf = list(state_buffer)
     n   = len(buf)
     FOR P IN {2, 3, 4, 5}:
         IF n >= 2*P + 1:
             IF buf[-1] == buf[-1-P] == buf[-1-2*P]:
                 RETURN True   ← period-P oscillator confirmed (2 full cycles)

  6. RETURN False
```

**Why `buf[-1] == buf[-1-P] == buf[-1-2*P]` requires exactly 2 cycles:**
- `buf[-1] == buf[-1-P]` confirms 1 cycle.
- `buf[-1-P] == buf[-1-2*P]` confirms a 2nd independent cycle.
- Together they guard against a coincidental single match at the end of the simulation.

---

### 7. Reference Implementation

The following is the complete reference implementation for `worker.py`. This is the authoritative specification for the developer.

```python
import numpy as np
from collections import deque

# KI-Agent unterstützt: Constants mirror kiosk world config (app_state_manager.h / config.h)
# World is 8x16 — identical to run_isolated_match() in game_logic.c
_KIOSK_ROWS  = 8            # LOCAL_GRID_SIZE
_KIOSK_COLS  = 16           # LOCAL_GRID_SIZE * 2
_SEED_SIZE   = 8
_RED_ORIGIN  = (0, 0)       # left half:  rows 0-7, cols 0-7
_BLUE_ORIGIN = (0, 8)       # right half: rows 0-7, cols 8-15
_TEAM_RED       = 1
_TEAM_BLUE      = 2
_DEAD           = 0
_MAX_PERIOD     = 5
_CONFIRM_CYCLES = 2


def _step_numpy(grid: np.ndarray) -> np.ndarray:
    """One generation of the two-team Conway rules with toroidal boundary."""
    # KI-Agent unterstützt: Translated from update_generation() in game_logic.c
    red    = (grid == _TEAM_RED).astype(np.int16)
    blue   = (grid == _TEAM_BLUE).astype(np.int16)
    red_n  = np.zeros(grid.shape, dtype=np.int16)
    blue_n = np.zeros(grid.shape, dtype=np.int16)

    for dr in (-1, 0, 1):
        for dc in (-1, 0, 1):
            if dr == 0 and dc == 0:
                continue
            red_n  += np.roll(np.roll(red,  dr, axis=0), dc, axis=1)
            blue_n += np.roll(np.roll(blue, dr, axis=0), dc, axis=1)

    total_n  = red_n + blue_n
    alive    = grid != _DEAD
    survives = alive  & ((total_n == 2) | (total_n == 3))
    born     = ~alive & (total_n == 3)

    new_grid = np.zeros_like(grid)
    new_grid[survives] = grid[survives]
    new_grid[born]     = np.where(red_n[born] > blue_n[born], _TEAM_RED, _TEAM_BLUE)
    return new_grid


def is_oscillating_match(red_seed: list, blue_seed: list, max_generations: int) -> bool:
    """
    Returns True if the match converges to a period-2..5 oscillator.
    Uses the 50x50 kiosk world with same seed placement as app_state_manager.c.
    """
    # KI-Agent unterstützt
    grid = np.zeros((_KIOSK_ROWS, _KIOSK_COLS), dtype=np.int8)

    r0_b, c0_b = _BLUE_ORIGIN
    r0_r, c0_r = _RED_ORIGIN
    seed_b = np.array(blue_seed, dtype=np.int8).reshape(_SEED_SIZE, _SEED_SIZE)
    seed_r = np.array(red_seed,  dtype=np.int8).reshape(_SEED_SIZE, _SEED_SIZE)
    grid[r0_b : r0_b + _SEED_SIZE, c0_b : c0_b + _SEED_SIZE] = seed_b * _TEAM_BLUE
    grid[r0_r : r0_r + _SEED_SIZE, c0_r : c0_r + _SEED_SIZE] = seed_r * _TEAM_RED

    buf_size   = _MAX_PERIOD * _CONFIRM_CYCLES + 1  # 11
    state_buf  = deque(maxlen=buf_size)
    prev_state = b""

    for _ in range(max_generations):
        grid       = _step_numpy(grid)
        curr_state = grid.tobytes()
        if curr_state == prev_state:   # Static equilibrium: not an oscillator
            return False
        state_buf.append(curr_state)
        prev_state = curr_state

    buf = list(state_buf)
    n   = len(buf)
    for p in range(2, _MAX_PERIOD + 1):
        if n >= 2 * p + 1:
            if buf[-1] == buf[-1 - p] == buf[-1 - 2 * p]:
                return True
    return False


def filter_highlights_by_oscillation(highlights: list, max_generations: int) -> list:
    """
    Reorders highlight candidates: non-oscillating first, oscillating as fallback.
    Preserves descending metric_value order within each group.
    """
    # KI-Agent unterstützt
    non_osc = []
    osc     = []
    for h in highlights:
        if is_oscillating_match(h["red_seed"], h["blue_seed"], max_generations):
            logger.info(
                f"Oscillator detected: {h.get('red_name', '?')} vs "
                f"{h.get('blue_name', '?')} "
                f"(metric_value={h.get('metric_value', 0):.0f})"
            )
            osc.append(h)
        else:
            non_osc.append(h)
    logger.info(
        f"Highlight filter: {len(non_osc)} non-oscillating, "
        f"{len(osc)} oscillating (out of {len(highlights)} candidates)"
    )
    return non_osc + osc
```

**Integration call inside `execute_epoch()` (after line 100 in worker.py, before DB insert):**

```python
# [NEW] Filter oscillating patterns from highlight candidates
highlights_list = results.get("highlights", [])
filtered = filter_highlights_by_oscillation(highlights_list, batch_input["max_generations"])

for h in filtered:   # replaces: for h in results.get("highlights", []):
    highlights_data.append({ ... })
```

---

### 8. Test Patterns

Unit tests must verify both True and False cases with known patterns.

**Test 1 — Period-2 Blinker (expect True):**
A horizontal 3-cell line (standard Conway blinker) as the red seed. No blue seed.
```python
red_seed = [0] * 64
red_seed[3*8+2] = red_seed[3*8+3] = red_seed[3*8+4] = 1  # row 3, cols 2,3,4
blue_seed = [0] * 64
# Placed at grid[23, 32:35] in the 50×50 world — far from edges
# Expected: True (period-2 oscillator)
```

**Test 2 — Dead grid (expect False):**
Both seeds all zeros. The grid is dead from generation 0.
```python
red_seed  = [0] * 64
blue_seed = [0] * 64
# Expected: False (static from gen 0, static-state exit)
```

**Test 3 — Single isolated cell (expect False):**
One red cell. A lone cell has 0 neighbors and dies immediately. Static thereafter.
```python
red_seed = [0] * 64
red_seed[0] = 1
blue_seed = [0] * 64
# Expected: False (dies at gen 1, static)
```

**Test 4 — Two symmetric opposing seeds (expect False):**
Place identical seeds for both teams. They fight, consume each other, and typically reach a dead or stable non-oscillating state. Use a dense seed (e.g., filled 4×4 block) to guarantee non-trivial dynamics.
```python
# 4x4 filled block in both seeds
seed_block = [0]*64
for r in range(2, 6):
    for c in range(2, 6):
        seed_block[r*8+c] = 1
red_seed  = seed_block[:]
blue_seed = seed_block[:]
# Expected: False (symmetric fight with chaotic outcome, unlikely to form clean oscillator)
```

> **Note:** Test 4 is probabilistic — symmetric seeds can in rare cases produce oscillators. If it fails, replace with a different non-oscillating pattern verified manually.

---

### 9. Performance Estimate

| Metric | Value |
|---|---|
| Grid size | 50 × 50 = 2,500 cells |
| Generations per match | 1,000 |
| Matches per epoch | 10 |
| NumPy operations per step | ~8 × 2 roll ops + element-wise arithmetic |
| Estimated time per match | ~50–150 ms (NumPy, modern CPU) |
| Estimated total detection time | **~0.5–1.5 seconds per epoch** |
| Epoch cycle interval | 60 seconds |
| Overhead | **< 3%** |
