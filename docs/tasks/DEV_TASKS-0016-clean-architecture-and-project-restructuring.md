# DEV_TASKS-0016: Clean Architecture and Project Restructuring

This task plan details the step-by-step execution of the physical project restructuring as defined in **ADR-0016** and **DEV_SPEC-0016**.

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality.

**Briefing Documents:**
*   [ADR-0016: Clean Architecture and Project Restructuring](../adr/ADR-0016-clean-architecture-and-project-restructuring.md)
*   [DEV_SPEC-0016: Clean Architecture Restructuring](../specs/DEV_SPEC-0016-clean-architecture-restructuring.md)
*   [DEV_TECH_DESIGN-0016: Clean Architecture Restructuring](../tech_design/DEV_TECH_DESIGN-0016-clean-architecture-restructuring.md)

---

## Phase 1: Preparation and Base Structure

*Goal: Establish the new directory hierarchy and verify the initial environment stability.*

- [ ] **Step 1.1: Verify Current Build Stability**
    - [ ] **Action:** Run a full clean build of the current project.
    - [ ] **Verification:**
        1. Run `make clean && make`.
        2. Verify that `biotope`, `biotope_headless`, and `biotope_hyper_worker` are created without warnings.
        3. **Expected Result:** Successful compilation of all targets.

- [ ] **Step 1.2: Create Directory Hierarchy**
    - [ ] **Action:** Create the following directory structure in the root:
        ```bash
        mkdir -p src/core src/gui src/io src/apps/gui src/apps/headless src/apps/hyper src/vendor web/editor web/bridge scripts/testing scripts/generation assets/shaders build
        ```
    - [ ] **Verification:** Run `ls -R src web scripts assets build` and verify all folders exist.

- [ ] **Step 1.3: Update Git Ignore**
    - [ ] **Action:** Add `build/` to the `.gitignore` file.
    - [ ] **Verification:** Ensure `git status` does not show the `build/` directory as untracked.

---

## Phase 2: Domain Migration (Core, GUI, IO, Vendor)

*Goal: Move internal modules to their respective domain folders using `git mv`.*

- [ ] **Step 2.1: Migrate Core Domain**
    - [ ] **Action:** Move core files:
        ```bash
        git mv game_logic.c game_logic.h core_types.h config.h src/core/
        ```
    - [ ] **Verification:** Run `ls src/core/` to confirm files are present.

- [ ] **Step 2.2: Migrate GUI Subsystem**
    - [ ] **Action:** Move GUI-related files:
        ```bash
        git mv renderer.c renderer.h app_state_manager.c app_state_manager.h src/gui/
        ```
    - [ ] **Verification:** Run `ls src/gui/` to confirm files are present.

- [ ] **Step 2.3: Migrate IO Subsystem**
    - [ ] **Action:** Move IO files:
        ```bash
        git mv file_io.c file_io.h src/io/
        ```
    - [ ] **Verification:** Run `ls src/io/` to confirm files are present.

- [ ] **Step 2.4: Migrate Vendor Code**
    - [ ] **Action:** Move cJSON files:
        ```bash
        mkdir -p src/vendor/cJSON
        git mv cJSON.c cJSON.h src/vendor/cJSON/
        ```
    - [ ] **Verification:** Run `ls src/vendor/cJSON/` to confirm files are present.

---

## Phase 3: Application Target Migration

*Goal: Move entry-point files to their target-specific folders.*

- [ ] **Step 3.1: Migrate App Entry Points**
    - [ ] **Action:** Move main application files:
        ```bash
        git mv main.c src/apps/gui/
        git mv main_headless.c src/apps/headless/
        git mv main_hyper.c src/apps/hyper/
        ```
    - [ ] **Verification:** Run `ls -R src/apps/` to confirm files are present in their respective subfolders.

---

## Phase 4: Web and Assets Consolidation

*Goal: Organize non-C assets and scripts.*

- [ ] **Step 4.1: Migrate Web Assets**
    - [ ] **Action:** Move web-related files:
        ```bash
        git mv editor.html web/editor/
        git mv submit.php web/bridge/
        ```
    - [ ] **Verification:** Run `ls -R web/` to confirm.

- [ ] **Step 4.2: Migrate Shaders**
    - [ ] **Action:** Move shaders:
        ```bash
        git mv resources/shaders/* assets/shaders/
        rmdir resources/shaders resources
        ```
    - [ ] **Verification:** Run `ls assets/shaders/` to confirm.

- [ ] **Step 4.3: Migrate Scripts**
    - [ ] **Action:** Move Python scripts:
        ```bash
        git mv run_test_suite.py generate_stress_test.py scripts/testing/
        git mv generate_szenario_*.py scripts/generation/
        ```
    - [ ] **Verification:** Run `ls -R scripts/` to confirm.

---

## Phase 5: Build System Modernization (Makefile Refactoring)

*Goal: Update the Makefile to support the new structure and redirect artifacts to `build/`.*

- [ ] **Step 5.1: Update Makefile Includes and Paths**
    - [ ] **Action:** Refactor the `Makefile` according to the Technical Design (Section 3). Add `-I` flags for all `src/` subdirectories.
    - [ ] **Verification:** Run `make` (it will likely fail initially, but should find headers now).

- [ ] **Step 5.2: Implement Build Directory Logic**
    - [ ] **Action:** Update the `Makefile` object rules to place `.o` files into `build/` and binaries into `build/`.
    - [ ] **Verification:**
        1. Run `make clean && make`.
        2. Verify that no `.o` files are in the `src/` hierarchy.
        3. Verify all binaries exist in `build/`.

- [ ] **Step 5.3: Final Functional Verification (Interactive Test)**
    - [ ] **Action:** Run the GUI application from the new build location.
    - [ ] **Verification:**
        1. Run `./build/biotope`.
        2. Verify that the simulation starts and shaders are loaded correctly (check console for shader errors).
        3. Enter Singleplayer mode and verify that grid editing and simulation work.
        4. **Expected Result:** Full application functionality preserved.

---

## Phase 6: Documentation and Finalization

*Goal: Update project documentation to reflect the new state.*

- [ ] **Step 6.1: Update README.md**
    - [ ] **Action:** Update the "Project Structure" section in the root `README.md`.
    - [ ] **Verification:** Read the `README.md` and ensure paths are correct.

- [ ] **Step 6.2: Finalize ADR**
    - [ ] **Action:** Update the status of `ADR-0016` to `accepted`.
    - [ ] **Verification:** Check the file content.
