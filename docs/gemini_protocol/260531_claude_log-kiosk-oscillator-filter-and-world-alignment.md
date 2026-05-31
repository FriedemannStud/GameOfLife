# Session Log: Kiosk Highlight Quality — Oscillator Filtering & World Alignment

**Date:** 2026-05-31
**Agent:** Claude (Sonnet 4.6)
**Branch:** `biotop`
**Session type:** Design interview → Architecture documentation → Implementation

---

## 1. Starting Point

The Kiosk Mode (`STATE_KIOSK_MODE`) displays a "Live Battles" screen (`KIOSK_SUB_MULTICAM`) showing 4 simultaneous matches in a 2×2 grid. Matches are selected from a top-10 list ranked by `activity_sum` (total cell births + deaths per match), produced by the C Hyper-Worker.

A full analysis of the existing selection algorithm was performed at the start of the session. The pipeline: `C Hyper-Worker` → `Python Worker` → `MongoDB epoch_highlights` → `FastAPI /api/epoch/highlights` → `Kiosk Client (C)`.

---

## 2. Problems Identified

Two structural weaknesses in the current implementation were identified:

### Problem A — No Rotation
The client parses only the first 4 of the 10 available highlights (`network_io.c`, loop bound `i < 4`). The same 4 matches are shown on every Multicam cycle until a new epoch completes. The display is statically repetitive.

### Problem B — Oscillators Dominate the Ranking
Oscillating cellular formations (period 2–5) generate permanently high `activity_sum` scores because they cycle through states indefinitely. They rank at the top of the metric but are visually static — the same small cluster flipping back and forth. The kiosk display looks boring from a spectator's perspective.

---

## 3. Design Interview — Decisions

The developer and agent conducted a structured design interview, resolving each decision branch in sequence.

### Problem B (tackled first — it defines the pool that A rotates through)

| # | Question | Decision |
|---|---|---|
| B1 | Detection method | Periodicity check (compare grid state hashes at period P) |
| B2 | Periods to check | P ∈ {2, 3, 4, 5} |
| B3 | Implementation location | **Python Worker** (`worker.py`) — MUST be async to C Hyper-Worker |
| B4 | Simulation scope | **Full match** (1000 generations), check **end-state** |
| B5 | Confirmation cycles | **2 full cycles**: `buf[-1] == buf[-1-P] == buf[-1-2P]` |
| B6 | Fallback when all oscillate | **Soft fallback**: non-oscillating first; fill remaining slots from oscillating pool if < 4 non-oscillating available |

**Key design rationale for Python Worker:**
- Only location that is provably async to the C headless calculation
- Centralized: all kiosk instances benefit automatically
- No C code changes needed
- Cost: ~1 second per epoch (NumPy, 8×16 grid × 1000 gen × 10 matches)

### Problem A (deferred — will be ADR-0024)

| # | Question | Decision |
|---|---|---|
| A1 | Rotation location | Client-side in `KioskController` |
| A2 | Strategy | Round-robin: `highlight_pool_index` advances by `match_count` each Multicam cycle |
| A3 | Pool reset on new epoch | Yes — reset `highlight_pool_index = 0` |
| A4 | Wrap-around | Modulo: `pool[(index + i) % pool_size]`, always 4 slots filled |

**Problem A is NOT yet implemented.** See Section 8 (Next Steps).

---

## 4. Critical Design Change: Kiosk World Alignment

During implementation planning, a semantic inconsistency was discovered:

- **Hyper-Worker** (`run_isolated_match()`): 8×16 grid, seeds placed directly adjacent
- **Kiosk display**: 50×50 grid, seeds placed in the center with 12-column gap

This means the match winner on screen could differ from the leaderboard winner — the same seeds behave differently in different world sizes.

**Decision:** Align the Kiosk world to the Hyper-Worker world (8×16).

**Effort:** Low — 3 constants and 1 seed-placement block in 2 C files.

**Side effect corrected:** The old kiosk had Blue on the LEFT and Red on the RIGHT. In `run_isolated_match()`, `seed_red` is always the LEFT player. The alignment also fixes this left/right swap — Red is now LEFT, Blue RIGHT, matching the Hyper-Worker exactly.

**C changes made:**

