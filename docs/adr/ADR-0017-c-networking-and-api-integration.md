### **ADR-0017: C-Networking and API Integration (libcurl & cJSON)**

**Status:** Proposed

**Date:** 2026-05-27

#### **1. Context and Problem Statement**

The "Game of Life - Biotope" project is being expanded to include a Uni-Messe Kiosk Mode. This mode requires displaying a live leaderboard and rendering "Greatest Hits" (highlight matches) from the ongoing multiplayer tournament. Currently, the Python FastAPI backend manages the tournament via a background worker and stores match results in MongoDB. However, the Raylib C-application—which will act as the visual Kiosk frontend—operates in isolation and lacks the capability to dynamically fetch remote tournament data. To realize the Kiosk Mode, the C-application must be able to communicate with the FastAPI backend over HTTP to retrieve structured JSON payloads containing rankings and simulation seed configurations.

#### **2. Decision**

We will integrate a lightweight C-networking layer using `libcurl` and `cJSON` into the `src/io/` module of the C-application.

*   **Backend Extension (WP 1):** The FastAPI backend will be extended with new endpoints (`/api/leaderboard` and `/api/epoch/highlights`) and a new MongoDB collection (`epoch_highlights`) to serve tournament data.
*   **C-App Data Layer (WP 2):** The C-application will use `libcurl` to execute HTTP GET requests against these new endpoints. The incoming JSON responses will be parsed using the existing `cJSON` library (already present in `src/vendor/`). 
*   **Architecture Compliance:** This networking capability will be strictly encapsulated within the `src/io/` module (e.g., in a new `network_io.c` or integrated into `file_io.c`). This adherence to our Clean Architecture ensures that the core game logic (`src/core/`) and rendering modules (`src/gui/`) remain completely unaware of the network implementation.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

*   **Real-time Synchronization:** Enables near real-time data synchronization between the backend tournament engine and the visual Kiosk client, fulfilling the Uni-Messe requirements.
*   **Industry Standards:** Utilizes `libcurl` (the de facto standard for C networking) and `cJSON`, ensuring robust, cross-platform, and well-documented behavior.
*   **Clean Architecture:** By isolating network IO in the `src/io/` folder, we maintain strict module boundaries and prevent network logic from leaking into the Raylib rendering loop.
*   **Stateless Retrieval:** Polling REST endpoints is stateless and highly resilient to brief network drops, which is ideal for a long-running Kiosk environment.

**Negative Consequences (Disadvantages):**

*   **New Dependency:** Introduces `libcurl` as a new external build dependency, requiring updates to the `Makefile` and local development environment setup instructions.
*   **Blocking vs. Non-Blocking:** Standard `libcurl` synchronous requests block the execution thread. If executed on the main thread, slow network responses will stall the Raylib 60FPS render loop. We must implement careful timeout management or background threading for network polling.
*   **Error Handling Complexity:** Requires robust C-level error handling for timeouts, malformed JSON, and dropped connections to prevent the application from crashing when left unattended.

#### **4. Alternatives Considered**

*   **WebSocket Connection:** 
    *   *Concept:* Establish a persistent, bi-directional WebSocket connection between the C-app and the Python backend.
    *   *Rejected:* Over-engineering. The tournament data updates infrequently (only once per epoch, ~every 60 seconds). Simple HTTP GET polling is perfectly adequate, far more resilient to drops, and significantly less complex to implement in C than a WebSocket client.
*   **Shared Database Access (C-App reading MongoDB directly):** 
    *   *Concept:* Use a MongoDB C driver to connect the Kiosk application directly to the database.
    *   *Rejected:* Violates separation of concerns and security principles. The Kiosk client should not possess direct database access credentials. The FastAPI backend must serve as the secure, validated intermediary.
*   **File-System Polling (NFS/Shared Folder):** 
    *   *Concept:* Have the Python worker write JSON output files to a shared directory that the C-application reads using the existing `file_io.c`.
    *   *Rejected:* While simpler to implement in C, it tightly couples the deployment architecture. The backend and Kiosk client might not run on the same physical machine at the Uni-Messe. Network-based HTTP APIs provide necessary deployment flexibility.
