# DEV_TASKS-0015: Architectural Consolidation and Modularization

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality.

**Briefing Documents:**
*   [ADR-0015: Architectural Consolidation and Modularization](../adr/ADR-0015-architectural-consolidation-and-modularization.md)
*   [DEV_SPEC-0015: Architectural Consolidation](../specs/DEV_SPEC-0015-architectural-consolidation.md)
*   [DEV_TECH_DESIGN-0015: Architectural Consolidation](../tech_design/DEV_TECH_DESIGN-0015-architectural-consolidation.md)

---

## Phase 1: Foundation - Global Types and Configuration

*Goal: Establish the new core headers to prevent circular dependencies before refactoring the logic.*

- [ ] **Step 1.1: Create `core_types.h`**
    - [ ] **Action:** Create a new file `core_types.h` in the project root.
    - [ ] **Action:** Define the include guards (`#ifndef CORE_TYPES_H`, etc.).
    - [ ] **Action:** Include `<stdint.h>` and `<stdbool.h>`.
    - [ ] **Action:** Move the definitions for `AppState`, `GameConfig`, `World`, `Team`, and `MatchResult` from `gui.h` and `game_logic.h` into `core_types.h`.
    - [ ] **Action:** Add `// KI-Agent unterstützt: Refactoring core types` at the top of the file.
    - [ ] **Verification (Compile Test):**
        1. Run `make clean && make`.
        2. Note: It will likely fail because other files still expect these structs in `gui.h` or `game_logic.h`. This is expected. If it fails, proceed to Step 1.2.

- [ ] **Step 1.2: Refactor Imports for `core_types.h`**
    - [ ] **Action:** In `gui.h` and `game_logic.h`, remove the old struct definitions and add `#include "core_types.h"`.
    - [ ] **Action:** In `file_io.h`, `main.c`, `main_headless.c`, and `main_hyper.c`, ensure `#include "core_types.h"` is added where necessary.
    - [ ] **Verification (Compile Test):**
        1. Run `make clean && make`.
        2. **Expected Result:** The project must compile without warnings (`-Wall -Wextra`).

- [ ] **Step 1.3: Create `config.h`**
    - [ ] **Action:** Create a new file `config.h`.
    - [ ] **Action:** Search for hardcoded magic numbers (e.g., chunk dimensions like 8x16 in `main_hyper.c`, default screen widths in `gui.c`, max paths).
    - [ ] **Action:** Move these into `config.h` as `#define` macros (e.g., `#define DEFAULT_WINDOW_WIDTH 1200`).
    - [ ] **Action:** Include `config.h` in the files that need these constants and replace the magic numbers with the macros.
    - [ ] **Verification (Compile Test):**
        1. Run `make clean && make`.
        2. Run the application `./biotope`.
        3. **Expected Result:** The application starts normally, and the grid dimensions/window sizes remain visually identical.

- [ ] **Step 1.4: Establish Foundation DEV_TEST**
    - [ ] **Action:** Create a state-of-the-art C unit test (e.g., in a new `tests/test_core_types.c` file or extending an existing test framework).
    - [ ] **Action:** Verify the correct memory layout, alignment, and default initialization of `World` and `GameConfig` structs.
    - [ ] **Action:** Validate that configuration macros from `config.h` fall within expected, safe boundaries.
    - [ ] **Verification (Test Execution):** Compile and run the new tests, ensuring 100% pass rate and zero memory leaks.

---

## Phase 2: Centralize IO Logic

*Goal: Move all JSON parsing and file operations from the entry points into `file_io.c`.*

- [ ] **Step 2.1: Expand `file_io.h` API**
    - [ ] **Action:** Add the new function signatures to `file_io.h` as defined in DEV_TECH_DESIGN-0015 (e.g., `load_config_from_json`, `initialize_world_from_file`).
    - [ ] **Verification:** Ensure the signatures match exactly and use pointers for `GameConfig` and `World`.

- [ ] **Step 2.2: Implement IO Functions in `file_io.c`**
    - [ ] **Action:** Extract the JSON parsing logic currently residing in `main_headless.c` and `main_hyper.c`.
    - [ ] **Action:** Implement `load_config_from_json` and `initialize_world_from_file` inside `file_io.c` using `cJSON`. Add strict type and array bounds checking.
    - [ ] **Verification (Compile Test):**
        1. Run `make file_io.o` (or just `make` and ignore link errors for now).
        2. **Expected Result:** `file_io.c` compiles without warnings.

- [ ] **Step 2.3: Clean Up Entry Points**
    - [ ] **Action:** In `main_headless.c` and `main_hyper.c`, remove `#include "cJSON.h"` and delete the redundant parsing logic.
    - [ ] **Action:** Replace the deleted logic with calls to the new functions in `file_io.h`.
    - [ ] **Verification (Automated Test):**
        1. Run `make clean && make`.
        2. Run `python3 run_test_suite.py`.
        3. **Expected Result:** The test suite passes completely, proving the headless/hyper workers still load JSON data correctly.

