# Requirements Analysis & Specification: Kiosk Mode Render Budget for Office-Laptop Operation

This document details the requirements for reducing the Kiosk Mode render load so it can run unattended for hours on an ordinary office laptop (integrated GPU), as described in **ADR-0033**.

---

### 1. Detailed Requirements Specification

**Background.** Kiosk Mode alternates a text-only leaderboard view with a 30 s
Multicam view that renders four live 8×16 matches through the fossil-trail
shader pipeline (`DrawGridAndCellsCtx`, `assets/shaders/biotope_base.fs`). The
render loop runs at a hard-coded `SetTargetFPS(120)` while the simulation only
advances 2.5×/s, producing a ~48× over-draw whose cost is dominated by the
per-quadrant ping-pong FBO shader passes — not by the Conway simulation. See
ADR-0033 for the full analysis.

This increment is a **performance tuning**, not a redesign. It must reduce
sustained GPU/thermal load and cap the high-DPI worst case **without changing
the Multicam appearance on existing hardware**.

#### Functional Requirements

- **FR-1 (Configurable frame cap).** The render frame-rate cap must be defined
  by a single named constant `KIOSK_TARGET_FPS` in `src/core/config.h`, default
  value `60`. `SetTargetFPS` must use this constant instead of a literal.
- **FR-2 (Frame-rate-independent fade).** The fossil-trail fade applied by the
  shader must decay at a constant rate in **wall-clock time**, independent of
  the actual frame rate. The per-frame fade factor must be derived from a
  reference decay constant and the measured frame time
  (`fade = pow(REF_FADE, dt * REF_FPS)`), with `REF_FADE = 0.95` and
  `REF_FPS = 120` chosen so that at 120 fps the behaviour is bit-for-bit the
  previous behaviour.
- **FR-3 (Capped internal render resolution).** The per-quadrant ping-pong FBO
  resolution must be bounded by a reference maximum (≈1080p-equivalent) defined
  as a named constant. When a quadrant viewport is at or below the cap, the FBO
  is allocated at native viewport size (no-op). When above the cap, the FBO is
  allocated at the capped resolution and upscaled to the viewport on the final
  blit.
- **FR-4 (VSync).** The window must be created with `FLAG_VSYNC_HINT` so the
  frame cap is enforced against the display refresh and the GPU yields between
  frames.

#### Non-Functional Requirements

- **NFR-1 (Visual parity).** On a ≤1080p display, the Multicam view must be
  visually indistinguishable from the pre-change build: same trail length, same
  colours, same sharpness of live cells.
- **NFR-2 (Load reduction).** Sustained Multicam GPU load at the default cap
  must be approximately halved versus the 120 fps baseline, and must no longer
  scale with panel resolution beyond the cap.
- **NFR-3 (No regressions in other states).** Interactive states
  (`STATE_RUNNING`, `STATE_OBSERVER`, replay, ignition) and the leaderboard
  sub-state must behave exactly as before. The fade change applies through the
  shared `DrawGridAndCellsCtx` path, so parity must hold there too.
- **NFR-4 (Build hygiene).** `make` must complete with **zero warnings**
  (`-Wall -Wextra -std=c99 -O3 -fopenmp`). All literals introduced must be named
  constants per the coding style; every modified block carries the
  `// KI-Agent unterstützt` attribution.
- **NFR-5 (Tuning knob).** Lowering a specific deployment device to 30 fps must
  require changing only `KIOSK_TARGET_FPS` (no logic edits).
- **NFR-6 (WASM safety).** The web build path (`PLATFORM_WEB`,
  `biotope_base_web.fs`, GLSL ES 1.00) must remain functional; the fade and FPS
  changes must not break the `#version 100` shader.

#### Constraints

- No architectural change to the state machine, ping-pong scheme, or shader
  pipeline structure.
- The default match count stays at 4 (`KIOSK_DEFAULT_MATCH_COUNT` unchanged).
- The fossil-trail aesthetic is retained (no "render only on sim step").

---

### 2. User Stories & Acceptance Criteria

**Epic: Run the Kiosk unattended on an office laptop**

*   **User Story 1: Cool, quiet all-day operation**
    *   **As an** exhibition operator, **I want** the Kiosk Mode to run for hours
        on a normal office laptop without the fan ramping up or the chassis
        getting hot, **so that** I can deploy it on hardware I already own.
    *   **Acceptance Criteria:**
        *   With the default `KIOSK_TARGET_FPS = 60`, the Multicam view renders
            at ≤ 60 fps (verifiable via FPS readout / `GetFPS()` instrumentation
            or vendor GPU monitor).
        *   VSync is active: with no work to do the process does not busy-spin a
            CPU core to 100 %.
        *   Over a sustained Multicam run the GPU load is visibly lower than the
            120 fps baseline (operator observation: lower fan noise / GPU-util).

*   **User Story 2: The show looks the same**
    *   **As a** viewer, **I want** the live battles to look exactly as designed
        — trails fading at the same speed, cells crisp — **so that** the
        performance change is invisible to the audience.
    *   **Acceptance Criteria:**
        *   On a 1080p panel, fossil trails decay over the same wall-clock
            duration as the pre-change build.
        *   Live RED/BLUE cells remain sharp (point-filtered), the hemisphere
            separator and HUD are unchanged.
        *   Switching `KIOSK_TARGET_FPS` between 60 and 30 produces **no change**
            in trail length or colour (only smoothness differs).