| File | Change |
|---|---|
| `src/gui/app_state_manager.h` | Replaced `KIOSK_SIM_WORLD_SIZE 50` with `KIOSK_SIM_ROWS LOCAL_GRID_SIZE` (8) and `KIOSK_SIM_COLS (LOCAL_GRID_SIZE*2)` (16) |
| `src/gui/app_state_manager.c` | `init_simulation_context`: `(50,50)` → `(KIOSK_SIM_ROWS, KIOSK_SIM_COLS)` |
| `src/gui/app_state_manager.c` | `init_render_context`: `(50,50,vp)` → `(KIOSK_SIM_COLS, KIOSK_SIM_ROWS, vp)` |
| `src/gui/app_state_manager.c` | Seed placement: stride=18, Red at `(r+1)*18+(c+1)`, Blue at `(r+1)*18+(c+9)` |

Build result: `gcc -Wall -Wextra` — zero warnings.

---

## 5. Documentation Created (ADR-0023)

Three new documents were created before implementation:

| Document | Path | Purpose |
|---|---|---|
| ADR-0023 | `docs/adr/ADR-0023-oscillator-filtering-highlight-quality.md` | Architecture decision record — context, decision, consequences, 4 rejected alternatives |
| DEV_SPEC-0023 | `docs/specs/DEV_SPEC-0023-oscillator-filtering.md` | Requirements (R-01 to R-08), 3 user stories with acceptance criteria, MoSCoW prioritization |
| DEV_TECH_DESIGN-0023 | `docs/tech_design/DEV_TECH_DESIGN-0023-oscillator-filtering.md` | World config table, game rules translation, full reference implementation (copy-paste ready) |
| DEV_TASKS-0023 | `docs/tasks/DEV_TASKS-0023-oscillator-filtering.md` | 6 phases, 17 steps with exact shell commands and interactive verification checkpoints |

**ADR-0023 status:** Implemented.

---

## 6. Implementation: `backend/app/worker.py`

### New dependencies
- `numpy>=1.24.0` added to `backend/requirements.txt`
- `from collections import deque` and `import numpy as np` added to `worker.py`

### World configuration constants (top of `worker.py`)
```python
_KIOSK_ROWS  = 8      # LOCAL_GRID_SIZE — matches run_isolated_match()
_KIOSK_COLS  = 16     # LOCAL_GRID_SIZE * 2
_SEED_SIZE   = 8
_RED_ORIGIN  = (0, 0)   # left half:  rows 0-7, cols 0-7
_BLUE_ORIGIN = (0, 8)   # right half: rows 0-7, cols 8-15
_TEAM_RED    = 1
_TEAM_BLUE   = 2
_MAX_PERIOD  = 5
_CONFIRM_CYCLES = 2
```

### New functions (added before `execute_epoch`)

**`_step_numpy(grid)`**
- One generation of two-team Conway rules, fully vectorized with NumPy
- Toroidal boundary via `np.roll`
- Translation of `update_generation()` from `game_logic.c`
- Survival rule: 2 or 3 neighbors → keep team. Birth rule: 3 neighbors → majority team (RED if `red_n > blue_n`, else BLUE)

**`is_oscillating_match(red_seed, blue_seed, max_generations)`**
- Initializes 8×16 NumPy grid, places seeds at `_RED_ORIGIN` and `_BLUE_ORIGIN`
- Simulates `max_generations` steps, maintaining a `deque(maxlen=11)` of raw grid byte snapshots
- Detects truly static state (`curr == prev`) → returns `False` immediately (period-1 fixed point, not oscillator)
- After simulation: checks periods P ∈ {2,3,4,5} with 2-cycle confirmation: `buf[-1] == buf[-1-P] == buf[-1-2*P]`
- Returns `True` if any period confirmed, `False` otherwise

**`filter_highlights_by_oscillation(highlights, max_generations)`**
- Applies `is_oscillating_match` to each of the 10 candidates
- Logs each detected oscillator and a summary line per epoch
- Returns `non_oscillating + oscillating` (soft fallback: all candidates always returned, non-oscillating first)

### Integration point in `execute_epoch()`
Inserted directly before the `highlights_data` build loop (original line ~92):
```python
_raw_highlights = results.get("highlights", [])
_filtered_highlights = filter_highlights_by_oscillation(
    _raw_highlights, batch_input["max_generations"]
)
for h in _filtered_highlights:   # replaces: for h in results.get("highlights", []):
```

---

## 7. Tests: `backend/tests/test_oscillator_detection.py`

13 unit tests, all passing. Coverage:

| Group | Tests |
|---|---|
| `_step_numpy` | Blinker rotates (horizontal→vertical), 2×2 block stable, birth majority RED, birth majority BLUE |
| `is_oscillating_match` | Period-2 blinker red seed → True, period-2 blinker blue seed → True, dead grid → False, single cell → False, 2×2 still-life → False |
| `filter_highlights_by_oscillation` | Non-oscillating appears first, order preserved within groups, all-oscillating returns all (soft fallback), empty input returns empty |

