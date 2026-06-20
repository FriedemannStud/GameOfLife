### **ADR-0033: Kiosk Mode Render Budget for Office-Laptop Operation**

**Status:** accepted

**Date:** 2026-06-20

> **Implementation note (2026-06-20):** Implemented as the minimal set A+B+E+H.
> During on-device interactive testing on the target office laptop the developer
> confirmed 30 fps is visually acceptable for the exhibition loop, so the
> committed default of `KIOSK_TARGET_FPS` is **30** (not the 60 originally
> proposed below). Because the fossil fade is now wall-clock-based (B), trail
> length is identical at 30, 60, or 120 fps — only motion smoothness differs.
> At 30 fps the sustained Multicam render load is roughly **quartered** versus
> the old 120 fps baseline. 60 remains a one-line config change for a smoother
> look on stronger hardware. A header-dependency caveat surfaced during
> implementation: the Makefile does not track `.h` dependencies, so the
> `RenderContext` field additions required `make clean && make`.

#### **1. Context and Problem Statement**

The Kiosk Mode (`STATE_KIOSK_MODE`) is the exhibition/attract loop of the
Biotope GUI. It alternates between a leaderboard view (~15 s, text only — GPU
light) and a 2×2 Multicam view (30 s) that renders four simultaneous live
matches through the fossil-trail shader pipeline (`DrawGridAndCellsCtx`,
`assets/shaders/biotope_base.fs`).

We want to run the Kiosk Mode unattended for hours on an ordinary **office
laptop** (integrated Intel-class GPU, no discrete graphics), not on the
workshop/exhibition machine it was tuned on. A deep dive into the render path
established the following:

- The Conway simulation itself is negligible: four 8×16 worlds stepped at
  `KIOSK_SIM_GEN_INTERVAL_S = 0.4 s` (2.5 generations/second), chunk-skipped in
  `game_logic.c`. CPU cost is not the constraint.
- The cost is driven entirely by the **render loop**, which runs at
  `SetTargetFPS(120)` (`renderer.c:494`). The grid content changes 2.5×/s but
  is redrawn 120×/s — a ~48× over-draw.
- The over-draw is *structurally required by the current design*: the
  fossil-trail shader accumulates `prevColor * fadeRate` through a per-quadrant
  ping-pong FBO every frame, so the picture is re-shaded continuously even when
  no cell changes.
- During Multicam, each frame performs, per quadrant: one CPU→GPU
  `UpdateTexture`, one full-viewport shader pass into the FBO, and one
  full-viewport blit to screen. With four quadrants this fills roughly two full
  screens per frame → ≈ 0.5 Gpixel/s at 1080p, sustained.
- The fragment shader's `fadeRate` is a hard-coded **per-frame** constant
  (`0.95f`, `renderer.c:323`). The trail decay is therefore tied to the frame
  rate: lowering FPS would make trails persist proportionally longer.
- The ping-pong FBOs are sized to the on-screen quadrant pixel dimensions
  (`renderer.c:276`), so on a high-DPI / 4K office display the shader workload
  scales with panel resolution (up to ~4× a 1080p panel).

The limiting factor on an office laptop is therefore **not** "does it run" — it
almost certainly does — but sustained thermal/fan load and the high-DPI
worst case during all-day operation. The problem is a render budget that is far
larger than the content requires, with no headroom for weak integrated GPUs.

Constraints for the solution:
- The Multicam **look must not change** on the hardware it runs on today —
  fossil trails must decay at the same wall-clock rate as before.
- No architectural rewrite of the render pipeline; this is a tuning increment.
- The build must stay warning-free (`-Wall -Wextra`, `-O3`, C99).
- Changes must respect the coding style (English, `// KI-Agent unterstützt`
  attribution, named constants instead of magic numbers).

#### **2. Decision**

Reduce the Kiosk render budget through four contained, low-risk changes (the
"minimal set" A+B+E+H), without altering the visual design:

1. **Configurable frame-rate cap (A).** Replace the hard-coded
   `SetTargetFPS(120)` with a named constant in `config.h`
   (`KIOSK_TARGET_FPS`, committed default **30** — see implementation note;
   60 was the originally proposed value). The cap is configurable so a specific
   deployment device can be raised (e.g. to 60) for smoother motion without
   touching logic. At 30 fps the render load is roughly quartered versus 120 fps
   while remaining fluid for an attract loop; 2.5 generations/s leaves ample
   temporal headroom.

