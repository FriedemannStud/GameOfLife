### **ADR-0008: "Epic Scale" Tournament Architecture**

**Status:** Proposed

**Date:** 2026-05-05

#### **1. Context and Problem Statement**

This ADR formalizes the "SO" (Strength-Opportunity) aggressive growth strategy. We possess a highly optimized, massively parallel C-core via OpenMP (S1) and a clean State-Machine architecture (S3). Concurrently, there is a distinct market opportunity in the "Emergent Gameplay/E-Sport" space (O2) and a strong trend toward visually overwhelming "Minimalist Systems" (O1).

While the WASM port (ADR-0005) guarantees accessibility, it has performance ceilings. To truly dominate the indie space and create viral, spectacular content, we must also lean into the extreme upper limits of our engine. We need an architecture that supports "Epic Scale" battles (e.g., 5000x5000 grids) and provides the tools necessary to broadcast these battles to an audience.

#### **2. Decision**

We will architecturally bifurcate the execution target to fully exploit native hardware capabilities for a dedicated **"Tournament Mode"**.

1.  **Native "Epic Scale" Target:** While the primary player funnel is WASM, the flagship executable (via native Makefile build) will be aggressively optimized for massive grids. We will implement "Sparse Matrix" or "Active Chunk" heuristics in the OpenMP loop to avoid processing entirely dead sectors of the grid, allowing simulation of astronomical biotope sizes.
2.  **Observer UI State:** We will introduce a new application state (`STATE_OBSERVER`) specifically designed for spectating AI vs. AI or pre-recorded high-tier matches.
    *   This state will feature free-cam panning, extreme zoom capabilities, and real-time analytical overlays (graphs of population dominance over time).
    *   It is designed to be the definitive tool for Twitch streamers and YouTube content creators.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**
*   **Spectacle Generation:** Creates the visually stunning, overwhelming emergent scenarios that go viral. A 5000x5000 battle looks like a galactic war, elevating the game far beyond a simple puzzle.
*   **E-Sport Broadcast Readiness:** The dedicated Observer UI provides exactly the tools content creators need to "cast" algorithmic battles, providing free, high-leverage marketing.
*   **Technical Supremacy:** Establishes Biotope as the definitively most powerful engine in its niche.

**Negative Consequences (Disadvantages):**
*   **Bifurcated Codebase Complexity:** We must maintain code that works within the WASM sandbox (smaller grids) while also supporting the extreme optimizations (Sparse Matrix) required for the native Epic Scale, increasing testing overhead.
*   **Memory Management Risks:** Handling 5000x5000 grids (25 million cells) with Double Buffering requires robust memory management to avoid catastrophic heap fragmentation over long tournament runs.

#### **4. Alternatives Considered**

*   **Cloud-Computed Grids:** Offloading the simulation of massive grids to a central server and streaming the visual result to clients. *Rejected:* Server infrastructure costs would immediately bankrupt an indie project, and it introduces unacceptable network latency for the UI.
*   **Artificial Grid Limits:** Capping the grid size globally to ensure absolute parity between the WASM version and the Native version. *Rejected:* Fails to leverage our greatest technical strength (native C performance) and leaves the "spectacle" opportunity unexploited.