# DEV_TASKS-0031: Auto-Paging Rotation for the Kiosk Global Leaderboard

Implementation plan for ADR-0031. The kiosk "GLOBAL LEADERBOARD" screen currently
shows only the top ~9 players. This plan adds time-driven auto-paging so the slot
cycles through all pages (ranks 1–9, 10–18, …) at full row size, and raises the
end-to-end capacity from 20 to 50 ranked players.

**Developer:** Please follow these steps precisely. The plan is broken into phases
and small steps to allow for interruptions and ensure stability. After each
"Verification" step, report the outcome. This iterative process is crucial for
maintaining quality. Recompile with `make` after every C change — the build **must**
stay warning-free (`-Wall -Wextra`). Every function/block you write or significantly
modify must carry a `// KI-Agent unterstützt` comment.

**Briefing Document:**
*   [ADR-0031: Auto-Paging Rotation for the Kiosk Global Leaderboard](../adr/ADR-0031-kiosk-leaderboard-auto-paging.md)

*Note: For this contained kiosk-rendering change, the requirements specification and
technical design are folded into ADR-0031 (Section 2) rather than produced as
separate DEV_SPEC / DEV_TECH_DESIGN documents.*

---

## Phase 1: Backend capacity raise

*Goal: Let the API return up to 50 ranked rows behind a named constant, without
touching the honest `total_count` overflow logic.*

- [x] **Step 1.1: Introduce `LEADERBOARD_MAX_ROWS` and raise the limit**
    - [x] **Action:** In `backend/app/main.py`, add a module-level named constant
      `LEADERBOARD_MAX_ROWS = 50` near the top of the file (with a
      `# KI-Agent unterstützt` comment referencing ADR-0031).
    - [x] **Action:** In `get_leaderboard`, replace the literal `.limit(20)` and
      `.to_list(length=20)` with `.limit(LEADERBOARD_MAX_ROWS)` and
      `.to_list(length=LEADERBOARD_MAX_ROWS)`.
    - [x] **Action:** Leave the `total_count = await db.submissions.count_documents(query)`
      line unchanged — it must remain an independent server-side count so "+N more"
      stays honest beyond 50.
    - [x] **Verification:**
        1.  Start the stack: `docker-compose up backend mongo` (or run the backend
            standalone per CLAUDE.md).
        2.  `curl -s http://localhost:8000/api/leaderboard | python3 -m json.tool`
        3.  **Expected Result:** The response still has `leaderboard` and
            `total_count`. The `leaderboard` array length is `min(eligible_rows, 50)`
            (no longer capped at 20). `total_count` equals the true number of active,
            played submissions. With the current small dataset, behaviour is
            unchanged except the cap is now 50.

## Phase 2: C data-layer capacity raise

*Goal: Allow the client to parse and store up to 50 rows. Pure capacity change, no
behaviour change yet.*

- [x] **Step 2.1: Raise `MAX_LEADERBOARD_ENTRIES`**
    - [x] **Action:** In `src/io/network_io.h`, change
      `#define MAX_LEADERBOARD_ENTRIES 20` to `50`. Update the adjacent comment to
      note the value matches the backend `LEADERBOARD_MAX_ROWS` (ADR-0031).
    - [x] **Action:** Confirm the JSON parsing loop in `src/io/network_io.c`
      (`for (... i < size && i < MAX_LEADERBOARD_ENTRIES; ...)`) and the renderer
      clamp at `renderer.c:1351` already use the macro (they do) — no literal `20`
      should remain for this array.
    - [x] **Action:** Run `make`.
    - [x] **Verification:**
        1.  `make` completes with **zero warnings/errors**.
        2.  `grep -rn "\b20\b" src/io/network_io.c src/io/network_io.h` shows no
            leftover literal tied to the leaderboard array size.
        3.  **Expected Result:** Build is clean; the data layer can now hold 50 rows.
            Visible kiosk behaviour is still unchanged (renderer still clips to ~9).

## Phase 3: Renderer auto-paging

*Goal: Page through all parsed rows at full size, with correct global rank numbering,
podium styling only on the real top-3, a page indicator, and last-page "+N more".*

- [x] **Step 3.1: Add paging constants and the page-count helper**
    - [x] **Action:** In `src/gui/renderer.h`, add the shared constants
      `KIOSK_LB_SECONDS_PER_PAGE (6.0f)`, `KIOSK_LB_MIN_DURATION (15.0f)`,
      `KIOSK_LB_MAX_DURATION (48.0f)` and declare
      `int kiosk_leaderboard_page_count(const KioskController *ctrl, int screen_w, int screen_h);`
      (with `// KI-Agent unterstützt` comments). Place them so both `renderer.c` and
      `app_state_manager.c` can use them.
    - [x] **Action:** In `src/gui/renderer.c`, implement
      `kiosk_leaderboard_page_count`: compute `parsed_entries` (clamped to
      `MAX_LEADERBOARD_ENTRIES`), derive `rows_per_page` from `compute_kiosk_layout`
      using the existing `max_fit` / `rowHeight >= 28` logic, subtract one row from
      `rows_per_page` when `true_total > parsed_entries` (reserve space for "+N more"),
      and return `num_pages = max(1, ceil(parsed_entries / rows_per_page))`.
    - [x] **Action:** Run `make`.
    - [x] **Verification:** `make` is warning-free. (No visible change yet; the helper
      is not wired into rendering.)

