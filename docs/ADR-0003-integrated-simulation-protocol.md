### **ADR-0003: Integrated Simulation Protocol and Replay System**

**Status:** Proposed

**Date:** 2026-01-17

#### **1. Context and Problem Statement**

The "Biotope - Game of Life" application currently allows users to configure and run competitive simulations. However, the system lacks a comprehensive mechanism to document and reproduce these simulations. 

Currently:
- The **initial state** (grid configuration) is mutable and is overwritten in memory as the simulation progresses. Unless a user manually saves before running, the starting setup is lost.
- The **simulation parameters** (e.g., `max_rounds`, `delay_ms`) are not persisted in the current `.bio` file format.
- **Results logging** is limited to a Markdown summary at the end, which is insufficient for re-running the exact same scenario.
- There is no **integrated user interface** to browse, select, and load previous simulation protocols; users would have to rely on external file management.

We need a solution that ensures every simulation run is fully documented (protocolled) and can be easily reloaded by the user to verify results or replay interesting scenarios.

#### **2. Decision**

We will implement an **Integrated Simulation Protocol System** ("Solution 1") with the following core components:

1.  **Extended Protocol Format:** 
    We will extend the existing file I/O logic to support a comprehensive protocol format (likely preserving the `.bio` extension or using a new `.log` extension). This format will store:
    - **Header:** Configuration metadata (`max_rounds`, `delay_ms`, `max_population`, timestamp).
    - **Body:** The complete initial grid state (RLE or coordinate list).
    - **Footer (Optional/Appended):** Simulation results (Winner, final scores), appended if the simulation completes successfully.

2.  **"Save-on-Start" Strategy:**
    To ensure robustness, the application will automatically serialize the complete *initial* state (Grid + Config) to a file in the `biotope_results/` directory immediately upon transitioning from `STATE_EDIT` to `STATE_RUNNING`. This guarantees that a protocol exists even if the application crashes during execution.

3.  **In-App File Browser (UI):**
    A new UI state (`STATE_LOAD`) will be implemented to replace the generic "Load" function. This custom interface will:
    - Scan the `biotope_results/` directory.
    - Allow the user to navigate through available protocol files using keyboard input (e.g., Arrow Keys).
    - Display metadata previews (Date, Config) for the selected file.
    - Load the selected file into the `STATE_EDIT` mode for inspection or replay.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**
- **Complete Reproducibility:** Users can exactly recreate any past simulation run, satisfying the core requirement for a "protocol".
- **Robustness:** By saving at the start, data is preserved even in the event of a runtime crash or power failure.
- **Usability:** The integrated browser eliminates the need for users to interact with the OS file system to swap "save files".
- **Portability:** A single file contains everything needed to define a simulation run, making it easy to share specific scenarios between users.

**Negative Consequences (Disadvantages):**
- **Implementation Complexity:** Implementing a directory scanner and a scrollable list UI in Raylib (immediate mode drawing) is more involved than using standard OS dialogs.
- **Storage Footprint:** Automatically saving every run will generate a large number of files over time, potentially requiring a future "cleanup" feature.
- **Format Migration:** The loader must be written defensively to handle legacy `.bio` files that lack the new protocol headers.

#### **4. Alternatives Considered**

**Alternative 2: Snapshot & Memory (Save-at-End)**
- *Concept:* Keep a copy of the initial world in RAM and write the protocol file only after the simulation finishes (`STATE_GAME_OVER`).
- *Pros:* Reduces disk I/O; avoids creating files for aborted/mistaken runs.
- *Cons:* **Critical failure point:** If the application crashes during the simulation, the initial configuration is lost forever. This was deemed unacceptable for a "protocol" system.

**Alternative 3: JSON Manifest (Separated Data)**
- *Concept:* Use a central `manifest.json` to track runs and separate grid data (`.bio`) from metadata (`.json`).
- *Pros:* Cleaner data structure; standard format for metadata.
- *Cons:* Introduces a dependency on a JSON parser (e.g., `cJSON`) or complex string parsing in C. Given the project's constraint to minimize external dependencies and maintain simplicity, adding a JSON handling layer was rejected in favor of a custom, line-based text format.
