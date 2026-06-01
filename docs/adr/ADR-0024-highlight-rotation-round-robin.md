### **ADR-0024: Client-Side Round-Robin Highlight Rotation for Kiosk Mode**

**Status:** Implemented

**Date:** 2026-05-31

---

#### **1. Context and Problem Statement**

The Kiosk Mode displays up to 10 highlight matches per epoch in its MongoDB `epoch_highlights` collection. However, the C network layer (`network_io.h`) parses only the first 4 matches (`matches[4]`, loop bound `i < 4`), and the `KioskController` loads these same 4 matches on every Multicam cycle. The result is that a single epoch — which may run for 60 seconds but whose matches play for 30 seconds each — shows the **same 4 matches on every display rotation** until the next epoch completes.

This is Problem A identified in the design review (companion to ADR-0023 Problem B). A spectator who watches the kiosk for longer than one 30-second Multicam cycle sees no new content. The exhibition fails to demonstrate the breadth of the tournament pool.

**Root cause:** The network layer caps the highlight pool at 4, and there is no mechanism to advance through the remaining candidates.

---

#### **2. Decision**

Implement **client-side round-robin rotation** within the `KioskController`. The full highlight pool (up to 10 matches) is fetched and stored on the client. Each time the kiosk completes a Multicam cycle and transitions back to the Leaderboard, the pool index advances by `match_count` (4). On the next Multicam cycle, the next 4 matches from the pool are loaded. Wrap-around is handled with modulo arithmetic so the rotation is seamless.

**Changes required:**

| File | Change |
|---|---|
| `src/io/network_io.h` | `MatchHighlight matches[4]` → `matches[10]`; update comment |
| `src/io/network_io.c` | Parse loop bound `i < 4` → `i < 10` |
| `src/gui/app_state_manager.h` | Add `int highlight_pool_index` and `int highlight_pool_size` to `KioskController` |
| `src/gui/app_state_manager.c` | (a) Init: set `pool_index = 0`, `pool_size = 0`; (b) on new highlight data: reset `pool_index = 0`, set `pool_size = hd.count`; (c) slot loading: `pool[(pool_index + i) % pool_size]`; (d) on MULTICAM → LEADERBOARD transition: `pool_index = (pool_index + match_count) % pool_size` |

**Slot-loading formula (pseudocode):**
```c
for (int i = 0; i < kiosk_ctrl.match_count; i++) {
    int slot = (kiosk_ctrl.highlight_pool_index + i) % kiosk_ctrl.highlight_pool_size;
    load_match_into_sim(i, &kiosk_ctrl.cached_highlights.matches[slot]);
}
```

**Guard condition:** Slot-loading and index-advance must only execute when `highlight_pool_size > 0` to avoid division-by-zero on modulo and NULL-access before the first API response arrives.

**Why client-side and not server-side:**

The server already returns up to 10 highlights per epoch, ordered by the Python Worker (ADR-0023: non-oscillating first). Rotation on the server would require per-client session state or a global index counter, introducing statefulness into what is currently a stateless API. Client-side rotation is simpler, correct, and requires no backend changes.

**Why round-robin and not random shuffle:**

Round-robin is deterministic and guarantees that every match in the pool is shown within `ceil(pool_size / match_count)` Multicam cycles. A random shuffle could repeat matches before exhausting the pool. Round-robin also makes the behavior predictable during debugging.

**Why reset on new epoch data:**

When a new epoch completes, `pool_index` is reset to 0 so the highest-ranked (non-oscillating) matches are always shown first in the new epoch. This preserves the ranking intent of the Python Worker filter.

---

#### **3. Consequences of the Decision**

**Positive Consequences:**

- **Visual variety:** Up to 10 distinct matches are shown per epoch across 2–3 Multicam cycles (~1.5–2 minutes of continuous display). A spectator who stays for the full cycle sees different content each rotation.
- **No backend changes:** The FastAPI endpoint and MongoDB schema are unchanged. The Python Worker already stores up to 10 highlights; the client simply consumes more of them.
- **Minimal C changes:** 4 files touched, ~15 lines changed. No new data structures, no heap allocation, no threading concerns.
- **Synergy with ADR-0023:** The oscillator filter ensures the pool contains visually dynamic matches. Round-robin ensures all of them are displayed, not just the first 4.
- **Graceful degradation:** If fewer than 4 highlights exist in the pool, `% pool_size` wraps correctly and slots may repeat — the same behavior as before this change.

**Negative Consequences:**

- **Slight implementation coupling:** `highlight_pool_index` must be reset whenever `cached_highlights` is refreshed. If a future refactor changes where API data is consumed in `app_state_manager.c`, this reset must be moved accordingly.
- **Pool hardcoded to 10:** The `matches[10]` cap in `network_io.h` is a new magic number. If the backend is ever changed to return more than 10 highlights, the C struct must be updated in sync.

---

#### **4. Alternatives Considered**

**Alternative 1 — Server-side cursor / pagination:**
The API could accept a `?offset=N` parameter and return a sliding window of 4 highlights per request. Rejected: requires server-side session state or epoch-aware cursor logic, adds API complexity, and makes multiple kiosk instances diverge in what they display (each would independently advance their offset).

**Alternative 2 — Random selection from pool on each cycle:**
On each Multicam load, pick 4 random highlights from the pool. Rejected: non-deterministic, can repeat matches before exhausting the pool, harder to debug, and does not guarantee all highlights are shown within a bounded number of cycles.

**Alternative 3 — Fetch all 10 on each cycle (no local pool):**
Issue a new API request each time the kiosk leaves MULTICAM. Rejected: increases network load, introduces latency at a moment when the kiosk is transitioning states, and adds error-handling complexity for mid-cycle request failures.
