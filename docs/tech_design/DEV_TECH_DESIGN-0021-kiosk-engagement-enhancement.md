# Technical Design: Kiosk Mode Engagement Enhancement

**Version:** 1.0
**Date:** 2026-05-31
**Author:** Claude (KI-Agent)
**Related Documents:** [ADR-0021](../adr/ADR-0021-kiosk-engagement-enhancement.md), [DEV_SPEC-0021](../specs/DEV_SPEC-0021-kiosk-engagement-enhancement.md)

---

### 1. Introduction

This document describes the concrete changes required to implement the six kiosk engagement enhancements (P1–P6). All changes are confined to three files:

- `src/gui/app_state_manager.h` — struct extension
- `src/gui/app_state_manager.c` — data plumbing fixes
- `src/gui/renderer.c` — rendering additions in `draw_current_state`

No changes to `core_types.h`, `network_io.h`, game logic, or the backend are required.

---

### 2. System Architecture and Components

#### 2.1. Data Flow (Current vs. Target)

**Current (broken) flow for participant names:**
```
network_io → HighlightData.matches[i].participant_red   ←  never copied
                                                         ↓
                                               kiosk_ctrl.sims[i].participant_red = ""  (empty)
                                                         ↓
                                               renderer → draws nothing
```

**Target flow:**
```
network_io → HighlightData.matches[i].participant_red
                      ↓  (copied in update_app_state when highlight arrives)
             kiosk_ctrl.sims[i].participant_red = "Eva"
                      ↓
             renderer → draws "EVA" in THEME_RED above quadrant i
```

**Current (wasted) population data:**
```
update_generation_ctx(&kiosk_ctrl.sims[i], &dummy_red, &dummy_blue)
                                                ↑ discarded
```

**Target:**
```
update_generation_ctx(&kiosk_ctrl.sims[i], &kiosk_ctrl.quad_red_pop[i], &kiosk_ctrl.quad_blue_pop[i])
                                                ↑ stored, read by renderer
```

---

### 3. Data Model Specification

#### 3.1. `KioskController` struct extension (`app_state_manager.h`)

Add two arrays to persist per-quadrant live population counts:

```c
typedef struct {
    KioskSubState current_sub_state;
    float state_timer;

    RenderContext* renders;
    SimulationContext* sims;
    bool initialized;
    LeaderboardData cached_lb;
    HighlightData cached_highlights;

    // KI-Agent unterstützt: Per-quadrant live population for score bar (ADR-0021)
    int quad_red_pop[4];
    int quad_blue_pop[4];
} KioskController;
```

No other struct changes are needed. `SimulationContext.participant_red/blue` fields (already present, 64 chars each) are used for name storage.

---

### 4. Backend / Logic Specification (`app_state_manager.c`)

#### 4.1. Name forwarding fix (P1 data plumbing)

**Location:** `update_app_state`, inside the `if (network_get_highlights(&hd))` block, after the existing grid-seeding loop.

**Change:** After populating the grid for quadrant `i`, also copy the participant names:

```c
// KI-Agent unterstützt: Forward participant names for kiosk HUD (ADR-0021)
strncpy(kiosk_ctrl.sims[i].participant_red, hd.matches[i].participant_red,
        sizeof(kiosk_ctrl.sims[i].participant_red) - 1);
kiosk_ctrl.sims[i].participant_red[sizeof(kiosk_ctrl.sims[i].participant_red) - 1] = '\0';
strncpy(kiosk_ctrl.sims[i].participant_blue, hd.matches[i].participant_blue,
        sizeof(kiosk_ctrl.sims[i].participant_blue) - 1);
kiosk_ctrl.sims[i].participant_blue[sizeof(kiosk_ctrl.sims[i].participant_blue) - 1] = '\0';
```

#### 4.2. Per-quadrant population tracking (P2 data plumbing)

**Location:** `update_app_state`, `KIOSK_SUB_MULTICAM` simulation tick (currently uses `dummy_red`, `dummy_blue`).

**Change:** Replace the local dummy variables with the persistent arrays:

```c
// Before (discards data):
int dummy_red, dummy_blue;
for (int i = 0; i < 4; i++) {
    update_generation_ctx(&kiosk_ctrl.sims[i], &dummy_red, &dummy_blue);
}

// After (persists data):
// KI-Agent unterstützt: Store per-quadrant pop for score bar (ADR-0021)
for (int i = 0; i < 4; i++) {
    update_generation_ctx(&kiosk_ctrl.sims[i],
                          &kiosk_ctrl.quad_red_pop[i],
                          &kiosk_ctrl.quad_blue_pop[i]);
}
```

---

### 5. Renderer Specification (`renderer.c`)

All rendering additions are inside `draw_current_state`, `case STATE_KIOSK_MODE`, `KIOSK_SUB_MULTICAM` branch.

#### 5.1. Layout constants

```
Quadrant viewport: (rx, ry, q_w, q_h)
Name header bar:   y = ry,            height = 28 px  (drawn OVER the simulation)
Score bar:         y = ry + q_h - 18, height = 14 px
Seed thumbnail:    x = rx + 4, y = ry + 30, size = 8×6 px per cell = 48×48 px total
```

#### 5.2. P1 — Player name header

For each quadrant `i`, after `DrawGridAndCellsCtx`:

