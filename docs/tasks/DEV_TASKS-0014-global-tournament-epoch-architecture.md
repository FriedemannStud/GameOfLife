# DEV_TASKS-0014: Global Tournament Epoch Architecture (C-Hyper-Worker)

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality, especially when modifying C pointers and OpenMP threads.

**Briefing Documents:**
*   [ADR-0014: Global Tournament Epoch Architecture](../adr/ADR-0014-global-tournament-epoch-architecture.md)
*   [DEV_SPEC-0014: Global Tournament Epoch Architecture](../specs/DEV_SPEC-0014-global-tournament-epoch-architecture.md)
*   [DEV_TECH_DESIGN-0014: Technical Specification](../tech_design/DEV_TECH_DESIGN-0014-global-tournament-epoch-architecture.md)
*   [CODING_STYLE](../CODING_STYLE.md)

---

## Phase 1: Skeleton and JSON Parsing (`main_hyper.c`)

*Goal: Create the new C-executable entry point, correctly read a large JSON batch file into memory, and validate the input structures without triggering the simulation.*

- [x] **Step 1.1: Create `main_hyper.c` Skeleton**
    - [x] **Action:** Create a new file `main_hyper.c`. Include necessary headers (`stdio.h`, `stdlib.h`, `string.h`, `cJSON.h`, `game_logic.h`).
    - [x] **Action:** Define the `Competitor` struct to hold `player_id` (char array) and the starting grid (an 8x8 integer array or a list of coordinates).
    - [x] **Action:** Write the `main` function to accept two arguments: `<input_batch.json>` and `<output_results.json>`.
    - [x] **Action:** Update the `Makefile` to include a new build target `hyper` that compiles `main_hyper.c` and links against `cJSON.c` and `game_logic.c`. Add the `-fopenmp` flag.
    - [x] **Verification (Interactive Test):**
        1.  Run `make clean && make hyper`.
        2.  Run `./biotope_hyper_worker`.
        3.  **Expected Result:** The program prints usage instructions and exits with code 1. No compilation warnings.

- [x] **Step 1.2: Implement `cJSON` Input Parsing**
    - [x] **Action:** In `main_hyper.c`, implement a function `parse_batch_file(const char* filepath, Competitor** competitors, int* count, int* max_gen)`.
    - [x] **Action:** Read the entire file into memory.
    - [x] **Action:** Use `cJSON_Parse` to traverse the JSON. Extract `max_generations` and dynamically allocate an array of `Competitor` structs based on the size of the `competitors` JSON array.
    - [x] **Action:** Iterate through the `competitors` array. Copy the `player_id`. Safely parse the `cells` array (ensure coordinates are $\ge 0$ and $< 8$) and store them in the `Competitor` struct.
    - [x] **Verification (Interactive Test):**
        1.  Create a dummy `test_input.json` containing 3 competitors with a few valid cells.
        2.  Modify `main` temporarily to print the parsed IDs and the number of living cells for each parsed competitor.
        3.  Run `./biotope_hyper_worker test_input.json dummy.json`.
        4.  **Expected Result:** The program prints the 3 IDs and the correct cell counts without segmentation faults.

## Phase 2: Thread-Safe Match Logic & Early Termination (`game_logic.c`)

*Goal: Refactor the simulation rules to be runnable inside a parallel loop safely, and add the Still-Life detection.*

- [x] **Step 2.1: Implement `run_isolated_match`**
    - [x] **Action:** In `game_logic.c` (and `.h`), add: `MatchResult run_isolated_match(int left_cells[8][8], int right_cells[8][8], int max_gen)`.
    - [x] **Action:** Inside the function, allocate `local_current` and `local_next` using `create_world(8, 16)`.
    - [x] **Action:** Translate the `left` competitor's cells into `TEAM_RED` (columns 1-8) and the `right` competitor's cells into `TEAM_BLUE` (columns 9-16) on `local_current`.
    - [x] **Action:** Implement the simulation `for` loop up to `max_gen`.
    - [x] **Action:** At the end of the simulation, count populations, determine the winner (Red, Blue, or Draw), free `local_current` and `local_next`, and return the result.
    - [x] **Verification (Interactive Test):**
        1.  In `main_hyper.c`, call `run_isolated_match` manually for Competitor 0 vs Competitor 1 from your parsed JSON.
        2.  Print the winner.
        3.  **Expected Result:** The match computes correctly.

