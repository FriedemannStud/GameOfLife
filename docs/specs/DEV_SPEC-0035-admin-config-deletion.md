# Requirements Analysis & Specification: Password-Protected Admin Configuration Deletion

This document details the requirements for the admin configuration-deletion capability, as described in **ADR-0035**.

---

### 1. Detailed Requirements Specification

**Purpose.** Give the single operator a phone-usable, password-protected way to curate
the `submissions` collection live during the fair by soft-deleting configurations in
three selection modes, with a mandatory preview and reversible restore.

#### Functional Requirements

- **FR-1 — Authentication.** Every admin endpoint requires a valid `X-Admin-Key` header
  matching the `ADMIN_PASSWORD` env var. Missing/invalid key → `401`. Verification uses a
  constant-time comparison (`secrets.compare_digest`).
- **FR-2 — Single delete endpoint.** `POST /api/admin/delete` accepts `mode` (`A`|`B`|`C`),
  the mode-specific parameter(s), and a mandatory `dry_run` boolean.
- **FR-3 — Mode A (de-duplicate per nickname, global sweep).** Across **all** normalized
  nicknames, keep exactly the single best submission per nickname and soft-delete the
  rest. "Best" ordering: `win_rate` ↓, then `avg_stable_generation` ↑, then `created_at`
  ↓ (newest wins on a pure tie). Rated submissions therefore always outrank unrated
  (`matches_played==0`). Nicknames with only one submission are untouched.
- **FR-4 — Mode B (delete all of a nickname).** Soft-delete every non-`removed`
  submission whose normalized nickname matches the requested nickname (including unrated
  submissions). The corresponding `players` record is **not** modified — the name stays
  reserved.
- **FR-5 — Mode C (delete by rank).** Selection is by current leaderboard rank, but the
  request resolves the rank to a concrete `submission_id` and deletes by that id, never
  by the volatile rank number. Exactly one submission is affected.
- **FR-6 — Dry-run preview.** With `dry_run=true`, all three modes execute their identical
  selection logic but mutate nothing and return: the list of affected submissions (each
  with nickname, 8×8 seed grid, and win-rate), the total count to be removed, and (for
  mode A) which submission is kept per nickname.
- **FR-7 — Soft delete semantics.** A delete flips `status: "active"` → `"removed"` and
  writes `removed_at` (UTC timestamp) and a `removed_reason` tag identifying the mode.
  Already-`removed` submissions are never re-affected.
- **FR-8 — Trash listing.** `GET /api/admin/removed` returns all `status:"removed"`
  submissions (nickname, seed, win-rate, `removed_at`, `removed_reason`), newest first.
- **FR-9 — Restore.** `POST /api/admin/restore` takes a `submission_id`, flips
  `"removed"` → `"active"`, and clears `removed_at`/`removed_reason`.
- **FR-10 — Admin web page.** `web/admin/admin.html` asks for the password once (stored in
  `localStorage`, sent as `X-Admin-Key`), offers the three delete modes (each with a
  preview→confirm flow), and a trash tab with per-row restore.
- **FR-11 — Worker self-healing (observed, not coded).** Because the worker derives its
  roster from `status:"active"`, deletes/restores change the fingerprint and trigger
  re-aggregation automatically (≤60 s). No `match_results` cleanup or manual
  invalidation is performed.

#### Non-Functional Requirements

- **NFR-1 — Security.** `ADMIN_PASSWORD` ≥16 random characters; only ever read from
  `.env`; documented in `.env.example` with no real value committed. All admin traffic
  assumed over HTTPS. No rate-limiting in this increment.
- **NFR-2 — Reversibility.** No hard deletes; every destructive action is recoverable via
  the trash/restore path.
- **NFR-3 — Consistency / safety.** Selection logic is shared between dry-run and live
  execution so the preview can never disagree with what is deleted. Mode C deletes by
  resolved id to be immune to concurrent re-ranking.
- **NFR-4 — Code conformance.** Follows `docs/CODING_STYLE.md`; AI-written blocks carry
  `// KI-Agent unterstützt`; mirrors existing `duel_router.py` / `web/editor/` patterns.
- **NFR-5 — Backwards compatibility.** No change to existing public endpoints, schemas,
  or the worker; the only schema addition is the optional `removed_at`/`removed_reason`
  fields on submissions, set only on removed rows.

---

### 2. User Stories & Acceptance Criteria

**Epic: Live Curation of Submitted Configurations**

*   **User Story 1: Secure admin access**
    *   **As the operator,** I want every admin action gated by a password, **so that**
        no booth visitor can delete configurations.
    *   **Acceptance Criteria:**
        *   A request to any `/api/admin/*` endpoint without a valid `X-Admin-Key`
            returns `401` and performs no mutation.
        *   A request with the correct key succeeds.
        *   The key is compared in constant time; the real password never appears in code
            or committed files.

*   **User Story 2: Remove duplicate configs per player (Mode A)**
    *   **As the operator,** I want to remove all but each player's best configuration in
        one action, **so that** the leaderboard isn't dominated by one player's many
        variants.
    *   **Acceptance Criteria:**
        *   A dry-run lists, per affected nickname, which submission is kept and which are
            removed, plus a total removal count, each row showing nickname, seed, win-rate.
        *   "Best kept" follows `win_rate` ↓, `avg_stable_generation` ↑, `created_at` ↓.
        *   Confirming removes exactly the previewed submissions (soft delete) and nothing
            else; nicknames with a single submission are untouched.

*   **User Story 3: Remove all configs of a nickname (Mode B)**
    *   **As the operator,** I want to remove every configuration belonging to one
        nickname, **so that** I can clear out a player's entries while keeping their name
        reserved.
    *   **Acceptance Criteria:**
        *   Nickname matching is normalized (case/whitespace-insensitive).
        *   Dry-run shows all affected submissions (incl. unrated) with count.
        *   Confirming soft-deletes them all; the `players` record for the nickname is
            unchanged.

