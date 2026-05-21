# DEV_TASKS-0009: Maintenance and Compiler Warning Fixes

This document tracks small bugfixes, refactorings, and maintenance tasks to ensure the codebase remains compliant with `docs/CODING_STYLE.md`.

## Phase 1: Resolve Compiler Warnings (Zero-Warning Policy)

*Goal: Ensure the project compiles with `make` and `make -f Makefile.web` without any warnings.*

- [x] **Step 1.1: Fix Unused Variables in `gui.c`**
    - [x] **Action:** Locate `locUseMetaballs` and `locRenderSize` in `gui.c`.
    - [x] **Action:** Check if they are part of a partially implemented feature or safe to remove.
    - [x] **Verification:** Recompile and ensure warnings are gone.

- [x] **Step 1.2: Fix `sscanf` Format Specifier in `file_io.c`**
    - [x] **Action:** In `file_io.c`, change `%ld` to `%lld` (or appropriate `PRId64` macro) for `time_t` fields to support 64-bit timestamps.
    - [x] **Verification:** Recompile with `make -f Makefile.web` and ensure the warning is gone.

---
*Developer: Adhere to Rule 10 of CODING_STYLE.md for all changes. Verify with clean builds.*
