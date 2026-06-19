# LLM Log — 3D Torus Viewer, k=0 Combinatorics Note & Editor Backend-URL Config

**Date:** 2026-06-19
**Branch:** biotop
**Scope:** Frontend/report work on the WIAI25 "Zwischenbericht #2" website
variants plus a math review of the Frage-2 combinatorics. No C / backend code
was changed.

Working directory for all HTML artifacts:
`docs/Präsentation/260525_WIAI25_Website/`.

---

## 1. Goal

Three related requests were handled in this session:

1. Add an **interactive 3D torus visualization** of the 8×8 field to the report
   page(s), in Frage 1, driven by the page's own simulator/editor.
2. **Double-check the Frage-2 combinatorics** (the `k=0` question), correct the
   analysis document, and add a clarifying footnote to the report HTML.
3. Add a **note + single config point** so the recipient of the upload variant
   can point `[Ab in die Arena]` at the correct backend URL.

---

## 2. The report variants (context)

Three HTML files carry the identical three-question article body but differ in
theme / upload capability:

| File | Theme | Editor | Server needed |
|---|---|---|---|
| `260618_WIAI25_Zwischenbericht_2.html` | Light WIAI | Simulator only | No |
| `bericht_und_editor2.html` | Conway Lab dark | Full editor + upload | Yes (same origin / configurable) |
| `bericht_und_editor2_offline.html` | Conway Lab dark | Simulator only | No (runs from `file://`) |

All three share the same JS grid model: a module-scope `grid[y][x]` (values 0/1),
`GRID_SIZE = 8`, with `createGrid(); updateUI();` at the end of the script.

---

## 3. Feature: interactive 3D torus viewer (Frage 1)

### Placement
Inserted in **all three files** exactly between the paragraph ending
*"…kein Ort ist mehr 'besonders'"* and the *"Wie wir das im Code machen"* box.

### Design decisions
- **Dependency-free renderer** (no Three.js / no CDN) so the offline variant
  keeps working from `file://`. Implemented as a small hand-rolled 3D pipeline on
  a 2D `<canvas>`.
- **Pipeline:** each of the 64 cells is a torus quad → rotate (yaw + pitch) →
  perspective project → painter's algorithm (far faces drawn first) → fill with
  normal-based shading + a faint wireframe "net". Alive cells glow cyan.
- **Live sync with zero wiring:** a `requestAnimationFrame` loop reads the
  existing module-scope `grid` every frame, so the page's own simulator drives
  the donut automatically. Drawing cells + pressing "Simulieren" animates the
  torus in lockstep.
- **Rotation handles:** pointer drag (mouse/touch/pen) orbits the model around
  its centre, pitch clamped to avoid flipping. Buttons: **Auto-Dreh** (toggle
  idle yaw spin) and **Ansicht zurücksetzen** (restore initial tilt).

### Per-variant differences
- The two **dark** variants reuse identical CSS + JS (cyan-on-dark stage).
- The **light** variant is re-themed: a WIAI-blue card with a dark `--grid-bg`
  stage (so the `#00f2ff` cells still glow) and ghost-style blue buttons. The
  renderer logic itself is identical (glow color tuned to `#00f2ff`).

### Key elements / hooks
- HTML: `<figure class="torus-figure">` containing
  `<canvas id="torus-canvas">`, plus buttons `#torus-autorotate` and
  `#torus-reset`.
- JS: a single `(function torusViewer(){ … })()` IIFE appended to the existing
  script, after `createGrid(); updateUI();` (and after `fetchGlobalCount();` in
  the upload variant).
- Each insertion is marked `// KI-Agent unterstützt`. JS validated with
  `node --check` in all three files.

---

## 4. Math review: the `k=0` (empty-field) question

The Frage-2 callout prints
`Σ_{k=0}^{24} C(64,k) = 552 859 891 708 071 949 ≈ 553 Billiarden`.

### Verification (all confirmed)
- `Σ_{k=0}^{24} C(64,k) = 552,859,891,708,071,949` (exact, matches the printed
  number).
- The total is **odd** (ends in 9). `Σ_{k=1}^{24}` would be `…948` (even). The
  difference is exactly `C(64,0)=1`.
