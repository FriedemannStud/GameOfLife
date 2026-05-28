# DEV_TASKS-0017: C-Networking and API Integration

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality.

**Briefing Documents:**
*   [ADR-0017: C-Networking and API Integration (libcurl & cJSON)](../adr/ADR-0017-c-networking-and-api-integration.md)
*   [DEV_SPEC-0017: C-Networking and API Integration](../specs/DEV_SPEC-0017-c-networking-and-api-integration.md)
*   [DEV_TECH_DESIGN-0017: Technical Design: C-Networking and API Integration](../tech_design/DEV_TECH_DESIGN-0017-c-networking-and-api-integration.md)

---

## Phase 1: Environment & Build Configuration

*Goal: Ensure all external C dependencies are available and the project compiles successfully with them.*

- [x] **Step 1.1: Verify `libcurl` availability**
    - [x] **Action:** Check if `libcurl` development headers are installed on the local system.
    - [x] **Verification (Interactive Test):**
        1.  Open the terminal.
        2.  Run: `curl-config --version` or `pkg-config --modversion libcurl`
        3.  **Expected Result:** A valid version number is printed (e.g., `libcurl 7.81.0`). *Note: If not installed, install via `sudo apt install libcurl4-openssl-dev`.*

- [x] **Step 1.2: Update the `Makefile`**
    - [x] **Action:** Open `Makefile`.
    - [x] **Action:** Add `-lcurl` and `-lpthread` to the `LDFLAGS` or `LDLIBS` variable.
    - [x] **Action:** Verify that `cJSON.c` from `src/vendor/cJSON/` is included in the source file list (`SRC_C`).

- [x] **Step 1.3: Compile Check**
    - [x] **Action:** Run a clean build.
    - [x] **Verification (Interactive Test):**
        1.  Run: `make clean && make`
        2.  **Expected Result:** The project compiles successfully without any linker errors regarding `-lcurl` or `-lpthread`.

---

## Phase 2: Python Backend - Database & Worker Updates

*Goal: Extend the FastAPI application to calculate and persist "Greatest Hits" at the end of each epoch.*

- [x] **Step 2.1: Implement `epoch_highlights` collection**
    - [x] **Action:** Open `backend/app/database.py` (or equivalent).
    - [x] **Action:** Define the `epoch_highlights` MongoDB collection reference.

- [x] **Step 2.2: Update Worker Logic (`worker.py`)**
    - [x] **Action:** Open `backend/app/worker.py`.
    - [x] **Action:** At the end of the simulation epoch logic, add a function to select the longest match (Metric 1) and most volatile match (Metric 4).
    - [x] **Action:** Write these matches (including participant names, metrics, and the two 8x8 seed configurations) to the `epoch_highlights` collection.
    - [x] **Action:** Format the code using `black .` and `ruff check .` to maintain PEP 8 standards.

- [x] **Step 2.3: Verification of Backend Worker**
    - [x] **Verification (Interactive Test):**
        1.  Start the local MongoDB instance.
        2.  Run the python worker: `python backend/app/worker.py` (or via `make` target).
        3.  Let one epoch finish.
        4.  Inspect MongoDB using a client (or mongo shell) `db.epoch_highlights.find().pretty()`.
        5.  **Expected Result:** A document exists containing an `epoch_id` and an array of `highlights` with 8x8 seeds.

---

## Phase 3: Python Backend - API Endpoints

*Goal: Expose the data via FastAPI GET endpoints.*

- [x] **Step 3.1: Implement `/api/leaderboard`**
    - [x] **Action:** Open `backend/app/main.py`.
    - [x] **Action:** Create the `GET /api/leaderboard` route. It should return a JSON object with a `leaderboard` array of `{name, elo, win_rate}`.

- [x] **Step 3.2: Implement `/api/epoch/highlights`**
    - [x] **Action:** Create the `GET /api/epoch/highlights` route. It should fetch the most recent entry from the `epoch_highlights` collection and return it as JSON.

- [x] **Step 3.3: Verification of Endpoints**
    - [x] **Verification (Interactive Test):**
        1.  Start the FastAPI server: `uvicorn backend.app.main:app --reload`
        2.  In a separate terminal, run: `curl http://localhost:8000/api/leaderboard`
        3.  Run: `curl http://localhost:8000/api/epoch/highlights`
        4.  **Expected Result:** Both commands return valid JSON payloads formatted exactly as defined in `DEV_TECH_DESIGN-0017`.

---

## Phase 4: C-Application - Core Networking Scaffolding

*Goal: Create the thread-safe `network_io` module without breaking existing code. (Adhere strictly to `CODING_STYLE.md`!)*

- [x] **Step 4.1: Define Data Structures (`network_io.h`)**
    - [x] **Action:** Create `src/io/network_io.h`.
    - [x] **Action:** Add include guards `#ifndef NETWORK_IO_H`.
    - [x] **Action:** Add the C data structures defined in `DEV_TECH_DESIGN-0017` (`LeaderboardEntry`, `LeaderboardData`, `MatchHighlight`, `HighlightData`). Use `PascalCase` for structs.
    - [x] **Action:** Add function prototypes (`network_init`, `network_cleanup`, async fetchers, and getters).

