# Multiplayer Tournament Architecture (Uni-Messe)

 This diagram visualizes the system architecture for the Multiplayer Tournament Mode, specifically highlighting the transition from the existing backend/simulation core to the new **Uni-Messe Kiosk Mode**.

```mermaid
flowchart
    %% Styling
    classDef existing fill:#d4edda,stroke:#28a745,stroke-width:2px,color:#155724;
    classDef to_develop fill:#f8d7da,stroke:#dc3545,stroke-width:2px,color:#721c24,stroke-dasharray: 5 5;
    classDef external fill:#fff3cd,stroke:#856404,stroke-width:2px,color:#856404;

    %% Components
    Editor[HTML/JS Editor]:::external
    API[FastAPI Backend]:::existing
    DB[(MongoDB)]:::existing
    
    Worker[Python Tournament Worker]:::existing
    HyperC[C Hyper-Worker Engine]:::existing
    
    CApp[C-Application Core]:::existing
    Renderer[Renderer.c single-view]:::existing
    
    %% New Components (To be developed for Uni-Messe)
    API_New[API: /leaderboard & /highlights]:::to_develop
    C_Net[C-Networking Layer]:::to_develop
    KioskState[STATE_KIOSK_MODE]:::to_develop
    MultiRenderer[Multi-World Renderer]:::to_develop
    Protocols[Match Seed Persistence]:::to_develop

    %% Data Flow
    Editor -- "POST /submit" --> API
    API --> DB
    Worker -- "Fetch Configs" --> DB
    Worker -- "Batch Run" --> HyperC
    HyperC -- "Win/Loss Stats" --> Worker
    Worker -- "Update Ranking" --> DB
    
    %% The Gap (Uni-Messe Requirements)
    HyperC -.-> Protocols
    Protocols -.-> API_New
    DB -.-> API_New
    
    API_New -- "JSON" --> C_Net
    C_Net --> KioskState
    KioskState --> MultiRenderer
    MultiRenderer --> Renderer

    subgraph "Uni-Messe Kiosk Mode (NEW)"
        KioskState
        MultiRenderer
        C_Net
    end

    subgraph "Backend Extensions"
        API_New
        Protocols
    end
```  


### Key:
 *   **Green (Solid):** Existing components and data paths.
 *   **Red (Dashed):** Components and paths to be implemented for the Uni-Messe Kiosk Mode.
 *   **Yellow:** External interfaces (Web Editor).