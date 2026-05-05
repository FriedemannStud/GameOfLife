# DEV_TASKS-0005: Frictionless WASM Distribution and Onboarding Architecture

This document breaks down the implementation of the WASM port and the Puzzle Campaign into actionable, verifiable steps for a full-stack C developer. Quality precedes speed.

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality and adhering to `docs/CODING_STYLE.md`.

**Briefing Documents:**
*   [ADR-0005: Frictionless WASM Distribution and Onboarding Architecture](../adr/ADR-0005-frictionless-wasm-onboarding.md)
*   [DEV_SPEC-0004: Frictionless WASM Distribution and Onboarding Architecture](../specs/DEV_SPEC-0004-frictionless-wasm-onboarding.md)
*   [DEV_TECH_DESIGN-0004: Frictionless WASM Distribution and Onboarding Architecture](../tech_design/DEV_TECH_DESIGN-0004-frictionless-wasm-onboarding.md)

---

## Phase 1: Web Build Pipeline & Emscripten Setup

*Goal: Establish a secondary build target (`Makefile.web`) that compiles the existing native codebase into WebAssembly without breaking the native Linux/Windows builds.*

- [ ] **Step 1.1: Environment Preparation**
    - [ ] **Action:** Ensure the Emscripten SDK (`emsdk`) is installed and activated in your terminal environment.
    - [ ] **Action:** Verify Raylib is available for the web target. If necessary, download the pre-compiled Raylib web libraries or compile Raylib from source using Emscripten.
    - [ ] **Verification:** Run `emcc -v` in the terminal.
        - **Expected Result:** The terminal outputs the Emscripten version information. Please report this version.

- [ ] **Step 1.2: Create `Makefile.web`**
    - [ ] **Action:** Create a new file named `Makefile.web` in the project root.
    - [ ] **Action:** Configure `Makefile.web` to use `emcc` as the compiler.
    - [ ] **Action:** Set compilation flags: `-Os -Wall -I. -I/path/to/raylib_web/include -L/path/to/raylib_web/lib -s USE_GLFW=3 -s ASYNCIFY -s TOTAL_MEMORY=67108864 -s ALLOW_MEMORY_GROWTH=1`. (Adjust Raylib paths as per your local setup).
    - [ ] **Action:** Define the output target as `biotope.html`.
    - [ ] **Verification:** Do not compile yet. Just ensure the `Makefile.web` structure mirrors the native `Makefile` source files (`main.c`, `game_logic.c`, `gui.c`, `file_io.c`). Please confirm the file is created.

- [ ] **Step 1.3: Isolate OpenMP (Temporary)**
    - [ ] **Action:** Open `game_logic.c`.
    - [ ] **Action:** Wrap the `#pragma omp parallel for ...` directive in preprocessor macros so it is only active for native builds.
        ```c
        #ifndef PLATFORM_WEB
        #pragma omp parallel for reduction(+:local_red, local_blue)
        #endif
        ```
    - [ ] **Verification (Native Regression):** Run `make clean && make` and then `./biotope`.
        - **Expected Result:** The native version compiles and runs normally. OpenMP should still be active natively. Please confirm.

---

## Phase 2: Main Loop Refactoring

*Goal: Adapt `main.c` to use `emscripten_set_main_loop` for the web build while maintaining the standard `while` loop for the native build.*

- [ ] **Step 2.1: Extract `UpdateDrawFrame`**
    - [ ] **Action:** Open `main.c`.
    - [ ] **Action:** Create a new function `void UpdateDrawFrame(void)`.
    - [ ] **Action:** Move the contents of the existing `while (!WindowShouldClose())` loop into `UpdateDrawFrame()`. This should primarily be a call to a GUI step function or rendering logic. *Note: You may need to refactor `run_gui_app()` in `gui.c` to not contain its own blocking `while` loop, but rather act as a single-frame step function.*
    - [ ] **Verification (Native Regression):** Run `make clean && make` and `./biotope`.
        - **Expected Result:** The native game runs exactly as before. Please confirm.

- [ ] **Step 2.2: Implement Emscripten Main Loop**
    - [ ] **Action:** In `main.c`, include Emscripten headers conditionally:
        ```c
        #if defined(PLATFORM_WEB)
            #include <emscripten/emscripten.h>
        #endif
        ```
    - [ ] **Action:** Modify the `main()` function to branch based on the platform:
        ```c
        #if defined(PLATFORM_WEB)
            emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
        #else
            SetTargetFPS(60);
            while (!WindowShouldClose()) {
                UpdateDrawFrame();
            }
        #endif
        ```
    - [ ] **Verification (Web Build):** Run `make -f Makefile.web` (ensure `PLATFORM_WEB` is defined in the Makefile CFLAGS, e.g., `-DPLATFORM_WEB`). Serve the output directory using a local web server (e.g., `python -m http.server 8000`). Open `localhost:8000/biotope.html` in a browser.
        - **Expected Result:** The Biotope main menu renders in the browser canvas. The UI should be responsive. Please report success and any console errors.

---

## Phase 3: Puzzle Campaign Data Models & State

*Goal: Introduce the `PuzzleConfig` and the new `STATE_PUZZLE` to guide new players.*

- [ ] **Step 3.1: Define `PuzzleConfig`**
    - [ ] **Action:** Open `gui.h` (or a dedicated `puzzle.h`).
    - [ ] **Action:** Implement the `PuzzleConfig` struct as defined in DEV_TECH_DESIGN-0004 (Section 3.1).
    - [ ] **Verification:** Run `make clean && make`.
        - **Expected Result:** Compilation succeeds with no warnings. Please confirm.

- [ ] **Step 3.2: Introduce `STATE_PUZZLE`**
    - [ ] **Action:** In `gui.h`, add `STATE_PUZZLE` to the `AppState` enum.
    - [ ] **Action:** In `gui.c`, inside the main state machine `switch(state)`, add a `case STATE_PUZZLE:`.
    - [ ] **Action:** For now, have `STATE_PUZZLE` render a simple text "Tutorial Level 1" and switch back to `STATE_CONFIG` when `Enter` is pressed.
    - [ ] **Action:** Modify the startup logic in `gui.c` to initialize the app in `STATE_PUZZLE` instead of `STATE_CONFIG`.
    - [ ] **Verification (Interactive Test):** Run the native build `./biotope`.
        - **Expected Result:** The app starts, shows "Tutorial Level 1", and pressing Enter takes you to the standard config screen. Please confirm.

---

## Phase 4: Emscripten VFS (Virtual File System)

*Goal: Ensure `.bio` files can be saved and loaded persistently in the browser using IndexedDB.*

- [ ] **Step 4.1: Abstract `save_grid`**
    - [ ] **Action:** Open `file_io.c`.
    - [ ] **Action:** Add the `#if defined(PLATFORM_WEB)` block with `EM_ASM` for `FS.syncfs` as detailed in DEV_TECH_DESIGN-0004 (Section 4.2).
    - [ ] **Action:** Ensure Emscripten headers are included in `file_io.c` when compiling for the web.
    - [ ] **Verification (Web Build):** Run `make -f Makefile.web`. Start the local server. Open the browser, enter `STATE_CONFIG`, and try to save a layout (pressing 'S').
        - **Expected Result:** The browser console should print "Biotope saved to IndexedDB" (or any error). Refresh the page and check if the file can be loaded. Please report the console output.

---

*Developer: Upon completing these phases, the MVP for the WASM port and the architectural foundation for the puzzle campaign are complete. Ensure all changes are committed and the DoD from DEV_SPEC is met.*