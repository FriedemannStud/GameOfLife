# DEV_TASKS-0010: Headless Simulation Worker

This document breaks down the implementation of the Headless Simulation Worker (Issue #3) into actionable, verifiable steps for a C developer. Quality precedes speed.

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality and adhering to `docs/CODING_STYLE.md`.

**Briefing Documents:**
*   [ADR-0010: Headless Simulation Worker](../adr/ADR-0010-headless-simulation-worker.md)
*   [DEV_SPEC-0010: Headless Simulation Worker](../specs/DEV_SPEC-0010-headless-simulation-worker.md)
*   [DEV_TECH_DESIGN-0010: Headless Simulation Worker](../tech_design/DEV_TECH_DESIGN-0010-headless-simulation-worker.md)

---

## Phase 1: Build System & Entry Point Foundation

*Goal: Establish the new headless entry point and ensure it can be compiled independently of Raylib.*

- [x] **Step 1.1: Create `main_headless.c`**
    - [x] **Action:** Create a new file `main_headless.c` in the root directory.
    - [x] **Action:** Add standard includes (`stdio.h`, `stdlib.h`, `string.h`) and project includes (`game_logic.h`, `cJSON.h`).
    - [x] **Action:** Implement a basic `int main(int argc, char *argv[])`.
    - [x] **Action:** Add CLI argument validation: If `argc < 3`, print usage `Usage: ./biotope_headless <red_config.json> <blue_config.json> [output.json]` and return `1`.
    - [x] **Verification (Interactive Test):**
        1.  (Skip compiling for now, just verify the code visually looks correct).
        2.  Ensure AI attribution comments are present if code was AI generated.

- [x] **Step 1.2: Update the Makefile**
    - [x] **Action:** Open `Makefile`.
    - [x] **Action:** Add a new target `headless:`.
    - [x] **Action:** Define the headless sources: `HEADLESS_SOURCES = main_headless.c game_logic.c file_io.c cJSON.c`.
    - [x] **Action:** The compilation command for `headless` MUST NOT include `-lraylib` or OpenGL flags. It should look like: `$(CC) $(HEADLESS_SOURCES) $(CFLAGS) -o biotope_headless -lm`.
    - [x] **Verification (Interactive Test):**
        1.  Run `make headless` in the terminal.
        2.  Run `./biotope_headless`.
        3.  **Expected Result:** The program should print the Usage message and exit cleanly without any Raylib/display initialization errors. Please report the exact output.

---

## Phase 2: JSON Parsing and Grid Initialization

*Goal: Read the incoming 8x8 JSON payloads and correctly place them into a combined 16x8 simulation grid.*

- [x] **Step 2.1: Prepare Test Data**
    - [x] **Action:** Create two temporary files `test_red.json` and `test_blue.json` in the project root.
    - [x] **Action:** Fill them with valid JSON structures according to `DEV_TECH_DESIGN-0010` (e.g., a simple block `[[0,0], [0,1], [1,0], [1,1]]`).

- [x] **Step 2.2: Implement File Loading Helper**
    - [x] **Action:** In `main_headless.c`, create a helper function `char* read_file(const char* path)` that reads a file's entire content into a malloc'd string.
    - [x] **Action:** Ensure proper error handling (return `NULL` if file doesn't exist) and close the file.

- [x] **Step 2.3: Parse and Stamp Patterns**
    - [x] **Action:** In `main_headless.c`'s `main` function, call `create_world(16, 8)` to create a 16x8 arena. Create both `current_gen` and `next_gen`.
    - [x] **Action:** Load and parse `argv[1]` (Red). Iterate over the JSON array `cells`. For each `[x, y]`, validate `x < 8` and `y < 8`. If valid, set `current_gen->grid[y * 16 + x] = TEAM_RED`.
    - [x] **Action:** Load and parse `argv[2]` (Blue). Iterate over the JSON array `cells`. Validate `x < 8` and `y < 8`. If valid, set `current_gen->grid[y * 16 + (x + 8)] = TEAM_BLUE`.
    - [x] **Action:** Free the parsed `cJSON` objects and the file strings.
    - [x] **Verification (Interactive Test):**
        1.  Add a temporary debug loop to print the 16x8 grid to the console (e.g., print 'R' for red, 'B' for blue, '.' for dead).
        2.  Run `make headless`.
        3.  Run `./biotope_headless test_red.json test_blue.json`.
        4.  **Expected Result:** The console prints a 16x8 grid showing the red pattern on the left and the blue pattern on the right. Please confirm the visual output.

---

## Phase 3: The 100-Generation Engine

*Goal: Execute the headless simulation loop without graphical overhead.*

- [x] **Step 3.1: The Simulation Loop**
    - [x] **Action:** In `main_headless.c`, remove the temporary debug printing.
    - [x] **Action:** Implement a `for` loop that runs exactly 100 times.
    - [x] **Action:** Inside the loop, call `update_generation(current_gen, next_gen, 16, 8, &red_pop, &blue_pop)`. (Note: you may need to pass dummy pointers for the population parameters if the function signature requires them, or just read the results).
    - [x] **Action:** Swap the `current_gen` and `next_gen` pointers.
    - [x] **Action:** Ensure NO `usleep` or rendering functions are called.

- [x] **Step 3.2: Final Population Count**
    - [x] **Action:** After the loop, iterate through `current_gen->grid` (16 * 8 = 128 cells).
    - [x] **Action:** Count the exact number of `TEAM_RED` and `TEAM_BLUE` cells.
    - [x] **Action:** Determine the winner ("red", "blue", or "draw").
    - [x] **Verification (Interactive Test):**
        1.  Temporarily `printf` the final counts and winner.
        2.  Run `make headless` and execute `./biotope_headless test_red.json test_blue.json`.
        3.  **Expected Result:** The program should output the final populations instantly. If you used static blocks in step 2.1, the populations should remain 4 and 4. Please report the console output and perceived execution speed.

---

## Phase 4: Result Persistence (JSON Output)

*Goal: Output the match results in the specified JSON format so the backend can consume it.*

- [x] **Step 4.1: Generate Output JSON**
    - [x] **Action:** In `main_headless.c`, after calculating the winner, use `cJSON` to build the result object.
    - [x] **Action:** Add fields: `winner`, `red_population`, `blue_population`, `generations` (100).
    - [x] **Action:** Use `time(NULL)` to get the current timestamp and add it.
    - [x] **Action:** Format the JSON to a string using `cJSON_PrintUnformatted`.

- [x] **Step 4.2: Write to Destination**
    - [x] **Action:** Check if `argv[3]` (output path) is provided.
    - [x] **Action:** If provided, `fopen` the path and write the JSON string to the file.
    - [x] **Action:** If not provided, print the JSON string to `stdout`.
    - [x] **Action:** Free the JSON string, the JSON object, and both `World` structs (`free_world`).

- [x] **Step 4.3: Final Validation**
    - [x] **Verification (Interactive Test):**
        1.  Run `make headless`.
        2.  Run `./biotope_headless test_red.json test_blue.json result.json`.
        3.  Check the contents of `result.json` using `cat result.json`.
        4.  **Expected Result:** A valid JSON string containing the correct population numbers and winner. Ensure no memory leaks occurred (if you have Valgrind available, run `valgrind ./biotope_headless ...`). Please report the final JSON output.