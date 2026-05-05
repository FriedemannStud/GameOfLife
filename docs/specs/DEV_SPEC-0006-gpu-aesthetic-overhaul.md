# Requirements Analysis & Specification: GPU Aesthetic Overhaul via Shader Pipeline

This document details the requirements for the visual transformation of the game from a static grid to a fluid, procedural aesthetic, as described in **ADR-0006**.

---

### 1. Detailed Requirements Specification

The primary objective is to implement a custom GPU shader pipeline (GLSL) using Raylib to render the "Biotope" simulation. This will decouple the visual presentation from the raw integer grid calculated by the CPU, allowing for high-performance visual effects ("Juiciness") necessary to attract the indie market, without compromising the OpenMP simulation speed.

**Technical Constraints & Requirements:**
*   **Data Transfer:** The CPU must pass the raw `World` state (an array of integers representing Dead, Red, or Blue) to the GPU as efficiently as possible, likely encoded as a 1D or 2D raw data texture.
*   **GLSL Compatibility:** The shaders must be written in a version of GLSL that is compatible with both native desktop OpenGL (OpenGL 3.3) and WebGL (GLSL 100 or 300 ES) to ensure the WASM port (ADR-0005) is not broken.
*   **The "Three Layers of Juiciness":**
    1.  **Interpolation/Smoothing:** The hard square edges of cells must be visually smoothed using techniques like metaballs or marching squares within the fragment shader.
    2.  **Temporal Fading (Trails):** Dead cells must leave a slowly decaying visual echo (e.g., a fading white/grey footprint) to visualize the history of movement (Fossils & Heatmaps). This requires passing data from the previous frame to the shader.
    3.  **Bloom/Glow:** High-density areas of living cells must emit a localized glow, emphasizing the intensity of "battles."

---

### 2. User Stories & Acceptance Criteria

**Epic: The Living Petri Dish (Visual Overhaul)**

*   **User Story 1: Organic Cell Rendering**
    *   **As a player,** I want the cells to look like connected, organic matter rather than rigid squares, **so that** the simulation feels like a living biological entity.
    *   **Acceptance Criteria:**
        *   A custom fragment shader replaces the default `DrawTexturePro` pixel drawing.
        *   Adjacent cells of the same color visually "merge" without sharp corners.
        *   The shader successfully reads the raw data texture passed from the CPU.

*   **User Story 2: Temporal Trails (Fossils)**
    *   **As an observer,** I want to see a fading trail where cells have recently died, **so that** I can track the history and movement patterns of the simulation over time.
    *   **Acceptance Criteria:**
        *   The shader pipeline retains a memory of the previous frame's color data.
        *   When a cell transitions from living (Red/Blue) to dead, it leaves a glowing footprint that fades to black over N frames.
        *   The fade rate is adjustable via a shader uniform variable.

*   **User Story 3: Density-Based Bloom**
    *   **As a player,** I want areas with many active cells to glow intensely, **so that** major clashes and complex oscillators look visually explosive and exciting.
    *   **Acceptance Criteria:**
        *   A post-processing bloom effect is applied over the rendered grid.
        *   The intensity of the glow correlates with the density of living cells in a specific area.

**Epic: Performance and Compatibility**

*   **User Story 4: 60 FPS on Epic Scale**
    *   **As a power user,** I want the shader effects to remain smooth even on a 2000x2000 grid, **so that** the "Epic Scale" battles remain playable.
    *   **Acceptance Criteria:**
        *   The CPU loop no longer iterates over the grid to assign colors to a CPU-side pixel array.
        *   The frame rate on a 2000x2000 grid natively is stable at 60fps (or CPU bound by the simulation, not GPU bound by the drawing).

*   **User Story 5: WebGL Parity**
    *   **As a web player,** I want to see the exact same shaders in the browser, **so that** the WASM version is visually identical to the native version.
    *   **Acceptance Criteria:**
        *   The shaders compile without errors using Emscripten.
        *   The web build runs with the shader effects active in a standard browser.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Setup Raylib `Shader` loading and application infrastructure in `gui.c`.
        *   Pass grid data to the shader as a raw texture.
        *   Basic fragment shader that correctly maps the integer data to Red, Blue, and Black (replaces current CPU pixel pushing).
    *   **Should-Have:**
        *   Temporal fading (Trails/Fossils) implemented in the shader.
        *   WebGL / GLSL ES compatibility.
    *   **Could-Have:**
        *   Cellular Automata smoothing (Metaballs/Marching Squares).
        *   Post-processing Bloom effect.
    *   **Won't-Have (in this increment):**
        *   Audio-reactive shaders (syncing glow to the "Sonic Ecosystem").
        *   Customizable shader palettes in the UI.

*   **Dependencies:**
    1.  **Topic: WASM Compatibility:** The shader GLSL version must be chosen carefully to align with the Emscripten build pipeline established in DEV_SPEC-0004.
    2.  **Topic: Double Buffering:** The shader implementation must interact correctly with the `current_gen` and `next_gen` pointer swapping defined in ADR-0004.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| TSK-01 | Visual Overhaul | Create basic `biotope.fs` (Fragment Shader) and `biotope.vs` (Vertex Shader) files | High |
| TSK-02 | Visual Overhaul | Refactor `gui.c` to load and apply the custom Shader using Raylib `LoadShader` | High |
| TSK-03 | Visual Overhaul | Implement CPU-to-GPU data transfer (convert `World->grid` to a raw Texture/Buffer for the shader) | High |
| TSK-04 | Visual Overhaul | Remove old CPU-side pixel calculation loop in `DrawGridAndCells` | High |
| TSK-05 | Visual Overhaul | Implement Temporal Fading (Trails) logic in GLSL (requires passing previous frame texture) | Medium |
| TSK-06 | Compatibility | Test and adjust shaders for WebGL (GLSL 100) compatibility | Medium |
| TSK-07 | Visual Overhaul | Implement Cell Smoothing (Metaball heuristic) in GLSL | Low |
| TSK-08 | Visual Overhaul | Implement Post-Processing Bloom pass | Low |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The C code and GLSL shader code are formatted properly. AI attribution comments are present where applicable.
*   **Compilation:** The project compiles natively (`make`) and for the web (`make -f Makefile.web`) without shader compilation errors.
*   **Execution:** The game runs and visually renders the Red/Blue cells correctly using the GPU pipeline. The old CPU-side `pixels` array is completely removed.
*   **Tests:**
    *   Performance regression test: Verify FPS on a large grid (e.g., 1000x1000) is equal to or better than the previous CPU-rendering method.
    *   Visual test: Verify Trails/Fossils render correctly and fade over time.
*   **Acceptance Criteria:** All acceptance criteria defined for the story have been met.
*   **Code Review:** The implementation has been reviewed for memory leaks (especially regarding Raylib texture and shader unloading).
*   **Documentation:** `CHANGELOG.md` is updated. Any new required dependencies for shader compilation (if applicable) are documented.