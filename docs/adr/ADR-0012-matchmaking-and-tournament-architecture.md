# ADR-0012: Matchmaking Service and Tournament Architecture

**Status:** Proposed  
**Date:** 2026-05-22  
**Author:** Gemini CLI

## 1. Context and Problem Statement

The "Biotope" ecosystem has transitioned from a local simulation to an asynchronous multiplayer platform. We currently have a REST API for pattern submission (Issue #4) and a headless simulation worker (Issue #3). However, the "competitive" aspect is currently manual: submissions are simply saved as files.

To achieve the "Massive-Parallel Multiplayer" goal, we need an automated system that:
1.  **Pairs Players:** Automatically finds suitable opponents for new or existing submissions.
2.  **Orchestrates Matches:** Triggers the `biotope_headless` worker to simulate matches.
3.  **Manages Ranking:** Calculates and updates Elo ratings based on match outcomes.
4.  **Scales:** Handles hundreds or thousands of submissions without manual intervention.

The central question is how to manage state (players, submissions, matches) and how to pair players fairly to ensure a meaningful leaderboard.

## 2. Decision

We will implement a **Cloud-Native Matchmaking & Persistence Architecture** centered around MongoDB Atlas.

### 2.1. Persistence: MongoDB Atlas
We will utilize the provided MongoDB Atlas instance as the central data store. 
- **Reasoning:** MongoDB's document-based nature is a perfect fit for our JSON-centric protocol. Storing complex pattern arrays, player metadata, and match results as documents avoids the overhead of relational mapping.
- **Collections:**
    - `players`: Store `player_id`, `nickname`, current `elo_rating`, and match history.
    - `submissions`: Store the 8x8 JSON pattern, `player_id`, timestamp, and "active" status.
    - `matches`: Store the history of simulations, including participants, final populations, winner, and Elo delta.

### 2.2. Matchmaking Algorithm: "Proximity Swiss"
Instead of a simple random pairing, we will use a **Weighted Proximity Algorithm**:
- **Selection:** The service identifies "Active" submissions (those recently submitted or with few matches).
- **Pairing:** Players are paired with opponents whose Elo rating is within a ±150 point range.
- **Priority:** New submissions (0 matches) are prioritized for "Placement Matches" against established "Benchmark" patterns or random opponents to establish a baseline.
- **Frequency:** Matches are triggered in batches by a background worker service.

### 2.3. Job Queue: Database-Driven Worker
Instead of introducing a heavy message broker (like RabbitMQ or Redis/Celery), we will implement a **Lightweight DB-Polling Worker**:
- A dedicated Python service (`matchmaker`) will poll MongoDB for "Pending" match jobs or "New" submissions.
- It will use atomic operations (`find_one_and_update`) to claim a match task, preventing race conditions between multiple worker instances.
- It will execute the `biotope_headless` binary via a subprocess, parse the result JSON, and update the DB.

### 2.4. Ranking: Elo Rating System
We will use the standard Elo formula ($R'_a = R_a + K \cdot (S_a - E_a)$) to update ratings.
- **K-Factor:** We will use a dynamic K-factor (higher for new players to facilitate fast convergence).

## 3. Consequences

### Positive (Advantages)
- **Extreme Flexibility:** MongoDB handles the JSON pattern format natively without schema migrations.
- **Zero-Infrastructure Overhead:** MongoDB Atlas is a managed service, reducing local server maintenance.
- **Scalability:** The architecture supports horizontal scaling of matchmaker workers across multiple containers.
- **Unified Logic:** The Python backend can share models (`models.py`) between the API and the Matchmaker.

### Negative (Disadvantages)
- **External Dependency:** Reliance on a cloud provider (MongoDB Atlas) introduces latency and requires internet connectivity during development/simulation.
- **Polling Latency:** Database polling is slightly less efficient than a real-time message queue (milliseconds of delay).
- **Consistency:** While MongoDB supports transactions, complex multi-document updates (Player A, Player B, Match result) require careful implementation.

## 4. Alternatives Considered

### 4.1. PostgreSQL (Relational)
- **Pros:** Stronger consistency, better for financial/ranking systems.
- **Cons:** Rigid schema makes storing varying JSON pattern versions more difficult. Requires more setup for the JSON arrays.

### 4.2. Redis + Celery
- **Pros:** True real-time task queue, extremely high performance.
- **Cons:** Adds another moving part to the architecture. For the current scale of Biotope, the complexity might be overkill.

### 4.3. Pure File-System Matchmaking
- **Pros:** No external database needed.
- **Cons:** Impossible to implement fair Elo-based pairing or global leaderboards efficiently. Does not scale beyond a single machine.

## 5. Implementation Strategy (The "Vibe" Path)

1.  **Phase 1:** Update `backend/requirements.txt` to include `motor` (asynchronous MongoDB driver).
2.  **Phase 2:** Implement the `Matchmaker` service as a separate process in the `backend` container.
3.  **Phase 3:** Create a `RankingService` to handle Elo calculations.
4.  **Phase 4:** Integrate the MongoDB URI into the environment configuration.

---

### 🎓 For the 1st-Semester Student
Stell dir das Matchmaking wie einen digitalen Schiedsrichter vor. Anstatt dass du manuell Dateien kopierst, schaut der Schiedsrichter (unser Hintergrund-Prozess) in eine große, schlaue Liste (die MongoDB), wer gerade spielen möchte. Er sucht zwei Spieler aus, die ungefähr gleich gut sind (basierend auf ihrem Elo-Wert), lässt sie gegeneinander antreten und schreibt das Ergebnis wieder in die Liste. So entsteht ganz automatisch eine Rangliste, genau wie bei League of Legends oder Schach.
