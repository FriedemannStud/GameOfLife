# Deployment – Biotop auf einem Server starten

Diese Anleitung beschreibt, wie die komplette Biotop-Anwendung auf einem Server
(z. B. einer VM, Zugriff per SSH) in Betrieb genommen wird.

## Architektur in einem Satz

Auf dem Server laufen **vier Dienste in Docker** (MongoDB, Backend, Editor, Matchmaker).
Die **grafische Anzeige** ist ein separates C-Programm, das einen Bildschirm braucht
(siehe [Teil 7](#teil-7--die-grafische-anzeige-simulation--leaderboard)).

```
   Browser (Editor) ─┐
                     │  Port 8000
   GUI-Anzeige ──────┤────────────►  [ Backend FastAPI ] ──► [ MongoDB ]
   (build/biotope)   │                      ▲
                     │                      │ schreibt Elo
                     └──────────►   [ Matchmaker-Worker ]
                                    (ruft build/biotope_hyper_worker alle 60s)
```

| Komponente | Was es ist | Display nötig? |
|---|---|---|
| **MongoDB** | `mongo`-Container (docker-compose) | nein |
| **Backend (FastAPI)** | Port 8000, liefert API **und** den Editor aus | nein |
| **Matchmaker** | Worker-Container, ruft alle 60 s `build/biotope_hyper_worker` auf (Elo) | nein |
| **HTML-Editor** | Browser → `http://SERVER:8000/editor/editor.html` (nutzt relative `/api/...`-URLs) | nein |
| **Simulations-Anzeige + Leaderboard** | das C-Raylib-Programm `build/biotope` (startet direkt im Kiosk-Modus), holt Daten von hardcodiert `http://localhost:8000` | **JA – braucht OpenGL/X11-Display** |

> Der Server-Stack (Mongo, Backend, Editor, Matchmaker) läuft komplett **headless** –
> ohne Raylib und ohne Display. Nur die grafische Anzeige braucht einen Bildschirm.

---

## Teil 1 – Voraussetzungen auf dem Server (einmalig)

```bash
# Auf dem Server (per SSH eingeloggt):
sudo apt update
sudo apt install -y docker.io docker-compose-plugin git build-essential libcurl4-openssl-dev
sudo usermod -aG docker $USER   # danach einmal aus- und wieder einloggen
```

`build-essential` (= gcc + make) und `libcurl4-openssl-dev` werden gebraucht, um die
C-Binaries zu bauen. **Kein** Raylib nötig für den Server-Stack.

---

## Teil 2 – `.env` anlegen

Die `.env` enthält das MongoDB-Passwort und ist absichtlich nicht in Git. Erzeuge sie
aus der Vorlage:

```bash
cd ~/GameOfLife          # in dein geklontes Projektverzeichnis
cp .env.example .env
nano .env
```

In der `.env` setzt du **ein eigenes Passwort** – und zwar an **zwei Stellen identisch**:

```env
MONGO_USER=biotope_admin
MONGO_PASSWORD=DeinStarkesPasswort123
MONGODB_URI=mongodb://biotope_admin:DeinStarkesPasswort123@mongo:27017/biotope_db?authSource=admin
MONGODB_DB=biotope_db
```

> ⚠️ Wichtig: Das Passwort in `MONGO_PASSWORD` **und** in der `MONGODB_URI` muss exakt
> gleich sein, sonst kommt das Backend nicht an die DB. Der Hostname `mongo` ist der
> Docker-Service-Name – nicht ändern.

---

## Teil 3 – C-Binaries bauen (für den Matchmaker zwingend)

Der `build/`-Ordner ist gitignored, ist nach `git pull` also leer. Der Matchmaker bricht
ohne seine Binary ab. Bauen:

```bash
cd ~/GameOfLife
make build/biotope_hyper_worker
make build/biotope_headless
```

Der erste Befehl kopiert `biotope_hyper_worker` automatisch auch nach `worker_bin/`
(wird vom Matchmaker-Container verwendet). Prüfen:

```bash
ls -l build/biotope_hyper_worker worker_bin/biotope_hyper_worker
```

> `make` (ohne Target) würde auch die GUI bauen und **scheitert ohne Raylib**.
> Für den Server reichen die zwei obigen Targets.

---

## Teil 4 – Dienste starten

```bash
cd ~/GameOfLife
docker compose up -d
```

Das startet:
- **mongo** – Datenbank
- **backend** – FastAPI auf Port 8000 (liefert API + Editor)
- **matchmaker** – Worker, der alle 60 s die Turniere rechnet

> Der `c-dev`-Container ist nur zum lokalen GUI-Bauen gedacht – auf dem Server brauchst du
> ihn nicht. Falls `docker compose up -d` ihn mitstartet, ist das egal (er macht nichts
> außer rumstehen).

Logs ansehen (mit `Strg+C` wieder raus):

```bash
docker compose logs -f backend
docker compose logs -f matchmaker
```

Beim Backend solltest du `Successfully connected to MongoDB` sehen, beim Matchmaker alle
60 s eine „Epoch finished“-Zeile (sobald Patterns eingereicht wurden).

---

## Teil 5 – Funktioniert es? (Verifizieren)

Direkt auf dem Server:

```bash
curl http://localhost:8000/                 # -> {"message":"Hello Biotope"}
curl http://localhost:8000/api/leaderboard  # -> JSON-Liste (anfangs leer [])
```

---

## Teil 6 – Den HTML-Editor öffnen

Der Editor wird vom Backend selbst ausgeliefert und nutzt **relative** API-URLs – er
funktioniert also über jede Adresse, unter der Port 8000 erreichbar ist.

Im Browser (von deinem Laptop):

```
http://<SERVER-IP>:8000/editor/editor.html
```

> Falls die Seite nicht lädt: In der Firewall/Security-Group des Servers muss **Port 8000**
> offen sein. Alternativ ohne Firewall-Öffnung per SSH-Tunnel (siehe Teil 7, Weg C).

---

## Teil 7 – Die grafische Anzeige (Simulation + Leaderboard)

Das ist `build/biotope`. Es startet direkt im **Kiosk-Modus** (2×2-Live-Matches +
Leaderboard) und holt seine Daten **fest von `http://localhost:8000`**. Es braucht
zwingend einen Bildschirm/OpenGL. Drei Wege:

### Weg A – Monitor direkt am Server
Nur sinnvoll, wenn die VM ein echtes Display/HDMI hat (Ausstellungs-Kiosk). Cloud-VMs
haben das praktisch nie. Dann: Raylib installieren, `make`, `./build/biotope` lokal auf
dem Server-Display starten.

### Weg B – X11 über SSH (`ssh -X`)
GUI rendert auf deinem Laptop. Braucht einen X-Server auf deiner Seite (unter WSL: WSLg
oder VcXsrv). OpenGL über X11-Forwarding ist oft langsam und zickig → für eine flüssige
Animation nicht ideal.

### Weg C – GUI lokal + SSH-Tunnel ⭐ (empfohlen)
Der Server liefert nur die **Daten**; die GUI baust und startest du auf **deinem eigenen
Rechner** (der hat ja einen Bildschirm). Weil die GUI fest `localhost:8000` anspricht,
baust du einen Tunnel – dann landet „localhost:8000“ deines Rechners automatisch beim
Server:

```bash
# Auf deinem lokalen Rechner:
ssh -L 8000:localhost:8000 <user>@<SERVER-IP>
```

Dieses Terminal offen lassen. In einem zweiten lokalen Terminal (im lokalen Projekt-Klon,
wo Raylib installiert ist):

```bash
make            # baut die GUI lokal
./build/biotope # Kiosk-Anzeige, spricht via Tunnel das Server-Backend an
```

Vorteil: keine Code-Änderung, keine Firewall-Öffnung, flüssige lokale Grafik, echte
Server-Daten.

---

## Empfohlene Reihenfolge zum Mitschreiben

1. **Server:** Pakete + Docker installieren (Teil 1)
2. **Server:** `.env` anlegen, Passwort 2× gleich (Teil 2)
3. **Server:** `make build/biotope_hyper_worker build/biotope_headless` (Teil 3)
4. **Server:** `docker compose up -d` (Teil 4)
5. **Server:** mit `curl` prüfen (Teil 5)
6. **Browser:** Editor öffnen (Teil 6)
7. **Anzeige:** Weg C – Tunnel + lokale GUI (Teil 7)

---

## Nützliche Befehle im Betrieb

```bash
docker compose ps             # Status aller Container
docker compose logs -f        # Live-Logs aller Dienste
docker compose restart backend
docker compose down           # Dienste stoppen (Daten bleiben im mongo_data-Volume)
docker compose down -v        # Dienste + Datenbank-Volume löschen (Achtung: Datenverlust)
```
