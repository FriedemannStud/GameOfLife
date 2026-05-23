### **ADR-0014: Global Tournament Epoch Architecture**

**Status:** Proposed

**Date:** 2026-05-23

#### **1. Context and Problem Statement**

The "Biotope" project is preparing for a university exhibition where participants will successively submit 8x8 Game of Life patterns to compete in a live leaderboard. The number of participants is expected to grow dynamically throughout the event, with a projected maximum of several hundred submissions. The previous architecture (ADR-0012) relied on an Elo-based matchmaking system orchestrated by a Python background worker that executed individual 1v1 matches via separate C-process calls.

During the research phase, two critical flaws were identified:
1.  **Intransitivity (The Rock-Paper-Scissors Problem):** Game of Life patterns exhibit non-linear "skill" relationships. A pattern may defeat the champion but lose to a beginner. Elo ratings fail in such "Schere-Stein-Papier" ecosystems, as they assume a transitive skill hierarchy (if A > B and B > C, then A > C).
2.  **Process Overhead & Scaling:** As the pool of participants grows, orchestrating a "Round-Robin" (everybody vs. everybody) tournament via separate processes becomes exponentially more expensive ($O(N^2)$). For a few hundred players, this results in over 100,000 matches. Starting these as individual Linux processes via Python is computationally expensive and introduces massive OS overhead, making frequent leaderboard updates impossible as the event progresses.

To ensure absolute fairness throughout the successive onboarding of players and provide near-instant feedback, we need a high-performance, deterministic tournament architecture that scales efficiently with a growing participant pool.

#### **2. Decision**

We will implement a **Global Tournament Epoch Architecture** using a batch-processing C-Hyper-Worker.

1.  **Epoch-Based Execution:** Instead of continuous 1v1 matching, the system will run a full tournament "Epoch" periodically (e.g., every 60 seconds) including all currently active submissions.
2.  **C-Hyper-Worker (Batch Processing):** The Python backend will export the entire current set of active submissions into a single batch file. A revised C-executable (`biotope_hyper_worker`) will load all patterns into RAM and execute the complete $O(N^2)$ Round-Robin loop internally.
3.  **Home and Away Logic:** To ensure absolute symmetry fairness, every pair will play twice, with roles (Left/Right side of the grid) swapped between matches.
4.  **Win-Rate Metric:** Elo is replaced by a "Win Percentage" or "Points-per-Match" metric. This provides a transparent, fair leaderboard where every submission is tested against the entire current field.
5.  **Parallelization:** The internal C-loop will utilize OpenMP to distribute the matches across all available CPU cores, ensuring that even as the participant count reaches its peak, the total execution time remains within a few seconds.
6.  **Early Termination:** The simulation logic will detect static states (Still-Lifes) and abort early, potentially saving up to 90% of processing time per match and logging the 'stable_at_generation' metric.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**
*   **Absolute Fairness:** Every player is tested against every other player, eliminating "luck of the draw" in matchmaking and solving the intransitivity problem regardless of when a player joins the tournament.
*   **Extreme Performance:** Moving the loop from Python/Subprocesses to an internal C-loop reduces overhead by several orders of magnitude, allowing for sub-minute updates even at the projected maximum load.
*   **Deterministic Integrity:** The entire tournament state is recalculated from scratch in each epoch, ensuring that new submissions are immediately and correctly integrated into the global ranking.
*   **User Experience:** Players receive a "Final Ranking" update every minute, creating a high-energy "Live Leaderboard" atmosphere that stays responsive as more students participate.

**Negative Consequences (Disadvantages):**
*   **Exponential Load Growth:** Since the complexity is $O(N^2)$, the computation time will increase quadratically as more players join (though for several hundred players, this remains well within the limits of a single C-process).
*   **Backend Refactoring:** Requires significant changes to `worker.py` and the creation of a more complex `main_headless.c` entry point.
*   **Latency Jitter:** Players must wait until the end of the current periodic window to see their ranking.

#### **4. Alternatives Considered**

*   **Ligue System:** Dividing players into smaller groups to run Round-Robin. *Rejected:* Leads to "Local Meta Traps" where a champion-tier pattern might get stuck in a lower league because it loses to a specific beginner counter-pattern.
*   **Benchmark Gauntlet:** Testing new patterns against a fixed set of "Elite" patterns. *Rejected:* Too static for a live exhibition where the "Meta" should evolve based on what students are currently submitting.
*   **Scaling Elo with more Python Workers:** *Rejected:* Does not solve the fundamental mathematical flaw of Elo in intransitive games.