- [x] **Step 4.2: Scaffolding (`network_io.c`)**
    - [x] **Action:** Create `src/io/network_io.c`.
    - [x] **Action:** Include `network_io.h`, `pthread.h`, `curl/curl.h`, and `"../vendor/cJSON/cJSON.h"`.
    - [x] **Action:** Define static global variables for the shared state: `static LeaderboardData g_leaderboard;` and `static HighlightData g_highlights;`.
    - [x] **Action:** Define `static pthread_mutex_t g_network_mutex = PTHREAD_MUTEX_INITIALIZER;`.
    - [x] **Action:** Implement `network_init(void)` (calls `curl_global_init(CURL_GLOBAL_ALL)`) and `network_cleanup(void)` (calls `curl_global_cleanup()`).
    - [x] **Action:** Add the `// KI-Agent unterstützt` comment to the file header.

- [x] **Step 4.3: Integrate into Lifecycle**
    - [x] **Action:** Open `src/apps/gui/main.c` (or wherever application startup happens).
    - [x] **Action:** Call `network_init()` at startup and `network_cleanup()` before exiting.
    - [x] **Verification (Interactive Test):**
        1.  Run: `make clean && make`
        2.  Run: `./build/biotope` (or equivalent execution command).
        3.  **Expected Result:** The application launches and closes without segfaults or compilation warnings.

---

## Phase 5: C-Application - Leaderboard Fetch Thread

*Goal: Implement the background threading and JSON parsing for the leaderboard.*

- [x] **Step 5.1: Implement Leaderboard Fetcher (`network_io.c`)**
    - [x] **Action:** Create a static `void* fetch_leaderboard_thread(void* arg)` function.
    - [x] **Action:** In this thread, use `libcurl` to `GET` the `/api/leaderboard` endpoint. Handle `CURLOPT_TIMEOUT` (e.g., 5s).
    - [x] **Action:** Use a `libcurl` write callback to collect the response body into a dynamically allocated string buffer.
    - [x] **Action:** Parse the string buffer using `cJSON_Parse`. **Crucial:** Extract the array, lock `g_network_mutex`, populate `g_leaderboard`, set `g_leaderboard.is_ready = true`, unlock the mutex.
    - [x] **Action:** Call `cJSON_Delete` and `free()` to prevent memory leaks!

- [x] **Step 5.2: Implement Async Trigger and Getter**
    - [x] **Action:** Implement `network_fetch_leaderboard_async(void)` to span `fetch_leaderboard_thread` using `pthread_create` as a detached thread.
    - [x] **Action:** Implement `network_get_leaderboard(LeaderboardData* out_data)`: Lock mutex, check `is_ready`, `memcpy` if true, set `is_ready = false`, unlock mutex, return bool.

- [x] **Step 5.3: Verification of Leaderboard Logic**
    - [x] **Action:** In `src/gui/app_state_manager.c` (or equivalent update loop), add a temporary test: trigger fetch on a specific keypress (e.g., `L`), and poll `network_get_leaderboard` every frame. If it returns true, `printf` the first user's name to the terminal.
    - [x] **Verification (Interactive Test):**
        1.  Start the Python backend.
        2.  Compile and run the C-App.
        3.  Press `L`.
        4.  **Expected Result:** The application does NOT stutter/freeze. A fraction of a second later, the terminal prints the fetched leaderboard name.

---

## Phase 6: C-Application - Highlights Fetch Thread & Memory Safety

*Goal: Implement the background threading for 8x8 seed highlights and ensure zero memory leaks.*

- [x] **Step 6.1: Implement Highlights Fetcher**
    - [x] **Action:** Duplicate the logic from Phase 5 to create `fetch_highlights_thread` for `/api/epoch/highlights`.
    - [x] **Action:** Inside the `cJSON` parsing logic, carefully iterate through the JSON arrays to populate the `seed_red` and `seed_blue` 64-int arrays. Validate array bounds (`cJSON_GetArraySize`).
    - [x] **Action:** Lock mutex, populate `g_highlights`, unlock.
    - [x] **Action:** Call `cJSON_Delete` and `free()`.

- [x] **Step 6.2: Implement Highlights Trigger and Getter**
    - [x] **Action:** Implement `network_fetch_highlights_async(void)` and `network_get_highlights(HighlightData* out_data)`.

- [x] **Step 6.3: Final Verification and Valgrind Test**
    - [x] **Action:** Add a temporary keybind (e.g., `H`) to trigger the highlights fetch, and poll for completion in the update loop, printing a success message.
    - [x] **Verification (Interactive Test):**
        1.  Compile the C-App.
        2.  Run with Valgrind: `valgrind --leak-check=full ./build/biotope`
        3.  Press `L` and `H` several times to trigger threads and cJSON parsing.
        4.  Exit the application normally.
        5.  **Expected Result:** `valgrind` reports `0 bytes leaked` (excluding known Raylib/X11 driver single-allocations). The UI remains perfectly smooth during the requests.

---
**Completion:** Once Phase 6 is complete and verified, the C-Networking layer is robust and ready for the Kiosk Mode UI implementation. Please update `docs/CHANGELOG.md` accordingly.
