### **ADR-0028: Shoulder-Duel Mode — Server-Authoritative Real-Time 1:1 Matches**

**Status:** implemented

**Date:** 2026-06-13

---

#### **1. Context and Problem Statement**

The current multiplayer loop is **asynchronous and pool-based**: a player draws an 8x8 start
configuration in the web editor (`web/editor/editor.html`), submits it to the server
(`POST /api/v1/submit_config`), and the configuration joins a global tournament pool. A background
worker (`backend/app/worker.py`) periodically runs an O(N²) round-robin via
`build/biotope_hyper_worker` and updates Elo ratings. The player watches an aggregate leaderboard;
there is **no way to play a specific opponent on demand**.

The motivating scenario is the **university fair** and, crucially, *beyond* it. Two students stand
shoulder to shoulder — at the booth, or later in the cafeteria — phones in hand, and want to settle
a direct 1:1 *right now*: "I beat Frank earlier, let's see who's better." The desired loop is
**spontaneous, location-independent, and immediate**: connect, each pick one of their own
configurations, watch them fight, see a winner, rematch. The driving persona ("Bob") values three
things: a sub-ten-second start, a result he can *brag about* against a *named* opponent, and a
match experience that feels like a game (a build-up, a soundtrack, a payoff) rather than a data
view.

**Hard constraints derived from the setting:**

- **Location-independent.** The duel must not depend on the kiosk's large screen (`STATE_KIOSK_MODE`)
  or on any fixed installation. Two phones plus internet is the only guaranteed environment. The
  cafeteria has no booth screen.
- **Flaky network.** Fair WiFi and cellular in a crowded hall are unreliable. The design must
  tolerate latency and brief dropouts without producing two phones that disagree about what
  happened.
- **No cheating, no disputes.** When Bob loses he must not be able to claim the other phone "showed
  something different." There must be a single source of truth for the outcome.
- **Minimal friction at the "let's go!" moment.** No account creation, no lobby-code typing, no
  pairing menus in the critical path.

**Existing structure we can build on:**

- The simulation engine is **fully deterministic**: red start + blue start uniquely determine the
  entire match and its winner. There is no randomness and no in-match player input. A match outcome
  is therefore a *pure function* of the two start configurations.
- `build/biotope_headless <red.json> <blue.json> [out.json]` already computes exactly one such
  match in milliseconds and emits a result JSON. This is precisely the primitive a duel needs — it
  exists today and is used by the worker.
- The backend is **FastAPI + MongoDB** (`backend/app/`), already exposing GET endpoints
  (`/api/leaderboard`, `/api/epoch/highlights`) and the `submit_config` POST, with collections
  `submissions`, `players`, `epoch_highlights`.
- **Identity already exists end-to-end.** Per ADR-0027, every browser carries a silent UUID
  `player_id` in `localStorage['biotope_player_id']`, a claimed display `nickname`, and a recovery
  code. A duel can reuse this identity instead of inventing a new one.
- A **WebAssembly build of the core** already exists (`Makefile.wasm`, ADR-0013), i.e. the exact
  C rule-set can run in the browser without re-implementing the rules in JavaScript.

The problem: provide a **synchronous, on-demand, head-to-head duel** between two co-located phones,
authoritative and robust over flaky networks, that records a **personal, per-player match history**
the player can show off — and that *feels* like a game (build-up, soundtrack, payoff).

---

#### **2. Decision**

Introduce a **Shoulder-Duel Mode**: a new web flow (in `web/`, alongside the editor) backed by new
FastAPI endpoints and a new `duels` collection. The architecture rests on five decisions.

**2.1 The server is the referee; matches are computed once, not streamed.**

Because the engine is deterministic, a duel does **not** require live frame-by-frame
synchronization between phones. Instead:

1. Each phone sends only its chosen **8x8 start configuration** (tiny payload).
2. The server runs `biotope_headless` **exactly once** for the pairing and obtains the
   **authoritative result** (winner, end generation, final populations, optional highlight seed).
3. This result is the single source of truth. It is stored and returned to *both* phones
   identically. No phone can fabricate a different outcome; there is nothing to dispute.

This deliberately rejects live streaming of the running simulation. There is no per-frame network
traffic and therefore no desynchronization failure mode over flaky WiFi.

**2.2 Replay is local and deterministic; the server verdict is canonical.**

