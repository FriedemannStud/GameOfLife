### **ADR-0023: Oscillator Detection and Filtering for Kiosk Highlight Quality**

**Status:** Implemented

**Date:** 2026-05-31

---

#### **1. Context and Problem Statement**

The Kiosk Mode's "Live Battles" display (`KIOSK_SUB_MULTICAM`) selects the top 4 from 10 highlight candidates produced by the C Hyper-Worker. Candidates are ranked by `activity_sum` — the cumulative count of all cell births and deaths across every generation of a match. This metric was designed to surface the most *dynamic* battles: contests where populations rise, fall, and compete for territory.

However, `activity_sum` has a systematic blind spot: **stable oscillating formations** (cellular patterns that cycle through states with period 2 to 5) achieve disproportionately high scores. A period-2 oscillator running for all 1000 maximum generations contributes approximately `activity_per_cycle × 500` to the sum. These formations rank at the very top of the activity metric yet are visually identical to a static image for a spectator: the same small cluster of cells flips back and forth with no net territorial change, generation after generation.

The problem manifests in two concrete ways:

1. **Incorrect ranking bias:** Oscillators permanently displace genuinely exciting matches (complex territorial dynamics, shifting front lines, dramatic comebacks) from the top-10 highlight pool.
2. **Poor exhibition experience:** Visitors at the university fair who observe the kiosk screen during a 30-second display window see no meaningful visual change. The exhibition fails its core goal of demonstrating interesting emergent behavior.

The root cause is that `activity_sum` cannot distinguish between *progress* (cells moving, teams competing for new territory) and *stasis* (periodic repetition with no net change). Both produce high `activity_sum` values, but only the former is visually engaging.

---

#### **2. Decision**

Implement oscillator detection as a **post-processing filter in the Python Worker** (`backend/app/worker.py`), executed after the C Hyper-Worker completes and before highlights are persisted to MongoDB. The feature is implemented in two new functions in `worker.py` and one integration call inside `execute_epoch()`. No C files are modified.

**Detection Algorithm:**

For each of the 10 highlight candidates output by the Hyper-Worker:

1. **Re-simulate** the match in Python using the **Kiosk world configuration**: an 8×16 grid with toroidal (wrapping) boundaries, with the red seed placed at rows 0–7 / cols 0–7 (left half) and the blue seed at rows 0–7 / cols 8–15 (right half). This mirrors the placement in `app_state_manager.c` (`KIOSK_SIM_ROWS = 8`, `KIOSK_SIM_COLS = 16`, constants defined via `LOCAL_GRID_SIZE` from `config.h`). The constants in `worker.py` are `_RED_ORIGIN = (0, 0)` and `_BLUE_ORIGIN = (0, 8)`.

2. **Run** the simulation for up to `max_generations` steps (1000 by default), applying the same two-team Conway rules as `update_generation()` in `game_logic.c`:
   - A living cell survives if it has 2 or 3 total neighbors (keeps its team).
   - A dead cell is born if it has exactly 3 total neighbors; it joins the majority team (RED if `red_neighbors > blue_neighbors`, else BLUE).
   - Boundary: toroidal (wrap-around).

3. **Maintain a rolling buffer** of the last 11 grid states as raw byte snapshots (`grid.tobytes()`). After each generation, check for truly static state (`current == previous`) — if detected, return `False` immediately (not an oscillator; the match has settled).

4. **After the simulation completes**, apply the period check for P ∈ {2, 3, 4, 5} with 2 confirmed cycles:
   ```
   state_buffer[-1] == state_buffer[-1 - P]  AND
   state_buffer[-1 - P] == state_buffer[-1 - 2*P]
   ```
   If any P satisfies both conditions → the match is flagged as an oscillator.

5. **Reorder** the final highlights list: non-oscillating matches first (preserving descending `metric_value` order), oscillating matches appended as a fallback pool (also descending by `metric_value`).

**Soft Fallback Policy:**

If fewer than 4 non-oscillating matches exist, oscillating matches fill the remaining slots from the fallback pool. The kiosk never displays fewer than `min(4, total_highlights_count)` matches. The MongoDB document always stores the complete ordered list of all available highlights.

**Why the 8×16 world (same as Hyper-Worker):**

