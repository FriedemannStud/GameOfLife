# LLM Session Log — Kiosk Mode Render Budget for Office-Laptop Operation

**Date:** 2026-06-20
**Branch:** `biotop`
**Topic:** Technical deep dive into Kiosk Mode rendering, plus a full `/run-dev`
document set (ADR → SPEC → TECH_DESIGN → TASKS) to reduce render load so the
Kiosk runs on an ordinary office laptop.
**Status at end of session:** All four planning documents written; **no code
changed yet**. Implementation (Step 4 of `/run-dev`) not started.

---

## 1. Goal of the Session

Run the Kiosk Mode (`STATE_KIOSK_MODE`) unattended for hours on an ordinary
office laptop (integrated Intel-class GPU), instead of the workshop machine it
was tuned on. Produce a feasibility assessment and a planned, low-risk change set
that lowers sustained render/thermal load **without changing the Multicam look**.

---

## 2. Deep-Dive Findings (the analysis that motivated the work)

- **Kiosk has two sub-states** (`app_state_manager.c`):
  - `KIOSK_SUB_LEADERBOARD` (~15 s, adaptive to 48 s): text + rectangles only —
    GPU-light.
  - `KIOSK_SUB_MULTICAM` (30 s): four live 8×16 matches through the fossil-trail
    shader pipeline — GPU-heavy.
- **Simulation cost is negligible.** Four 8×16 worlds stepped every
  `KIOSK_SIM_GEN_INTERVAL_S = 0.4 s` (= 2.5 generations/s), chunk-skipped in
  `game_logic.c`. CPU is not the constraint.
- **The cost is the render loop.** `init_renderer` sets `SetTargetFPS(120)`
  (`renderer.c:494`). Content changes 2.5×/s but is redrawn 120×/s → ~48×
  over-draw.
- **The over-draw is structurally required by the fossil-trail shader.**
  `assets/shaders/biotope_base.fs` computes `currentColor = max(BG, prevColor *
  fadeRate)` through a per-quadrant ping-pong FBO every frame, so the picture is
  re-shaded continuously even when no cell changes.
- **Per Multicam frame, per quadrant** (`DrawGridAndCellsCtx`, `renderer.c:232`):
  one CPU→GPU `UpdateTexture`, one full-viewport shader pass into the FBO, one
  full-viewport blit. Four quadrants ≈ two full-screen fills/frame ≈ 0.5
  Gpixel/s at 1080p, sustained, with a busy-wait FPS cap.
- **Two coupling/scaling problems:**
  - `fadeRate` is a hard-coded **per-frame** constant `0.95f` (`renderer.c:323`),
    so trail decay is tied to frame rate — lowering FPS would lengthen trails.
  - Ping-pong FBOs are sized to on-screen quadrant pixels (`renderer.c:276`), so
    on a 4K panel the shader workload scales up to ~4× the 1080p case.
- **Verdict:** It likely already runs on most office laptops; the real risk is
  sustained thermal/fan load in all-day operation and the high-DPI worst case —
  both fixable with small, low-risk tuning.

---

## 3. Decisions Taken (user-confirmed)

- **FPS cap:** configurable named constant `KIOSK_TARGET_FPS`, **default 60**
  (one-line per-device tuning knob; drop to 30 for weak GPUs).
- **Scope:** the minimal set **A + B + E + H** only.
- **Document numbering convention:** the whole `/run-dev` document set shares the
  **ADR's number** (0033) across `adr/specs/tech_design/tasks`, overriding the
  skill's per-directory "next available" numbering. (Saved to agent memory.)

---

## 4. The Chosen Change Set (A + B + E + H)

| ID | Change | File / location |
|----|--------|-----------------|
| **A** | `SetTargetFPS(120)` → configurable `KIOSK_TARGET_FPS` (default 60) | `config.h`, `renderer.c:494` |
| **B** | Frame-rate-independent fossil fade: `fade = pow(0.95, dt·120)` | `renderer.c:323` |
| **E** | Cap ping-pong FBO resolution at ≈1080p, upscale on blit (no-op ≤1080p) | `renderer.c` `DrawGridAndCellsCtx` + `init_render_context`; new `RenderContext.fbo_w/fbo_h` |
| **H** | Add `FLAG_VSYNC_HINT` to window flags | `renderer.c:483` |

**Deliberately deferred** (documented as follow-up levers for very weak GPUs):
- **C** — render the FBO down at grid scale (bigger GPU win, coarser trails).
- **F** — reduce `KIOSK_DEFAULT_MATCH_COUNT` from 4 to 2 (emergency lever).
- Dropping the fossil-trail shader entirely (visual redesign — out of scope).

