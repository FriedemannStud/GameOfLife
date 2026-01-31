# ADR-0004: Double Buffering Strategy for Grid Memory Management

**Status:** Proposed

**Date:** 2026-01-31

## 1. Context and Problem Statement

The current implementation of the "Biotope" Game of Life simulation handles memory management inefficiently during the simulation phase.

In the `STATE_RUNNING` loop (located in `gui.c`), the application performs the following operations for every single frame/generation:
1.  Calls `create_world` (which invokes `malloc` multiple times) to allocate memory for the `next_gen` grid.
2.  Computes the evolution.
3.  Calls `free_world` (which invokes `free` multiple times) to destroy the old `gui_world`.

**Problem:**
Dynamic memory allocation (`malloc`/`free`) is a relatively expensive operation on the system level. Doing this continuously (up to 60 times per second) results in:
*   Unnecessary CPU overhead managing the heap.
*   Potential memory fragmentation over long runtimes.
*   Increased risk of allocation failures during the simulation.
*   Lower maximum frame rate potential.

We need a solution to optimize the memory access patterns during the simulation loop to ensure high performance and stability, especially for larger grids.

## 2. Decision

We will implement a **Double Buffering** (also known as Ping-Pong) strategy for the simulation grid.

Instead of allocating and freeing memory in every frame, we will:
1.  Allocate **two** persistent `World` instances (let's call them `buffer_A` and `buffer_B`) when the simulation initializes or when the grid size changes.
2.  Maintain two pointers: `current_gen` (read-only source) and `next_gen` (write-only target).
3.  Inside the game loop:
    *   Pass both pointers to `update_generation`.
    *   After the update, **swap the pointers**.
4.  Free both buffers only when the application terminates or when the user reconfigures the grid size.

## 3. Consequences of the Decision

**Positive Consequences (Advantages):**
*   **Performance:** Eliminates the overhead of `malloc` and `free` from the "hot loop" entirely. Cycle time will be determined solely by the calculation logic and rendering.
*   **Stability:** Memory is reserved upfront. If the allocation succeeds at start, the simulation is guaranteed not to run out of memory during execution.
*   **Data Locality:** Keeping the same memory blocks active increases the likelihood of data remaining in the CPU cache (L2/L3), further improving performance.

**Negative Consequences (Disadvantages):**
*   **Memory Footprint:** This approach requires exactly double the memory for the grid data compared to a single grid snapshot (though the previous implementation effectively used double memory momentarily during the transition). Given the `int` data type and typical grid sizes (e.g., 2000x2000), the RAM usage (~16MB to ~32MB) is negligible on modern hardware.
*   **Refactoring Effort:** Requires modifying `gui.c` to manage two persistent pointers instead of one, and updating `game_logic.c` to handle reusable buffers without re-initialization (clearing the `next_gen` buffer is not strictly necessary if every cell is overwritten, but good practice).

## 4. Alternatives Considered

**Alternative A: In-Place Updates (Single Buffer)**
*   *Concept:* Update the `grid` array directly without a second buffer.
*   *Reason for Rejection:* In Conway's Game of Life, the state of a cell at time `t+1` depends on the state of its neighbors at time `t`. Updating a cell in-place would change the neighbor data for the next cell in the loop, corrupting the simulation rules.

**Alternative B: Dirty Rectangles / Sparse Updates**
*   *Concept:* Only allocate/update memory for changed areas.
*   *Reason for Rejection:* Adds significant code complexity to track state changes. While efficient for huge empty grids, it violates the "minimalist architecture" principle for this specific project phase and introduces overhead that might outweigh benefits for dense "Biotope" battles.

**Alternative C: Hashlife (Quadtrees)**
*   *Concept:* Use a complex algorithm to memoize patterns.
*   *Reason for Rejection:* Overkill for the current requirements and introduces drastic architectural complexity unsuited for the existing C codebase structure.
