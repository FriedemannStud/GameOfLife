# DEV_TASKS-0033: Kiosk Mode Render Budget for Office-Laptop Operation

This plan implements the four-part minimal render-budget reduction (configurable
FPS cap, frame-rate-independent fossil fade, capped FBO resolution, VSync) so the
Kiosk Mode runs cool and quiet on an ordinary office laptop, **without changing
the Multicam look** on existing hardware.

**Developer:** Please follow these steps precisely. The plan is broken into
phases and small steps to allow for interruptions and ensure stability. After
each "Verification" step, report the outcome. This iterative process is crucial
for maintaining quality. The GUI tests follow the Team Principle (`CLAUDE.md`):
Claude states exactly what to do and observe; you run the app and report what you
see. `make` must produce **zero warnings** after every code change.

**Briefing Documents:**
*   [ADR-0033: Kiosk Mode Render Budget for Office-Laptop Operation](../adr/ADR-0033-kiosk-office-laptop-render-budget.md)
*   [DEV_SPEC-0033: Requirements Specification](../specs/DEV_SPEC-0033-kiosk-office-laptop-render-budget.md)
*   [DEV_TECH_DESIGN-0033: Technical Design](../tech_design/DEV_TECH_DESIGN-0033-kiosk-office-laptop-render-budget.md)

**Critical ordering:** FR-2 (frame-rate-independent fade) is implemented and
verified **at the unchanged 120 fps** in Phase 1 *before* the cap is lowered in
Phase 2. Otherwise lowering the FPS would visibly lengthen the trails. Do not
reorder Phase 1 and Phase 2.

---

## Phase 1: Configuration constants + frame-rate-independent fade (FR-2)

*Goal: Introduce the tuning constants and make the fossil fade decay in
wall-clock time, while the frame rate is still 120 fps — so this phase must be
visually a no-op. This is the safety net that lets Phase 2 lower the cap.*

- [ ] **Step 1.1: Add render-budget constants to `config.h`**
    - [ ] **Action:** In `src/core/config.h`, below the existing Kiosk timing
          block, add the five constants with a `// KI-Agent unterstützt:`
          comment and an `ADR-0033` reference:
          `KIOSK_TARGET_FPS` (60), `KIOSK_FADE_REF_FPS` (120.0f),
          `KIOSK_FADE_PER_REF_FRAME` (0.95f), `KIOSK_FADE_DT_MAX` (0.1f),
          `KIOSK_MAX_FBO_HEIGHT` (1080). Do **not** use them yet.
    - [ ] **Verification:** Run `make`. **Expected Result:** Build completes with
          zero warnings; binaries unchanged in behaviour (constants unused so far
          — a `-Wunused-macros` is not enabled, so no warning expected).

- [ ] **Step 1.2: Replace the hard-coded fade with a frame-time-based fade**
    - [ ] **Action:** In `src/gui/renderer.c`, inside `DrawGridAndCellsCtx`
          (~line 323), replace `float fade = 0.95f;` with the clamped,
          frame-time-derived computation from the tech design §4.3(c):
          read `GetFrameTime()`, clamp to `KIOSK_FADE_DT_MAX`, then
          `fade = (dt > 0.0f) ? powf(KIOSK_FADE_PER_REF_FRAME, dt * KIOSK_FADE_REF_FPS) : KIOSK_FADE_PER_REF_FRAME;`
          Add the `// KI-Agent unterstützt:` attribution. Confirm `<math.h>` is
          already included (it is — `ceilf`/`sqrtf` are used above).
    - [ ] **Verification:** Run `make`. **Expected Result:** Zero warnings (watch
          for an implicit-declaration warning on `powf` — if it appears, add
          `#include <math.h>` to `renderer.c`).

- [ ] **Step 1.3: Confirm visual parity at 120 fps (Interactive Test)**
    - [ ] **Action:** Leave `SetTargetFPS(120)` untouched for now. Run
          `./build/biotope`.
    - [ ] **Verification (Interactive Test):**
        1.  Start the app; it opens in Kiosk Mode (leaderboard first).
        2.  Wait ~15 s until the view switches to the 2×2 "LIVE BATTLES"
            Multicam grid.
        3.  Watch the fading "fossil" trails behind the moving cells in each
            quadrant for ~20 s.
        4.  **Expected Result:** The trails fade exactly as before this change —
            same length, same speed, same colour; live RED/BLUE cells are crisp.
            Report any difference in trail persistence (there should be none,
            since at 120 fps `pow(0.95, 1) = 0.95`).

