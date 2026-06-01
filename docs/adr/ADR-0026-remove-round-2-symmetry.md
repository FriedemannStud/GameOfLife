### **ADR-0026: Remove Redundant Round 2 from Tournament Match Pairs**

**Status:** Accepted

**Date:** 2026-06-01

---

#### **1. Context and Problem Statement**

The original tournament design specifies that every pairing between competitors `P_A` and `P_B` consists of **two rounds**:

- **Round 1:** `P_A` placed on the left half of the board (assigned `TEAM_RED`), `P_B` on the right (assigned `TEAM_BLUE`).
- **Round 2:** Sides swapped — `P_B` left (`TEAM_RED`), `P_A` right (`TEAM_BLUE`).

The stated rationale was: *the two different starting positions create two genuinely different situations, ensuring neither competitor benefits from an unfair positional advantage.*

Empirical observation revealed that **every Round 2 produces an outcome that is a colour-swapped mirror of Round 1** — the same winner, the same margin, the same stabilisation generation. This raised the question: are the starting positions actually distinct, or is Round 2 informationally identical to Round 1?

The board topology is a **torus** (`sync_ghost_borders` in `src/core/game_logic.c:59` wraps all four edges, including corners). The two starting configurations `s₁` (Round 1) and `s₂` (Round 2) are related by the compound transformation

```
T = σ_{(0,8)}  ∘  χ       (shift 8 columns on the torus, then swap RED↔BLUE)
```

A formal analysis (see [DEV_TECH_DESIGN-0026](../tech_design/DEV_TECH_DESIGN-0026-two-round-symmetry-analysis.md)) proves that `T` is an **exact symmetry** of the update rule `Φ` — that is, `Φ ∘ T = T ∘ Φ`. Consequently, `s₂(g) = T(s₁(g))` holds for every generation `g`, and both rounds terminate at the same step with populations related by `pop_RED(s₂) = pop_BLUE(s₁)` and vice versa. Round 2 is therefore **mathematically redundant**: it cannot change any ranking, it produces only duplicate highlight entries with swapped seeds, and it doubles every tournament statistic by a constant factor.

This result depends on three structural properties of the current implementation (the "tripwire conditions"):
1. **Torus topology** — `sync_ghost_borders` makes the grid homogeneous with no edges.
2. **Colour-symmetric update rule** — births require exactly `n = 3` live neighbours (an odd number), making `ρ = β` impossible and the asymmetric tie-breaker unreachable.
3. **Position-independent rule** — `Φ` does not depend on absolute cell coordinates.

If any of these three conditions is violated in a future rule change, Round 2 must be re-evaluated.

---

#### **2. Decision**

**Remove Round 2 from all tournament match pairs.**

In `src/apps/hyper/main_hyper.c`, each iteration of the O(N²) round-robin loop currently calls `run_isolated_match` twice per pair. The second call is removed. Scoring, win/draw/loss counters, and highlight collection are adjusted to reflect a single result per pair.

Concretely:

| File | Change |
|---|---|
| `src/apps/hyper/main_hyper.c` | Remove the `res2 = run_isolated_match(j, i, ...)` call and all score/stats/highlight lines that reference `res2`. |
| `src/apps/hyper/main_hyper.c` | `scores[i].matches_played += 2` → `+= 1`; same for `scores[j]`. All `_atomic` pairs that operated on both `res1` and `res2` collapse to a single operation on `res1`. |

The single remaining call (`run_isolated_match(i, j, ...)`) assigns `TEAM_RED` to `P_A` (left) and `TEAM_BLUE` to `P_B` (right), which is the existing convention and requires no change to `game_logic.c` or any other file.

A **code comment** is added at the call site documenting the symmetry argument and the tripwire conditions, so a future developer who considers reintroducing Round 2 (e.g. after changing the topology) can understand the rationale immediately.

---

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

- **Halved tournament compute time.** The hyper-worker performs `N(N−1)/2` match calls instead of `N(N−1)`. For a tournament of N=20 competitors this reduces from 380 to 190 simulations; for N=50 from 2450 to 1225. OpenMP parallelisation benefit is retained.
- **Identical rankings.** Because Round 2 scores always equal Round 1 scores (by the proven symmetry), removing it leaves every relative ranking unchanged.
- **Cleaner highlight pool.** The `epoch_highlights` collection in MongoDB currently receives near-duplicate entries (same match, colours swapped). Single-round evaluation eliminates this duplication and improves the variety of highlights shown in Kiosk Mode.
- **Truthful statistics.** `matches_played` will reflect the actual number of distinct games run, not double-counted pairs.
- **Future-proof documentation.** The tripwire comment at the call site makes the invariant explicit, guarding against silent breakage if the rule or topology is changed.

**Negative Consequences (Disadvantages):**

- **Original design intent is visibly abandoned.** The two-round scheme was a deliberate design choice. Removing it without clear documentation could confuse future contributors who recall the original rationale. Mitigated by this ADR and the in-code comment.
- **Tripwire risk.** If a future change violates one of the three symmetry conditions (torus topology, odd-birth-count rule, position-independent rule) and Round 2 is not reintroduced, the tournament will silently become position-biased. The in-code comment and this ADR are the only safeguards; there is no automated test for the symmetry property.
- **Asymmetric colour assignment remains.** In the single remaining round, `P_A` is always `TEAM_RED` and `P_B` is always `TEAM_BLUE`. This is fair (proven by the analysis), but may still *look* asymmetric to a participant who notices their pattern is always the same colour. Cosmetic concern only; no ranking impact.

---

#### **4. Alternatives Considered**

**Option B — Keep Round 2 as documented insurance.**
Leave the code unchanged but add an ADR and in-code comment marking Round 2 as currently redundant. Retains the compute overhead (`N(N−1)` matches) in exchange for zero code change and automatic re-activation should a symmetry condition ever break. Rejected because the overhead is real and ongoing, while the "insurance" case (a rule change that breaks colour symmetry) would require code changes elsewhere anyway — at which point reintroducing Round 2 is a trivial one-line restore.

**Option C — Break a symmetry condition to make position genuinely matter.**
Modify the game design so that left and right starting positions yield different win probabilities. Three concrete mechanisms:
- Switch from torus to a **bounded grid** (dead-cell border): edges create differential effects that distinguish the two halves.
- Use an **asymmetric offset** for initial placement (not a half-torus shift) so that `s₂` is no longer a `T`-image of `s₁`.
- Introduce a **position- or colour-dependent rule** (e.g. different birth thresholds per half, or a resource gradient).

All three constitute game-design changes that affect balance and player strategy, and each would require its own ADR. They would honour the original intention of "position creates different situations", but at the cost of introducing genuine positional bias that must then be carefully balanced. Not appropriate as a consequence of this ADR alone; left open for a future design decision.

**Option D — Replace Round 2 with a random second starting arrangement.**
For each pair, run one match with the standard placement and one with a randomly perturbed offset (e.g. a non-half-torus column shift chosen per epoch). This would create genuinely different situations in Round 2 without altering the fundamental game rule. Rejected because it introduces non-determinism into tournament scoring (two runs of the same tournament would yield different results), which undermines reproducibility and the validity of the Elo rating system.

---

*Related analysis:* [DEV_TECH_DESIGN-0026 — Symmetrie-Untersuchung der Zwei-Runden-Regel](../tech_design/DEV_TECH_DESIGN-0026-two-round-symmetry-analysis.md)