The *animation* of the match is reconstructed **locally on each phone** by re-running the same
deterministic simulation from the two start configurations — reusing the **existing
WASM-compiled core** (`Makefile.wasm`), so the rules are not re-implemented and cannot diverge.
Both phones, running the identical engine from identical seeds, render an identical animation. The
**displayed winner is always the server's verdict**, never the local re-run, so even a
hypothetical engine mismatch can never change the official outcome — it would at worst affect
pixels, not the record. The exact wiring (WASM module vs. a lightweight frame payload as fallback)
is left to the technical-design step; this ADR fixes only the principle: *small seed-based payload,
deterministic local replay, server-canonical verdict.*

**2.3 Pairing is via QR scan and carries identity.**

To connect, player A taps **"Duel"**; the server creates a short-lived **duel room** and A's phone
displays a **QR code** encoding the room id (with a 4-character room code as a manual fallback).
Player B taps **"Scan"**, reads the code, and joins the room. The QR/room handshake **also carries
each phone's existing identity** (`player_id` + claimed `nickname` from ADR-0027), so no name is
typed in the critical path. Co-location is required only for the scan — exactly Bob's
shoulder-to-shoulder scenario.

**Guest fallback:** a phone that has never claimed a name in the editor joins as a transient guest
(e.g. `Gast-7F3K`). Such matches are recorded locally immediately; after the first match a gentle,
non-blocking prompt invites claiming a name so the result "counts" and syncs to the server. The
"let's go!" moment is never gated behind identity setup.

**2.4 State transitions use short polling, not websockets.**

A duel has very few state transitions (room created → both joined → both locked in → result ready →
rematch). The phones discover these by **short polling** the room state — consistent with the
existing GET-based architecture, trivial to reason about, and resilient to brief dropouts (a missed
poll simply retries). No websocket/long-lived-connection infrastructure is introduced. Choices are
submitted **hidden and simultaneously**; the server only reveals both picks and computes the match
once *both* lock-ins have arrived.

**2.5 Personal duel record falls out of the referee, stored local-first with server backup.**

Because the server computes *every* duel, it records each one into a new **`duels`** collection
(both `player_id`s, both `nickname`s, both chosen config ids, winner, end generation, timestamp).
The player-facing **"My Duels"** view is then a query over this collection for the requesting
identity, surfacing:

- **Head-to-head per opponent** ("vs. FRANK 3:1") — the bragging primitive.
- **"Best weapon"** — which of the player's own configurations wins most, closing the loop back to
  the editor.

Per Bob's requirement, the record is **local-first** (counted instantly in `localStorage` for
zero-friction, no-login bragging) **and mirrored server-side** when an identity is present, keyed
to the ADR-0027 name + recovery code so the record survives device loss, cache clearing, and a new
semester. On divergence the **server is authoritative**.

**2.6 Sound: a self-hosted Suno soundtrack with attribution.**

Each duel plays a soundtrack (build-up on the VS splash, payoff on the WIN/LOSE screen). The track
is the project's own Suno creation ("Open Flow"), **downloaded and self-hosted** as a static asset
(e.g. `web/assets/`), **not** streamed live from Suno. Self-hosting makes playback reliable on
flaky/absent network (browser-cached after first load), keeps full control over volume/loop/fade,
avoids fragile CDN hotlinking, and is legally clean for a self-generated track. An **unobtrusive
credit link** ("♪ Music: 'Open Flow' — made with Suno") points interested players to the source.

**The complete game loop:**

> QR-connect (carries identity) → hidden simultaneous config choice → both lock in → server
> computes once (referee) → synchronized local replay with VS-splash & soundtrack → WIN/LOSE
> payoff → **Rematch** — with every result written to the player's personal duel record.

**Explicitly out of scope for this increment** (structure may be prepared, logic deferred):
remote/asynchronous duels between players who are *not* co-located (the "share a link, play later"
variant); tournaments of 3+ players in this mode; in-match interaction; spectating via the kiosk
big screen (a desirable later booth bonus, but not a dependency). The kiosk big screen remains an
optional showcase, never a requirement.

---

#### **3. Consequences of the Decision**

**Positive Consequences (Advantages):**

- **Goal met:** two co-located phones can play an on-demand, authoritative 1:1 anywhere, with a
  result neither can dispute, in well under ten seconds from "let's go."
- **Robust over flaky networks by construction:** compute-once + seed-based payload + local
  deterministic replay + short polling means no per-frame traffic and no desync failure mode.
