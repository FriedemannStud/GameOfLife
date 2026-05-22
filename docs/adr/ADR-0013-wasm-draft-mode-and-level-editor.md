### **ADR-0013: WASM Draft Mode and Level Editor**

**Status:** proposed

**Date:** 2026-05-22

#### **1. Context and Problem Statement**

With the successful implementation of the competitive backend (FastAPI/MongoDB) and the automated matchmaking worker, the "Biotope" ecosystem has transitioned from a local simulation to a global multiplayer platform. However, the current user interface (Raylib-based C GUI) is primarily a viewer and lacks an interactive mechanism for users to design, validate, and submit their own patterns directly from the browser.

To foster a competitive community, we need a "Level Editor" or "Draft Mode" that allows players to:
- Interactively place cells on an 8x8 grid.
- Stay within the "Fair Play" biomass limit (max 24 cells).
- Enter a nickname and a unique player ID.
- Submit the resulting configuration to the `POST /api/v1/submit_config` endpoint via a WebAssembly (WASM) interface.

#### **2. Decision**

We will implement a dedicated **Draft Mode** directly within the `gui.c` module. This mode will be activated via a UI toggle and will handle the entire submission lifecycle within the WASM environment.

Key technical components of this decision:
- **Interaction Model:** Left-click to place/remove cells within a visually highlighted 8x8 bounding box.
- **Rule Enforcement:** A real-time counter will track the cell count and block further placement once the 24-cell limit is reached.
- **WASM Text Input:** Implementation of a simple text buffer in C for nickname entry, or alternatively, an HTML overlay for better accessibility.
- **API Integration:** Use the Emscripten `emscripten_fetch` API or a JavaScript bridge (`emscripten_run_script`) to perform asynchronous HTTP POST requests to the backend.
- **State Management:** A new state machine within `gui.c` (e.g., `MODE_SIMULATION` vs. `MODE_DRAFT`) to manage UI transitions.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**
- **Unified Experience:** Users can design and test their patterns in the same environment where they watch matches.
- **Zero-Install Onboarding:** By targeting WASM, the editor remains frictionless and platform-independent.
- **Immediate Feedback:** Real-time biomass counting ensures users know exactly when they exceed limits before attempting to submit.
- **Code Reuse:** Leveraging existing Raylib grid rendering and camera logic reduces implementation overhead.

**Negative Consequences (Disadvantages):**
- **Complexity in C:** Adding UI elements like buttons and text boxes in pure C/Raylib is more labor-intensive than using HTML/CSS.
- **WASM Sandbox Restrictions:** Handling network requests and CORS from within WASM requires careful configuration of the Emscripten bridge.
- **Build Size:** Adding more UI logic and potentially a small UI library (like `raygui`) will slightly increase the WASM binary size.

#### **4. Alternatives Considered**

- **Pure HTML/JS Editor:** Developing a separate editor in JavaScript using HTML5 Canvas.
    - *Pros:* Easier text input and network handling.
    - *Cons:* Duplicates grid logic and breaks the visual consistency of the project.
- **Standalone Desktop Editor:** A separate C binary for designing patterns.
    - *Pros:* Easier to implement with standard C libraries.
    - *Cons:* Significantly higher barrier to entry for casual web users; inconsistent with the "Biotope-on-WASM" goal.
- **Manual JSON Submission:** Users write JSON files manually and submit them via `curl` or a web form.
    - *Pros:* No UI work required.
    - *Cons:* Not user-friendly and highly prone to validation errors.