---

## Phase 2: Lower the frame cap + enable VSync (FR-1, FR-4)

*Goal: Halve the sustained render load and stop the busy-wait. Because Phase 1
made the fade time-based, the trails must look identical at the new cap.*

- [ ] **Step 2.1: Use the configurable cap and enable VSync**
    - [ ] **Action:** In `src/gui/renderer.c` `init_renderer` (~line 483),
          change `SetConfigFlags(FLAG_WINDOW_RESIZABLE);` to
          `SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);`.
    - [ ] **Action:** Change `SetTargetFPS(120);` (~line 494) to
          `SetTargetFPS(KIOSK_TARGET_FPS);`. Add/extend the
          `// KI-Agent unterstützt:` attribution with the `ADR-0033` reference.
    - [ ] **Verification:** Run `make`. **Expected Result:** Zero warnings.

- [ ] **Step 2.2: Confirm cool/quiet operation and unchanged look (Interactive Test)**
    - [ ] **Action:** Run `./build/biotope` and let it reach the Multicam view.
    - [ ] **Verification (Interactive Test):**
        1.  Enter Kiosk Mode, wait for the "LIVE BATTLES" Multicam grid.
        2.  Observe the fossil trails for ~20 s and compare against your memory
            of Phase 1.3.
        3.  If you have a system monitor handy, note GPU utilisation / fan
            behaviour versus the 120 fps baseline; otherwise just note whether
            the laptop fan is quieter / the chassis cooler after a few minutes.
        4.  Confirm the animation is still smooth (no stutter) and the cells
            still step at the same visible rate.
        5.  **Expected Result:** Trails look identical to Phase 1.3 (same length
            and fade speed); animation is smooth at 60 fps; GPU/fan load is
            noticeably lower over time. Report trail appearance and, if measured,
            the FPS/GPU/fan observation.

- [ ] **Step 2.3: Confirm 60↔30 swap changes only smoothness, not trail length (Interactive Test)**
    - [ ] **Action:** Temporarily set `KIOSK_TARGET_FPS` to `30` in `config.h`,
          run `make`, then `./build/biotope`.
    - [ ] **Verification (Interactive Test):**
        1.  Reach the Multicam view and watch the trails for ~20 s.
        2.  **Expected Result:** Trails fade over the **same wall-clock duration**
            as at 60 fps (same visible length); only the motion is slightly less
            smooth. Report whether trail length changed (it must not).
    - [ ] **Action:** Restore `KIOSK_TARGET_FPS` to `60`, run `make`.

---

## Phase 3: Cap internal FBO resolution for high-DPI displays (FR-3)

*Goal: Bound the per-quadrant shader/blit cost at ≈1080p so a 4K office panel
does not multiply the load. No-op on ≤1080p displays.*

- [ ] **Step 3.1: Add the two FBO-size fields to `RenderContext`**
    - [ ] **Action:** In `src/gui/renderer.h`, add `int fbo_w;` and `int fbo_h;`
          to the `RenderContext` struct (next to `last_draw_w`/`last_draw_h`),
          each with a `// KI-Agent unterstützt:` comment referencing `ADR-0033`.
    - [ ] **Verification:** Run `make`. **Expected Result:** Zero warnings.

- [ ] **Step 3.2: Add the `compute_fbo_size` helper**
    - [ ] **Action:** In `src/gui/renderer.c`, add the file-static pure helper
          `compute_fbo_size(int draw_w, int draw_h, int *out_w, int *out_h)` from
          tech design §4.2 (aspect-preserving cap at `KIOSK_MAX_FBO_HEIGHT`,
          width clamped to `≥ 1`), with the `// KI-Agent unterstützt:`
          attribution. Place it above `init_render_context`.
    - [ ] **Verification:** Run `make`. **Expected Result:** Zero warnings (the
          helper may be unused until Step 3.3 — `static` unused functions *do*
          warn under `-Wall`, so if a `-Wunused-function` appears, proceed
          directly to Step 3.3 in the same edit session before re-running `make`).

