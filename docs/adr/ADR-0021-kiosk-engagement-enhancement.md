### **ADR-0021: Kiosk Mode Engagement Enhancement**

**Status:** Accepted

**Date:** 2026-05-31

#### **1. Context and Problem Statement**

The Kiosk Mode (`STATE_KIOSK_MODE`) is the primary eye-catcher at the university fair. It cycles between a leaderboard and a 2×2 Multicam grid showing four simultaneous live matches. User feedback from a first real-world test session revealed a critical engagement gap:

> *"And where is my simulation?"*

A participant who had submitted a pattern via the web editor could not identify their own match in the Multicam view. The root cause is threefold:

1. **No personalization:** The `MatchHighlight` struct already carries `participant_red` and `participant_blue` names from the API, but they are never forwarded to the `SimulationContext` instances and never rendered. The Multicam view shows four anonymous red-vs-blue battles.
2. **No live conflict signal:** There is no per-quadrant score display. A passerby has no immediate visual cue that a competitive outcome is being decided right now. Population counters returned by `update_generation_ctx` are discarded into `dummy_red`/`dummy_blue` variables.
3. **No call to action:** The leaderboard screen shows rankings but provides no invitation for a new visitor to participate (no QR code, no submission URL).

Additionally, the "magic" of Conway's Life — that a tiny 8×8 starting pattern produces a complex evolving colony — is invisible: the initial seed is never shown alongside the running simulation.

#### **2. Decision**

Implement six targeted enhancements to `STATE_KIOSK_MODE` rendering and data plumbing, prioritized by impact-to-effort ratio:

| ID | Enhancement | Component |
|----|-------------|-----------|
| P1 | Show player names (`RED name vs. BLUE name`) above each Multicam quadrant | `app_state_manager.c` + `renderer.c` |
| P2 | Show a live per-quadrant population bar (`RED ■■■□ BLUE`) at the bottom of each quadrant | `app_state_manager.c` + `renderer.c` |
| P3 | Show the `metric_reason` label (e.g., "★ LONGEST MATCH") as a subtitle under the names | `renderer.c` |
| P4 | Show an 8×8 seed thumbnail in the corner of each quadrant to visualize the starting pattern | `renderer.c` |
| P5 | Add a QR code placeholder and submission CTA to the leaderboard screen | `renderer.c` |
| P6 | Replace the text-based "SWITCHING IN X SECONDS" timer with a thin progress bar | `renderer.c` |

**Structural change:** `KioskController` gains two arrays `quad_red_pop[4]` and `quad_blue_pop[4]` to persist per-quadrant live population counts across frames.

The enhancement is purely additive to the existing Kiosk state machine. No existing states, data flows, or API contracts are changed.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**
- Participants can immediately identify their own match, creating a personal connection to the simulation.
- A visible live score creates an instant competitive hook for passersby (stops-the-walk effect).
- The seed thumbnail communicates the core concept of Conway's Life (emergence) without any text.
- A QR code lowers the barrier for new participants to submit patterns on the spot.
- All changes are isolated to the kiosk rendering path; no risk of regressions in interactive or headless modes.

**Negative Consequences (Disadvantages):**
- The HUD overlay per quadrant reduces the visible grid area slightly (estimated 30–40 px per side).
- The 8×8 thumbnail overlaps a corner of the grid; in rare cases, active cells may be obscured. This is an acceptable trade-off for the informational gain.
- Player names are fetched asynchronously; during the first render cycle after a highlight refresh, names may briefly show as empty strings until data arrives.

#### **4. Alternatives Considered**

- **Dedicated "My Match" notification screen:** Would require tracking the local visitor's submission player ID, which is not currently stored on the client. Rejected as too complex for this iteration.
- **Full-screen single-match view for the "winning" match:** Would lose the visual impact of the 2×2 grid. Rejected.
- **Real QR code generation in C:** Would require an embedded QR library. Replaced with a static stylized placeholder (already used in `STATE_PUZZLE`) pointing to a fixed URL.
