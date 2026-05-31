# DEV_TASKS-0022: Kiosk UI Architecture Refactor & UX Enhancement

Two-phase refactor of the Kiosk Mode rendering. Phase A establishes a scalable foundation;
Phase B implements 10 UX improvements using that foundation.

**Developer:** Follow each step precisely. Every step ends with a compile check or an interactive
test. Do not proceed to the next step until the current verification passes. Report the outcome
of each interactive test before continuing. This cadence is intentional — quality over speed.

**Briefing Documents:**
- [ADR-0022](../adr/ADR-0022-kiosk-ui-architecture-and-ux-refactor.md)
- [DEV_SPEC-0022](../specs/DEV_SPEC-0022-kiosk-ui-architecture-and-ux-refactor.md)

**Coding rules (binding for all Phase B steps):**
- Every new or significantly modified function: add `// KI-Agent unterstützt` comment.
- All pixel values must come from `KioskLayout` fields — no new magic numbers anywhere.
- `snake_case` for variables/functions, 4-space indent, no trailing whitespace.
- After every sub-step that touches a `.c` file: run `make` and confirm zero warnings before moving on.

---

## Phase A: Scalable Foundation ✓

*Completed and tested 2026-05-31. All steps below are verified.*

- [x] **Step A.1:** Add `KioskLayout` struct and `compute_kiosk_layout()` to `renderer.h` / `renderer.c`
- [x] **Step A.2:** Add named constants and update `KioskController` in `app_state_manager.h`
- [x] **Step A.3:** Implement `init_kiosk_controller()` and `free_kiosk_controller()` in `app_state_manager.c`
- [x] **Step A.4:** Replace all hardcoded `4`s and `50`s with `match_count` / `KIOSK_SIM_WORLD_SIZE`
- [x] **Step A.5:** Update `main.c` cleanup to use `free_kiosk_controller()`
- [x] **Step A.6:** Full build and functional test (without backend + with backend) — all confirmed

---

## Phase B: UX Enhancement

*Goal: Implement 10 UX improvements using the KioskLayout foundation from Phase A.
Every pixel size must come from `KioskLayout` — zero new magic numbers allowed.*

### Pre-flight check before starting Phase B

- [x] **Pre-flight:** Confirm clean build baseline.
    - [x] **Action:** Run `make` in the project root.
    - [x] **Verification:** Terminal output ends with the `biotope` link line and shows **zero warnings**.
    - [x] **Action:** Run `./build/biotope`, wait 5 seconds, close the window.
    - [x] **Verification:** App starts and closes without crash. If either check fails, resolve before continuing.

---

## Part 1 — Extract Render Functions (Step B.1)

*Goal: Move the two kiosk render blocks out of the monolithic `draw_current_state()` switch into
their own dedicated static functions. Zero visual change. This is the prerequisite for all
subsequent steps — do not skip.*

- [x] **Step B.1.1: Add forward declarations for the two new functions**
    - [x] **Action:** Open `src/gui/renderer.c`. Find the line that begins
      `void draw_current_state(AppState state, ...` (around line 900 after Phase A).
      Directly **above** that function definition, insert these two lines:
        ```c
        static void draw_kiosk_leaderboard(const KioskController *ctrl, int screen_w, int screen_h);
        static void draw_kiosk_multicam(const KioskController *ctrl, int screen_w, int screen_h);
        ```
    - [x] **Verification:** Run `make`. Zero warnings expected (forward declarations alone introduce no warnings).

- [x] **Step B.1.2: Create `draw_kiosk_leaderboard()` function body**
    - [x] **Action:** Scroll to the very **end** of `renderer.c` (after `draw_current_state()` closes).
      Add a new function:
        ```c
        // KI-Agent unterstützt: Dedicated leaderboard render function extracted from draw_current_state (ADR-0022)
        static void draw_kiosk_leaderboard(const KioskController *ctrl, int screen_w, int screen_h) {
        ```
    - [x] **Action:** Copy the entire content of the `if (kiosk_ctrl.current_sub_state == KIOSK_SUB_LEADERBOARD)`
      block from inside `draw_current_state()` and paste it as the body of this new function.
      Close the function with `}`.
    - [x] **Action:** Inside the pasted body, replace every occurrence of `screenWidth` with `screen_w`
      and every occurrence of `screenHeight` with `screen_h`.
    - [x] **Action:** Replace every occurrence of `kiosk_ctrl` with `ctrl->` (adjust member access
      accordingly, e.g. `kiosk_ctrl.cached_lb` → `ctrl->cached_lb`).
    - [x] **Verification:** Run `make`. Expect warnings about the function being defined but not yet
      called — that is acceptable at this step. Expect **zero errors**.