- [ ] **Step 2.4: Establish IO Logic DEV_TEST**
    - [ ] **Action:** Create a state-of-the-art unit test suite for `file_io.c` (e.g., `tests/test_file_io.c`).
    - [ ] **Action:** Mock or provide edge-case JSON inputs (missing fields, out-of-bounds arrays, corrupted JSON strings).
    - [ ] **Action:** Assert that `load_config_from_json` and `initialize_world_from_file` handle all edge cases gracefully without segmentation faults.
    - [ ] **Verification (Test Execution):** Run the new test suite to ensure robust error handling and correct deserialization.

---

## Phase 3: Extract Application State Manager

*Goal: Decouple logical state transitions from `gui.c`.*

- [ ] **Step 3.1: Create `app_state_manager.c/h`**
    - [ ] **Action:** Create `app_state_manager.h` and define `AppState update_app_state(AppState current_state, GameConfig* config, World* world);`. Include `core_types.h`.
    - [ ] **Action:** Create `app_state_manager.c`. Extract the logical state transition code (e.g., switching from `STATE_CONFIG` to `STATE_RUNNING` based on conditions, not UI clicks yet, or define a clean interface for UI events). *Note: This might require passing an input event struct or letting the manager poll input if decoupled from Raylib drawing.*
    - **Refined Action:** Let `update_app_state` handle the simulation clock and logical state transitions. It should NOT contain Raylib drawing calls.
    - [ ] **Verification (Compile Test):** Compile just the object file: `gcc -c app_state_manager.c -Wall -Wextra`.

- [ ] **Step 3.2: Establish State Manager DEV_TEST**
    - [ ] **Action:** Create a state-of-the-art unit test `tests/test_app_state_manager.c`.
    - [ ] **Action:** Mock the inputs (`current_state`, `config`, `world`, and input events) to deterministically test state transitions (e.g., verifying `STATE_CONFIG` transitions to `STATE_RUNNING` only when all criteria are met).
    - [ ] **Verification (Test Execution):** Compile and execute the test, proving that the state machine behaves predictably independent of Raylib rendering.

---

## Phase 4: Extract Renderer

*Goal: Isolate Raylib dependencies into a single module.*

- [ ] **Step 4.1: Create `renderer.c/h`**
    - [ ] **Action:** Create `renderer.h` and define `init_renderer`, `draw_current_state`, and `close_renderer`. Include `core_types.h`.
    - [ ] **Action:** Create `renderer.c` and include `raylib.h`.
    - [ ] **Action:** Move all `DrawRectangle`, `DrawText`, `BeginDrawing`, and `EndDrawing` calls from `gui.c` into `renderer.c`.
    - [ ] **Action:** Move the UI interaction logic (button clicks) here, and have it return state change requests to be processed by the main loop or state manager.
    - [ ] **Verification (Compile Test):** Run `make`. Fix any missing include or variable scope issues.

- [ ] **Step 4.2: Establish Renderer DEV_TEST**
    - [ ] **Action:** Create a state-of-the-art test setup for UI logic (e.g., `tests/test_renderer.c`).
    - [ ] **Action:** Since drawing is hard to unit test, focus on testing the UI interaction logic: mock user inputs (clicks, key presses) and verify that the correct state change requests are generated.
    - [ ] **Verification (Test Execution):** Compile and run the mock UI tests, ensuring input handling does not crash under boundary-case conditions.

---

## Phase 5: Refactor Main GUI Loop and Cleanup

*Goal: Wire the new decoupled modules together and delete `gui.c`.*

- [ ] **Step 5.1: Rewrite `main.c` Loop**
    - [ ] **Action:** Update `main.c` to use `init_renderer`, `update_app_state`, and `draw_current_state` instead of the old `gui.c` functions.
    - [ ] **Action:** Ensure the double-buffering pointer swap for `World` still occurs correctly in the `RUNNING` state.
    - [ ] **Verification (Compile Test):** Run `make clean && make`.

- [ ] **Step 5.2: Remove `gui.c/h`**
    - [ ] **Action:** Delete `gui.c` and `gui.h`.
    - [ ] **Action:** Remove them from the `Makefile`.
    - [ ] **Verification (Interactive Test):**
        1. Run `make clean && make`.
        2. Run `./biotope`.
        3. Click through the Configuration screen.
        4. Start the simulation.
        5. Pause the simulation.
        6. **Expected Result:** The application behaves exactly as it did before the refactoring. No visual or logical regressions.

- [ ] **Step 5.3: Establish End-to-End System DEV_TEST**
    - [ ] **Action:** Expand the automated Python test suite (`run_test_suite.py`) to perform an end-to-end integration test of the refactored system.
    - [ ] **Action:** Use a deterministic seed to verify that a full simulation cycle (load config, initialize, run N generations, save state) produces bit-for-bit identical results compared to a pre-refactoring golden snapshot.
    - [ ] **Verification (Test Execution):** Run the complete end-to-end suite. The golden snapshot test must pass flawlessly to guarantee zero logical regressions.

- [ ] **Step 5.4: Final Documentation**
    - [ ] **Action:** Update `docs/CHANGELOG.md` to reflect the architectural consolidation (ADR-0015).
    - [ ] **Action:** Ensure all modified files have the `// KI-Agent unterstützt` comment where significant structural changes occurred.
    - [ ] **Verification:** Read the changelog to ensure clarity.