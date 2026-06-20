# LLM Session Log — Poster Analytics & Tournament Performance Monitoring

**Date:** 2026-06-20
**Branch:** `biotop`
**Scope:** Conference-poster content for the uni fair, a data analysis of winning Game-of-Life start patterns, a new tournament **performance-monitoring** feature (ADR-0034), a scaling study to 10 000 patterns, and the Git/CI cleanup needed to push it all.

This document is a structured summary so a developer can reconstruct and continue the work.

---

## 1. Poster Content (uni fair)

Goal: propose "research questions" for a conference-style poster (audience: lay visitors + university members), then flesh out the chosen ones.

- **Proposal list:** `docs/Präsentation/260619_Poster_Forschungsfragen_Vorschlaege.md` — questions in three directions: (1) programming/"handmade C → distributed system", (2) Game of Life itself, (3) extra angles (Elo, audience interaction, parallelism, ADR process, ecosystem metaphor).
- **Chosen three** and worked into poster sections in `docs/Präsentation/260619_Poster_Sektionen_Ausarbeitung.md`:
  - **F2.1** (left, "Fascination"): emergence from 4 rules + live kiosk.
  - **F3.2** (centre, "Participate"): visitor draws an 8×8 pattern in the web editor → enters the live tournament → appears on the leaderboard. The poster's unique magnet.
  - **F1.1** (right, "Origin story"): a 1st-semester student + AI built a full distributed system. Includes a metrics bar.
- **Real repo metrics** (counted, for the F1.1 bar): ~11 500 lines own code (C ~4 200 / Python ~1 700 / Web ~5 700; vendor cJSON ~3 500 excluded), 3 C binaries, 14 tests (5 C + 9 Python), 31 ADRs at the time.
- **Layout aid:** ASCII A0 mock-up + "open issues" list in the Ausarbeitung doc.

---

## 2. F2.3 Data Analysis — "Do some patterns almost always win?"

MongoDB runs locally via Docker Desktop, published on `127.0.0.1:27018`. Two databases:
- `biotope_db` — real, human-drawn patterns (29 at session start).
- `biotope_study` — synthetic random patterns (1 647 at session start; `league: "study"`, `player_id: "exhaust_engine"`, nicknames `exhaust#NNNNNNNN`).

### Tooling built (all in `backend/scripts/`, read `MONGODB_URI`/`MONGODB_DB` from repo-root `.env`, auto-rewrite docker host `mongo:27017` → `127.0.0.1:27018`)
- **`top_patterns.py [N]`** — prints the top-N winning start patterns as ASCII (same filter/sort as `GET /api/leaderboard`: `{status:"active", matches_played:{$gt:0}}`, sort `win_rate desc, avg_stable_generation asc`).
- **`pattern_heatmap.py [N]`** — emits print-ready **SVG** (vector, scales to A0): an aggregate win-rate-weighted occupancy heatmap and the top-N seed tiles. Output filenames are tagged by DB (`_db` / `_study`).

### Findings
- **Human data (29):** top patterns looked sparse → naive takeaway "fewer cells is better". The occupancy heatmap shows clear hot spots (humans place cells with structure).
- **Random data:** heatmap is near-uniform (no "good neighbourhoods"); the differentiator is *how many* cells, not *where*.
- Comparison written up in `docs/Präsentation/260619_F2.3_Datenvergleich_db_vs_study.md`.
- **Graphics:** `docs/Präsentation/260619_F2.3_heatmap_{db,study}.svg`, `260619_F2.3_top_seeds_{db,study}.svg`.

> NOTE: no SVG→PNG renderer in the env; SVGs are validated as well-formed XML and opened in the browser for the developer to eyeball.

---

## 3. The `exhaust_engine` Generator — Not in the Repo

The user asked to find the script that fills `biotope_study`. **It does not exist** anywhere — working tree, full git history (`-S exhaust_engine`/`cells_int`), all branches, stash. Only its data exists.

- **Faithful rebuild:** `backend/scripts/generate_study_configs.py` reproduces the document schema:
  `metadata.{player_id:"exhaust_engine", nickname:"exhaust#NNNNNNNN", league:"study"}`,
  `config.{bounding_box_x, bounding_box_y, cell_count, cells, cells_int, next_gen_cells_int}`,
  `status:"active"`, `elo_rating:1200`.
- **Critical discovery:** `biotope_study.submissions` has a **UNIQUE index on `config.cells_int`** (each 64-bit bitboard pattern may appear only once → "exhaust" = exhaustive/unique). The generator preloads existing bitboards and retries to avoid collisions.
- Usage: `python3 backend/scripts/generate_study_configs.py <target_total>` (fills *up to* the target).

