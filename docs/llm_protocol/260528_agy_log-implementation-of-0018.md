# Implementation Log: DEV_TASKS-0018 (Multicam Render Context Architecture)
**Date:** May 28, 2026

## Overview
This document summarizes the successful implementation of the Multicam Render Context Architecture (ADR-0018). The primary goal was to decouple the core simulation and rendering logic from global state, enabling independent Game of Life instances to run and render simultaneously in a multi-viewport Kiosk Mode.

## Phase 1: Core Logic Refactoring
- Introduced `SimulationContext` in `core_types.h` to encapsulate `current_world`, `next_world`, and telemetry data.
- Refactored `game_logic.c` functions (e.g., `update_generation`) to accept `SimulationContext*` instead of relying on global instances.
- Ensured legacy single-player modes successfully integrated the new context pattern.

## Phase 2: Renderer Refactoring
- Introduced `RenderContext` to encapsulate `pixel_buffer`, FBOs (`ping_pong_target`), grid textures, and viewport bounds.
- Refactored `DrawGridAndCellsCtx` in `renderer.c` to accept `RenderContext*`.
- Successfully decoupled rendering bounds from the main window bounds, laying the groundwork for split-screen layouts.

## Phase 3: Multicam 2x2 PoC Implementation
- Created `STATE_KIOSK_MODE` in `main.c`.
- Initialized an array of four `SimulationContext` and four `RenderContext` instances, mapping each to a distinct quadrant of the screen.
- Configured a 10 FPS update loop that steps all four simulations independently.

### Bug Fix: Identical Random Seeds
- **Issue:** The 2x2 grid displayed the exact same simulation in all four quadrants.
- **Root Cause:** `srand(time(NULL))` was being called inside `init_world`. Since all four quadrants initialized in the same second, they received identical random seeds.
- **Resolution:** Removed `srand` from `init_world` and relocated it to run exactly once at the beginning of `main()`. This ensured the random sequence progressed sequentially, providing unique initial states for each quadrant.

## Phase 4: Memory Safety and Polish
- Implemented `BeginScissorMode()` / `EndScissorMode()` inside the drawing routines to prevent any texture bleeding between the 2x2 quadrants during camera pan/zoom operations.
- Performed rigorous memory leak verification using `valgrind --leak-check=full`.

### Bug Fix: Context Memory Leaks
- **Issue:** The application leaked the instances of `kiosk_sims` and `kiosk_renders` if the user closed the application window directly while Kiosk Mode was active.
- **Root Cause:** The cleanup routines for the Kiosk arrays were only mapped to the `[ESC]` key (returning to the config state).
- **Resolution:** Added a cleanup check at the end of `main()` to iterate and call `free_simulation_context` and `free_render_context` for the Kiosk arrays if `kiosk_initialized` was true during shutdown.

### Valgrind Verification
- Subsequent Valgrind analysis confirmed that our C implementation generated **0 bytes** of memory leaks.
- *Note:* A residual leak reported by Valgrind (`969 bytes in 4 blocks`) was traced precisely to `_glfwInitGLX` and `extensionSupportedGLX`. This was verified as an external driver-level issue originating from the Mesa/GLX stack on X11, completely independent of our codebase.

## Final Result
- All tasks in `DEV_TASKS-0018` have been successfully completed.
- The `CHANGELOG.md` has been updated.
- The application now supports robust, decoupled, and memory-safe multi-viewport simulations.
