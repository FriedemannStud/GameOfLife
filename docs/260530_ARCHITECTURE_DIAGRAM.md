# Architecture Diagrams – Uni‑Messe (Updated 30 May 2026)

> **Purpose** – This document gives a concise, production‑ready visual overview of the full system – both the standalone single‑player client and the distributed multiplayer‑tournament (Uni‑Messe) solution. The diagrams are written in **Mermaid** and can be rendered directly in VS Code, GitHub, or any Markdown viewer that supports Mermaid.

---

## 1️⃣ Single‑player Architecture (C‑Application)

```mermaid
flowchart TD
%% ------------------------------------------------
%% Global style definitions
%% ------------------------------------------------
classDef core      fill:#e8f5e9,stroke:#43a047,stroke-width:2px,color:#1b5e20;
classDef ui        fill:#fff3cd,stroke:#856404,stroke-width:2px,color:#856404;
classDef data      fill:#e2e3e5,stroke:#383d41,stroke-width:2px,color:#383d41;
classDef external  fill:#f0f0f0,stroke:#6c757d,stroke-width:2px,color:#6c757d;

%% ------------------------------------------------
%% Entities
%% ------------------------------------------------
User((User)):::external

subgraph "C‑Application Core"[C‑Application Core]
    Main[main.c – Event Loop]:::core
    subgraph "Presentation Layer (Raylib)"
        Renderer[renderer.c – GUI & Shaders]:::core
        Input[Input Handler]:::core
    end
    subgraph "State Management"
        StateMgr[app_state_manager.c]:::core
        AppState{App State}:::core
    end
    subgraph "Simulation Engine"
        Logic[game_logic.c]:::core
        World[World Struct]:::core
    end
end

subgraph "Data Layer"
    FileIO[file_io.c – JSON Persistence]:::data
    Storage[(Local File System / JSON Files)]:::data
end

%% ------------------------------------------------
%% Inter‑component relationships
%% ------------------------------------------------
User -- "Clicks / Keys" --> Input
Input -- "Process UI Events" --> StateMgr
Input -- "Place Cells" --> World

Main -- "1. Poll Input" --> Input
Main -- "2. Update Logic" --> StateMgr
Main -- "3. Render" --> Renderer

StateMgr -- "Update Transition" --> AppState
AppState -. "Config / Edit Red / Edit Blue" .- Renderer
AppState -. "Ignition / Running" .- Logic
AppState -. "Archive / Load" .- FileIO

Logic -- "Double Buffering" --> World
World -- "Read Grid" --> Renderer
Renderer -- "GPU Shader (UpdateTexture)" --> Renderer

StateMgr -- "Auto‑Save before Run" --> FileIO
FileIO -- "Serialize / Deserialize (cJSON)" --> Storage
```

**Legend**
- **Core** – Core C‑application components (logic, UI, state management).
- **Data** – Local persistence (JSON + file system).
- **External** – Human user.

---

## 2️⃣ Multiplayer Tournament Architecture (Uni‑Messe Kiosk Mode)

```mermaid
flowchart TD
%% ------------------------------------------------
%% Global style definitions (reuse core & data)
%% ------------------------------------------------
classDef core      fill:#e8f5e9,stroke:#43a047,stroke-width:2px,color:#1b5e20;
classDef planned   fill:#cce5ff,stroke:#004085,stroke-width:2px,color:#004085;
classDef external  fill:#fff3cd,stroke:#856404,stroke-width:2px,color:#856404;

%% ------------------------------------------------
%% External client (Web Editor)
%% ------------------------------------------------
Editor["HTML/JS Editor"]:::external

%% ------------------------------------------------
%% Backend components (existing)
%% ------------------------------------------------
API["FastAPI Backend"]:::core
DB[(MongoDB)]:::core
Worker["Python Tournament Worker"]:::core
Hyper["C Hyper‑Worker Engine"]:::core

%% ------------------------------------------------
%% New / planned components (Kiosk Mode)
%% ------------------------------------------------
Highlights["DB: epoch_highlights"]:::planned
API_New["API: /leaderboard & /epoch/highlights"]:::planned
NetIO["network_io.c – libcurl + pthread"]:::planned
JSON["cJSON Parser"]:::planned
KioskCtrl["KioskController – 15 s / 30 s cycle"]:::planned
Failsafe["Global Inactivity Failsafe (60 s)"]:::planned
SimCtx["SimulationContext ×4"]:::planned
RenCtx["RenderContext ×4"]:::planned
GPU["Raylib GPU – UpdateTexture"]:::core

%% ------------------------------------------------
%% Data flows
%% ------------------------------------------------
Editor -- "POST /submit" --> API
API --> DB
Worker -- "Fetch Configs" --> DB
Worker -- "Batch Run" --> Hyper
Hyper -- "Win/Loss Stats" --> Worker
Worker -- "Update Ranking" --> DB
Worker -- "Save Metrics 1 & 4" --> Highlights

Highlights --> API_New
DB --> API_New
API_New -- "HTTP GET (Async)" --> NetIO
NetIO -- "Raw String" --> JSON
JSON -- "Populate Shared Structs" --> KioskCtrl

Failsafe -- "Force Kiosk State (ADR‑0019)" --> KioskCtrl
KioskCtrl -- "User Click (Interrupt)" --> SimCtx
KioskCtrl -- "Map Seeds" --> SimCtx
SimCtx -- "Logic Data" --> RenCtx
RenCtx -- "DrawTextureRec (ADR‑0018)" --> GPU
```

**Legend**
- **Core** – Already‑implemented backend & C‑application components.
- **Planned** – New functionality introduced for the Uni‑Messe kiosk (all already in code as of 27 May 2026).
- **External** – Web‑based editor used by participants.

---

*Document generated on **2026‑05‑30** – updated from the initial draft (27 May 2026) with refined styling, explicit legends, and clearer flow descriptions.*
