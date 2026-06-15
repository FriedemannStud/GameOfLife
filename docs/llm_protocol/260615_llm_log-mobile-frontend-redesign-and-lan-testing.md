# LLM Session Log — Mobile Frontend Redesign & LAN Testing

**Date:** 2026-06-15
**Branch:** `biotop`
**Scope:** (1) Visual redesign of the two mobile web frontends to match the kiosk
look & feel; (2) enabling on-device testing from an iPhone over the home LAN with
the backend running inside WSL2.

---

## 1. Goal

The project has **three frontends**:

- **A) Kiosk & Simulation** — the native C/Raylib app (the "Digital Lab" look,
  runs on the exhibition monitor).
- **B) `web/editor/editor.html`** — mobile 8×8 species editor.
- **C) `web/duel/duel.html`** — mobile 1-vs-1 Shoulder-Duel (ADR-0028).

**Task:** Bring B) and C) up to the visual quality of A) — *Modern Retro, Dark
Mode, neon accents* at indie-game polish (benchmark: gamescom / Steam). All
existing features must keep working; only the design layer changes.

---

## 2. Design Source of Truth (Frontend A)

The kiosk palette was read directly from `src/gui/renderer.c` (the `THEME_*`
colors) and reused so the web pages match the monitor exactly:

| Role | Kiosk value (`renderer.c`) | Web hex |
|------|----------------------------|---------|
| Background | `{20,24,32}` | `#141820` (page base set to mandated `#1a1a1a`) |
| Neon Red / Team Red | `THEME_RED {255,60,100}` | `#ff3c64` |
| Neon Cyan / Team Blue | `THEME_BLUE {0,220,255}` | `#00dcff` |
| Text | `THEME_TEXT {220,220,220}` | `#e7eaf0` |
| Muted / Hint | `THEME_HINT {130,140,160}` | `#8b97a8` |
| Display surface (wells) | `THEME_BG` | `#0d0f13` |
| Reward accent (codes / win) | — (new) | `#ffb84d` amber |

Other A) cues that were ported: uppercase technical labels, monospace
zero-padded counters, `[KEY]`-style hints, faint HUD separators, semi-transparent
panels.

---

## 3. What Was Changed (Implementation)

Both files received a full CSS rewrite sharing **one identical design-token block**
(`:root`). Markup IDs, classes toggled by JS, and all behavior were preserved.

### Shared "Digital Lab / Modern-Retro" system
- Layered background: cyan/red corner glows + faint lab grid + subtle CRT
  scanline veil (`body::after`).
- HUD bracket corners on panels (via `::before/::after`).
- Typography: `Orbitron` (titles), `Rajdhani` (UI), `Share Tech Mono` (codes/
  counters), loaded from Google Fonts **with system-font fallbacks** (offline-safe).
- Neon cell bloom (`box-shadow`), ghost/neon buttons, gradient cyan CTA.

### B) `editor.html`
- Console panel with bracket corners, HUD stat readouts (`Zellen 0/24` cyan,
  `Gen` accent), neon-cyan live cells.
- Recovery card restyled as a "legendary drop" (amber on dark glass).
- **PNG export colors updated** (were hardcoded `#004a99/#ffcc00`) to dark/amber
  so the downloaded recovery card matches the on-screen card.

### C) `duel.html`
- Team Red/Blue now use the exact kiosk neon colors.
- Proportional population meter with glow, pulsing `YOU WIN` verdict, amber room
  code with glow, terminal-style 4-char code input.
- **Replay-canvas colors extracted into constants** `REPLAY_BG/RED/BLUE`
  (`#0d0f13 / #ff3c64 / #00dcff`) so the on-phone mini-simulation looks identical
  to the monitor. QR box kept white for scannability.

### Preserved (no functional change)
- All element IDs / JS hooks (`btn-play`/`btn-stop` toggling, `recovery-group`
  display logic, screen-state machine, polling, audio, localStorage identity).
- Backend endpoints and serving paths untouched.

### Verification performed
- `grep` for leftover old tokens (`#004a99`, `#ffcc00`, `#00f2ff`, `#e74c3c`,
  `#3498db`, `wiai`) → none remain.
- Python check that every `var(--x)` referenced is defined in `:root` → both files
  pass.
- Developer confirmed the redesign looks good on PC. (On-device iPhone interactive
  test still pending — see §4/§5.)

---

## 4. Serving Topology (relevant for testing)

From `docker-compose.yml` and `backend/app/main.py`:

- Backend = FastAPI/uvicorn on **port 8000**, `--host 0.0.0.0`, port mapping
  `8000:8000`.
