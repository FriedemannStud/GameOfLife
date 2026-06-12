# Requirements Analysis & Specification: Name-Claiming with Recovery Code

This document details the requirements for unique, ownable species names with a server-generated
recovery code in the web editor, as described in **ADR-0027**.

---

### 1. Detailed Requirements Specification

**Functional Requirements**

*   **FR-1 (Name uniqueness):** A normalized name (`trim()` + `lowercase`) may be owned by exactly
    one identity. The system enforces this via a unique index on `players.nickname_normalized`. The
    original spelling is preserved separately for display.
*   **FR-2 (Silent ownership):** The first submit of a free name binds `nickname_normalized →
    player_id`. A later submit whose `player_id` matches the bound one is accepted without any
    additional step.
*   **FR-3 (Recovery code generation):** On the first claim of a name only, the server generates a
    short, human-readable recovery code (Crockford Base32, no confusable `0/O`/`1/I`, prefixed,
    e.g. `BIOTOP-7F3A`). The plaintext is returned exactly once in the `201` response body.
*   **FR-4 (Recovery code storage):** Only a SHA-256 hash of the code (`recovery_code_hash`) is
    persisted on the player record. The plaintext is never stored.
*   **FR-5 (Inline reclaim):** When a name is taken and the presented `player_id` does not match,
    the visitor may retry the **same** `submit_config` call with the code in `auth.recovery_code`.
    On hash match the submit is accepted.
*   **FR-6 (Re-binding):** A successful reclaim overwrites the stored `player_id` with the
    requesting device's UUID; that device becomes the silent owner thereafter.
*   **FR-7 (Secret isolation):** The recovery code is carried in a top-level `auth` block, never in
    `metadata`, and is never written into the `submissions` collection.
*   **FR-8 (Error contract):** The endpoint returns machine-readable outcomes: `201` (+ code on
    first claim), `409 {"error":"name_taken"}`, `403 {"error":"invalid_recovery_code"}`, `400`
    (domain rule violation).
*   **FR-9 (Recovery card UI):** After a first claim, the frontend shows the code on a
    screenshot-friendly card in a closable overlay, with an optional "save as PNG" download button.
*   **FR-10 (Reclaim UI):** On `409`, the frontend reveals a recovery-code input and re-submits; on
    `403` the field stays visible with a "wrong code" hint.
*   **FR-11 (Reset script):** A clean-slate script clears `players` + `submissions` and ensures the
    unique index on `nickname_normalized` exists.

**Non-Functional Requirements**

*   **NFR-1 (Scope fit):** No email, no password, no account system. Friction on the happy path
    (same-device returning visitor) must be zero.
*   **NFR-2 (Security hygiene):** Secrets stored only as hashes; secret structurally separated from
    content; normalization prevents case/whitespace impersonation.
*   **NFR-3 (No C-side impact):** The C application (`network_io.c` is GET-only) must remain
    unchanged and unaffected.
*   **NFR-4 (Backwards behavior):** The existing `metadata`/`config` payload remains valid; the new
    `auth` block is optional and defaults to empty.
*   **NFR-5 (Robustness):** Malformed/empty `auth` blocks, missing codes, and concurrent claims of
    the same name (race) must not corrupt state; the unique index is the concurrency guard.
*   **NFR-6 (Testability):** Normalization, code generation/hashing, and the claim/reclaim decision
    logic must be unit-testable without a live database.

---

### 2. User Stories & Acceptance Criteria

**Epic: Own and Recover a Species Name**

*   **User Story 1: Claim a fresh name**
    *   **As a** first-time visitor, **I want** my chosen species name to become mine when I submit,
        **so that** nobody else can later pose as me on the leaderboard.
    *   **Acceptance Criteria:**
        *   Submitting a free name returns `201`.
        *   The response body contains a plaintext `recovery_code`.
        *   A `players` record exists with `nickname`, `nickname_normalized`, my `player_id`, and a
            `recovery_code_hash` (never the plaintext).
        *   The submission is stored in `submissions` and contains **no** recovery code.

*   **User Story 2: Return on the same device**
    *   **As a** returning visitor on the same browser, **I want** to submit again without any extra
        step, **so that** revisiting is frictionless.
    *   **Acceptance Criteria:**
        *   Submitting my own name with my stored `player_id` returns `201`.
        *   No recovery code is shown again.
        *   No new `players` record is created; the existing one is reused.

*   **User Story 3: Be blocked from taking someone else's name**
    *   **As a** different visitor, **I want** clear feedback that a name is taken, **so that** I
        understand I cannot impersonate its owner.
    *   **Acceptance Criteria:**
        *   Submitting an existing name with a non-matching `player_id` and no code returns
            `409 {"error":"name_taken"}`.
        *   `vibemaster`, `VibeMaster`, and `  VibeMaster ` are all treated as the same name.
        *   No data is overwritten by the rejected attempt.

*   **User Story 4: Recover my name on a new device**
    *   **As the** legitimate owner on a new phone, **I want** to enter my recovery code, **so that**
        I regain my name.
    *   **Acceptance Criteria:**
        *   After a `409`, entering the correct code and re-submitting returns `201`.
        *   The stored `player_id` is now my new device's UUID (re-binding).
        *   An incorrect code returns `403 {"error":"invalid_recovery_code"}` and does not change any
            stored data.

