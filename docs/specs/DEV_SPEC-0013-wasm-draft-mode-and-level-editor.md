# Requirements Analysis & Specification: Mobile-First Web Draft Editor

This document details the requirements for the standalone web-based pattern editor and submission system, as described in **ADR-0013**.

---

### 1. Detailed Requirements Specification

The **Draft Editor** is a standalone web application designed for smartphones, enabling users to create and submit Game of Life patterns for the competitive Biotope platform.

#### **Core Functionality:**
- **Touch-Optimized Grid Editing:** Users toggle cell states (Alive/Dead) by tapping on a responsive 8x8 grid.
- **Biomass Validation:** Real-time display of placed cells (e.g., "Cells: 12/24"). The UI prevents adding more than 24 cells.
- **User Identity:**
    - **Nickname:** A standard HTML text input field.
    - **Persistent Player ID:** Automatic generation of a unique UUID on first visit, stored in `localStorage`.
- **Submission System:**
    - "Submit" button active only when the pattern is valid (1-24 cells).
    - Asynchronous transmission to the FastAPI backend.
    - Clear visual feedback for "Submitting...", "Success", and "Error".

#### **Technical Constraints:**
- **Technology Stack:** HTML5, CSS3 (Mobile-First), Vanilla JavaScript.
- **Responsive Design:** Must fit comfortably on standard smartphone screens (portrait mode).
- **Communication:** Browser `fetch()` API for JSON POST requests.
- **Hosting:** Served as a static page (e.g., `editor.html`) alongside the main Biotope viewer or via a dedicated path.

---

### 2. User Stories & Acceptance Criteria

**Epic: Mobile Pattern Design and Submission**

*   **User Story 1: Touch-Based Editing**
    *   **As a mobile player,** I want to tap on a grid to design my pattern, **so that** I can easily participate from my smartphone.
    *   **Acceptance Criteria:**
        *   Tapping a grid cell toggles its state.
        *   Grid is responsive and centered on mobile viewports.
        *   Visual feedback on cell state (e.g., Neon Blue for Alive, Dark for Dead).

*   **User Story 2: Mobile-Native Input**
    *   **As a player,** I want to use my phone's keyboard to enter my name, **so that** I can identify my submissions.
    *   **Acceptance Criteria:**
        *   Standard HTML text input activates the native mobile keyboard.
        *   Input is clearly labeled and accessible.

*   **User Story 3: Identity Persistence**
    *   **As a player,** I want the system to remember my ID on my phone, **so that** my ELO rating is correctly tracked across multiple submissions.
    *   **Acceptance Criteria:**
        *   UUID is generated once and stored in `localStorage`.
        *   Subsequent submissions use the same `player_id`.

*   **User Story 4: Instant Feedback & Submission**
    *   **As a player,** I want to know if my pattern is valid before I hit submit, **so that** I don't waste time on rejected attempts.
    *   **Acceptance Criteria:**
        *   Real-time counter (X/24).
        *   Submit button is disabled if rules are violated.
        *   Success message provides confirmation after submission.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Responsive 8x8 Grid.
        *   Cell toggle logic.
        *   `localStorage` for `player_id`.
        *   API integration (POST to `/api/v1/submit_config`).
    *   **Should-Have:**
        *   Visual polish (matching Biotope aesthetic).
        *   Success/Error toast notifications.
        *   Loading spinner during submission.
    *   **Could-Have:**
        *   "Reset Grid" button.
        *   QR Code link from the main Biotope viewer.

*   **Dependencies:**
    1.  **Backend Accessibility:** The API must have CORS enabled for the domain where the editor is hosted.
    2.  **Schema Alignment:** The JSON payload must strictly match the `Submission` Pydantic model in the backend.

---

### 4. Definition of Done (DoD)

A Product Backlog Item is considered "Done" when:

*   **Responsiveness:** The UI is fully functional on iOS (Safari) and Android (Chrome).
*   **Validation:** 
    *   Frontend blocks >24 cell placements.
    *   Backend confirms acceptance of generated JSON.
*   **Networking:** `fetch()` call handles network errors gracefully.
*   **Identity:** `player_id` is successfully retrieved from `localStorage` on reload.
*   **Documentation:** `CHANGELOG.md` reflects the new web-based editor.