---

## 4. Performance Monitoring Feature (ADR-0034)

**Motivation:** the round-robin is **O(N²)**; each match runs up to `max_generations` (**hard stop at 1000** — the gen-1000 board state decides the match, no timeout-draw). The dominant performance risk is a pattern that **never stabilises**, forcing every one of its matches to the cap. During the fair, humans keep submitting patterns, so this needs live visibility.

### Components
- **`backend/scripts/perf_tournament.py`** — runs `build/biotope_hyper_worker` over all active submissions of the current DB, measures wall-clock + child CPU, parses results, writes **one document per run** into the new **`performance_metrics`** collection, and writes rankings back into `submissions` (mirrors `worker.execute_epoch`). Metrics: `n_competitors`, `total_matches_played`, `wall_clock_s`, `cpu_time_s`, `parallel_speedup`, `matches_per_s`, `us_per_match`, `stable_gen_{mean,p50,p95,max}`, `n_never_stabilized`, `pct_never_stabilized`, `top_expensive_configs`.
- **`GET /api/performance?limit=N`** (`backend/app/main.py`) → `{ latest, runs, count }`.
- **`web/dashboard/performance.html`** — standalone live ops dashboard, polls the endpoint every 10 s; `?api=http://host:8000` override for cross-host use.
- **ADR:** `docs/adr/ADR-0034-tournament-performance-monitoring.md`. CHANGELOG updated.

> The collection is append-only and decoupled from the live match pipeline.
> Docker is unavailable in this WSL distro, so the hyper-worker is invoked **directly**, not via `docker-compose`. The 60 s `worker_loop` also caps at `to_list(length=1000)`, so it cannot run the large benchmarks.

---

## 5. Scaling Study (results)

Filled `biotope_study` 1 647 → 3 000 → 10 000 and ran the instrumented round-robin (20-core host, `max_gen=1000`).

| Metric | 3 000 | 10 000 |
|---|---|---|
| Matches | 8 997 000 | 99 990 000 |
| Wall-clock | 308 s (5.1 min) | 2 918 s (48.6 min) |
| CPU time | 5 997 s | 57 538 s |
| Parallel speedup | 19.5× | 19.7× |
| Throughput | 29 215 /s | 34 271 /s |
| µs / match | 34.2 | 29.2 |
| never-stabilized | 0 % | 0 % |

### Key results
- **Strategy law:** mean win-rate rises monotonically with starting cell count — ~26 % at 2 cells to ~64 % at 24 cells, crossing 50 % at ~10–11 cells. The 3 000 and 10 000 curves are nearly identical → it is a stable law, not noise. This *overturns* the small-sample human takeaway ("fewer is better"). Chart: `docs/Präsentation/260620_F2.3_winrate_vs_cells_study10k.svg` (3k version: `260619_F2.3_winrate_vs_cells_study.svg`).
- **Lifespan ≠ success:** the longest-living (most expensive) patterns are sparse (5–7 cells) and only ~50 % win-rate; winners are dense (22–24 cells).
- **Top win-rate shrinks with field size:** 76.8 % (29 humans) → 70.7 % (1 000) → 70.3 % (3 000) → 69.6 % (10 000).
- **Scaling is better than linear:** 11× more matches but only ~9.5× more wall-clock; per-match cost dropped (larger batches amortise better).
- **No perf killer in random data:** 0 % never-stabilized even at 100 M matches (all settled ≤ 708 gen).

### Poster write-ups
- `docs/Präsentation/260619_Poster_Auswertung_study_3000.md` (interim)
- `docs/Präsentation/260620_Poster_Auswertung_study_10000.md` (final) — Part A strategy, Part B performance.

> NOTE: `*_study.svg` heatmap/seeds were regenerated for 10 000 (3 000 versions preserved in commit `77be329`).

---

## 6. ADR Renumber

ADR for performance monitoring was first created as **ADR-0032**, then renumbered to **ADR-0034** (0032 = incremental tournament computation, 0033 = kiosk render budget were already taken on the remote). All references in `docs/` updated (file rename, title, CHANGELOG, poster doc). Two code comments still say ADR-0032 and were left for the user to decide: `backend/app/main.py:282`, `web/dashboard/performance.html:2`.

---

## 7. Commit, Pre-commit Hook, and CI

