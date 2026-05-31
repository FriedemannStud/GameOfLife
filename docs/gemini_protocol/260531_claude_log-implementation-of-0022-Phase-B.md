# Development Log — ADR-0022 Kiosk UI Architecture Refactor (Phase B)

**Date:** 2026-05-31
**Branch:** `biotop`
**AI Agent:** Claude Sonnet 4.6
**Status at end of session:** Phase B complete and fully tested. Additional improvements beyond original spec implemented and verified.

---

## 1. Session Goal

Implement all Phase B steps defined in `docs/tasks/DEV_TASKS-0022-kiosk-ui-architecture-and-ux-refactor.md`, building on the Phase A foundation (dynamic `KioskLayout`, `init/free_kiosk_controller`, named constants) completed in the previous session.

---

## 2. Pre-flight

- Clean build confirmed: `make clean && make` → zero warnings, zero errors.
- App starts and closes without crash.
- Docker backend started with `docker compose up backend matchmaker`.

**Docker volume issue (known, documented):** After `make clean`, running containers lose the bind mount to `./build` because `rm -rf build` destroys the inode. Fix: always follow `down → make → up → run` workflow (see Section 9).

---

## 3. Implementation Steps

### Step B.1 — Extract Render Functions (`renderer.c`)

**Goal:** Move both kiosk render blocks out of the monolithic `draw_current_state()` switch.

**Changes:**
- Added two forward declarations directly above `draw_current_state()`:
  ```c
  static void draw_kiosk_leaderboard(const KioskController *ctrl, int screen_w, int screen_h);
  static void draw_kiosk_multicam(KioskController *ctrl, int screen_w, int screen_h);
  ```
- Replaced the entire `case STATE_KIOSK_MODE:` inline block with two function calls:
  ```c
  case STATE_KIOSK_MODE:
      if (kiosk_ctrl.current_sub_state == KIOSK_SUB_LEADERBOARD)
          draw_kiosk_leaderboard(&kiosk_ctrl, screenWidth, screenHeight);
      else
          draw_kiosk_multicam(&kiosk_ctrl, screenWidth, screenHeight);
      break;
  ```
- Created `draw_kiosk_leaderboard()` and `draw_kiosk_multicam()` at the end of `renderer.c`.
- Inside both functions: `screenWidth` → `screen_w`, `screenHeight` → `screen_h`, `kiosk_ctrl` → `ctrl->`.
- `DrawGridAndCellsCtx` second argument changed from `config` to `NULL` (config is `(void)`d inside, per ADR-0020).
- Note on const: `draw_kiosk_multicam` uses `KioskController *ctrl` (non-const) because `DrawGridAndCellsCtx` mutates `RenderContext` GPU resources.

**Result:** Zero visual change confirmed interactively.

---

### Step B.2 — Fix Interaction Model (`app_state_manager.c`)

**Goal:** Replace mouse-click replay trigger with keyboard keys `[1]`–`[4]`.

**Changes:**
- Removed entire `if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))` block.
- Added keyboard loop:
  ```c
  for (int i = 0; i < kiosk_ctrl.match_count; i++) {
      if (IsKeyPressed(KEY_ONE + i)) { ... return STATE_IGNITION; }
  }
  ```
  `KEY_ONE + i` is valid because Raylib digit key constants are sequential.
- Updated CTA text in `draw_kiosk_multicam()`:
  `"CLICK ANY MATCH TO VIEW REPLAY"` → `"[1-4] WATCH MATCH  |  [P] PLAY"`

**Bug discovered and fixed (B.2 extension):**
The original code (and the initial keyboard replacement) read from `kiosk_ctrl.sims[i].current_world` — the live, mid-game state — instead of the original seed. Fix: read from `cached_highlights.matches[i].seed_blue/red` (the 8×8 start pattern). This also simplifies the code: magic grid offsets `+21, +11, +31` replaced by flat array access `seed_blue[r * 8 + c]`.

---

### Steps B.3–B.6 — Leaderboard UX (`draw_kiosk_leaderboard`)

All changes are inside `draw_kiosk_leaderboard()`. Each calls `compute_kiosk_layout()` at the top for all pixel values.

