# DEV_TASKS-0003: Integrated Simulation Protocol and Replay System

This document outlines the step-by-step implementation plan for the Integrated Simulation Protocol and Replay System.

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality.

**Briefing Documents:**
*   [ADR-0003: Integrated Simulation Protocol](../../docs/ADR-0003-integrated-simulation-protocol.md)
*   [DEV_SPEC-0003: Integrated Simulation Protocol Spec](../../docs/DEV_SPEC-0003-integrated-simulation-protocol.md)
*   [DEV_TECH_DESIGN-0003: Technical Specification](../../docs/DEV_TECH_DESIGN-0003-integrated-simulation-protocol.md)

---

## Phase 1: File I/O Core Enhancements

*Goal: Update the file persistence layer to support the new extended protocol format (v2) while maintaining backward compatibility with existing .bio files.*

- [x] **Step 1.1: Update `GameConfig` and Headers**
    - [x] **Action:** In `gui.h`, ensure `GameConfig` struct is fully defined and accessible for `file_io.c`. (It is currently defined in `gui.c` but might need to be moved to `gui.h` or `game_logic.h` if `file_io.h` needs to see it, or forward declared). *Correction:* `file_io.h` already includes `gui.h`.
    - [x] **Action:** Open `file_io.h`. Add the `ProtocolInfo` struct definition as specified in the Tech Design (Section 4.1).
    - [x] **Action:** Update `save_grid` prototype in `file_io.h` to explicitly take `GameConfig *c` (already done in current code, just verify).
    - [x] **Action:** Add prototypes for `list_protocol_files` and `load_protocol_metadata` to `file_io.h`.
    - [x] **Verification (Interactive Test):**
        1.  Run `gcc -c main.c gui.c file_io.c game_logic.c` (or your build command).
        2.  **Expected Result:** Compilation succeeds. No "unknown type" errors for `ProtocolInfo`.

- [x] **Step 1.2: Implement Extended Save Logic**
    - [x] **Action:** In `file_io.c`, modify `save_grid`.
    - [x] **Action:** Change the `fprintf` header writing logic. Instead of just `%d %d %d`, it should now write `2 %ld %d %d %d %d %d` (Version, Timestamp, Rows, Cols, MaxPop, MaxRounds, Delay).
    - [x] **Action:** Use `time(NULL)` to get the current timestamp.
    - [x] **Verification (Interactive Test):**
        1.  Compile and run the game.
        2.  Go to Editor, place some cells.
        3.  Press `S` to save to `setup.bio`.
        4.  Open `setup.bio` in a text editor.
        5.  **Expected Result:** The first line should now look like `2 1705481234 50 50 100 1000 100` (7 numbers starting with 2).

- [x] **Step 1.3: Implement Backward-Compatible Load Logic**
    - [x] **Action:** In `file_io.c`, modify `load_grid`.
    - [x] **Action:** Update the `fscanf` logic to read the first line.
    - [x] **Action:** Check the return value of `fscanf`.
        -   If it reads 3 items: It's legacy. Parse as `Rows, Cols, MaxPop`. Set `MaxRounds=1000`, `Delay=100`.
        -   If it reads 7 items (and first is 2): It's v2. Parse all fields.
    - [x] **Action:** Ensure `GameConfig *c` is updated with the loaded values (`max_rounds`, `delay_ms`).
    - [x] **Verification (Interactive Test):**
        1.  Create a file `legacy_test.bio` manually with content: `20 20 50
0 0 1`. *Note: The UI currently hardcodes "setup.bio" for loading.*
        2.  **Alternative Verification:** Save a new file using the game (Step 1.2). Restart game. Press `L`.
        3.  **Expected Result:** The grid loads correctly. The configuration (Rounds, Delay) matches what was saved.

## Phase 2: Directory Helper & File Listing

*Goal: Enable the application to inspect the file system and list available protocol files.*

- [x] **Step 2.1: Implement `list_protocol_files`**
    - [x] **Action:** In `file_io.c`, include `<dirent.h>` (or `<windows.h>` if MinGW is missing it, but try dirent first).
    - [x] **Action:** Implement `int list_protocol_files(const char *dir_path, ProtocolInfo **out_list)`.
    - [x] **Action:** Logic:
        1.  Open directory.
        2.  Loop through files.
        3.  Filter for `.bio` extension.
        4.  For each match:
            -   `malloc` or `realloc` the list.
            -   Store filename.
            -   Call helper `load_protocol_metadata` (stub for now) to get details.
    - [x] **Verification (Interactive Test):**
        1.  Create a temporary `test_list.c` main file that calls `list_protocol_files(".")` and prints found names.
        2.  Run it.
        3.  **Expected Result:** It lists `setup.bio` and any other .bio files in the folder.

