# DEV_TASKS-0024: Client-Side Round-Robin Highlight Rotation

This task implements the round-robin highlight rotation described in ADR-0024. The goal is to expand the highlight pool from 4 to 10 matches and cycle through them across Multicam display cycles, so a spectator watching the kiosk sees new matches on every rotation.

**Developer:** Follow each step in sequence. After every Verification step, report the result before moving to the next step. This iterative approach catches regressions before they accumulate. Phases are self-contained — you may safely stop and resume at any phase boundary.

**Briefing Document:**
- [ADR-0024: Client-Side Round-Robin Highlight Rotation](../adr/ADR-0024-highlight-rotation-round-robin.md)

---

## Phase 0: Global Search and Analysis

*Goal: Identify every location in the codebase affected by this change before writing a single line of code. Follows CODING_STYLE.md Rule 10.*

- [ ] **Step 0.1: Search for all highlight pool size references**
    - [ ] **Action:** Run the following commands from the project root and read every result:
        ```bash
        grep -rn "matches\[4\]" src/
        grep -rn "i < 4" src/io/network_io.c
        grep -rn "cached_highlights\.matches" src/
        grep -rn "hd\.matches" src/
        grep -rn "HighlightData" src/
        ```
    - [ ] **Verification:** Confirm you see exactly these locations (no others):

        | File | Line | What |
        |------|------|------|
        | `src/io/network_io.h` | 37 | `MatchHighlight matches[4];` — struct array size |
        | `src/io/network_io.c` | 147 | `i < size && i < 4` — parse loop cap |
        | `src/gui/app_state_manager.c` | 176, 180 | `hd.matches[i].seed_red/blue` — initial load |
        | `src/gui/app_state_manager.c` | 187, 190 | `hd.matches[i].participant_*` — name copy |
        | `src/gui/app_state_manager.c` | 302, 306 | `cached_highlights.matches[i].seed_*` — key handler |

        Report: "Search complete, N locations confirmed." If you find additional locations, list them — they must be handled before proceeding.

---

## Phase 1: Expand the Highlight Pool in the Network Layer

*Goal: Allow the C network parser to receive and store up to 10 matches instead of 4. This is a pure data-capacity change — no behavior change yet. The kiosk will still display only 4 matches after this phase.*

- [ ] **Step 1.1: Expand `MatchHighlight matches[4]` in `network_io.h`**
    - [ ] **Action:** Open `src/io/network_io.h`. Find line 37:
        ```c
            MatchHighlight matches[4];     // Assume top 4 highlights for Multicam
        ```
        Replace it with:
        ```c
            MatchHighlight matches[10];    // KI-Agent unterstützt: Pool of up to 10 highlights for round-robin rotation (ADR-0024)
        ```
    - [ ] **Verification:** Run:
        ```bash
        grep -n "matches\[" src/io/network_io.h
        ```
        **Expected result:** `matches[10]` is shown. Report the exact grep output.

- [ ] **Step 1.2: Expand the parse loop cap in `network_io.c`**
    - [ ] **Action:** Open `src/io/network_io.c`. Find line 147:
        ```c
                        for (int i = 0; i < size && i < 4; i++) {
        ```
        Replace it with:
        ```c
                        for (int i = 0; i < size && i < 10; i++) {  // KI-Agent unterstützt: expanded to full pool (ADR-0024)
        ```
    - [ ] **Verification:** Run:
        ```bash
        grep -n "i < 4\|i < 10" src/io/network_io.c
        ```
        **Expected result:** Only `i < 10` appears (no remaining `i < 4`). Report the exact grep output.

- [ ] **Step 1.3: Build and confirm zero warnings**
    - [ ] **Action:** Run:
        ```bash
        make
        ```
    - [ ] **Verification:** Report the full compiler output.
        **Expected result:** `gcc` produces no warnings (`-Wall -Wextra`). The three binaries are rebuilt. If warnings appear, fix them before continuing.

---

