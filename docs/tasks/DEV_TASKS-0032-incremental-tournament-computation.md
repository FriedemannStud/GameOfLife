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

- [ ] **Step 2.1: Define the `match_results` cache contract**
    - [ ] **Action:** In `backend/app/database.py`, document the new `match_results` collection and ensure an index on `pair_key` (unique). Document the schema in a comment: `pair_key` (`"<min_hash>:<max_hash>"`), `hash_a`, `hash_b`, `winner` (`"a"|"b"|"draw"`), `pop_a`, `pop_b`, `activity_sum`, `stable_at_generation`, `computed_at`.
    - [ ] **Action:** Also document the `worker_state` singleton collection introduced in Phase 1.
    - [ ] **Verification:**
        1.  Run: `python -c "import backend.app.database as d; print('ok')"` (or the project's standard import check).
        2.  **Expected Result:** Imports cleanly; the unique index on `pair_key` is created on startup (confirm via Mongo: index list contains `pair_key_1`).

- [ ] **Step 2.2: Add the canonical pair-key helper + tests**
    - [ ] **Action:** In `backend/app/worker.py`, add `pair_key(h_a, h_b) -> str` returning `f"{min(h_a,h_b)}:{max(h_a,h_b)}"`. Mark `# KI-Agent unterstützt`.
    - [ ] **Verification:**
        1.  Extend `backend/tests/test_epoch_guard.py` (or a new `test_match_cache.py`) to assert `pair_key(a,b) == pair_key(b,a)` and that the self-pair `pair_key(a,a)` is well-formed.
        2.  Run the test file.
        3.  **Expected Result:** All assertions pass.

---

## Phase 3: Stage 2 — C Hyper-Worker Explicit-Pairings Mode

*Goal: Let the C worker compute only a supplied list of pairings and emit per-pair results, while staying backwards compatible when no pairings are given.*

- [ ] **Step 3.1: Extend the batch parser to accept an optional `pairings` array**
    - [ ] **Action:** In `src/io/file_io.c` (`parse_batch_file`), parse an optional top-level `pairings` array of `[idx_a, idx_b]` integer pairs into a new out-parameter (e.g. `int (*pairings)[2]` + count). When absent, signal "full round-robin" (count = -1) so existing callers are unaffected. Mark `// KI-Agent unterstützt`.
    - [ ] **Action:** Recompile: `make`.
    - [ ] **Verification:**
        1.  `make` completes with **zero warnings**.
        2.  Run an existing batch (full round-robin, no `pairings`) through `./build/biotope_hyper_worker <in.json> <out.json>` and confirm output is byte-identical to before the change (same rankings).
        3.  **Expected Result:** Backwards-compatible; full-mode output unchanged.

- [ ] **Step 3.2: Compute only supplied pairings and emit per-pair results**
    - [ ] **Action:** In `src/apps/hyper/main_hyper.c`, when `pairings` is present, replace the implicit `for (i) for (j>i)` loop (`main_hyper.c:67-69`) with a loop over the supplied pairs, calling `run_isolated_match` for each. Keep the OpenMP parallelisation.
    - [ ] **Action:** In `src/io/file_io.c` (`save_batch_results` or a new sibling), add a `match_results` array to the output JSON: for each computed pair emit `{hash_a, hash_b, winner, pop_a, pop_b, activity_sum, stable_at_generation}`. Derive `hash_a/hash_b` from the competitor seeds (reuse `grid_to_bitboard` as the content key, or pass the Python-side hashes through the input and echo them back — choose one and document it). Mark `// KI-Agent unterstützt`.
    - [ ] **Action:** Recompile: `make`.
    - [ ] **Verification:**
        1.  `make` completes with zero warnings.
        2.  Craft a tiny 3-competitor batch with `pairings: [[0,1],[0,2]]` and run the worker.
        3.  **Expected Result:** Output contains exactly two `match_results` entries; their outcomes match the corresponding pairs from a full-round-robin run of the same three competitors.

- [ ] **Step 3.3: C unit test for explicit-pairings mode**
    - [ ] **Action:** Add `tests/test_hyper_pairings.c` exercising `parse_batch_file` with and without `pairings`, asserting the parsed pair list and count.
    - [ ] **Action:** Compile and run per the project's C test convention (gcc with the relevant sources + `-Isrc/...`).
    - [ ] **Verification:**
        1.  The test binary runs and prints all assertions passed.
        2.  **Expected Result:** Exit code 0, no failures.

---

## Phase 4: Stage 2 — Python Delta Orchestration

*Goal: Compute only missing pairs each epoch, cache them, and aggregate the full ranking + highlights from the cache.*

- [ ] **Step 4.1: Build the missing-pairs set from the cache**
    - [ ] **Action:** In `execute_epoch()`, after computing each submission's `seed_hash`, enumerate all unordered competitor pairs and their `pair_key`s, query `match_results` for those keys, and build the list of **uncached** pairs (mapping back to competitor indices for the batch). Mark `# KI-Agent unterstützt`.
    - [ ] **Verification:**
        1.  Add a unit test (`test_match_cache.py`) with a fake/stubbed cache asserting: empty cache → all pairs missing; fully populated cache → empty missing set; one new competitor among N → exactly N missing pairs.
        2.  Run the test file.
        3.  **Expected Result:** All assertions pass.

- [ ] **Step 4.2: Invoke the C worker on missing pairs and upsert results**
    - [ ] **Action:** Build the batch with the `pairings` array of uncached index pairs, invoke `biotope_hyper_worker`, parse the `match_results` output, and upsert each into `db.match_results` keyed by `pair_key`. Skip the C call entirely when the missing set is empty. Mark `# KI-Agent unterstützt`.
    - [ ] **Verification:**
        1.  Run the worker against a roster of 3 active submissions on an empty cache; then add a 4th and watch the next epoch.
        2.  **Expected Result:** First epoch computes 3 pairs; after adding the 4th, the next epoch computes exactly 3 new pairs (4 vs each existing), and `match_results` now holds 6 documents.

- [ ] **Step 4.3: Aggregate ranking + highlights from the cache**
    - [ ] **Action:** Replace the C-`rankings`-driven update (`worker.py:240-262`) with a Python aggregation over the cached results for the **currently active** set: per player accumulate wins/draws/losses, `total_score`, `matches_played`, and `sum_stable_gen`; derive `win_rate` and `avg_stable_generation`. Write the same fields to `submissions` and `players` as today.
    - [ ] **Action:** Derive highlight candidates (top `activity_sum`) from the cached results for the active set, then run the existing `filter_highlights_by_oscillation(...)` (ADR-0023) unchanged before storing.
    - [ ] **Action:** Add the **determinism tripwire** comment at the cache read/write boundary referencing ADR-0032 and ADR-0026. Mark new blocks `# KI-Agent unterstützt`.
    - [ ] **Verification:**
        1.  Run `python backend/tests/test_ranking.py` and `python backend/tests/test_oscillator_detection.py`.
        2.  Compare the cached-aggregation ranking for a fixed roster against a one-shot full round-robin (status-quo path) for the same roster.
        3.  **Expected Result:** Existing tests pass; the cached ranking is **identical** to the full-recompute ranking (positions, win_rate, total_score, avg_stable_generation).

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

- [ ] **Step 5.3: Finalise documentation**
    - [ ] **Action:** Update `docs/CHANGELOG.md` with the Stage 2 entry; flip ADR-0032 **Status** from `proposed` to `accepted`.
    - [ ] **Action:** (Optional) add a short note in `CLAUDE.md`'s Backend section that the worker is now incremental and cache-backed.
    - [ ] **Verification:** `git diff` shows the changelog entry and the ADR status change; the build is clean and all tests pass.
