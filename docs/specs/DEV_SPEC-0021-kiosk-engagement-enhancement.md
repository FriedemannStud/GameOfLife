# Requirements Analysis & Specification: Kiosk Mode Engagement Enhancement

This document details the requirements for the Kiosk Mode Engagement Enhancement, as described in **ADR-0021**.

---

### 1. Detailed Requirements Specification

The Kiosk Mode must be enhanced to (a) make individual participants' simulations identifiable, (b) provide an immediate competitive signal for passersby, and (c) invite new visitors to participate. All changes are confined to `STATE_KIOSK_MODE` rendering and data plumbing within the existing state machine.

---

### 2. User Stories & Acceptance Criteria

**Epic: Kiosk Mode as Fair Eye-Catcher**

*   **User Story 1: Identify my own simulation**
    *   **As a fair visitor who submitted a pattern,** I want to see my name displayed above my running match in the Multicam view, **so that** I can point it out to others and feel personally invested.
    *   **Acceptance Criteria:**
        *   The name of the red player is shown in `THEME_RED` color above or at the top of the quadrant.
        *   The name of the blue player is shown in `THEME_BLUE` color.
        *   Both names are sourced from the `MatchHighlight.participant_red/blue` fields.
        *   Names update correctly when a new highlight batch arrives from the API.

*   **User Story 2: Understand who is winning at a glance**
    *   **As a passerby with no prior context,** I want to see a live score indicator per match, **so that** I immediately understand that a competition is happening and who is ahead.
    *   **Acceptance Criteria:**
        *   Each quadrant displays a proportional population bar showing Red vs. Blue cell counts.
        *   The bar updates every simulation tick (consistent with the 0.1 s update cycle).
        *   When one team has zero cells, the bar shows 100% for the surviving team.

*   **User Story 3: Understand the concept from the starting pattern**
    *   **As a curious visitor,** I want to see the tiny 8×8 pattern that started the simulation alongside the running result, **so that** I intuitively grasp that simple rules produce complex behavior.
    *   **Acceptance Criteria:**
        *   A small 8×8 thumbnail is displayed in a corner of each quadrant.
        *   Red seed cells are shown in `THEME_RED`, blue seed cells in `THEME_BLUE`.
        *   The thumbnail is visually distinct from the main simulation (bordered, semi-transparent background).

*   **User Story 4: Know why this match was selected**
    *   **As a viewer,** I want to see a short label explaining why this match is being featured, **so that** the curation feels meaningful rather than random.
    *   **Acceptance Criteria:**
        *   A label from `MatchHighlight.metric_reason` (e.g., "★ LONGEST MATCH") is shown below the player names.
        *   The label uses `THEME_ACCENT` color.

*   **User Story 5: Submit my own pattern on the spot**
    *   **As a new fair visitor,** I want a clear visual invitation with a URL or QR code to submit my own pattern, **so that** I can participate without needing to ask a booth attendant.
    *   **Acceptance Criteria:**
        *   The leaderboard sub-state displays a stylized QR code placeholder.
        *   A submission URL and CTA text ("Submit YOUR strategy!") is shown next to the QR code.
        *   The QR code is derived from the existing pattern in `STATE_PUZZLE` (no new library required).

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   P1 — Player names per Multicam quadrant
        *   P2 — Live population score bar per quadrant
    *   **Should-Have:**
        *   P3 — Metric reason label
        *   P5 — QR code + CTA on leaderboard
    *   **Could-Have:**
        *   P4 — 8×8 seed thumbnail
        *   P6 — Progress bar replacing text timer
    *   **Won't-Have (in this increment):**
        *   Real QR code generation (requires external library)
        *   "My Match" push notification to the participant's browser

*   **Dependencies:**
    1. **P1 data plumbing** must be completed before P1 rendering (names must be stored in `KioskController` before they can be drawn).
    2. **P2 data plumbing** (adding `quad_red_pop[4]` / `quad_blue_pop[4]` to `KioskController`) must be completed before P2 rendering.
    3. **P3 and P4** can be implemented independently once P1 data plumbing is done (they read from `cached_highlights` directly).
    4. **P5 and P6** have no data dependencies; they are pure renderer additions.

---

### 4. Product Backlog

| ID  | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| 21-1 | Data Plumbing | Forward participant names from `HighlightData` into `kiosk_ctrl.sims[i]` | Must |
| 21-2 | Data Plumbing | Add `quad_red_pop[4]` / `quad_blue_pop[4]` to `KioskController`; populate in update loop | Must |
| 21-3 | Renderer | Draw player name header per Multicam quadrant | Must |
| 21-4 | Renderer | Draw live population bar per Multicam quadrant | Must |
| 21-5 | Renderer | Draw metric reason label per quadrant | Should |
| 21-6 | Renderer | Draw 8×8 seed thumbnail per quadrant | Could |
| 21-7 | Renderer | Add QR placeholder + CTA to leaderboard sub-state | Should |
| 21-8 | Renderer | Replace text timer with progress bar | Could |

---

### 5. Definition of Done (DoD)

A feature item is considered "Done" when:

*   **Code Quality:** Code follows `docs/CODING_STYLE.md` (snake_case, PascalCase, AI attribution comment, 4-space indent, no trailing whitespace).
*   **Compilation:** `make` produces zero warnings with `-Wall -Wextra`.
*   **Functional Test:** Kiosk mode is launched, highlight data is received, and all rendered elements (names, score bar, labels) appear correctly in the Multicam view.
*   **No Regressions:** Interactive mode (`STATE_RUNNING`, `STATE_EDIT_RED/BLUE`) is unaffected. The click-to-replay feature still works.
*   **Documentation:** `CHANGELOG.md` updated upon completion.