## Phase 2: Add Pool State to `KioskController`

*Goal: Give `KioskController` the two fields it needs to track which part of the highlight pool is currently displayed.*

- [ ] **Step 2.1: Add `highlight_pool_index` and `highlight_pool_size` to the struct**
    - [ ] **Action:** Open `src/gui/app_state_manager.h`. Find the `KioskController` struct (lines 20–34). Locate the line:
        ```c
            HighlightData     cached_highlights;
        ```
        Add the two new fields immediately after it:
        ```c
            HighlightData     cached_highlights;

            // KI-Agent unterstützt: Round-robin pool state (ADR-0024)
            int highlight_pool_index;   // index of first match in current display window
            int highlight_pool_size;    // total matches available in cached_highlights
        ```
    - [ ] **Verification:** Run:
        ```bash
        grep -n "highlight_pool" src/gui/app_state_manager.h
        ```
        **Expected result:** Two lines appear — `highlight_pool_index` and `highlight_pool_size`. Report the output.

- [ ] **Step 2.2: Initialize the new fields in `init_kiosk_controller()`**
    - [ ] **Action:** Open `src/gui/app_state_manager.c`. Find `init_kiosk_controller()` (line 51). Locate the line that sets `ctrl->initialized = true;` (line 87). Insert the initialization of the two new fields directly before it:
        ```c
            // KI-Agent unterstützt: Pool starts empty; populated on first API response (ADR-0024)
            ctrl->highlight_pool_index = 0;
            ctrl->highlight_pool_size  = 0;
            ctrl->initialized = true;
        ```
    - [ ] **Verification:** Run:
        ```bash
        grep -n "highlight_pool" src/gui/app_state_manager.c
        ```
        **Expected result:** The two initialization lines appear. Report the output.

- [ ] **Step 2.3: Build and confirm zero warnings**
    - [ ] **Action:** Run:
        ```bash
        make
        ```
    - [ ] **Verification:** Report the full compiler output.
        **Expected result:** Zero warnings. If the compiler reports an error about unknown struct members, check that the struct definition in Step 2.1 was saved correctly.

---

## Phase 3: Extract `load_kiosk_sims_from_pool()` Helper

*Goal: Move the inline sim-loading block (currently triggered only when new data arrives) into a reusable static function. This function can then be called both on new data and on each LEADERBOARD → MULTICAM transition.*

- [ ] **Step 3.1: Write the static helper function**
    - [ ] **Action:** Open `src/gui/app_state_manager.c`. Find line 90 (the line `static double time_since_last_input = 0.0;`). Insert the following new static function **immediately before** that line (i.e., after `init_kiosk_controller` ends at line 88 and the blank line 89):
        ```c
        // KI-Agent unterstützt: Loads match_count sims from the pool starting at highlight_pool_index (ADR-0024)
        // Uses modulo wrap-around so the pool is cycled continuously.
        static void load_kiosk_sims_from_pool(void) {
            if (kiosk_ctrl.highlight_pool_size == 0) return;  // guard: no data yet
            for (int i = 0; i < kiosk_ctrl.match_count; i++) {
                int slot = (kiosk_ctrl.highlight_pool_index + i) % kiosk_ctrl.highlight_pool_size;
                MatchHighlight *m = &kiosk_ctrl.cached_highlights.matches[slot];

                int stride = KIOSK_SIM_COLS + 2;
                World *w   = kiosk_ctrl.sims[i].current_world;
                for (int k = 0; k < (KIOSK_SIM_ROWS + 2) * stride; k++) w->grid[k] = DEAD;
                if (w->chunk_map) memset(w->chunk_map, 0, w->chunk_rows * w->chunk_cols);

                for (int r = 0; r < LOCAL_GRID_SIZE; r++) {
                    for (int c = 0; c < LOCAL_GRID_SIZE; c++) {
                        if (m->seed_red[r * LOCAL_GRID_SIZE + c] == 1) {
                            w->grid[(r+1)*stride + (c+1)] = TEAM_RED;
                            activate_chunk_at(w, r, c);
                        }
                        if (m->seed_blue[r * LOCAL_GRID_SIZE + c] == 1) {
                            w->grid[(r+1)*stride + (c+LOCAL_GRID_SIZE+1)] = TEAM_BLUE;
                            activate_chunk_at(w, r, c + LOCAL_GRID_SIZE);
                        }
                    }
                }
                strncpy(kiosk_ctrl.sims[i].participant_red,  m->participant_red,
                        sizeof(kiosk_ctrl.sims[i].participant_red) - 1);
                kiosk_ctrl.sims[i].participant_red[sizeof(kiosk_ctrl.sims[i].participant_red) - 1] = '\0';
                strncpy(kiosk_ctrl.sims[i].participant_blue, m->participant_blue,
                        sizeof(kiosk_ctrl.sims[i].participant_blue) - 1);
                kiosk_ctrl.sims[i].participant_blue[sizeof(kiosk_ctrl.sims[i].participant_blue) - 1] = '\0';
            }
        }
        ```
    - [ ] **Verification:** Run:
        ```bash
        grep -n "load_kiosk_sims_from_pool" src/gui/app_state_manager.c
        ```
        **Expected result:** The function definition appears. Report the line number.

