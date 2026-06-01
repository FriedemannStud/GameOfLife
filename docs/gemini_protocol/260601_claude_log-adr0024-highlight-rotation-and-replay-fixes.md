# Development Log: ADR-0024 — Round-Robin Highlight Rotation & Replay Fixes

**Date:** 2026-06-01
**Branch:** biotop (kiosk feature work)
**Agent:** Claude Sonnet 4.6 (Claude Code)
**Related Documents:** ADR-0024, DEV_TASKS-0024, ADR-0023, ADR-0022, ADR-0020

---

## 1. Session Goal

Implement client-side round-robin highlight rotation for Kiosk Mode as specified in ADR-0024.
The kiosk previously showed the same 4 highlight matches on every display cycle. The goal was
to cycle through up to 10 highlights per epoch so a spectator sees different content each rotation.

---

## 2. Phase 0: Global Search — Undocumented Files Found

**Action:** Ran the full Phase 0 grep search as specified in DEV_TASKS-0024.

**Finding:** `renderer.c` contained three additional `cached_highlights.matches[i]` references
not listed in DEV_TASKS-0024:

| File | Line | What |
|------|------|------|
| `src/gui/renderer.c` | ~1433 | `metric_reason` badge text |
| `src/gui/renderer.c` | ~1455 | Blue seed thumbnail |
| `src/gui/renderer.c` | ~1465 | Red seed thumbnail |

**Decision:** These were added as a bonus fix (Phase 4.5) to ensure thumbnails and the badge
display the correct pool-slot match after rotation — not just the first 4 matches.

---

## 3. Phases 1–4: Core ADR-0024 Implementation

### 3.1 Network Layer Expansion (Phase 1)

- `src/io/network_io.h:37`: `MatchHighlight matches[4]` → `matches[10]`
- `src/io/network_io.c:147`: parse loop bound `i < 4` → `i < 10`

### 3.2 KioskController Pool State (Phase 2)

Added two new fields to `KioskController` in `app_state_manager.h`:

```c
int highlight_pool_index;   // first match index of the current display window
int highlight_pool_size;    // total matches in cached_highlights
```

Initialised to `0` in `init_kiosk_controller()`.

### 3.3 Helper Function `load_kiosk_sims_from_pool()` (Phase 3)

Extracted the inline sim-loading block into a `static void` function in `app_state_manager.c`.
Key logic:

```c
int slot = (kiosk_ctrl.highlight_pool_index + i) % kiosk_ctrl.highlight_pool_size;
MatchHighlight *m = &kiosk_ctrl.cached_highlights.matches[slot];
```

Guard: returns immediately if `highlight_pool_size == 0`.

### 3.4 Rotation Wiring (Phase 4)

- **MULTICAM → LEADERBOARD:** `pool_index` advances by `match_count` (modulo `pool_size`).
- **LEADERBOARD → MULTICAM:** `load_kiosk_sims_from_pool()` loads the next window.
- **KEY_1..KEY_4 handler:** `slot` variable introduced; `matches[slot]` used instead of `matches[i]`.
- **`renderer.c` bonus fix:** `render_slot` variable introduced in the per-quadrant HUD loop;
  `metric_reason` and seed thumbnails read from `matches[render_slot]`.

---

## 4. Bug 1: Rotation Not Working (Root Cause & Fix)

### Symptom
Second Multicam cycle showed identical matches as cycle 1.

### Root Cause
In `process_network_events()` (inside `update_app_state`), the handler for
`network_get_highlights()` always reset `pool_index = 0` on every response.
`network_fetch_highlights_async()` is called at each LB→MULTICAM transition; the async
response arrives during the 30-second MULTICAM window and silently reset the index to 0,
undoing the rotation advance.

### Fix
Changed the handler to distinguish first load from epoch refresh:

```c
bool first_load = (kiosk_ctrl.highlight_pool_size == 0);
kiosk_ctrl.cached_highlights   = hd;
kiosk_ctrl.highlight_pool_size = hd.count;
if (first_load) {
    kiosk_ctrl.highlight_pool_index = 0;
    if (current_state == STATE_KIOSK_MODE) load_kiosk_sims_from_pool();
}
// on refresh: pool_index is intentionally kept — rotation continues uninterrupted
```

**Result:** Bug 1 confirmed fixed. Cycle 2 showed a different set of matches.

---

## 5. Bug 2: No Player Names in KEY_1 Full-Screen Replay

### Symptom
Pressing KEY_1 during Multicam launched a full-screen replay but the header showed
"SIMULATION ACTIVE" — no player names visible.

### Root Cause (two parts)
1. The KEY_1..4 handler never copied participant names from
   `cached_highlights.matches[slot]` to `sim_ctx->participant_red/blue`.
2. `draw_current_state()` did not receive `sim_ctx`, so the renderer had no access to names.

### Fix
**`app_state_manager.c` — KEY handler:** Added name copy before `reset_simulation_context`:

