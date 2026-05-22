### **ADR-0011: REST API and Server-Side Validation**

**Status:** Proposed

**Date:** 2026-05-22

#### **1. Context and Problem Statement**

To transform the Game of Life into a competitive, asynchronous multiplayer ecosystem ("Biotope"), we need a centralized backend to handle pattern submissions, matchmaking, and tournament coordination. 

The primary challenge is ensuring "Fair Play." Clients (Web/WASM editors) are inherently untrusted; players could bypass client-side restrictions to submit patterns that exceed the allowed biomass (38%) or use dimensions larger than the standardized 8x8 grid. We need a robust mechanism to receive these patterns, validate them against strict criteria, and store them for the `biotope_headless` worker to process.

#### **2. Decision**

We will implement the backend using **Python and the FastAPI framework**. 

Key implementation details include:
- **Endpoint:** `POST /api/v1/submit_config` will be the primary entry point for players to submit their 8x8 patterns.
- **Validation Engine:** 
    - Use **Pydantic** for structural JSON validation (types, required fields).
    - Implement a custom **Business Logic Validator** to enforce Biotope-specific rules:
        - **Grid Dimensions:** All cells must be within a [0..7, 0..7] coordinate space.
        - **Biomass Limit:** Maximum of 38% living cells (max 24 cells in an 8x8 grid).
        - **Metadata Integrity:** Verify `player_id` and `nickname` presence.
- **EVA Principle:** The API will follow the Input-Process-Output principle, where input is the submitted JSON, processing is the validation and persistence, and output is a confirmation or a detailed error message.
- **Integration:** The backend will generate standardized JSON files compatible with the `biotope_headless` worker's input requirements.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**
- **High Developer Velocity:** FastAPI allows for rapid development of robust APIs with minimal boilerplate.
- **Automatic Documentation:** Built-in Swagger UI and ReDoc for frontend developers to test endpoints.
- **Type Safety:** Pydantic ensures that the data reaching the business logic is already structurally sound.
- **Security:** Strict server-side validation prevents "cheating" and ensures tournament integrity.
- **Scalability:** Python/FastAPI fits well into the existing Docker-based infrastructure.

**Negative Consequences (Disadvantages):**
- **Runtime Overhead:** Crossing from a Python web layer to a C simulation layer adds minor latency (mitigated by asynchronous job processing).
- **Environment Complexity:** Adds Python dependencies to the project, requiring careful Docker management.

#### **4. Alternatives Considered**

- **Node.js (Express):** A popular choice for REST APIs. However, Python was selected due to its superior ecosystem for potential future data analysis, leaderboard algorithms, and AI integration.
- **Pure C (libmicrohttpd):** While offering the highest performance, it would significantly increase development time for standard REST features (auth, validation, JSON parsing) and would be harder to maintain.
- **Go (Gin/Echo):** Offers great performance and type safety. FastAPI was preferred for its faster prototyping speed and superior automatic documentation features in a "vibe-coding" context.

// AI-attributed: Decision to use FastAPI for the multiplayer backend to balance performance with developer efficiency.
