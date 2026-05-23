# Requirements Analysis & Specification: Architectural Consolidation and Modularization

This document details the requirements for consolidating redundant logic and modularizing the application structure, as described in **ADR-0015**.

---

### 1. Detailed Requirements Specification

The consolidation effort focuses on four key areas to reduce technical debt and improve maintainability:

1.  **IO Centralization:**
    *   Migrate all JSON parsing logic (using `cJSON`) from `main.c`, `main_headless.c`, and `main_hyper.c` to `file_io.c`.
    *   Expose generic functions for `load_config_from_json`, `save_result_to_json`, and `initialize_world_from_file`.
    *   Ensure all entry points use these centralized functions to maintain consistency in data handling.

2.  **GUI Decomposition:**
    *   Extract the application state machine logic from `gui.c` into `app_state_manager.c`.
    *   Isolate Raylib-specific rendering code (HUD, grid drawing, menus) into `renderer.c`.
    *   Define a clear interface where the state manager triggers renders and the renderer reports user interactions back to the state manager.

3.  **Global Type Standardization:**
    *   Create `core_types.h` to house `GameConfig`, `World`, `AppState`, `Team`, and `MatchResult`.
    *   Remove these definitions from `gui.h` and `game_logic.h` to prevent circular dependencies.
    *   Update all modules to include `core_types.h` instead of local or cross-module headers for these types.

4.  **Configuration & Parameter Localization:**
    *   Move all hardcoded constants (e.g., default simulation rules, colors, layout offsets) into a new `config.h`.
    *   Standardize grid dimension handling so that "magic numbers" like 8x16 chunks are defined in exactly one place.

---

### 2. User Stories & Acceptance Criteria

**Epic: Unified Project Architecture**

*   **User Story 1: Centralized IO for all Entry Points**
    *   **As a developer,** I want all file and JSON operations to be handled by a single module, **so that** changes to the data format only need to be implemented once.
    *   **Acceptance Criteria:**
        *   `main_headless.c` and `main_hyper.c` no longer contain `cJSON` calls.
        *   A single change to `file_io.c` correctly propagates to GUI, Headless, and Hyper modes.
        *   Persistence functions are documented and return clear error codes.

*   **User Story 2: Decoupled UI and Application Logic**
    *   **As a developer,** I want the application state logic to be separate from the rendering code, **so that** I can change the UI framework or run simulations without a graphics context more easily.
    *   **Acceptance Criteria:**
        *   `app_state_manager.c` contains no Raylib drawing calls (e.g., `DrawText`, `DrawRectangle`).
        *   `renderer.c` only handles visual output and input polling, calling state manager functions to update application status.
        *   The main loop in `main.c` is simplified to calling `UpdateAppState()` and `DrawCurrentState()`.

*   **User Story 3: Clean Dependency Graph**
    *   **As a developer,** I want to include headers without worrying about circular dependencies, **so that** compilation is fast and predictable.
    *   **Acceptance Criteria:**
        *   `core_types.h` exists and is the "Source of Truth" for all shared structs.
        *   No header file includes another header file that includes it back (directly or indirectly).
        *   The project compiles with `make` without warnings.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Creation of `core_types.h` and migration of shared structs.
        *   Centralization of IO/JSON logic into `file_io.c`.
        *   Elimination of logic duplication in `main_headless.c` and `main_hyper.c`.
    *   **Should-Have:**
        *   Decomposition of `gui.c` into `app_state_manager.c` and `renderer.c`.
        *   Creation of `config.h` for centralized constants.
    *   **Could-Have:**
        *   Unit tests for `app_state_manager.c` using a mock renderer.
        *   Automated verification script for header dependency health.
    *   **Won't-Have (in this increment):**
        *   Migration to a different UI framework (Raylib remains).

*   **Dependencies:**
    1.  **Types first:** `core_types.h` must be implemented before modules can be decoupled.
    2.  **IO second:** Centralizing IO is required to clean up the secondary entry points (`headless`, `hyper`).
    3.  **UI last:** GUI decomposition is the most complex task and depends on stable types and IO interfaces.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| T15.1 | Unified Architecture | Implement `core_types.h` and refactor imports | Must |
| T15.2 | Unified Architecture | Move JSON/File logic from `main_headless/hyper` to `file_io.c` | Must |
| T15.3 | Unified Architecture | Create `config.h` and centralize "magic numbers" | Should |
| T15.4 | Unified Architecture | Split `gui.c` into `app_state_manager.c` and `renderer.c` | Should |
| T15.5 | Unified Architecture | Simplify `main.c` loop to use new decoupled modules | Should |

---

### 5. Definition of Done (DoD)

A Product Backlog Item is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The code follows `docs/CODING_STYLE.md` (snake_case, AI attribution comments).
*   **Zero Warnings:** The project compiles with `make` (using `-Wall -Wextra`) without any warnings.
*   **Consistency:** All three entry points (GUI, Headless, Hyper) function correctly with the refactored code.
*   **Verification:**
    *   `run_test_suite.py` passes completely for the Hyper worker.
    *   Manual verification of GUI state transitions (Config -> Running -> Finished).
*   **Documentation:** `CHANGELOG.md` is updated and any new files are added to the directory structure overview in `docs/`.

#### **AI Attribution**
// KI-Agent unterstützt: Requirement analysis and specification generation.
