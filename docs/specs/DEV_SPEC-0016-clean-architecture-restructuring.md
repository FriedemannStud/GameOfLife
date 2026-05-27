# Requirements Analysis & Specification: Clean Architecture Restructuring

This document details the requirements for the physical restructuring of the Biotope - Game of Life codebase, as described in **ADR-0016**.

---

### 1. Detailed Requirements Specification

The primary objective is to transition the current flat and cluttered project structure into a modular, domain-driven "Clean Architecture." This involves moving files from the root directory into logical subdirectories while ensuring the build system (Makefile) remains fully functional.

#### 1.1 Structural Requirements
*   **Source Code Separation:** All C source (`.c`) and header (`.h`) files must be moved into a `src/` hierarchy.
    *   **Core Domain:** `game_logic.c/h`, `core_types.h`, and `config.h` move to `src/core/`.
    *   **GUI Subsystem:** `renderer.c/h` and `app_state_manager.c/h` move to `src/gui/`.
    *   **IO Subsystem:** `file_io.c/h` moves to `src/io/`.
    *   **Applications (Targets):** `main.c`, `main_headless.c`, and `main_hyper.c` move to `src/apps/gui/`, `src/apps/headless/`, and `src/apps/hyper/` respectively.
    *   **Vendor Code:** `cJSON.c/h` moves to `src/vendor/cJSON/`.
*   **Web Asset Consolidation:** `editor.html` and `submit.php` move to `web/`.
*   **Script Organization:** Automation and generation scripts move to `scripts/`.
*   **Resource Management:** Shaders move from `resources/shaders/` to `assets/shaders/`.
*   **Build Artifacts:** A new `build/` directory must be used for all object files (`.o`) and resulting binaries, keeping the source tree clean.

#### 1.2 Build System Requirements
*   The `Makefile` must be updated to support the new directory structure.
*   Compiler flags must include `-Isrc/core`, `-Isrc/gui`, etc., to allow existing `#include` statements to function with minimal modification.
*   The build system must support the three primary targets: `biotope` (GUI), `biotope_headless`, and `biotope_hyper_worker`.
*   The WebAssembly build (`Makefile.wasm`) must be similarly updated.

---

### 2. User Stories & Acceptance Criteria

**Epic: Improve Codebase Maintainability and Scalability**

*   **User Story 1: Domain-Driven Code Organization**
    *   **As a developer,** I want all source files organized by their logical domain (Core, GUI, IO), **so that** I can easily locate and modify specific subsystems without navigating a cluttered root directory.
    *   **Acceptance Criteria:**
        *   No `.c` or `.h` files remain in the project root.
        *   Core logic is isolated in `src/core/`.
        *   Raylib-specific code is isolated in `src/gui/`.
        *   Third-party code is clearly separated in `src/vendor/`.

*   **User Story 2: Clean Build Environment**
    *   **As a developer,** I want all compilation artifacts to be stored in a dedicated `build/` folder, **so that** my source directory remains clean and I don't accidentally commit binaries or object files.
    *   **Acceptance Criteria:**
        *   Running `make` creates a `build/` directory.
        *   All `.o` files are generated inside `build/`.
        *   Final binaries are placed in `build/`.
        *   The `build/` directory is ignored by Git.

*   **User Story 3: Multi-Target Project Clarity**
    *   **As a maintainer,** I want the different application entry points (GUI, Headless, Hyper) to be clearly separated, **so that** the purpose of each target is immediately obvious.
    *   **Acceptance Criteria:**
        *   Each target has its own subdirectory under `src/apps/`.
        *   The `Makefile` builds each target correctly from its new location.

*   **User Story 4: Resource and Asset Consolidation**
    *   **As a full-stack developer,** I want web-related files and static assets to be moved out of the root, **so that** the repository follows modern project standards.
    *   **Acceptance Criteria:**
        *   `editor.html` and `submit.php` are located in `web/`.
        *   Shaders are located in `assets/shaders/`.
        *   Scripts are located in `scripts/`.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Restructuring of `src/` (Core, GUI, IO, Apps).
        *   Updating the `Makefile` to maintain existing build targets.
        *   Implementation of the `build/` directory for artifacts.
    *   **Should-Have:**
        *   Consolidation of `web/` and `scripts/` directories.
        *   Relocation of `assets/shaders/`.
    *   **Could-Have:**
        *   Refactoring `#include` directives to use absolute paths from `src/`.
        *   Updating `Makefile.wasm` for WebAssembly support.
    *   **Won't-Have (in this increment):**
        *   Complete replacement of `submit.php` (legacy bridge).

*   **Dependencies:**
    1.  **Directory Creation:** Directories must be created before moving files.
    2.  **File Move:** Files must be moved using `git mv` to preserve history.
    3.  **Makefile Update:** The build system update is dependent on the final file locations.
    4.  **Verification:** Compilation and functional testing depend on the updated build system.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| BL-01 | Restructuring | Create new directory structure (`src/`, `web/`, `scripts/`, `assets/`, `build/`) | Must |
| BL-02 | Restructuring | Move Core files (`game_logic.c/h`, `core_types.h`, `config.h`) to `src/core/` | Must |
| BL-03 | Restructuring | Move GUI files (`renderer.c/h`, `app_state_manager.c/h`) to `src/gui/` | Must |
| BL-04 | Restructuring | Move IO files (`file_io.c/h`) to `src/io/` | Must |
| BL-05 | Restructuring | Move App entry points (`main.c`, etc.) to `src/apps/` | Must |
| BL-06 | Restructuring | Move Vendor code (`cJSON`) to `src/vendor/` | Must |
| BL-07 | Build System | Update `Makefile` to support new paths and include flags | Must |
| BL-08 | Build System | Implement `build/` directory for all output artifacts | Must |
| BL-09 | Restructuring | Move Web assets and Scripts to their respective folders | Should |
| BL-10 | Verification | Verify all build targets (`biotope`, `headless`, `hyper`) | Must |
| BL-11 | Build System | Update `Makefile.wasm` for new structure | Could |

---

### 5. Definition of Done (DoD)

A Product Backlog Item is considered "Done" when all of the following criteria are met:

*   **Code Quality:** Files are moved using `git mv`. No dead code or unused includes are introduced.
*   **Build Integrity:** 
    *   The project compiles without warnings using the updated `Makefile`.
    *   All binaries (`biotope`, `biotope_headless`, `biotope_hyper_worker`) are functional.
    *   The source tree is clean of `.o` files and binaries (all in `build/`).
*   **Acceptance Criteria:** All acceptance criteria for the respective User Story are met and verified.
*   **Regression Testing:** 
    *   Singleplayer mode is verified manually.
    *   Headless simulation is verified via CLI.
*   **Documentation:** 
    *   ADR-0016 is updated to "accepted" if the restructuring is successful.
    *   The `README.md` is updated to reflect the new directory structure.
