# 🚀 Start-Routine: Biotope Game of Life

Diese Anleitung hilft dir, das Projekt schnell zu starten und wieder in den Workflow zu finden, auch wenn du längere Zeit nicht daran gearbeitet hast.

---

## 1. Das Projekt bauen (Kompilieren)

Bevor du die App starten kannst, musst du den Quellcode in ein ausführbares Programm verwandeln.

### 💻 Lokal (Native App für Linux/Windows)
Wenn du direkt auf deinem Rechner (mit installierter Raylib) arbeitest:

#### Linux-Voraussetzungen (Einmalige Einrichtung)
Sollte `raylib.h` beim Kompilieren nicht gefunden werden, fehlen die System-Bibliotheken. Installiere sie mit:
```bash
sudo apt update
sudo apt install libraylib-dev  # Falls im Repo vorhanden
# ODER (Manuell bauen, falls obiges nicht geht):
sudo apt install make git cmake libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
```
*Hinweis: Wenn du Raylib manuell baust, folge den Instruktionen auf [raylib.com](https://www.raylib.com).*

1. Öffne ein Terminal im Hauptverzeichnis des Projekts.
2. Führe diesen Befehl aus:
   ```bash
   make clean && make
   ```
   *Tipp: `make clean` sorgt dafür, dass keine alten Reste den Build stören.*

### 🌐 Für den Browser (Web/WASM)
Wenn du die Web-Version erstellen möchtest:
1. Führe diesen Befehl aus:
   ```bash
   make -f Makefile.web
   ```

---

## 2. Die App starten

### 🚀 Native App ausführen
Nach dem erfolgreichen Kompilieren kannst du das Programm starten:
- **Linux:** 
  ```bash
  ./biotope
  ```
- **Windows:** Doppelklick auf `biotope.exe` oder im Terminal:
  ```bash
  biotope.exe
  ```

### 🌐 Web-Version im Browser anschauen
Die Web-Dateien (`biotope.html`) können nicht einfach per Doppelklick geöffnet werden. Du benötigst einen kleinen Web-Server:
1. Starte einen einfachen Server (z.B. mit Python):
   ```bash
   python3 -m http.server 8080
   ```
2. Öffne deinen Browser und gib diese Adresse ein:
   [http://localhost:8080/biotope.html](http://localhost:8080/biotope.html)

---

## 3. Arbeiten mit Docker (Sorglos-Paket)

Wenn du keine Bibliotheken (wie Raylib oder Emscripten) lokal installieren möchtest, kannst du die vorbereitete Docker-Umgebung nutzen.

1. **Container starten:**
   ```bash
   docker-compose up -d
   ```
2. **In die Entwicklungsumgebung wechseln:**
   ```bash
   docker-compose exec c-dev bash
   ```
3. **Innerhalb von Docker bauen:**
   Jetzt bist du "im" System und kannst einfach `make` oder `make -f Makefile.web` nutzen.

---

## 🛠️ Kurzer Check beim Wiedereinstieg

Wenn du nach einer Pause zurückkehrst, empfiehlt sich dieser kurze Workflow:

1. **Status prüfen:** `git status` (Was habe ich zuletzt geändert?)
2. **Aufgabenliste ansehen:** Schau in `docs/tasks/`, welche Punkte noch offen sind (Checkboxen `- [ ]`).
3. **Logbuch lesen:** In `docs/CHANGELOG.md` siehst du, was zuletzt erfolgreich abgeschlossen wurde.
4. **Bauen & Starten:** Nutze die Befehle oben, um sicherzustellen, dass die aktuelle Version stabil läuft.

---

*Viel Erfolg beim Weiterforschen im Biotope!* 🧬
