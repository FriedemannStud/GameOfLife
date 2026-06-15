### **ADR-0030: Honest "+N more" Overflow Count on the Global Leaderboard**

**Status:** Proposed

**Date:** 2026-06-15

Supersedes a single unintended consequence of
[ADR-0025](./ADR-0025-leaderboard-per-config-and-seed-icon.md) (the overflow indicator's
semantics). All other ADR-0025 decisions remain in force.

---

#### **1. Context and Problem Statement**

The Kiosk "Global Leaderboard" (`draw_kiosk_leaderboard`, `src/gui/renderer.c`) draws a capped
number of rows and, when entries are clipped, a `"+ N more"` line below the table. The intent of
that line is to tell the viewer how many further start configurations exist beyond what fits on
screen.

The indicator is **frozen** and does not reflect reality. Once enough ranked configurations exist,
it stalls at a constant value (observed in the field: `"+ 11 more"`) and never changes again, no
matter how many additional configurations are submitted and ranked.

Two hard caps of **20** are responsible:

1. **Backend** (`backend/app/main.py`, `get_leaderboard`): the query ends in
   `.limit(20).to_list(length=20)`. The endpoint never returns more than 20 rows and never reports
   how many eligible rows exist in total.

2. **C client** (`src/io/network_io.h`): `#define MAX_LEADERBOARD_ENTRIES 20`; the parser
   (`network_io.c`) stops at that bound and stores the parsed row count in
   `LeaderboardData.count`.

The renderer then computes the overflow from the **capped** total:

```c
int total_entries = ctrl->cached_lb.count;                 // ≤ 20
if (total_entries > MAX_LEADERBOARD_ENTRIES) total_entries = MAX_LEADERBOARD_ENTRIES;
...
int max_fit  = entries_area_h / rowHeight;                  // geometry only, constant per window
show_count   = max_fit - 1;                                 // when clipped
hidden_count = total_entries - show_count;                  // = 20 - show_count  → constant
```

Because `total_entries` saturates at 20 and `show_count` depends only on window geometry, once the
backend holds ≥ 20 ranked configurations `hidden_count` is mathematically constant. A reported
`"+ 11 more"` implies `show_count = 9` (`max_fit = 10`). The number can therefore never exceed
`20 − show_count` and is structurally incapable of representing the true backlog.

A second, related gap: the overflow line is only emitted on the *geometric* clip path
(`total_entries > max_fit`). If every parsed row happens to fit on screen but more eligible rows
exist server-side, no `"+ N more"` is shown at all.

**Constraints:**
- Kiosk readability is non-negotiable: the *number of drawn rows* must stay bounded (the geometric
  cap and the footer panel protection from ADR-0022 must remain intact).
- The C build must stay at zero warnings (`-Wall -Wextra -std=c99`).
- Backwards compatibility: a newer client must not break against an older backend that does not
  yet send the new field, and vice versa.

---

#### **2. Decision**

Transport the **true server-side total** of eligible leaderboard rows alongside the (still capped)
20-row payload, and compute the overflow indicator from that true total instead of from the capped
row count.

**Semantics:** "eligible" is defined by exactly the same filter the leaderboard already uses —
`status == "active"` **and** `matches_played > 0`. The count and the returned rows must be derived
from one shared query so the number can never disagree with the list.

**Changes required:**

| File | Change |
|---|---|
| `backend/app/main.py` | In `get_leaderboard`, factor the filter into one `query` dict; add `total_count = await db.submissions.count_documents(query)`; reuse `query` for the existing `find(...).limit(20)`; return `"total_count"` alongside `"leaderboard"`. |
| `src/io/network_io.h` | Add `int total_count;` to `LeaderboardData` (distinct from `count`, the number of parsed rows; may exceed `MAX_LEADERBOARD_ENTRIES`). |
| `src/io/network_io.c` | Parse `"total_count"` into the struct under the existing mutex; **fall back to `count`** when the field is absent or non-numeric. |
| `src/gui/renderer.c` | In `draw_kiosk_leaderboard`, derive `hidden_count` from the true total: emit `"+ N more"` whenever rows are clipped *or* the true total exceeds the parsed count, with a `true_total >= parsed_entries` safety clamp. |

The two 20-caps are **retained by design**: the backend still returns at most 20 *records* and the
client still stores at most 20. Only the *count* becomes honest. Showing the 21st+ record would be
a separate decision (raising both `.limit(...)` and `MAX_LEADERBOARD_ENTRIES`) and is explicitly
out of scope.

---

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

- The `"+ N more"` line reflects the actual number of additional ranked configurations and updates
  as submissions are ranked — the reported defect is resolved.
- The overflow line now also appears in the previously-missed case where all parsed rows fit but
  more eligible rows exist server-side.
- One extra `count_documents` on an already-filtered set is cheap (it uses the same predicate as
  the existing indexed `find`); no N+1, no new collection access.
- Forwards/backwards compatible: the `count`-fallback means a new client against an old backend
  behaves exactly as today, and an old client simply ignores the new field.

**Negative Consequences (Disadvantages):**

- A second query is issued per leaderboard fetch (`count_documents` + `find`). On a 60 s Kiosk
  refresh cadence this is negligible, but it is a non-atomic read: between the two queries the set
  could change, so `total_count` and the row list may momentarily differ by one. The
  `true_total >= parsed_entries` clamp keeps this benign (never a negative `hidden_count`).
- The displayed number can now exceed what the table can ever show (e.g. "+ 40 more" while only 20
  records are transmittable), which is the intended honesty but means the line is informational,
  not navigational — there is no UI to page to those rows.

---

#### **4. Alternatives Considered**

1. **Raise both 20-caps (e.g. to 100) instead of adding a count.** Rejected: it inflates payload
   and parse cost for data the Kiosk cannot legibly display (the geometric row cap stays ~10), and
   it still would not produce a correct count beyond the new cap — it only moves the ceiling.

2. **Compute the overflow purely client-side from a separate `/api/leaderboard/count` endpoint.**
   Rejected: a second round-trip and a second async fetch path in `network_io.c` for one integer,
   with its own freshness/race window, is more moving parts than piggy-backing the value on the
   existing response.

3. **Aggregate the count from the `players` collection.** Rejected: ADR-0025 deliberately moved the
   leaderboard off `players` (per-nickname last-write-wins distortion). Counting `players` would
   reintroduce a different denominator than the `submissions`-based rows and could disagree with
   the list.

4. **Drop the `"+ N more"` line entirely.** Rejected: it communicates real competitive depth in the
   exhibition context; the defect is that it lies, not that it exists.
