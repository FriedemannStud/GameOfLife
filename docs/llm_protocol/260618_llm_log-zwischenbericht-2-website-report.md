# LLM Log — Interim Report #2 ("Zwischenbericht #2") for wiai25.de

**Date:** 2026-06-18
**Branch:** biotop
**Scope:** Authoring a second public interim report (HTML) for the WIAI25
anniversary website, replacing report #1 at
`https://wiai25.de/projekt-werkstatt/#biotop-game-of-life`.

---

## 1. Goal

Create "Zwischenbericht #2" in the style of report #1. The report must address
three guiding questions for a general/curious audience (with first-semester CS
"Diskrete Modellierung" depth):

1. **How does a chessboard become infinite?** (board topology)
2. **How many different biotopes can exist on an 8×8?** (combinatorics)
3. **Is the right or left side at an advantage?** (symmetry / fairness)

Working directory for all artifacts:
`docs/Präsentation/260525_WIAI25_Website/` (kept here so the existing video
asset `GoLf_optimiert.mp4` and relative paths still resolve).

---

## 2. Research — where each question is grounded in the repo

Before writing, the codebase was searched. All three questions have
authoritative sources:

- **Q1 (Torus topology):** `src/core/game_logic.c:58` `sync_ghost_borders()`.
  Comment literally reads *"für unendliches Spielfeld (Wrapping)"*. All four
  edges + corners wrap → discrete **torus**. The browser editors mirror this
  with `(pos + 8) % 8` modulo wrapping.
- **Q2 (Combinatorics):** `Szenarios/variants/kombinatorik_analyse_8x8.md`
  (dated 2026-05-25). Independently re-derived and confirmed:
  - Unconstrained: `2^64 = 18,446,744,073,709,551,616` (~18.4 Trillionen).
  - Fair-Play constrained (≤ 24 live cells): `Σ_{k=0..24} C(64,k) =
    552,859,891,708,071,949` (~553 Billiarden), ≈ 3 % of the maximum.
  - The 24-cell cap = the 38 % biomass Fair-Play rule (`backend/app/validators.py:9-17`,
    rationale in ADR-0011).
- **Q3 (Side advantage):** `docs/tech_design/DEV_TECH_DESIGN-0026-two-round-symmetry-analysis.md`
  and `docs/adr/ADR-0026-remove-round-2-symmetry.md`.
  - **Answer: neither side is favored — provable.** On a torus (no edge) with a
    colour-symmetric rule (birth requires *exactly 3* neighbours → odd → never a
    RED=BLUE tie), Round 2 is an exact image of Round 1 under
    `T = shift-8-columns ∘ colour-swap`. Round 2 was therefore redundant and
    removed (halves tournament compute; identical ranking).
  - **Narrative through-line:** Q1 (torus) is the precondition that makes Q3's
    answer "no". Q2 explains why the tournament can never be brute-forced.

---

## 3. Source/style references used

- `docs/Präsentation/260525_WIAI25_Website/bericht_und_editor_ohne_upload.html`
  — report #1, offline variant (light WIAI blue/yellow theme, embedded 8×8
  simulator, no upload).
- `docs/Präsentation/260525_WIAI25_Website/bericht_und_editor.html`
  — report #1, upload variant (nickname + "Ab in die Arena" submit + global
  counter, posting to FastAPI).
- `web/landing/index.html` — "Conway Lab" Modern-Retro dark theme (neon cyan
  `#00dcff` / red `#ff3c64`, Orbitron/Rajdhani/Share Tech Mono fonts, live GoL
  backdrop canvas).
- `web/editor/editor.html` — the real frontend editor sharing that dark theme,
  incl. the ADR-0027 name-claiming / recovery-code flow.

---

## 4. Artifacts produced (in `docs/Präsentation/260525_WIAI25_Website/`)

| File | Theme | Editor | Server needed |
|---|---|---|---|
| `260618_WIAI25_Zwischenbericht_2.html` | Light WIAI (report #1 style) | Simulator only (+ video) | No |
| `bericht_und_editor2.html` | **Conway Lab dark** (frontend) | Full frontend editor + upload | Yes (same origin) |
| `bericht_und_editor2_offline.html` | **Conway Lab dark** | Simulator only | No (standalone) |

All three contain the identical three-question article body.

---

## 5. Chronological development steps

1. **Initial report created** (`260618_WIAI25_Zwischenbericht_2.html`):
   report #1's light theme + embedded simulator + video, three questions with
   yellow "Frage" tags, dark big-number callouts, green answer boxes.
2. **Upload variant created** (`bericht_und_editor2.html`): same content but
   with the full report-#1 editor (nickname, "Ab in die Arena", global counter,
   FastAPI integration). Fixed a misleading error string ("Brücke (submit.php)…"
   → "Backend nicht erreichbar").
3. **Q2 trimmed** (both files at the time): on request, removed the `2^64`
   derivation, the product-rule explanation, the sand-grains/universe-age
   analogy, and the comparison table. Frage 2 now presents **only the Fair-Play
   result** (binomial sum → ~553 Billiarden).
4. **Re-skin to frontend design** (`bericht_und_editor2.html`): re-themed the
   whole page to the Conway Lab dark theme and embedded the real
   `web/editor/editor.html` editor verbatim (HUD brackets, neon buttons, stat
   readouts, ADR-0027 recovery overlay + PNG export) plus the landing page's
   live GoL backdrop canvas.
   - **API endpoints changed to relative** (`/api/v1/submit_config`,
     `/api/v1/stats/count`) to match the frontend → only works when served from
     the same origin as the backend (previously absolute `127.0.0.1:8000`).