*   **User Story 5: Save my recovery code**
    *   **As a** visitor who just claimed a name, **I want** an easy way to keep my code, **so that**
        I can recover later.
    *   **Acceptance Criteria:**
        *   A closable overlay shows the code on a clearly laid-out card (name + code).
        *   A "save as PNG" button downloads an image of the card.
        *   Closing the overlay does not invalidate the code (server-side it remains valid).

*   **User Story 6: Operator resets before the fair**
    *   **As the** booth operator, **I want** a one-shot reset, **so that** I start the event with a
        clean, correctly-indexed database.
    *   **Acceptance Criteria:**
        *   Running the reset script empties `players` and `submissions`.
        *   After it runs, a unique index exists on `players.nickname_normalized`.
        *   Attempting to insert two players with the same normalized name fails at the DB level.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   FR-1 normalized unique name (model + index).
        *   FR-2 silent ownership via `player_id`.
        *   FR-3/FR-4 recovery code generation + hashed storage.
        *   FR-5/FR-6 inline reclaim + re-binding.
        *   FR-7 secret isolation in `auth` block.
        *   FR-8 error contract (`201`/`409`/`403`/`400`).
        *   FR-11 reset script + index creation.
    *   **Should-Have:**
        *   FR-9 recovery card overlay (screenshot-friendly).
        *   FR-10 reclaim input UI on `409`/`403`.
    *   **Could-Have:**
        *   "Save as PNG" download (FR-9 sub-feature) — graceful degradation to a plain card if
            `<canvas>` is unavailable.
    *   **Won't-Have (in this increment):**
        *   Recovery-code **regeneration** (data model prepared, logic deferred — ADR-0027).
        *   Email/account authentication.
        *   Web Share API integration.
        *   Migration of existing data (clean slate instead).

*   **Dependencies:**
    1.  **Data model before API:** the unique index and player schema (FR-1) must exist before the
        endpoint logic (FR-2/5/6) can rely on it.
    2.  **Error contract before frontend:** the `409`/`403`/`201` semantics (FR-8) must be fixed
        before the inline reclaim UI (FR-10) can branch on them. This also requires fixing the
        catch-all `except Exception → 500` (`main.py:82`).
    3.  **Payload schema before both ends:** the `auth` block (FR-7) is shared by backend models and
        the frontend submit payload.
    4.  **Reset script before testing:** the unique index (FR-11) is a precondition for integration
        tests that assert uniqueness.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| B-01 | Own/Recover Name | Add `nickname_normalized`, `recovery_code_hash` to `Player`; `auth` block + optional `recovery_code` to `Submission` models | Must |
| B-02 | Own/Recover Name | Normalization helper (`trim`+`lowercase`) — unit-tested | Must |
| B-03 | Own/Recover Name | Recovery code generator (Crockford Base32) + SHA-256 hashing helper — unit-tested | Must |
| B-04 | Own/Recover Name | Claim/reclaim decision logic in `submit_config` (silent / reclaim / re-bind) | Must |
| B-05 | Own/Recover Name | Error contract: `409 name_taken`, `403 invalid_recovery_code`, fix `500` catch-all | Must |
| B-06 | Own/Recover Name | Persist submission without leaking `auth.recovery_code` | Must |
| B-07 | Own/Recover Name | Reset script + ensure unique index on `nickname_normalized` | Must |
| B-08 | Own/Recover Name | Frontend: send `auth` block; handle `201`+code / `409` / `403` | Should |
| B-09 | Own/Recover Name | Frontend: recovery-card overlay (closable) | Should |
| B-10 | Own/Recover Name | Frontend: reclaim input field flow | Should |
| B-11 | Own/Recover Name | Frontend: "save as PNG" download of the card | Could |
| B-12 | Own/Recover Name | Backend unit + integration tests for claim/reclaim/uniqueness | Must |
| B-13 | Own/Recover Name | Frontend: clarifying `409` hint for first-time visitors with a name collision (no recovery code) — distinguish "not your name, pick another" from "your name, enter code" | Won't (this increment) |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following
criteria are met:

*   **Code Quality:** The code is written and formatted according to the guidelines in
    `docs/CODING_STYLE.md`. Python is formatted/linted (`black .`, `ruff check .`); any
    AI-authored block carries the `# KI-Agent unterstützt` attribution comment.
*   **Tests:**
    *   New backend functions (normalization, code generation/hashing, claim/reclaim decision) are
        covered by unit tests runnable without a live DB.
    *   The end-to-end claim → block → reclaim flow is verified by an integration test.
    *   All existing backend tests continue to pass (no regressions).
*   **Acceptance Criteria:** All acceptance criteria for the story are met and manually verified in
    the web editor against a running backend.
*   **Security:** No plaintext recovery code is ever persisted (verified by inspecting `players` and
    `submissions` after a reclaim); the `auth` block never appears in stored submissions.
*   **Code Review:** The code has been reviewed (or is in a reviewable PR state).
*   **Merge:** Merged into the development branch (`biotop`).
*   **Documentation:** ADR-0027 reflects the final decision; the CHANGELOG is updated on completion.
