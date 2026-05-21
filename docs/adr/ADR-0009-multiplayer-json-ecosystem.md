### **ADR-0009: Multiplayer JSON Payload Specification**

**Status:** Proposed

**Date:** 2026-05-21

#### **1. Context and Problem Statement**

The project is evolving from a local 1v1 simulation to a massive-parallel multiplayer ecosystem ("Biotope"). Visitors at the university fair should be able to create start configurations on their smartphones via a Web-Editor (WASM) and submit them to a central server. 

The existing `.bio` (Version 2) format has several limitations for this use case:
- It uses absolute coordinates for the entire grid.
- It encodes specific team colors (Red/Blue) into the file.
- It is a custom text-based format that is harder to handle in modern Web-APIs compared to standard formats like JSON.
- It lacks the necessary metadata for matchmaking (player IDs, league tiers, forking history).

#### **2. Decision**

We will implement a new **JSON-based transport format** for all client-to-server communication.

**Key Specifications:**
1.  **Relative Coordinates:** Patterns are stored as `[x, y]` offsets relative to the top-left `[0,0]` of the player's bounding box.
2.  **Color Neutrality:** The payload contains only the pattern of living cells. Team assignment (Red vs. Blue) is performed dynamically by the simulation server.
3.  **League Constraints:** The payload must specify the intended league, which dictates the maximum bounding box and biomass (cell count) limit:
    - **Einsteiger:** 8x8 Box, max 12 cells.
    - **Rookie:** 50x50 Box, max 475 cells.
    - **Champions:** 100x100 Box, max 1900 cells.
4.  **Metadata Integration:** Every submission includes metadata for the Elo-system, player identification, and evolutionary tracking (forking).

**Draft Structure:**
```json
{
  "metadata": {
    "player_id": "string",
    "nickname": "string",
    "league": "einsteiger|rookie|champions",
    "parent_config_id": "string|null",
    "timestamp": "ISO8601_string"
  },
  "config": {
    "bounding_box_x": number,
    "bounding_box_y": number,
    "cell_count": number,
    "cells": [[x1, y1], [x2, y2], ...]
  }
}
```

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**
- **Interoperability:** JSON is natively supported by almost all programming languages and web frameworks.
- **Scalability:** The server can easily validate incoming patterns against league-specific rules before starting a simulation.
- **Flexibility:** Patterns can be "stamped" into any part of the arena grid by the simulation engine.
- **Evolutionary Tracking:** The `parent_config_id` enables the creation of "ancestry trees" for successful patterns.

**Negative Consequences (Disadvantages):**
- **Parsing Overhead:** JSON parsing in C (for the headless worker) is slightly more complex than `sscanf` for the legacy text format (requires a library like `jsmn` or `cJSON`).
- **File Size:** JSON is more verbose than the compact sparse text format of `.bio` files.

#### **4. Alternatives Considered**

- **Extending `.bio` v2:** Rejected, as adding metadata and relative coordinates to the custom text format would lead to a "Frankenstein" format that remains difficult to use in web environments.
- **Binary Formats (Protocol Buffers):** Considered for performance, but rejected for now to keep the project accessible for 1st-semester students and easy to debug in the browser console.

// KI-Agent unterstützt: Standardized JSON payload for multiplayer interoperability.
