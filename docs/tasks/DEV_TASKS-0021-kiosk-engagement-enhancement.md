# DEV_TASKS-0021: Kiosk Mode Engagement Enhancement

Implements six targeted improvements to `STATE_KIOSK_MODE` to make the Biotope exhibit compelling for fair visitors: personalized match display, live score bars, context labels, seed thumbnails, a submission CTA, and a visual progress indicator.

**Developer:** Follow these steps precisely. Verify compilation after each phase. Report outcome of each Verification step.

**Briefing Documents:**
*   [ADR-0021: Kiosk Mode Engagement Enhancement](../adr/ADR-0021-kiosk-engagement-enhancement.md)
*   [DEV_SPEC-0021: Requirements Specification](../specs/DEV_SPEC-0021-kiosk-engagement-enhancement.md)
*   [DEV_TECH_DESIGN-0021: Technical Design](../tech_design/DEV_TECH_DESIGN-0021-kiosk-engagement-enhancement.md)

---

## Phase 1: Data Plumbing (P1 + P2)

*Goal: Ensure participant names and live population counts flow from the API data into the KioskController so the renderer can read them.*

- [x] **Step 1.1: Extend `KioskController` with per-quadrant population arrays**
    - [x] **Action:** In `src/gui/app_state_manager.h`, add `int quad_red_pop[4];` and `int quad_blue_pop[4];` to the `KioskController` struct, with the AI attribution comment.
    - [x] **Action:** In `src/gui/app_state_manager.c`, zero-initialize both arrays in the static initializer of `kiosk_ctrl`.
    - [x] **Verification:** Run `make`. Expected: zero warnings, clean compile.

- [x] **Step 1.2: Forward participant names when highlights arrive (P1 fix)**
    - [x] **Action:** In `src/gui/app_state_manager.c`, inside `update_app_state`, locate the `if (network_get_highlights(&hd))` block. After the existing grid-seeding loop for quadrant `i`, add `strncpy` calls to copy `hd.matches[i].participant_red` and `hd.matches[i].participant_blue` into `kiosk_ctrl.sims[i].participant_red/blue`. Add null terminator guard. See DEV_TECH_DESIGN-0021 §4.1 for the exact code.
    - [x] **Verification:** Run `make`. Expected: zero warnings.

- [x] **Step 1.3: Replace discarded population outputs with persistent storage (P2 fix)**
    - [x] **Action:** In `src/gui/app_state_manager.c`, in the `KIOSK_SUB_MULTICAM` simulation tick, remove the `int dummy_red, dummy_blue;` local variables and replace them with `&kiosk_ctrl.quad_red_pop[i]` and `&kiosk_ctrl.quad_blue_pop[i]` as the output arguments to `update_generation_ctx`. See DEV_TECH_DESIGN-0021 §4.2.
    - [x] **Verification:** Run `make`. Expected: zero warnings.

---

## Phase 2: Renderer — Name Header + Score Bar (P1 + P2)

*Goal: Render the player names and live score bar on each Multicam quadrant.*

- [x] **Step 2.1: Draw player name header strip per quadrant**
    - [x] **Action:** In `src/gui/renderer.c`, inside `draw_current_state`, `case STATE_KIOSK_MODE`, `KIOSK_SUB_MULTICAM` branch — after the `DrawGridAndCellsCtx` loop, add a second loop over `i = 0..3` that draws a semi-transparent header strip and the `participant_red` / `vs` / `participant_blue` text. Retrieve `rx`, `ry`, `q_w` from `kiosk_ctrl.renders[i].viewport_bounds`. See DEV_TECH_DESIGN-0021 §5.2.
    - [x] **Action:** Ensure the AI attribution comment `// KI-Agent unterstützt` is present on the new code block.
    - [x] **Verification (Interactive Test):** All 4 quadrants show dark header strip with colored player names (nicknames confirmed with backend data). Layout bug (top quads hidden under global bar) found and fixed by adjusting viewport top_offset to 44px.

- [x] **Step 2.2: Draw live population score bar per quadrant**
    - [x] **Action:** In the same rendering loop, add the score bar at the bottom of each quadrant. Use `kiosk_ctrl.quad_red_pop[i]` and `quad_blue_pop[i]` for proportional fill. See DEV_TECH_DESIGN-0021 §5.4.
    - [x] **Verification (Interactive Test):** Score bar visible and moves proportionally with live Red/Blue cell counts. Shows "0/0" labels without backend (correct). Fills with color when backend provides simulation data.

---

## Phase 3: Renderer — Metric Label + Seed Thumbnail (P3 + P4)

*Goal: Add the contextual label and the 8×8 seed thumbnail to each quadrant.*

- [x] **Step 3.1: Draw metric reason label (P3)**
    - [x] **Action:** In the quadrant HUD loop in `renderer.c`, append a right-aligned metric label using `kiosk_ctrl.cached_highlights.matches[i].metric_reason`. See DEV_TECH_DESIGN-0021 §5.3.
    - [x] **Verification (Interactive Test):** "MOST VOLATILE" displayed right-aligned in header. Raw "activity_sum" translated in `network_io.c`. Previously overwritten by long UUIDs — fixed by nickname substitution in worker.py.

- [x] **Step 3.2: Draw 8×8 seed thumbnail (P4)**
    - [x] **Action:** Add the thumbnail rendering loop below the name header for each quadrant. Draw a dark background rectangle first, then iterate `seed_red[64]` and `seed_blue[64]` from `cached_highlights.matches[i]`. See DEV_TECH_DESIGN-0021 §5.5.
    - [x] **Verification (Interactive Test):** Thumbnails filled with colored cells. Overlap bug (both at same coordinates) found and fixed — now side-by-side: blue left, red right, matching game field layout.

---

## Phase 4: Renderer — Leaderboard CTA + Progress Bar (P5 + P6)

*Goal: Add a submission call-to-action to the leaderboard screen and replace the text timer with a progress bar.*

- [x] **Step 4.1: QR placeholder and CTA text on leaderboard (P5)**
    - [x] **Action:** In `draw_current_state`, `KIOSK_SUB_LEADERBOARD` branch, add a right-side panel with a stylized QR placeholder and the CTA text "Submit YOUR strategy!" + "editor.biotope.io".
    - [x] **Verification (Interactive Test):** QR code placeholder and CTA text confirmed visible on leaderboard screen.

- [x] **Step 4.2: Replace text timer with progress bar (P6)**
    - [x] **Action:** Removed "SWITCHING IN X SECONDS" text from both sub-states. Replaced with thin filling progress bar.
    - [x] **Verification (Interactive Test):** Progress bar confirmed visible and functioning in both leaderboard and multicam sub-states.

---

## Phase 5: Final Verification and Cleanup

*Goal: Confirm zero regressions across all affected states, clean up any debug output.*

- [x] **Step 5.1: Full compilation check**
    - [x] **Verification:** `make clean && make` — zero warnings, all three binaries built successfully.

- [x] **Step 5.2: Interactive mode regression test**
    - [x] **Verification:** Interactive mode unaffected. All states render correctly.

- [x] **Step 5.3: Kiosk click-to-replay regression test**
    - [x] **Verification:** Click on quadrant → countdown → simulation. [Q] from STATE_FINISHED now correctly returns to Kiosk Mode (pre-existing bug fixed: session_origin check was missing in STATE_FINISHED).

- [x] **Step 5.4: Update CHANGELOG**
    - [x] **Verification:** CHANGELOG.md updated with full ADR-0021 entry.