- The repo has an active pre-commit hook via `core.hooksPath=scripts/hooks` (`scripts/hooks/pre-commit`) that runs **black + ruff** on staged Python and blocks the commit on violations.
- GitHub Actions CI (`.github/workflows/ci.yml`, on push to `biotop`): jobs `build-c`, `test-c`, `test-backend`, and **`lint-python`** (`black --check backend/` + `ruff check backend/`, pinned `black==24.10.0`, `ruff==0.15.17`).
- The feature was committed (message: "feat: add tournament performance monitoring with live dashboard"). First attempt was blocked by the hook (black/ruff); fixed by running black/ruff + manual fixes (long f-string lines, ambiguous `l`→`lo`), then committed.

---

## 8. Push Blocked — Diagnosis & Fix

Symptom: `git push` blocked; VS Code only showed routine git-extension commands (not the real error).

- **Root cause:** **non-fast-forward divergence** — `origin/biotop` had 11 commits (incremental tournament ADR-0032, kiosk ADR-0033, integrity guards, localization) not present locally; local had 1 commit not on the remote.
- **Fix:** `git rebase origin/biotop`. Only `docs/CHANGELOG.md` conflicted (both sides prepended entries) — resolved **additively** (ADR-0034 entry on top, all remote entries kept). `backend/app/main.py` auto-merged cleanly (the new `/api/performance` endpoint coexists with the remote's match-cache index code).
- **Secondary CI finding:** `backend/app/worker.py` (from remote commit `157a7ad`) was not black-clean → `lint-python` already red on the remote. Fixed in a **separate** commit (`style: black-format worker.py to satisfy CI lint`). After it, `black --check backend/` and `ruff check backend/` are both green.
- Result: `biotop` is a clean fast-forward, ahead of `origin/biotop`. **Push not yet performed** (awaiting user go-ahead; outward action on a shared branch).

**Prevention guidance given:** `git pull --rebase origin biotop` before working/pushing; set `git config core.hooksPath scripts/hooks` on every clone.

---

## 9. Security Finding — Leaked Atlas Credential (open)

A MongoDB **Atlas connection string with credentials** (`mongodb+srv://<user>:<password>@<cluster>.mongodb.net/`) is committed and already pushed in 9 tracked files (pasted into Gemini prompts, saved as logs):
- `docs/llm_protocol/260522c_gemini_log-implementation-of-{0012,012}.{json,md}`
- `docs/llm_protocol/260525_gemini_log-bericht-html->MongoDB.{json,md}`
- `docs/llm_protocol/260528_gemini_log-implementation-of-0017.{json,md}`
- `docs/tasks/DEV_TASKS-0012-matchmaking-and-tournament-architecture.md`

Password looked like a placeholder ("mein_passwort"), but username + cluster host are real and public. GitHub treats Mongo URIs as secrets (secret-scanning alerts / push protection). `.env` itself is **not** tracked (good).

**Recommended (open):** treat as compromised → rotate Atlas password; replace strings with placeholders; optionally scrub from history (`git filter-repo`/BFG + force-push). The project already runs on local Mongo, so Atlas is likely legacy.

---

## 10. Open Items / Next Steps

1. **Commit** the 10 000 poster evaluation + new chart + regenerated study SVGs (currently uncommitted).
2. **Push** `biotop` (2 commits ahead, fast-forward ready, CI lint now green).
3. **Clean up the leaked Atlas credential** (rotate + scrub).
4. Optional: update the two remaining `ADR-0032` code comments to `ADR-0034`.
5. **Restore `.env`** to `MONGODB_DB=biotope_db` for live operation (currently set to `biotope_study` for the experiment).
6. Optional: cap/TTL-index `performance_metrics` if it grows.

---

## File Index (created/changed this session)

- `backend/scripts/top_patterns.py`, `pattern_heatmap.py`, `generate_study_configs.py`, `perf_tournament.py`
- `backend/app/main.py` (+ `GET /api/performance`), `backend/app/worker.py` (black format)
- `web/dashboard/performance.html`
- `docs/adr/ADR-0034-tournament-performance-monitoring.md`
- `docs/CHANGELOG.md`
- `docs/Präsentation/260619_Poster_Forschungsfragen_Vorschlaege.md`, `260619_Poster_Sektionen_Ausarbeitung.md`
- `docs/Präsentation/260619_F2.3_Datenvergleich_db_vs_study.md`, `260619_Poster_Auswertung_study_3000.md`, `260620_Poster_Auswertung_study_10000.md`
- `docs/Präsentation/260619_F2.3_{heatmap,top_seeds}_{db,study}.svg`, `260619_F2.3_winrate_vs_cells_study.svg`, `260620_F2.3_winrate_vs_cells_study10k.svg`
- New MongoDB collection: `performance_metrics` (in `biotope_study`; will also populate `biotope_db` when run there)
