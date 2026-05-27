# Multiplayer Tournament Architecture (Uni-Messe)

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