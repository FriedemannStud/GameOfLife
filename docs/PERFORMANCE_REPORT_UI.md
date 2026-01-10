# Performance Analysis: Visual Upgrade "Digital Lab"

**Date:** 2026-01-09
**Target:** Raylib Implementation in `gui.c`

## 1. Proposed Changes
1.  **Dark Mode:** Change background from `RAYWHITE` to Dark Grey (`#1a1a1a`).
2.  **Neon Cells:** Change cell colors to bright Cyan/Magenta.
3.  **Cell Padding:** Draw cells slightly smaller than grid slots to create visual gaps.
4.  **HUD Overlay:** Draw static header/footer bars.
5.  **Ghost Cursor:** Highlight cell under mouse in Editor.

## 2. Impact Analysis

### A. Background & Colors
*   **Technical Detail:** `ClearBackground(COLOR)` operates on the framebuffer.
*   **Cost:** O(1). Constant time operation.
*   **Verdict:** No performance impact.

### B. Cell Rendering (Padding vs. Lines)
*   **Current State:** Cells are drawn filling the slot. Grid lines are drawn over them using `DrawLine` loops (Rows + Cols iterations).
*   **Proposed State:** Cells are drawn with a 1px inset (padding). The background color creates the "grid lines".
*   **Math:** Calculating padding (`x+1`, `w-2`) is trivial integer arithmetic (CPU).
*   **Draw Calls:**
    *   *Current:* N Cells + (Rows + Cols) Lines.
    *   *Proposed:* N Cells.
*   **Verdict:** **Performance Improvement.** Eliminating the `DrawLine` calls reduces the number of instructions sent to the GPU, especially for large grids (e.g., 200x200 grid = 400 fewer draw calls per frame).

### C. HUD Overlays
*   **Technical Detail:** Drawing 2 large static rectangles.
*   **Cost:** Negligible (2 primitives).
*   **Verdict:** Safe.

### D. Ghost Cursor
*   **Technical Detail:** Calculating mouse index (already done for logic) + 1 `DrawRectangle` call with Alpha blending.
*   **Cost:** Negligible. Only active in Editor mode.
*   **Verdict:** Safe.

## 3. Conclusion
The proposed design changes adhere to the strict performance requirements. By replacing explicit grid lines with negative space (padding) during the simulation phase, we reduce the rendering load while improving aesthetics.

**Approved for Implementation.**
