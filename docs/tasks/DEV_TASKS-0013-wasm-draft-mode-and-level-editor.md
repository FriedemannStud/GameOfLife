# DEV_TASKS-0013: WASM Draft Mode and Level Editor

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality.

**Briefing Documents:**
*   [ADR-0013: WASM Draft Mode and Level Editor](../adr/ADR-0013-wasm-draft-mode-and-level-editor.md)
*   [DEV_SPEC-0013: Requirements Analysis & Specification](../specs/DEV_SPEC-0013-wasm-draft-mode-and-level-editor.md)
*   [DEV_TECH_DESIGN-0013: Technical Specification](../tech_design/DEV_TECH_DESIGN-0013-wasm-draft-mode-and-level-editor.md)

---

## Phase 1: State Machine & UI Foundations

*Goal: Introduce the `UI_STATE_DRAFT` into the application lifecycle and render the basic visual overlay (Bounding Box) for the drafting area.*

- [ ] **Step 1.1: Define UI States**
    - [ ] **Action:** Open `gui.h` and define an enum for the application states if one doesn't already exist (e.g., `typedef enum { UI_STATE_SIMULATION, UI_STATE_DRAFT } ui_state_t;`).
    - [ ] **Action:** Add a global or static variable in `gui.c` to track the current state: `ui_state_t current_ui_state = UI_STATE_SIMULATION;`.
    - [ ] **Action:** Ensure `// KI-Agent unterstützt: Added UI state machine` is added above the declaration.

- [ ] **Step 1.2: Add "Draft Mode" Toggle Button**
    - [ ] **Action:** In `gui.c` (inside the HUD rendering logic, e.g., `draw_hud`), add a Raylib button (using `CheckCollisionPointRec` and `IsMouseButtonPressed` or `raygui` if included). 
    - [ ] **Action:** The button should toggle `current_ui_state` between `UI_STATE_SIMULATION` and `UI_STATE_DRAFT`.
    - [ ] **Action:** When entering `UI_STATE_DRAFT`, pause the simulation (if not already paused) and clear the grid to provide a blank canvas.
    - [ ] **Verification (Interactive Test):**
        1. Compile the desktop version using `make clean && make`.
        2. Run `./biotope`.
        3. Click the newly created "Draft Mode" button.
        4. **Expected Result:** The application should transition smoothly, the simulation should stop, and the grid should clear. Clicking it again should return to simulation mode. Report the result.

- [ ] **Step 1.3: Render the 8x8 Bounding Box**
    - [ ] **Action:** In `gui.c`, modify the grid drawing loop. If `current_ui_state == UI_STATE_DRAFT`, calculate the screen coordinates for the top-left 8x8 grid cells.
    - [ ] **Action:** Draw a semi-transparent rectangle (e.g., `DrawRectangle(x, y, width, height, (Color){ 0, 121, 241, 50 })`) over this 8x8 area to visually indicate the valid drafting zone.
    - [ ] **Verification (Interactive Test):**
        1. Recompile using `make`.
        2. Run `./biotope` and enter Draft Mode.
        3. **Expected Result:** A visual bounding box overlay appears strictly over the 8x8 area on the grid. Report the result.

---

## Phase 2: Grid Interaction & Biomass Validation

*Goal: Allow the user to place and remove cells within the 8x8 bounding box, strictly enforcing the 24-cell limit.*

- [ ] **Step 2.1: Implement Mouse Interaction**
    - [ ] **Action:** In the `gui.c` input handling section, add logic that only executes if `current_ui_state == UI_STATE_DRAFT`.
    - [ ] **Action:** Detect `IsMouseButtonPressed(MOUSE_LEFT_BUTTON)`. Calculate the grid coordinates `gx` and `gy` based on the mouse position and the current camera/zoom offsets.
    - [ ] **Action:** Restrict interactions: Only process clicks if `0 <= gx < 8` and `0 <= gy < 8`.

- [ ] **Step 2.2: Biomass Counting and Toggling Logic**
    - [ ] **Action:** Create a helper function `int count_draft_biomass(World *world)` that iterates over the 8x8 area and returns the number of living cells.
    - [ ] **Action:** Implement toggle logic on click:
        - If the clicked cell is DEAD and `count_draft_biomass(world) < 24`: Set cell to ALIVE.
        - If the clicked cell is ALIVE: Set cell to DEAD.
    - [ ] **Verification (Interactive Test):**
        1. Recompile and run `./biotope`.
        2. Enter Draft Mode.
        3. Click inside the 8x8 area to place cells. Verify you cannot place cells outside this area.
        4. Place 24 cells. Try to place a 25th cell.
        5. **Expected Result:** The 25th cell is rejected. Clicking an existing cell removes it. Report the result.

- [ ] **Step 2.3: Render Biomass Counter**
    - [ ] **Action:** In `gui.c` (HUD rendering), if in `UI_STATE_DRAFT`, draw text displaying the current biomass (e.g., `DrawText(TextFormat("Biomass: %d/24", current_biomass), ...)`).
    - [ ] **Action:** Use a warning color (e.g., Red) if the biomass is exactly 24.
    - [ ] **Verification:** Recompile and verify the counter updates accurately in real-time as cells are placed and removed.

---

## Phase 3: Identity & Nickname Input

*Goal: Implement persistent player identity using WASM JS-bridges and allow nickname entry.*