- [x] **Step 2.2: Implement `load_protocol_metadata`**
    - [x] **Action:** In `file_io.c`, implement `load_protocol_metadata`.
    - [x] **Action:** Logic: Open file, read *only* the first line (header), parse version, rows, cols, rounds, timestamp. Close file. Return 1 on success.
    - [x] **Verification (Interactive Test):**
        1.  Update `test_list.c` to print the metadata (Rows, Cols, Rounds) next to the filename.
        2.  **Expected Result:** Correct metadata is displayed for the v2 file created in Phase 1.

- [x] **Step 2.3: Sorting**
    - [x] **Action:** In `list_protocol_files`, after collecting all files, use `qsort` to sort the array of `ProtocolInfo`.
    - [x] **Action:** Comparator: Sort by `timestamp` descending (newest first).
    - [x] **Verification:**
        1.  Create two bio files manually with different timestamps in the header.
        2.  Run `test_list.c`.
        3.  **Expected Result:** The file with the higher timestamp appears first.

## Phase 3: GUI - Auto-Save Implementation

*Goal: Automate the creation of protocol files when the simulation starts.*

- [x] **Step 3.1: Trigger Auto-Save**
    - [x] **Action:** In `gui.c`, locate `STATE_EDIT`. Find the block handling `KEY_ENTER` (transition to `STATE_RUNNING`).
    - [x] **Action:** Before changing state, construct a filename: `biotope_results/run_YYYYMMDD_HHMMSS.bio`.
    - [x] **Action:** Ensure `biotope_results` directory exists (use `mkdir` or `CreateDirectory` wrapper in `file_io` if needed, or assume it exists/create manually for now).
    - [x] **Action:** Call `save_grid(filename, gui_world, &config)`.
    - [x] **Verification (Interactive Test):**
        1.  Start game.
        2.  Setup a pattern.
        3.  Press `ENTER` to run.
        4.  Check `biotope_results/` folder in Windows Explorer.
        5.  **Expected Result:** A new file `run_....bio` exists with the correct content.

## Phase 4: GUI - File Browser (The "Replay" UI)

*Goal: Replace the simple "Load" button with a full file browser.*

- [x] **Step 4.1: Define `STATE_LOAD`**
    - [x] **Action:** In `gui.c`, add `STATE_LOAD` to the `AppState` enum.
    - [x] **Action:** In `gui.c`, add global (or static) variables for the file list state:
        -   `ProtocolInfo *fileList = NULL`
        -   `int fileCount = 0`
        -   `int selectedFileIndex = 0`

- [x] **Step 4.2: Implement Transition and Loading List**
    - [x] **Action:** In `STATE_EDIT` (or `STATE_CONFIG`), change `KEY_L` logic. Instead of calling `load_grid` immediately:
        -   Call `list_protocol_files("biotope_results", &fileList)`.
        -   Set `selectedFileIndex = 0`.
        -   Set `state = STATE_LOAD`.
    - [x] **Verification (Interactive Test):**
        1.  Start game. Press `L`.
        2.  **Expected Result:** Screen changes (likely blank or black for now as we haven't rendered `STATE_LOAD` yet), but no crash.

- [x] **Step 4.3: Implement Rendering for `STATE_LOAD`**
    - [x] **Action:** In the main drawing loop (switch `state`), add `case STATE_LOAD:`
    - [x] **Action:** Draw Title "PROTOCOL ARCHIVE".
    - [x] **Action:** Loop through `fileList`. Draw filenames.
    - [x] **Action:** Draw a rectangle/highlight around `fileList[selectedFileIndex]`.
    - [x] **Action:** Draw a "Preview" box on the right side:
        -   "Date: ..." (convert timestamp to string)
        -   "Grid: WxH"
        -   "Rounds: ..."
    - [x] **Verification (Interactive Test):**
        1.  Start game. Press `L`.
        2.  **Expected Result:** You see the list of files generated in Phase 3. The first one is highlighted.

- [x] **Step 4.4: Implement Input for `STATE_LOAD`**
    - [x] **Action:** In the logic loop for `STATE_LOAD`:
        -   `KEY_UP`: `selectedFileIndex--` (clamp to 0).
        -   `KEY_DOWN`: `selectedFileIndex++` (clamp to `fileCount-1`).
        -   `KEY_ENTER`:
            -   Call `load_grid(fileList[selectedFileIndex].filepath, ...)`
            -   Free `fileList`.
            -   Transition to `STATE_EDIT`.
        -   `KEY_Q` or `KEY_ESC`:
            -   Free `fileList`.
            -   Transition back to `STATE_EDIT`.
    - [x] **Verification (Interactive Test):**
        1.  Start game. Run a few simulations to generate files.
        2.  Go to Editor. Press `L`.
        3.  Navigate with arrows. Observe preview changing.
        4.  Press Enter on an old run.
        5.  **Expected Result:** The game returns to Editor mode with the EXACT grid and settings from that old run.

---
**Final Verification:**
1.  Full walkthrough: Config -> Edit -> Run (Auto-save) -> Finish -> Edit -> Load (Browser) -> Select -> Replay.
2.  Ensure no memory leaks (check `fileList` freeing).
