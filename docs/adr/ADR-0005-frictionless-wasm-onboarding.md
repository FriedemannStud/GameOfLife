### **ADR-0005: Frictionless WASM Distribution and Onboarding Architecture**

**Status:** Proposed

**Date:** 2026-05-05

#### **1. Context and Problem Statement**

To achieve the strategic goal of evolving "Biotope GameOfLife" into a highly sought-after indie game, we must address the "WT" (Weakness-Threat) combination: The current technical barriers (Docker/X11 setup, W2) and the inherent complexity of the simulation (W3) severely clash with the modern indie market's expectation for instant "click-and-play" gratification (T2) and the oversaturation of the simulation genre (T1).

Currently, the friction to experience the core gameplay loop is too high. A player must configure a local X-server and understand Docker simply to see the main menu. Furthermore, dropping players directly into an unguided sandbox leads to immediate churn. If the game cannot be played instantly and understood intuitively within the first 60 seconds, it will fail to gain traction regardless of its technical brilliance.

#### **2. Decision**

We will pivot the primary top-of-funnel distribution strategy to a **WebAssembly (WASM) target**, utilizing Emscripten to compile our existing C/Raylib codebase for execution directly in standard web browsers.

Concurrently, we mandate an architectural shift in the `STATE_CONFIG` state machine to support a "Guided Puzzle Campaign" layer. This layer will serve as a mandatory onboarding sequence (a "Trojan Horse" tutorial), gradually introducing the core mechanic (the Red vs. Blue majority spawning rule) through curated, bite-sized scenarios before unlocking the epic-scale competitive sandbox.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**
*   **Zero-Friction Access:** Players can instantly play the game via a URL on platforms like itch.io or a dedicated landing page, maximizing conversion rates from social media marketing (e.g., Reddit, TikTok).
*   **Codebase Retention:** Because Raylib seamlessly supports Emscripten, we maintain our performant C core without rewriting the engine in JavaScript or a heavyweight engine like Unity.
*   **Retention via Progression:** The puzzle-driven onboarding mitigates "complexity shock," transitioning the player from an observer of a mathematical tool to an active participant in a strategic game.

**Negative Consequences (Disadvantages):**
*   **Performance Ceiling:** WASM execution incurs a slight CPU penalty compared to native execution, and utilizing OpenMP threading in the browser requires SharedArrayBuffer support, which introduces complex CORS/security header requirements on the hosting server.
*   **I/O Sandbox Restrictions:** The browser sandbox limits direct local file I/O. Our current `file_io.c` relying on native file system access for saving `.bio` protocols will require abstraction (e.g., using Raylib's IndexedDB integration or Base64 string exports/imports) for the web build.

#### **4. Alternatives Considered**

*   **Porting to Unity/Godot:** Rewriting the game in an established engine to gain easy web exports and UI tools. *Rejected:* Destroys the "hand-crafted C performance" USP, discards our highly optimized OpenMP core, and delays development by months.
*   **Native-Only Standalone Executable:** Distributing only pre-compiled Windows/Linux binaries. *Rejected:* While better than Docker, it still requires a download/install step, which creates unacceptable drop-off rates for a novel, unproven indie IP.
*   **Pure JavaScript/Canvas Rewrite:** *Rejected:* Would completely eliminate our performance advantage for large-scale grids.