# Requirements Analysis & Specification: Kiosk UI Architecture Refactor & UX Enhancement

This document details the requirements for the two-phase kiosk refactor described in **ADR-0022**.
Phase A is already implemented and tested. This spec focuses on the acceptance criteria for Phase B.

---

### 1. Detailed Requirements Specification

The Kiosk Mode must operate as a self-running exhibition display at a university fair. It cycles
between a leaderboard and a multi-match live view. The following non-functional requirements apply:

- **Readability at distance:** All text must be legible from 80 cm without leaning forward.
- **No mouse dependency:** All interaction is keyboard-only. No on-screen instruction may reference a mouse.
- **Scalability:** The layout must adapt to any screen resolution and any number of simultaneous matches without code changes.
- **Visual consistency:** Font sizes, spacing, and color usage must follow the Modern Retro Dark Mode palette (defined in `renderer.c` theme constants).

---

### 2. User Stories & Acceptance Criteria

**Epic: Kiosk Display Quality**

- **User Story 1: Leaderboard Readability**
  - **As a** fair visitor standing 80 cm from the screen, **I want to** read the leaderboard rankings clearly, **so that** I immediately understand who is winning the tournament.
  - **Acceptance Criteria:**
    - Rank 1, 2, 3 have a distinct background highlight in addition to their rank color.
    - The column header "STAMINA" is replaced by a self-explanatory term ("ENDURANCE").
    - All text is ≥ 20 px.

- **User Story 2: QR Code & Call to Action**
  - **As a** fair visitor who wants to participate, **I want to** see a clear invitation with a scan-able QR code, **so that** I know how to submit my own pattern.
  - **Acceptance Criteria:**
    - The QR placeholder occupies a clearly bordered panel on the right side of the leaderboard.
    - The QR image is ≥ 140×140 px.
    - The CTA text ("Submit YOUR strategy!") is ≥ 20 px.
    - The submission URL is ≥ 18 px.

- **User Story 3: Footer Visibility**
  - **As a** fair visitor, **I want to** see a clear, persistent call-to-action at the bottom of every kiosk screen, **so that** I know how to interact with the installation.
  - **Acceptance Criteria:**
    - Both kiosk screens (leaderboard and multicam) have a solid footer panel backing the progress bar and "PRESS [P] TO PLAY" text.
    - The footer panel is visually separated from the simulation content.
    - No simulation content is obscured by the footer.

- **User Story 4: Multicam Live Drama**
  - **As a** fair visitor watching the live battles, **I want to** instantly understand which team is winning each match, **so that** I feel the competitive tension.
  - **Acceptance Criteria:**
    - The score bar at the bottom of each quadrant is ≥ 20 px tall.
    - Score numbers inside the bar are ≥ 14 px.
    - The four quadrants are separated by a visible cross-line (≥ 2 px).

- **User Story 5: Pattern Recognition**
  - **As a** participant who submitted a pattern, **I want to** recognise my starting seed in the multicam view, **so that** I can identify my own match.
  - **Acceptance Criteria:**
    - Each quadrant shows two seed thumbnails (blue left, red right) with cells ≥ 8 px each.
    - Thumbnails have a dark backing rectangle ensuring visibility on any background.

- **User Story 6: Correct Interaction Instructions**
  - **As a** fair visitor, **I want** every on-screen instruction to be accurate, **so that** I am not confused by instructions that do not work.
  - **Acceptance Criteria:**
    - The text "CLICK ANY MATCH TO VIEW REPLAY" is removed.
    - Match replay is accessible via keyboard (number keys `[1]`–`[N]` or `[ENTER]`).
    - The on-screen hint reflects the actual key.

- **User Story 7: Professional Screen Title**
  - **As a** fair visitor, **I want** screen titles to be descriptive, **so that** I understand what I am watching.
  - **Acceptance Criteria:**
    - The multicam top-bar label "WUSEL-MULTICAM KIOSK MODE" is replaced by a visitor-facing label (e.g., "LIVE BATTLES").
    - The `metric_reason` label (e.g., "★ LONGEST MATCH") is displayed as a distinct badge below the player name strip.

---

### 3. Prioritization and Dependency Analysis

- **Must-Have (MVP):**
  - Remove "CLICK ANY MATCH" and add keyboard replay (correctness issue)
  - Footer backing panel (readability)
  - Score bar ≥ 20 px (readability)
  - Quadrant separator line (visual clarity)

- **Should-Have:**
  - Top-3 background highlight
  - QR panel enlarged
  - "STAMINA" → "ENDURANCE"
  - Better multicam title

- **Could-Have:**
  - Seed thumbnails enlarged (nice to have; already visible at 5 px/cell)
  - `metric_reason` as badge

- **Won't-Have (this increment):**
  - Real QR code generation (requires external library)
  - Animated transitions between screens

- **Dependencies:**
  - Phase A must be complete and tested before Phase B begins. ✓ (Done 2026-05-31)
  - All Phase B items use `KioskLayout` values from `compute_kiosk_layout()`. No item may introduce new magic-number pixel values.

---

### 4. Definition of Done

A Phase B item is "Done" when:

- **Code:** Implemented in `draw_kiosk_leaderboard()` or `draw_kiosk_multicam()` (or `app_state_manager.c` for interaction changes). No magic pixel numbers — all sizes from `KioskLayout`.
- **Style:** Complies with `docs/CODING_STYLE.md` (snake_case, `// KI-Agent unterstützt`, 4-space indent, zero `make` warnings).
- **Build:** `make` produces zero warnings.
- **Test:** Visually verified by the developer in an interactive session with running Docker backend.
- **Regression:** All previously working features (leaderboard data, multicam simulations, `[P]`/`[K]` navigation, session replay) still function correctly.
