# Requirements Analysis & Specification: Shoulder-Duel Mode

This document details the requirements for the **Shoulder-Duel Mode** — an on-demand,
server-authoritative 1:1 duel between two co-located phones — as decided in **ADR-0028**.

---

### 1. Detailed Requirements Specification

#### 1.1 Functional Requirements

**Pairing & Identity**

- **FR-1** A player can start a duel from the web UI, which creates a short-lived **duel room** on
  the server and displays a **QR code** encoding the room id, plus a 4-character manual room code as
  fallback.
- **FR-2** A second player can join the room by **scanning the QR code** (or entering the room
  code). On joining, the room holds exactly two participants.
- **FR-3** The QR/room handshake transports each phone's existing identity (`player_id` and claimed
  `nickname` from ADR-0027). No name is typed during the join flow.
- **FR-4** A phone without a claimed name joins as a **transient guest** (e.g. `Gast-7F3K`). The
  duel proceeds normally; identity is not a precondition to play.
- **FR-5** A room expires after a defined idle timeout and is cleaned up; a room that already holds
  two participants rejects further joins.

**Choice & Match**

- **FR-6** Each participant selects exactly one of **their own** start configurations for the duel.
- **FR-7** Choices are **hidden** from the opponent until both participants have locked in
  (simultaneous, blind selection).
- **FR-8** Once both participants have locked in, the server runs `biotope_headless` **exactly
  once** for the pairing and obtains the **authoritative result** (winner team, end generation,
  final red/blue populations, and — if available — a highlight seed).
- **FR-9** The authoritative result is returned **identically** to both phones and is the single
  source of truth for the outcome.
- **FR-10** Each phone reconstructs and renders the match **animation locally and deterministically**
  from the two start configurations (reusing the existing WASM core, per ADR-0028 §2.2). The
  **displayed winner is always the server verdict**, never the local re-run.

**Presentation & Loop**

- **FR-11** Before the animation, both phones show an identical **VS splash** (red config + name vs.
  blue config + name) with the soundtrack building up.
- **FR-12** At the end, both phones show a **WIN / LOSE** result screen with a payoff (soundtrack
  cue + visual emphasis).
- **FR-13** A **Rematch** action is offered on the result screen; accepting it returns both
  participants to the hidden-choice step within the same room.
