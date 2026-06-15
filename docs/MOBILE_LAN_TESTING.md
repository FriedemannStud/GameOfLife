# Mobile / LAN Testing Guide (iPhone ↔ WSL2 Backend)

How to open the **editor** and **duel** web frontends on a phone (iPhone/Android)
over your home network, when the backend runs inside **WSL2** on a Windows PC.

> Why this is needed: WSL2 uses NAT networking, so it lives behind a hidden
> virtual network. A phone on the Wi-Fi can only reach the **Windows host**, not
> the WSL VM directly. We therefore forward a Windows port into WSL.

---

## 0. Prerequisites

- Phone and PC are on the **same router** (same LAN/Wi-Fi). The PC may be wired
  (Ethernet) and the phone on Wi-Fi — that's fine as long as it's the same router.
- The router does **not** have *client/AP isolation* enabled (common on guest
  Wi-Fi). Use the normal network if unsure.
- The backend stack is running:
  ```bash
  docker-compose up
  ```
  It serves on port **8000** at `/editor/editor.html` and `/duel/duel.html`.

---

## 1. Get the WSL IP (inside WSL/Ubuntu)

```bash
hostname -I
```
Note the first address, e.g. `172.30.84.55`.

> ⚠️ **This IP changes after every reboot of Windows/WSL.** If testing stops
> working later, this is the first thing to re-check (see §6).

---

## 2. Set up the port forward (Windows, Admin PowerShell)

Open **PowerShell as Administrator** (Start → type "PowerShell" → right-click →
*Run as administrator*).

> 📋 Paste each command on a **single line**. Multi-line pastes get split by
> PowerShell and fail.

```powershell
# Ensure the IP Helper service runs (portproxy silently no-ops without it)
Start-Service iphlpsvc
Set-Service iphlpsvc -StartupType Automatic
```

```powershell
# Forward Windows:8000 -> WSL:8000  (replace the IP with YOUR `hostname -I`)
netsh interface portproxy add v4tov4 listenaddress=0.0.0.0 listenport=8000 connectaddress=172.30.84.55 connectport=8000
```

```powershell
# Allow inbound TCP on port 8000 through the Windows firewall
New-NetFirewallRule -DisplayName "WSL Biotop 8000" -Direction Inbound -Protocol TCP -LocalPort 8000 -Action Allow
```

Verify the forward exists:
```powershell
netsh interface portproxy show v4tov4
```
Expected:
```
Adresse abhören  Port   Adresse verbinden  Port
---------------  -----  -----------------  -----
0.0.0.0          8000   172.30.84.55       8000
```

---

## 3. Find the PC's LAN IP (Windows PowerShell)

```powershell
ipconfig | findstr /C:"IPv4"
```

Pick the right one:

| Address | Meaning | Use for phone? |
|---------|---------|----------------|
| `192.168.x.x` or `10.x.x.x` | Your real LAN adapter (Ethernet or Wi-Fi) | ✅ **Yes** |
| `172.30.x.x` | WSL virtual bridge | ❌ No |
| `169.254.x.x` | No DHCP / not connected | ❌ No |

If the PC is wired, use the **Ethernet** adapter's `192.168.x.x` / `10.x.x.x`.

---

## 4. Open on the phone

In the phone browser (Safari/Chrome), using the PC's LAN IP from §3:
```
http://192.168.x.x:8000/editor/editor.html
http://192.168.x.x:8000/duel/duel.html
```

> 🔗 Always load via the **PC's LAN IP**, never `localhost`. The duel QR code
> encodes the page's origin, so a wrong origin would make the join link
> unreachable for the second device.

**Quick sanity check from the PC first:** open `http://192.168.x.x:8000/editor/editor.html`
(via the LAN IP, not localhost) in the PC browser. If that works, the phone almost
certainly will too.

---

## 5. What works over plain HTTP

No HTTPS is required for these features:
- ✅ Editor: draw, simulate, submit, recovery-code card + PNG download.
- ✅ Duel: create/join room, 4-char code, QR (scanned by the phone's **camera
  app**, which opens the URL — no in-page camera permission needed), VS replay,
  audio (after the first tap), `localStorage` identity & history.
- ℹ️ Google Fonts need the phone to have internet (normal on Wi-Fi). Without it,
  the pages fall back to system fonts and still work.

---

## 6. After a reboot (IP changed) / cleanup

The WSL IP usually changes on reboot, breaking the forward. Re-do it:

```powershell
# Remove all forwards
netsh interface portproxy reset
```
Then get the new WSL IP (`hostname -I` in WSL) and re-run the `add` command from §2.

Inspect / remove the firewall rule if needed:
```powershell
Get-NetFirewallRule -DisplayName "WSL Biotop 8000"
Remove-NetFirewallRule -DisplayName "WSL Biotop 8000"
```

---

## 7. Cleaner alternative — Mirrored networking (Windows 11)

Avoids the portproxy and the per-reboot IP maintenance entirely.

Create/edit `%USERPROFILE%\.wslconfig`:
```ini
[wsl2]
networkingMode=mirrored
```
Then restart WSL:
```powershell
wsl --shutdown
```
After restart, WSL shares the host's IP. The phone can reach
`http://<PC-LAN-IP>:8000/...` directly (you may still need the firewall rule
from §2).

---

## 8. Troubleshooting checklist

- `portproxy show` empty → IP Helper service not running, or `add` was skipped.
- Phone can't connect → wrong IP (used `172.30.*`), different network, or router
  client-isolation.
- Worked yesterday, not today → WSL IP changed after reboot (§6).
- PC LAN IP works but phone doesn't → firewall rule missing or scoped to the wrong
  network profile (the rule above uses the default `Any` profile).