- [x] **Step B.1.3: Create `draw_kiosk_multicam()` function body**
    - [x] **Action:** Directly after the closing `}` of `draw_kiosk_leaderboard()`, add:
        ```c
        // KI-Agent unterstützt: Dedicated multicam render function extracted from draw_current_state (ADR-0022)
        static void draw_kiosk_multicam(const KioskController *ctrl, int screen_w, int screen_h) {
        ```
    - [x] **Action:** Copy the entire content of the
      `else if (kiosk_ctrl.current_sub_state == KIOSK_SUB_MULTICAM)` block and paste it as the body.
      Close with `}`.
    - [x] **Action:** Replace `screenWidth` → `screen_w`, `screenHeight` → `screen_h`,
      `kiosk_ctrl` → `ctrl->` (member access) throughout the pasted body.
    - [x] **Action:** The `DrawGridAndCellsCtx` calls pass `config` as second argument.
      Since `config` is `(void)`d inside that function (ADR-0020), replace it with `NULL`:
        ```c
        DrawGridAndCellsCtx(&ctrl->renders[i], NULL, ctrl->sims[i].current_world, false);
        ```
    - [x] **Verification:** Run `make`. Zero errors expected. Unused-function warnings still
      acceptable at this point.

- [x] **Step B.1.4: Replace inline blocks in `draw_current_state()` with function calls**
    - [x] **Action:** In `draw_current_state()`, locate the `case STATE_KIOSK_MODE:` block.
      Replace the entire `if / else if` structure (both render blocks) with:
        ```c
        case STATE_KIOSK_MODE:
            if (kiosk_ctrl.current_sub_state == KIOSK_SUB_LEADERBOARD)
                draw_kiosk_leaderboard(&kiosk_ctrl, screenWidth, screenHeight);
            else
                draw_kiosk_multicam(&kiosk_ctrl, screenWidth, screenHeight);
            break;
        ```
    - [x] **Verification:** Run `make`. **Zero warnings, zero errors.** This is the go/no-go gate
      for the rest of Phase B.

- [x] **Step B.1.5: Interactive test — confirm zero visual change**
    - [x] **Verification (Interactive Test):**
        1. Start the app: `./build/biotope`
        2. The leaderboard screen appears. Observe: layout, text positions, progress bar, and
           `PRESS [P] TO PLAY` look **identical** to before this step.
        3. Wait 15 seconds. The multicam view appears. Observe: 4 quadrants, header strips,
           score bars, thumbnails — all **identical** to before this step.
        4. Press `[P]`. Config screen appears. Press `[K]`. Kiosk returns.
        5. Close the window. No crash.
        - **Expected Result:** Visually and functionally indistinguishable from the pre-B.1 state.
          Report any deviation immediately.

---

## Part 2 — Fix Interaction Model (Step B.2)

*Goal: Remove the mouse-based match replay trigger and replace it with keyboard keys `[1]`–`[4]`.
Update the on-screen instruction to reflect the real interaction.*

- [x] **Step B.2.1: Remove the mouse handler in `app_state_manager.c`**
    - [x] **Action:** Open `src/gui/app_state_manager.c`. Find the block:
        ```c
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            for (int i = 0; i < kiosk_ctrl.match_count; i++) {
                if (CheckCollisionPointRec(mousePos, kiosk_ctrl.renders[i].viewport_bounds)) {
        ```
      Delete the entire `if (IsMouseButtonPressed(...))` block including its closing braces.
      Keep everything outside this block intact.
    - [x] **Verification:** Run `make`. Zero warnings.

