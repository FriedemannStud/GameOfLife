# DEV_TASKS-0025: Per-Configuration Leaderboard with 8x8 Start-Config Icon

This task implements ADR-0025: the Kiosk "Global Leaderboard" reads per-configuration from the
`submissions` collection and renders each entry's 8x8 start configuration as an icon between rank
and name. This realises the icon feature and fixes the per-nickname last-write-wins ranking
distortion in one step.

**Developer:** Follow each step in sequence. After every Verification step, report the result
before moving on.

**Briefing Document:**
- [ADR-0025: Per-Configuration Leaderboard with 8x8 Start-Config Icon](../adr/ADR-0025-leaderboard-per-config-and-seed-icon.md)

> **Follow-up:** The `"+ N more"` overflow indicator introduced here counted only rows within the
> 20-record cap, so it froze (observed `"+ 11 more"`) once ≥ 20 configs were ranked. This is
> corrected in [ADR-0030](../adr/ADR-0030-honest-leaderboard-overflow-count.md) /
> [DEV_TASKS-0030](./DEV_TASKS-0030-honest-leaderboard-overflow-count.md).

---

## Phase 0: Orientation Convention (read before coding)

- [x] **Step 0.1:** Confirm the binding index convention so the icon is not mirrored.
    - `config.cells` = `[x=col, y=row]` (`file_io.c:87`).
    - Dense index `b = y*8 + x` (`game_logic.c:197`, serialised `file_io.c:549`).
    - Renderer reads `seed[tr*8+tc]`, `tr=row, tc=col` (`renderer.c:1482-1489`).
    - **Backend conversion must set `seed[y*8 + x] = 1`.**

---

## Phase 1: Backend — Data Source and Conversion

- [x] **Step 1.1:** Add `backend/app/grid_utils.py` with `cells_to_grid(cells)` returning a dense
  64-int list (`index = y*8 + x`), defensive against empty / malformed / out-of-bounds cells.
- [x] **Step 1.2:** In `backend/app/main.py`, import `cells_to_grid` and rewrite `get_leaderboard`
  to read from `db.submissions.find({"status": "active", "matches_played": {"$gt": 0}})`, sorted
  `[("win_rate", -1), ("avg_stable_generation", 1)]`, `limit(20)`. Use `metadata.nickname` as
  `name`, keep `win_rate * 100` rounding, add `"seed": cells_to_grid(config.cells)`.
- [x] **Step 1.3 (Verification):** Run the orientation/defensive unit check:
    ```bash
    cd backend && python3 -c "from app.grid_utils import cells_to_grid; \
      g=cells_to_grid([[0,0],[7,0]]); assert g[0]==1 and g[7]==1 and sum(g)==2; \
      assert cells_to_grid([[3,5]])[43]==1; print('PASS')"
    ```

---

## Phase 2: C — Network Struct and Parser

- [x] **Step 2.1:** Add `int seed[GRID_SIZE_8X8];` to `LeaderboardEntry` (`src/io/network_io.h`).
- [x] **Step 2.2:** In the leaderboard parser (`src/io/network_io.c`), parse the `"seed"` array
  into the entry with a default-0 fill and a 64-element bound guard (mirror the highlight parser).

---

## Phase 3: C — Rendering

- [x] **Step 3.1:** In `draw_kiosk_leaderboard` (`src/gui/renderer.c`), insert a `CONFIG` icon
  column between RANK and PLAYER; reflow the proportional column x-positions; add the header label.
- [x] **Step 3.2:** Render the 8x8 icon in the row loop (single colour `THEME_ACCENT`), with icon
  size coupled to `rowHeight` (`icon_px = rowHeight - 8`, clamped ≤ 24) so it cannot collide with
  text or the footer.

---

## Phase 4: Build and Verify

- [x] **Step 4.1 (Verification):** `make` produces **zero warnings**.
- [ ] **Step 4.2 (Verification — Backend, optional live):** With a running backend,
  `curl -s localhost:8000/api/leaderboard | jq '.leaderboard[0]'` shows a `seed` field of length
  64 with 0/1 values.
- [ ] **Step 4.3 (Verification — Interactive Test, Developer):** Start the app, press `[K]`, wait
  for the Leaderboard view. Report:
    1. Does an 8x8 icon appear between RANK and PLAYER on each row?
    2. Does the icon pattern match the known config (not mirrored/rotated)?
    3. Do players with multiple configs appear multiple times with *different* icons and their
       own stats?
    4. Does the layout/footer stay intact across different window sizes?

---

## Phase 5: Documentation

- [x] **Step 5.1:** ADR-0025 created.
- [x] **Step 5.2:** Update `docs/CHANGELOG.md`.