- [x] **Step 2.2: Implement Early Termination (Still-Life Detection)**
    - [x] **Action:** Inside the `run_isolated_match` simulation loop, immediately after `update_generation`, add a `memcmp` check.
    - [x] **Action:** Compare `local_current->grid` and `local_next->grid`.
    - [x] **Action:** If they are identical (result `0`), break the loop early. Record the current iteration counter in the `MatchResult` struct as `stable_at_generation`.
    - [x] **Verification (Interactive Test):**
        1.  Create a `test_still_life.json` where Competitor 0 is a 2x2 block (Still-Life) and Competitor 1 is empty.
        2.  Run the test.
        3.  **Expected Result:** The program reports `Stable At=2`.

## Phase 3: The $O(N^2)$ Loop, OpenMP, and Output (`main_hyper.c`)

*Goal: Orchestrate the full tournament, handle symmetric matches, parallelize, and generate the final JSON.*

- [x] **Step 3.1: The Double Loop and Home/Away**
    - [x] **Action:** In `main_hyper.c`, allocate an array `RankingScore` for each competitor (initialized to 0).
    - [x] **Action:** Implement the double loop: `for (int i = 0; i < num_competitors; i++) { for (int j = i + 1; j < num_competitors; j++) { ... } }`.
    - [x] **Action:** Inside the loop, run `MatchResult r1 = run_isolated_match(&comp[i], &comp[j], max_gen);` (i is Red/Left, j is Blue/Right).
    - [x] **Action:** Immediately run the symmetric match: `MatchResult r2 = run_isolated_match(&comp[j], &comp[i], max_gen);` (j is Red/Left, i is Blue/Right).
    - [x] **Action:** Update the scores in the `RankingScore` array based on `r1` and `r2` (1.0 for win, 0.5 for draw).
    - [x] **Verification:** Verified with 3 test competitors.

- [x] **Step 3.2: OpenMP Parallelization and Atomics**
    - [x] **Action:** Add `#pragma omp parallel for schedule(dynamic)` above the outer `for` loop.
    - [x] **Action:** IMPORTANT: Ensure that the scoring updates use `#pragma omp atomic`.
    - [x] **Verification (Interactive Test):**
        1.  Watch CPU utilization during execution of a larger batch.
        2.  **Expected Result:** Multiple cores are utilized.

- [x] **Step 3.3: Output JSON Generation**
    - [x] **Action:** Sort the `RankingScore` array descending by `total_score`.
    - [x] **Action:** Use `cJSON` to build the `output_results.json` structure.
    - [x] **Action:** Write the JSON string to the file specified in `argv[2]`.
    - [x] **Action:** Ensure all `cJSON` objects and the `Competitor` array are freed before `main` exits.
    - [x] **Verification:** Checked the generated `output_results.json` for structural correctness.

## Phase 4: Python Backend Integration (`worker.py`)

*Goal: Connect the C-engine to the MongoDB schedule.*

- [x] **Step 4.1: Refactor `worker.py` to Epoch Scheduler**
    - [x] **Action:** Replace the continuous queue polling with a periodic loop (`asyncio.sleep(60)`).
    - [x] **Action:** Query MongoDB for all `status: "active"` submissions.
    - [x] **Action:** Serialize them into the `input_batch.json` format.
    - [x] **Action:** Call `./biotope_hyper_worker input_batch.json output_results.json` using `subprocess.run`.
    - [x] **Action:** Parse `output_results.json` and perform a bulk update on the MongoDB.
    - [x] **Verification:** Refactored script is ready.