*   **User Story 4: Remove a config by leaderboard rank (Mode C)**
    *   **As the operator,** I want to pick a row from the live leaderboard and remove
        that configuration, **so that** I can drop a specific entry I can see ranked.
    *   **Acceptance Criteria:**
        *   The UI shows the live leaderboard; selecting rank N previews that exact
            submission (nickname, seed, win-rate).
        *   Confirming deletes by the resolved `submission_id`; if the ranking shifted
            between preview and confirm, the originally previewed submission is the one
            removed (never a different rank-N row).

*   **User Story 5: Preview before deleting (dry-run)**
    *   **As the operator,** I want to see exactly what will be removed before it happens,
        **so that** I don't misfire from my phone.
    *   **Acceptance Criteria:**
        *   Every mode requires an explicit confirm after a dry-run preview.
        *   The preview shows seed image and win-rate per affected row plus totals.
        *   Dry-run mutates nothing.

*   **User Story 6: View and restore removed configs (trash)**
    *   **As the operator,** I want a trash list with a restore button, **so that** an
        accidental deletion is recoverable from the booth.
    *   **Acceptance Criteria:**
        *   The trash tab lists all removed submissions newest-first with nickname, seed,
            win-rate, `removed_at`, and reason.
        *   Restoring a row returns it to `active`, clears `removed_at`/`removed_reason`,
            and it reappears on the leaderboard after the next worker cycle.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   FR-1 auth dependency + `ADMIN_PASSWORD` config (US1).
        *   FR-2/FR-6/FR-7 single delete endpoint with dry-run + soft delete.
        *   FR-3, FR-4, FR-5 modes A/B/C (US2–US4).
        *   FR-8, FR-9 trash listing + restore (US6).
        *   FR-10 admin web page with preview→confirm and trash tab (US5).
    *   **Should-Have:**
        *   Seed thumbnails rendered in previews (reuse `cells_to_grid`).
        *   "Logout"/clear-password control on the admin page.
    *   **Could-Have:**
        *   Mode C rank ranges / multi-select.
        *   Offline hard-purge tool for `removed` rows.
        *   Full `admin_actions` audit collection incl. failed logins.
    *   **Won't-Have (in this increment):**
        *   Rate-limiting / lockout (NFR-1).
        *   Expiring session tokens / real login.
        *   Deleting `players` records / name release in mode B.

*   **Dependencies:**
    1.  **Auth before everything:** the FR-1 dependency must exist before any admin
        endpoint is usable.
    2.  **Selection logic before UI:** the shared dry-run/execute selection functions
        (FR-3/4/5) must exist before the web page can preview/confirm.
    3.  **Soft-delete schema fields** (`removed_at`, `removed_reason`) before trash/restore
        (FR-8/9) are meaningful.
    4.  **Leaderboard read** (existing `/api/leaderboard`) is reused by mode C's UI for
        rank→submission selection.
    5.  **Worker behaviour** (ADR-0032) is a relied-upon invariant (FR-11), not modified.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| B-1 | Live Curation | Add `ADMIN_PASSWORD` to `.env.example`; auth dependency with `secrets.compare_digest` (US1) | Must |
| B-2 | Live Curation | `admin_router.py` scaffold + wire into `main.py` | Must |
| B-3 | Live Curation | Soft-delete fields `removed_at`/`removed_reason` + shared soft-delete helper (FR-7) | Must |
| B-4 | Live Curation | Mode A selection logic (per-nickname best-keep sweep) + dry-run (US2) | Must |
| B-5 | Live Curation | Mode B selection logic (normalized nickname) + dry-run (US3) | Must |
| B-6 | Live Curation | Mode C resolve rank→`submission_id` + dry-run (US4) | Must |
| B-7 | Live Curation | `POST /api/admin/delete` endpoint dispatching modes + `dry_run` (FR-2/6) | Must |
| B-8 | Live Curation | `GET /api/admin/removed` trash listing (US6) | Must |
| B-9 | Live Curation | `POST /api/admin/restore` (US6) | Must |
| B-10 | Live Curation | `web/admin/admin.html`: password prompt + 3 modes preview→confirm (US1–US5) | Must |
| B-11 | Live Curation | `web/admin/admin.html`: trash tab with restore + seed thumbnails (US6) | Must |
| B-12 | Live Curation | Backend unit tests for selection logic (A/B/C, normalization, tie-break, dry-run no-op) | Must |
| B-13 | Live Curation | Auth tests (401 missing/invalid, success) | Must |
| B-14 | Live Curation | Clear-password / logout control on admin page | Should |

---

### 5. Definition of Done (DoD)

A Product Backlog Item is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The code is written and formatted according to `docs/CODING_STYLE.md`
    (`black .`, `ruff check .` for Python); AI-written blocks carry `// KI-Agent unterstützt`.
*   **Tests:**
    *   All new backend selection/auth functions are covered by unit tests
        (`backend/tests/test_*.py`, self-running per CLAUDE.md).
    *   Dry-run-equals-execution and "no-op when no duplicates" are explicitly tested.
    *   All existing tests continue to pass (no regressions).
*   **Acceptance Criteria:** All acceptance criteria for the story are met and manually
    verified via the admin page (interactive test by the developer).
*   **Code Review:** The code is in a reviewable state (PR).
*   **Merge:** Merged into the `biotop` branch.
*   **Documentation:** ADR-0035 finalized; `docs/CHANGELOG.md` updated; `.env.example`
    documents `ADMIN_PASSWORD`.