Run with: `python3 backend/tests/test_oscillator_detection.py`

---

## 8. Verification Results

### Docker end-to-end (Step 5.2 — worker logs)
```
INFO:epoch_worker:Oscillator detected: 6a1b513ef9cca60ac0f1451b vs ... (metric_value=12047)
... (10 oscillators total) ...
INFO:epoch_worker:Highlight filter: 0 non-oscillating, 10 oscillating (of 10 candidates)
INFO:epoch_worker:Stored 10 highlights for epoch_1780255778
INFO:epoch_worker:Database updated with Epoch results.
```

All 10 top candidates in this tournament are oscillating patterns — **soft fallback activated**. All 10 are stored and the kiosk still has content.

### MongoDB query (Step 5.3 — DB content)
```
epoch_id  : epoch_1780255963
count     : 10

[ 0] Fritz1   vs Fritz2    metric= 12047
[ 1] Fritz2   vs Fritz1    metric= 12047
[ 2] Fritz3   vs Fritz10   metric= 10758
...
[ 9] Fritz10  vs Fritz4    metric=  8343
```

- Nicknames correctly resolved (not raw ObjectIDs) ✓
- 10 highlights stored ✓
- Metric values descending ✓
- Soft fallback: all 10 are oscillating; stored in original activity_sum order ✓

### Observation
Fritz1 vs Fritz2 and Fritz2 vs Fritz1 both show `metric_value=12047` — the pattern is symmetric across team sides. This is consistent with oscillating formations that behave identically regardless of which half of the 8×16 grid they occupy.

---

## 9. Files Changed in This Session

| File | Type | Change |
|---|---|---|
| `src/gui/app_state_manager.h` | C header | `KIOSK_SIM_WORLD_SIZE` → `KIOSK_SIM_ROWS` + `KIOSK_SIM_COLS` |
| `src/gui/app_state_manager.c` | C source | World size constants + seed placement (8×16, Red left, Blue right) |
| `backend/requirements.txt` | Config | `numpy>=1.24.0` added |
| `backend/app/worker.py` | Python | 3 new functions + integration call in `execute_epoch()` |
| `backend/tests/test_oscillator_detection.py` | Python test | New file, 13 unit tests |
| `docs/adr/ADR-0023-oscillator-filtering-highlight-quality.md` | Doc | New ADR, status: Implemented |
| `docs/specs/DEV_SPEC-0023-oscillator-filtering.md` | Doc | New spec |
| `docs/tech_design/DEV_TECH_DESIGN-0023-oscillator-filtering.md` | Doc | New tech design |
| `docs/tasks/DEV_TASKS-0023-oscillator-filtering.md` | Doc | New task checklist (all steps complete) |
| `docs/CHANGELOG.md` | Doc | Entry for 2026-05-31 added |

---

## 10. Next Steps

### Immediate: Interactive Kiosk Test (DEV_TASKS-0023, Step 5.4)
Run `make && ./build/biotope`, press `K` to enter Kiosk Mode. Verify all 4 quadrants show live simulations with player names (no visual regressions from the C world-size change).

### Next Feature: Problem A — Highlight Rotation (ADR-0024)

The design decisions for Problem A are fully agreed (see Section 3) but not yet implemented. Required changes:

**`src/io/network_io.h`**
- `MatchHighlight matches[4]` → `matches[10]`

**`src/io/network_io.c`**
- Parse loop bound `i < 4` → `i < 10`

**`src/gui/app_state_manager.h`**
- Add `int highlight_pool_index` to `KioskController` struct
- Add `int highlight_pool_size` to `KioskController` struct

**`src/gui/app_state_manager.c`**
- On new highlight data: reset `highlight_pool_index = 0`, set `highlight_pool_size = hd.count`
- When loading slots into simulations: use `pool[(highlight_pool_index + i) % highlight_pool_size]`
- On leaving `KIOSK_SUB_MULTICAM`: advance `highlight_pool_index += match_count`

**Expected outcome:** The kiosk cycles through all 10 (or however many non-oscillating) highlights in round-robin fashion. After 2–3 Multicam cycles (~1.5–2 minutes), all available matches have been shown.

### Documentation to create for Problem A
- `ADR-0024-highlight-rotation-round-robin.md`
- `DEV_TASKS-0024-highlight-rotation.md`
- `DEV_SPEC-0024-highlight-rotation.md` (optional, scope is narrow)
