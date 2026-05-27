### **ADR-0016: Clean Architecture and Project Restructuring**

**Status:** accepted

**Date:** 2026-05-26

#### **1. Context and Problem Statement**

The "Biotope - Game of Life" project has evolved from a single-file C simulation into a complex multi-target system including a desktop GUI (Raylib), a headless CLI simulator, a high-performance tournament worker (OpenMP), and a Python-based FastAPI backend. 

Currently, the root directory is significantly cluttered. Source files (.c, .h), build configurations (Makefile), web assets (editor.html), bridge scripts (submit.php), and various Python utility scripts reside in the same flat hierarchy. This lack of physical separation makes it difficult to:
1.  Identify core simulation logic versus target-specific application code.
2.  Manage third-party (vendor) code separately from project-owned code.
3.  Scale the project for upcoming features like the "Uni-Messe Kiosk Mode" without further increasing root-level clutter.
4.  Onboard new developers who must navigate a "mixed-domain" root folder.

#### **2. Decision**

We will implement a "Clean Architecture" directory structure that strictly separates concerns by domain and responsibility. The project will be restructured according to the following layout:

*   **`src/`**: Centralized location for all C source code.
    *   **`src/core/`**: Stateless simulation logic (`game_logic.c`), fundamental types (`core_types.h`), and global configuration (`config.h`).
    *   **`src/apps/`**: Target-specific entry points (e.g., `gui/main.c`, `headless/main_headless.c`, `hyper/main_hyper.c`).
    *   **`src/gui/`**: Raylib-specific rendering logic (`renderer.c`) and UI state management (`app_state_manager.c`).
    *   **`src/io/`**: Data persistence, JSON handling (`file_io.c`), and future networking layers.
    *   **`src/vendor/`**: Third-party libraries (e.g., `cJSON/`) kept separate from original code.
*   **`web/`**: Consolidation of web-based assets (`editor.html`) and legacy bridge scripts (`submit.php`).
*   **`scripts/`**: Grouping of automation, testing, and scenario generation scripts.
*   **`assets/`**: Centralized storage for shaders and static resources.
*   **`build/`**: Dedicated, git-ignored directory for compilation artifacts (.o files and binaries).

The `Makefile` will be updated to handle these new paths using explicit object file tracking and `-I` (include) flags to maintain modularity.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**
*   **Improved Maintainability:** Clear physical boundaries between simulation logic, rendering, and I/O.
*   **Scalability:** New features (like `libcurl` networking) have a dedicated home in `src/io/` without cluttering the root.
*   **Target Clarity:** Developers can easily distinguish between the different application targets (GUI vs. Headless vs. Worker).
*   **Professional Standards:** Aligns the project with industry-standard C project layouts, improving onboarding and code quality.
*   **Build Cleanliness:** By using a `build/` directory, the source tree remains clean of temporary artifacts.

**Negative Consequences (Disadvantages):**
*   **Refactoring Overhead:** Moving files requires updating the `Makefile` and `Makefile.wasm`, which can be error-prone.
*   **Include Path Complexity:** Requires careful management of `#include` directives and compiler search paths (e.g., adding `-Isrc/core`).
*   **Git History Interruption:** While `git mv` preserves history, large-scale moves can complicate `git blame` or merge operations in progress.

#### **4. Alternatives Considered**

*   **Status Quo (Flat Hierarchy):** Rejected due to poor scalability and high cognitive load for developers.
*   **Module-Only Folders (No `src/` parent):** Considered putting `core/`, `gui/`, and `apps/` directly in the root. Rejected because it still mixes code with documentation (`docs/`), backends (`backend/`), and build scripts, failing to achieve true domain separation.
*   **Header/Source Separation (`include/` vs `src/`):** Rejected for this specific project as it is not currently intended to be distributed as a library; keeping `.c` and `.h` files together in domain folders (e.g., `src/core/`) is more idiomatic for standalone C applications.