**Parity guarantee (B):** the fade is a geometric per-frame decay; holding
wall-clock decay constant requires `fade = REF_FADE^(dt·REF_FPS)`. At the
reference 120 fps, `dt = 1/120` ⟹ `fade = 0.95` exactly — bit-for-bit the old
behaviour. So the look is unchanged on existing hardware.

---

## 5. Document Set Produced (all numbered 0033)

- **ADR:** `docs/adr/ADR-0033-kiosk-office-laptop-render-budget.md`
  (Status: `proposed`) — context, decision (A+B+E+H), consequences, alternatives.
- **Spec:** `docs/specs/DEV_SPEC-0033-kiosk-office-laptop-render-budget.md`
  — FR-1..FR-4, NFR-1..NFR-6, MoSCoW, backlog K1–K10, DoD adapted to C/GUI
  (warning-free `make`, interactive verification per Team Principle).
- **Tech Design:** `docs/tech_design/DEV_TECH_DESIGN-0033-kiosk-office-laptop-render-budget.md`
  — concrete edits, the five `config.h` constants, the `compute_fbo_size` helper,
  exact line references, two Mermaid diagrams, robustness/perf analysis.
- **Tasks:** `docs/tasks/DEV_TASKS-0033-kiosk-office-laptop-render-budget.md`
  — 4 phases, interactive verifications, critical ordering enforced.

---

## 6. Implementation Plan (DEV_TASKS-0033) — summary

**Critical ordering:** implement and verify **FR-2 (fade) at the unchanged
120 fps first** (must be a visual no-op), *then* lower the cap. Do not reorder.

- **Phase 1 — Constants + frame-rate-independent fade (FR-2):**
  1.1 add 5 constants to `config.h` (`KIOSK_TARGET_FPS=60`,
  `KIOSK_FADE_REF_FPS=120.0f`, `KIOSK_FADE_PER_REF_FRAME=0.95f`,
  `KIOSK_FADE_DT_MAX=0.1f`, `KIOSK_MAX_FBO_HEIGHT=1080`).
  1.2 replace `float fade = 0.95f;` with clamped `powf`-based fade.
  1.3 interactive parity check at 120 fps (trails must look identical).
- **Phase 2 — Lower cap + VSync (FR-1/FR-4):**
  2.1 `FLAG_VSYNC_HINT` + `SetTargetFPS(KIOSK_TARGET_FPS)`.
  2.2 interactive check: trails identical, smooth at 60 fps, cooler/quieter.
  2.3 temporarily set cap to 30, confirm trail length unchanged (wall-clock
  invariance), then restore to 60.
- **Phase 3 — FBO resolution cap (FR-3):**
  3.1 add `fbo_w/fbo_h` to `RenderContext`.
  3.2 add `compute_fbo_size` helper.
  3.3 allocate FBOs at capped size in both creation paths; bilinear filter;
  draw step uses `fbo_w/fbo_h` for `fboDest`/`screenSource`, `screenDest` stays
  in true pixels.
  3.4 interactive no-op check on ≤1080p; 3.5 optional high-DPI check.
- **Phase 4 — Quality gate:** `make clean && make` (zero warnings), WASM build
  path, CHANGELOG + CLAUDE.md tuning note, set ADR status to `accepted`.

**`-Wall` traps flagged in the plan:** unused `static` helper between 3.2 and
3.3; `powf`/`<math.h>` in 1.2 (math.h already included via `ceilf`/`sqrtf`).

---

## 7. How to Continue

1. Read the four 0033 documents (ADR → SPEC → TECH_DESIGN → TASKS).
2. Execute DEV_TASKS-0033 **in order**, phase by phase. Run `make` (zero
   warnings) after every code change.
3. The GUI verifications are **interactive** (Team Principle): the developer runs
   `./build/biotope`, navigates to the Multicam view, and reports observations;
   the agent does not capture the screen autonomously.
4. Keep the Phase 1 → Phase 2 ordering: prove the fade is a no-op at 120 fps
   before lowering the cap.
5. On completion, update CHANGELOG/CLAUDE.md and flip ADR-0033 to `accepted`.

---

## 8. Key References

- Render path: `src/gui/renderer.c` — `DrawGridAndCellsCtx` (232),
  `init_render_context` (144), `init_renderer` (482).
- Shaders: `assets/shaders/biotope_base.fs`, `biotope_base_web.fs` (unchanged).
- Kiosk state machine: `src/gui/app_state_manager.c` (`STATE_KIOSK_MODE`, 286).
- Constants: `src/core/config.h`.
- Build: `make` (`gcc -Wall -Wextra -std=c99 -O3 -fopenmp`), must be warning-free.
