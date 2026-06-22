### **ADR-0035: Password-Protected Admin Access for Configuration Deletion**

**Status:** proposed

**Date:** 2026-06-22

#### **1. Context and Problem Statement**

During the fair, the `submissions` collection accumulates configurations that the
operator needs to be able to curate live: duplicate entries from the same player
(who submitted many variants), entire sets belonging to one nickname, or a single
configuration identified by its current leaderboard rank. Today there is **no
administrative capability and no authentication of any kind** in the backend — every
endpoint is open and read-only/append-only.

The operator (a single admin) must be able to perform these deletions **from a phone**
while standing at the booth, which rules out a local-only CLI script and requires a
web UI backed by network-reachable endpoints. That, in turn, means a publicly
reachable deletion path must be introduced and secured.

Constraints derived from the existing system and the booth context:

- A deleted submission is **not isolated**: the tournament worker (`worker.py`) reads
  its roster via `find({"status": "active"})`, caches deterministic match outcomes in
  `match_results`, and guards its 60 s epoch with a roster fingerprint (ADR-0032).
  Name ownership lives separately in `players` (keyed by `nickname_normalized`,
  ADR-0027). Any deletion design must not corrupt these.
- The action is performed **live, under time pressure, from a phone** — a misfire must
  not cause irreversible data loss.
- The booth backend is served over **HTTPS**, so a secret in a request header is not
  exposed on the wire, but the endpoint is still reachable by anyone who finds it.

The decision is: *how do we introduce a secure, phone-usable, reversible deletion
capability for the three required selection modes without destabilising the
tournament pipeline?*

#### **2. Decision**

Introduce a small admin surface consisting of a static-password-protected API router
and a dedicated web page, operating on the principle of **soft delete** rather than
hard delete.

**Authentication.** A single static `ADMIN_PASSWORD` (≥16 random characters) is stored
in `.env` and required on every admin request via an `X-Admin-Key` header. The server
verifies it with `secrets.compare_digest` (constant-time, no timing leak) exposed as a
FastAPI dependency. The web page asks for the password once and keeps it in
`localStorage`. Transport security relies on the booth's HTTPS. No session tokens, no
rate-limiting — a 16+ character random secret over HTTPS is not brute-forceable within
the fair's lifetime, and IP-based lockout is unreliable behind the booth's shared
WLAN/NAT and could lock out the operator.

**Soft delete + restore.** Deletion sets `status: "active"` → `"removed"` and records
`removed_at` plus a reason tag (which option triggered it). The leaderboard already
filters on `status:"active"`, so removed configs vanish from the ranking immediately.
Because the worker derives its roster from the active set, a status change inherently
alters the roster fingerprint, so the worker **re-aggregates automatically** on its
next run (≤60 s) with no manual invalidation and no `match_results` cleanup. Restore is
the inverse status flip. A "trash" view (`GET` removed) plus a restore endpoint make
reversibility reachable from the phone.

**Three selection modes**, exposed through **one endpoint** `POST /api/admin/delete`
with a `mode` field (`A`/`B`/`C`) and a mandatory `dry_run` flag:

- **Mode A — de-duplicate per nickname (global sweep):** For every normalized nickname,
  keep the single best configuration and remove the rest. "Best" is ordered by
  `win_rate` ↓, then `avg_stable_generation` ↑, then `created_at` ↓ (newest wins on a
  pure tie). A rated config therefore always beats an unrated one (`matches_played==0`,
  `win_rate==0`).
- **Mode B — delete all of a nickname:** Soft-delete every non-`removed` submission of
  a nickname (matched **normalized**), including unrated ones. The `players` ownership
  record is left untouched, so the name stays reserved (consistent with reversible
  soft-delete semantics).
- **Mode C — delete by leaderboard rank:** The operator picks a row from the live
  leaderboard; the rank is used only to *select*. Deletion references the resolved
  `submission_id`, never the volatile rank number, so concurrent worker reordering
  cannot delete the wrong row.

All three support `dry_run`, which runs the identical selection logic but mutates
nothing and returns the affected submissions (nickname, seed image, win-rate) plus
counts for an explicit confirmation step in the UI.

**Code placement** follows existing patterns: `backend/app/admin_router.py` (mirroring
`duel_router.py`), wired into `main.py`; web page at `web/admin/admin.html` (mirroring
`web/editor/editor.html`); new `ADMIN_PASSWORD` key documented in `.env.example`.
Auditability is provided by the per-submission `removed_at` + reason tag; no separate
audit collection.

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

- **Reversible by design:** every deletion is a status flip recoverable from the phone,
  so a booth misfire is never data loss.
- **No tournament-pipeline coupling:** the soft-delete approach piggy-backs on the
  existing active-roster filter, so the worker self-heals (≤60 s) with zero new
  invalidation logic and no orphaned `match_results`.
- **Robust mode C:** deleting by `submission_id` after rank-based selection is immune to
  the worker's constant re-ranking.
- **Low surface area & consistent:** one parameterised endpoint, a static-key
  dependency, and files that mirror established project patterns.
- **Mandatory dry-run** with seed + win-rate preview makes the dangerous bulk modes
  (A global sweep, B) safe to trigger from a small screen.

**Negative Consequences (Disadvantages):**

- **A publicly reachable deletion endpoint now exists**; its safety rests entirely on
  the strength of one static secret and on HTTPS being actually enforced at the booth.
- **No defence against credential brute-force or replay** beyond secret strength (no
  rate-limit, no rotation, no expiring sessions); a leaked password grants full delete
  access until the secret is changed and redeployed.
- **Soft delete leaves removed rows in `submissions`** forever; the collection grows and
  a later hard-purge tool may eventually be wanted.
- **Stale stats window:** the surviving configs' win-rates remain slightly outdated for
  up to one worker cycle (≤60 s) after a deletion or restore.
- **`localStorage` password persistence** means a shared/forgotten device retains admin
  access until cleared.

#### **4. Alternatives Considered**

- **Local CLI script only.** Simplest and most secure (no network surface), but cannot
  be operated from a phone at the booth — rejected against the primary requirement.
- **Hard delete.** Clean removal, but requires cascading `match_results` cleanup,
  explicit fingerprint invalidation, and is irreversible — a poor fit for a live,
  phone-driven action where misfires are likely. Rejected in favour of soft delete; a
  hard-purge of `removed` rows can be added later as an offline tool.
- **Real login with expiring session tokens (JWT).** "Proper" auth, but overkill for a
  single operator and more code; the static key over HTTPS meets the threat model.
- **IP-based rate-limiting / lockout.** Unreliable behind booth NAT/shared WLAN, risks
  locking out the operator, and is unnecessary against a 16+ character random secret.
- **Deleting the `players` record in mode B** (freeing the name). Rejected because it is
  irreversible and would orphan restored submissions; name moderation, if ever needed,
  is a separate concern.
- **Deleting mode C blindly by rank number.** Rejected — the rank is volatile across
  worker runs, so it could delete a different config than the one shown.
- **Dedicated `admin_actions` audit collection (incl. failed logins).** Useful but
  beyond a single-admin uni project's needs; the per-submission `removed_at` + reason
  already answers "what was removed and when".
