# LLM Session Log — Name-Claiming with Recovery Code (ADR-0027)

**Date:** 2026-06-12
**Branch:** `biotop`
**Agent:** Claude (Opus 4.8)
**Mode:** Design session (`/grill-me`) → spec-driven documentation (`/run-dev`), stopping before implementation.

---

## 1. Objective

Make a species **name in the web editor (`web/editor/editor.html`) ownable**: once a name is
claimed it cannot be reused by a different person. Constraint: this is a **single-day, playful
university-fair** application, so full **email/account authentication is out of scope** (too much
friction at a booth). A lightweight ownership-proof was required, and the JSON payload should be
**structurally prepared** for it.

---

## 2. Starting Point (codebase facts established)

- Frontend already generates a UUID `player_id`, stores it in `localStorage['biotope_player_id']`,
  and sends it in `metadata.player_id` on every submit (`editor.html:224-232, 356`). This is, in
  effect, a **silent per-browser secret that already exists end-to-end** — only server enforcement
  is missing.
- Backend `submit_config` upserts the player **keyed by `nickname`** (`main.py:56`) with no
  uniqueness guarantee — a second visitor typing an existing name silently overwrites the original
  `player_id` (name takeover). Inline comment already flags this as a placeholder pending auth.
- `DBSubmission(**submission.model_dump())` (`main.py:72`) persists the **whole** submission incl.
  `metadata` — so any secret placed in `metadata` would leak into the `submissions` collection.
- The catch-all `except Exception → 500` (`main.py:82`) swallows legitimate validation/conflict
  errors.
- **The C application never submits:** `network_io.c` is GET-only (`/api/leaderboard`,
  `/api/epoch/highlights`); locally-saved C configs carry placeholder identity (`file_io.c:58-59`)
  and never reach the backend. → The new mechanism has **zero impact on C code**.
- `worker.py:243` updates `players` by exact `nickname`; it creates no players and never touches
  recovery fields → unaffected.

---

## 3. Design Decisions (resolved via structured interview)

| # | Decision | Choice |
|---|---|---|
| 1 | Ownership-proof mechanism | Silent `localStorage` `player_id` (factor 1) **+** server-generated recovery code (factor 2) |
| 2 | Recovery code form | Separate **short human-readable** code (Crockford Base32, e.g. `BIOTOP-7F3A`), not the raw UUID |
| 3 | Saving the code (UX) | Screenshot-friendly **card** + optional **"save as PNG"** download, in a closable overlay (web page cannot trigger an OS screenshot itself) |
| 4 | Code storage | **SHA-256 hash** only (`recovery_code_hash`); plaintext returned once, never persisted |
| 5 | Name uniqueness | **Normalized** identity: `trim()` + `lowercase` (+ collapsed whitespace), stored as `nickname_normalized` with a **unique index**; original spelling kept for display |
| 6 | Reclaim flow | **Inline**: `auth.recovery_code` carried in the same `submit_config` call (single schema, single endpoint) |
| 7 | After successful reclaim | **Re-binding**: stored `player_id` is overwritten with the new device's UUID (password-reset semantics; one active device per name) |
| 8 | Error contract | `409 {"error":"name_taken"}`, `403 {"error":"invalid_recovery_code"}`, `201` (+ code on first claim); fix the `500` catch-all. **To be documented in the ADR.** |
| 9 | Existing data | **Clean slate** reset script (no migration — no codes were ever issued) + ensure unique index |
| 10 | Code regeneration | **Out of scope** (data model prepared, logic deferred); accepted edge cases: lost code + lost device = name lost; first-claim response lost in transit = code unrecoverable |
| 11 | Where the secret lives | **Top-level `auth` block**, never inside `metadata` → no `model_dump()` path can leak it into stored submissions |

### Trust model (one sentence)
> Ownership of a name is proven by **either** the silent `player_id` **or** the recovery code.
> First claim wins; later devices need the code.