Oscillator behavior is world-size-dependent: the same seeds produce different dynamics in differently sized arenas. Using the same world for detection and display is a correctness requirement — a match flagged as oscillating must actually be oscillating as the spectator sees it. Aligning the Kiosk display world to the Hyper-Worker world (both 8×16, `LOCAL_GRID_SIZE × LOCAL_GRID_SIZE × 2`) also eliminates a semantic inconsistency where the leaderboard winner and the on-screen winner could differ. The Python detector uses `_KIOSK_ROWS = 8`, `_KIOSK_COLS = 16` — identical to `run_isolated_match()` in `game_logic.c`.

**Decision Summary — Why Python Worker:**

| Criterion | C Hyper-Worker | Kiosk Client (C) | Python Worker |
|---|---|---|---|
| Async to headless calc (MUST) | **✗ Violates** | ✓ | ✓ |
| Centralized (all kiosks benefit) | ✓ | ✗ | ✓ |
| No C codebase changes | ✗ | ✗ | **✓** |
| Implementation cost | High | High | **Low** |
| Maintainability | Low | Low | **High** |
| Consistency across kiosk instances | ✓ | ✗ | ✓ |

---

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

- **Visual quality improvement:** The kiosk display consistently shows dynamically evolving battles; oscillating patterns are suppressed before they ever reach the client.
- **No changes to the C codebase:** The Hyper-Worker, Kiosk Client (`app_state_manager.c`), and network layer (`network_io.c`) are entirely unaffected.
- **Centralized logic:** One filtering implementation in `worker.py` serves all consumers of `/api/epoch/highlights`. Multiple kiosk instances automatically receive pre-filtered data without independent detection.
- **Deterministic per epoch:** The same set of seeds always produces the same detection outcome; no random variation or race conditions across display instances.
- **Graceful degradation:** The soft fallback ensures the kiosk always displays content, even in a degenerate case where all submitted patterns produce oscillators.
- **Logging:** Detection events are logged per epoch, giving operators visibility into how many highlights were filtered.

**Negative Consequences (Disadvantages):**

- **Duplicated simulation logic:** The Conway's Life rules are implemented in both `game_logic.c` (C) and `worker.py` (Python). If the game rules change in `game_logic.c`, `worker.py` must be updated in sync. This creates a maintenance dependency that must be documented.
- **World configuration coupling:** The kiosk seed placement coordinates (`_RED_ORIGIN = (0, 0)`, `_BLUE_ORIGIN = (0, 8)`) and world dimensions (`_KIOSK_ROWS = 8`, `_KIOSK_COLS = 16`) are duplicated from `app_state_manager.h` / `config.h` into `worker.py`. If the C client changes the world size or seed placement, the Python detector becomes inaccurate without a corresponding update.
- **Marginal epoch processing time increase:** Simulating 10 matches × 1000 generations × 8×16 grid in Python (NumPy) adds well under 1 second to each 60-second epoch cycle. This is negligible in practice.

---

#### **4. Alternatives Considered**

**Alternative 1 — Detection in the C Hyper-Worker (`main_hyper.c`):**
Would be the most accurate option, reusing the existing simulation context and the exact C game rules. Rejected because it would add detection work synchronously inside the OpenMP match loop, violating the hard constraint that detection must be asynchronous to the headless calculation.

**Alternative 2 — Detection in the Kiosk Client (C):**
The Kiosk Client already simulates all 4 matches live and has full access to grid states at every generation. However, this approach requires: (a) expanding the API to expose all 10 highlights instead of 4; (b) adding pool management, oscillator flag state, and slot-swap logic to `KioskController`; (c) each physical kiosk device independently detecting and swapping matches, leading to inconsistent display across multiple kiosk instances. High implementation cost, low robustness, no centralization benefit.

**Alternative 3 — Short re-simulation window (e.g., 200 generations):**
Would reduce execution time but miss oscillators that only emerge after long convergence. A match might undergo genuine territorial conflict for 800 generations before one team is eliminated and the survivor's remnant colony enters an oscillating final state. The full 1000-generation simulation is required to reliably catch end-state oscillators.

**Alternative 4 — Filtering at the API endpoint on each request:**
Per-request simulation would be unacceptable for a hot path served to potentially multiple concurrent kiosk clients. No persistence benefit — filtering would be redundantly re-run on every API call with no ability to log or audit results.
