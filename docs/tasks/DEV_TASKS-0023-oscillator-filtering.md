# DEV_TASKS-0023: Oscillator Detection and Filtering for Kiosk Highlight Quality

This task implements the oscillator detection pipeline described in ADR-0023. The goal is to ensure that the Kiosk Mode "Live Battles" display shows only visually dynamic matches by filtering out patterns that converge to periodic oscillation (period 2–5).

**Developer:** Follow these steps precisely. Each "Verification" step requires you to run the indicated command or action and report back the result. Do not proceed to the next step until verification has passed. This iterative approach prevents hidden regressions.

**Briefing Documents:**
- [ADR-0023: Oscillator Detection and Filtering](../adr/ADR-0023-oscillator-filtering-highlight-quality.md)
- [DEV_SPEC-0023: Requirements Specification](../specs/DEV_SPEC-0023-oscillator-filtering.md)
- [DEV_TECH_DESIGN-0023: Technical Design and Reference Implementation](../tech_design/DEV_TECH_DESIGN-0023-oscillator-filtering.md)

---

## Phase 0: Prerequisites

*Goal: Confirm the development environment is ready and all required dependencies are in place before touching any code.*

- [ ] **Step 0.1: Verify NumPy is available**
    - [ ] **Action:** Open `backend/requirements.txt` and search for `numpy`.
    - [ ] **Verification:** Run the following command and report the output:
        ```bash
        grep -i numpy backend/requirements.txt
        ```
        **Expected Result:** A line like `numpy>=1.x.x` or `numpy` is printed.
        If numpy is **not** found: add `numpy>=1.24.0` to `backend/requirements.txt` and re-run `pip install -r backend/requirements.txt` inside the backend container or virtual environment.

- [ ] **Step 0.2: Confirm seed format from the live data**
    - [ ] **Action:** Open `backend/app/worker.py` and locate the line `"red_seed": h["red_seed"]` (around line 96). Then open `src/io/file_io.c` and locate the `save_batch_results` function — find the loop that builds `red_seed` and `blue_seed` JSON arrays.
    - [ ] **Verification:** Confirm that the seed is serialized as a JSON array of 64 integers (0 or 1), where index `b` corresponds to bit `b` of the `uint64_t` bitboard (`(bitboard >> b) & 1`). This means `seed[r*8 + c]` is the cell at row `r`, column `c` of the 8×8 pattern.
        **Expected Result:** You can read `file_io.c` line ~549: `cJSON_CreateNumber((highlights[i].seed_red & (1ULL << b)) ? 1 : 0)` inside a loop `for (int b = 0; b < 64; b++)`.
        Report: "Seed format confirmed as 64-element int array, row-major."

- [ ] **Step 0.3: Locate the exact integration point in worker.py**
    - [ ] **Action:** Open `backend/app/worker.py`. Find the block starting at `for h in results.get("highlights", []):` (around line 92). This is the loop that builds `highlights_data`. The filter call will be inserted **before** this loop.
    - [ ] **Verification:** Report the exact line number where the `for h in results.get("highlights", []):` loop begins.

---

## Phase 1: Implement the Oscillator Detector

*Goal: Write and validate the two core functions in isolation, completely independent of the epoch pipeline. This phase produces a working, tested module before any integration work begins.*

- [ ] **Step 1.1: Add module-level constants and imports to `worker.py`**
    - [ ] **Action:** At the top of `backend/app/worker.py`, after the existing imports, add the following block. Do **not** modify any existing imports.
        ```python
        import numpy as np
        from collections import deque

        # KI-Agent unterstützt: Constants mirror kiosk world config (app_state_manager.c / config.h)
        _KIOSK_ROWS     = 50
        _KIOSK_COLS     = 50
        _SEED_SIZE      = 8
        _BLUE_ORIGIN    = (20, 10)   # (row, col) 0-indexed — matches (r+21)*stride+(c+11) in ghost grid
        _RED_ORIGIN     = (20, 30)   # (row, col) 0-indexed — matches (r+21)*stride+(c+31) in ghost grid
        _TEAM_RED       = 1
        _TEAM_BLUE      = 2
        _DEAD           = 0
        _MAX_PERIOD     = 5
        _CONFIRM_CYCLES = 2
        ```
    - [ ] **Verification:** Run `python -c "import sys; sys.path.insert(0, 'backend'); from app import worker"` from the project root. Report: no ImportError printed.

