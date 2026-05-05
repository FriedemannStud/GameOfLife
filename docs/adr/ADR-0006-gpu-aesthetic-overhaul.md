### **ADR-0006: GPU Aesthetic Overhaul via Shader Pipeline**

**Status:** Proposed

**Date:** 2026-05-05

#### **1. Context and Problem Statement**

This ADR addresses the "WO" (Weakness-Opportunity) strategic vector. Currently, the visual presentation of "Biotope" is a static, rigid, and "dry" grid of pixels (W1). However, the indie market heavily rewards games with a distinct "Procedural Aesthetic" and high "Juiciness" (O3) – making them highly shareable and satisfying to interact with.

Our underlying simulation is a mathematically pure C engine, but treating it purely as a visual array of squares prevents it from being perceived as a modern game. We need to transform the aesthetic from a "sterile laboratory tool" into a "living, breathing petri dish" to capitalize on emergent gameplay shareability, without compromising the raw performance of the C simulation core.

#### **2. Decision**

We will deprecate the pure CPU-side pixel buffer generation (`UpdateTexture` in `gui.c`) in favor of a **Custom GPU Shader Pipeline (GLSL)** managed by Raylib.

The architecture will evolve as follows:
1.  The CPU (Game Logic) will continue to calculate the raw integer grid (`DEAD`, `TEAM_RED`, `TEAM_BLUE`).
2.  This integer state will be passed to the GPU as a raw data texture or buffer.
3.  A custom Fragment Shader will be responsible for the final visual output. This shader will interpolate the rigid grid into organic shapes using techniques such as:
    *   **Cellular Automata Smoothing (e.g., marching squares or metaballs visually applied over the grid).**
    *   **Bloom and Glow effects** to signify highly active or dense population clusters.
    *   **Temporal fading (Trails/Ghosting)** to show the history of a moving pattern (like a Glider), giving the illusion of motion and fluid dynamics.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**
*   **Massive Aesthetic Upgrade:** The game immediately elevates its perceived production value, looking "expensive" and modern, crucial for the indie market.
*   **Decoupled Performance:** Offloading visual processing to the GPU frees up CPU cycles exclusively for the OpenMP simulation loop. The CPU no longer iterates over arrays just to set colors.
*   **Viral Shareability:** Fluid, glowing, emergent patterns are inherently more captivating in video format (social media marketing) than rigid pixels.

**Negative Consequences (Disadvantages):**
*   **Technical Complexity:** Introduces GLSL into the stack, requiring specialized graphics programming knowledge that diverges from standard C logic.
*   **Hardware Floor:** Raises the minimum system requirements. While Raylib is efficient, complex shaders require a dedicated or capable integrated GPU, potentially excluding extremely low-end hardware.
*   **WASM/WebGL Constraints:** Shaders must be written with WebGL compatibility in mind (GLSL 100/300 ES) to ensure parity with the WebAssembly port (ADR-0005).

#### **4. Alternatives Considered**

*   **CPU-Side Tweening and Interpolation:** Calculating visual transitions between frames using C logic. *Rejected:* Too computationally expensive. Traversing massive grids multiple times per frame to calculate fades would cripple the simulation speed.
*   **High-Resolution Sprites:** Replacing simple pixels with detailed, hand-drawn PNG sprites for cells. *Rejected:* Clashes with the minimalist, macro-scale "biotope" aesthetic we are aiming for, and scaling thousands of sprites introduces heavy draw-call overhead.