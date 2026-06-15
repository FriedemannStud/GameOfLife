# LLM Log — Duel Audio Fix & Mission Statement Page

**Date:** 2026-06-15
**Branch:** `biotop`
**Scope:** Frontend polish on the Shoulder-Duel page + new "Mission Statement" rules page.
**Files touched:**
- `web/duel/duel.html` (audio behaviour)
- `web/mission/mission.html` (new page)
- `web/landing/index.html` (new menu card)
- `backend/app/main.py` (new static mount)

---

## 1. Bug: soundtrack blips on the first tap (mobile)

### Symptom
- Desktop browser: music starts exactly as intended — only after the 3-2-1 countdown ("FIGHT").
- Smartphone: music also (correctly) restarts after the countdown, **but additionally** plays briefly already when "Start Duel" is tapped.

### Root cause (evidence-based, from the codebase)
The duel page uses the classic mobile autoplay-unlock trick. A global listener fires on the
very first `pointerdown` anywhere on the page (which is normally the "Start Duel" tap):

- `document.addEventListener('pointerdown', unlockAudio)`
- `unlockAudio()` calls `duelAudio.play()` and only pauses **inside the resolved promise**
  (`.then(() => { duelAudio.pause(); ... })`).

On desktop the `play()` promise resolves almost instantly, so the pause lands within a few ms
and the blip is inaudible. On mobile the resolve latency (audio pipeline / decoding / power
states) is large enough that the first fraction of the track is audible before the pause runs.
The intended start (`playBuildup()`) is a separate, later call after the countdown — hence on
mobile you hear *both*.

### Fix
Mute the element during the unlock, restore afterwards. Chosen over a `volume = 0` approach
because it does not entangle with the existing volume state machine
(`playBuildup` → `0.55`, `playPayoff` → `0.9`).

- `duelAudio.muted = true` **before** `play()`.
- `duelAudio.muted = false` in both the `.then()` (success) and the `.catch()` (gesture not yet
  sufficient) so the track can never get stuck silent.

**Status:** Implemented and verified by the developer on a smartphone (no blip on "Start Duel";
intended start after countdown unchanged).

---

## 2. Feature: fade music out when leaving the result screen

### Request
After a finished fight, pressing **REMATCH**, **MY DUELS**, **NEW DUEL** or **Back to Lab**
should fade the soundtrack out instead of stopping it abruptly.

### Implementation (`web/duel/duel.html`)
- New helper `fadeOutAudio(onDone)`:
  - ~600 ms ramp (24 steps × 25 ms) lowering `volume` to 0, then `stopAudio()`
    (pause + `currentTime = 0`), then **restore the original volume** so the next
    `playBuildup`/`playPayoff` starts clean.
  - If nothing is playing (`duelAudio.paused`), it stops immediately and still calls `onDone`.
  - Guards against overlapping fades via a module-level `fadeTimer`.
- Wiring:
  - **NEW DUEL** → `leaveToHome()` now calls `fadeOutAudio()` (was the hard `stopAudio()`).
  - **REMATCH** → `fadeOutAudio()` right after the double-click guard.
  - **MY DUELS** → `fadeOutAudio()` in `openMyDuels()`.
  - **Back to Lab** → link given `id="link-lab"`; a click handler fades, then navigates
    (`window.location.href = href`). If nothing is playing it lets the link behave normally.
- The mute toggle button intentionally keeps the immediate `stopAudio()` (a manual "Sound: off"
  should cut instantly).

**Status:** Implemented; developer verification pending (steps below).

---

## 3. Feature: new "Mission Statement" page (rules of Biotop)

### Request
A standalone page presenting the rules of Biotop, linked from `/` (landing) below "Shoulder
Duel", styled like `editor.html` / `duel.html` and matching `index.html`
(gamescom / Steam indie modern-retro mini-game look).

### Rules sourced from the codebase (authoritative)
- `src/core/game_logic.c:171-176` — survival & team-birth logic:
  - Living cell survives with **2 or 3** neighbours (colour-agnostic count), keeps its colour.
  - Dead cell with **exactly 3** neighbours is born in the **majority colour** of those three
    (`red_neighbors > blue_neighbors ? RED : BLUE`). With 3 neighbours a tie is impossible.
- `src/apps/headless/main_headless.c:69` — match runs **100 generations**.
- `run_isolated_match` (game_logic.c) — board is two 8×8 halves; **Red left, Blue right**.
- Editor / duel constraints — 8×8 genome, **max 24 living cells** (`MAX_BIOMASS`).
- Winner: more living cells after 100 generations; equal population = draw; deterministic.

### New page `web/mission/mission.html`
Self-contained HTML + CSS + JS, reusing the shared "Digital Lab" theme (palette, fonts,
scanline veil, HUD-bracket panels). Sections:
1. **Hero** — "Mission Statement / Die Regeln des Biotops" + lede.
2. **Das Ziel** — objective (8×8 genome, ≤24 cells, Red/Blue sides, 100 generations).
3. **Das Spielfeld** — two sample 8×8 genomes as a VS legend.
4. **Die Evolution** — 3 rule cards with animated 3×3 mini-diagrams (survive / death / birth),
   amber-ringed centre cell; the "birth" cell pulses.
5. **Sieg & Niederlage** — more cells wins, tie = draw, deterministic note.
6. **CTA** — buttons to editor + duel; footer "« Zurück zum Lab".
- Includes the same ambient red-vs-blue Conway backdrop as the landing page for cohesion.
- Page language: German (matches the landing page).

### Routing (`backend/app/main.py`)
- Added `app.mount("/mission", StaticFiles(directory="web/mission"), name="mission")`
  (each page has its own mount; there is no catch-all `web/` mount).
- Page reachable at `/mission/mission.html`.

### Landing card (`web/landing/index.html`)
- New third menu card **"Mission Statement" (tag: Codex, amber accent)** below the Shoulder-Duel
  card, with a live red-vs-blue mini-preview grid (`preview-codex`).
- Added `.card-codex` CSS (amber border/glow), a new `.reveal.d6` stagger step; footer bumped to `d6`.
- Registered the new preview in the existing `cardPreviews()` IIFE.

**Status:** Implemented; developer verification pending (steps below).

---

## 4. Verification checklist (interactive, run by the developer)

### Audio fade (section 2)
1. Play a match to the result screen (music playing).
2. In separate runs press REMATCH, MY DUELS, NEW DUEL, Back to Lab.
3. Expect a ~0.6 s fade-out (not an abrupt cut); Back to Lab navigates after the fade.
4. Rematch: music restarts normally in the next fight after the countdown.

### Mission Statement (section 3)
1. Start backend, open `/` → third amber card "Mission Statement" under Shoulder Duel; mini-grid lives.
2. Click it → `/mission/mission.html` loads; Conway backdrop runs; the three rule diagrams render
   with red/blue cells and an amber centre; the "birth" cell pulses.
3. Buttons "Spezies bauen", "Ab ins Duell", "« Zurück zum Lab" navigate correctly.
4. Re-check layout on a smartphone (optimised for ≤460 px).

---

## 5. Notes for continuation
- Commits are done by the developer (per working agreement); none of the above is committed yet
  at the time of this log.
- No ADR was created — these are UI/polish + a static content page. If documentation rigor is
  desired, a short note in `docs/CHANGELOG.md` would suffice.
- Possible follow-ups: localise the duel page wording (currently English) vs. the German mission
  page; consider linking the Mission Statement from within the duel page footer too.