- [ ] **Step 1.2: Add `_step_numpy(grid)` to `worker.py`**
    - [ ] **Action:** Add the following function to `backend/app/worker.py`, **after** the constants block and **before** the `execute_epoch` function. This function implements exactly one generation of the two-team Conway rules with toroidal boundary using NumPy. It is a direct translation of `update_generation()` in `src/core/game_logic.c`.
        ```python
        def _step_numpy(grid: np.ndarray) -> np.ndarray:
            """One generation of two-team Conway rules with toroidal (wrap-around) boundary."""
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
        ```
    - [ ] **Verification:** Run the following quick smoke test from the project root:
        ```bash
        python -c "
        import sys; sys.path.insert(0, 'backend')
        from app.worker import _step_numpy, _TEAM_RED
        import numpy as np
        g = np.zeros((10,10), dtype=np.int8)
        g[5,4] = g[5,5] = g[5,6] = _TEAM_RED   # horizontal blinker
        g2 = _step_numpy(g)
        assert g2[4,5] == _TEAM_RED and g2[5,5] == _TEAM_RED and g2[6,5] == _TEAM_RED, 'Blinker rotation failed'
        print('_step_numpy smoke test passed')
        "
        ```
        **Expected Result:** `_step_numpy smoke test passed` is printed. Report the exact output.

- [ ] **Step 1.3: Add `is_oscillating_match(red_seed, blue_seed, max_generations)` to `worker.py`**
    - [ ] **Action:** Add the following function immediately after `_step_numpy`:
        ```python
        def is_oscillating_match(red_seed: list, blue_seed: list, max_generations: int) -> bool:
            """
            Returns True if the match converges to a period-2..5 oscillator in the kiosk world.
            Simulates a 50x50 grid with same seed placement as app_state_manager.c.
            """
            # KI-Agent unterstützt
            grid = np.zeros((_KIOSK_ROWS, _KIOSK_COLS), dtype=np.int8)

            r0_b, c0_b = _BLUE_ORIGIN
            r0_r, c0_r = _RED_ORIGIN
            seed_b = np.array(blue_seed, dtype=np.int8).reshape(_SEED_SIZE, _SEED_SIZE)
            seed_r = np.array(red_seed,  dtype=np.int8).reshape(_SEED_SIZE, _SEED_SIZE)
            grid[r0_b : r0_b + _SEED_SIZE, c0_b : c0_b + _SEED_SIZE] = seed_b * _TEAM_BLUE
            grid[r0_r : r0_r + _SEED_SIZE, c0_r : c0_r + _SEED_SIZE] = seed_r * _TEAM_RED

            buf_size   = _MAX_PERIOD * _CONFIRM_CYCLES + 1  # 11 entries
            state_buf  = deque(maxlen=buf_size)
            prev_state: bytes = b""

            for _ in range(max_generations):
                grid       = _step_numpy(grid)
                curr_state = grid.tobytes()
                if curr_state == prev_state:  # Truly static: period-1, not an oscillator
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
        ```
    - [ ] **Verification:** Run the following command and report the output:
        ```bash
        python -c "
        import sys; sys.path.insert(0, 'backend')
        from app.worker import is_oscillating_match

        # Test 1: horizontal blinker as red seed (period-2 oscillator)
        red_seed = [0]*64
        red_seed[3*8+2] = red_seed[3*8+3] = red_seed[3*8+4] = 1
        blue_seed = [0]*64
        result = is_oscillating_match(red_seed, blue_seed, 100)
        print(f'Blinker test: {result}  (expected True)')

        # Test 2: dead grid
        result2 = is_oscillating_match([0]*64, [0]*64, 100)
        print(f'Dead grid test: {result2}  (expected False)')
        "
        ```
        **Expected Result:**
        ```
        Blinker test: True  (expected True)
        Dead grid test: False  (expected False)
        ```
        Report the exact output. If either test fails, investigate before proceeding.

