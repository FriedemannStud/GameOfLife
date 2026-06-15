### **ADR-0031: Auto-Paging Rotation for the Kiosk Global Leaderboard**

**Status:** accepted

**Date:** 2026-06-15

#### **1. Context and Problem Statement**

The kiosk exhibition mode renders a single, non-interactive "GLOBAL LEADERBOARD"
screen (`draw_kiosk_leaderboard`, `src/gui/renderer.c`). Today this screen shows
only the **top ~9 players**: the row layout targets ~10 visible rows
(`rowHeight = entries_area_h / 10`) and the last visible slot is reserved for the
honest "+N more" overflow line (ADR-0030).

The 9-row limit is purely a *rendering* limit, not a data limit:

*   The backend `GET /api/leaderboard` already returns up to **20** ranked rows
    plus a truthful server-side `total_count` (`backend/app/main.py`, ADR-0030).
*   The C client parses up to `MAX_LEADERBOARD_ENTRIES = 20` rows into a fixed
    `LeaderboardData` array (`src/io/network_io.h`).

We expect the participant field to grow well beyond 9. With the current design a
visitor never sees ranks 10+ at all — they are collapsed into the "+N more" line.

Hard constraints that shape any solution:

*   **No user input.** The kiosk screen is passive; there is no scrolling, mouse,
    or keyboard interaction available during the leaderboard slot. Any "show more"
    mechanism must be driven by time, not interaction.
*   **Glanceability.** The display is read from a distance, so rows must stay large
    enough to read; we cannot solve this by shrinking rows indefinitely.
*   **Existing timing.** The leaderboard sub-state currently runs a fixed **15 s**
    (`state_timer > 15.0f` in `src/gui/app_state_manager.c`) before switching to
    the Multicam sub-state for 30 s.
*   **Zero-warning build** (`-Wall -Wextra`) and the `create_world`/fixed-array,
    no-hot-loop-allocation conventions of the project.

#### **2. Decision**

Introduce **time-driven auto-paging** of the leaderboard: within the leaderboard
slot, the display automatically advances through *all* pages of ranked players
(e.g. ranks 1–9, then 10–18, then 19–…), then hands off to Multicam as today.
This mirrors the existing round-robin rotation pattern used for highlights
(ADR-0024) but keeps the rows full-size and glanceable.

Two product parameters were fixed during the design interview:

*   **Rotation scope:** *all pages per visit*. Every player is shown during each
    leaderboard visit; the slot duration scales with the number of pages.
*   **Capacity:** support up to **50 ranked players** end-to-end.

**Concrete design:**

1.  **Capacity raise (data layer).**
    *   Backend: a named constant `LEADERBOARD_MAX_ROWS = 50` replaces the literal
        `20` in the `.limit(...)`/`.to_list(...)` of `get_leaderboard`. `total_count`
        stays an independent server-side count, so "+N more" remains honest if more
        than 50 eligible submissions exist.
    *   C client: `MAX_LEADERBOARD_ENTRIES` is raised `20 → 50`. The
        `LeaderboardEntry entries[MAX_LEADERBOARD_ENTRIES]` array grows accordingly
        (still a fixed, statically sized array — no dynamic allocation added). The
        non-atomic-read safety clamps already present (ADR-0030) are preserved.

2.  **Paging math (single source of truth in the renderer).**
    A new helper `kiosk_leaderboard_page_count(const KioskController *ctrl,
    int screen_w, int screen_h)` (declared in `renderer.h`, defined in
    `renderer.c`) computes, from the existing `compute_kiosk_layout`:
    *   `rows_per_page` = the number of full rows that fit above the footer
        (the current `max_fit` logic, with the `rowHeight >= 28 px` floor kept).
        When the true total exceeds the parsed cap (i.e. more than 50 eligible
        players), `rows_per_page` is reduced by one so the last page always has
        room for the "+N more" line. This keeps `rows_per_page` uniform across all
        pages.
    *   `num_pages = ceil(parsed_entries / rows_per_page)`, minimum 1.

    Both the renderer and the state machine derive the **current page** purely from
    the already-existing `state_timer`, so no new mutable rotation state is stored
    on `KioskController`:
    `page_index = clamp((int)(state_timer / KIOSK_LB_SECONDS_PER_PAGE), 0, num_pages - 1)`.