**B.3 — Footer backing panel:**
```c
int footer_y = screen_h - layout.bottom_panel_h;
DrawRectangle(0, footer_y, screen_w, layout.bottom_panel_h, THEME_HUD);
```
Progress bar and CTA text repositioned to `footer_y` baseline.

**B.4 — QR panel (later removed, see Section 4):**
Initial implementation: proportional panel (screen_w/4 wide). Subsequently removed entirely at user request (analog QR code decided instead).

**B.5 — Top-3 row highlights + proportional row height:**
```c
int rowHeight = entries_area_h / 10;
if (rowHeight < 28) rowHeight = 28;
```
Rows 0–2 receive a semi-transparent background (red/blue/accent tint).

**B.6 — Column rename:**
`"STAMINA"` → `"ENDURANCE"`

---

### Steps B.7–B.11 — Multicam UX (`compute_kiosk_layout` + `draw_kiosk_multicam`)

**B.7 — Proportional score bar** (in `compute_kiosk_layout`):
```c
l.score_bar_h = l.quad_h / 18;
if (l.score_bar_h < 20) l.score_bar_h = 20;
```
Score font derived from bar height: `score_font = (int)(layout.score_bar_h * 0.72f)`, min 14.

**B.8 — Screen title:**
`"WUSEL-MULTICAM KIOSK MODE"` → `"LIVE BATTLES"`

**B.9 — Quadrant separator cross-lines:**
Positions derived from `ctrl->renders[i].viewport_bounds` — not hardcoded. Vertical separators between columns, horizontal separators between rows.

**B.10 — `metric_reason` badge:**
Header split into two rows: name strip (26 px) + badge strip (`badge_h`).
```c
l.badge_h = (int)(26 * 0.85f);  // = 22 px
if (l.badge_h < 16) l.badge_h = 16;
l.header_h = 26 + l.badge_h;    // = 48 px combined
```
`metric_reason` rendered centred in badge strip; removed from right-aligned name strip position.

**B.11 — Larger seed thumbnails:**
```c
l.seed_cell_px = l.quad_h / 40;  // ~20% of quad height
if (l.seed_cell_px < 7) l.seed_cell_px = 7;
```
Divisor changed from 30 (≈26%) to 40 (≈20%) after visual feedback.
`thumb_y` updated to `ry + layout.header_h + 4` to clear the new two-row header.

**B.11 (also) — Multicam footer panel:**
Same pattern as leaderboard: `DrawRectangle(0, footer_y, screen_w, layout.bottom_panel_h, THEME_HUD)`.

---

## 4. Improvements Beyond Original Spec

### 4.1 Viewport Scaling on Window Resize

**Problem:** `init_kiosk_controller()` sets `viewport_bounds` once at startup. Resizing the window or entering fullscreen left quadrants at their original size.

**Root cause:** `DrawGridAndCellsCtx` already auto-reallocates GPU resources when `viewport_bounds` changes (resize check at line 206). The only missing piece: updating `viewport_bounds` each frame.

**Fix:** Added a viewport update loop at the top of `draw_kiosk_multicam()`, before rendering:
```c
for (int i = 0; i < ctrl->match_count; i++) {
    int col = i % layout.grid_cols;
    int row = i / layout.grid_cols;
    ctrl->renders[i].viewport_bounds = (Rectangle){
        (float)(layout.pad + col * (layout.quad_w + layout.pad)),
        (float)(layout.top_bar_h + row * (layout.quad_h + layout.pad)),
        (float)layout.quad_w,
        (float)layout.quad_h
    };
}
```
GPU resources are only reallocated when size actually changes — not every frame.
Motion-trail effect briefly resets on resize (acceptable, confirmed by user).

### 4.2 QR Code Removed

User decision: QR code will be provided as a physical printout. The entire B.4 panel block was removed from `draw_kiosk_leaderboard()`.

### 4.3 Clip-to-Fit Leaderboard List

**Problem:** With many players, the list grew past the footer panel, covering `PRESS [P] TO PLAY` and the progress bar.

