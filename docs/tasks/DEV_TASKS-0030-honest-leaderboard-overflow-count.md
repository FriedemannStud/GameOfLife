# DEV_TASKS-0030: Honest "+N more" Overflow Count on the Global Leaderboard

This task implements ADR-0030: the Kiosk "Global Leaderboard" reports the **true** number of
eligible ranked configurations behind its `"+ N more"` line, instead of a value that saturates at
`20 − visible_rows` (observed in the field as a frozen `"+ 11 more"`).

The change threads one new integer — the server-side total — through three layers: backend
response, C network struct/parser, and the renderer's overflow computation. Both existing 20-record
caps are retained by design; only the *count* becomes honest.

**Developer:** Follow each step in sequence. After every Verification step, report the result
before moving on. Per `docs/CODING_STYLE.md`: English only, `snake_case`/`PascalCase`/`UPPER_SNAKE`,
4-space indent, every AI-written block carries `// KI-Agent unterstützt`. Per
`docs/DEVELOPMENT_GUIDELINES.md`: `make` must end at **zero warnings**.

**Briefing Document:**
- [ADR-0030: Honest "+N more" Overflow Count on the Global Leaderboard](../adr/ADR-0030-honest-leaderboard-overflow-count.md)
- Context: [ADR-0025: Per-Configuration Leaderboard](../adr/ADR-0025-leaderboard-per-config-and-seed-icon.md), [DEV_TASKS-0025](./DEV_TASKS-0025-leaderboard-config-icon.md)

---

## Phase 1: Backend — Report the True Total

*Goal: `/api/leaderboard` returns `total_count`, the number of eligible rows, derived from the same
filter as the returned rows.*

- [x] **Step 1.1: Single-source the filter and add the count.**
    - [x] **Action:** In `backend/app/main.py`, `get_leaderboard`, extract the filter into one local
      `query = {"status": "active", "matches_played": {"$gt": 0}}`.
    - [x] **Action:** Reuse `query` in the existing `db.submissions.find(query)...limit(20)` call (no
      behavioural change to the rows).
    - [x] **Action:** Add `total_count = await db.submissions.count_documents(query)` **before** the
      `find`, with a `// KI-Agent unterstützt` comment noting the count must share `query` with the
      list so the two can never disagree.
    - [x] **Action:** Return `{"leaderboard": leaderboard, "total_count": total_count}`.
    - [x] **Verification:** With a running backend and ranked data:
        ```bash
        curl -s localhost:8000/api/leaderboard | jq '{rows: (.leaderboard|length), total_count}'
        ```
        **Expected Result:** `rows` is ≤ 20; `total_count` is an integer ≥ `rows`. When more than 20
        eligible configurations exist, `total_count` > 20 while `rows` stays 20.

- [x] **Step 1.2: Guard the empty / no-data case.**
    - [x] **Action:** Confirm that with zero eligible submissions the endpoint returns
      `{"leaderboard": [], "total_count": 0}` (no exception). No code change expected — `count_documents`
      on an empty match returns 0.
    - [x] **Verification:** If a clean DB is available, the curl above shows `{"rows": 0, "total_count": 0}`.
      Otherwise reason about it and report. **Expected Result:** No 500; `total_count` is 0, not absent.

---

## Phase 2: C — Network Struct and Parser

*Goal: the client carries the true total separately from the parsed-row count, and degrades safely
against an old backend.*

- [x] **Step 2.1: Add the field.**
    - [x] **Action:** In `src/io/network_io.h`, add `int total_count;` to `LeaderboardData`
      (immediately after `int count;`), with a `// KI-Agent unterstützt` comment: *true server-side
      total, may exceed `MAX_LEADERBOARD_ENTRIES`; `count` remains the number of parsed rows.*
    - [x] **Verification:** Field added; struct still compiles in the next build step.

- [x] **Step 2.2: Parse with a fallback.**
    - [x] **Action:** In `src/io/network_io.c`, inside the leaderboard parse path and under the
      existing `g_network_mutex` lock, read `total_count` from the JSON root *after* the row loop:
        ```c
        // KI-Agent unterstützt: Honest overflow total; fall back to parsed count for old backends
        cJSON* total = cJSON_GetObjectItemCaseSensitive(root, "total_count");
        g_leaderboard.total_count = cJSON_IsNumber(total) ? total->valueint : g_leaderboard.count;
        ```
    - [x] **Action:** Ensure the getter `network_get_leaderboard` copies the whole struct (it copies
      by value / `memcpy` of `LeaderboardData`), so no extra plumbing is needed — verify, don't assume.
    - [x] **Verification:** Read the getter and confirm `total_count` propagates. **Expected Result:**
      the consumer (`KioskController.cached_lb`) will see `total_count`.

