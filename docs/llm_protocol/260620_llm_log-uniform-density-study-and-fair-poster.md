# LLM Session Log — Uniform-Density Study & Fair Poster

**Date:** 2026-06-20
**Branch:** `biotop`
**Scope:** (1) a second, independent research study with a new cell-density
distribution, (2) a print-ready fair poster draft, (3) a data inset built from
the study's headline finding.

> No secrets in this file. The MongoDB connection string lives only in repo-root
> `.env`; it is never reproduced here.

---

## 1 · New Study: Uniform Cell-Density 2–64

### Goal
Run a completely new study of 10,000 random start configurations, with
`cell_count` drawn **uniformly over the full range 2–64** (the existing
`exhaust#` study only covered 2–24, i.e. only the rising edge of the curve).
Keep it fully separated from existing datasets.

### Key decision (asked the user)
Separation strategy → **own database** `biotope_study_uniform` (not a shared
collection). This guarantees the round-robin tournament runs over the new 10k
only (~50M pairs), independent of the old `exhaust#` set, and avoids a blended
20k tournament (~200M matches).

| Property | Value |
|---|---|
| Database | `biotope_study_uniform` (separate Mongo DB) |
| Nickname prefix | `uni64#NNNNNNNN` |
| `metadata.player_id` | `uniform_engine`, league `study` |
| Distribution | `cell_count` uniform in `[2, 64]` |
| Schema | identical to `exhaust#` study, incl. **unique index on `config.cells_int`** |

### Generator
`backend/scripts/generate_study_configs_uniform.py <total> [--db NAME]`
- Mirrors `generate_study_configs.py` but: range 2–64, prefix `uni64#`, fixed
  target DB, creates the unique `cells_int` index.
- Bit convention unchanged: `bit index = y*8 + x`, signed 64-bit bitboard.

### Run & verification
- Generated 10,000 configs (background).
- Distribution check: all 63 values 2–64 present, ~159 configs per bucket.
- **Edge effect (expected, not a bug):** the 64-cell bucket holds exactly **1**
  config — there is only one full-board pattern and the unique `cells_int`
  index truncates that degenerate top end.

### Tournament
- Runner: `MONGODB_DB=biotope_study_uniform python3 backend/scripts/perf_tournament.py`
  (env override points the existing perf runner at the new DB).
- **Process note / gotcha:** the first launch used `nohup … &`, which detached
  the worker from the harness AND killed the Python parent that does the
  write-back. Fix: relaunch as a *tracked* background task (no `nohup`, no `&`)
  so the parent survives the whole run and rankings are written back.
- Result: 10,000 competitors, **99,990,000 matches**, wall clock **2853 s
  (~47.5 min)**, CPU 53,355 s (**18.7× OpenMP speedup**), **0 % never
  stabilized** (max stable-gen 727 < cap 1000). Rankings + a
  `performance_metrics` doc written back.

### Headline finding (the scientifically interesting part)
Average win-rate vs. `cell_count` is an **inverted-U**:

| Cells | Ø win-rate |
|---|---|
| 2–3 | ~19 % |
| 8–11 | ~41 % |
| 16–19 | ~61 % |
| **24–32** | **~66 % (peak, max at 26 cells = 66.6 %)** |
| 36–39 | ~61 % |
| 44–47 | ~45 % |
| 56–64 | ~40 % |

**Interpretation:** there is a **density sweet spot at ~⅓–½ of the board
(24–32 of 64 cells)**. The old 2–24 study only saw the rising edge, which made
"fuller is not better / sparse beats dense" look like the whole story. The full
range shows the trend *reverses* past ~⅓ density — an optimum, not a monotone
relationship.

---

## 2 · Fair Poster Draft

### Brief
Build the poster per the recommendation in
`docs/Präsentation/260619_Poster_Forschungsfragen_Vorschlaege.md`:
three questions along the narrative **Fascination → Participation → Origin**:
- **F2.1** visual entry (emergence, live kiosk),
- **F3.2** interactive core (audience plays — the unique selling point),
- **F1.1** personal story (first-semester + AI builds a distributed system).

Must follow the Uni-Bamberg guideline `Poster_wirkungsvoll_gestalten.pdf`.

### Binding guideline constraints (extracted from the PDF)
- **A0 portrait** (exception: A1 landscape).
- Less is more; large/simple/clear/consistent; sans-serif (Arial/Tahoma).
- Min font sizes: **Title 78 pt, Author 72 pt, Subheads & Text 36 pt**.
- **Most important content in the center**, not the bottom; strong contrast.

