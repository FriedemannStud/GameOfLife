# DEV_TASKS-0018: Multicam Render Context Architecture

**Developer:** Please follow these steps precisely. This is a highly invasive refactoring phase. Adhere strictly to Rule 10 (Context-Aware Refactoring) from `CODING_STYLE.md`. The plan is broken into phases to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality.

**Briefing Documents:**
*   [ADR-0018: Multicam Render Context Architecture](../adr/ADR-0018-multicam-render-context-architecture.md)
*   [DEV_SPEC-0018: Multicam Render Context Architecture](../specs/DEV_SPEC-0018-multicam-render-context-architecture.md)
*   [DEV_TECH_DESIGN-0018: Multicam Render Context Architecture](../tech_design/DEV_TECH_DESIGN-0018-multicam-render-context-architecture.md)

---

## Phase 1: Core Logic Refactoring (`SimulationContext`)

*Goal: Eliminate global state from the simulation engine (`game_logic.c`).*

- [x] **Step 1.1: Define `SimulationContext`**
    - [x] **Action:** Open `src/core/core_types.h`.
    - [x] **Action:** Define the `SimulationContext` struct exactly as specified in `DEV_TECH_DESIGN-0018` (Section 3.1).
    - [x] **Action:** Add prototypes for `init_simulation_context` and `free_simulation_context`.

- [x] **Step 1.2: Implement Constructor/Destructor**
    - [x] **Action:** Open `src/core/game_logic.c`.
    - [x] **Action:** Implement `init_simulation_context`. It MUST allocate memory for `grid_buffer_a` and `grid_buffer_b` based on `rows * cols`. Initialize pointers (`current_grid`, `next_grid`).
    - [x] **Action:** Implement `free_simulation_context`. It MUST `free()` both buffers and set pointers to `NULL`.

- [x] **Step 1.3: Identify Global Variables**
    - [x] **Action:** In `src/core/game_logic.c`, identify all `static` or global arrays representing the grid and generation counters.
    - [x] **Action:** **Delete or comment out** these global variables. This will intentionally break the build.

- [x] **Step 1.4: Refactor Signatures (Context-Aware)**
    - [x] **Action:** For every compilation error in `game_logic.c`, update the function signature to accept a `SimulationContext* ctx`.
    - [x] **Action:** Replace references to the old global variables with `ctx->current_grid`, `ctx->current_generation`, etc.
    - [x] **Action:** Do the same for `src/core/game_logic.h`. Update all prototypes.

- [x] **Step 1.5: Fix Upstream Callers (Single Instance)**
    - [x] **Action:** Open `src/apps/gui/main.c` (or wherever the state machine is driven).
    - [x] **Action:** Instantiate a single `SimulationContext global_sim;` temporarily. Call `init_simulation_context` at startup and `free_simulation_context` at exit.
    - [x] **Action:** Update all calls to game logic functions (e.g., in `app_state_manager.c`) to pass `&global_sim`.
    - [x] **Verification:**
        1.  Run: `make clean && make`
        2.  **Expected Result:** The code compiles successfully without warnings.

---

## Phase 2: Renderer Refactoring (`RenderContext`)

*Goal: Eliminate global Raylib textures and buffers from `renderer.c`.*

- [x] **Step 2.1: Define `RenderContext`**
    - [x] **Action:** Open `src/gui/renderer.h`.
    - [x] **Action:** Include `"../core/core_types.h"`.
    - [x] **Action:** Define the `RenderContext` struct as specified in `DEV_TECH_DESIGN-0018` (Section 3.2).
    - [x] **Action:** Add prototypes for `init_render_context` and `free_render_context`.

- [x] **Step 2.2: Implement Constructor/Destructor**
    - [x] **Action:** Open `src/gui/renderer.c`.
    - [x] **Action:** Implement `init_render_context`. It MUST allocate `pixel_buffer` and create the Raylib `Texture2D` using `LoadTextureFromImage` or equivalent. Set the `viewport_bounds`.
    - [x] **Action:** Implement `free_render_context`. It MUST call `UnloadTexture(ctx->grid_texture)` and `free(ctx->pixel_buffer)`.

- [x] **Step 2.3: Remove Global Renderer State**
    - [x] **Action:** In `renderer.c`, delete or comment out the global `gridTex` and the global pixel buffer. (Leave the global Shader intact for now, as it can be shared).
    - [x] **Action:** This will break the build.

