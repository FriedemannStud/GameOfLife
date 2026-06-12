# Demo-Leitfaden: Biotop vom Laptop vorführen

**Szenario:** Der Server läuft bereits an der Uni. Du hast nur deinen Laptop dabei und möchtest die App im Meeting mit dem Professor vorführen.

> Server-IP: **`10.99.0.180`**

---

## Voraussetzungen (einmalig prüfen, am besten vor dem Meeting)

- [ ] SSH-Zugang zum Uni-Server funktioniert (`ssh <user>@10.99.0.180`)
- [ ] Raylib ist auf deinem Laptop installiert (`make` funktioniert lokal)
- [ ] Das lokale Projekt ist aktuell (`git pull`)
- [ ] Die lokale GUI baut ohne Fehler (`make` → kein Warning, kein Fehler)

---

## Schritt 1 – SSH-Tunnel öffnen (Terminal A)

```bash
ssh -L 8000:localhost:8000 <user>@10.99.0.180
```

Dieses Terminal **offen lassen**. Solange es läuft, ist `localhost:8000` auf deinem Laptop direkt mit dem Backend auf dem Server verbunden.

> Wenn du bereits SSH-Keys hinterlegt hast, brauchst du kein Passwort einzugeben.

---

## Schritt 2 – Backend-Status prüfen (im selben Terminal A oder Terminal B)

```bash
curl http://localhost:8000/
# Erwartete Ausgabe: {"message":"Hello Biotope"}

curl http://localhost:8000/api/leaderboard
# Erwartete Ausgabe: JSON-Liste (ggf. [] wenn noch keine Submissions)
```

Wenn das funktioniert, ist der Tunnel aktiv und das Backend läuft.

---

## Schritt 3 – Lokale GUI starten (Terminal B)

```bash
cd ~/dev/GameOfLife   # oder wo dein Projektordner liegt

make                  # baut die GUI (biotope, biotope_headless, biotope_hyper_worker)

./build/biotope       # startet den Kiosk-Modus
```

Die GUI holt ihre Daten von `http://localhost:8000` – das geht automatisch durch den Tunnel zum echten Server.

---

## Schritt 4 – Web-Editor im Browser öffnen

```
http://localhost:8000/editor/editor.html
```

Dort können Muster eingezeichnet und direkt zum Server submitted werden.

---

## Was du im Meeting zeigen kannst

| Was | Wo |
|---|---|
| Kiosk-Modus (4 Live-Matches + Leaderboard) | GUI läuft lokal (Schritt 3) |
| Web-Editor (Muster zeichnen & einreichen) | Browser (Schritt 4) |
| Leaderboard / API | `http://localhost:8000/api/leaderboard` im Browser |
| Backend-Logs (live) | `docker compose logs -f backend` im SSH-Terminal |

---

## Schnellstart-Checkliste (für kurz vor dem Meeting)

```
[ ] Terminal A: ssh -L 8000:localhost:8000 <user>@10.99.0.180   → offen lassen
[ ] curl http://localhost:8000/ → gibt {"message":"Hello Biotope"} zurück
[ ] Terminal B: cd ~/dev/GameOfLife && ./build/biotope            → GUI startet
[ ] Browser: http://localhost:8000/editor/editor.html             → Editor lädt
```

---

## Fehlerbehandlung

**`curl` gibt "Connection refused"**
→ Entweder der SSH-Tunnel ist nicht aktiv (Terminal A prüfen) oder der Docker-Stack läuft nicht. Im SSH-Terminal prüfen:
```bash
docker compose ps     # alle Container sollten "running" zeigen
docker compose up -d  # startet den Stack falls nötig
```

**GUI startet nicht / `make` schlägt fehl**
→ Raylib fehlt lokal. Installieren:
```bash
sudo apt install libraylib-dev
# Falls nicht verfügbar: siehe docs/START_ROUTINE.md → "Linux Prerequisites"
```

**Port 8000 wird lokal schon verwendet**
→ Tunnel auf einen anderen lokalen Port legen:
```bash
ssh -L 9000:localhost:8000 <user>@10.99.0.180
```
Dann im Browser und bei `curl` Port `9000` verwenden. Die GUI selbst nimmt immer `localhost:8000`, daher in diesem Fall zurück zum ursprünglichen Port wechseln oder die GUI neu kompilieren (nicht empfohlen kurz vor dem Meeting).
