### **ADR-0034: Tournament Performance Monitoring (`performance_metrics`)**

**Status:** accepted

**Date:** 2026-06-19

#### **1. Context and Problem Statement**

During the university fair, visitors continuously submit 8×8 start patterns that
enter the round-robin tournament run by the C hyper-worker
(`src/apps/hyper/main_hyper.c`, invoked from `backend/app/worker.py`). The
tournament is **O(N²)** in the number of competitors: every pattern plays every
other pattern, and each match simulates up to `max_generations` (currently 1000)
generations.

The dominant performance risk is not the number of patterns alone but
**pathological patterns that never stabilise**: a pattern that keeps changing
forces *every* match it participates in to run to the full generation cap instead
of terminating early. A single such crowd-submitted pattern can disproportionately
inflate epoch wall-clock time, and today we have no online visibility into this.

We need a way to (a) watch tournament throughput live during the fair and spot a
degradation immediately, and (b) keep a queryable history for later analysis
(e.g. comparing the human `biotope_db` dataset against the synthetic
`biotope_study` dataset). A controlled scaling experiment was run by filling
`biotope_study` with thousands of random patterns and measuring how the
hyper-worker scales.

#### **2. Decision**

Introduce a dedicated MongoDB collection **`performance_metrics`** plus tooling
around it:

*   **`backend/scripts/perf_tournament.py`** runs the hyper-worker over all active
    submissions of the *currently selected* database, measures wall-clock and CPU
    time, and writes **one metrics document per run** into `performance_metrics`.
    Key fields: `n_competitors`, `total_matches_played`, `wall_clock_s`,
    `cpu_time_s`, `parallel_speedup`, `matches_per_s`, `us_per_match`, the
    stable-generation distribution (`stable_gen_p50/p95/max`), the perf-risk
    indicators `n_never_stabilized` / `pct_never_stabilized`, and a
    `top_expensive_configs` list (longest-living patterns). It also writes the
    rankings back into `submissions` (same fields as `worker.execute_epoch`).
*   **`GET /api/performance`** (`backend/app/main.py`) serves the most recent
    metrics documents (newest first) as `{ latest, runs, count }`.
*   **`web/dashboard/performance.html`** is a standalone live dashboard that polls
    the endpoint every 10 s and renders the KPIs, the most expensive patterns, and
    the run history. It doubles as the fair "ops screen".
*   **`backend/scripts/generate_study_configs.py`** reproduces the (uncommitted)
    `exhaust_engine` schema to populate `biotope_study` for scaling experiments.

The metrics collection is deliberately *append-only* and decoupled from the live
match pipeline, so monitoring never interferes with the tournament itself.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

*   Live, glanceable insight into tournament health during the fair; a
    never-stabilising pattern shows up immediately as a rising
    `pct_never_stabilized` and in the "most expensive" table.
*   A durable, queryable performance history for post-hoc analysis and for the
    poster's F2.3 data story (human vs. random datasets).
*   Reusable on any dataset: the tool follows the active `MONGODB_DB`, so the same
    code measures `biotope_db` (live) and `biotope_study` (experiment).

**Negative Consequences (Disadvantages):**

*   `perf_tournament.py` re-runs the full O(N²) round-robin; at 10 000 competitors
    this is ~100 M matches and roughly an hour of wall-clock on a 20-core host, so
    it is a deliberate/triggered run, not a per-minute loop.
*   `performance_metrics` grows unbounded unless capped/TTL-indexed later.
*   The dashboard reads aggregated metrics only; it does not stream per-match data.

#### **4. Alternatives Considered**

*   **Reuse `epoch_highlights`.** Rejected: highlights are about *interesting
    matches*, not performance, and mixing concerns would muddy both.
*   **Log-only / Prometheus exporter.** Overkill for a fair laptop and would add an
    operational dependency; a MongoDB collection reuses the existing stack and is
    trivially queryable for later analysis.
*   **Instrument the existing 60 s `worker_loop`.** Rejected for the scaling
    experiment because the loop caps at 1000 competitors (`to_list(length=1000)`)
    and is tuned for steady-state operation, not on-demand large benchmarks.
