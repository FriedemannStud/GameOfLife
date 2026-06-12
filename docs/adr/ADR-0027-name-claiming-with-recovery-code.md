### **ADR-0027: Name-Claiming with Server-Generated Recovery Code**

**Status:** Implemented

**Date:** 2026-06-12

---

#### **1. Context and Problem Statement**

Participants of the university-fair web editor (`web/editor/editor.html`) choose a free-text
species name (`nickname`) and submit their 8x8 pattern to `POST /api/v1/submit_config`. The name
is the public identity shown on the kiosk leaderboard.

Today there is **no ownership guarantee on a name**. The player upsert is keyed by `nickname`
(`main.py:56`, `update_one({"nickname": ...})` with `upsert=True`). A second visitor who types an
existing name simply overwrites the original player's `player_id` — they silently *take over* the
name. There is no uniqueness index and no proof-of-ownership. The inline comment at `main.py:55`
already flags this as a placeholder: *"Keyed by nickname for test phase; swap to player_id once
email auth is added."*

The goal: **a name, once claimed, cannot be reused by a different person.** The hard constraint is
the setting — a playful, single-day exhibition. Full email/account authentication (verification
mail, password reset flow) is disproportionate friction at a fair booth. We need the *weakest
mechanism that still enforces single ownership* while letting a legitimate owner return — including
from a different device.

Relevant existing structure we can build on:
- The frontend already generates a UUID `player_id`, persists it in
  `localStorage['biotope_player_id']`, and sends it in `metadata.player_id` on every submit
  (`editor.html:224-232, 356`). This is, in effect, a silent per-browser secret that already
  exists end-to-end — only server-side enforcement is missing.
- The **only** producer of `submit_config` requests is the web editor. The interactive C-GUI
  (`network_io.c`) performs *GET-only* calls (`/api/leaderboard`, `/api/epoch/highlights`); it
  never POSTs a submission. Locally saved C configs carry placeholder identity
  (`player_id: "local_user"`, `file_io.c:58-59`) and never reach the backend. The new mechanism
  therefore has **zero impact on the C application**.

---

#### **2. Decision**

Introduce **name-claiming with a two-factor ownership proof**:

> Ownership of a name is proven by **either** the silent `player_id` (from `localStorage`) **or**
> a server-generated **recovery code**. First claim wins; later devices need the code.

**Mechanism**

1. **Normalization.** A name's identity is its normalized form: `trim()` + `lowercase`. Stored as
   `nickname_normalized`, carrying a **unique index**. The original spelling is preserved in
   `nickname` for display. This closes trivial impersonation via case (`VibeMaster` vs
   `vibemaster`).

2. **Silent ownership (factor 1).** On first submit of a free name, the server binds
   `nickname_normalized → player_id`. Subsequent submits whose `player_id` matches are accepted
   silently. Zero friction for a returning visitor on the same browser.

3. **Recovery code (factor 2).** On the **first claim only**, the server generates a short,
   human-readable code (Crockford Base32, e.g. `BIOTOP-7F3A`, no confusable `0/O`, `1/I`). The
   **plaintext is returned exactly once** in the `201` response body and is never persisted; only
   a **SHA-256 hash** (`recovery_code_hash`) is stored. The frontend shows it on a
   screenshot-friendly card with an optional "save as PNG" download.

4. **Reclaim (inline).** When a name is taken and the presented `player_id` does not match, the
   request is rejected. The visitor enters their recovery code; the **same** `submit_config` call
   is retried with the code carried in a dedicated top-level `auth.recovery_code` field. The
   server hashes it, compares against `recovery_code_hash`, and on match performs **re-binding**:
   the stored `player_id` is overwritten with the requesting device's UUID (the most recently
   code-verified device becomes the silent owner). This is exactly password-reset semantics.

5. **Secret/content separation.** The recovery code lives in a top-level `auth` block —
   **never** inside `metadata`. The submission persists `metadata` + `config` only, so no
   `model_dump()` path can leak the plaintext code into the `submissions` collection.

**Machine-readable error contract** (so the inline frontend flow can branch deterministically):

| Situation | Response |
|---|---|
| Name free | `201` + `recovery_code` (plaintext) in body |
| Name taken, `player_id` matches | `201`, no code |
| Name taken, valid `auth.recovery_code` | `201` (reclaim → re-binding), no code |
| Name taken, **no** code | `409 {"error": "name_taken"}` → frontend reveals code field |
| Name taken, **wrong** code | `403 {"error": "invalid_recovery_code"}` → field stays, hint shown |
| Domain rule violation (biomass etc.) | `400` |