- **High reuse, low new surface:** `biotope_headless` (referee), the WASM core (replay), ADR-0027
  identity (pairing + record ownership), and FastAPI/MongoDB are all existing. The genuinely new
  parts are the duel-room endpoints, the `duels` collection, and the web duel UI.
- **No second rule implementation:** reusing the WASM-compiled core avoids a JavaScript GOL port
  and the divergence risk of maintaining two engines.
- **Personal leaderboard is nearly free:** it is a byproduct of the referee writing to `duels`;
  head-to-head and "best weapon" require no new game logic.
- **Closes the design loop back to the editor:** "best weapon" pressures players to build better
  start configurations, driving editor re-engagement.
- **Reliable, controllable, legally clean audio** with a discovery funnel back to Suno.

**Negative Consequences (Disadvantages):**

- **New stateful server surface:** duel rooms are ephemeral shared state with their own lifecycle
  (creation, join, lock-in, expiry/cleanup) — more moving parts than the stateless GET endpoints,
  and a new source of race conditions (e.g. double lock-in) to handle carefully.
- **Polling cost and latency:** short polling adds request volume and a small, perceptible reaction
  delay at each transition versus a push model. Accepted for robustness and simplicity at fair
  scale.
- **WASM replay coupling:** the browser replay depends on the WASM build staying in lockstep with
  the headless engine's rules. Mitigated by the server-canonical verdict (divergence affects pixels,
  not the record) but still an integration point to watch.
- **Local-first record can drift:** before a name is claimed, the bragging record lives only in one
  browser and is lost on cache clear — identical to the ADR-0027 trade-off, accepted for the
  setting.
- **Guest matches dilute the named record** until claimed; the post-match claim prompt is the only
  mitigation, and some guests will never claim.
- **Co-location required:** this increment intentionally does not serve remote play; "play me later"
  is deferred.

---

#### **4. Alternatives Considered**

- **Live frame streaming between phones (server relays each generation).** The "obvious"
  real-time approach, rejected: it introduces per-frame traffic and a genuine desynchronization
  failure mode over flaky fair WiFi, and gains nothing because the match is deterministic and fully
  known the instant both seeds are submitted.
- **Server sends the full generation-by-generation frame history; phones are pure video players.**
  Guarantees pixel-identical playback with no client engine, but the frame history of a match on a
  growing grid is heavy (potentially megabytes) — a poor fit for mobile over weak networks.
  Rejected in favor of the tiny seed payload plus local deterministic replay; a compact frame
  payload remains a viable *fallback* if WASM replay proves impractical.
- **Re-implement the GOL/Biotop rules in JavaScript for replay.** Avoids the WASM toolchain, but
  creates a second authoritative-looking rule engine to keep in sync with the C core — exactly the
  divergence risk we want to avoid. Rejected in favor of reusing the existing WASM build.
- **Peer-to-peer between phones (Bluetooth / WebRTC / local network).** Removes the server from the
  match path, but pairing and NAT/Bluetooth reliability are a fair-floor nightmare (iOS
  restrictions, range, permissions), and it surrenders the server-as-referee guarantee. Rejected.
- **Lobby with typed room codes instead of QR.** Functional, but typing a code is friction in the
  "let's go!" moment; the QR scan is faster and more magical, with a typed code retained only as a
  fallback. Rejected as the primary mechanism.
- **Websockets for room state.** Lower latency and cleaner push semantics, but introduces
  long-lived-connection infrastructure and reconnection handling for a flow with only a handful of
  transitions. Rejected for short polling at this scale; revisitable if latency proves objectionable.
- **Mandatory name before the first duel (no guest path).** Cleanest records, but a hurdle at the
  exact "let's go!" instant. Rejected in favor of guest play with a post-match claim prompt.
- **Server-only duel record (no local-first).** Simpler consistency, but forces identity/login
  before anything is counted and stalls on flaky networks. Rejected in favor of local-first +
  server backup per the player's explicit requirement.
- **Live-streaming the Suno track from `suno.com/s/...`.** Rejected: the share page exposes no
  stable embeddable audio source, live streaming is fragile and network-dependent (silence at the
  booth), and hotlinking the CDN is legally grey. Self-hosting the self-generated track is reliable
  and clean.
- **Reuse the existing asynchronous tournament pool for "duels."** The pool is aggregate and
  delayed; it cannot express "play *this* specific person *now*" or a personal head-to-head record.
  Rejected as not solving the stated problem.