- [x] **Step B.2.2: Add keyboard replay trigger**
    - [x] **Action:** In the same `KIOSK_SUB_MULTICAM` section of `update_app_state()`, directly
      after the `kiosk_ctrl.state_timer > 30.0f` transition block, add:
        ```c
        // KI-Agent unterstützt: Keyboard match selection replaces mouse click (ADR-0022)
        // Keys [1]..[match_count] trigger a replay of the corresponding match.
        for (int i = 0; i < kiosk_ctrl.match_count; i++) {
            if (IsKeyPressed(KEY_ONE + i)) {
                reset_simulation_context(sim_ctx, config->rows, config->cols);
                // [paste the body of the former mouse-click block here, replacing the
                //  hardcoded index with variable i]
                if (session_origin) *session_origin = ORIGIN_KIOSK_REPLAY;
                interactive_time_accumulator = 0.0f;
                return STATE_IGNITION;
            }
        }
        ```
      **Note:** `KEY_ONE + i` maps to `KEY_ONE` (i=0), `KEY_TWO` (i=1), etc.
      This is valid in Raylib because the KEY_* constants for digit keys are sequential.
    - [x] **Verification:** Run `make`. Zero warnings.

- [x] **Step B.2.3: Update the on-screen CTA in `draw_kiosk_multicam()`**
    - [x] **Action:** Open `src/gui/renderer.c`. In `draw_kiosk_multicam()`, find:
        ```c
        DrawText("CLICK ANY MATCH TO VIEW REPLAY", ...
        ```
      Replace it with:
        ```c
        DrawText("[1-4] WATCH MATCH  |  [P] PLAY",
                 screen_w - MeasureText("[1-4] WATCH MATCH  |  [P] PLAY", 16) - 20,
                 12, 16, THEME_ACCENT);
        ```
    - [x] **Verification:** Run `make`. Zero warnings.

- [x] **Step B.2.4: Interactive test — keyboard replay**
    - [x] **Verification (Interactive Test):**
        1. Start `./build/biotope`. Wait for the multicam view (15 s).
        2. Read the top-right corner. It should now say
           `[1-4] WATCH MATCH  |  [P] PLAY` — not "CLICK ANY MATCH".
        3. Press `[1]`. The app should transition into the ignition countdown and then
           `STATE_RUNNING`, showing a full-screen simulation.
        4. Press `[Q]` or `[BACKSPACE]`. The app should return to kiosk mode.
        5. Press `[2]`, `[3]`, `[4]` in the multicam view — each should trigger a replay.
        - **Expected Result:** Mouse click no longer triggers replay. Keys `[1]`–`[4]` work.
          "CLICK ANY MATCH" is gone from the screen. Report any deviation.

---

## Part 3 — Leaderboard UX (Steps B.3–B.6)

*Goal: Footer backing panel, enlarged QR panel, top-3 row highlights, column rename.
All changes confined to `draw_kiosk_leaderboard()` and `compute_kiosk_layout()`.
After each step: compile. Interactive test at end of Part 3.*

- [x] **Step B.3: Footer backing panel — leaderboard**
    - [x] **Action:** Open `src/gui/renderer.c`. In `draw_kiosk_leaderboard()`, find the section
      that draws the progress bar and the `PRESS [P] TO PLAY` text (currently near the bottom
      of the function). **Before** those drawing calls, insert:
        ```c
        // KI-Agent unterstützt: Solid footer panel backing CTA and progress bar (ADR-0022)
        KioskLayout layout = compute_kiosk_layout(screen_w, screen_h, ctrl->match_count);
        int footer_y = screen_h - layout.bottom_panel_h;
        DrawRectangle(0, footer_y, screen_w, layout.bottom_panel_h, THEME_HUD);
        ```
    - [x] **Action:** Update the y-coordinates of the progress bar and CTA text to use
      `footer_y` as their baseline instead of the current hardcoded values:
        - Progress bar: `int bar_y = screen_h - 22;` → `int bar_y = footer_y + layout.bottom_panel_h - 14;`
        - CTA text: `screen_h - 46` → `footer_y + 8`
    - [x] **Verification:** Run `make`. Zero warnings.

