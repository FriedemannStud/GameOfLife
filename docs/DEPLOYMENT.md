# Deployment – Starting Biotop on a Server

This guide describes how to bring the complete Biotop application into operation on a server (e.g. a VM, accessed via SSH).

## Architecture in One Sentence

**Four services run in Docker** on the server (MongoDB, backend, editor, matchmaker). The **graphical display** is a separate C program that requires a screen (see [Part 7](#part-7--the-graphical-display-simulation--leaderboard)).

```
   Browser (Editor) ─┐
                     │  Port 8000
   GUI display ──────┤────────────►  [ Backend FastAPI ] ──► [ MongoDB ]
   (build/biotope)   │                      ▲
                     │                      │ writes Elo
                     └──────────►   [ Matchmaker Worker ]
                                    (calls build/biotope_hyper_worker every 60s)
```

| Component | What it is | Display required? |
|---|---|---|
| **MongoDB** | `mongo` container (docker-compose) | no |
| **Backend (FastAPI)** | Port 8000, serves API **and** the editor | no |
| **Matchmaker** | Worker container, calls `build/biotope_hyper_worker` every 60 s (Elo) | no |
| **HTML Editor** | Browser → `http://SERVER:8000/editor/editor.html` (uses relative `/api/...` URLs) | no |
| **Simulation display + leaderboard** | The C-Raylib program `build/biotope` (starts directly in kiosk mode), fetches data from hardcoded `http://localhost:8000` | **YES – requires OpenGL/X11 display** |

> The server stack (Mongo, backend, editor, matchmaker) runs completely **headless** –
> without Raylib and without a display. Only the graphical display requires a screen.

---

## Part 1 – Server Prerequisites (One-Time)

```bash
# On the server (logged in via SSH):
sudo apt update
sudo apt install -y docker.io docker-compose-plugin git build-essential libcurl4-openssl-dev
sudo usermod -aG docker $USER   # log out and back in once afterwards
```

`build-essential` (= gcc + make) and `libcurl4-openssl-dev` are needed to build the C binaries. **No** Raylib required for the server stack.

---

## Part 2 – Creating `.env`

The `.env` contains the MongoDB password and is intentionally not in Git. Create it from the template:

```bash
cd ~/GameOfLife          # into your cloned project directory
cp .env.example .env
nano .env
```

In `.env` you set **your own password** – in **two places identically**:

```env
MONGO_USER=biotope_admin
MONGO_PASSWORD=YourStrongPassword123
MONGODB_URI=mongodb://biotope_admin:YourStrongPassword123@mongo:27017/biotope_db?authSource=admin
MONGODB_DB=biotope_db
```

> ⚠️ Important: The password in `MONGO_PASSWORD` **and** in the `MONGODB_URI` must be exactly the same, otherwise the backend cannot reach the DB. The hostname `mongo` is the Docker service name – do not change it.

---

## Part 3 – Building C Binaries (Mandatory for the Matchmaker)

The `build/` folder is gitignored, so it is empty after `git pull`. The matchmaker will abort without its binary. Build:

```bash
cd ~/GameOfLife
make build/biotope_hyper_worker
make build/biotope_headless
```

The first command also automatically copies `biotope_hyper_worker` to `worker_bin/` (used by the matchmaker container). Check:

```bash
ls -l build/biotope_hyper_worker worker_bin/biotope_hyper_worker
```

> `make` (without a target) would also build the GUI and **fails without Raylib**.
> For the server, the two targets above are sufficient.

---

## Part 4 – Starting the Services

```bash
cd ~/GameOfLife
docker compose up -d
```

This starts:
- **mongo** – database
- **backend** – FastAPI on port 8000 (serves API + editor)
- **matchmaker** – worker that calculates tournaments every 60 s

> The `c-dev` container is only intended for local GUI building – you do not need it on the server. If `docker compose up -d` starts it as well, that is fine (it does nothing but sit there).

View logs (exit with `Ctrl+C`):

```bash
docker compose logs -f backend
docker compose logs -f matchmaker
```

The backend should show `Successfully connected to MongoDB`, the matchmaker should show an "Epoch finished" line every 60 s (once patterns have been submitted).

---

## Part 5 – Does It Work? (Verification)

Directly on the server:

```bash
curl http://localhost:8000/                 # -> {"message":"Hello Biotope"}
curl http://localhost:8000/api/leaderboard  # -> JSON list (empty [] initially)
```

---

## Part 6 – Opening the HTML Editor

The editor is served by the backend itself and uses **relative** API URLs – it therefore works via any address at which port 8000 is reachable.

In the browser (from your laptop):

```
http://<SERVER-IP>:8000/editor/editor.html
```

> If the page does not load: **Port 8000** must be open in the server's firewall/security group. Alternatively, without opening the firewall, via SSH tunnel (see Part 7, Option C).

---

## Part 7 – The Graphical Display (Simulation + Leaderboard)

This is `build/biotope`. It starts directly in **kiosk mode** (2×2 live matches + leaderboard) and fetches its data **hardcoded from `http://localhost:8000`**. It requires a screen/OpenGL. Three options:

### Option A – Monitor Directly on the Server
Only practical if the VM has a real display/HDMI (exhibition kiosk). Cloud VMs almost never have this. Then: install Raylib, run `make`, start `./build/biotope` locally on the server display.

### Option B – X11 over SSH (`ssh -X`)
GUI renders on your laptop. Requires an X server on your side (under WSL: WSLg or VcXsrv). OpenGL over X11 forwarding is often slow and temperamental → not ideal for smooth animation.

### Option C – GUI Locally + SSH Tunnel ⭐ (Recommended)
The server provides only the **data**; you build and start the GUI on **your own machine** (which has a screen). Since the GUI hardcodes `localhost:8000`, you build a tunnel – then `localhost:8000` on your machine is automatically forwarded to the server:

```bash
# On your local machine:
ssh -L 8000:localhost:8000 <user>@<SERVER-IP>
```

Keep this terminal open. In a second local terminal (in the local project clone, where Raylib is installed):

```bash
make            # builds the GUI locally
./build/biotope # kiosk display, talks to the server backend via tunnel
```

Advantage: no code change, no firewall opening, smooth local graphics, real server data.

---

## Recommended Step-by-Step Order

1. **Server:** Install packages + Docker (Part 1)
2. **Server:** Create `.env`, password identical in both places (Part 2)
3. **Server:** `make build/biotope_hyper_worker build/biotope_headless` (Part 3)
4. **Server:** `docker compose up -d` (Part 4)
5. **Server:** Check with `curl` (Part 5)
6. **Browser:** Open editor (Part 6)
7. **Display:** Option C – tunnel + local GUI (Part 7)

---

## Useful Commands During Operation

```bash
docker compose ps             # status of all containers
docker compose logs -f        # live logs of all services
docker compose restart backend
docker compose down           # stop services (data remains in mongo_data volume)
docker compose down -v        # stop services + delete database volume (Warning: data loss)
```
