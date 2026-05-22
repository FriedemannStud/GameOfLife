# DEV_TASKS-0013: Mobile-First Web Draft Editor

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality.

**Briefing Documents:**
*   [ADR-0013: Mobile-First Web Draft Editor](../adr/ADR-0013-wasm-draft-mode-and-level-editor.md)
*   [DEV_SPEC-0013: Requirements Analysis & Specification](../specs/DEV_SPEC-0013-wasm-draft-mode-and-level-editor.md)

---

## Phase 1: Foundation & Responsive Grid

*Goal: Create the basic HTML structure and a touch-responsive 8x8 grid that fits smartphone screens.*

- [x] **Step 1.1: Create `editor.html` and Styles**
    - [x] **Action:** Create `editor.html` in the project root.
    - [x] **Action:** Implement a mobile-first layout using CSS Grid for the 8x8 area.
    - [x] **Action:** Use a "Biotope" aesthetic: dark background, neon blue/red accents.
    - [x] **Action:** Add a viewport meta tag for mobile responsiveness.
    - [x] **Verification:** Open `editor.html` in a browser (mobile simulation mode). The grid should be centered and fill the width of the screen. (Verified: 2026-05-22)

- [x] **Step 1.2: Implement Cell Toggle Logic**
    - [x] **Action:** Add JavaScript to handle clicks/taps on grid cells.
    - [x] **Action:** Toggle a CSS class (e.g., `.alive`) to visually change the cell state.
    - [x] **Action:** Maintain an internal state (e.g., a 2D array or list of coordinates) of living cells.
    - [x] **Verification:** Tapping cells in the browser should toggle their color. (Verified: 2026-05-22)

---

## Phase 2: Biomass Validation & Identity

*Goal: Enforce the "Fair Play" rules and manage persistent user identity.*

- [x] **Step 2.1: Real-time Biomass Counter**
    - [x] **Action:** Add a `<span>` to display the current cell count (e.g., "Cells: 12/24").
    - [x] **Action:** Update this counter on every toggle.
    - [x] **Action:** Block cell placement if the count reaches 24.
    - [x] **Verification:** Verify that you cannot activate more than 24 cells. (Verified: 2026-05-22)

- [x] **Step 2.2: LocalStorage Identity (UUID)**
    - [x] **Action:** Implement a script to check `localStorage` for a `player_id`.
    - [x] **Action:** If not found, generate a unique UUID and save it.
    - [x] **Action:** Log the `player_id` to the console for verification during development.
    - [x] **Verification:** Refresh the page. The `player_id` should remain the same in the console. (Verified: 2026-05-22)

- [x] **Step 2.3: Nickname Input**
    - [x] **Action:** Add an `<input type="text">` for the nickname.
    - [x] **Action:** Ensure it has a maximum length (e.g., 32 characters).
    - [x] **Verification:** Tapping the input on a mobile device should trigger the native keyboard. (Verified: 2026-05-22)

---

## Phase 3: API Integration & Submission

*Goal: Package the data into the correct JSON format and submit it to the backend.*

- [x] **Step 3.1: JSON Payload Generation**
    - [x] **Action:** Create a function `generatePayload()` that constructs the JSON object matching the `Submission` Pydantic model.
    - [x] **Action:** Example structure:
        ```json
        {
          "metadata": { "player_id": "...", "nickname": "...", "league": "local" },
          "config": { "bounding_box_x": 8, "bounding_box_y": 8, "cells": [[x,y], ...] }
        }
        ```
    - [x] **Verification:** Log the generated JSON to the console and compare it against `backend/app/models.py`. (Verified: 2026-05-22)

- [x] **Step 3.2: Asynchronous Submission (Fetch)**
    - [x] **Action:** Add a "Submit" button.
    - [x] **Action:** Implement an `async` function to `fetch()` the backend endpoint `POST /api/v1/submit_config`.
    - [x] **Action:** Handle loading states (e.g., disable button, show "Submitting...") and response statuses (201 Created vs. 400/500 Errors).
    - [x] **Verification:** 
        1. Run the backend (`docker-compose up`).
        2. Submit a pattern from the browser.
        3. Verify the "Success" message and check the backend logs/database for the new entry. (Verified: 2026-05-22)

---

## Phase 4: Integration & Visual Polish

*Goal: Link the editor to the main application and finalize the UI.*

- [x] **Step 4.1: QR Code / Link Integration**
    - [x] **Action:** In the main Raylib app (`gui.c`), add a link or a "Create Pattern" button that opens `editor.html`.
    - [x] **Action:** (Optional) If running on a desktop viewer, display a QR code pointing to the hosted `editor.html` URL.
    - [x] **Verification:** Clicking the button in the WASM viewer should open the editor in a new tab. (Verified via system-call log: 2026-05-22)

- [x] **x] **Step 4.2: Final Aesthetic Review**
    - [x] **Action:** Ensure fonts, colors, and animations match the main Biotope app.
    - [x] **Action:** Add a "Clear Grid" button for convenience.
    - [x] **Verification:** Final walkthrough on a physical smartphone. (Verified: 2026-05-22)

---

## Phase 5: Documentation & Cleanup

- [x] **Step 5.1: Update Changelog**
    - [x] **Action:** Add the Mobile Draft Editor to `CHANGELOG.md`. (Verified: 2026-05-22)
- [x] **Step 5.2: Final Check off**
    - [x] **Action:** Ensure all tasks in this document are marked as `[x]`. (Verified: 2026-05-22)