2. **Frame-rate-independent fossil fade (B).** Make the shader `fadeRate`
   compensate for the actual frame time so the trail decay stays constant in
   wall-clock time regardless of the FPS cap. The per-frame factor is derived
   from a reference decay (`0.95` at a `120 fps` reference) and the measured
   `GetFrameTime()`, e.g. `fade = pow(REF_FADE, dt * REF_FPS)`. This is a
   prerequisite for (A): without it, lowering FPS would visibly lengthen the
   trails. With it, the Multicam look is identical at any cap.

3. **Capped internal render resolution (E).** Bound the per-quadrant ping-pong
   FBO resolution to a reference maximum (≈1080p-equivalent) and upscale on the
   final blit to the actual viewport. On 1080p-or-smaller panels this is a
   **no-op**; on high-DPI / 4K office displays it prevents the shader workload
   from scaling with panel resolution. The grid texture stays point-filtered;
   the FBO→screen blit upscales the already-coarse trail buffer.

4. **VSync hint (H).** Add `FLAG_VSYNC_HINT` to the window config flags so the
   frame cap is enforced against the display refresh and the GPU yields between
   frames instead of busy-spinning, reducing heat and power draw.

Explicitly **in scope:** only the four changes above. The decision deliberately
keeps the fossil-trail pipeline intact and the default match count at 4.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

- Sustained Multicam GPU load drops by roughly 50 % (120→60 fps) and is capped
  against panel resolution, giving comfortable headroom on integrated GPUs and
  enabling cool, quiet all-day operation on an office laptop.
- The Multicam visual result is unchanged on existing hardware: fade decay is
  now wall-clock-based, and the resolution cap is a no-op at ≤1080p.
- `KIOSK_TARGET_FPS` and the fade reference become named constants, removing two
  magic numbers and giving a single per-device tuning knob (config-only, no
  logic change to drop to 30 fps if a specific machine needs it).
- VSync removes the busy-wait, lowering power draw and fan noise further.
- No architectural change: the state machine, FBO ping-pong, and shader pipeline
  are untouched in structure; risk and review surface are small.

**Negative Consequences (Disadvantages):**

- The frame-rate-independent fade introduces a per-frame `pow()` in the shader
  uniform path; cost is negligible (one call per quadrant per frame, CPU-side).
- The resolution cap means high-DPI displays show the fossil trail at a slightly
  coarser internal resolution than the panel's native one (live cells stay sharp
  via the point-filtered grid texture; only the soft trail is upscaled). On
  ≤1080p panels there is no difference.
- The fade formula assumes `GetFrameTime()` is well-behaved; a frame-time spike
  (e.g. a stall) momentarily fades trails faster. Acceptable and self-correcting.
- Very weak integrated GPUs (old Atom/Celeron office devices) are improved but
  not guaranteed; the stronger structural levers (low-res FBO down to grid
  scale, reducing the match count to 2) are intentionally left out of this
  increment and remain available as a follow-up if a target device fails.

#### **4. Alternatives Considered**

- **Do nothing.** The Kiosk likely runs on many office laptops already. Rejected
  because the 120 fps sustained load creates thermal/fan stress in unattended
  all-day operation and offers no headroom for weaker GPUs or 4K panels — the
  actual deployment risk.

- **Drop the fossil-trail shader in Kiosk and render only on simulation steps.**
  Would cut GPU load to near zero between the 0.4 s steps (the biggest possible
  win). Rejected for this increment because it changes the Multicam aesthetic
  significantly; it is a visual-design decision, not a performance tuning, and
  is out of scope for "run it cooler without changing the look".

- **Render the FBO at native grid resolution (≈low-res, option C).** A targeted
  ~30× reduction in fragment-shader work by sizing the FBO close to the 8×16
  grid. Rejected as the default here because it visibly coarsens the trail and
  needs more careful filtering/QA; the chosen resolution **cap** (E) is the same
  mechanism with a conservative ≈1080p bound that is a no-op on normal displays.
  C remains the documented next lever for very weak hardware.

- **Reduce `KIOSK_DEFAULT_MATCH_COUNT` from 4 to 2.** Halves Multicam load
  trivially. Rejected as a default because it reduces the exhibition's visual
  appeal (half as many live battles); kept as a documented emergency lever for a
  specific underpowered device.

- **Render the whole frame into one global down-scaled RenderTexture.** A single
  global cap instead of per-quadrant FBO capping. Rejected as more invasive: it
  touches every app state (not just Kiosk) and complicates mouse-coordinate
  mapping in the interactive states, contradicting the minimal-risk goal.