---

## Phase 2: Write Unit Tests

*Goal: Create a proper test file that can be re-run as a regression guard after any future changes to the detection logic.*

- [ ] **Step 2.1: Create `backend/tests/test_oscillator_detection.py`**
    - [ ] **Action:** Create the file `backend/tests/test_oscillator_detection.py` with the following content:
        ```python
        """
        Unit tests for the oscillator detection functions in worker.py.
        Run with: python backend/tests/test_oscillator_detection.py
        """
        # KI-Agent unterstützt
        import sys
        import os
        sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'backend'))

        from app.worker import is_oscillating_match

        def test_period2_blinker():
            """Horizontal blinker (3 cells in a row) is a period-2 oscillator."""
            red_seed = [0] * 64
            red_seed[3*8+2] = red_seed[3*8+3] = red_seed[3*8+4] = 1  # row 3, cols 2-4
            blue_seed = [0] * 64
            result = is_oscillating_match(red_seed, blue_seed, 200)
            assert result is True, f"Expected True for period-2 blinker, got {result}"
            print("PASS: test_period2_blinker")

        def test_dead_grid():
            """An all-dead grid is static from generation 0: not an oscillator."""
            result = is_oscillating_match([0]*64, [0]*64, 200)
            assert result is False, f"Expected False for dead grid, got {result}"
            print("PASS: test_dead_grid")

        def test_single_cell_dies():
            """A single cell has no neighbors, dies at gen 1, static thereafter."""
            red_seed = [0] * 64
            red_seed[0] = 1
            result = is_oscillating_match(red_seed, [0]*64, 200)
            assert result is False, f"Expected False for single cell, got {result}"
            print("PASS: test_single_cell_dies")

        def test_period2_blinker_blue():
            """Blinker works the same when assigned to the BLUE team."""
            blue_seed = [0] * 64
            blue_seed[3*8+2] = blue_seed[3*8+3] = blue_seed[3*8+4] = 1
            red_seed = [0] * 64
            result = is_oscillating_match(red_seed, blue_seed, 200)
            assert result is True, f"Expected True for blue blinker, got {result}"
            print("PASS: test_period2_blinker_blue")

        def test_two_cell_block_stable():
            """
            A 2x2 block is a stable still-life (period-1 fixed point).
            It should NOT be flagged as an oscillator.
            """
            red_seed = [0] * 64
            red_seed[0*8+0] = red_seed[0*8+1] = 1
            red_seed[1*8+0] = red_seed[1*8+1] = 1  # 2x2 block
            result = is_oscillating_match(red_seed, [0]*64, 200)
            assert result is False, f"Expected False for stable 2x2 block, got {result}"
            print("PASS: test_two_cell_block_stable")

        if __name__ == "__main__":
            test_period2_blinker()
            test_dead_grid()
            test_single_cell_dies()
            test_period2_blinker_blue()
            test_two_cell_block_stable()
            print("\nAll oscillator detection tests passed.")
        ```
    - [ ] **Verification (Interactive Test):**
        1. Run: `python backend/tests/test_oscillator_detection.py`
        2. Report the full output.
        **Expected Result:**
        ```
        PASS: test_period2_blinker
        PASS: test_dead_grid
        PASS: test_single_cell_dies
        PASS: test_period2_blinker_blue
        PASS: test_two_cell_block_stable

        All oscillator detection tests passed.
        ```
        If any test fails, stop and report the failure message before continuing.

---

## Phase 3: Implement the Filter Function