```c
// Semi-transparent header strip
Rectangle header = { rx, ry, (float)q_w, 28.0f };
DrawRectangleRec(header, Fade(THEME_HUD, 0.85f));

// "REDNAME vs BLUENAME"  — names uppercased by convention (all HUD text is caps)
DrawText(kiosk_ctrl.sims[i].participant_red, rx + 6, ry + 5, 16, THEME_RED);
int vs_x = rx + 6 + MeasureText(kiosk_ctrl.sims[i].participant_red, 16) + 6;
DrawText("vs", vs_x, ry + 7, 14, THEME_HINT);
int blue_x = vs_x + MeasureText("vs", 14) + 6;
DrawText(kiosk_ctrl.sims[i].participant_blue, blue_x, ry + 5, 16, THEME_BLUE);
```

#### 5.3. P3 — Metric reason label

Appended to the header strip, right-aligned:

```c
const char* reason = kiosk_ctrl.cached_highlights.matches[i].metric_reason;
if (strlen(reason) > 0) {
    int rw = MeasureText(reason, 13);
    DrawText(reason, rx + q_w - rw - 6, ry + 8, 13, THEME_ACCENT);
}
```

#### 5.4. P2 — Live score bar

```c
Rectangle score_bg = { rx, ry + q_h - 14, (float)q_w, 14.0f };
DrawRectangleRec(score_bg, Fade(THEME_HUD, 0.85f));

int total = kiosk_ctrl.quad_red_pop[i] + kiosk_ctrl.quad_blue_pop[i];
if (total > 0) {
    float red_frac = (float)kiosk_ctrl.quad_red_pop[i] / total;
    DrawRectangle(rx, ry + q_h - 14, (int)(q_w * red_frac), 14, Fade(THEME_RED, 0.8f));
    DrawRectangle(rx + (int)(q_w * red_frac), ry + q_h - 14,
                  q_w - (int)(q_w * red_frac), 14, Fade(THEME_BLUE, 0.8f));
}
// Numeric labels
char score_buf[32];
sprintf(score_buf, "%d", kiosk_ctrl.quad_red_pop[i]);
DrawText(score_buf, rx + 4, ry + q_h - 13, 11, WHITE);
sprintf(score_buf, "%d", kiosk_ctrl.quad_blue_pop[i]);
int sw = MeasureText(score_buf, 11);
DrawText(score_buf, rx + q_w - sw - 4, ry + q_h - 13, 11, WHITE);
```

#### 5.5. P4 — 8×8 seed thumbnail

```c
int thumb_cell = 6; // px per seed cell
int thumb_x = rx + 4;
int thumb_y = ry + 32; // below name header
DrawRectangle(thumb_x - 1, thumb_y - 1, 8 * thumb_cell + 2, 8 * thumb_cell + 2,
              Fade(BLACK, 0.6f));
for (int tr = 0; tr < 8; tr++) {
    for (int tc = 0; tc < 8; tc++) {
        int idx = tr * 8 + tc;
        if (kiosk_ctrl.cached_highlights.matches[i].seed_red[idx]) {
            DrawRectangle(thumb_x + tc * thumb_cell, thumb_y + tr * thumb_cell,
                          thumb_cell - 1, thumb_cell - 1, Fade(THEME_RED, 0.9f));
        } else if (kiosk_ctrl.cached_highlights.matches[i].seed_blue[idx]) {
            DrawRectangle(thumb_x + tc * thumb_cell, thumb_y + tr * thumb_cell,
                          thumb_cell - 1, thumb_cell - 1, Fade(THEME_BLUE, 0.9f));
        }
    }
}
```

#### 5.6. P5 — QR code + CTA on leaderboard

In `KIOSK_SUB_LEADERBOARD`, add a right-side panel (mirroring the existing layout in `STATE_PUZZLE`):

```
x = screenWidth - 200, y = screenHeight/2 - 80
QR placeholder: 100×100 px (reuse the pattern from STATE_PUZZLE lines 886–892)
CTA text: "Submit YOUR strategy!"
URL text:  "biotope.uni-xyz.de/editor"
```

#### 5.7. P6 — Progress bar replacing text timer

Replace `DrawText("SWITCHING IN %.0f SECONDS", ...)` with:

```c
float progress = kiosk_ctrl.state_timer /
    (kiosk_ctrl.current_sub_state == KIOSK_SUB_LEADERBOARD ? 15.0f : 30.0f);
int bar_w = 200;
int bar_x = screenWidth / 2 - bar_w / 2;
int bar_y = screenHeight - 20;
DrawRectangle(bar_x, bar_y, bar_w, 6, Fade(THEME_HINT, 0.3f));
DrawRectangle(bar_x, bar_y, (int)(bar_w * progress), 6, THEME_ACCENT);
```

---

### 6. Security Considerations

No network, file I/O, or external input is involved in the rendering changes. Player names from the API are already bounded by `MAX_NAME_LENGTH` (32) in `LeaderboardEntry` / `MatchHighlight` and are copied with explicit `strncpy` + null termination.

---

### 7. Performance Considerations

- All new draw calls are O(1) per quadrant (constant number of `DrawRectangle` / `DrawText` calls).
- The thumbnail loop is 8×8 = 64 iterations — negligible at 120 FPS.
- `quad_red_pop[4]` / `quad_blue_pop[4]` are `int` arrays on the stack; zero allocation overhead.
- No changes to the hot simulation loop (`update_generation_ctx`) are made; only the output destination changes from discarded local variables to persistent struct fields.