- **FR-14** The duel plays a **self-hosted soundtrack** (the project's Suno track), with build-up on
  the VS splash and payoff on the result screen.
- **FR-15** An **unobtrusive credit link** ("♪ Music: 'Open Flow' — made with Suno") is shown,
  pointing to the Suno source.

**Personal Duel Record ("My Duels")**

- **FR-16** Every completed duel is recorded by the server into a **`duels`** collection: both
  `player_id`s, both `nickname`s, both chosen config ids, winner, end generation, timestamp.
- **FR-17** The result is recorded **local-first** (counted immediately in `localStorage`) for
  zero-friction bragging without login, and **mirrored to the server** when an identity is present.
  On divergence, the **server is authoritative**.
- **FR-18** A player can open a **"My Duels"** view showing:
  - **Head-to-head per opponent** (e.g. "vs. FRANK 3:1").
  - **Best weapon** — which of the player's own configurations has the most duel wins.
  - Overall win/loss tally.
- **FR-19** The server-side record is retrievable for an identity authenticated by the ADR-0027
  `player_id` / recovery code, so it survives device loss, cache clearing, and a new session.
- **FR-20** A guest who later claims a name is prompted (gently, post-match, non-blocking) to claim;
  on claim, their local guest record is associated with the claimed identity.

#### 1.2 Non-Functional Requirements

- **NFR-1 (Latency to play):** From "let's go" to the first VS splash, the flow targets **under ~10
  seconds** on a normal phone and fair-grade network.
- **NFR-2 (Network resilience):** The flow tolerates brief dropouts and latency. State transitions
  use **short polling**; a missed poll retries without corrupting state. There is **no per-frame
  network traffic**.
- **NFR-3 (Authoritativeness):** The match outcome is computed solely server-side; no client input
  can alter the recorded result.
- **NFR-4 (Determinism integrity):** Local replay must not contradict the server verdict in the
  displayed outcome, even if the WASM replay diverges from the headless engine.
- **NFR-5 (Audio reliability):** The soundtrack plays from a **self-hosted** asset, browser-cached
  after first load; it must not depend on a third-party live stream.
- **NFR-6 (No regression):** The existing editor, submission pool, kiosk mode, and C application are
  unaffected. The C-GUI continues to perform GET-only API calls.
- **NFR-7 (Security/exposure):** Recovery codes are never exposed by duel endpoints; duel payloads
  carry no secrets (consistent with ADR-0027's `auth`/`metadata` separation).
- **NFR-8 (Mobile-first):** All duel UI is usable one-handed on a phone screen in portrait.

---

### 2. User Stories & Acceptance Criteria

**Epic A: Connect & Start a Duel**

*   **User Story A1: Instant QR pairing**
    *   **As a** fair visitor standing next to a friend, **I want** to start a duel and have my
        friend join by scanning a code, **so that** we are playing within seconds without typing
        anything.
    *   **Acceptance Criteria:**
        *   Tapping "Duel" creates a room and shows a QR code plus a 4-character fallback code.
        *   Scanning the QR (or entering the code) places both players in the same room.
        *   Both phones show a "connected" state once two participants are present.
        *   A third scan of a full room is rejected with a clear message.

*   **User Story A2: Identity travels with the connection**
    *   **As a** returning player who already has a name, **I want** my name to be used
        automatically when I connect, **so that** the duel and its record show *me*, not a stranger.
    *   **Acceptance Criteria:**
        *   The joined room shows each player's claimed `nickname` without any name entry step.
        *   A player without a claimed name is shown as a guest (e.g. `Gast-7F3K`) and can still
            play.

**Epic B: Play the Match**

*   **User Story B1: Blind simultaneous choice**
    *   **As a** competitor, **I want** to pick my configuration without seeing my opponent's
        choice, **so that** the duel is fair and suspenseful.
    *   **Acceptance Criteria:**
        *   Each player selects one of their own configurations.
        *   Neither player can see the other's selection before both have locked in.
        *   The match is computed only after both lock-ins are received.

*   **User Story B2: Authoritative, dispute-free result**
    *   **As a** player who might lose, **I want** the outcome to be computed by the server and
        shown identically on both phones, **so that** there is nothing to argue about.
    *   **Acceptance Criteria:**
        *   The winner shown on both phones is identical and originates from the server.
        *   The match is computed exactly once per pairing via `biotope_headless`.

*   **User Story B3: It feels like a game**
    *   **As a** player, **I want** a VS build-up, a synchronized match animation, a soundtrack, and
        a WIN/LOSE payoff, **so that** the duel is exciting, not a data view.
    *   **Acceptance Criteria:**
        *   A VS splash shows both configs + names with a build-up sound cue.
        *   Both phones render the same match animation from the same seeds.
        *   The result screen clearly shows WIN or LOSE with an audio/visual payoff.
        *   The soundtrack plays from a self-hosted asset and survives a dropped network.

*   **User Story B4: One more!**
    *   **As a** player who just finished, **I want** an obvious Rematch button, **so that** we
        immediately play again without re-pairing.
    *   **Acceptance Criteria:**
        *   The result screen shows a Rematch action.
        *   Accepting returns both players to the hidden-choice step within the same room.

**Epic C: Bragging Rights (My Duels)**

*   **User Story C1: Personal head-to-head record**
    *   **As** Bob, **I want** a personal list of who I have beaten and lost to, **so that** I can
        show Frank "vs. FRANK 3:1" to his face.
    *   **Acceptance Criteria:**
        *   "My Duels" shows a per-opponent head-to-head tally.
        *   Results are counted immediately after each match (local-first), without login.
        *   With an identity present, the record is mirrored to and retrievable from the server.

*   **User Story C2: Best weapon**
    *   **As a** player, **I want** to see which of my configurations wins the most duels, **so
        that** I know what to bring — and what to improve in the editor.
    *   **Acceptance Criteria:**
        *   "My Duels" surfaces the player's most-winning own configuration.

*   **User Story C3: My record survives**
    *   **As a** player who switches devices or clears their browser, **I want** my record back via
        my recovery code, **so that** my bragging rights are not fragile.
    *   **Acceptance Criteria:**
        *   With a valid ADR-0027 identity/recovery code, the server-side duel record is restored.
        *   A guest is gently prompted after a match to claim a name so results "count".

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   QR/room-code pairing of two phones with identity transport (FR-1–FR-5).
        *   Hidden simultaneous choice from own configs (FR-6, FR-7).
        *   Server-authoritative single computation via `biotope_headless` (FR-8, FR-9).
        *   Result screen with WIN/LOSE and Rematch (FR-12, FR-13).
        *   `duels` recording + local-first "My Duels" with head-to-head (FR-16–FR-18, C1).
    *   **Should-Have:**
        *   Deterministic local match animation via WASM core (FR-10).
        *   VS splash and self-hosted soundtrack with build-up/payoff + Suno credit
            (FR-11, FR-14, FR-15).
        *   Server-side record mirroring + recovery via ADR-0027 identity (FR-17, FR-19, C3).
        *   Best-weapon insight (FR-18 best weapon, C2).
        *   Guest fallback + post-match claim prompt (FR-4, FR-20).
    *   **Could-Have:**
        *   Emotes / lightweight trash-talk during VS or result.
        *   Highlight-seed sharing of a finished duel.
        *   Sudden-death / shrinking-field tension mechanic for long matches.
        *   Kiosk big-screen as an optional spectator showcase at the booth.
    *   **Won't-Have (in this increment):**
        *   Remote / asynchronous duels between players who are not co-located.
        *   Tournaments of 3+ players in duel mode.
        *   In-match interaction (the match remains deterministic).
        *   Websocket-based push transport (short polling is used instead).

*   **Dependencies:**
    1.  **Identity:** Pairing, record ownership, and recovery all build on ADR-0027 (`player_id`,
        `nickname`, recovery code). Guest fallback depends on the same identity model.
    2.  **Headless engine:** The authoritative result depends on `build/biotope_headless`
        (`<red.json> <blue.json> [out.json]`) being present and invoked by the backend.
    3.  **WASM core:** Deterministic local replay (Should-Have) depends on the `Makefile.wasm`
        build being usable from the duel page; otherwise a compact frame-payload fallback (per
        ADR-0028 §2.2 / alternatives) is required.
    4.  **Room state:** Hidden choice, lock-in, computation, and rematch all depend on the new
        server-side duel-room lifecycle and short-polling state contract.
    5.  **Audio asset:** The self-hosted Suno track must be downloaded and placed as a static asset
        before audio stories can be verified.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| B-01 | A | Duel-room model + create/join endpoints with idle expiry and full-room rejection | Must |
| B-02 | A | QR code generation + 4-char fallback code; scan/enter join flow | Must |
| B-03 | A | Identity transport (`player_id` + `nickname`) through join; guest fallback | Must |
| B-04 | B | Own-config selection UI on phone (from player's saved configs) | Must |
| B-05 | B | Hidden simultaneous lock-in; reveal only after both lock in (short-polling contract) | Must |
| B-06 | B | Server runs `biotope_headless` once per pairing; store + return authoritative result | Must |
| B-07 | B | Result screen (WIN/LOSE) + Rematch returning to choice within the room | Must |
| B-08 | C | `duels` collection schema + write-on-result | Must |
| B-09 | C | Local-first "My Duels" view with per-opponent head-to-head | Must |
| B-10 | B | Deterministic local match animation via WASM core (verdict stays server-canonical) | Should |
| B-11 | B | VS splash + self-hosted Suno soundtrack (build-up/payoff) + unobtrusive credit link | Should |
| B-12 | C | Server-side record mirroring + retrieval via ADR-0027 identity/recovery | Should |
| B-13 | C | Best-weapon computation + display | Should |
| B-14 | C | Guest post-match claim prompt; associate local guest record on claim | Should |
| B-15 | A/B | Emotes / trash-talk on VS or result screen | Could |
| B-16 | C | Share a finished duel via highlight seed | Could |
| B-17 | B | Sudden-death / shrinking-field tension mechanic for long matches | Could |
| B-18 | A | Kiosk big-screen optional spectator showcase at the booth | Could |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The code is written and formatted according to the guidelines in
    `docs/CODING_STYLE.md`. Python is formatted/linted (`black .`, `ruff check .`); any C changes
    compile with `make` producing **zero warnings**; AI-authored blocks carry the
    `// KI-Agent unterstützt` attribution.
*   **Tests:**
    *   All new backend functions (room lifecycle, duel recording, head-to-head/best-weapon
        aggregation) are covered by unit tests under `backend/tests/`.
    *   The end-to-end duel flow (create room → join → both lock in → compute → record) is verified
        by an integration test that invokes the built C binary.
    *   All existing tests continue to pass (no regressions in editor, submission pool, kiosk, C
        app).
*   **Acceptance Criteria:** All acceptance criteria for the story are met and manually verified on
    a phone-sized viewport (interactive test driven by the developer per the Team Principle).
*   **Code Review:** The code has been reviewed (or is in a reviewable PR state).
*   **Merge:** The code has been successfully merged into the `biotop` branch.
*   **Documentation:** `docs/CHANGELOG.md` is updated; any architectural deltas are reflected back
    into ADR-0028 or a follow-up ADR.