**Fix:**
```c
int max_fit      = entries_area_h / rowHeight;
int show_count   = total_entries;
int hidden_count = 0;
if (total_entries > max_fit) {
    show_count   = max_fit - 1;
    hidden_count = total_entries - show_count;
}
```
If entries are clipped, a `+ N more` line is shown at the bottom of the list in `THEME_HINT` colour.

### 4.4 Proportional Column Layout (Leaderboard)

**Problem:** All column X-positions were hardcoded relative to `screen_w / 2`. Total table width was fixed at 610 px regardless of screen size. With QR panel removed, the right half of the screen was wasted.

**Fix:** Table spans 80 % of screen width with 10 % margins on each side:
```c
int table_x       = (int)(screen_w * 0.10f);
int table_w       = (int)(screen_w * 0.80f);
int col_rank      = table_x;
int col_player    = table_x + (int)(table_w * 0.08f);
int col_winrate   = table_x + (int)(table_w * 0.42f);
int col_wdl       = table_x + (int)(table_w * 0.58f);
int col_endurance = table_x + (int)(table_w * 0.80f);
```
Column proportions: RANK 8%, PLAYER 34%, WIN RATE 16%, W/D/L 22%, ENDURANCE 20%.

---

## 5. Boy Scout Cleanup

After removing the QR panel, three `KioskLayout` struct fields became unused and were removed:

| Field | Reason for removal |
|---|---|
| `font_cta` | Only used in QR panel block |
| `font_name` | Name drawing used hardcoded `16`, not this field |
| `font_score` | Score font derived locally from `score_bar_h * 0.72f` |

Removed from `KioskLayout` struct in `renderer.h` and from `compute_kiosk_layout()` in `renderer.c`.

---

## 6. Files Changed

| File | Changes |
|---|---|
| `src/gui/renderer.c` | `compute_kiosk_layout()` updated; forward declarations added; `case STATE_KIOSK_MODE` replaced; `draw_kiosk_leaderboard()` and `draw_kiosk_multicam()` added at end |
| `src/gui/renderer.h` | `KioskLayout` struct: 3 unused fields removed |
| `src/gui/app_state_manager.c` | Mouse handler removed; keyboard `[1]–[4]` handler added; replay seed source fixed |
| `docs/CHANGELOG.md` | ADR-0022 Phase B entry added |
| `docs/tasks/DEV_TASKS-0022-...md` | All Phase B checkboxes marked `[x]` |

---

## 7. Build & Test Results

- `make` → zero warnings, zero errors (verified after every change).
- All 16 interactive test points confirmed ✓ (leaderboard UX, multicam UX, keyboard navigation, clean exit).
- Viewport scaling confirmed: window resize and fullscreen both scale quadrants correctly.
- Leaderboard clip-to-fit confirmed: footer always visible.
- Proportional column layout confirmed: spacing correct at tested resolution.

---

## 8. Key Invariants to Maintain

- **Zero warnings rule:** `gcc -Wall -Wextra -std=c99` must produce zero warnings after every `.c` change.
- **No magic numbers:** All pixel values in kiosk rendering come from `KioskLayout` fields.
- **`KI-Agent unterstützt` comment:** Every new or significantly modified function carries this comment.
- **`free_kiosk_controller` safety:** Only call when `ctrl->initialized == true`.
- **`KioskLayout` is stateless:** `compute_kiosk_layout()` has no side effects. Called at the top of both render functions each frame; not cached between frames.
- **Viewport update in `draw_kiosk_multicam`:** Must remain before the `DrawGridAndCellsCtx` loop so GPU resources resize correctly.
- **Replay seed source:** Always read from `cached_highlights.matches[i].seed_blue/red`, never from `sims[i].current_world`.

---

## 9. Stable App Start Workflow

Two terminals required. Always follow this order:

**Terminal A (WSL2 Host):**
```bash
docker compose down
```

**Terminal B (c-dev container):**
```bash
make clean && make
```

**Terminal A:**
```bash
docker compose up backend matchmaker
# Wait for: Application startup complete.
```

**Terminal B:**
```bash
./build/biotope
```

**Rule:** Build before starting containers. `make clean` deletes `build/`, which breaks Docker volume mounts on running containers. Always stop containers first.
