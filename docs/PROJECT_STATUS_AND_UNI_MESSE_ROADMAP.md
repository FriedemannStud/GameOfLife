# Project Status and Uni-Messe Architecture Roadmap

## 1. Executive Summary

The "Game of Life - Biotope" project encompasses two distinct play concepts designed to cater to both individual exploration and competitive group dynamics.

*   **Singleplayer Concept:** A local, interactive experience where a single user configures simulation parameters (Grid Size, Delay, Max Rounds, Max Init Pop), designs starting configurations for Team Red and Team Blue via an integrated GUI, and visually observes the resulting simulation.
*   **Multiplayer Tournament (Uni-Messe Concept):** A distributed, competitive ecosystem designed for a Uni-Messe (Uni-Messe) environment. Participants use a mobile-friendly HTML editor to upload 8x8 starting configurations to a central database. A backend worker periodically (every 60s) simulates a Round-Robin tournament among all submissions head-to-less using a high-performance C-engine. The results are aggregated into a persistent leaderboard.

This document establishes the current implementation status of these concepts and specifies the architectural roadmap for completing the Uni-Messe visualization requirements.

---

## 2. System Architecture (Current State)

The project leverages a hybrid architecture, utilizing Python/FastAPI for orchestration and web interfaces, and a highly optimized C-engine (Raylib + OpenMP) for simulation and rendering.

### 2.1 Singleplayer Core (C-Application)
*   **Status: Completed.**
*   **Implementation:** The C-application operates via a well-defined state machine (`app_state_manager.c`). Users navigate through `STATE_CONFIG`, `STATE_EDIT_RED`, and `STATE_EDIT_BLUE` to setup the game. Rendering is handled by `renderer.c` using Raylib. The core simulation logic (`game_logic.c`) is fully functional.

### 2.2 Multiplayer Backend & Simulation Engine
*   **Status: Completed.**
*   **Implementation:** 
    *   **Editor:** An external `editor.html` allows users to design and submit patterns to a REST API.
    *   **Backend:** A Python backend (`backend/app/main.py`) stores configurations in MongoDB.
    *   **Tournament Worker:** A periodic worker (`backend/app/worker.py`) fetches all configurations every 60 seconds and initiates an Epoch.
    *   **Headless Execution:** The worker delegates the actual simulation to `biotope_hyper_worker` (compiled from `main_hyper.c`), which executes a 1v1 Round-Robin tournament using OpenMP for parallelization without graphical overhead. Results and rankings are written back to the database.

---

## 3. Uni-Messe Kiosk Mode (Target State & Specification)

The critical missing component is the visual representation of the tournament results within the C-application for the Uni-Messe audience. To satisfy the requirements of individual analysis, observer engagement, and competitive tracking, a new **Uni-Messe Kiosk Mode** will be implemented within the Raylib C-application.

### 3.1 State Machine Extension
A new overarching state, `STATE_KIOSK_MODE`, will be introduced. This state acts as an idle-loop controller, cycling between different visualization modules. User input (e.g., mouse movement or click) will interrupt the Kiosk loop, allowing for interactive selection.

### 3.2 Modul 1: Live-Rangliste (Competitive Tracking)
*   **Behavior:** Displays the current tournament standings (Top 10/20 participants, Elo ratings, Win Rates).
*   **Implementation:** The C-application will periodically query a new REST API endpoint (e.g., `/api/leaderboard`) from the Python backend. The data will be parsed using the existing `cJSON` library and rendered as a tabular UI using Raylib text drawing functions.

### 3.3 Modul 2: Wusel-Multicam (Observer Engagement)
*   **Behavior:** A visually dynamic "screensaver" mode that captures attention without degrading into visual chaos.
*   **Architecture:** **Multicam + Seed-based Replays.**
    *   **On-the-fly Generation:** To minimize data transfer and ensure high fidelity, the C-application fetches only the starting seeds (8x8 patterns) and participant names. It utilizes the shared `game_logic.c` to simulate the match locally in real-time.
    *   **Highlights Selection:** The backend worker identifies "Greatest Hits" during the tournament based on two primary metrics:
        1.  **Duration (Metric 1):** Matches that last the longest before reaching stability.
        2.  **Volatility (Metric 4):** Matches with the highest fluctuation in cell counts (maximum visual action).
    *   **Multicam Layout:** The C-application divides the screen into a grid (e.g., 2x2 or 3x3).
    *   **RenderContext Refactoring:** `renderer.c` will be refactored to replace global static GPU resources with a `RenderContext` struct, allowing multiple independent `World` instances to be rendered simultaneously in different viewports.

### 3.4 Modul 3: High-Fidelity Replay (Individual Analysis)
*   **Behavior:** Allows an individual user to step out of the Kiosk Mode to focus on a specific match.
*   **Implementation:** From the Leaderboard or Multicam view, users can click to select a specific match. The application transitions to a modified `STATE_LOAD`/`STATE_RUNNING` view, taking over the full screen to replay the seeds with full graphical fidelity (colors, grid lines, UI overlay).

---

## 4. Data Flow & Interfaces

To support the Kiosk Mode, the interface between the C-application and the Python backend must be expanded:

1.  **Backend Extensions:**
    *   `GET /api/leaderboard`: Returns the current tournament standings and Elo ratings.
    *   `GET /api/epoch/highlights`: Returns the metadata (names, 8x8 seeds, and metric type) for the top matches of the latest epoch.
2.  **Persistence Layer:**
    *   A new MongoDB collection `epoch_highlights` will be implemented to store the metadata of selected matches permanently.
3.  **C-Application C-Networking:**
    *   A lightweight HTTP client integration (e.g., `libcurl`) will be added to `file_io.c` to fetch these JSON payloads dynamically.

---

## 5. Roadmap & Next Steps

The following work packages are refined to reflect the technical architecture:

*   **WP 1 (Backend):** Implement `epoch_highlights` storage and API endpoints for `/api/leaderboard` and `/api/epoch/highlights`.
*   **WP 2 (C-App Data Layer):** Integrate `libcurl` and `cJSON` to retrieve match seeds and rankings from the REST API.
*   **WP 3 (C-App Refactoring):** Refactor `renderer.c` and `game_logic.c` to utilize `RenderContext` for instanced rendering of multiple worlds.
*   **WP 4 (C-App UI):** Implement `STATE_KIOSK_MODE` with a cycling view between the Leaderboard and the Multicam grid.
*   **WP 5 (Integration & Polish):** Finalize the "Uni-Messe Kiosk" experience with automated rotations and user-interrupt for High-Fidelity replays.