### Prepared payload shape
```jsonc
{
  "metadata": { "player_id": "uuid-from-localStorage", "nickname": "VibeMaster", "league": "einsteiger" },
  "config":   { "bounding_box_x": 8, "bounding_box_y": 8, "cells": [[0, 0]] },
  "auth":     { "recovery_code": null }   // normally empty; filled only on reclaim
}
```
`nickname_normalized` and `recovery_code_hash` are derived **server-side**, never in the payload.

---

## 4. Backend Outcome Matrix (the contract)

| Condition | Status | Body |
|---|---|---|
| Domain rule violation (biomass > 24, out of box) | `400` | `{"detail": "<reason>"}` |
| Name free → claim | `201` | `{..., "recovery_code": "BIOTOP-7F3A"}` |
| Name taken, `player_id` matches (silent owner) | `201` | no code |
| Name taken, valid `auth.recovery_code` → reclaim | `201` | `{..., "reclaimed": true}` |
| Name taken, **no** code | `409` | `{"error": "name_taken"}` |
| Name taken, **wrong** code | `403` | `{"error": "invalid_recovery_code"}` |

Concurrency: simultaneous claims race on `insert_one`; the unique index → `DuplicateKeyError` →
converted to `409 name_taken`.

---

## 5. Artifacts Produced (this session)

All under `docs/`, numbered **0027**:

- `adr/ADR-0027-name-claiming-with-recovery-code.md` — Status `proposed`. Context, decision,
  consequences, alternatives.
- `specs/DEV_SPEC-0027-name-claiming-with-recovery-code.md` — 11 FR + 6 NFR, 6 user stories with
  acceptance criteria, MoSCoW prioritization, backlog B-01…B-12, DoD.
- `tech_design/DEV_TECH_DESIGN-0027-name-claiming-with-recovery-code.md` — component overview +
  Mermaid interaction diagram, data model, backend spec (new `auth_utils.py`, decision pseudocode),
  frontend spec + Mermaid sequence diagram (claim→block→reclaim), security & performance.
- `tasks/DEV_TASKS-0027-name-claiming-with-recovery-code.md` — 6-phase implementation plan with a
  verification step after every change.

No production/source code was changed yet. Implementation (Step 4 of `/run-dev`) was **not** started.

---

## 6. Planned Implementation (DEV_TASKS-0027 phases)

1. **Phase 1 — Auth utilities (`backend/app/auth_utils.py`):** `normalize_nickname`,
   `generate_recovery_code` (Crockford Base32, alphabet `0123456789ABCDEFGHJKMNPQRSTVWXYZ`),
   `hash_recovery_code` (SHA-256) + unit tests (`test_auth_utils.py`). DB-free first.
2. **Phase 2 — Models (`models.py`):** add `Auth` model + `Submission.auth`; ensure `DBSubmission`
   excludes `auth`; add `nickname_normalized` + `recovery_code_hash` to `Player`.
3. **Phase 3 — Endpoint (`main.py`):** create unique index at startup; rewrite `submit_config`
   (claim / silent / reclaim+re-bind); fix exception handling; remove the old nickname-keyed upsert.
   Verified via a 6-case `curl` matrix + Mongo inspection (no plaintext persisted).
4. **Phase 4 — Ops & tests:** `backend/scripts/reset_db.py` (clean slate + index); integration test
   for claim→block→reclaim; regression run of existing tests.
5. **Phase 5 — Frontend (`editor.html`):** send `auth` block; handle response matrix; recovery-card
   overlay + PNG download. Interactive tests performed by the developer (Team Principle).
6. **Phase 6 — Finalize:** CHANGELOG entry, ADR status → `Implemented`, `black`/`ruff`, full test
   sweep.

---

## 7. How to Continue

- Review the four 0027 documents.
- Resume `/run-dev` at **Step 4 / DEV_TASKS-0027 Phase 1, Step 1.1** (create `auth_utils.py`).
- Each task step ends with an explicit verification; report results before proceeding.
- Remember: recovery code must **never** appear in the `submissions` collection — the dedicated test
  in Phase 3.3 / Phase 4.2 asserts this.
- C side requires **no** changes.