3.  **Rendering.** `draw_kiosk_leaderboard` renders the row window
    `[page_index * rows_per_page, …]`. Rank numbers and the top-3 colour/background
    highlight use the **global** rank (`page_index * rows_per_page + i + 1`), so the
    podium styling only appears on the first three overall — not on every page. A
    `PAGE x / y` indicator is drawn next to the title when `num_pages > 1`. The
    "+N more" line is drawn only on the **last** page, using the existing
    `true_total - parsed_entries` overflow count.

4.  **Adaptive slot duration.** The leaderboard sub-state duration becomes
    `lb_duration = clamp(num_pages * KIOSK_LB_SECONDS_PER_PAGE,
    KIOSK_LB_MIN_DURATION, KIOSK_LB_MAX_DURATION)` instead of the hard-coded 15 s.
    Proposed constants (in `renderer.h`, shared by both translation units):
    `KIOSK_LB_SECONDS_PER_PAGE = 6.0f`, `KIOSK_LB_MIN_DURATION = 15.0f`,
    `KIOSK_LB_MAX_DURATION = 48.0f`. With a single page this yields the unchanged
    15 s behaviour; with 6 pages (50 players) it yields 36 s before handing off to
    Multicam. The footer progress bar is updated to use `lb_duration` instead of the
    literal `15.0f`.

All new or significantly modified blocks carry the `// KI-Agent unterstützt`
attribution comment per `docs/CODING_STYLE.md`.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

*   Scales to many more players (up to 50 directly, with an honest "+N more" beyond)
    while keeping rows large and readable from a distance.
*   Reuses an established, understood project pattern (time-driven kiosk rotation,
    ADR-0024), aiding maintainability.
*   Mostly **stateless**: the page index is derived from the existing `state_timer`,
    adding no new mutable controller fields and therefore no new reset/lifecycle
    bugs. The page-count math lives in exactly one helper.
*   Single-page behaviour is bit-for-bit equivalent to today (15 s, no rotation),
    so the change is a no-op for small fields and degrades gracefully.
*   Backend change is a one-line cap raise behind a named constant; `total_count`
    keeps "+N more" correct independent of the cap.

**Negative Consequences (Disadvantages):**

*   Lower-ranked players are visible only after waiting through earlier pages
    (inherent to a passive, input-free display).
*   The leaderboard slot is no longer a fixed 15 s; the overall kiosk cycle length
    now varies with the number of active players (bounded by `KIOSK_LB_MAX_DURATION`).
*   Raising `MAX_LEADERBOARD_ENTRIES` to 50 enlarges the static `LeaderboardData`
    array (~50 × ~300 B); negligible, but worth noting as a fixed-size bump.
*   The renderer and state machine must agree on `KIOSK_LB_SECONDS_PER_PAGE`; this is
    enforced by sharing the constant and the page-count helper rather than
    duplicating the math.

#### **4. Alternatives Considered**

*   **One page per visit (round-robin across visits).** Show a single page each
    leaderboard visit and advance on the next visit, exactly like the highlight pool
    (ADR-0024). Keeps the 15 s timing untouched, but a visitor must wait through
    multiple full kiosk cycles (≈ a minute+ each) to see lower ranks — poor for a
    walk-up exhibition. Rejected in favour of showing all pages per visit.

*   **Multi-column dense layout.** Render two columns (ranks 1–10 / 11–20) on one
    static screen. Shows more at a glance with no waiting, but is hard-capped at what
    fits (~20), reduces the player-name column width, and is denser/less readable
    from a distance. Solves the problem only partially for a growing field.

*   **Shrink rows / raise the visible cap only.** Lower the `rowHeight` floor and
    raise the parsed cap so more rows fit on one screen. Trivial, but readability
    degrades and it remains hard-capped. A stop-gap, not a scalable solution.

*   **Interactive scrolling.** Not viable: the kiosk leaderboard slot has no input
    devices bound, by design.