```c
strncpy(sim_ctx->participant_red,
        kiosk_ctrl.cached_highlights.matches[slot].participant_red, ...);
strncpy(sim_ctx->participant_blue, ...);
```

**`renderer.h` / `renderer.c`:** Added `const SimulationContext *sim_ctx` parameter to
`draw_current_state()`. In the `STATE_RUNNING` case, replaced the generic title with names:

```c
if (sim_ctx && sim_ctx->participant_red[0] != '\0') {
    DrawText(sim_ctx->participant_red, 20, 18, 24, THEME_RED);
    DrawText(" vs ", ...);
    DrawText(sim_ctx->participant_blue, ...);
} else {
    DrawText("SIMULATION ACTIVE", 20, 18, 24, THEME_RED);
}
```

**`main.c`:** Call site updated to pass `&global_sim`.

---

## 6. Bug 3: Replay Running on 50×50 Grid Instead of 8×16

### Symptom
The full-screen replay launched by KEY_1 used `config->rows = 50`, `config->cols = 50`
(the main game dimensions). The tournament match was played on 8×16. Different grid size
means different dynamics — the replay did not show the same match.

### Fix Overview
Three-part change across four files:

**1. `src/core/config.h`:** Added constants:
```c
#define DEFAULT_GRID_ROWS 50
#define DEFAULT_GRID_COLS 50
```

**2. `src/gui/app_state_manager.h`:** Declared `restore_main_game_context(config, r_ctx)`.
Extended `update_app_state` signature to include `RenderContext *r_ctx`.

**3. `src/gui/app_state_manager.c` — KEY handler:**
```c
config->rows = KIOSK_SIM_ROWS;   // 8
config->cols = KIOSK_SIM_COLS;   // 16
free_render_context(r_ctx);
init_render_context(r_ctx, KIOSK_SIM_COLS, KIOSK_SIM_ROWS, kiosk_vp);
reset_simulation_context(sim_ctx, KIOSK_SIM_ROWS, KIOSK_SIM_COLS);
// centering math: center_r=0, center_c_b=0, center_c_r=8 — identical to kiosk layout
```

`restore_main_game_context()` added as a helper that restores 50×50 only if config was
modified (guard: `config->rows != DEFAULT_GRID_ROWS`).

**4. `src/gui/renderer.c`:** `restore_main_game_context(config, r_ctx)` called before
`cleanup_interactive_session` at all four kiosk-replay exit paths:
- STATE_RUNNING KEY_Q/BACKSPACE (ORIGIN_KIOSK_REPLAY branch)
- STATE_RUNNING KEY_K
- STATE_OBSERVER KEY_K
- STATE_FINISHED KEY_Q (ORIGIN_KIOSK_REPLAY branch)

`update_global_input` timeout also calls `restore_main_game_context` before cleanup.

---

## 7. Files Changed

| File | Change |
|------|--------|
| `src/io/network_io.h` | `matches[4]` → `matches[10]` |
| `src/io/network_io.c` | parse loop cap 4 → 10 |
| `src/core/config.h` | `DEFAULT_GRID_ROWS`, `DEFAULT_GRID_COLS` added |
| `src/gui/app_state_manager.h` | new pool fields, `restore_main_game_context`, `update_app_state` signature |
| `src/gui/app_state_manager.c` | `load_kiosk_sims_from_pool()`, rotation wiring, pool-reset fix, KEY handler, restore helper |
| `src/gui/renderer.h` | `draw_current_state` signature extended with `sim_ctx` |
| `src/gui/renderer.c` | `render_slot` formula, player name display, restore calls at exit paths |
| `src/apps/gui/main.c` | updated `update_app_state` and `draw_current_state` call sites |
| `docs/adr/ADR-0024-*.md` | Status: Proposed → Implemented |
| `docs/tasks/DEV_TASKS-0024-*.md` | All checkboxes → `[x]` |
| `docs/CHANGELOG.md` | Entry added for 2026-06-01 |

---

## 8. Verification Results

| Test | Result |
|------|--------|
| `make` — zero warnings | PASS |
| Kiosk Mode starts, Leaderboard + Multicam visible | PASS |
| Cycle 2 shows different matches than Cycle 1 | PASS |
| KEY_1 launches replay, names visible in header | PASS |
| Replay runs on 8×16 grid (correct tournament arena) | PASS |
| KEY_Q returns to Kiosk, Kiosk continues normally | PASS |

---

## 9. Open Items / Known Limitations

- `highlight_pool_index` resets to 0 only on first load, not on new-epoch boundaries.
  Within a demo session this is acceptable. A future improvement could detect epoch changes
  via an epoch ID in the API response and reset the index when the epoch changes.
- The 10-match cap (`matches[10]`) in `network_io.h` is a new magic number. If the backend
  ever returns more than 10 highlights, the C struct must be updated.
- Confirmed working highlights require at least one completed tournament epoch in the backend.
  Run `docker-compose up -d` and wait for the epoch worker to complete before entering
  Kiosk Mode.
