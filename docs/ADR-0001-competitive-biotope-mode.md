### ADR-0001: Competitive Biotope Mode (Red vs Blue)

**Status:** Proposed

**Date:** 2026-01-08

#### 1. Context and Problem Statement

The current project implements a standard, zero-player version of Conway's Game of Life. A new requirement ("Bonus: Interaktiver Wettbewerb um das nachhaltigste Zell-Biotop") calls for a competitive, interactive variation of the game.

In this "Biotope" mode:
- Two teams (Red and Blue) compete on the same infinite (toroidal) grid.
- Players define the initial state.
- Standard Conway rules apply for birth and death.
- **New Rule:** When a new cell is born (3 neighbors), it adopts the "team color" of the majority of its 3 parents.
- Winning condition is based on the count of living cells per team after a fixed number of generations.

The challenge is to integrate this logic into the existing high-performance C implementation without sacrificing efficiency or completely rewriting the core engine. We need to define how the grid state represents teams and how the evolution logic handles color inheritance.

#### 2. Decision

We will extend the existing integer-based grid system to support multi-state values representing team affiliation.

**Specifics:**
1.  **Data Representation:** 
    - The `World` structure's `int *grid` will retain its format.
    - Cell values will be defined as constants:
        - `0`: DEAD
        - `1`: TEAM_RED
        - `2`: TEAM_BLUE
    - This avoids the overhead of complex structs (`struct Cell { ... }`) and keeps memory access contiguous and cache-friendly.

2.  **Core Logic Extension (`update_generation`):**
    - The neighbor counting logic will be updated to count neighbors *per team* (e.g., `red_neighbors` and `blue_neighbors`).
    - **Survival Logic:** Total neighbors (`red + blue`) determines survival (2 or 3). A surviving cell retains its current value (1 or 2).
    - **Birth Logic:** If total neighbors == 3, a new cell is born. The new value is determined by comparing `red_neighbors` vs `blue_neighbors`.
        - If `red_neighbors > blue_neighbors` -> New cell is `TEAM_RED`.
        - Otherwise -> New cell is `TEAM_BLUE`.

3.  **Visualization:**
    - The `print_world` function will be updated to distinguish teams visually (e.g., using ANSI color codes or distinct characters like 'X' for Red and 'O' for Blue).

4.  **Game Loop:**
    - Will include a maximum turn limit (`MAX_ROUNDS`).
    - Will implement a final tally of cell values to declare a winner.

#### 3. Consequences of the Decision

**Positive Consequences (Advantages):**
*   **Performance:** Maintains the high-performance characteristics of the original C code by using primitive types and simple arithmetic. No object overhead.
*   **Simplicity:** Reuses the existing memory management and grid traversal architecture.
*   **Extensibility:** Easy to add more teams later (e.g., value `3` for Green) by just changing the comparison logic.

**Negative Consequences (Disadvantages):**
*   **Complexity in Neighbor Counting:** The unrolled loop for neighbor counting in `update_generation` becomes slightly more complex as we need to inspect the *value* of neighbors, not just their existence.
*   **UI Dependency:** The console output must now reliably handle distinct representations for different teams, which might vary across terminal environments (though standard ASCII/ANSI is generally safe).

#### 4. Alternatives Considered

**Alternative A: Struct-based Cells**
*   *Concept:* Change `int *grid` to `Cell *grid` where `typedef struct { bool is_alive; int team_id; } Cell;`.
*   *Reason for Rejection:* Introduces memory alignment padding and potentially higher cache miss rates. For a grid simulation, "Array of Structures" (AoS) is often less performant than "Structure of Arrays" (SoA) or simple primitive arrays for this specific access pattern. The current `int` approach is leaner.

**Alternative B: Separate Bitboards/Grids**
*   *Concept:* Use two separate grids, one for Red positions and one for Blue.
*   *Reason for Rejection:* Makes collision detection (ensuring a cell isn't occupied by both) and neighbor interaction logic cumbersome. A single "Truth" grid prevents inconsistent states where a cell is both Red and Blue.

**Alternative C: Object-Oriented Approach (C++)**
*   *Concept:* Rewrite in C++ with Cell objects.
*   *Reason for Rejection:* Violation of the constraint to use the existing C codebase and maintaining the specific "minimalist C" aesthetic of the project.
