# Architecture Diagrams (Uni-Messe)

> **Historical snapshot — as of 27 May 2026.**
> The components marked as "Planned" (blue) — `network_io.c`, `KioskController`, `SimulationContext ×4`, `RenderContext ×4`, API endpoints — are now fully implemented. Current diagram: [260530_ARCHITECTURE_DIAGRAM.md](260530_ARCHITECTURE_DIAGRAM.md).

This document visualizes the system architecture for the "Game of Life - Biotope" project. It covers both the local Singleplayer experience and the distributed Multiplayer Tournament mode.

## 1. Singleplayer Architecture

This diagram visualizes the internal architecture of the standalone C-Application, detailing how user input drives the state machine, simulation, and local persistence.

```mermaid
flowchart TD
    %% Styling
    classDef existing fill:#d4edda,stroke:#28a745,stroke-width:2px,color:#155724;
    classDef io fill:#e2e3e5,stroke:#383d41,stroke-width:2px,color:#383d41;
    classDef user fill:#fff3cd,stroke:#856404,stroke-width:2px,color:#856404;

    %% User Interaction
    User((User)):::user

    %% C-Application Core
    subgraph "C-Application Core"
        Main[main.c: Event Loop]:::existing
        
        subgraph "Presentation Layer (Raylib)"
            Renderer[renderer.c: GUI & Shader]:::existing
            InputHandler[Input Processing]:::existing
        end
        
        subgraph "State Management"
            StateManager[app_state_manager.c]:::existing
            AppState{App State}:::existing
        end
        
        subgraph "Simulation Engine"
            Logic[game_logic.c]:::existing
            Structs[(World Struct)]:::existing
        end
    end

    %% Data Layer
    FileIO[file_io.c: JSON Persistence]:::io
    Storage[(Local File System\nJSON Protocols)]:::io

    %% User Flow & Input
    User -- "Clicks/Keys" --> InputHandler
    InputHandler -- "Process UI Events" --> StateManager
    InputHandler -- "Place Cells" --> Structs
    
    %% Main Loop Orchestration
    Main -- "1. Poll Input" --> InputHandler
    Main -- "2. Update Logic" --> StateManager
    Main -- "3. Render" --> Renderer

    %% State Transitions
    StateManager -- "Update Transition" --> AppState
    AppState -. "Config / Edit Red / Edit Blue" .-> Renderer
    AppState -. "Ignition / Running" .-> Logic
    AppState -. "Archive / Load" .-> FileIO

    %% Simulation Engine Data Flow
    Logic -- "Double Buffering" --> Structs
    Structs -- "Read Grid" --> Renderer
    Renderer -- "GPU Shader (UpdateTexture)" --> Renderer
    
    %% Persistence Data Flow
    StateManager -- "Auto-Save before Run" --> FileIO
    FileIO -- "Serialize/Deserialize (cJSON)" --> Storage
```

### Key:
*   **Green:** Core C-Application components (Simulation, UI, Logic).
*   **Grey:** Local File I/O and JSON storage components.
*   **Yellow:** User interaction.

---

## 2. Multiplayer Tournament Architecture (Uni-Messe Kiosk)

 This diagram visualizes the system architecture for the Multiplayer Tournament Mode, specifically highlighting the transition from the existing backend/simulation core to the new **Uni-Messe Kiosk Mode**.

```mermaid
flowchart TD
    %% Styling
    classDef existing fill:#d4edda,stroke:#28a745,stroke-width:2px,color:#155724;
    classDef planned fill:#cce5ff,stroke:#004085,stroke-width:2px,color:#004085;
    classDef external fill:#fff3cd,stroke:#856404,stroke-width:2px,color:#856404;

    %% Components
    Editor[HTML/JS Editor]:::external
    API[FastAPI Backend]:::existing
    DB[(MongoDB)]:::existing
    
    Worker[Python Tournament Worker]:::existing
    HyperC[C Hyper-Worker Engine]:::existing
    
    %% New Components (Planned via ADR-0017, 0018, 0019)
    Collection[DB: epoch_highlights]:::planned
    API_New[API: /leaderboard & /epoch/highlights]:::planned
    
    NetIO[network_io.c: libcurl + pthread]:::planned
    JSON[cJSON Parser]:::planned
    
    KioskCtrl[KioskController: 15s/30s Cycle]:::planned
    Failsafe[Global Inactivity Failsafe: 60s]:::planned
    
    SimContexts[SimulationContext sims4]:::planned
    RenContexts[RenderContext renders4]:::planned
    
    GPU[Raylib GPU: UpdateTexture]:::existing

    %% Data Flow
    Editor -- "POST /submit" --> API
    API --> DB
    Worker -- "Fetch Configs" --> DB
    Worker -- "Batch Run" --> HyperC
    HyperC -- "Win/Loss Stats" --> Worker
    Worker -- "Update Ranking" --> DB
    Worker -- "Save Metrics 1 & 4 (ADR-0017)" --> Collection
    
    Collection --> API_New
    DB --> API_New
    
    API_New -- "HTTP GET (Async)" --> NetIO
    NetIO -- "Raw String" --> JSON
    JSON -- "Populate Shared Structs" --> KioskCtrl
    
    Failsafe -- "Force Kiosk State (ADR-0019)" --> KioskCtrl
    KioskCtrl -- "User Click (Interrupt)" --> SimContexts
    
    KioskCtrl -- "Map Seeds" --> SimContexts
    SimContexts -- "Logic Data" --> RenContexts
    RenContexts -- "DrawTextureRec (ADR-0018)" --> GPU

    subgraph "Python Backend (FastAPI)"
        API
        API_New
        Worker
    end

    subgraph "C-Application: Kiosk Mode Frontend"
        Failsafe
        NetIO
        JSON
        KioskCtrl
        SimContexts
        RenContexts
    end
```  

### Key:
 *   **Green:** Existing components and data paths.
 *   **Blue:** Planned components specified in ADR-0017, ADR-0018, and ADR-0019.
 *   **Yellow:** External interfaces (Web Editor).