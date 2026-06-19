# DEV_TASKS-0032: Incremental Tournament Computation via a Deterministic Match Cache

This plan implements the two-stage incremental tournament strategy from
ADR-0032. **Stage 1** (Phase 1) is a self-contained, low-risk early-exit guard
that is safe to ship on its own. **Stage 2** (Phases 2–5) layers a deterministic
match cache on top and is gated on field size — it can be deferred without
touching Stage 1.

**Developer:** Please follow these steps precisely. The plan is broken into
phases and small steps to allow for interruptions and ensure stability. After
each "Verification" step, report the outcome. Recompile the C with `make` (zero
warnings required) after every C change; Python backend tests are self-running
(`python backend/tests/test_*.py`).

**Briefing Documents:**
*   [ADR-0032: Incremental Tournament Computation via a Deterministic Match Cache](../adr/ADR-0032-incremental-tournament-computation.md)
*   DEV_SPEC / DEV_TECH_DESIGN: intentionally skipped for this increment (decision detail lives in ADR-0032).
*   Related: [ADR-0026: Single match per pair (torus symmetry)](../adr/ADR-0026-single-match-per-pair.md), [ADR-0023: Oscillator detection](../adr/ADR-0023-oscillator-detection.md)

---

## Phase 1: Stage 1 — Roster-Change Early-Exit Guard (Python only)

*Goal: Skip an epoch entirely when the active roster is unchanged since the last successful epoch. No C changes, no change to ranking numbers.*

- [x] **Step 1.1: Add the seed-hash + roster-fingerprint helpers**
    - [x] **Action:** In `backend/app/worker.py`, add a `seed_hash(cells) -> str` helper: SHA-256 (hex, first 16 chars) over a canonical JSON serialisation of the 8×8 `cells` (e.g. `json.dumps(cells, separators=(",",":"), sort_keys=False)`). Mark the block `# KI-Agent unterstützt`.
    - [x] **Action:** Add `roster_fingerprint(submissions) -> str`: sort the list of `f"{str(s['_id'])}:{seed_hash(s['config']['cells'])}"`, join, and SHA-256 the result. This captures additions, removals, deactivations, and edits.
    - [x] **Verification:**
        1.  Create `backend/tests/test_epoch_guard.py` with a small self-running test that asserts: identical rosters → equal fingerprint; reordered rosters → equal fingerprint; one edited cell → different fingerprint; one added submission → different fingerprint.
        2.  Run: `python backend/tests/test_epoch_guard.py`  *(locally lacks `bson`/`motor`; verified via a sys.modules stub harness — all 7 assertions pass)*
        3.  **Expected Result:** All assertions pass, process exits 0. ✅

- [x] **Step 1.2: Persist the last-epoch fingerprint and early-exit on no change**
    - [x] **Action:** In `execute_epoch()` (`backend/app/worker.py`), after loading active submissions (the `db.submissions.find({"status": "active"})` call) and before building the batch, compute `current_fp = roster_fingerprint(submissions)`.
    - [x] **Action:** Read the stored fingerprint from a new singleton doc `db.worker_state.find_one({"_id": "epoch_guard"})`. If it equals `current_fp`, `logger.info("Roster unchanged since last epoch — skipping.")` and `return` (no batch, no C call, no DB writes).
    - [x] **Action:** At the **end** of a successful epoch (after the DB ranking updates), upsert `db.worker_state.update_one({"_id": "epoch_guard"}, {"$set": {"fingerprint": current_fp, "updated_at": datetime.utcnow()}}, upsert=True)`. Only write on success so a failed epoch retries next tick. Mark all new blocks `# KI-Agent unterstützt`.
    - [ ] **Verification:**
        1.  Ensure binaries exist (`make`) and the stack is up (`docker-compose up`), or run the worker standalone against the local Mongo.
        2.  Observe the worker log across at least three 60 s ticks with **no** new submissions.
        3.  **Expected Result:** The first tick runs a full epoch ("Epoch finished. Matches played: …"); subsequent ticks log "Roster unchanged since last epoch — skipping." and perform no match computation.

- [ ] **Step 1.3: Confirm a roster change re-triggers a full epoch**
    - [ ] **Action:** None (behavioural check of Step 1.2).
    - [ ] **Verification:**
        1.  With the worker running and idle (skipping), submit one new pattern via the web editor (`web/editor/editor.html`) or insert an active submission directly.
        2.  Watch the next worker tick.
        3.  **Expected Result:** The tick detects the changed fingerprint, runs a full epoch, updates the ranking, and then returns to "skipping" on following idle ticks. Stage 1 is now complete and shippable.

- [x] **Step 1.4: Update the changelog for Stage 1**
    - [x] **Action:** Add an entry to `docs/CHANGELOG.md` describing the Stage 1 early-exit guard (ADR-0032).
    - [x] **Verification:** `git diff docs/CHANGELOG.md` shows the new entry. ✅

---

## Phase 2: Stage 2 — Match-Cache Data Layer

