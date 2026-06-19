### **ADR-0032: Incremental Tournament Computation via a Deterministic Match Cache**

**Status:** accepted

**Date:** 2026-06-19

#### **1. Context and Problem Statement**

The tournament worker (`backend/app/worker.py`) recomputes the **entire**
ranking from scratch on a fixed 60 s cycle:

*   `worker_loop()` calls `execute_epoch()` every 60 s
    (`worker.py:300-304`).
*   `execute_epoch()` loads **all** active submissions and packs **all** of them
    into a fresh batch on every tick (`worker.py:139-154`).
*   The C hyper-worker then plays a full **O(N²) round-robin** — every unordered
    pair `(i, j)` exactly once (`src/apps/hyper/main_hyper.c:67-69`).
*   The result wholesale-replaces the previous ranking; the design is explicitly
    *epoch-fresh* with no historical state carried over
    (`worker.py:240-262`).

The key observation is that **a match is fully deterministic.**
`run_isolated_match()` (`src/core/game_logic.c:220-264`) places the two fixed
8×8 seeds (RED left, BLUE right) and runs the deterministic two-team Conway
rules. There is **no randomness on the match path** — the only `rand()` in the
file is in `init_world()` (`game_logic.c:44`), which is used solely for the
interactive GUI's random seeding and is never reached by the tournament. The
same observation underpins the symmetry proof in **ADR-0026**.

Consequently:

*   A match between seed A and seed B **always** yields the same winner, the same
    per-player populations, the same `stable_at_generation`, and the same
    `activity_sum`.
*   Re-running an unchanged roster produces a **bit-for-bit identical** ranking.
    The repeated full round-robin is therefore pure wasted work whenever the set
    of active submissions has not changed.

This wasted work scales quadratically and bounds how large the field can grow
within the 60 s budget:

| Active players (N) | Matches per full round-robin = N(N−1)/2 |
|--------------------|------------------------------------------|
| 50                 | 1 225                                    |
| 200                | 19 900                                   |
| 1 000              | 499 500                                  |

Each match simulates up to `max_generations = 1000` generations
(`worker.py:149`). At exhibition scale (≤ 50 players) the OpenMP-parallel full
round-robin still fits comfortably in 60 s, but the cost is paid on **every**
tick even when nothing changed, and the approach does not scale toward the
"up to 1000" submissions the loader already provisions for (`worker.py:139`).

Hard constraints that shape any solution:

*   **Determinism must be preserved** — the entire optimisation rests on it
    (see ADR-0026).
*   **Patterns may be re-submitted/edited**, so a cache must invalidate cleanly
    when a player's seed content changes.
*   **Submissions may be deactivated** (`status != "active"`); this must not force
    a recomputation.
*   **Zero-warning C build** (`-Wall -Wextra -std=c99`), the
    `create_world`/`free_world` and no-hot-loop-allocation conventions, and the
    `// KI-Agent unterstützt` attribution rule (`docs/CODING_STYLE.md`).

#### **2. Decision**

Adopt **incremental tournament computation**, exploiting match determinism, and
roll it out in **two stages** so the high-value/low-risk win lands first.

##### Stage 1 — Roster-change early-exit guard (do now)

Before building any batch, `execute_epoch()` computes a cheap **roster
fingerprint** of the current active set and compares it to the fingerprint of
the last *successful* epoch. If unchanged, the epoch is skipped entirely — no
batch, no C invocation, no DB writes.

*   **Fingerprint definition:** a stable hash over the sorted list of
    `(submission_id, seed_hash)` pairs of all active submissions, where
    `seed_hash` is a content hash of `config.cells` (see Stage 2). This detects
    additions, removals, deactivations, **and** edits (a changed pattern changes
    its `seed_hash`).
*   **Persistence:** the last fingerprint is stored in a small singleton document
    in a new `worker_state` collection (`_id: "epoch_guard"`), updated only after
    a successful epoch.

This alone eliminates ~95 % of the redundant load in the steady state (no new
players between ticks) for a one-line-of-logic guard, with **zero change** to the
C worker or the ranking semantics.

##### Stage 2 — Deterministic match cache with delta computation (scale path)

Persist every computed match result and, on each epoch, simulate **only the
pairs that are not yet cached** — i.e. the pairs that involve at least one
newly added (or edited) seed.

1.  **Seed hashing (single source of truth).**
    A helper `seed_hash(cells) -> str` returns a SHA-256 (hex, truncated) over a
    canonical serialisation of the 8×8 `cells`. Keying the cache by seed content
    (not by `player_id`) gives two properties for free:
    *   **De-duplication:** two players submitting identical patterns share one
        cached result.
    *   **Edit invalidation:** an edited pattern hashes differently, so its old
        cached matches are simply never read again (no manual invalidation).

2.  **Match-result cache (`match_results` collection).**
    Each document is keyed by the **unordered** pair of seed hashes
    `pair_key = "<min(h_a,h_b)>:<max(h_a,h_b)>"` and stores the outcome in terms
    of the two seeds: winner (`a` / `b` / `draw`), each seed's final population,
    `activity_sum`, and `stable_at_generation`.
    Using an *unordered* key is justified by **ADR-0026**: because
    `T = shift-8-cols ∘ swap(RED↔BLUE)` is an exact symmetry of the update rule,
    the orientation in which a pair is played (which seed is RED/left) does not
    change the winning seed or the per-seed populations. The result is therefore
    a well-defined function of the unordered pair `{h_a, h_b}` (the degenerate
    `h_a == h_b` mirror match — two identical patterns — is a valid, cacheable
    self-pair).

