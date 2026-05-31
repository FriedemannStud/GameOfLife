# Requirements Analysis & Specification: Oscillator-Aware Highlight Filtering

This document details the requirements for oscillator detection and filtering in the Kiosk Mode highlight pipeline, as described in **ADR-0023**.

---

### 1. Detailed Requirements Specification

The current highlight selection pipeline ranks matches by `activity_sum`. Oscillating cellular patterns (period 2–5) achieve disproportionately high scores because they generate constant cell activity without territorial progress. The Python Worker pipeline must be extended to detect and demote such patterns before highlights are stored in MongoDB and served to Kiosk clients.

**Core Requirements:**

| ID | Requirement | Priority |
|---|---|---|
| R-01 | Oscillating patterns with periods 2, 3, 4, and 5 MUST be detected and demoted. | Must |
| R-02 | Non-oscillating matches MUST appear before oscillating matches in the stored highlights list. | Must |
| R-03 | If fewer than 4 non-oscillating matches are available, oscillating matches MUST fill remaining slots (soft fallback). | Must |
| R-04 | Detection MUST run asynchronously to the C Hyper-Worker (post-processing step only). | Must |
| R-05 | Detection MUST use the 50×50 Kiosk world configuration matching `app_state_manager.c`. | Must |
| R-06 | The C codebase (Hyper-Worker, Kiosk Client, network layer) MUST NOT be modified. | Must |
| R-07 | Epoch processing time increase from detection MUST remain below 10 seconds. | Should |
| R-08 | Each oscillator detection event SHOULD be logged (match names, period detected, metric value). | Could |

---

### 2. User Stories & Acceptance Criteria

**Epic: Improve Kiosk Live Battles Display Quality**

- **User Story 1: Suppress Oscillating Matches**
    - **As an** exhibition visitor watching the kiosk display, **I want** the Live Battles screen to show matches that visibly evolve over time, **so that** the display remains engaging and demonstrates interesting emergent behavior throughout my observation.
    - **Acceptance Criteria:**
        - AC-1.1: No displayed match shall consist of a cellular pattern repeating with period 2, 3, 4, or 5 — provided at least 4 non-oscillating candidates exist in the epoch highlights pool.
        - AC-1.2: The ordering of the stored `epoch_highlights` document places all non-oscillating matches before all oscillating matches.
        - AC-1.3: Within each group (non-oscillating / oscillating), matches remain sorted by `metric_value` descending (preserving the original activity-sum order).

- **User Story 2: Stable Fallback When All Candidates Oscillate**
    - **As a** system operator at the university fair, **I want** the kiosk to always display exactly 4 matches (assuming 4+ highlights exist), **so that** the screen is never empty or partially filled due to aggressive filtering.
    - **Acceptance Criteria:**
        - AC-2.1: The stored highlights list in MongoDB always contains `min(10, total_candidates)` entries.
        - AC-2.2: If N non-oscillating matches exist with N < 4, the first `4 - N` oscillating matches (highest `metric_value`) fill the remaining slots at positions N through 3 of the stored list.
        - AC-2.3: The existing Kiosk Client requires no code changes to benefit from the filtered ordering.

- **User Story 3: No Regression in Epoch Performance**
    - **As a** tournament operator, **I want** epoch processing to complete within the existing ~60-second window, **so that** the leaderboard and highlight update schedule is not disrupted.
    - **Acceptance Criteria:**
        - AC-3.1: The oscillator detection step (10 matches × 1000 generations × 50×50 grid) adds less than 10 seconds to a single epoch execution on the target server hardware.
        - AC-3.2: All existing backend tests (`test_ranking.py`, `test_validators.py`) continue to pass without modification.

---

### 3. Prioritization and Dependency Analysis

- **Prioritization (MoSCoW Method):**
    - **Must-Have (MVP):**
        - Detection of period-2 through period-5 oscillators in the Python Worker.
        - Reordering of the highlights list: non-oscillating first, oscillating as fallback.
        - No regressions in existing epoch pipeline or backend tests.
    - **Should-Have:**
        - Unit tests for the oscillator detector covering known oscillator and non-oscillator patterns.
        - Logging of detection results per epoch.
    - **Could-Have:**
        - Detection of period-6 and period-7 oscillators (minor extension of the period loop).
        - A `is_oscillator` boolean field on individual highlight documents in MongoDB.
    - **Won't-Have (in this increment):**
        - Detection of periods > 5.
        - Visual indicator on the kiosk display indicating a match was promoted from the oscillator fallback pool.
        - Any changes to the C Hyper-Worker or Kiosk Client.

- **Dependencies:**
    1. **ADR-0023** must be in "Accepted" status before implementation begins.
    2. **ADR-0024 (Rotation / Problem A):** The oscillator-filtered pool (up to 10 ordered matches in MongoDB) is a prerequisite for implementing client-side highlight rotation. ADR-0023 must be implemented and verified before ADR-0024 work begins.
    3. **Docker environment:** `docker-compose up` must produce a running backend with MongoDB for integration verification.
    4. **NumPy:** Must be listed in `backend/requirements.txt`; if absent, add it before implementation.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| B-01 | Highlight Quality | Add NumPy to requirements.txt if missing | Must |
| B-02 | Highlight Quality | Implement `_step_numpy(grid)` — one generation of two-team Conway rules | Must |
| B-03 | Highlight Quality | Implement `is_oscillating_match(red_seed, blue_seed, max_gen)` | Must |
| B-04 | Highlight Quality | Implement `filter_highlights_by_oscillation(highlights, max_gen)` with logging | Must |
| B-05 | Highlight Quality | Integrate filter call into `execute_epoch()` | Must |
| B-06 | Highlight Quality | Unit test: period-2 blinker pattern → True | Should |
| B-07 | Highlight Quality | Unit test: period-3 pattern → True | Should |
| B-08 | Highlight Quality | Unit test: dead grid → False | Should |
| B-09 | Highlight Quality | Unit test: dynamic multi-team match → False | Should |
| B-10 | Highlight Quality | Integration test: run epoch, inspect MongoDB highlight ordering | Must |

---

### 5. Definition of Done (DoD)

A backlog item is considered "Done" when all of the following criteria are met:

- **Code Quality:** Code follows `docs/CODING_STYLE.md`. AI-generated or AI-assisted functions are marked `# KI-Agent unterstützt`. Python code uses `snake_case` for functions/variables.
- **Tests:** All unit tests in `backend/tests/test_oscillator_detection.py` pass. No regressions in `test_ranking.py` or `test_validators.py`.
- **Acceptance Criteria:** All ACs from User Stories 1–3 are met and verified.
- **Integration:** `docker-compose up` runs cleanly; epoch worker logs show oscillator detection results; MongoDB `epoch_highlights` documents show non-oscillating matches before oscillating matches.
- **Documentation:** ADR-0023 status updated from "Proposed" to "Implemented".
