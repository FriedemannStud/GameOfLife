# Session Log: Two-Round Symmetry Analysis & ADR-0026

**Date:** 2026-06-01
**Model:** Claude Opus 4.8 → Sonnet 4.6
**Branch:** `biotop`

---

## 1. Context and Motivation

The original tournament design specified that every match between two competitors `P_A` and `P_B` consists of two rounds:

- **Round 1:** `P_A` left (TEAM_RED), `P_B` right (TEAM_BLUE)
- **Round 2:** Sides swapped — `P_B` left (TEAM_RED), `P_A` right (TEAM_BLUE)

Rationale: different starting positions should create different situations and ensure no positional advantage.

**Observation triggering the analysis:** After watching many matches, every Round 2 produced a mirror-symmetric game with identical outcome to Round 1. The question raised: is this coincidence, or is the two-round rule structurally redundant?

---

## 2. Analysis Approach

The question was reformulated precisely:

> On a toroidal 8×16 board, do win chances depend on whether a configuration starts on the left or the right?

A formal analysis was conducted using methods of Discrete Modelling (cellular automaton as a discrete dynamical system, symmetry groups, equivariance, invariants). Code was read before any claims were made.

**Key source files inspected:**
- `src/core/game_logic.c` — update rule (`Φ`), torus wrapping (`sync_ghost_borders`), `run_isolated_match`
- `src/apps/hyper/main_hyper.c` — two-round structure, scoring logic

---

## 3. Proof Summary

### 3.1 The Transform T

Round 2 arises from swapping the arguments to `run_isolated_match`. Since `left = RED` and `right = BLUE` are hardcoded, a side-swap is simultaneously a colour-swap. The starting configuration of Round 2 is related to Round 1 by:

```
T = σ_{(0,8)}  ∘  χ      (shift 8 columns on torus) ∘ (swap RED↔BLUE)
```

T is an involution (T² = id).

### 3.2 Two Lemmas

**Lemma 1 — Translation-equivariance:** `Φ ∘ σ_δ = σ_δ ∘ Φ` for any shift δ.
- Holds because the board is a **torus** (`sync_ghost_borders` wraps all four edges including corners). Every cell has a complete 8-cell Moore neighbourhood; no edge truncation exists.

**Lemma 2 — Colour-swap equivariance:** `Φ ∘ χ = χ ∘ Φ`.
- Holds because births require exactly `n = 3` live neighbours. Since 3 is **odd**, a tie `ρ = β` (equal red and blue neighbours) is impossible. The asymmetric tie-breaker in the code is therefore **never reached**, making the rule genuinely colour-fair.

### 3.3 Main Theorem

```
Φ ∘ T = T ∘ Φ      (T is an exact symmetry of the update rule)
```

Proof: chain Lemma 2 then Lemma 1.

### 3.4 Corollary (by induction over generations)

```
s₂(g) = T( s₁(g) )   for all g ≥ 0
```

Both rounds evolve as T-images of each other at every timestep.

### 3.5 Consequence for scores

Because T is a bijection that swaps colours but preserves population counts per colour:

```
pop_RED(s₂(g*))  = pop_BLUE(s₁(g*))  =: b
pop_BLUE(s₂(g*)) = pop_RED(s₁(g*))   =: a
```

Every player ends with the **same population in both rounds**, regardless of side. Round 2 is mathematically redundant: it multiplies every tournament statistic by exactly 2 and can never change a ranking.

### 3.6 Tripwire Conditions

The result depends on exactly three structural properties. Breaking any one makes position relevant again:

1. **Torus topology** — `sync_ghost_borders` must wrap all four edges.
2. **Colour-symmetric rule** — births only at odd neighbour counts (currently: only `n = 3`).
3. **Position-independent rule** — `Φ` must not depend on absolute coordinates.

---

## 4. Documents Created

| File | Description |
|---|---|
| `docs/tech_design/DEV_TECH_DESIGN-0026-two-round-symmetry-analysis.md` | Full German-language proof document: formal model, two lemmas, main theorem, induction, scoring consequence, tripwire conditions, counterexamples, recommendation with option table |
| `docs/adr/ADR-0026-remove-round-2-symmetry.md` | Architecture Decision Record — status **Accepted** |

---

## 5. Decision (ADR-0026)

**Decision:** Remove Round 2. Run one match per pair instead of two.

**Rationale:** Win chances are position-independent (proven). By the project's own decision rule, Round 2 is therefore unnecessary.

**Impact:**
- Tournament compute halved: `N(N−1)` → `N(N−1)/2` match calls.
- Rankings unchanged.
- Highlight pool no longer contains near-duplicate (colour-swapped) entries.
- `matches_played` now reflects distinct games, not double-counted pairs.

**Alternatives considered (and rejected):**
- **Keep as insurance** — ongoing compute cost, no current benefit.
- **Break symmetry by design** — valid future option (bounded grid, asymmetric offset, or colour-biased rule), but a separate game-design decision requiring its own ADR.
- **Random second arrangement** — breaks tournament reproducibility.

---

## 6. Code Change

**File:** `src/apps/hyper/main_hyper.c`

- `res2 = run_isolated_match(j, i, ...)` and all `res2`-based scoring/stats/highlight lines **removed**.
- `results[2]` highlight loop simplified to a single block on `res`.
- `matches_played += 2` → `+= 1` for both players.
- Tripwire comment added at the call site, referencing ADR-0026 and DEV_TECH_DESIGN-0026.

Build verified: `make` produces zero warnings.

---

## 7. How to Continue

- **If the game rule or topology changes:** re-read DEV_TECH_DESIGN-0026 §7, check which tripwire condition is violated, and decide whether to restore the second `run_isolated_match` call in `main_hyper.c`.
- **To restore Round 2:** add back `MatchResult res2 = run_isolated_match(competitors[j].cells, competitors[i].cells, max_generations);` and the corresponding scoring/stats/highlight blocks. Reference the old git diff or the pre-ADR-0026 code for the exact lines.
- **Next open ADR slot:** ADR-0027.