- Static mounts: `/editor`, `/duel`, `/assets`, `/vendor`.
- Frontend URLs:
  - `http://<HOST>:8000/editor/editor.html`
  - `http://<HOST>:8000/duel/duel.html`
- The frontends call the backend **same-origin** (`/api/v1/...`), so the page must
  be loaded via a host/IP that the device can actually reach.

---

## 5. LAN Testing from iPhone (WSL2) — Problem & Solution

### Problem
The codebase runs in **WSL2** (NAT networking — confirmed via `resolv.conf`
nameserver `10.255.255.254`). WSL2 sits behind a hidden virtual network, so a LAN
device (iPhone) sees only the **Windows host**, not the WSL VM. Binding uvicorn to
`0.0.0.0` inside WSL is necessary but **not sufficient**.

### Environment facts gathered
- WSL IP (eth0): **`172.30.84.55`** (⚠️ changes on every reboot).
- Backend confirmed listening on `*:8000`.
- The PC is connected by **Ethernet cable**; its **Wi-Fi adapter is disconnected**
  (`Medium getrennt`). iPhone joins via Wi-Fi → both share the router/subnet.

### Solution: Windows portproxy + firewall (classic NAT mode)
Run in an **Administrator PowerShell**, each command on a **single line** (pasting
wrapped multi-line commands split them and caused failures earlier):

```powershell
# 1. IP Helper service must run, or netsh portproxy silently no-ops
Start-Service iphlpsvc
Set-Service iphlpsvc -StartupType Automatic

# 2. Forward Windows:8000 -> WSL:8000  (use the current WSL IP!)
netsh interface portproxy add v4tov4 listenaddress=0.0.0.0 listenport=8000 connectaddress=172.30.84.55 connectport=8000

# 3. Firewall: allow inbound TCP 8000 (scoped to the port, not all TCP)
New-NetFirewallRule -DisplayName "WSL Biotop 8000" -Direction Inbound -Protocol TCP -LocalPort 8000 -Action Allow

# 4. Verify
netsh interface portproxy show v4tov4
```

Confirmed working result:
```
0.0.0.0   8000   ->   172.30.84.55   8000     (EXIT=0)
```

### Pitfalls hit during the session
- **Multi-line paste** broke commands → run each as one line.
- First firewall rule was created **without `-LocalPort`** (too broad). Fixed via
  `Remove-NetFirewallRule -DisplayName "WSL Biotop 8000"` then re-create with
  `-LocalPort 8000`.
- `portproxy show` was **empty** because the `add` step was skipped / the IP Helper
  service angle; resolved once `add` was actually run (EXIT=0).
- **Use the Ethernet adapter IPv4** (`192.168.x.x` / `10.x.x.x`) for the iPhone —
  **not** `172.30.x.x` (WSL internal) and **not** `169.254.x.x` (no DHCP).
  Quick lookup: `ipconfig | findstr /C:"IPv4"`.

### Connect from iPhone (Safari)
```
http://<PC-ETHERNET-IP>:8000/editor/editor.html
http://<PC-ETHERNET-IP>:8000/duel/duel.html
```

### Cleaner alternative (Windows 11): mirrored networking
`%USERPROFILE%\.wslconfig`:
```ini
[wsl2]
networkingMode=mirrored
```
Then `wsl --shutdown` + restart. WSL then shares the host IP → no portproxy and no
per-reboot maintenance (firewall rule may still be needed).

> Full step-by-step lives in **`docs/MOBILE_LAN_TESTING.md`**.

---

## 6. Open / Next Steps

- [ ] Developer to find the Ethernet IPv4 (`ipconfig | findstr /C:"IPv4"`) and load
      the pages on the iPhone.
- [ ] Interactive on-device test (developer-run, per team principle):
  - Editor: draw → Simulieren → submit; check neon cells, recovery card.
  - Duel: Start Duel → QR/code → join from a 2nd device → VS replay; check
    red/blue match the monitor, pulsing `YOU WIN`.
- [ ] Remember: after any WSL/PC reboot, the WSL IP changes →
      `netsh interface portproxy reset` and re-add (or use mirrored mode).
- [ ] Optional: add an ADR / CHANGELOG entry for the mobile redesign per project
      doc conventions.

---

## 7. Touched / Added Files

- `web/editor/editor.html` — redesigned (CSS + PNG-export colors).
- `web/duel/duel.html` — redesigned (CSS + replay-canvas color constants).
- `docs/MOBILE_LAN_TESTING.md` — new standalone connection guide.
- `docs/llm_protocol/260615_llm_log-mobile-frontend-redesign-and-lan-testing.md` —
  this log.