5. **Offline variant created** (`bericht_und_editor2_offline.html`): standalone,
   Conway Lab dark theme, **no server, no upload, no nickname, no recovery flow,
   no global counter, no fetch calls**. Keeps draw + Simulate/Reset/Clear +
   stats + 24-cell limit + torus wrapping. Panel renamed "Spezies-Simulator".
   Runs from `file://` (only Google Fonts load online, with graceful fallback).
6. **Removed doc references in Frage 3** (all three files): deleted the closing
   sentence pointing to DEV_TECH_DESIGN-0026 and ADR-0026 from the "Die schöne
   Konsequenz" box.
7. **Interactive 3D torus viewer added to Frage 1** (all three files): an
   `<canvas id="torus-canvas">` figure inserted exactly between the
   "…kein Ort ist mehr 'besonders'" paragraph and the "Wie wir das im Code
   machen" box. It visualises the *same* 8×8 simulation on the surface of a
   donut so the wrapped (glued) edges become literally visible.
   - **Dependency-free renderer** (no Three.js / no CDN → still runs from
     `file://`): a small hand-rolled 3D pipeline — each of the 64 cells is a
     torus quad, rotated → perspective-projected → drawn far-to-near
     (painter's algorithm) with normal-based shading and a faint wireframe net.
   - **Live sync, zero wiring:** a `requestAnimationFrame` loop reads the
     existing module-scope `grid` (and `GRID_SIZE`) each frame, so the page's
     own simulator/editor drives its own donut. Drawing cells + "Simulieren"
     animates the torus in lockstep; alive cells glow cyan.
   - **Rotation handles:** drag (pointer = mouse/touch/pen) orbits the model
     around its centre (pitch clamped to avoid flipping); idle **Auto-Dreh**
     yaw spin (toggle button) + **Ansicht zurücksetzen** button.
   - **Theming:** the two dark variants reuse identical CSS/JS (cyan-on-dark);
     the light variant (`260618_WIAI25_Zwischenbericht_2.html`) is re-themed to
     a WIAI-blue card with a dark `--grid-bg` stage (so the `#00f2ff` cells
     still glow) and ghost-style blue buttons. Renderer logic is identical.
   - Each insertion is marked `// KI-Agent unterstützt`. JS validated with
     `node --check` in all three files.
8. **`k=0` (empty-field) clarification — math review + footnote** (Frage 2):
   - Double-checked the printed combinatorics. `Σ_{k=0}^{24} C(64,k) =
     552,859,891,708,071,949` is exact. **Parity cross-check (Lucas):** since
     `64 = 2⁶`, `C(64,k)` is even for all `0 < k < 64` (odd only at `k=0` and
     `k=64`), so in range `k=0..24` the only odd summand is `C(64,0)=1` → the
     total is odd (ends in 9). Without `k=0` it would be `…948`. This *proves*
     the empty field is counted.
   - **Semantic ruling for Biotop** (grounded in the repo): `validators.py`
     enforces only the upper bound (≤ 24), no lower bound → backend would
     formally accept an empty field; but `web/editor/editor.html:560` disables
     submit at `count === 0` → not submittable via the UI; and an empty board is
     game-theoretically dead. So the sum counts fair-play *configurations*; the
     *submittable* count is `Σ_{k=1}^{24} = …948` (exactly one less).
   - Updated `Szenarios/variants/kombinatorik_analyse_8x8.md`: new section "3.1
     Präzisierung: Der Sonderfall k=0", fixed the 552→553 Billiarden rounding,
     noted the Hamming-ball-radius-24 equivalence (but no "Wildtyp" reference in
     Biotop).
   - Added a one-line themed footnote under the Σ big-number callout in **all
     three report HTML variants** (WIAI-blue ∗ on light, cyan ∗ on the two dark).

---

## 6. Key numbers (for reuse)

- 8×8 grid = **64 cells**; Fair-Play biomass cap = **24 cells (38 %)**.
- Constrained species count: **552,859,891,708,071,949 ≈ 553 Billiarden**
  (552.86 Billiarden; ≈ 2.997 % of `2^64`). Starts at `k=0` (empty field);
  *submittable* species = `Σ_{k=1}^{24} = 552,859,891,708,071,948` (one less).
- (Dropped from text, kept for reference) unconstrained: `2^64 ≈ 1.84 × 10^19`;
  two-species field `3^64 ≈ 3.43 × 10^30`.
- Tournament round removal: N=50 → 1225 instead of 2450 matches.

---

## 7. Open items / how to continue

- **Publishing** to wiai25.de is done by the developer (external; not performed
  by the agent).
- **Decide which variant ships:** light vs. dark; upload vs. offline. The
  upload dark variant needs the FastAPI backend reachable on the same origin.
- **Old light variant** `260618_WIAI25_Zwischenbericht_2.html` now also carries
  the Frage-1 torus viewer (step 7); otherwise its content is unchanged —
  confirm whether to keep or delete it.
- The torus renderer is duplicated verbatim in all three files (reads `grid` /
  `GRID_SIZE` by name). If the embedded simulator's grid model is renamed, the
  three copies must be updated together.
- If the dark upload variant should run locally (not same-origin), revert its
  two endpoints to absolute URLs (`http://127.0.0.1:8000/...`).
- The dark variants embed a copy of the editor JS; if `web/editor/editor.html`
  changes (e.g. ADR-0027 flow), the embedded copy must be re-synced manually.