- [x] **Step B.4: QR code as a dedicated panel**
    - [x] **Action:** In `draw_kiosk_leaderboard()`, find the QR placeholder drawing block
      (the section starting with `int qr_size = 90;`). Replace the entire block with the
      following panel-based layout:
        ```c
        // KI-Agent unterstützt: QR panel — proportional to screen, not a fixed 90x90 corner orphan (ADR-0022)
        int panel_w  = screen_w / 4;
        int panel_h  = (int)(screen_h * 0.42f);
        int panel_x  = screen_w - panel_w - 20;
        int panel_y  = (screen_h - layout.bottom_panel_h - panel_h) / 2;
        int qr_size  = (int)(panel_w * 0.55f);
        if (qr_size < 140) qr_size = 140;
        int qr_x     = panel_x + (panel_w - qr_size) / 2;
        int qr_y     = panel_y + 20;
        int font_cta = layout.font_cta;
        int font_url = font_cta - 2;

        // Panel background and border
        DrawRectangle(panel_x - 2, panel_y - 2, panel_w + 4, panel_h + 4,
                      Fade(THEME_ACCENT, 0.25f));
        DrawRectangle(panel_x, panel_y, panel_w, panel_h, THEME_HUD);

        // QR placeholder (stylised dot grid — same pattern as existing)
        DrawRectangle(qr_x - 4, qr_y - 4, qr_size + 8, qr_size + 8, THEME_TEXT);
        DrawRectangle(qr_x, qr_y, qr_size, qr_size, BLACK);
        int cell = qr_size / 10;
        for (int qi = 0; qi < 9; qi++) {
            for (int qj = 0; qj < 9; qj++) {
                if ((qi + qj) % 2 == 0)
                    DrawRectangle(qr_x + qi * cell + 2, qr_y + qj * cell + 2,
                                  cell - 2, cell - 2, THEME_BLUE);
            }
        }

        // CTA text centred in panel below QR
        const char *cta1 = "Submit YOUR strategy!";
        const char *cta2 = "editor.biotope.io";
        int cta1_w = MeasureText(cta1, font_cta);
        int cta2_w = MeasureText(cta2, font_url);
        DrawText(cta1, panel_x + (panel_w - cta1_w) / 2,
                 qr_y + qr_size + 14, font_cta, THEME_ACCENT);
        DrawText(cta2, panel_x + (panel_w - cta2_w) / 2,
                 qr_y + qr_size + 14 + font_cta + 6, font_url, THEME_BLUE);
        ```
    - [x] **Verification:** Run `make`. Zero warnings.

- [x] **Step B.5: Top-3 leaderboard row highlights**
    - [x] **Action:** In `draw_kiosk_leaderboard()`, find the section that defines `rowHeight`
      and the loop that draws leaderboard entries. Currently `rowHeight = 30` is hardcoded.
      Replace it with a proportional value:
        ```c
        // KI-Agent unterstützt: Row height derived from available screen space (ADR-0022)
        int entries_area_h = footer_y - (startY + 40);
        int max_visible     = kMaxVisible; // use existing MAX_LEADERBOARD_ENTRIES cap
        int rowHeight        = (max_visible > 0)
                               ? (entries_area_h / max_visible)
                               : 32;
        if (rowHeight < 28) rowHeight = 28;
        ```
      **Note:** `startY` is already defined in the function; `footer_y` was added in B.3.
    - [x] **Action:** Inside the entry-drawing loop, before the `DrawText` calls for rank 1, 2, 3,
      add a semi-transparent background rectangle:
        ```c
        if (i < 3) {
            Color row_bg = (i == 0) ? Fade(THEME_RED,  0.12f)
                         : (i == 1) ? Fade(THEME_BLUE, 0.12f)
                                    : Fade(THEME_ACCENT, 0.10f);
            DrawRectangle(screen_w / 2 - 240, y - 4,
                          610, rowHeight, row_bg);
        }
        ```
    - [x] **Verification:** Run `make`. Zero warnings.

- [x] **Step B.6: Column header rename — "STAMINA" → "ENDURANCE"**
    - [x] **Action:** In `draw_kiosk_leaderboard()`, find:
        ```c
        DrawText("STAMINA",  screenWidth/2 + 260, startY, 20, THEME_HINT);
        ```
      Replace `"STAMINA"` with `"ENDURANCE"`.
    - [x] **Verification:** Run `make`. Zero warnings.

