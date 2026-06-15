### **ADR-0029: Prevent Accidental Duplicate Submissions via Per-Player Name Uniqueness**

**Status:** Invalid / Withdrawn (2026-06-15)

> This ADR has been declared **invalid**. The feature was a mistake and is not pursued.
> The corresponding task plan [DEV_TASKS-0029](../tasks/DEV_TASKS-0029-prevent-duplicate-submissions.md)
> is likewise void. The content below is retained for historical reference only.

**Date:** 2026-06-15

---

#### **1. Context and Problem Statement**

In the web editor (`web/editor/editor.html`) a participant can press **"Ab in die Arena"**
repeatedly and re-submit the **same** starting configuration under the **same** species name. Each
press creates a fresh `submissions` document, because the silent-owner branch of `submit_config`
persists unconditionally (`main.py:150-157`, branch 2b). The harm is concrete:

- The leaderboard is **per configuration** (one row per submission, ADR-0025) → duplicate rows.
- The tournament is an **O(N²)** round-robin (`main_hyper.c`) → each duplicate costs quadratic work.

**Threat model (deliberately narrowed).** We distinguish two cases:

- **Accidental** — a visitor presses the submit button again without changing the form. This is the
  real, common problem and the one we must defeat.
- **Intentional flooding** — a visitor *deliberately* types a fresh species name each time to spam
  the database. This is destructive, but the **manual friction of entering a new name** in the
  species field is an acceptable, "critically-bounded" hurdle for a single-day university exhibition.
  We explicitly do **not** engineer against it.

An earlier exploration of a heavier model (max-3 mutations per player with an overwrite picker, a
config-hash deduplicator, and `retire`+insert persistence) was **rejected as disproportionate** to
the actual problem (see §4). The simplification rests on one observation: the accidental case is
*always* "same name, same device, pressed again", which corresponds exactly to branch 2b.

**Relevant existing structure we build on:**
- ADR-0027 already binds a normalized species name to one owner and routes same-device re-submits
  through branch 2b (silent owner). That branch is precisely where the accidental duplicate is born.
- The submit button is already disabled for the duration of an in-flight request (`editor.html`,
  `submitBtn.disabled = true`), so the only remaining same-device race is a deliberate one.
- The C application never POSTs submissions (GET-only, `network_io.c`), so this change has **zero
  impact on the C side** — identical reasoning to ADR-0027.

---

#### **2. Decision**

Adopt a single rule, enforced in two layers:

> **Per `player_id`, a normalized species name may back at most one *active* submission.**
> To submit again, the participant must enter a **different** species name.

**Layer 1 — Browser guard (primary; where the accidental press happens).**
After a successful `201`, the editor remembers the just-submitted **normalized** name. `updateUI()`
keeps "Ab in die Arena" disabled while the name field still equals that name, with a hint to rename.
This kills the repeated-press case at the source, in the browser — exactly where the problem occurs.

**Layer 2 — Server backstop (guarantee).**
In branch 2b (silent owner), before persisting, reject with `409 {"error": "duplicate_submission"}`
if an **active** submission already exists for this `(player_id, nickname_normalized)`. To support a
direct, normalization-consistent query — and to aid later data analysis — the persisted submission
gains a derived `nickname_normalized` field. This turns branch 2b, which is otherwise always a
"same name again" event, into a deterministic rejection while leaving the claim (2a), reclaim (2c),
and validation (400) paths untouched.

**Normalization parity.** The client mirrors the server normalization
(`trim` + `lowercase` + collapse internal whitespace) so the disabled-state and the server verdict
never disagree.

**Error-contract addition** (extends ADR-0027's machine-readable matrix):

| Situation | Response |
|---|---|
| Same `player_id`, **same** name, active submission exists | `409 {"error": "duplicate_submission"}` → editor shows "rename to resubmit" |
| Same `player_id`, **different** name | normal claim path (`201`) |
| Name taken by a **different** `player_id`, no code | `409 {"error": "name_taken"}` (unchanged, ADR-0027) |

The frontend's existing `409` handler is split on `data.detail.error`: `name_taken` keeps the
recovery-code reveal (ADR-0027); `duplicate_submission` shows the rename hint and leaves submit
disabled until the name changes.

**Scope of "active".** The check filters `status: "active"`, so a future `retired` submission would
free its name for resubmission. Today nothing sets `retired`, so in practice branch 2b always
rejects once a name has been claimed — which is the intended behavior.

---

#### **3. Consequences of the Decision**

**Positive:**
- **Accidental duplicates are eliminated** at their source (the repeated press), both in the UI and
  as a server guarantee.
- **Minimal, localized change:** one guard in branch 2b plus one derived field; no new endpoint, no
  new collection, no C-side work.
- **Leaderboard hygiene:** no more duplicate per-config rows; no wasted O(N²) tournament work on
  identical entries.
- **`nickname_normalized` on submissions** is reusable for the analysis data the team wants to keep.

**Negative / accepted:**
- **New name + identical genome still slips through.** A deliberate duplicate under a fresh name is
  allowed. Accepted: this is the "intentional" class, gated only by manual name entry, per the
  threat model.
- **The app-level check is not atomic.** Two truly simultaneous same-device requests could both pass
  the `find_one`. Mitigated by the in-flight button disable and the low concurrency of a booth. If a
  hard guarantee is ever required, the noted upgrade is a partial unique index (§4).
- **Iterating a pattern requires a rename.** Re-submitting an improved genome means choosing a new
  name. This is minor, intentional friction consistent with the "different names per player" rule.

---

#### **4. Alternatives Considered**

- **Max-3 mutations per player + overwrite picker (the "Variante 3" we explored).** Rejected as
  disproportionate: it adds a config-card selection UI, `retire`+insert persistence, and forces
  handling of a tournament write-back race onto the overwritten slot — all to solve a problem that
  is fundamentally just "don't double-submit". The investigation did, however, surface two useful
  facts now recorded here: the `in_match` status is **never written** anywhere, and both the
  tournament worker and the duel run on a **snapshot** of `cells` (worker temp-batch; duel
  `_resolve_config` at lock-in), so no in-progress match can be corrupted by a later edit.
- **Config-hash deduplication** (reject identical genomes regardless of name). Heavier (canonical
  sorting + hashing + a stored field) and *mismatched to intent*: the rule we want is "different
  names per player", not "different genomes". Rejected.
- **Partial unique index on `submissions` `(metadata.player_id, nickname_normalized)` with
  `partialFilterExpression {status: "active"}`.** The atomic-guarantee version, mirroring the
  `players.nickname_normalized` unique index (`main.py:139`). Deferred: the application-level check
  is sufficient for the booth's concurrency, and the index is the clean upgrade path if needed —
  the new `nickname_normalized` field already prepares for it.
- **Browser-only guard, no server check.** Sufficient for the *accidental* goal but bypassable by
  reload/`curl`. Rejected because the decision is to make the rule **guaranteed**, not merely
  convenient.
- **Note on Elo.** The investigation confirmed `elo_rating` is **dead weight**: the worker never
  writes it back (`worker.py:250-260` sets `win_rate`/`rank`/match stats only) and the leaderboard
  sorts by `win_rate` (`main.py:213`). Removing the vestigial field is out of scope here and left as
  a separate cleanup.