- [x] **Step 2.4: Refactor Renderer Signatures**
    - [x] **Action:** Update `update_gpu_texture(RenderContext* r_ctx, const SimulationContext* s_ctx)`. It must read from `s_ctx->current_grid`, map colors into `r_ctx->pixel_buffer`, and call `UpdateTexture(r_ctx->grid_texture, ...)`.
    - [x] **Action:** Update `draw_grid(const RenderContext* r_ctx)`. It must use `DrawTextureRec` or `DrawTexturePro` to render the texture strictly within `r_ctx->viewport_bounds`.

- [x] **Step 2.5: Fix Upstream Callers (Single Instance)**
    - [x] **Action:** In `src/apps/gui/main.c`, instantiate a single `RenderContext global_render;`.
    - [x] **Action:** Call `init_render_context` with fullscreen bounds at startup. Call `free_render_context` at exit.
    - [x] **Action:** Update all drawing calls in the state machine to pass `&global_render`.
    - [x] **Verification (Interactive Test):**
        1.  Run: `make clean && make`
        2.  Run the application. Start a standard single-player simulation.
        3.  **Expected Result:** The game looks and plays exactly as it did before the refactoring. Visually, nothing has changed, but the architecture is now object-oriented.

---

## Phase 3: Multicam 2x2 Implementation (Proof of Concept)

*Goal: Utilize the new architecture to run 4 independent simulations on screen.*

- [x] **Step 3.1: Instantiate Arrays**
    - [x] **Action:** In `src/gui/app_state_manager.c` (or a new testing state), allocate `SimulationContext sims[4]` and `RenderContext renders[4]`.

- [x] **Step 3.2: Initialize 2x2 Grid**
    - [x] **Action:** Calculate 4 distinct `Rectangle` bounds dividing the screen equally (top-left, top-right, bottom-left, bottom-right).
    - [x] **Action:** Initialize all 4 `sims` and all 4 `renders` with these bounds. (Populate them with random noise or gliders to ensure they are active).

- [x] **Step 3.3: The Multicam Loop**
    - [x] **Action:** In the update loop, iterate `for (int i=0; i<4; i++)`: call `update_generation(&sims[i])` and `update_gpu_texture(&renders[i], &sims[i])`.
    - [x] **Action:** In the draw loop, iterate `for (int i=0; i<4; i++)`: call `draw_grid(&renders[i])`.

- [x] **Step 3.4: Verification (Interactive Test)**
    - [x] **Verification:**
        1.  Compile and run the application. Navigate to the state where the 2x2 grid is active.
        2.  **Expected Result:** You see four completely independent Game of Life simulations running simultaneously in the four quadrants of the screen.

---

## Phase 4: Memory Safety and Polish

*Goal: Ensure no memory or VRAM leaks exist before moving to Kiosk UI.*

- [x] **Step 4.1: Valgrind Check**
    - [x] **Action:** Compile the application with debug symbols (`-g`).
    - [x] **Verification (Interactive Test):**
        1.  Run: `valgrind --leak-check=full ./build/biotope`
        2.  Let the 2x2 Multicam run for a few seconds.
        3.  Close the application cleanly.
        4.  **Expected Result:** Valgrind reports that all blocks allocated for `SimulationContext` buffers and `RenderContext` pixel buffers were freed. No VRAM leaks related to missing `UnloadTexture` calls.

- [x] **Step 4.2: Scissor Mode Implementation (Optional but Recommended)**
    - [x] **Action:** Inside `draw_grid`, wrap the texture drawing logic with `BeginScissorMode(...)` and `EndScissorMode()` using the context's viewport bounds to ensure no artifacts bleed across quadrant lines.

- [x] **Step 4.3: Final Cleanup**
    - [x] **Action:** Review `game_logic.c` and `renderer.c` one last time. Ensure the `// KI-Agent unterstützt` comments are present where significant changes were made.
    - [x] **Action:** Run `make clean && make`.

---
**Completion:** Once Phase 4 is complete, the application's renderer is decoupled from global state and fully capable of driving the Kiosk Mode. Please update `docs/CHANGELOG.md` to note the completion of the Multicam Refactoring.
