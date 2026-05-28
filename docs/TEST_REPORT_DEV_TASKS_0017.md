# Evidenzbasierter Untersuchungsbericht: DEV_TASKS-0017 (C-Networking)

**Datum:** 28. Mai 2026  
**Thema:** Analyse der Integrationsprobleme und Laufzeitfehler nach Abschluss der Programmierarbeiten für DEV_TASKS-0017.

## 1. Management Summary
Die Implementierung der in `DEV_TASKS-0017` geforderten C-Networking-Komponenten (Phase 4-6) sowie der Python-Backend-Erweiterungen (Phase 2-3) wurde code-technisch korrekt durchgeführt. Der beschriebene Fehler – ein "kleines Konfigurationsproblem" mit "komplexen Ursachen" – ist **kein Fehler im C-Code**, sondern ein **Abhängigkeitskonflikt in der Docker-Architektur** (Environment Mismatch). 

Darüber hinaus wurde eine potenziell unsichere String-Operation (`strncpy`) im C-Code identifiziert, die zu den beim Build beobachteten Compiler-Warnungen führt.

## 2. Root Cause Analysis: Docker & `libcurl` Laufzeitumgebung

### Symptom
Der Python-Worker `backend/app/worker.py`, der innerhalb des `matchmaker` Docker-Containers läuft, versucht am Ende einer Epoche den in C geschriebenen `biotope_hyper_worker` auszuführen. Dieser Vorgang schlägt mit folgendem Fehler im Container-Log fehl:
```
ERROR:epoch_worker:Hyper-Worker failed: ./build/biotope_hyper_worker: error while loading shared libraries: libcurl.so.4: cannot open shared object file: No such file or directory
```

### Ursache
1. **Build-Prozess (Host/c-dev):** Der Befehl `make` wird auf dem Host-System oder im `c-dev` Container ausgeführt. Da in Phase 1 die Bibliothek `-lcurl` zur Kompilierung hinzugefügt wurde, ist die resultierende Binärdatei `biotope_hyper_worker` dynamisch gegen `libcurl.so.4` gelinkt.
2. **Ausführung (matchmaker):** Über das Docker-Volume (`./build:/app/build`) wird diese kompilierte Binärdatei in den `matchmaker` Container gemountet.
3. **Konfigurationslücke:** Der `matchmaker` Container basiert auf dem Image `python:3.11-slim` (einem minimalen Debian). In der `docker-compose.yml` wird beim Start lediglich `libgomp1` installiert, jedoch **nicht** `libcurl4`. Da die dynamische Bibliothek fehlt, verweigert das Betriebssystem die Ausführung der C-Binärdatei.

### Beweisführung
Ein `ldd`-Check innerhalb des `matchmaker`-Containers bestätigt die fehlende Abhängigkeit:
```bash
$ docker compose exec matchmaker ldd ./build/biotope_hyper_worker
        libcurl.so.4 => not found
```

## 3. Code Quality & Memory Safety (`network_io.c`)

Die Überprüfung des neuen Moduls `src/io/network_io.c` (Phase 5 & 6) zeigt eine hohe Code-Qualität, die die Projektrichtlinien erfüllt:
- **Thread Safety:** Die geteilten Zustände `g_leaderboard` und `g_highlights` sind korrekt mit `PTHREAD_MUTEX_INITIALIZER` abgesichert. Die Getter kopieren die Daten via `memcpy` innerhalb des Mutex-Locks.
- **Memory Management (Valgrind):** Der für `libcurl` allokierte Speicher (via `realloc` im `write_callback`) wird am Ende der Threads korrekt mit `free(chunk.data)` freigegeben. Der von `cJSON_Parse` allokierte Speicher wird mit `cJSON_Delete(root)` aufgeräumt. Es sind keine Speicherlecks (Memory Leaks) ersichtlich.
- **Fehlerbehandlung:** HTTP-Response-Codes und fehlgeschlagenes JSON-Parsing werden abgefangen.

## 4. Compiler-Warnungen (`main_hyper.c`)

### Symptom
Beim Kompilieren des Hyper-Workers treten Warnungen auf:
```
src/apps/hyper/main_hyper.c:110:25: warning: ‘__builtin_strncpy’ output may be truncated copying 63 bytes from a string of length 63 [-Wstringop-truncation]
```

### Ursache
In `main_hyper.c` Zeile 110 wird versucht, eine Spieler-ID in das Struct `HighlightEntry` zu kopieren:
```c
strncpy(my_highlights[pos].player_id_red, m == 0 ? competitors[i].player_id : competitors[j].player_id, 63);
```
Das Array `player_id_red` ist 64 Bytes lang. Ein `strncpy` mit Länge 63 garantiert *keine* Null-Terminierung, wenn der Quellstring exakt 63 Zeichen lang ist. Dies warnt der GCC-Compiler (`-Wstringop-truncation`), da im schlimmsten Fall Garbage-Daten gelesen werden.

## 5. Actionable Next Steps (Lösungsvorschlag)

Um die Integration von DEV_TASKS-0017 vollständig abzuschließen, müssen folgende Schritte ausgeführt werden:

1. **Docker Compose aktualisieren (Fix für libcurl):**
   In `docker-compose.yml` unter dem Service `matchmaker` den `command`-String anpassen, sodass `libcurl4` installiert wird:
   ```yaml
   command: /bin/bash -c "pip install -r requirements.txt && apt-get update && apt-get install -y libgomp1 libcurl4 && python3 app/worker.py"
   ```

2. **Compiler-Warnung beheben (Fix für strncpy):**
   In `src/apps/hyper/main_hyper.c` die explizite Null-Terminierung hinzufügen:
   ```c
   strncpy(my_highlights[pos].player_id_red, m == 0 ? competitors[i].player_id : competitors[j].player_id, 63);
   my_highlights[pos].player_id_red[63] = '\0';
   strncpy(my_highlights[pos].player_id_blue, m == 0 ? competitors[j].player_id : competitors[i].player_id, 63);
   my_highlights[pos].player_id_blue[63] = '\0';
   ```

3. **Neustart der Container:**
   ```bash
   docker compose restart matchmaker
   ```

Nach diesen Anpassungen werden die Tasks in `DEV_TASKS-0017` erfolgreich abschließbar sein.