*Goal: Add the orchestration function that applies the detector to all highlight candidates and reorders the list.*

- [ ] **Step 3.1: Add `filter_highlights_by_oscillation(highlights, max_generations)` to `worker.py`**
    - [ ] **Action:** Add the following function immediately after `is_oscillating_match` in `backend/app/worker.py`:
        ```python
        def filter_highlights_by_oscillation(highlights: list, max_generations: int) -> list:
            """
            Reorders highlight candidates so non-oscillating matches appear first.
            Oscillating matches are appended as a soft fallback pool.
            Within each group, the original metric_value order (descending) is preserved.
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
                f"Highlight filter result: {len(non_osc)} non-oscillating, "
                f"{len(osc)} oscillating (of {len(highlights)} total candidates)"
            )
            return non_osc + osc
        ```
    - [ ] **Verification:** Run:
        ```bash
        python -c "
        import sys; sys.path.insert(0, 'backend')
        from app.worker import filter_highlights_by_oscillation

        # Create one oscillating and one non-oscillating fake highlight
        def make_highlight(is_blinker, name):
            rs = [0]*64
            if is_blinker:
                rs[3*8+2] = rs[3*8+3] = rs[3*8+4] = 1
            return {'red_seed': rs, 'blue_seed': [0]*64,
                    'red_name': name, 'blue_name': 'B',
                    'metric_value': 100.0}

        highlights = [make_highlight(True, 'oscillator'), make_highlight(False, 'dynamic')]
        result = filter_highlights_by_oscillation(highlights, 200)
        print(f'First entry: {result[0][\"red_name\"]}  (expected: dynamic)')
        print(f'Second entry: {result[1][\"red_name\"]}  (expected: oscillator)')
        "
        ```
        **Expected Result:**
        ```
        First entry: dynamic  (expected: dynamic)
        Second entry: oscillator  (expected: oscillator)
        ```
        Report the exact output.

---

## Phase 4: Integration into `execute_epoch()`

*Goal: Wire the filter function into the existing epoch pipeline at the correct location.*

- [ ] **Step 4.1: Identify the exact integration point**
    - [ ] **Action:** Open `backend/app/worker.py`. Find the line:
        ```python
        for h in results.get("highlights", []):
        ```
        This is the loop that builds `highlights_data`. Note the exact line number.
    - [ ] **Verification:** Report the line number (should be around line 92–95).

- [ ] **Step 4.2: Insert the filter call before the loop**
    - [ ] **Action:** Replace the line:
        ```python
        for h in results.get("highlights", []):
        ```
        with:
        ```python
        # KI-Agent unterstützt: Filter oscillating patterns before storing highlights (ADR-0023)
        _raw_highlights = results.get("highlights", [])
        _filtered_highlights = filter_highlights_by_oscillation(
            _raw_highlights, batch_input["max_generations"]
        )
        for h in _filtered_highlights:
        ```
        **Important:** The loop body (`highlights_data.append({...})`) and all subsequent code remain completely unchanged.
    - [ ] **Verification:** Run a Python syntax check on the modified file:
        ```bash
        python -m py_compile backend/app/worker.py && echo "Syntax OK"
        ```
        **Expected Result:** `Syntax OK` is printed. Report any errors if they appear.

- [ ] **Step 4.3: Verify no regressions in existing backend tests**
    - [ ] **Action:** Run the existing backend test suite:
        ```bash
        python backend/tests/test_ranking.py
        python backend/tests/test_validators.py
        ```
    - [ ] **Verification:** Report the complete output of both commands.
        **Expected Result:** Both scripts exit without errors. All existing tests pass.

---

## Phase 5: End-to-End Verification

*Goal: Confirm the full pipeline works in the running Docker environment: hyper-worker runs, oscillator detection fires, MongoDB stores filtered highlights.*