- [ ] **Step 3.3: Allocate ping-pong FBOs at the capped size in both creation paths**
    - [ ] **Action:** In `init_render_context` (~lines 163-164) and in the
          reallocation branch of `DrawGridAndCellsCtx` (~lines 276-277), call
          `compute_fbo_size` on the viewport dimensions, store the result in
          `r_ctx->fbo_w`/`fbo_h`, and allocate both `ping_pong_target[i]` with
          those values instead of `bounds.width/height` / `drawWidth/drawHeight`.
          After each `LoadRenderTexture`, call
          `SetTextureFilter(... .texture, TEXTURE_FILTER_BILINEAR)`.
    - [ ] **Action:** In the draw step of `DrawGridAndCellsCtx` (~lines 313-340),
          change `fboDest` and `screenSource` to use `r_ctx->fbo_w`/`fbo_h`;
          leave `screenDest` in true viewport pixels (`drawWidth`/`drawHeight`)
          so the blit upscales. Add `// KI-Agent unterstützt:` attribution.
    - [ ] **Verification:** Run `make`. **Expected Result:** Zero warnings.

- [ ] **Step 3.4: Confirm no-op on ≤1080p (Interactive Test)**
    - [ ] **Action:** On your normal (≤1080p) display, run `./build/biotope`.
    - [ ] **Verification (Interactive Test):**
        1.  Reach the Multicam view; observe the trails and live cells for ~20 s.
        2.  Resize the window larger and smaller a few times.
        3.  **Expected Result:** Identical to Phase 2.2 (cap is a no-op at this
            resolution); no clipping, no mis-scaled trails, live cells stay
            crisp; resizing does not corrupt the grid. Report any visual change.

- [ ] **Step 3.5: Confirm bounded load on a high-DPI display (Interactive Test — if available)**
    - [ ] **Action:** If a >1080p / 4K display is available, run the app there
          maximised. (If none is available, mark this step skipped and note it.)
    - [ ] **Verification (Interactive Test):**
        1.  Reach the Multicam view; observe the trails and live cells.
        2.  **Expected Result:** Layout fills the screen correctly; live cells
            stay crisp; the soft trail may look marginally softer than native —
            acceptable. GPU/fan load is bounded, not 4× the 1080p case. Report
            appearance and, if measured, GPU load.

---

## Phase 4: Quality gate, WASM, and documentation

*Goal: Confirm build hygiene across all targets and record the change.*

- [ ] **Step 4.1: Full warning-free native build**
    - [ ] **Action:** Run `make clean && make`.
    - [ ] **Verification:** **Expected Result:** All three binaries build with
          **zero warnings** under `-Wall -Wextra -std=c99 -O3 -fopenmp`.

- [ ] **Step 4.2: WASM build path still compiles (NFR-6)**
    - [ ] **Action:** If Emscripten is available, run `make -f Makefile.wasm`.
          (If `emcc` is not installed, mark skipped and note it — the shader and
          fade changes are C-side and `#version 100`-safe by design.)
    - [ ] **Verification:** **Expected Result:** WASM build completes; the web
          shader (`biotope_base_web.fs`) is unchanged and the fade uniform is set
          identically. Report success or that it was skipped.

- [ ] **Step 4.3: Update CHANGELOG and CLAUDE.md tuning note**
    - [ ] **Action:** Add an entry to `docs/CHANGELOG.md` summarising the
          render-budget change and referencing ADR-0033. In `CLAUDE.md`, note
          `KIOSK_TARGET_FPS` in `config.h` as the per-device tuning knob.
    - [ ] **Verification:** **Expected Result:** Both files updated; diff is
          coherent and references ADR-0033.

- [ ] **Step 4.4: Set ADR status to `accepted`**
    - [ ] **Action:** Once all interactive tests pass, change the ADR-0033
          `Status:` from `proposed` to `accepted`.
    - [ ] **Verification:** **Expected Result:** ADR reflects the implemented
          decision. Implementation increment complete.