- [ ] **Step 3.2: Replace the inline loading block in the new-data handler**
    - [ ] **Action:** Open `src/gui/app_state_manager.c`. Find the block that currently starts at line 162:
        ```c
            HighlightData hd;
            if (network_get_highlights(&hd)) {
                kiosk_ctrl.cached_highlights = hd;
                printf("--- Highlights Received: %d matches ---\n", hd.count);
                
                if (current_state == STATE_KIOSK_MODE) {
                    // KI-Agent unterstützt: loop bound and world size from named constants (ADR-0022)
                    for (int i = 0; i < kiosk_ctrl.match_count && i < hd.count; i++) {
                        // KI-Agent unterstützt: 8x16 world mirrors run_isolated_match() exactly (ADR-0023)
                        int stride = KIOSK_SIM_COLS + 2;  // 18
                        World* w = kiosk_ctrl.sims[i].current_world;
                        for (int k = 0; k < (KIOSK_SIM_ROWS + 2) * stride; k++) w->grid[k] = DEAD;
                        if (w->chunk_map) memset(w->chunk_map, 0, w->chunk_rows * w->chunk_cols);
                        for (int r = 0; r < LOCAL_GRID_SIZE; r++) {
                            for (int c = 0; c < LOCAL_GRID_SIZE; c++) {
                                if (hd.matches[i].seed_red[r * LOCAL_GRID_SIZE + c] == 1) {
                                    w->grid[(r+1)*stride + (c+1)] = TEAM_RED;   // left half
                                    activate_chunk_at(w, r, c);
                                }
                                if (hd.matches[i].seed_blue[r * LOCAL_GRID_SIZE + c] == 1) {
                                    w->grid[(r+1)*stride + (c+LOCAL_GRID_SIZE+1)] = TEAM_BLUE;  // right half
                                    activate_chunk_at(w, r, c + LOCAL_GRID_SIZE);
                                }
                            }
                        }
                        // KI-Agent unterstützt: Forward participant names for kiosk HUD (ADR-0021)
                        strncpy(kiosk_ctrl.sims[i].participant_red, hd.matches[i].participant_red,
                                sizeof(kiosk_ctrl.sims[i].participant_red) - 1);
                        kiosk_ctrl.sims[i].participant_red[sizeof(kiosk_ctrl.sims[i].participant_red) - 1] = '\0';
                        strncpy(kiosk_ctrl.sims[i].participant_blue, hd.matches[i].participant_blue,
                                sizeof(kiosk_ctrl.sims[i].participant_blue) - 1);
                        kiosk_ctrl.sims[i].participant_blue[sizeof(kiosk_ctrl.sims[i].participant_blue) - 1] = '\0';
                    }
                }
            }
        ```
        Replace the entire block with:
        ```c
            HighlightData hd;
            if (network_get_highlights(&hd)) {
                // KI-Agent unterstützt: New epoch data — reset pool to start (ADR-0024)
                kiosk_ctrl.cached_highlights    = hd;
                kiosk_ctrl.highlight_pool_index = 0;
                kiosk_ctrl.highlight_pool_size  = hd.count;
                printf("--- Highlights Received: %d matches (pool reset) ---\n", hd.count);

                if (current_state == STATE_KIOSK_MODE) {
                    load_kiosk_sims_from_pool();
                }
            }
        ```
    - [ ] **Verification:** Build and confirm zero warnings:
        ```bash
        make
        ```
        **Expected result:** Zero warnings, all three binaries rebuilt. Report the full compiler output.