*   **User Story 3: High-DPI display does not melt the GPU**
    *   **As an** operator with a 4K office laptop, **I want** the render cost to
        be bounded regardless of panel resolution, **so that** the Kiosk stays
        smooth on a high-DPI screen.
    *   **Acceptance Criteria:**
        *   On a >1080p display the per-quadrant FBO is allocated at the capped
            resolution, not the native panel resolution.
        *   On a ≤1080p display the cap is a no-op (FBO == viewport size).
        *   The Multicam view remains smooth and correctly laid out on the 4K
            display (no clipping, no mis-scaled trails).

*   **User Story 4: One-line per-device tuning**
    *   **As a** developer deploying to a specific weak device, **I want** to
        change one constant to drop the cap, **so that** I can tune without
        touching render logic.
    *   **Acceptance Criteria:**
        *   Changing `KIOSK_TARGET_FPS` and rebuilding is sufficient to change
            the cap; no other edits required.
        *   The fade still decays at the same wall-clock rate at the new cap
            (FR-2 guarantee).

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   FR-2 frame-rate-independent fade (prerequisite — without it any cap
            change alters the look).
        *   FR-1 configurable frame cap (default 60).
        *   FR-4 VSync hint.
        *   NFR-1 visual parity, NFR-4 warning-free build.
    *   **Should-Have:**
        *   FR-3 capped internal render resolution (protects the high-DPI case;
            no-op on the common 1080p target).
        *   NFR-5 one-line tuning knob.
    *   **Could-Have:**
        *   FPS readout / `GetFPS()` overlay (debug-only) to make verification
            objective.
    *   **Won't-Have (in this increment):**
        *   Low-res FBO down to grid scale (option C) — reserved follow-up for
            very weak iGPUs.
        *   Reducing `KIOSK_DEFAULT_MATCH_COUNT` to 2 (option F) — emergency
            lever only.
        *   Dropping the fossil-trail shader / render-only-on-step — visual
            redesign, out of scope.

*   **Dependencies:**
    1.  **FR-2 before FR-1:** the fade must become time-based **before** the cap
        is lowered, otherwise trails visibly lengthen. Implement and verify
        FR-2 at the unchanged 120 fps first (parity check), then lower the cap.
    2.  **FR-3 within `DrawGridAndCellsCtx`:** the resolution cap touches the
        FBO allocation block; it must keep the existing reallocation trigger
        (size-change detection) intact.
    3.  **NFR-6 WASM shader:** the fade uniform must be set the same way for both
        `biotope_base.fs` (`#version 330`) and `biotope_base_web.fs`
        (`#version 100`); verify the web build path still compiles.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| K1 | Run unattended | Add `KIOSK_TARGET_FPS` (default 60) + fade reference constants to `config.h` | Must |
| K2 | Run unattended | Make shader `fadeRate` frame-time-based in `DrawGridAndCellsCtx` (FR-2) | Must |
| K3 | Run unattended | Verify visual parity at 120 fps after K2 (interactive) | Must |
| K4 | Run unattended | Replace `SetTargetFPS(120)` with `KIOSK_TARGET_FPS` (FR-1) | Must |
| K5 | Run unattended | Add `FLAG_VSYNC_HINT` to window config flags (FR-4) | Must |
| K6 | Run unattended | Verify cool/quiet 60 fps operation + parity (interactive) | Must |
| K7 | High-DPI | Cap ping-pong FBO resolution + upscale blit (FR-3) | Should |
| K8 | High-DPI | Verify no-op on 1080p, bounded on >1080p (interactive) | Should |
| K9 | Tuning | Confirm 60↔30 swap changes only smoothness, not trail length | Should |
| K10 | Quality | Warning-free `make`; WASM build path still compiles (NFR-4/6) | Must |

---

### 5. Definition of Done (DoD)

A Product Backlog Item is considered "Done" when all of the following are met:

*   **Code Quality:** Code follows `docs/CODING_STYLE.md` — English identifiers,
    `snake_case`/`PascalCase`/`UPPER_SNAKE_CASE` as specified, 4-space indent, no
    trailing whitespace, no magic numbers (new literals are named constants), and
    every new/modified block carries `// KI-Agent unterstützt`.
*   **Build:** `make` completes with **zero warnings** under
    `gcc -Wall -Wextra -std=c99 -O3 -fopenmp`. Where feasible, the WASM build
    path (`make -f Makefile.wasm`) still compiles.
*   **Tests / Verification:** Because this is a GUI/render change, verification is
    primarily **interactive** per the project's Team Principle (`CLAUDE.md`):
    Claude specifies exact navigation + observation steps (enter Kiosk, wait for
    Multicam, observe trail decay / FPS / fan), and the developer runs the app and
    reports what they see. Any existing C unit tests that compile the touched
    units continue to pass (no regressions).
*   **Acceptance Criteria:** All acceptance criteria for the story are met and
    confirmed by the developer in the running app.
*   **Code Review:** The change is in a reviewable state (small, focused diff; PR
    on the `biotop` branch).
*   **Merge:** Merged into the `biotop` development branch.
*   **Documentation:** ADR-0033 referenced; `docs/CHANGELOG.md` updated on
    completion; `CLAUDE.md` updated if the constant becomes a documented tuning
    knob.