The current catch-all `except Exception → 500` (`main.py:82`) is corrected so that conflict and
validation paths are not swallowed into `500`.

**Data lifecycle.** Because production data is throwaway test data and no recovery codes have ever
been issued, migration is pointless. A **clean-slate** reset script clears `players` +
`submissions` and (re)creates the unique index on `players.nickname_normalized` before the fair.

**Explicitly out of scope (structure prepared, logic deferred):** code *regeneration* (issuing a
new code while still holding the device). The data model accommodates it, but for a single-day fair
the code is purely device-transfer insurance; a same-device owner never needs it.

**Payload shape (prepared structure):**

```jsonc
{
  "metadata": { "player_id": "uuid-from-localStorage", "nickname": "VibeMaster", "league": "einsteiger" },
  "config":   { "bounding_box_x": 8, "bounding_box_y": 8, "cells": [[0, 0]] },
  "auth":     { "recovery_code": null }   // normally empty; filled only on reclaim
}
```
`nickname_normalized` and `recovery_code_hash` are derived **server-side** and never appear in the
payload.

---

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

- **Goal met:** a normalized name can be owned by exactly one identity; impersonation by case or
  whitespace is structurally blocked by the unique index.
- **Near-zero friction on the happy path:** returning same-device visitors are recognized silently
  via the already-existing `localStorage` `player_id` — no new UI in the common case.
- **Cross-device recovery without email:** the short code is the entire account-recovery surface,
  appropriate to a fair.
- **Security hygiene as a teaching artifact:** recovery codes are stored only as hashes — the
  canonical "never store secrets in plaintext" pattern — and the secret is structurally isolated
  from submission content.
- **No C-side work and no regression risk** there: the C-GUI never submits.

**Negative Consequences (Disadvantages):**

- **Lost code + lost/cleared device = name lost forever.** With regeneration out of scope, a
  visitor who clears `localStorage` and never saved the code cannot recover the name. Accepted for
  the fair's scope.
- **Network-loss edge case:** if the first-claim succeeds server-side but the `201` (carrying the
  plaintext code) is lost in transit, the visitor owns the name silently but never saw the code.
  Because regeneration is deferred, the code is then unrecoverable. Accepted, noted as the
  strongest future argument *for* regeneration.
- **Re-binding allows takeover:** anyone who learns a code can capture the name and lock out the
  original device. This is inherent to any recovery-code/password-reset design and immaterial at a
  booth.
- **Clean slate is destructive:** the reset script must be run deliberately and only before the
  event.

---

#### **4. Alternatives Considered**

- **Email/account authentication.** The "correct" general solution, rejected as disproportionate
  friction (verification round-trips, password resets) for a single-day playful exhibit.
- **User-chosen PIN/password (portable secret).** Cross-device by design, but adds mandatory
  per-visit friction and a "forgot PIN" failure mode at the booth. Rejected in favor of the silent
  `player_id` happy path plus an optional code.
- **Recovery code = the raw `player_id` UUID.** Avoids an extra field, but a UUID is practically
  impossible to transcribe by hand at a booth, defeating the purpose of cross-device recovery.
  Rejected for a short human-readable code.
- **Plaintext recovery code storage.** Functionally sufficient for a throwaway DB, but contradicts
  the project's learning goal and leaks all codes on any DB/backup exposure. Rejected for SHA-256
  hashing.
- **Exact (case-sensitive) name matching.** Simplest, but allows trivial `VibeMaster`/`vibemaster`
  impersonation — directly undermining the feature's purpose. Rejected for normalized matching.
- **Recovery code inside `metadata`, stripped on persist.** Works, but is error-prone: a single
  missed `exclude` on any `model_dump()` path re-leaks the plaintext. Rejected for a structurally
  separate top-level `auth` block.
- **Separate `POST /api/v1/reclaim` endpoint.** Cleaner separation of concerns, but two round-trips
  and more moving parts/race conditions; the inline single-schema flow is simpler to test.
- **Real data migration instead of clean slate.** Pointless — existing players have no issued
  codes (so migrated codes are worthless) and normalized-name de-duplication adds complexity for
  throwaway test data.
