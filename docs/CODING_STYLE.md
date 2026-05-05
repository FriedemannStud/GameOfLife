# Coding Style and Clean Code Principles (C / Raylib)

This document defines the binding code quality and style guidelines for the Biotope GameOfLife project. All code changes, especially those made by AI agents, MUST strictly follow these rules.

## 1. Language

- **Rule:** All code, comments, and docstrings MUST be written in **English**. This ensures international comprehensibility.

## 2. Formatting

- **Rule:** All C code SHOULD be formatted consistently.
- **Instruction:** Use a consistent indentation (4 spaces). Ensure there is no trailing whitespace. 

## 3. Linter / Static Analysis

- **Rule:** The code MUST compile without warnings using the project's Makefile settings.
- **Instruction:** Run `make` and ensure `gcc` flags `-Wall -Wextra` report no issues. Use `cppcheck` for deeper static analysis if available.

## 4. Naming Conventions

- **Rule:** Names MUST be meaningful and unambiguous.
- **Instruction:**
  - **Variables & functions:** `snake_case` (e.g., `current_gen`, `calculate_population`).
  - **Structs & Types:** `PascalCase` (e.g., `World`, `GameConfig`).
  - **Constants & Macros:** `UPPER_SNAKE_CASE` (e.g., `MAX_ROWS`, `TEAM_RED`).
- **Examples:**
  - **BAD:** `w`, `arr`, `proc()`, `t`
  - **GOOD:** `world`, `cell_grid`, `update_generation()`, `temperature_celsius`

## 5. Functions and Modularity

- **Rule 1 (Single Responsibility):** Each function SHOULD perform only one task.
- **Rule 2 (Length):** Functions SHOULD be concise. Long functions (e.g., > 50 lines) should be decomposed.
- **Rule 3 (Arguments):** AVOID functions with more than 4 arguments. Pass a pointer to a struct instead (e.g., `GameConfig *config`).

## 6. Comments and AI Attribution

- **Rule 1 (Self-Explanatory):** Code should be readable. Only comment *why*, not *what*.
- **Rule 2 (AI Attribution):** ALL code generated or significantly modified by an AI agent MUST be marked with the comment `// KI-Agent unterstützt`.
- **Instruction:**
  - **GOOD:** `// KI-Agent unterstützt: Optimized pointer swapping to avoid O(N) copy`

## 7. Memory Management (Critical in C)

- **Rule 1 (DRY/RAII-ish):** Always pair `malloc`/`calloc` with `free`. Use constructor/destructor patterns (e.g., `create_world` / `free_world`).
- **Rule 2 (Performance):** Avoid dynamic memory allocation inside the hot simulation loop. Use pre-allocated buffers (Double Buffering / Ping-Pong).

## 8. Error Handling

- **Rule:** Use return codes (e.g., `bool` or `int` status) or state flags. AVOID leaving the program in an undefined state.
- **Instruction:** Check the return value of `fopen`, `malloc`, etc.

## 9. The Boy Scout Rule

- **Rule:** Leave the code cleaner than you found it. Improve a variable name or fix a typo even if it's not your main task.

## 10. Context-Aware Refactoring

**Rule:** To prevent broken references, follow this workflow for every change to existing code:

**Phase 1: Analysis**
1.  **Identification:** Identify the name of the element (e.g., function `load_grid`, struct `World`).
2.  **Global search:** Perform a project-wide search for all occurrences.
3.  **Analysis of references:** Analyze EVERY result. Create a checklist of affected locations.

**Phase 2: Implementation**
1.  **Atomic change:** Implement the change to the definition AND all usage locations.
2.  **Verification:** Recompile with `make` and run tests/verification scripts to ensure no regressions.

