# Development Log — ADR-0022 Kiosk UI Architecture Refactor (Phase A)

**Date:** 2026-05-31
**Branch:** `biotop`
**AI Agent:** Claude Sonnet 4.6
**Status at end of session:** Phase A complete and tested. Phase B documented, not yet implemented.

---

## 1. Session Goal

Pause after rapid feature development and conduct a structured UX review of the Kiosk Mode
(`STATE_KIOSK_MODE`). Identify design problems, define an architectural concept to fix them,
implement the foundation (Phase A), verify it, and document Phase B for the next session.

---

## 2. UX Audit — Problems Identified

Audit perspective: demanding indie game player standing 80 cm from the screen.
Two categories of problems were found.

### 2.1 Architectural Debt (root causes)

| # | Problem | Location |
|---|---------|----------|
| 1 | Static arrays `kiosk_sims[4]`, `kiosk_renders[4]`, `quad_pop[4]` — match count hardcoded to 4 | `app_state_manager.c` / `.h` |
| 2 | All HUD pixel sizes magic numbers (`26`, `14`, `5`, `90`, `240`, `6`) — do not scale | `renderer.c` |
| 3 | Both kiosk render blocks inlined in the 200-line `draw_current_state()` switch | `renderer.c` |
| 4 | Match replay triggered by `IsMouseButtonPressed()` — design principle is keyboard-only | `app_state_manager.c` |

### 2.2 UX Deficiencies (ten items)

| # | Item | Impact |
|---|------|--------|
| 1 | `"CLICK ANY MATCH TO VIEW REPLAY"` — mouse instruction, factually wrong | Misleads visitors |
| 2 | Progress bar + CTA float over content, no backing panel | Unreadable |
| 3 | Score bar 14 px — invisible from 80 cm | Unreadable |
| 4 | QR code 90×90 px, CTA text 14 px, orphaned in corner | Not inviting |
| 5 | Top bar reads `"WUSEL-MULTICAM KIOSK MODE"` — internal codename | Confusing |
| 6 | Top-3 leaderboard rows have no visual emphasis beyond text color | No drama |
| 7 | No separator line between the four quadrants | Quadrants bleed together |
| 8 | `metric_reason` (12 px) lost inside the 26 px name strip | Invisible |
| 9 | Seed thumbnails 40×40 px (5 px/cell) — invisible from standing distance | Cannot identify own match |
| 10 | Column header `"STAMINA"` — unknown term for visitors | Confusing |

---

## 3. Architectural Concept (ADR-0022)

### 3.1 Two-phase approach

**Phase A — Foundation**
- Introduce `KioskLayout` struct: single source of truth for all kiosk pixel geometry
- `compute_kiosk_layout(screen_w, screen_h, match_count)` — pure function, no side effects
- `init_kiosk_controller()` / `free_kiosk_controller()` — explicit lifecycle
- Replace all hardcoded `4`s and `50`s with `match_count` / `KIOSK_SIM_WORLD_SIZE`
- No visual change in Phase A

**Phase B — UX Enhancement** (next session)
- Extract `draw_kiosk_leaderboard()` and `draw_kiosk_multicam()` from the switch
- Implement all 10 UX fixes using `KioskLayout` values — zero new magic numbers

### 3.2 Grid layout algorithm for N matches

```
grid_cols = ceil(sqrt(match_count))
grid_rows = ceil(match_count / grid_cols)
```

Examples: 4→2×2, 6→3×2, 9→3×3, 11→4×3 (1 empty slot). Empty slot = loop ends before it, no code needed.

### 3.3 Quadrant geometry formulas

```c
quad_w = (screen_w - pad * (grid_cols + 1)) / grid_cols
quad_h = (screen_h - top_bar_h - bottom_panel_h - pad * (grid_rows - 1)) / grid_rows

// Viewport placement:
rx = pad + col * (quad_w + pad)
ry = top_bar_h + row * (quad_h + pad)
```

These produce identical geometry to the previous hardcoded 2×2 layout when `match_count=4`.

---

## 4. Phase A Implementation

### 4.1 Files changed

| File | Change |
|------|--------|
| `src/gui/renderer.h` | Added `KioskLayout` struct + `compute_kiosk_layout()` declaration |
| `src/gui/renderer.c` | Implemented `compute_kiosk_layout()`; updated 2 loop bounds `< 4` → `< match_count` |
| `src/gui/app_state_manager.h` | Added `KIOSK_SIM_WORLD_SIZE 50`, `KIOSK_DEFAULT_MATCH_COUNT 4`; added `match_count` field to `KioskController`; changed `quad_pop[4]` to `int *quad_pop`; declared `init/free_kiosk_controller()` |
| `src/gui/app_state_manager.c` | Removed static arrays; updated global init to NULL pointers; implemented `free_kiosk_controller()` and `init_kiosk_controller()`; replaced inline init block with function call; replaced all `< 4` loops and `50`/`50+2` literals with named constants |
| `src/apps/gui/main.c` | Replaced manual `for (i<4)` free loop with `free_kiosk_controller(&kiosk_ctrl)` |

### 4.2 Key design decisions made during implementation

- **`free_kiosk_controller` is only safe to call when `ctrl->initialized == true`.**
  Partial allocation failures in `init_kiosk_controller` are handled by a manual free
  path that does NOT go through `free_kiosk_controller` (avoids double-free on uninitialized arrays).

- **`compute_kiosk_layout` is defined in `renderer.c`** (rendering concern) and declared in
  `renderer.h`. It is called from `app_state_manager.c` via the existing include chain
  (`app_state_manager.h` → `renderer.h`).

- **`DrawGridAndCellsCtx` in the kiosk context:** the `config` parameter is already `(void)`d
  inside that function (ADR-0020). In Phase B, the multicam render function will pass `NULL`.

