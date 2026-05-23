### **ADR-0015: Architectural Consolidation and Modularization**

**Status:** proposed

**Date:** 2026-05-23

#### **1. Context and Problem Statement**

As the Biotope GameOfLife project has expanded to include multiple entry points (GUI, Headless CLI, Hyper Tournament Worker), several architectural "cracks" have emerged that threaten long-term maintainability:

1.  **Entry Point Fragmentation:** `main.c`, `main_headless.c`, and `main_hyper.c` contain redundant code for JSON parsing, file loading, and environment setup.
2.  **UI Monolithism:** `gui.c` acts as a central orchestrator that mixes rendering logic (Raylib), application state transitions, and simulation control. This makes it difficult to modify the UI without affecting the core application flow.
3.  **Circular Dependencies:** There is a tight coupling between `gui.h` and `file_io.h`, primarily driven by the shared `GameConfig` and `AppState` structures.
4.  **Parameter Leakage:** Constants such as grid dimensions and simulation rules are sometimes hardcoded in logic or worker scripts instead of being centrally managed.

To ensure the codebase remains "clean" and navigable, we need a formal decision to consolidate utilities and enforce stricter modular boundaries.

#### **2. Decision**

We will implement a four-pillar consolidation strategy:

1.  **Centralized IO:** All JSON parsing, grid persistence, and protocol logging will be moved exclusively to `file_io.c/h`. The entry points (`main`, `headless`, `hyper`) will only call high-level functions from this module.
2.  **Decomposition of `gui.c`:** We will split the monolithic GUI module into:
    *   `app_state_manager.c/h`: Responsible for state transitions and application logic.
    *   `renderer.c/h`: Responsible for Raylib-specific drawing and interactive elements.
3.  **Global Type Definition:** Introduce a `core_types.h` header to hold fundamental structs like `GameConfig`, `World`, and `AppState`. This eliminates circular dependencies between UI and IO modules.
4.  **Strict Parameter Localization:** All "magic numbers" and static configurations will be moved to a `config.h` or integrated into the `GameConfig` initialization logic.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

*   **DRY (Don't Repeat Yourself):** Eliminates bug surface area by having a single source of truth for file operations.
*   **Improved Navigability:** Separation of state logic from rendering makes it easier for developers (and AI agents) to locate specific behaviors.
*   **Testability:** Decoupled modules (like the state manager) can be tested independently of the Raylib window context.
*   **Consistency:** Standardizes the behavior across GUI and CLI variants.

**Negative Consequences (Disadvantages):**

*   **Initial Refactoring Effort:** Requires temporary "instability" while moving existing logic between files.
*   **Increased File Count:** Moving from a few large files to many smaller modules increases project overhead for simple lookups.
*   **Header Management:** Developers must be disciplined about including the correct "pillar" headers instead of a single `gui.h`.

#### **4. Alternatives Considered**

*   **"Status Quo":** Continue with duplication and monolithic GUI. This was rejected because the increasing complexity of the Tournament Epoch architecture (ADR-0014) makes manual synchronization of entry points prone to error.
*   **Object-Oriented C Patterns:** Using function pointers in structs to simulate objects. Rejected as it adds unnecessary complexity and deviates from the project's "snake_case" procedural C style mandated in `CODING_STYLE.md`.
*   **Single Main with Flags:** Merging all entry points into one binary using CLI flags. Rejected because the dependencies (Raylib for GUI vs. no graphics for workers) are too divergent for a clean single-binary approach.

#### **AI Attribution**
// KI-Agent unterstützt: Refactoring strategy and ADR generation.