---

## Phase 4: Wire the Round-Robin Rotation

*Goal: Advance the pool index at the MULTICAM → LEADERBOARD transition and reload sims at the LEADERBOARD → MULTICAM transition. Also update the KEY_1..KEY_4 match-select handler to use the slot formula.*

- [ ] **Step 4.1: Advance the pool index at MULTICAM → LEADERBOARD transition**
    - [ ] **Action:** Open `src/gui/app_state_manager.c`. Find the `KIOSK_SUB_MULTICAM` timer block (around line 271):
        ```c
                if (kiosk_ctrl.state_timer > 30.0f) {
                    kiosk_ctrl.current_sub_state = KIOSK_SUB_LEADERBOARD;
                    kiosk_ctrl.state_timer = 0.0f;
                    network_fetch_leaderboard_async();
                }
        ```
        Replace it with:
        ```c
                if (kiosk_ctrl.state_timer > 30.0f) {
                    // KI-Agent unterstützt: Advance round-robin index before switching away (ADR-0024)
                    if (kiosk_ctrl.highlight_pool_size > 0) {
                        kiosk_ctrl.highlight_pool_index =
                            (kiosk_ctrl.highlight_pool_index + kiosk_ctrl.match_count)
                            % kiosk_ctrl.highlight_pool_size;
                    }
                    kiosk_ctrl.current_sub_state = KIOSK_SUB_LEADERBOARD;
                    kiosk_ctrl.state_timer = 0.0f;
                    network_fetch_leaderboard_async();
                }
        ```
    - [ ] **Verification:** Run:
        ```bash
        grep -n "highlight_pool_index\|highlight_pool_size" src/gui/app_state_manager.c
        ```
        **Expected result:** Three locations: `init_kiosk_controller` (both fields set to 0), `network_get_highlights` block (both fields assigned), and the new advance block (both fields used). Report the line numbers.

- [ ] **Step 4.2: Load sims from new pool position at LEADERBOARD → MULTICAM transition**
    - [ ] **Action:** Find the `KIOSK_SUB_LEADERBOARD` timer block (around line 254):
        ```c
                if (kiosk_ctrl.state_timer > 15.0f) {
                    kiosk_ctrl.current_sub_state = KIOSK_SUB_MULTICAM;
                    kiosk_ctrl.state_timer = 0.0f;
                    network_fetch_highlights_async();
                }
        ```
        Replace it with:
        ```c
                if (kiosk_ctrl.state_timer > 15.0f) {
                    kiosk_ctrl.current_sub_state = KIOSK_SUB_MULTICAM;
                    kiosk_ctrl.state_timer = 0.0f;
                    network_fetch_highlights_async();
                    // KI-Agent unterstützt: Load the next rotation window from the pool (ADR-0024)
                    load_kiosk_sims_from_pool();
                }
        ```
    - [ ] **Verification:** Run:
        ```bash
        grep -n "load_kiosk_sims_from_pool" src/gui/app_state_manager.c
        ```
        **Expected result:** Two call sites appear: one in the `network_get_highlights` block, one in the `KIOSK_SUB_LEADERBOARD` timer block. Report both line numbers.