- [x] **Step 3.2: Render the current page window**
    - [x] **Action:** In `draw_kiosk_leaderboard`, replace the single-window
      `show_count` logic with paging: compute `rows_per_page`/`num_pages` (reuse the
      helper or a shared static so the math is not duplicated), then
      `page_index = clamp((int)(ctrl->state_timer / KIOSK_LB_SECONDS_PER_PAGE), 0, num_pages - 1)`.
      Render rows `[page_index * rows_per_page, min(parsed_entries, +rows_per_page))`.
    - [x] **Action:** Use the **global** rank for the `#` label and for the top-3
      colour/background highlight: `global_rank = page_index * rows_per_page + i`
      (so podium styling appears only on page 0, rows 0–2).
    - [x] **Action:** Draw "+N more" (`true_total - parsed_entries`) only on the last
      page, using the existing overflow count.
    - [x] **Action:** Run `make`.
    - [x] **Verification (Interactive Test):**
        1.  Ensure the backend has **more than 9** active, played submissions (seed
            test data via the editor / a few submissions if needed). Build the worker
            binaries and let the matchmaker rank them so they appear on the board.
        2.  Start `./build/biotope`, press **[K]** to enter kiosk mode, and wait on the
            GLOBAL LEADERBOARD screen.
        3.  Watch for ~20 seconds without touching anything.
        4.  **Expected Result:** Every ~6 s the table advances to the next page
            (ranks 1–9, then 10–18, …) at the same row size as before. Rank numbers are
            continuous and correct across pages. The red/blue/accent podium highlight
            appears **only** on overall ranks 1–3 (first page), not repeated on later
            pages. When fewer than one page of players exist, the screen looks exactly
            as it did before.

- [x] **Step 3.3: Page indicator**
    - [x] **Action:** When `num_pages > 1`, draw a `PAGE x / y` label near the
      "GLOBAL LEADERBOARD" title (style consistent with `THEME_HINT`/`font_header`).
      Hide it when there is only one page.
    - [x] **Action:** Run `make`.
    - [x] **Verification (Interactive Test):**
        1.  Re-run the kiosk leaderboard with >9 players.
        2.  **Expected Result:** A `PAGE 1 / N … PAGE 2 / N …` indicator updates in
            step with the table; it is absent when only one page of players exists.

## Phase 4: Adaptive slot duration & progress bar

*Goal: The leaderboard slot stays visible long enough to show every page, then hands
off to Multicam as before. Single-page case is unchanged (15 s).*

- [x] **Step 4.1: Drive the sub-state duration from page count**
    - [x] **Action:** In `src/gui/app_state_manager.c`, in the
      `KIOSK_SUB_LEADERBOARD` branch, compute
      `lb_duration = clamp(kiosk_leaderboard_page_count(&kiosk_ctrl, GetScreenWidth(), GetScreenHeight()) * KIOSK_LB_SECONDS_PER_PAGE, KIOSK_LB_MIN_DURATION, KIOSK_LB_MAX_DURATION)`
      and replace the literal `if (kiosk_ctrl.state_timer > 15.0f)` with
      `> lb_duration`.
    - [x] **Action:** In `draw_kiosk_leaderboard`, change the footer progress-bar
      ratio from the literal `15.0f` to the same `lb_duration` (compute it the same
      way so renderer and state machine agree).
    - [x] **Action:** Run `make`.
    - [x] **Verification (Interactive Test):**
        1.  With >9 players, enter kiosk mode and time the leaderboard slot.
        2.  **Expected Result:** The slot lasts ≈ `num_pages × 6 s` (e.g. ~18 s for 3
            pages, ~36 s for 6 pages / 50 players), then switches to Multicam. The
            progress bar fills smoothly over the whole slot and reaches full just as it
            hands off. With ≤ one page of players, the slot is still 15 s — identical
            to current behaviour.

## Phase 5: Regression checks, documentation, cleanup

*Goal: Confirm nothing else regressed and record the change.*

- [x] **Step 5.1: Full build and existing tests**
    - [x] **Action:** Run `make clean && make` — confirm all three binaries build
      warning-free.
    - [x] **Action:** Run the backend tests that touch ranking/integration:
      `python backend/tests/test_ranking.py` and, if binaries are built,
      `python backend/tests/test_system_integration.py`.
    - [x] **Verification:** Build is clean; tests pass (or any failure is understood
      and unrelated to this change).

- [x] **Step 5.2: Overflow ("+N more") edge case**
    - [x] **Action:** Only if practical: temporarily lower `LEADERBOARD_MAX_ROWS` /
      `MAX_LEADERBOARD_ENTRIES` (e.g. to 5) to force `total_count > parsed`, rebuild,
      and confirm "+N more" shows the correct remainder on the **last** page only.
      Restore the values to 50 afterwards.
    - [x] **Verification (Interactive Test):** On the last page, "+N more" shows
      `total_count − parsed` and appears on no other page. After restoring 50, the
      line is absent for the normal dataset.

- [x] **Step 5.3: Update CHANGELOG**
    - [x] **Action:** Add an entry to `docs/CHANGELOG.md` summarising ADR-0031:
      kiosk leaderboard now auto-pages through all players; capacity raised 20 → 50;
      adaptive slot duration.
    - [x] **Action:** Set ADR-0031 **Status** to `accepted` once the interactive
      tests pass.
    - [x] **Verification:** CHANGELOG reflects the change; ADR status updated.
