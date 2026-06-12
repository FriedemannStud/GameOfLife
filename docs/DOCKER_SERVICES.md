# Docker Services – Biotop

Diese Seite beschreibt jeden Docker-Container im Projekt: was er tut, wen er braucht, und wann er läuft.

---

## Überblick

```
                    ┌─────────────────────────────────────┐
                    │         Docker-Netzwerk              │
                    │                                      │
  Browser ──────────┤──► backend (Port 8000) ──────────► mongo
  (Editor/API)      │         FastAPI                   MongoDB
                    │            │                         ▲
                    │            │ schreibt Elo             │
                    │            ▼                         │
                    │       matchmaker ────────────────────┘
                    │   (alle 60 s Turniere)
                    │
                    │       c-dev  (nur lokal)
                    │   (Build-Umgebung für C-Code)
                    └─────────────────────────────────────┘

  build/biotope  ──────────────────────────────────────────►  GUI-Anzeige
  (Raylib, läuft direkt                                       (Kiosk, braucht
  auf dem Host/Laptop)                                         Display/OpenGL)
```

---

## Die vier Services

### `mongo` – Datenbank

| Eigenschaft | Wert |
|-------------|------|
| Image | `mongo:4.4` |
| Port (intern) | `27017` (nur im Docker-Netzwerk) |
| Port (Host) | `127.0.0.1:27018` → nur vom lokalen Rechner erreichbar |
| Daten | Volume `mongo_data` (bleibt erhalten, auch wenn der Container stoppt) |
| Benötigt von | `backend`, `matchmaker` |

**Was er tut:** Speichert alle persistenten Daten des Projekts:
- eingereichte Muster der Spieler (`submissions`)
- Elo-Bewertungen und Spielerprofile (`players`)
- Highlight-Replays vergangener Turniers-Epochen (`epoch_highlights`)

**Abhängigkeiten:** Keine — er startet als Erster. `backend` und `matchmaker` warten per `healthcheck` darauf, dass MongoDB bereit ist, bevor sie starten.

**Wann benötigt:** Immer, wenn die App läuft (lokal und auf dem Server).

---

### `backend` – API-Server

| Eigenschaft | Wert |
|-------------|------|
| Image | `python:3.11-slim` |
| Port | `8000` (nach außen offen) |
| Volumes | `./backend` → `/app`, `./build` → `/app/build`, `./web` → `/app/web` |
| Startet nach | `mongo` (wartet auf `service_healthy`) |

**Was er tut:**
- Stellt die REST-API bereit: `POST /api/v1/submit_config`, `GET /api/leaderboard`, etc.
- Liefert den Web-Editor als statische Datei aus (`/editor/editor.html`)
- Verbindet sich mit MongoDB für alle Lese- und Schreiboperationen

**Warum `./build` gemountet?**  
Das Backend-Volume `./build:/app/build` ist ein Überbleibsel für mögliche direkte Binary-Aufrufe und Integration-Tests (z. B. `backend/tests/test_system_integration.py`), die `./build/biotope_hyper_worker` direkt aufrufen.

**Wann benötigt:** Immer — er ist das Herzstück des Mehrspielermodus.

---

### `matchmaker` – Turnier-Worker

| Eigenschaft | Wert |
|-------------|------|
| Image | `python:3.11-slim` |
| Volumes | `./backend` → `/app`, `./worker_bin` → `/app/build` |
| Startet nach | `mongo` (wartet auf `service_healthy`) |
| Laufzeit | Endlosschleife, alle 60 Sekunden eine Turnier-Epoche |

**Was er tut:**
1. Holt alle aktiven Spieler-Submissions aus MongoDB
2. Serialisiert sie in eine temporäre JSON-Datei
3. Ruft `/app/build/biotope_hyper_worker` auf (das C-Programm für die Simulation)
4. Liest die Ergebnisse und aktualisiert Elo-Ratings in MongoDB

**Warum `./worker_bin` statt `./build`?**  
`build/` wird vom `c-dev`-Container als `root` angelegt. Wenn Docker den Ordner vor `make` anlegt, kann der User danach nicht mehr hineinschreiben. `worker_bin/` wird dagegen immer mit dem richtigen Besitzer (`fried`) angelegt — entweder via `git clone` (durch `.gitkeep`) oder via `make`.  
Sobald `make build/biotope_hyper_worker` ausgeführt wird, kopiert der Makefile die Binary automatisch nach `worker_bin/`.

**Wann benötigt:** Nur im Mehrspielermodus (lokal zum Testen oder auf dem Server).

---

### `c-dev` – Build-Container

| Eigenschaft | Wert |
|-------------|------|
| Image | Aus lokalem `Dockerfile` gebaut |
| Volumes | `.` → `/app` (komplettes Repo) |
| Port | keiner |
| Display | `DISPLAY=host.docker.internal:0` (X11-Forwarding) |

**Was er tut:**  
Stellt eine vollständige C-Entwicklungsumgebung bereit (gcc, make, Raylib, Emscripten), ohne dass du diese Tools lokal installieren musst. Du kannst in ihm interaktiv arbeiten:

```bash
docker compose exec c-dev bash
# Jetzt bist du "im" Container:
make
./build/biotope
```

**Wann benötigt:** Nur lokal beim Entwickeln, wenn du keine Raylib-Installation auf dem Host hast. Auf dem Server macht er nichts und kann ignoriert werden.

---

## Abhängigkeiten auf einen Blick

```
mongo ◄─── backend    (backend startet erst, wenn mongo healthy ist)
mongo ◄─── matchmaker (matchmaker startet erst, wenn mongo healthy ist)

backend    ──► ./build/         (liest Binaries für Integrationstests)
matchmaker ──► ./worker_bin/    (liest biotope_hyper_worker für Simulationen)
c-dev      ──► ./               (liest + schreibt das ganze Repo)
```

---

## Einsatzszenarien

| Szenario | mongo | backend | matchmaker | c-dev |
|----------|-------|---------|------------|-------|
| Lokale Entwicklung (Simulation testen) | ✓ | ✓ | ✓ | optional |
| Server-Deployment (Uni-Kiosk) | ✓ | ✓ | ✓ | — |
| Nur C-Code bauen (kein Backend nötig) | — | — | — | ✓ |
| Editor im Browser ausprobieren | ✓ | ✓ | — | — |

---

## Wichtige Reihenfolge beim Start

```bash
# 1. Binaries bauen — IMMER zuerst
make build/biotope_hyper_worker   # Server (kein Raylib nötig)
make                               # Lokal (mit Raylib, baut alles)

# 2. Container starten — DANACH
docker compose up -d
```

**Warum diese Reihenfolge?**  
Startet man `docker compose up` vor `make`, legt Docker den Ordner `worker_bin/` als `root` an (falls er nicht schon vorhanden ist), und `make` kann danach nicht mehr hineinschreiben. Die `.gitkeep`-Datei im Repo stellt sicher, dass `worker_bin/` nach `git clone` bereits als User-eigener Ordner existiert — damit ist diese Reihenfolge robust, aber es schadet nicht, sie trotzdem einzuhalten.

---

## Nützliche Befehle

```bash
docker compose ps                   # Status aller Container
docker compose logs -f matchmaker   # Live-Logs des Turnier-Workers
docker compose logs -f backend      # Live-Logs des API-Servers
docker compose restart matchmaker   # Einzelnen Service neu starten
docker compose down                 # Alles stoppen (Daten bleiben erhalten)
docker compose down -v              # Alles stoppen + Datenbank löschen
```
