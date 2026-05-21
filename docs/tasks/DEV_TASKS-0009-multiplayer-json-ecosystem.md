# DEV_TASKS-0009: Unified JSON Migration & Multiplayer Foundation

This task list tracks the implementation of the Hard Cut migration from legacy `.bio` to the unified JSON format (ADR-0009).

## Phase 1: Infrastructure & Library Integration
*Goal: Enable JSON support in the C environment.*

- [ ] **Step 1.1: Integrate cJSON Library**
    - [ ] **Action:** Download/Add `cJSON.c` and `cJSON.h` to the root directory.
    - [ ] **Action:** Update `Makefile` and `Makefile.wasm` to include `cJSON.o`.
    - [ ] **Verification:** Run `make` and `make -f Makefile.wasm` to ensure the project still compiles.

## Phase 2: Refactor Persistence Layer (`file_io.c`)
*Goal: Replace legacy text format with JSON.*

- [ ] **Step 2.1: Implement JSON `save_grid`**
    - [ ] **Action:** Rewrite `save_grid` to use `cJSON` for generating the ADR-0009 payload.
    - [ ] **Action:** Change default extension from `.bio` to `.json`.
- [ ] **Step 2.2: Implement JSON `load_grid`**
    - [ ] **Action:** Rewrite `load_grid` to parse JSON files and populate the `World` and `GameConfig` structs.
- [ ] **Step 2.3: Update File Listing**
    - [ ] **Action:** Update `list_protocol_files` to filter for `.json` instead of `.bio`.
    - [ ] **Action:** Update `load_protocol_metadata` to parse metadata from JSON fields.

## Phase 3: UI & System Integration (`gui.c`)
*Goal: Connect the new persistence logic to the application state machine.*

- [ ] **Step 3.1: Update Auto-Save Logic**
    - [ ] **Action:** Change auto-save filename template in `STATE_EDIT` (ENTER key) to use `.json`.
- [ ] **Step 3.2: Update UI Previews**
    - [ ] **Action:** Ensure `STATE_LOAD` correctly displays the richer metadata from JSON files.

## Phase 4: Final Cleanup & Validation
*Goal: Ensure zero warnings and architectural purity.*

- [ ] **Step 4.1: Remove Legacy Code**
    - [ ] **Action:** Delete unused `sscanf` patterns and legacy version detection logic.
- [ ] **Step 4.2: Global Verification**
    - [ ] **Action:** Run a full simulation, save it, and reload it to verify the round-trip works.
    - [ ] **Action:** Ensure `make` reports zero warnings.

---
*Developer: Adhere to Rule 10 of CODING_STYLE.md. Ensure educational comments for students.*
// KI-Agent unterstützt: Task list for the Unified JSON migration.