- [ ] **Step 3.1: Player ID (`EM_JS` bridge)**
    - [ ] **Action:** Open `gui.c`. Add a function `char* get_or_create_player_id(void)`.
    - [ ] **Action:** Wrap the implementation in `#ifdef __EMSCRIPTEN__` ... `#else` ... `#endif`.
    - [ ] **Action:** For the Emscripten block, implement the `EM_JS` macro as specified in the Technical Design (Section 4) to fetch or generate a UUID from `localStorage`.
    - [ ] **Action:** For the desktop block (`#else`), return a mock static string (e.g., `"local-dev-uuid"`).
    - [ ] **Action:** Call this function once during initialization and store the UUID in a global/static variable `char current_player_id[64];`.

- [ ] **Step 3.2: Nickname Input Field**
    - [ ] **Action:** Add a static variable `char player_nickname[33] = "\0";` and an `int letter_count = 0;`.
    - [ ] **Action:** Render a text box in the HUD when in `UI_STATE_DRAFT`.
    - [ ] **Action:** Implement standard Raylib text input logic using `GetCharPressed()` and `IsKeyPressed(KEY_BACKSPACE)` to populate `player_nickname`.
    - [ ] **Verification (Interactive Test):**
        1. Compile desktop version. Run `./biotope`.
        2. Enter Draft Mode.
        3. Type a nickname into the input field and use backspace.
        4. **Expected Result:** Text is captured and displayed correctly in the UI. Report the result.

---

## Phase 4: Serialization & API Submission

*Goal: Package the grid and metadata into a cJSON payload and submit it to the FastAPI backend via WASM.*

- [ ] **Step 4.1: cJSON Serialization**
    - [ ] **Action:** Ensure `cJSON.h` is included. Create a function `char* create_submission_json(World *world, const char* p_id, const char* name)`.
    - [ ] **Action:** Use `cJSON_CreateObject()`, `cJSON_CreateArray()`, etc., to build the JSON structure defined in `DEV_TECH_DESIGN-0013` (Section 3).
    - [ ] **Action:** Iterate the 8x8 grid. For every living cell, append an array `[x, y]` to the `cells` JSON array.
    - [ ] **Action:** Return `cJSON_PrintUnformatted(root)` and ensure `cJSON_Delete(root)` is called to prevent memory leaks. Add `// KI-Agent unterstützt: cJSON payload generation`.

- [ ] **Step 4.2: Submit Button and WASM Fetch**
    - [ ] **Action:** Add a "Submit Pattern" button to the Draft Mode HUD. It should only be clickable if `current_biomass > 0` and `player_nickname` is not empty.
    - [ ] **Action:** On click, generate the JSON payload using `create_submission_json`.
    - [ ] **Action:** Wrap the network call in `#ifdef __EMSCRIPTEN__`. Include `<emscripten/fetch.h>`.
    - [ ] **Action:** Initialize `emscripten_fetch_attr_t`, set `requestMethod` to "POST", set `requestHeaders` (Content-Type: application/json), and set the body to the JSON string.
    - [ ] **Action:** Implement `onsuccess` and `onerror` callbacks to update a UI status message (e.g., "Submission Successful!" or "Error!").
    - [ ] **Action:** For the desktop block (`#else`), simply `printf` the generated JSON string to stdout to simulate a successful send.
    - [ ] **Action:** `free()` the JSON string returned by `cJSON_PrintUnformatted` after it has been used.

- [ ] **Step 4.3: Emscripten Fetch Build Configuration**
    - [ ] **Action:** Open `Makefile.wasm`. Ensure the linker flags include `-s FETCH=1` so the Emscripten fetch API is compiled into the binary.

- [ ] **Step 4.4: Verification (Interactive Test)**
    - [ ] **Action:** First test locally: `make clean && make`. Run `./biotope`, design a pattern, and click Submit. Confirm the JSON prints to the console correctly.
    - [ ] **Action:** Build the WASM version: `make -f Makefile.wasm`.
    - [ ] **Action:** Serve the directory locally (e.g., `python3 -m http.server 8080`) and open `biotope.html`.
    - [ ] **Action:** Ensure the backend is running (`docker-compose up -d backend`).
    - [ ] **Action:** Draft a pattern, enter a name, and submit.
    - [ ] **Expected Result:** The browser's network tab shows a successful POST request to `/api/v1/submit_config`, and the UI displays the success status. Report the result.

---

## Phase 5: Cleanup & Code Quality

*Goal: Ensure the codebase adheres strictly to our quality guidelines before finalizing the feature.*

- [ ] **Step 5.1: Memory Leak Check**
    - [ ] **Action:** Review `get_or_create_player_id()` and `create_submission_json()`. Ensure every `_malloc` or `cJSON_Print` output is properly `free`'d after use.
    
- [ ] **Step 5.2: Code Formatting and Guidelines**
    - [ ] **Action:** Ensure all new functions, variables, and structs follow the naming conventions (`snake_case` for vars/funcs, `PascalCase` for structs).
    - [ ] **Action:** Verify that `// KI-Agent unterstützt` is placed appropriately above AI-generated logic blocks.
    - [ ] **Action:** Run `make clean && make` to verify compilation with `-Wall -Wextra` produces zero warnings.

- [ ] **Step 5.3: Update Documentation**
    - [ ] **Action:** Update `CHANGELOG.md` with a new feature entry linking to `DEV_TASKS-0013` and `ADR-0013`.
    - [ ] **Action:** Mark all tasks in this document as checked `[x]`.