- [ ] **Step 4.3: Update the KEY_1..KEY_4 match-select handler**
    - [ ] **Action:** Find the keyboard handler (around line 279). The inner loop currently reads seeds from `kiosk_ctrl.cached_highlights.matches[i]`. With round-robin, quadrant `i` displays pool slot `(pool_index + i) % pool_size`. The seed must be read from that same slot.

        Find these two lines (around 302 and 306):
        ```c
                                if (kiosk_ctrl.cached_highlights.matches[i].seed_blue[r * 8 + c]) {
        ```
        and
        ```c
                                if (kiosk_ctrl.cached_highlights.matches[i].seed_red[r * 8 + c]) {
        ```

        The `IsKeyPressed(KEY_ONE + i)` block starts with (around line 280):
        ```c
                    if (IsKeyPressed(KEY_ONE + i)) {
        ```
        Immediately after that opening brace, insert a slot variable (before `reset_simulation_context`):
        ```c
                    if (IsKeyPressed(KEY_ONE + i)) {
                        // KI-Agent unterstützt: Resolve quadrant index to current pool slot (ADR-0024)
                        int slot = (kiosk_ctrl.highlight_pool_size > 0)
                            ? (kiosk_ctrl.highlight_pool_index + i) % kiosk_ctrl.highlight_pool_size
                            : i;
        ```
        Then replace both occurrences of `.matches[i].seed_blue` and `.matches[i].seed_red` in that block:
        - `kiosk_ctrl.cached_highlights.matches[i].seed_blue[r * 8 + c]`
          → `kiosk_ctrl.cached_highlights.matches[slot].seed_blue[r * 8 + c]`
        - `kiosk_ctrl.cached_highlights.matches[i].seed_red[r * 8 + c]`
          → `kiosk_ctrl.cached_highlights.matches[slot].seed_red[r * 8 + c]`
    - [ ] **Verification:** Run:
        ```bash
        grep -n "cached_highlights\.matches\[" src/gui/app_state_manager.c
        ```
        **Expected result:** All remaining occurrences use `matches[slot]`, none use `matches[i]`. Report the output.

- [ ] **Step 4.4: Build — zero warnings required**
    - [ ] **Action:** Run:
        ```bash
        make
        ```
    - [ ] **Verification:** Report the full compiler output.
        **Expected result:** Zero warnings from `-Wall -Wextra`. If `slot` is reported as unused (because the block was not correctly updated), go back to Step 4.3. Any warning is a blocker.

---

## Phase 5: Interactive End-to-End Verification

*Goal: Confirm the rotation works correctly across at least two full Multicam cycles and that the match-select key (KEY_1) still launches the correct match.*

Before starting: make sure the Docker backend is running so the kiosk can fetch highlight data.

```bash
docker-compose up -d
```

Wait until you see `matchmaker-1 | INFO:epoch_worker:Database updated with Epoch results.` in the logs (one full epoch must complete before highlights are available).

- [ ] **Step 5.1: Start the app and enter Kiosk Mode**
    - [ ] **Action:** Start the application:
        ```bash
        ./build/biotope
        ```
        Then press `K` to enter Kiosk Mode.
    - [ ] **Verification (Interactive Test):**
        1. The Leaderboard phase appears (15 seconds).
        2. After 15 s, the Multicam view (2×2 grid) appears.
        3. Observe the **top-left quadrant**: note the two player names shown (e.g., "Fritz1 vs Fritz2").
        4. Observe all four quadrants: confirm all show running simulations and player names.
        5. Report: names of all 4 match pairs visible in the first Multicam cycle.
        **Expected result:** 4 quadrants, 4 different match pairs, all simulations running.