- [x] **Step B.3–B.6 Interactive test — leaderboard improvements**
    - [x] **Verification (Interactive Test — requires Docker backend):**
        1. Start the backend: `docker compose up` (in a separate terminal). Wait for
           `Application startup complete.`
        2. Start the app: `./build/biotope`
        3. Observe the leaderboard screen. Report for each point:
            - **Footer:** Is there a solid dark panel behind `PRESS [P] TO PLAY` and the
              progress bar? Does the panel visually separate from the content above?
            - **QR panel:** Is there a clearly bordered panel on the right side of the screen?
              Is the QR dot grid noticeably larger than before (≥ 140 px)? Is the text readable?
            - **Top-3 rows:** Do the top 3 entries have a subtle coloured background
              (red tint for #1, blue tint for #2, grey tint for #3)?
            - **Column header:** Does the header now read `ENDURANCE` instead of `STAMINA`?
        - **Expected Result:** All four points confirmed. Leaderboard data still loads and
          displays correctly. Report any deviation or crash.

---

## Part 4 — Multicam UX (Steps B.7–B.11)

*Goal: Proportional score bar, professional screen title, quadrant separator, metric badge,
larger thumbnails. All changes confined to `draw_kiosk_multicam()` and `compute_kiosk_layout()`.
After each step: compile. Interactive test at end of Part 4.*

- [x] **Step B.7: Score bar — proportional height**
    - [x] **Action:** Open `src/gui/renderer.c`. In `compute_kiosk_layout()`, find the line:
        ```c
        l.score_bar_h = 14;
        ```
      Replace with:
        ```c
        // KI-Agent unterstützt: Proportional score bar — readable from 80 cm (ADR-0022)
        l.score_bar_h = l.quad_h / 18;
        if (l.score_bar_h < 20) l.score_bar_h = 20;
        ```
    - [x] **Action:** In `draw_kiosk_multicam()`, find the score-bar drawing section
      (`int bar_y = ry + qh - 14;`). Update it:
        ```c
        KioskLayout layout = compute_kiosk_layout(screen_w, screen_h, ctrl->match_count);
        ```
      Add this call once at the **top** of `draw_kiosk_multicam()` (before the simulation
      rendering loop) so all subsequent steps can use `layout.*` values.
    - [x] **Action:** Replace `int bar_y = ry + qh - 14;` with:
        ```c
        int bar_y = ry + qh - layout.score_bar_h;
        ```
    - [x] **Action:** Replace the hardcoded `14` in `DrawRectangle(rx, bar_y, qw, 14, ...)` calls
      with `layout.score_bar_h`.
    - [x] **Action:** Derive the score font size from bar height. Replace the hardcoded font
      size `11` in the population number `DrawText` calls with:
        ```c
        int score_font = (int)(layout.score_bar_h * 0.72f);
        if (score_font < 14) score_font = 14;
        ```
      Then use `score_font` in the two `DrawText` calls that render the red and blue
      population numbers.
    - [x] **Verification:** Run `make`. Zero warnings.

- [x] **Step B.8: Multicam screen title**
    - [x] **Action:** In `draw_kiosk_multicam()`, find:
        ```c
        DrawText("WUSEL-MULTICAM KIOSK MODE", 20, 10, 20, THEME_BLUE);
        ```
      Replace with:
        ```c
        DrawText("LIVE BATTLES", 20, 10, 20, THEME_BLUE);
        ```
    - [x] **Verification:** Run `make`. Zero warnings.

- [x] **Step B.9: Quadrant separator cross-line**
    - [x] **Action:** In `draw_kiosk_multicam()`, after the two rendering loops
      (after the closing `}` of the HUD overlay loop), add:
        ```c
        // KI-Agent unterstützt: Visible cross-line separating quadrants (ADR-0022)
        // Positions are derived from stored viewport bounds — not hardcoded.
        if (ctrl->match_count > 1) {
            KioskLayout layout = compute_kiosk_layout(screen_w, screen_h, ctrl->match_count);
            int line_top    = (int)ctrl->renders[0].viewport_bounds.y;
            int line_bottom = screen_h - layout.bottom_panel_h;

            // Vertical separators between columns
            for (int col = 1; col < layout.grid_cols; col++) {
                int sep_x = (int)(ctrl->renders[col - 1].viewport_bounds.x
                                + ctrl->renders[col - 1].viewport_bounds.width)
                            + layout.pad / 2;
                DrawRectangle(sep_x - layout.separator_px / 2, line_top,
                              layout.separator_px, line_bottom - line_top,
                              Fade(THEME_GRID, 0.8f));
            }

            // Horizontal separators between rows
            for (int row = 1; row < layout.grid_rows; row++) {
                int first_in_row = row * layout.grid_cols;
                if (first_in_row >= ctrl->match_count) break;
                int sep_y = (int)(ctrl->renders[first_in_row].viewport_bounds.y)
                            - layout.pad / 2;
                DrawRectangle(0, sep_y - layout.separator_px / 2,
                              screen_w, layout.separator_px,
                              Fade(THEME_GRID, 0.8f));
            }
        }
        ```
      **Note:** The `layout` local variable for the separator block is independent of the one
      declared in B.7. If B.7 already added `KioskLayout layout = ...` at the top of the
      function, reuse that variable and remove the duplicate declaration here.
    - [x] **Verification:** Run `make`. Zero warnings.

- [x] **Step B.10: `metric_reason` as a badge below the name strip**
    - [x] **Action:** In `compute_kiosk_layout()`, update the Phase-A placeholder:
        ```c
        l.badge_h = 0;   // not yet rendered (Phase B)
        ```
      Replace with:
        ```c
        // KI-Agent unterstützt: Badge height for metric_reason label (ADR-0022)
        l.badge_h = (int)(l.header_h * 0.85f);
        if (l.badge_h < 16) l.badge_h = 16;
        ```
    - [x] **Action:** In `compute_kiosk_layout()`, update `header_h` to accommodate two rows:
        ```c
        l.header_h = 26;
        ```
      Replace with:
        ```c
        // Combined height: name row + badge row
        l.header_h = 26 + l.badge_h;
        ```
    - [x] **Action:** In `draw_kiosk_multicam()`, find the header-strip drawing block for each
      quadrant. Currently it draws the name strip (26 px) and places `metric_reason` right-aligned
      within it. Update as follows:
        - Keep the name strip at its current height (`layout.header_h - layout.badge_h` px for
          the name row):
            ```c
            DrawRectangle(rx, ry, qw, layout.header_h - layout.badge_h, Fade(THEME_HUD, 0.88f));
            ```
        - Draw a separate badge strip below the name row:
            ```c
            DrawRectangle(rx, ry + layout.header_h - layout.badge_h,
                          qw, layout.badge_h, Fade(THEME_HUD, 0.70f));
            ```
        - Remove the existing right-aligned `DrawText(reason, ...)` call from the name strip.
        - Add the badge text centred in the badge strip:
            ```c
            if (strlen(reason) > 0) {
                int rw = MeasureText(reason, layout.font_badge);
                DrawText(reason,
                         rx + (qw - rw) / 2,
                         ry + layout.header_h - layout.badge_h + (layout.badge_h - layout.font_badge) / 2,
                         layout.font_badge, THEME_ACCENT);
            }
            ```
    - [x] **Verification:** Run `make`. Zero warnings.

- [x] **Step B.11: Seed thumbnails — larger cells**
    - [x] **Action:** In `compute_kiosk_layout()`, replace:
        ```c
        l.seed_cell_px = 5;
        ```
      With:
        ```c
        // KI-Agent unterstützt: Proportional thumbnail cell — visible from standing distance (ADR-0022)
        l.seed_cell_px = l.quad_h / 30;
        if (l.seed_cell_px < 7) l.seed_cell_px = 7;
        ```
    - [x] **Action:** In `draw_kiosk_multicam()`, find the thumbnail drawing section.
      Replace the hardcoded `int thumb_cell = 5;` with:
        ```c
        int thumb_cell = layout.seed_cell_px;
        ```
      The rest of the thumbnail drawing code uses `thumb_cell` throughout, so this single
      change propagates to all thumbnail geometry (backing rect, cell drawing, gap).
    - [x] **Verification:** Run `make`. Zero warnings.

- [x] **Step B.11 — also: Footer backing panel for multicam**
    - [x] **Action:** At the top of `draw_kiosk_multicam()`, directly after the `layout`
      declaration (added in B.7), insert:
        ```c
        // KI-Agent unterstützt: Solid footer panel — same pattern as leaderboard (ADR-0022)
        int footer_y = screen_h - layout.bottom_panel_h;
        DrawRectangle(0, footer_y, screen_w, layout.bottom_panel_h, THEME_HUD);
        ```
    - [x] **Action:** Update the progress bar y-position in `draw_kiosk_multicam()`:
        - Find `int bar_y2 = screenHeight - 22;` (or `screen_h - 22` after B.1 rename).
        - Replace with `int bar_y2 = footer_y + layout.bottom_panel_h - 14;`
    - [x] **Action:** Update the `PRESS [P] TO PLAY` y-position:
        - Find `screen_h - 46` (or equivalent).
        - Replace with `footer_y + 8`.
    - [x] **Verification:** Run `make`. Zero warnings.

- [x] **Step B.7–B.11 Interactive test — multicam improvements**
    - [x] **Verification (Interactive Test — requires Docker backend):**
        1. Ensure Docker backend is running. Start `./build/biotope`.
        2. Wait 15 seconds for the multicam view. Observe and report each point:
            - **Footer:** Is there a solid dark panel behind `[1-4] WATCH MATCH` and the
              progress bar, visually separated from the simulation content?
            - **Score bar:** Is the coloured bar at the bottom of each quadrant noticeably
              taller than before (≥ 20 px)? Are the population numbers readable?
            - **Title:** Does the top-left corner now read `LIVE BATTLES` instead of
              `WUSEL-MULTICAM KIOSK MODE`?
            - **Separator:** Is there a visible line between the four quadrants (horizontal
              and vertical)?
            - **Badge:** Is the `metric_reason` text (e.g. `★ LONGEST MATCH`) displayed as
              a centred badge on its own line below the player names?
            - **Thumbnails:** Are the 8×8 seed patterns noticeably larger than before?
        3. Press `[1]`. Ignition countdown starts. Simulation runs full-screen. Press `[Q]`.
           Returns to kiosk. Cycle works without crash.
        - **Expected Result:** All six visual points confirmed. No regressions in navigation
          or simulation behaviour. Report any deviation.

---

## Part 5 — Final Regression Test (Step B.12)

*Goal: Confirm all 10 UX items are present and all existing features still work correctly.
Run with Docker backend active.*

- [x] **Step B.12: Full regression test**
    - [x] **Action:** Run `make`. Confirm **zero warnings, zero errors**.
    - [x] **Verification (Interactive Test — requires Docker backend):**
        1. Start `docker compose up`. Wait for `Application startup complete.`
        2. Start `./build/biotope`.
        3. **Leaderboard screen (first 15 s):** Confirm all of the following:
            - [x] Real player entries are visible
            - [x] Top 3 entries have coloured background highlights
            - [x] Column header reads `ENDURANCE` (not `STAMINA`)
            - [x] QR panel is a clearly bordered block on the right side (≥ 140 px QR image)
            - [x] CTA text in QR panel is ≥ 20 px and clearly readable
            - [x] Footer panel is solid behind `PRESS [P] TO PLAY` and the progress bar
        4. **Wait 15 s — Multicam screen:** Confirm all of the following:
            - [x] Title reads `LIVE BATTLES`
            - [x] Simulations run with red and blue cells in all 4 quadrants
            - [x] Player names visible in quadrant headers
            - [x] Metric badge visible on its own line below the names
            - [x] Score bar is ≥ 20 px tall, population numbers readable
            - [x] Cross-line separator visible between quadrants
            - [x] Seed thumbnails are larger and recognisable
            - [x] Footer panel is solid — no floating progress bar
            - [x] Top-right corner reads `[1-4] WATCH MATCH  |  [P] PLAY` (not "CLICK ANY MATCH")
        5. **Keyboard navigation:**
            - [x] Press `[1]` → ignition countdown → simulation runs full-screen
            - [x] Press `[Q]` → returns to kiosk leaderboard
            - [x] Press `[P]` → config screen appears
            - [x] Press `[K]` → returns to kiosk leaderboard
        6. **Clean exit:** Close the window. Terminal shows no crash, no memory error.
        - **Expected Result:** All checkboxes above confirmed. Report any single deviation
          before marking this step complete.

- [x] **Step B.12.1: Update CHANGELOG**
    - [x] **Action:** Open `docs/CHANGELOG.md`. Add an entry for ADR-0022 Phase B
      describing the 10 UX improvements and the architectural refactor.
    - [x] **Action:** Mark all Phase B steps in this document as `[x]`.