*Goal: Introduce the content-addressed match cache (collection + indexes) without yet wiring it into the epoch.*

- [x] **Step 2.1: Define the `match_results` cache contract**
    - [x] **Action:** In `backend/app/database.py`, document the new `match_results` collection (schema in comment: `pair_key`, `hash_a`, `hash_b`, `winner`, `pop_a`, `pop_b`, `activity_sum`, `stable_at_generation`, `computed_at`) and add the `match_results_col` handle. The unique index on `pair_key` is created at startup via a new `_ensure_match_cache_indexes` hook in `main.py` (matching the project's existing index-on-startup convention rather than creating it in `database.py`).
    - [x] **Action:** Also document the `worker_state` singleton collection introduced in Phase 1 and add the `worker_state_col` handle.
    - [ ] **Verification (Interactive — needs live stack):**
        1.  `docker-compose up`, watch the backend startup logs ("Successfully connected to MongoDB!").
        2.  In Mongo, list indexes on `match_results`.
        3.  **Expected Result:** the collection's index list contains `pair_key_1` (unique). *(Code complete; runtime check pending developer.)*

- [x] **Step 2.2: Add the canonical pair-key helper + tests**
    - [x] **Action:** In `backend/app/worker.py`, add `pair_key(h_a, h_b) -> str` returning the canonical `"<lo>:<hi>"` form. Mark `# KI-Agent unterstützt`.
    - [x] **Verification:**
        1.  Extended `backend/tests/test_epoch_guard.py` to assert `pair_key(a,b) == pair_key(b,a)`, the canonical ordering, and that the self-pair `pair_key(a,a)` is well-formed.
        2.  Ran the test file (stub harness, no `bson`/`motor` locally).
        3.  **Expected Result:** All assertions pass. ✅

---

## Phase 3: Stage 2 — C Hyper-Worker Explicit-Pairings Mode

*Goal: Let the C worker compute only a supplied list of pairings and emit per-pair results, while staying backwards compatible when no pairings are given.*

- [x] **Step 3.1: Extend the batch parser to accept an optional `pairings` array**
    - [x] **Action:** In `src/io/file_io.c` (`parse_batch_file`), parse an optional top-level `pairings` array of `[idx_a, idx_b]` integer pairs. New struct `Pairing {int a, b;}` in `core_types.h`; out-params `Pairing** pairings, int* pairing_count`. When absent, `*pairings = NULL` and `*pairing_count = -1` (full round-robin). Out-of-range indices are skipped defensively. `// KI-Agent unterstützt`.
    - [x] **Action:** Recompile: `make`.
    - [x] **Verification:**
        1.  `make` completes with **zero warnings**. ✅
        2.  Ran a 3-competitor full-round-robin batch; output keys are `total_matches_played, execution_time_cpu_s, rankings, highlights` — **no `match_results`** key, so full-mode output is unchanged.
        3.  **Expected Result:** Backwards-compatible; full-mode output unchanged. ✅

- [x] **Step 3.2: Compute only supplied pairings and emit per-pair results**
    - [x] **Action:** In `src/apps/hyper/main_hyper.c`, factored the per-match scoring+highlight body into `play_pairing(...)`; in pairings mode it loops over the supplied pairs (OpenMP retained), full mode keeps the `i<j` nested loop.
    - [x] **Action:** In `src/io/file_io.c` (`save_batch_results`), emit a `match_results` array of `{idx_a, idx_b, winner ("a"|"b"|"draw"), pop_a, pop_b, activity_sum, stable_at_generation}` in pairings mode only. **Decision:** results are keyed by **competitor index**, not seed hash — Python (which owns `seed_hash`) maps indices → `pair_key`. This keeps all hashing in Python and avoids coupling C to the JSON-canonicalisation scheme. New struct `PairOutcome` in `core_types.h`. `// KI-Agent unterstützt`.
    - [x] **Action:** Recompile: `make`.
    - [x] **Verification:**
        1.  `make` completes with zero warnings. ✅
        2.  3-competitor batch with `pairings: [[0,1],[0,2]]` → output has exactly two `match_results` entries.
        3.  **Expected Result:** Outcomes match the full round-robin: aggregating an all-pairs `[[0,1],[0,2],[1,2]]` run reproduces the full-mode rankings **exactly** (score/W/D/L identical for all three players). ✅

- [x] **Step 3.3: C unit test for explicit-pairings mode**
    - [x] **Action:** Added `tests/test_hyper_pairings.c` exercising `parse_batch_file` with pairings, without pairings (→ count -1, NULL), and with out-of-range indices (→ skipped).
    - [x] **Action:** Compile: `gcc -Wall -Wextra -std=c99 -fopenmp tests/test_hyper_pairings.c src/io/file_io.c src/core/game_logic.c src/vendor/cJSON/cJSON.c -Isrc/core -Isrc/io -Isrc/gui -Isrc/vendor/cJSON -Isrc -o tests/test_hyper_pairings -lm`.
    - [x] **Verification:**
        1.  Binary runs, all 3 tests print PASSED. ✅
        2.  **Expected Result:** Exit code 0, no failures. ✅

---

## Phase 4: Stage 2 — Python Delta Orchestration

*Goal: Compute only missing pairs each epoch, cache them, and aggregate the full ranking + highlights from the cache.*

- [x] **Step 4.1: Build the missing-pairs set from the cache**
    - [x] **Action:** Extracted a pure helper `enumerate_needed_pairs(hashes)` (pair_key → representative indices, dedups identical patterns) used by `execute_epoch()`; the cache `find({"pair_key": {"$in": ...}})` and the `missing` delta are computed inline. `# KI-Agent unterstützt`.
    - [x] **Verification:**
        1.  Added `backend/tests/test_match_cache.py`: empty cache → all missing; full cache → none missing; one new competitor among N → exactly N missing; duplicate pattern dedups; plus `cells_to_seed64` layout tests.
        2.  Ran the test file (stub harness).
        3.  **Expected Result:** All assertions pass. ✅

- [x] **Step 4.2: Invoke the C worker on missing pairs and upsert results**
    - [x] **Action:** `execute_epoch` builds the batch with a `pairings` array (canonical hash order), calls the extracted `run_hyper_worker(...)`, parses `match_results`, and `bulk_write`s `UpdateOne(..., upsert=True)` into `db.match_results` keyed by `pair_key`. The C call is skipped when the missing set is empty ("All pairs cached — aggregating from cache only."). `# KI-Agent unterstützt`.
    - [ ] **Verification (Interactive — needs live stack):**
        1.  Run the worker against 3 active submissions on an empty cache; then add a 4th and watch the next epoch.
        2.  **Expected Result:** first epoch computes 3 pairs; after adding the 4th, the next epoch computes exactly 3 new pairs and `match_results` holds 6 docs. *(Logic pre-validated autonomously; runtime check pending developer.)*

- [x] **Step 4.3: Aggregate ranking + highlights from the cache**
    - [x] **Action:** Replaced the C-`rankings`-driven update with a Python aggregation over the cached results for the active set (wins/draws/losses, `total_score`, `matches_played`, `sum_stable_gen` → `win_rate`, `avg_stable_generation`); writes the same fields to `submissions` and `players`. Deterministic rank sort (score, wins, id).
    - [x] **Action:** Highlight candidates (top `activity_sum`) are derived from the cache; seeds reconstructed via `cells_to_seed64` (mirrors `grid_to_bitboard`); `filter_highlights_by_oscillation(...)` (ADR-0023) applied unchanged before storing.
    - [x] **Action:** Added the **determinism tripwire** comment at the cache boundary referencing ADR-0032 and ADR-0026. `# KI-Agent unterstützt`.
    - [x] **Verification (autonomous logic check):**
        1.  `test_match_cache.py` passes. (`test_ranking.py` / `test_oscillator_detection.py` need `bson`/`motor` → run in container.)
        2.  Replicated `execute_epoch`'s aggregation against a **C full round-robin** for a 6-competitor roster including a duplicated pattern: the delta path computed 11 distinct pairs (vs 15) and reproduced the full-mode ranking **exactly** (score/W/D/L/avg_stable for all 6, incl. the duplicate pair). ✅
        3.  Live DB compare pending developer (interactive).

---

## Phase 5: Stage 2 — Integration, Edge Cases & Documentation

*Goal: Verify lifecycle edge cases and finalise docs.*

- [ ] **Step 5.1: Edge-case verification (edits, duplicates, deactivation)**
    - [ ] **Action:** None (behavioural checks).
    - [ ] **Verification:**
        1.  **Edit:** change one player's pattern → confirm only that player's pairs are recomputed (new `seed_hash` → new `pair_key`s) and the ranking updates.
        2.  **Duplicate:** submit two identical patterns → confirm their mutual pair and shared opponents reuse a single cached result per `pair_key`.
        3.  **Deactivation:** set one submission `status != "active"` → confirm no new matches are computed and the player drops out of the ranking, cache untouched.
        4.  **Expected Result:** All three behave as described.

- [ ] **Step 5.2: End-to-end integration test**
    - [ ] **Action:** Ensure binaries are built (`make`).
    - [ ] **Action:** Run `python backend/tests/test_system_integration.py`.
    - [ ] **Verification:**
        1.  The integration test (which invokes the C binaries) passes.
        2.  **Expected Result:** Exit code 0; rankings populated end-to-end.

- [x] **Step 5.3: Finalise documentation**
    - [x] **Action:** Updated `docs/CHANGELOG.md` with the Stage 2 entry; flipped ADR-0032 **Status** `proposed` → `accepted`.
    - [x] **Action:** Updated `CLAUDE.md`'s Backend section — worker is now incremental/cache-backed; documented `match_results`/`worker_state` collections and the C `pairings` mode.
    - [x] **Verification:** `git diff` shows the changelog entry, ADR status change, and CLAUDE.md update; build is clean and all autonomous tests pass. ✅