- [ ] **Step 5.2: Observe the second Multicam cycle**
    - [ ] **Action:** Let the kiosk run. After 30 s the Multicam view transitions to the Leaderboard (15 s), then back to Multicam. Watch what appears in the second Multicam cycle.
    - [ ] **Verification (Interactive Test):**
        1. Note the player names in the top-left quadrant of the **second Multicam cycle**.
        2. Compare to the names you noted in Step 5.1.
        3. Report: are the names different from the first cycle? List all 4 match pairs.
        **Expected result:** The second cycle shows a different set of 4 matches (the next 4 from the pool). If fewer than 8 non-oscillating highlights exist, some matches may repeat — that is correct behavior (modulo wrap-around). If the names are **identical** to cycle 1, the rotation is not working: stop and report before continuing.

- [ ] **Step 5.3: Observe the third Multicam cycle (wrap-around)**
    - [ ] **Action:** Let the kiosk continue for one more full cycle (30 s Multicam + 15 s Leaderboard).
    - [ ] **Verification (Interactive Test):**
        1. Note the player names in the third Multicam cycle.
        2. Compare to cycles 1 and 2.
        3. If the pool has 10 matches and match_count is 4: cycle 3 should show matches 8, 9, 0, 1 (wraps around).
        4. If the pool has fewer matches (e.g., 4): cycle 2 should already show matches 0–3 again (correct wrap-around).
        5. Report: the names shown in cycle 3.
        **Expected result:** Wrap-around is seamless — no crash, no blank quadrant, correct match names.

- [ ] **Step 5.4: Verify the KEY_1 match-select**
    - [ ] **Action:** During a Multicam cycle, press key `1` on the keyboard. This should launch a full-screen replay of the match shown in quadrant 1 (top-left).
    - [ ] **Verification (Interactive Test):**
        1. Does pressing `1` launch a single-match full-screen view?
        2. Is the player pair shown in that full-screen view the **same pair** that was in the top-left Multicam quadrant?
        3. Report: the pair name in the top-left quadrant and the pair name in the full-screen replay.
        **Expected result:** Both names match. This confirms the slot formula in the key handler (Step 4.3) is correct.

---

## Phase 6: Documentation

*Goal: Mark the feature complete in all documentation artifacts.*

- [ ] **Step 6.1: Update ADR-0024 status to Implemented**
    - [ ] **Action:** Open `docs/adr/ADR-0024-highlight-rotation-round-robin.md`. Change:
        ```
        **Status:** Proposed
        ```
        to:
        ```
        **Status:** Implemented
        ```
    - [ ] **Verification:** Run:
        ```bash
        grep "Status" docs/adr/ADR-0024-highlight-rotation-round-robin.md
        ```
        **Expected result:** `**Status:** Implemented`

- [ ] **Step 6.2: Add CHANGELOG entry**
    - [ ] **Action:** Open `docs/CHANGELOG.md`. Add the following entry under a new section at the top (after the existing 2026-05-31 entry):
        ```markdown
        ### 2026-05-31 — ADR-0024: Client-Side Highlight Rotation (Round-Robin)
        - `src/io/network_io.h`: `MatchHighlight matches[4]` expanded to `matches[10]`.
        - `src/io/network_io.c`: parse loop cap raised from 4 to 10.
        - `src/gui/app_state_manager.h`: `highlight_pool_index` and `highlight_pool_size`
          added to `KioskController`.
        - `src/gui/app_state_manager.c`: `load_kiosk_sims_from_pool()` extracted as
          static helper; pool index advances by `match_count` on each MULTICAM cycle;
          KEY_1..KEY_4 handler updated to use slot formula.
        - Effect: The kiosk now cycles through all available highlights in round-robin
          order, showing different matches on each 30-second display cycle.
        ```
    - [ ] **Verification:** Confirm the entry is saved.

- [ ] **Step 6.3: Mark all tasks in this file as done**
    - [ ] **Action:** Replace all `- [ ]` with `- [x]` in this file once all prior steps are complete.
    - [ ] **Verification:** Run:
        ```bash
        grep -c "\- \[ \]" docs/tasks/DEV_TASKS-0024-highlight-rotation.md
        ```
        **Expected result:** `0`