- **Parity cross-check (Lucas):** since `64 = 2⁶`, `C(64,k)` is even for all
  `0 < k < 64` — odd only at `k=0` and `k=64`. So in range `k=0..24` the only odd
  summand is `C(64,0)`. This *proves* the empty field is included in the printed
  number.
- Fair-Play share ≈ **2.997 %** of `2^64`.

### Semantic ruling for Biotop (grounded in the repo)
- `backend/app/validators.py` enforces **only the upper bound** (`> 24` → error);
  there is **no lower bound** → the backend would formally accept an empty field.
- `web/editor/editor.html` (and the embedded editors) **disable submit at
  `count === 0`** → an empty field is **not submittable** via the UI.
- Game-theoretically an empty board is dead (stays empty, can never win).
- Conclusion: `Σ_{k=0}^{24}` counts fair-play *configurations*. The number of
  *submittable* species is `Σ_{k=1}^{24} = 552,859,891,708,071,948` (exactly one
  less). The sum also equals the volume of a Hamming ball of radius 24 in
  `{0,1}^64`, but Biotop has no "Wildtyp" reference, so the quasispecies reading
  of `k=0` does not apply here.

### Edits made
- `Szenarios/variants/kombinatorik_analyse_8x8.md`: added section
  **"3.1 Präzisierung: Der Sonderfall k=0"** (parity cross-check + semantic
  ruling + Hamming-ball note); fixed the `552 → 553 Billiarden` rounding
  inconsistency.
- Added a one-line themed footnote under the Σ big-number callout in **all three
  report HTML variants** (WIAI-blue `∗` on light, cyan `∗` on the two dark).

---

## 5. Backend-URL configuration for the upload variant

Only `bericht_und_editor2.html` uploads. Its submit used **relative** endpoints
(`/api/v1/submit_config`, `/api/v1/stats/count`), so the JSON only arrived when
the page was served from the same origin as the backend.

### Change
- Introduced a single editable constant near the top of the editor script:
  ```js
  const API_BASE = "";
  ```
  - Empty string ⇒ relative paths ⇒ same-origin (unchanged default behaviour).
  - Set to a full base URL (no trailing slash), e.g.
    `"https://biotop.wiai25.de"`, to target a remote backend.
- Both `fetch(...)` calls now use `fetch(API_BASE + '/api/v1/...')`.
- Added a **"Hinweis an den Empfänger"** comment block in the file `<head>`
  explaining same-origin vs. remote, where to set `API_BASE`, and the **CORS**
  requirement for cross-origin POSTs.
- Reworded the note to avoid a literal `<script>` token inside the HTML comment
  (parser/minifier safety); JS re-validated with `node --check`.

This supersedes the older open item ("revert the two endpoints to absolute URLs
to run locally") — `API_BASE` now covers same-origin and remote without editing
the fetch calls.

---

## 6. Files touched

- `docs/Präsentation/260525_WIAI25_Website/260618_WIAI25_Zwischenbericht_2.html`
  — torus viewer (light theme) + k=0 footnote.
- `docs/Präsentation/260525_WIAI25_Website/bericht_und_editor2.html`
  — torus viewer (dark) + k=0 footnote + `API_BASE` config + recipient note.
- `docs/Präsentation/260525_WIAI25_Website/bericht_und_editor2_offline.html`
  — torus viewer (dark) + k=0 footnote.
- `Szenarios/variants/kombinatorik_analyse_8x8.md` — section 3.1 + rounding fix.
- `docs/llm_protocol/260618_llm_log-zwischenbericht-2-website-report.md`
  — chronological steps 7 & 8 + key-numbers/open-items updates.

---

## 7. Open items / how to continue

- **Publishing** to wiai25.de is done by the developer (external).
- **Decide which variant ships** (light vs. dark; upload vs. offline). The upload
  variant now needs either same-origin hosting or `API_BASE` + backend CORS.
- The torus renderer is **duplicated verbatim** in all three files (reads `grid`
  / `GRID_SIZE` by name). If the embedded simulator's grid model is renamed, all
  three copies must be updated together.
- The dark variants embed a copy of the editor JS; if `web/editor/editor.html`
  changes (e.g. ADR-0027 flow), the embedded copies must be re-synced manually.
- Optionally update the older log
  (`260618_llm_log-zwischenbericht-2-website-report.md`) open item about
  reverting endpoints, now obsolete due to `API_BASE`.
