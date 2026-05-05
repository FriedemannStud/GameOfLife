### **ADR-0007: Laser Focus on the "Biotope" Competitive USP**

**Status:** Proposed

**Date:** 2026-05-05

#### **1. Context and Problem Statement**

This ADR implements the "ST" (Strength-Threat) defense strategy. The market for Conway's Game of Life simulations is hyper-saturated (T1). Additionally, the flexibility of our custom C engine poses a significant risk of "Feature Creep" (T3) – the temptation to add every imaginable sandbox feature (different cellular automata rulesets, generic physics, etc.).

However, our primary strength is the unique, mathematically sound **1v1 Competitive Loop (Red vs. Blue with majority-rule spawning)** (S2), born from our "Vibe Coding" genesis. If we dilute this core identity by trying to be a generic sandbox, we will lose our distinct positioning and fail to capture a dedicated audience.

#### **2. Decision**

We structurally lock the game's feature roadmap to the **"Competitive Biotope" paradigm**. We will reject any architectural changes or feature proposals that do not directly enhance the 1v1 strategic gameplay or the meta-game surrounding it.

Specific architectural constraints derived from this decision:
1.  **Rule Immutability:** The core engine will *not* be abstracted to support arbitrary rule strings (like B3/S23 variations). It will remain hardcoded and hyper-optimized for the specific Red vs. Blue Biotope ruleset.
2.  **Telemetry and Analytics:** We will expand `file_io.c` to generate granular post-match telemetry (e.g., "frontline shifts," "population volatility," "efficiency of specific patterns"). This data will be used to build a robust post-game analysis screen, essential for competitive players.
3.  **UI Dominance:** The UI architecture (`gui.c`) must prioritize competitive elements: clear scoreboards, round timers, territory control indicators, and distinct drafting phases.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**
*   **Distinct Brand Identity:** "The first true E-sport Game of Life." This makes marketing messaging sharp and targeted.
*   **Optimized Engineering:** By refusing generic rule abstractions, we can continue to aggressively optimize the specific neighbor-counting loop in `game_logic.c` using bitwise operations or SIMD instructions tailored only to our 3-state system (Dead, Red, Blue).
*   **Deep Metagame:** Forcing focus onto the competitive aspect encourages the community to discover, document, and counter specific "openings" and "formations" (similar to chess or Starcraft).

**Negative Consequences (Disadvantages):**
*   **Alienation of Purists:** Players looking for a standard, multi-rule cellular automata explorer will be disappointed.
*   **Balancing Liability:** The entire success of the game hinges on the Red vs. Blue ruleset being mathematically fair and strategically deep. If a dominant, unbeatable pattern is found, the competitive integrity collapses.

#### **4. Alternatives Considered**

*   **The "Everything Sandbox" Engine:** Abstracting the engine to support any cellular automata rule and treating the 1v1 mode as just one of many minigames. *Rejected:* Dilutes the product identity, wastes engineering time on features that don't drive our core USP, and thrusts us into direct competition with established giants like *The Powder Toy*.
*   **Real-Time Strategy (RTS) Controls:** Allowing players to place cells *while* the simulation is running. *Rejected:* Ruins the purity of the mathematical outcome. The appeal is the deterministic clash of two planned starting states, akin to algorithmic programming battles.