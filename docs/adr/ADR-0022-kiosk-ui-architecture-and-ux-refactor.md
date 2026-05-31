### **ADR-0022: Kiosk UI Architecture Refactor & UX Enhancement**

**Status:** Phase A: Implemented & Tested. Phase B: Accepted.

**Date:** 2026-05-31

---

#### **1. Context and Problem Statement**

The Kiosk Mode (`STATE_KIOSK_MODE`) is the primary public-facing screen at the university fair. A combined code review and UX audit (perspective: demanding indie game player) identified two categories of problems:

**Category 1 — Architectural Debt (four root causes)**

1. **Static 4-element arrays:** `kiosk_sims[4]` and `kiosk_renders[4]` in `app_state_manager.c`, plus `quad_red_pop[4]` and `quad_blue_pop[4]` in `KioskController`, hardcoded the match count to exactly 4. Adding a fifth match or adapting to a larger monitor required touching at least 6 locations.

2. **Magic-number pixel sizes:** Every HUD dimension (`26`, `14`, `5`, `90`, `240`, `6`) in the kiosk renderer was an absolute pixel value unrelated to the actual screen or quadrant size. On a large monitor the HUD elements would appear tiny; on a small monitor they would overlap.

3. **Monolithic render switch:** Both kiosk views (leaderboard and multicam) were implemented as inline blocks inside the 200-line `switch` in `draw_current_state()`. The code was untestable and impenetrable.

4. **Incorrect interaction model:** `app_state_manager.c` used `IsMouseButtonPressed(MOUSE_LEFT_BUTTON)` to trigger match replays, while the stated design principle for kiosk mode is keyboard-only navigation.

**Category 2 — UX Deficiencies (ten specific issues)**

| # | Issue | Location |
|---|-------|----------|
| 1 | "CLICK ANY MATCH TO VIEW REPLAY" — mouse, wrong | `renderer.c` |
| 2 | Progress bar + CTA float over content, no backing panel | `renderer.c` |
| 3 | Score bar 14 px — unreadable from 1 m distance | `renderer.c` |
| 4 | QR code 90×90 px orphaned in corner, CTA text 14 px | `renderer.c` |
| 5 | Top bar label "WUSEL-MULTICAM KIOSK MODE" — internal codename | `renderer.c` |
| 6 | Leaderboard top-3 rows have no visual emphasis beyond color | `renderer.c` |
| 7 | No visible separator between the four quadrants | `renderer.c` |
| 8 | `metric_reason` badge (12 px) lost inside the name strip | `renderer.c` |
| 9 | Seed thumbnails 40×40 px — invisible from standing distance | `renderer.c` |
| 10 | Column header "STAMINA" — unknown term for visitors | `renderer.c` |

---

#### **2. Decision**

Implement a two-phase refactor:

**Phase A — Foundation (implemented 2026-05-31)**

Introduce a `KioskLayout` struct as the single source of truth for all kiosk pixel geometry. Extract `init_kiosk_controller()` and `free_kiosk_controller()` to give the kiosk an explicit lifecycle. Replace all static arrays and hardcoded `4`s with dynamic allocation driven by `match_count`.

| Change | File |
|--------|------|
| Add `KioskLayout` struct + `compute_kiosk_layout()` | `renderer.h`, `renderer.c` |
| Add `KIOSK_SIM_WORLD_SIZE`, `KIOSK_DEFAULT_MATCH_COUNT` | `app_state_manager.h` |
| Add `match_count` field, convert `quad_pop[4]` to dynamic `*` | `app_state_manager.h` |
| Add `init_kiosk_controller()` / `free_kiosk_controller()` | `app_state_manager.c` |
| Replace inline init block with `init_kiosk_controller()` call | `app_state_manager.c` |
| Replace all `< 4` loop bounds with `< match_count` | `app_state_manager.c`, `renderer.c` |
| Replace manual cleanup loop with `free_kiosk_controller()` | `main.c` |

**Phase B — UX Enhancement (to be implemented)**

Extract the two kiosk render blocks into dedicated private functions `draw_kiosk_leaderboard()` and `draw_kiosk_multicam()`. Implement all 10 UX fixes using `KioskLayout` values — no new magic numbers. Replace the mouse replay trigger with keyboard navigation.

---

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

- Match count is controlled by a single constant (`KIOSK_DEFAULT_MATCH_COUNT`). Changing to 9 matches on a large monitor requires one value change, zero other code touches.
- All HUD proportions scale automatically to any screen resolution via `compute_kiosk_layout()`.
- `draw_kiosk_leaderboard()` and `draw_kiosk_multicam()` are self-contained, readable, and independently modifiable.
- The keyboard-only interaction model is consistent and honest — no instruction ever references the mouse.
- The leaderboard and multicam views gain visual hierarchy appropriate for public display at a fair.

**Negative Consequences (Disadvantages):**

- Dynamic allocation (`calloc`/`free`) replaces compile-time static arrays. The kiosk controller must be properly initialized before first use and freed before program exit. Both invariants are enforced by the `initialized` flag check.
- `compute_kiosk_layout()` uses integer division which produces rounding differences of ±1 px for non-square match counts. This is visually imperceptible.

---

#### **4. Alternatives Considered**

- **Single `#define MATCH_COUNT 4` with static arrays:** Would fix the magic-number problem but not the scalability problem. Rejected — static arrays cannot be resized at runtime.
- **Storing `KioskLayout` in `KioskController`:** Would avoid recomputing the layout every frame. Rejected for Phase B — `compute_kiosk_layout()` is a pure arithmetic function with negligible cost; recomputing on demand avoids stale state if the window is resized.
- **Separate `kiosk_renderer.c` translation unit:** Would give kiosk rendering its own file. Reasonable future refactor; rejected for now to minimize the number of new files per change.
