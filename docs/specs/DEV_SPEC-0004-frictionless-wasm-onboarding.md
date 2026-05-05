# Requirements Analysis & Specification: Frictionless WASM Distribution and Onboarding Architecture

This document details the requirements for the "Frictionless WASM Distribution and Onboarding Architecture" feature, as described in **ADR-0005**.

---

### 1. Detailed Requirements Specification

The primary objective is to eliminate the technical and cognitive barriers to entry for the "Biotope GameOfLife". This involves two major engineering tracks:

1.  **Platform Pivot (WASM Build System):** The existing C/Raylib codebase must compile to WebAssembly using Emscripten. The resulting build must run flawlessly in modern desktop browsers (Chrome, Firefox, Safari) without requiring X11, Docker, or local installations.
2.  **Cognitive Onboarding (Puzzle Campaign):** The application flow must be modified. New players must not be dropped into the complex sandbox (`STATE_CONFIG` / `STATE_EDIT`). Instead, they must be routed through a linear sequence of curated puzzle levels that teach the core mechanics (Red vs. Blue majority rules).

**Technical Constraints & Requirements:**
*   **Virtual File System (VFS):** The Emscripten build must implement a VFS or alternative strategy to handle the `file_io.c` operations. Saving/loading `.bio` files must either utilize browser local storage (IndexedDB) or provide mechanisms to download/upload files from the host OS.
*   **Threading (OpenMP):** The OpenMP directives in `game_logic.c` must either be successfully compiled with Emscripten's Pthreads support (requiring `Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` headers on the host) OR conditionally disabled for the web build if server-side header configuration cannot be guaranteed.
*   **State Machine Extension:** A new state (`STATE_CAMPAIGN` or `STATE_PUZZLE`) must be introduced to `gui.c` to handle the logic of loading predefined puzzle levels, checking win conditions, and progressing to the next level.

---

### 2. User Stories & Acceptance Criteria

**Epic: Zero-Friction Web Distribution**

*   **User Story 1: Play in Browser**
    *   **As a user,** I want to click a link and immediately play the game in my web browser, **so that** I don't have to install Docker or configure X-Servers.
    *   **Acceptance Criteria:**
        *   A `Makefile.web` (or equivalent Emscripten build script) is created.
        *   Running the build produces an `index.html`, `.js`, and `.wasm` file.
        *   The game renders at 60fps in the browser with the correct UI layout.
        *   Mouse and keyboard inputs map correctly to the Raylib WASM context.

*   **User Story 2: Persistent Web Saves**
    *   **As a user,** I want to save my biotope protocols in the web version, **so that** I don't lose my cool simulations when I close the tab.
    *   **Acceptance Criteria:**
        *   The `save_grid` and `load_grid` functions in `file_io.c` are abstracted.
        *   In the WASM build, these functions read/write to the Emscripten IndexedDB virtual file system.
        *   Saved files persist across browser reloads.

**Epic: Cognitive Onboarding (The Trojan Horse Tutorial)**

*   **User Story 3: The First Encounter (Puzzle 1)**
    *   **As a new player,** I want to be presented with a simple, isolated scenario demonstrating the basic birth rules, **so that** I understand how cells interact without being overwhelmed.
    *   **Acceptance Criteria:**
        *   Upon first launch, the game enters a "Puzzle Mode".
        *   A pre-defined 10x10 grid is loaded with a specific challenge text (e.g., "Place 3 Blue Cells to conquer the red block").
        *   The UI restricts placement to a specific bounding box.
        *   Pressing ENTER simulates the grid, and a "Win Condition" check triggers if Blue population exceeds Red after a set number of rounds.

*   **User Story 4: Progressive Complexity (Campaign Flow)**
    *   **As a player,** I want to unlock harder puzzles sequentially, **so that** I gradually master the Red vs. Blue majority mechanic.
    *   **Acceptance Criteria:**
        *   A system to load sequential `.bio` files representing puzzle levels is implemented.
        *   Beating Level N unlocks Level N+1.
        *   Upon completing the final tutorial puzzle, the full Sandbox Mode (`STATE_CONFIG`) is permanently unlocked.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Emscripten build pipeline (`Makefile.web`).
        *   Playable simulation loop inside the browser.
        *   Basic UI rendering and mouse input in WASM.
    *   **Should-Have:**
        *   The linear "Puzzle Campaign" state machine.
        *   Implementation of the first 3 tutorial levels.
        *   Persistent save/load functionality via IndexedDB.
    *   **Could-Have:**
        *   Visual flair/animations when completing a puzzle.
        *   Exporting/Downloading `.bio` files from the browser to the local OS.
    *   **Won't-Have (in this increment):**
        *   Multiplayer server matchmaking (this remains a local 1v1 or AI experience).
        *   Cloud saving across different devices.

*   **Dependencies:**
    1.  **Build Environment:** Requires the installation of the Emscripten SDK (emsdk) in the development environment.
    2.  **Code Architecture:** The implementation of the Puzzle Campaign depends on a clean abstraction of the `App_State` machine in `gui.c`.
    3.  **File I/O:** The web save system depends on resolving how Emscripten handles asynchronous IndexedDB calls within Raylib's synchronous `main` loop.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| TSK-01 | Web Distro | Setup Emscripten toolchain in Docker/Local env | High |
| TSK-02 | Web Distro | Create `Makefile.web` for Raylib WASM compilation | High |
| TSK-03 | Web Distro | Refactor `main.c` loop for WASM (use `emscripten_set_main_loop`) | High |
| TSK-04 | Web Distro | Implement VFS / IndexedDB abstraction for `file_io.c` | Medium |
| TSK-05 | Web Distro | Test & fix OpenMP threading compatibility in WASM | Medium |
| TSK-06 | Onboarding | Create structural definitions for "Puzzle Levels" (win cons, restricted areas) | High |
| TSK-07 | Onboarding | Implement `STATE_PUZZLE` logic in `gui.c` | High |
| TSK-08 | Onboarding | Design and create `.bio` files for Level 1, 2, and 3 | Medium |
| TSK-09 | Onboarding | Implement Campaign Progression (Unlock Sandbox upon completion) | Medium |
| TSK-10 | Onboarding | Add "Victory" UI overlay and transition animations | Low |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The code is written and formatted according to the guidelines in `docs/CODING_STYLE.md` (e.g., proper C naming conventions, AI attribution comments).
*   **Compilation:** The project compiles successfully using both the standard `Makefile` (native) and the new `Makefile.web` (WASM) without critical warnings.
*   **Execution:** 
    *   The generated WASM build runs in a standard local HTTP server without crashing.
    *   The "Puzzle Mode" logic successfully detects win conditions and transitions states correctly.
*   **Tests:** Interactive/manual verification confirms that core UI buttons, inputs, and grid interactions function identically in the web build as they do in the native build.
*   **Acceptance Criteria:** All acceptance criteria defined for the story have been met.
*   **Code Review:** The code has been reviewed (or self-reviewed with extreme rigor adhering to the architectural directives).
*   **Documentation:** `CHANGELOG.md` is updated, and if necessary, instructions on how to build the WASM target are added to `DEVELOPMENT_GUIDELINES.md` or a new `README_WEB.md`.