### Design decision
The recommendation implied a 3-column landscape layout, but the guideline
mandates **A0 portrait**. Resolution: run the narrative **vertically**
(top→bottom) and place the interactive core **F3.2 in the visual center** — this
satisfies both the narrative and the "most important in the center" rule.

### Deliverables — `docs/Präsentation/poster_entwurf/`
| File | Purpose |
|---|---|
| `poster.md` | Master copy: per-section core message, visualization, ≤3 bullets, talking points; layout sketch; design spec; asset manifest; print checklist |
| `poster_a0.html` | **Assembled A0 print proof** — all SVGs + PNGs in final layout; open in browser → Print → Save as PDF (A0 portrait, no margins, background graphics on) |
| `make_poster_svgs.py` | Generator for the diagram SVGs (grids/glider computed with the real Conway step) |
| `f21_vier_regeln.svg` | F2.1 — four rules as before→after icons |
| `f21_gleiter_sequenz.svg` | F2.1 — glider over 5 generations |
| `f32_mitmach_ablauf.svg` | F3.2 — 4-step participation flow + simplified pipeline |
| `f11_architektur_3schichten.svg` | F1.1 — 3-layer architecture (Web / Python / C) |
| `f11_kennzahlen.svg` | F1.1 — key-figures bar |

### Repo metrics for the F1.1 figure bar (recounted 2026-06-20)
- ≈ **13,300** lines of own code (C ≈ 4,400 · Python ≈ 3,100 · Web ≈ 5,800)
- **3** native C programs · **17** automated tests (6 C, 11 Python) · **35** ADRs
- cJSON (~3,500 lines, embedded) deliberately excluded.

### Snapshots (delivered by the developer as PNG)
`kiosk_live.png`, `editor_tablet.png`, `leaderboard.png`, `qr_editor.png`,
`code_vignette.png` — all reviewed, content correct. Two are lower-res
(`editor_tablet` 539 px, `code_vignette` 606 px): fine at small/medium
placement, re-shoot higher-res if printed large.

---

## 3 · Poster Data Inset Replaced with the Density Finding

The old inset **"Gibt es Muster, die (fast) immer gewinnen?"** (heatmap +
top-seeds, from the small human dataset) was **replaced** by the new finding.

- New generator: `docs/Präsentation/poster_entwurf/make_winrate_curve.py`
  - Reads `biotope_study_uniform` directly (forces that DB), re-runnable after
    any new epoch.
  - Output: **`f23_winrate_vs_cells.svg`** — the win-rate-vs-cell-count curve:
    inverted-U, highlighted **sweet-spot band 24–32**, peak marker, end
    annotations ("zu leer → verliert ~19 %", "zu voll → verliert ~40 %").
  - Verified by rendering to PNG (cairosvg) and fixing two layout issues:
    title/subtitle overflow (reduced font + shortened) and a label collision
    (moved the peak caption below the marker).
- Integrated into **both** `poster.md` (Section 5) and `poster_a0.html`
  (inset now shows the curve; old `../260619_F2.3_*` references removed).
- New inset message: *"Wie viele Zellen gewinnen? — Es gibt ein Dichte-Optimum"*
  (~⅓–½ occupancy ≈ 66 %), with a note explaining why this is more precise than
  the earlier truncated claim.

---

## 4 · How to Continue

- **Refresh the curve** after a new tournament epoch:
  `python3 docs/Präsentation/poster_entwurf/make_winrate_curve.py`
- **Regenerate diagram SVGs:** `python3 docs/Präsentation/poster_entwurf/make_poster_svgs.py`
- **Re-run / extend the study:**
  `python3 backend/scripts/generate_study_configs_uniform.py 10000`
  then `MONGODB_DB=biotope_study_uniform python3 backend/scripts/perf_tournament.py`
- **Produce the printable PDF:** open `poster_a0.html` in a browser, verify it
  fits one A0 page (tighten `.band` margins in the `<style>` if it overflows),
  then Print → Save as PDF (A0 portrait, no margins, background graphics on).
- **Before print (open items in `poster.md` checklist):** replace `[Name(n)]`
  placeholders (in `poster.md` AND `poster_a0.html`); re-verify the metrics bar;
  test the QR in the stand LAN.

### Possible next analyses (not yet done)
- A heatmap / top-seeds view *for the uniform dataset* (analogous to the old
  `pattern_heatmap.py` outputs) to complement the curve.
- A dedicated study write-up `docs/Präsentation/..._study_uniform.md`.