---

## Phase 3: C — Honest Overflow in the Renderer

*Goal: `hidden_count` reflects the true total; the `"+ N more"` line also appears when all parsed
rows fit but more eligible rows exist server-side. The number of drawn rows is unchanged.*

- [x] **Step 3.1: Compute overflow from the true total.**
    - [x] **Action:** In `draw_kiosk_leaderboard` (`src/gui/renderer.c`), replace the overflow block.
      Keep the geometric row cap untouched; change only how `show_count` / `hidden_count` are derived:
        ```c
        // KI-Agent unterstützt: Overflow counts the true server-side total, not the 20-capped rows (ADR-0030)
        int parsed_entries = ctrl->cached_lb.count;
        if (parsed_entries > MAX_LEADERBOARD_ENTRIES) parsed_entries = MAX_LEADERBOARD_ENTRIES;

        int true_total = ctrl->cached_lb.total_count;
        if (true_total < parsed_entries) true_total = parsed_entries;   // non-atomic-read safety clamp

        int max_fit      = entries_area_h / rowHeight;
        int show_count   = parsed_entries;
        int hidden_count = 0;
        if (parsed_entries > max_fit || true_total > parsed_entries) {
            show_count   = (parsed_entries > max_fit) ? max_fit - 1 : parsed_entries;
            hidden_count = true_total - show_count;
        }
        ```
    - [x] **Action:** Leave the existing `"+ %d more"` draw block (`sprintf` + `DrawText`) as-is; it
      already keys off `hidden_count > 0` and `more_y` from `show_count`.
    - [x] **Verification:** `make` produces **zero warnings**. Report the tail of the build output.

- [x] **Step 3.2: Reason through the boundary cases (no code).**
    - [x] **Action:** Confirm on paper, given a window where `max_fit = 10`:
        1. `total_count = 25`, `count = 20` → `show_count = 9`, `hidden_count = 16` (was frozen 11).
        2. `total_count = 6`, `count = 6`, all fit → previously *no* line; now
           `6 > 10`? no, `6 > 6`? no → still no line. Correct (nothing hidden).
        3. `total_count = 14`, `count = 14`, `max_fit = 10` → clip path: `show_count = 9`,
           `hidden_count = 5`. Correct.
        4. `total_count = 8`, `count = 8`, `max_fit = 10`, but DB grew to 12 between count and find
           is impossible here (count ≥ rows by construction); clamp covers the reverse skew.
    - [x] **Verification:** State which of the four cases you walked and that the arithmetic matches.
      **Expected Result:** Case 1 demonstrates the freeze is gone.

---

## Phase 4: Build and Interactive Verification

- [x] **Step 4.1 (Verification — Build):** Run `make`. **Expected Result:** all three binaries
  rebuild, zero warnings.
- [x] **Step 4.2 (Verification — Live API, optional):** With ≥ 21 ranked configurations in the DB:
    ```bash
    curl -s localhost:8000/api/leaderboard | jq '{rows:(.leaderboard|length), total_count}'
    ```
    **Expected Result:** `rows == 20`, `total_count > 20`.
- [ ] **Step 4.3 (Verification — Interactive Test, Developer):** Start the app, press `[K]`, wait for
  the "GLOBAL LEADERBOARD" view. Report:
    1. When more configurations exist than fit on screen, does the bottom show `"+ N more"` with a
       plausible `N` (not stuck at 11)?
    2. After more configurations are submitted **and ranked** (allow a worker cycle), does `N` grow
       on the next refresh?
    3. With few configurations (all fit on screen), is the `"+ N more"` line correctly *absent*?
    4. Does the table layout / footer panel stay intact across window sizes (geometric row cap
       unchanged)?

---

## Phase 5: Documentation

- [x] **Step 5.1:** ADR-0030 created (done).
- [x] **Step 5.2:** Add a cross-reference note to `DEV_TASKS-0025` pointing at this task as the
  follow-up that corrected the overflow-count semantics.
- [x] **Step 5.3:** Update `docs/CHANGELOG.md` with a one-line entry under the current epoch:
  *"Fix: Global Leaderboard `+N more` now reports the true number of ranked configurations
  (ADR-0030) instead of freezing at the 20-row cap."*
