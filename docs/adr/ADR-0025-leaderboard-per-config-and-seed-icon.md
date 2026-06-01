### **ADR-0025: Per-Configuration Leaderboard with 8x8 Start-Config Icon**

**Status:** Implemented

**Date:** 2026-06-01

---

#### **1. Context and Problem Statement**

The Kiosk "Global Leaderboard" should show each entry's 8x8 start configuration as an icon
between the rank and the player name — analogous to the seed thumbnails already rendered in
the "Live Battles" Multicam quadrants (`renderer.c:1472-1500`).

Analysis surfaced two coupled problems:

1. **Data availability:** The leaderboard endpoint read from the `players` collection, which
   contains **no pattern data**. The start configuration lives as `config.cells` (a sparse
   `[x, y]` list) in the `submissions` collection. The leaderboard render path therefore had no
   access to the seed at all.

2. **Ranking distortion on multiple submissions:** The tournament computes results
   **per-submission** (each active submission is an independent competitor in the O(N²)
   round-robin, `worker.py:134-150`, `main_hyper.c:67-128`). The worker then aggregates onto
   `players` keyed by `nickname` using `update_one`/`$set` — last-write-wins
   (`worker.py:243-256`). Because rankings are iterated best-first, the **weakest** submission of
   a nickname overwrites the displayed stats. A player submitting several configs is shown with
   their worst result, even though their best config fully participated in the tournament.

---

#### **2. Decision**

The leaderboard reads **directly from `submissions`** (per-configuration) instead of aggregating
per-nickname from `players`. Each active, already-ranked submission becomes its own leaderboard
row with its own stats and its own 8x8 seed icon.

This single decision:
- Provides the pattern data (it is already in `submissions.config.cells`).
- Eliminates the last-write-wins distortion (each config keeps its own correct stats).
- Makes the icon meaningful — it visually disambiguates rows that share a nickname.

No worker change is required: the worker already writes all ranking fields per-submission onto
the `submissions` collection (`worker.py:225-240`).

**Changes required:**

| File | Change |
|---|---|
| `backend/app/grid_utils.py` | **New** `cells_to_grid()` — sparse `[x,y]` → dense 64-int grid, `index = y*8 + x` |
| `backend/app/main.py` | `get_leaderboard` reads `submissions` (`status=active`, `matches_played>0`), sorted `win_rate desc, avg_stable_generation asc`, limit 20; adds `"seed"` field per entry |
| `src/io/network_io.h` | Add `int seed[GRID_SIZE_8X8]` to `LeaderboardEntry` |
| `src/io/network_io.c` | Parse `"seed"` array into the entry (defensive default 0, 64-bound guard) |
| `src/gui/renderer.c` | `draw_kiosk_leaderboard`: insert CONFIG icon column between RANK and PLAYER; reflow column percentages; render 8x8 icon coupled to `rowHeight` |

**Orientation convention (binding):** `config.cells` are `[x=col, y=row]` (`file_io.c:87`). The
canonical dense index is `b = row*8 + col = y*8 + x` (`game_logic.c:197`, serialised at
`file_io.c:549`). The renderer reads `seed[tr*8+tc]` with `tr=row, tc=col`
(`renderer.c:1482-1489`). The backend conversion therefore sets `seed[y*8 + x] = 1`. Any
deviation produces a mirrored or transposed icon.

---

#### **3. Consequences**

- **Positive:** Correct per-config stats; pattern icon realised with the same visual language as
  Live Battles; no backend worker changes; one indexed `find` (no N+1 queries).
- **Trade-off:** A nickname can now appear multiple times (once per active config). This is
  intentional and is exactly what the icon disambiguates.
- **Coupling:** The leaderboard endpoint now depends on the `submissions.config.cells` schema.
  The sparse→dense conversion is isolated in the reusable `cells_to_grid()` helper to keep it
  DRY (the C hyper-worker performs the equivalent conversion via `grid_to_bitboard`).
- **Robustness:** Empty/missing/out-of-bounds cells yield an empty icon (no crash); the icon size
  is coupled to `rowHeight` so it never collides with text or the footer panel.
