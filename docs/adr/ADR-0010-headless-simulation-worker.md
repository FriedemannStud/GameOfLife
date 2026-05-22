### **ADR-0010: Headless Simulation Worker**

**Status:** Proposed

**Date:** 2026-05-22

#### **1. Context and Problem Statement**

The project is transitioning into an asynchronous, massive-parallel multiplayer ecosystem. In this new architecture, users submit their starting configurations (restricted to a uniform 8x8 grid, as the previously discussed league system has been discarded) to a central server. To determine the most sustainable configuration, these submissions must battle against each other.

The current implementation tightly couples the simulation logic (`game_logic.c`) with the graphical user interface (`gui.c` and Raylib). Running thousands of matches on a server using a graphical application is computationally expensive, slow, and operationally complex. We need a way to execute the game logic in a fast, automated, and scalable manner on a server environment without any graphical overhead. 

The required worker must accept two JSON configurations as input, simulate the match for exactly 100 generations, and output the winner.

#### **2. Decision**

We will implement a **Headless Mode** for the C-Simulation. 

This will involve adapting the existing C-codebase (specifically `main.c` and `game_logic.c`) so that it can be compiled and executed purely via the command line interface (CLI), bypassing `gui.c` and Raylib entirely. 

**Key Technical Decisions:**
- **Execution:** The executable will accept CLI arguments specifying the paths to two JSON configuration files (representing Team Red and Team Blue).
- **Simulation Constraints:** The worker will initialize the grid with the provided 8x8 configurations, run the simulation logic for exactly 100 generations, and then halt.
- **Output:** The worker will output the final result (the winner based on the highest living cell count) to standard output (stdout) or a specified result JSON file, making it easily consumable by the backend matchmaking service.
- **Code Reuse:** The core simulation loop within `game_logic.c` will remain unchanged to ensure absolute consistency between the visual spectator mode and the server-side calculations.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**
- **Extreme Performance:** By stripping away the rendering pipeline, a 100-generation simulation will complete in a fraction of a millisecond, allowing a single server to process thousands of matches per second.
- **Single Source of Truth:** Reusing `game_logic.c` guarantees that the server-side simulation behaves exactly identically to the client-side/spectator visualization. No logic needs to be duplicated or synchronized across different programming languages.
- **Easy Server Deployment:** A headless binary is trivial to deploy, orchestrate via a job queue, and run in lightweight Docker containers without needing virtual displays (Xvfb) or GPU access.

**Negative Consequences (Disadvantages):**
- **Refactoring Effort:** Requires restructuring `main.c` to gracefully handle different execution modes (GUI vs. Headless) based on compilation flags or startup arguments.
- **Dependency Management:** The build system (Makefile) must be updated to support a "headless" target that excludes Raylib dependencies.

#### **4. Alternatives Considered**

- **Rewriting Game Logic in the Backend Language (e.g., Python/Node.js):** 
  - *Reason for Rejection:* Translating the highly optimized C logic into Python or JavaScript would lead to significant performance degradation. More importantly, maintaining two separate codebases for the exact same ruleset introduces a high risk of inconsistencies and bugs over time.
- **Running the Web (WASM) Version via Headless Browser (Puppeteer/Selenium):**
  - *Reason for Rejection:* Extremely resource-intensive. Spinning up a headless browser instance for every match adds unacceptable overhead (RAM and CPU) and latency, defeating the purpose of a fast matchmaking backend.
- **Using Virtual Framebuffers (Xvfb) to run the existing GUI app on a server:**
  - *Reason for Rejection:* While it allows the app to run without a physical monitor, the application would still perform the computationally expensive rendering calculations, which is highly inefficient for background processing.