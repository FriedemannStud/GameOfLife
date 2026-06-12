# DEV_TASKS-0006: GPU Aesthetic Overhaul via Shader Pipeline

This document breaks down the implementation of the GLSL Shader Pipeline and the "Living Petri Dish" visual overhaul into actionable, verifiable steps for a full-stack C/Graphics developer.

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality and adhering to `docs/CODING_STYLE.md`.

**Briefing Documents:**
*   [ADR-0006: GPU Aesthetic Overhaul via Shader Pipeline](../adr/ADR-0006-gpu-aesthetic-overhaul.md)
*   [DEV_SPEC-0006: GPU Aesthetic Overhaul via Shader Pipeline](../specs/DEV_SPEC-0006-gpu-aesthetic-overhaul.md)
*   [DEV_TECH_DESIGN-0006: GPU Aesthetic Overhaul via Shader Pipeline](../tech_design/DEV_TECH_DESIGN-0006-gpu-aesthetic-overhaul.md)

---

## Phase 1: GLSL Shader Foundation & Data Pipeline

*Goal: Establish the basic shader file, load it via Raylib, and successfully transfer the C integer grid to the GPU as a 1-byte grayscale texture. (Visual parity with the old system).*

- [x] **Step 1.1: Create Shader Files**
    - [x] **Action:** Create a directory `resources/shaders/` in the project root.
    - [x] **Action:** Create a file `biotope_base.fs` (Fragment Shader).
    - [x] **Action:** Implement a basic GLSL 100 compatible shader that reads `texture0` (a grayscale texture) and outputs `COLOR_RED` if the value is > 0.9, `COLOR_BLUE` if > 0.4, else `COLOR_BG`. (See DEV_TECH_DESIGN Section 4.1 for pseudocode).
    - [x] **Verification:** Completed. Native build uses GLSL 330, Web build uses GLSL 100.

- [x] **Step 1.2: Refactor `gui.c` State Initialization**
    - [x] **Action:** Open `gui.c`. At the global or `run_gui_app` initialization level, define a `Shader` variable.
    - [x] **Action:** Use `LoadShader(0, "resources/shaders/biotope_base.fs")` to load the shader into memory during startup.
    - [x] **Action:** Allocate a global or static `unsigned char *gpu_data_buffer` sized to `config.cols * config.rows`.
    - [x] **Action:** Modify the texture initialization inside `DrawGridAndCells` (or move it to startup). Create `Image img = GenImageColor(config.cols, config.rows, BLANK);` and force `ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);` before loading it into `gridTex`.
    - [x] **Verification:** Completed. No segfaults.

- [x] **Step 1.3: Connect the Data Pipeline**
    - [x] **Action:** In `DrawGridAndCells` (inside `gui.c`), replace the loop that fills the `Color *pixels` array.
    - [x] **Action:** Write a new loop that iterates over `World->grid` (skipping the ghost border padding) and populates the `gpu_data_buffer`. If `DEAD` -> 0, `TEAM_BLUE` -> 127, `TEAM_RED` -> 255.
    - [x] **Action:** Use `UpdateTexture(gridTex, gpu_data_buffer)` instead of the old Color array.
    - [x] **Action:** Wrap the `DrawTexturePro(gridTex, ...)` call with `BeginShaderMode(biotopeShader)` and `EndShaderMode()`.
    - [x] **Verification:** Completed. Visual parity confirmed.

---

## Phase 2: Ping-Pong Render Targets (Temporal Trails)

*Goal: Implement two off-screen framebuffers (RenderTexture2D) to create the "Fossils" fading effect.*

- [x] **Step 2.1: Initialize Render Textures**
    - [x] **Action:** In `gui.c`, define an array `RenderTexture2D pingPongTarget[2]` and an integer `pingPongIndex = 0`.
    - [x] **Action:** During window initialization or when grid size changes, initialize both targets using `LoadRenderTexture(drawWidth, drawHeight)`. *Note: Ensure these are sized to the drawing area on the screen, not the grid dimensions.*
    - [x] **Verification:** Completed. Fixed Y-axis mirroring artifact in shader.

- [x] **Step 2.2: Setup Shader Uniforms**
    - [x] **Action:** In `gui.c`, get the shader locations for the uniforms: `locPrevFrame = GetShaderLocation(biotopeShader, "previousFrame")` and `locFadeRate = GetShaderLocation(biotopeShader, "fadeRate")`.
    - [x] **Action:** In `biotope_base.fs`, ensure the uniforms `uniform sampler2D previousFrame;` and `uniform float fadeRate;` are defined.
    - [x] **Verification:** Completed.

- [x] **Step 2.3: Implement the Ping-Pong Render Loop**
    - [x] **Action:** In `DrawGridAndCells`, implement the following flow (replacing the direct `DrawTexturePro` to the screen):
        1.  `BeginTextureMode(pingPongTarget[pingPongIndex])`
        2.  `BeginShaderMode(biotopeShader)`
        3.  Bind the previous frame's texture to the shader: `SetShaderValueTexture(biotopeShader, locPrevFrame, pingPongTarget[1 - pingPongIndex].texture)`
        4.  Set the fade rate: `float fade = 0.95f; SetShaderValue(biotopeShader, locFadeRate, &fade, SHADER_UNIFORM_FLOAT);`
        5.  `DrawTexturePro(gridTex, ...)` (Draws the raw grid data through the shader onto the current FBO).
        6.  `EndShaderMode()`
        7.  `EndTextureMode()`
    - [x] **Action:** Finally, draw the result to the screen: `DrawTextureRec(pingPongTarget[pingPongIndex].texture, ...)` (Remember that RenderTexture2D Y-axis is flipped in OpenGL, so the source rectangle height might need to be negative).
    - [x] **Action:** Swap the buffers: `pingPongIndex = 1 - pingPongIndex;`
    - [x] **Verification:** Completed.

- [x] **Step 2.4: Update Shader Fading Logic**
    - [x] **Action:** Update `biotope_base.fs` to implement the temporal fading logic (Section 4.1 in DEV_TECH_DESIGN). Mix the `currentColor` with `prevColor * fadeRate` when a cell is dead.
    - [x] **Verification:** Completed. Gliders leave a fading trail.

---

## Phase 3: Aesthetic Polish (Smoothing & Cleanup)

*Goal: Finalize the "Living Petri Dish" look by removing sharp pixel edges and cleaning up memory.*

- [x] **Step 3.1: Fragment Shader Smoothing (Optional/Bonus — bewusst übersprungen)**
    - [~] **Action:** Bewusst nicht umgesetzt — User entschied sich für Performance-Priorität. Die bestehenden Trails sind ausreichend.
    - [x] **Verification:** User chose to skip this to prioritize performance. Current "Trails" are sufficient.

- [x] **Step 3.2: Memory Leak Check**
    - [x] **Action:** Ensure that in `gui.c`, whenever the grid size changes or the application closes, `UnloadRenderTexture()`, `UnloadTexture()`, and `UnloadShader()` are called appropriately to prevent VRAM leaks.
    - [x] **Action:** Ensure `free(gpu_data_buffer)` is called on exit.
    - [x] **Verification:** Completed.

---

*Developer: GPU rendering pipeline is active, establishing the "Living Petri Dish" aesthetic. Feature Complete.*