- **`KioskLayout` in Phase A:** HUD proportions (`header_h=26`, `score_bar_h=14`, `seed_cell_px=5`)
  are kept at their Phase A values to ensure zero visual change. Phase B replaces them with
  proportional formulas.

### 4.3 Remaining hardcoded values (intentional, not bugs)

- `renderer.c:465`: `init_render_context(&default_render_ctx, 50, 50, ...)` —
  This is the interactive-mode default context, not kiosk-related.
- `app_state_manager.c` seed-placement offsets `r+21`, `c+11`, `c+31` —
  Logically tied to `KIOSK_SIM_WORLD_SIZE=50` but not causing bugs; cleanup deferred.

### 4.4 Build result

```
make  →  zero warnings, zero errors
```

---

## 5. Testing Results

### 5.1 Without Docker backend

| Test | Result |
|------|--------|
| App starts, shows leaderboard (`LOADING DATA...`) | ✓ |
| After 15 s: transition to multicam (4 empty quadrants — correct, no seeds) | ✓ |
| After 30 s: return to leaderboard | ✓ |
| `[P]` → Config, `[K]` → Kiosk | ✓ |
| Clean exit, no crash | ✓ |

### 5.2 With Docker backend

Docker issue encountered: stale WSL2 bind mount path
(`/run/desktop/mnt/host/wsl/docker-desktop-bind-mounts/...` no longer existed after restart).
**Fix:** `docker compose down && docker compose up` — regenerates bind mount paths.

| Test | Result |
|------|--------|
| Leaderboard shows real player entries | ✓ |
| Multicam: simulations running with red/blue cells | ✓ |
| Player names in quadrant headers | ✓ |
| Score bar farbig befüllt | ✓ |
| 8×8 seed thumbnails visible | ✓ |

---

## 6. Documentation Created

| Document | Path |
|----------|------|
| ADR-0022 | `docs/adr/ADR-0022-kiosk-ui-architecture-and-ux-refactor.md` |
| DEV_SPEC-0022 | `docs/specs/DEV_SPEC-0022-kiosk-ui-architecture-and-ux-refactor.md` |
| DEV_TASKS-0022 | `docs/tasks/DEV_TASKS-0022-kiosk-ui-architecture-and-ux-refactor.md` |

---

## 7. Phase B — What Needs to Be Done Next

Full task plan is in `docs/tasks/DEV_TASKS-0022-kiosk-ui-architecture-and-ux-refactor.md`.
Summary of the five parts:

### Part 1 — Extract render functions (B.1) — prerequisite for all other steps
- Add forward declarations for `draw_kiosk_leaderboard()` and `draw_kiosk_multicam()` above `draw_current_state()`
- Create each function by moving the corresponding inline block verbatim
- Replace `screenWidth`/`screenHeight` → `screen_w`/`screen_h`, `kiosk_ctrl` → `ctrl->`
- Replace the inline blocks in `draw_current_state()` with the two function calls
- **Verification:** `make` zero warnings + interactive test confirming zero visual change

### Part 2 — Fix interaction model (B.2)
- Remove `IsMouseButtonPressed(MOUSE_LEFT_BUTTON)` block from `app_state_manager.c`
- Add `for (int i = 0; i < kiosk_ctrl.match_count; i++) { if (IsKeyPressed(KEY_ONE + i)) ... }` — uses `KEY_ONE + i` (Raylib digit keys are sequential)
- Replace CTA text: `"CLICK ANY MATCH..."` → `"[1-4] WATCH MATCH  |  [P] PLAY"`

### Part 3 — Leaderboard UX (B.3–B.6)
- B.3: Add `THEME_HUD` footer panel rect; update bar/CTA y-positions to use `layout.bottom_panel_h`
- B.4: Replace 90 px QR corner orphan with a `screen_w/4` wide bordered panel; QR size = `panel_w * 0.55f` (min 140 px); font ≥ 20 px
- B.5: Derive `rowHeight` proportionally; add semi-transparent background rect for top-3 rows
- B.6: Replace `"STAMINA"` with `"ENDURANCE"`

### Part 4 — Multicam UX (B.7–B.11)
- B.7: `score_bar_h = quad_h / 18` (min 20); font = `score_bar_h * 0.72f` (min 14)
- B.8: Replace `"WUSEL-MULTICAM KIOSK MODE"` → `"LIVE BATTLES"`
- B.9: Add cross-line separator — derive positions from `ctrl->renders[i].viewport_bounds`; use `layout.separator_px = 2`
- B.10: `badge_h = header_h * 0.85f` (min 16); `header_h += badge_h`; move `metric_reason` to centred badge line below names
- B.11: `seed_cell_px = quad_h / 30` (min 7); replace `int thumb_cell = 5` with `int thumb_cell = layout.seed_cell_px`
- Also: add footer panel to multicam (same pattern as leaderboard, B.3)

### Part 5 — Final regression test (B.12)
- All 10 UX items verified against the checklist in DEV_TASKS-0022
- Navigation: `[1]`–`[4]`, `[P]`, `[K]`, `[Q]` all tested
- Clean exit confirmed
- CHANGELOG updated

---

## 8. Key Invariants to Maintain

- **Zero warnings rule:** `gcc -Wall -Wextra -std=c99` must produce zero warnings after every `.c` change.
- **No magic numbers:** Every pixel value in kiosk rendering code must come from a `KioskLayout` field.
- **`KI-Agent unterstützt` comment:** Every new or significantly modified function must carry this comment.
- **`free_kiosk_controller` safety:** Only call when `ctrl->initialized == true`. The `initialized` flag is the guard.
- **`KioskLayout` is stateless:** `compute_kiosk_layout()` has no side effects. Call it at the top of render functions; do not cache it between frames.