- [ ] **Step 5.1: Start the Docker environment**
    - [ ] **Action:** From the project root, run:
        ```bash
        docker-compose up --build -d
        ```
        Wait until all containers report healthy/started.
    - [ ] **Verification (Interactive Test):**
        1. Run: `docker-compose ps`
        2. Report the status of all containers.
        **Expected Result:** All services (backend, mongo, worker) show status `Up` or `healthy`.

- [ ] **Step 5.2: Trigger a tournament epoch and observe worker logs**
    - [ ] **Action:** The worker runs automatically every 60 seconds. Wait for one full cycle, or — if at least 2 active submissions exist — wait for the next epoch. Monitor the logs with:
        ```bash
        docker-compose logs -f worker
        ```
        Let it run until you see the `"Database updated with Epoch results."` log line.
    - [ ] **Verification (Interactive Test):**
        1. Look for log lines containing `"Highlight filter result:"`.
        2. Look for any log lines containing `"Oscillator detected:"`.
        3. Report the exact log lines you see.
        **Expected Result:** You see at least one `"Highlight filter result: X non-oscillating, Y oscillating (of Z total candidates)"` line. If no oscillators were detected (Y=0), that is a valid result if the active patterns are genuinely non-oscillating.

- [ ] **Step 5.3: Inspect MongoDB highlight ordering**
    - [ ] **Action:** After the epoch completes, query the most recent `epoch_highlights` document:
        ```bash
        docker-compose exec mongo mongosh biotope --eval \
          "db.epoch_highlights.find({}).sort({timestamp:-1}).limit(1).pretty()"
        ```
    - [ ] **Verification (Interactive Test):**
        1. Examine the `highlights` array in the returned document.
        2. Verify that if any oscillating matches were flagged (from Step 5.2 logs), they appear at higher indices in the array than non-oscillating matches.
        3. Report: the number of highlights stored and whether the ordering is correct.
        **Expected Result:** The `highlights` array contains entries. If Y oscillating matches were logged in Step 5.2, the last Y entries of the array correspond to those matches (cross-reference by player names).

- [ ] **Step 5.4: Confirm the Kiosk Client is unaffected**
    - [ ] **Action:** Build the C application and start it in kiosk mode:
        ```bash
        make
        ```
        Then start the application and navigate to Kiosk Mode (press `K`).
    - [ ] **Verification (Interactive Test):**
        1. Start the app: `./build/biotope`
        2. Press `K` to enter Kiosk Mode.
        3. Wait for the Multicam view to appear (after the 15-second leaderboard phase).
        4. Observe whether all 4 quadrants display matches normally.
        5. Report: do all 4 quadrants show simulations? Are player names visible?
        **Expected Result:** All 4 quadrants show running simulations with player names. No visual regressions from the C client's perspective — it is unaware of the filtering.

---

## Phase 6: Documentation Update

*Goal: Mark the ADR as implemented and confirm all deliverables are complete.*

- [ ] **Step 6.1: Update ADR-0023 status**
    - [ ] **Action:** Open `docs/adr/ADR-0023-oscillator-filtering-highlight-quality.md` and change the `**Status:** Proposed` line to `**Status:** Implemented`.
    - [ ] **Verification:** Confirm the change is saved.

- [ ] **Step 6.2: Update CHANGELOG.md**
    - [ ] **Action:** Add an entry to `docs/CHANGELOG.md` under a new section for today's date:
        ```
        ### 2026-05-31 — ADR-0023: Oscillator Filtering for Kiosk Highlight Quality
        - Added `_step_numpy`, `is_oscillating_match`, and `filter_highlights_by_oscillation`
          to `backend/app/worker.py`.
        - Oscillating matches (period 2–5) are now demoted to fallback pool in `epoch_highlights`.
        - Non-oscillating matches always appear at lower indices, ensuring the Kiosk Client
          receives dynamic highlights first.
        - 5 unit tests added in `backend/tests/test_oscillator_detection.py`.
        ```
    - [ ] **Verification:** Confirm the entry is saved.