3.  **C hyper-worker: explicit-pairings input mode.**
    `main_hyper.c` / `parse_batch_file` gain an optional `pairings` array in the
    batch JSON listing the index pairs to simulate. When present, the worker runs
    only those pairings (instead of the implicit full `i<j` loop) and emits a
    **per-pair result list** (both seed hashes + outcome fields) rather than only
    aggregated `rankings`. When `pairings` is absent, behaviour is unchanged
    (backwards compatible). All new/modified blocks carry `// KI-Agent unterstützt`.

4.  **Python delta orchestration.** `execute_epoch()` becomes:
    1.  Load active submissions, compute each `seed_hash`.
    2.  Enumerate all unordered pairs required for the full ranking.
    3.  Look up `match_results`; build the **missing-pairs** set.
    4.  Invoke the C worker on the missing pairs only; upsert results into
        `match_results`.
    5.  **Aggregate the full ranking in Python** from the cache (pure addition:
        wins/draws/losses, `total_score`, `matches_played`, `sum_stable_gen` →
        `avg_stable_generation`). This O(N²) summation is arithmetic only and is
        negligible next to the avoided simulations.
    6.  Derive highlights (top `activity_sum`) from the cache and apply the
        existing oscillation filter (`filter_highlights_by_oscillation`,
        ADR-0023) unchanged.

5.  **Determinism tripwire.** A prominent comment at the cache boundary (and a
    note in this ADR) records that the cache is valid **only** while matches are
    deterministic and the ADR-0026 orientation symmetry holds. If randomness or a
    topology/birth-rule change is ever introduced, the cache must be invalidated
    (drop `match_results`) and this decision re-evaluated.

Stage 1 is intended for immediate adoption. Stage 2 is fully designed here but
its rollout is gated on field size / the 60 s budget actually being approached;
it can be implemented later without revisiting Stage 1.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

*   **Stage 1** removes essentially all redundant computation in the common
    "nothing changed" steady state at near-zero cost and risk, and changes no
    ranking numbers.
*   **Stage 2** turns the dominant cost (simulation) incremental: adding one
    player to a field of N costs ~N new matches instead of N(N−1)/2 (e.g. 49 vs
    1 225 at N = 50; 999 vs 499 500 at N = 1 000), making the system scale toward
    the provisioned 1 000-submission ceiling.
*   Content-addressed keys give edit-invalidation and identical-pattern
    de-duplication for free.
*   Highlights and the oscillation filter are reused unchanged; they become
    derivable from the cache without re-simulation.
*   Deactivating a player needs no recomputation — it is simply excluded from the
    aggregation; the cache stays valid.

**Negative Consequences (Disadvantages):**

*   Stage 2 adds real moving parts: two new collections (`worker_state`,
    `match_results`), a seed-hashing contract, and a new C input/output mode —
    more code and more tests to maintain than today's stateless full recompute.
*   The optimisation is **load-bearing on determinism**. A future non-deterministic
    rule change would silently produce stale results unless the tripwire is
    honoured and the cache dropped.
*   `match_results` grows with the number of distinct seed pairs ever seen; it may
    need periodic pruning of pairs whose seeds are no longer active.
*   Moving ranking aggregation from C into Python (Stage 2) shifts a (cheap) part
    of the pipeline across the language boundary, a non-trivial refactor of
    `execute_epoch()` and its tests.

#### **4. Alternatives Considered**

*   **Keep full O(N²) recompute every 60 s (status quo).** Simplest and
    stateless, and adequate at ≤ 50 players. Rejected as the default because it
    burns the full quadratic cost on every tick even when nothing changed and
    does not scale toward the provisioned ceiling. (Stage 1 is the minimal fix
    that keeps the status quo's simplicity for the changed-roster case.)

*   **Stage 1 only (early-exit), never cache matches.** Eliminates the
    steady-state waste but still pays full O(N²) whenever *any* submission
    changes — adding the 51st player still replays all 1 225 pairs. Acceptable at
    exhibition scale; insufficient for large fields. Adopted as Stage 1, with
    Stage 2 layered on top when needed.

*   **Persisted Elo / incremental rating instead of a match cache.** Carry rating
    across epochs and only play the new entrant against a sample of opponents.
    Smaller storage, but it abandons the exact, reproducible round-robin ranking
    the project currently guarantees and introduces ordering/sampling bias —
    contrary to the deterministic, full-table design. Rejected.

*   **Key the cache by `player_id` instead of seed content.** Simpler to reason
    about per player, but requires explicit invalidation on every edit and misses
    identical-pattern de-duplication. Content hashing handles both cases with no
    extra bookkeeping. Rejected.

*   **Run incremental matches in Python (NumPy `_step_numpy` already exists).**
    The oscillation detector already simulates in NumPy (`worker.py:41-100`), so
    matches could be computed there, avoiding any C change. Rejected for the hot
    path: the C engine is far faster and is the project's source of truth for
    match outcomes; duplicating the rules in Python risks divergence from
    `game_logic.c`.
