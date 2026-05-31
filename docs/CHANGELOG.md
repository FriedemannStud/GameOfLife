2026-05-31 — ADR-0023: Oscillator Filtering & Kiosk World Alignment

- Kiosk-Simulationswelt von 50×50 auf 8×16 umgestellt (app_state_manager.h/.c),
  identisch mit run_isolated_match() im Hyper-Worker. Leaderboard-Ergebnis und
  Kiosk-Anzeige verwenden jetzt dieselbe Arena.
- Seed-Platzierung im Kiosk korrigiert: Rot links (wie im Hyper-Worker), Blau rechts.
- Oscillator-Filterung in backend/app/worker.py implementiert (ADR-0023):
  _step_numpy(), is_oscillating_match(), filter_highlights_by_oscillation().
  Oszillierende Muster (Periode 2–5) werden in epoch_highlights ans Ende verschoben;
  nicht-oszillierende Matches erscheinen zuerst. Soft Fallback bei 0 sauberen Kandidaten.
- numpy>=1.24.0 zu backend/requirements.txt hinzugefügt.
- 13 Unit-Tests in backend/tests/test_oscillator_detection.py, alle grün.
- End-to-End verifiziert: Worker erkennt und loggt Oszillatoren, MongoDB enthält
  korrekt aufgelöste Nicknames und 10 geordnete Highlights.

26.12.2025: GitHub Account eingerichtet, GitHub Repository eingerichtet, Dockerfile angelegt, docker-compose angelegt, DEV Container angelegt, C-Umgebung angelegt, Hello Skript angelegt und getestet.

28.12.2025: README.md erstellt

05.01.2026: Initialisierung der Matrix erstellt, Erste Version der Ausgabe der aktuellen Matrix (World) in Entwicklungsumgebung erstellt.

07.01.2026: Berechnung von zellen in update_generation() modelliert. Untere-linke Ecke und untere-rechte Ecke noch nicht modelliert. - README.md um Beschreibung der Gestalt des Spielfelds ergänzt.

08.01.2026: Berechnung von zellen in update_generation() fertiggestellt. - Spielfeldgröße und Verzögerung der Anzeige als Komandozeilenparameter integriert.

09.01.2026: Neuen Branch (biotop) für getrennte Versionsverwaltung von "manueller" Programmierung und "Vibe-Coding" erstellt. Vibe-Coding Konzept für erweiterte Funktionalität "Interaktives Spielkonzept Rot vs Blau" erstellt. Interaktives Spielkonzept Rot vs Blau mittels Nutzung von Gemini-CLI-Agent programmiert. Nach Programmierung der Spiele-Logik Refactoring des UI durchgeführt.

10.01.2026: Feintuning des UI.

16.01.2026:
feat(gui): Tastenwiederholung für Konfigurationsparameter implementiert - Hilfsfunktion `IsActionTriggered` zur Verarbeitung von initialem Tastendruck und kontinuierlicher Wiederholung hinzugefügt. - Kontinuierliche Eingabeverarbeitung für Grid Size, Delay, Max Rounds und Max Population aktiviert. - Initialverzögerung auf 500ms und Wiederholungsintervall auf 50ms für flüssige Wertänderungen eingestellt.

feat(gui): Drag-to-Paint Funktion im Editor-Modus hinzugefügt - Ermöglicht das Aktivieren oder Deaktivieren mehrerer Zellen durch Ziehen mit gedrückter Maustaste. - Status (Platzieren oder Löschen) wird beim ersten Klicken automatisch erkannt. - Überprüfung der Team-Hemisphären und Populationslimits bleibt beim Ziehen aktiv.

perf(gui): Rendering-Flaschenhals durch texturbasiertes Zeichnen behoben -`DrawGridAndCells` auf Textur-Rendering umgestellt, um VcXsrv/X11-Lag zu minimieren - Ersetzt tausende `DrawRectangle`-Aufrufe durch ein einzelnes Textur-Update pro Frame. - Implementiert Ressourcen-Management (Lazy Init) für Textur- und Pixel-Buffer.- Nutzt Point-Filtering für pixelgenaue Darstellung bei der Skalierung.

17.01.2026:
feat ADR-0003-integrated-simulation-protocol: Implementierung des integrierten Simulations-Protokoll- und Replay-Systems - Einführung des Protokoll-Formats v2 (enthält Zeitstempel, Rundenanzahl und Delay) - Implementierung einer "Save-on-Start"-Logik: Jede Simulation wird beim Start automatisch in 'biotope_results/' archiviert - Neuer In-App Datei-Browser (STATE_LOAD) zum Durchsuchen und Laden vergangener Simulationen - Erweiterung der Protokolle um Ergebnis-Daten (Gewinner, Endstand) nach Abschluss einer Simulation - Integration einer Metadaten-Vorschau im Browser (Datum, Gittergröße, Ergebnis) - Sicherstellung der Abwärtskompatibilität für bestehende .bio-Dateien - Entfernung des nun redundanten Markdown-Exports (export_stats_md), da Ergebnisse direkt im Protokoll gespeichert werden - Optimierung der UI-Layouts zur Vermeidung von Textüberlappungen im Archiv-Modus

05.05.2026 (Anniversary Edition):
feat ADR-0005: Frictionless WASM Onboarding - Portierung der gesamten C-Anwendung nach WebAssembly mittels Emscripten. - Einführung einer `UpdateDrawFrame`-basierten Main-Loop für Browser-Kompatibilität. - Implementierung von persistentem Speicher via IndexedDB (FS.syncfs) zur Sicherung von .bio-Protokollen im Browser. - Hinzufügen von Phase-basiertem Onboarding (Tutorial/Puzzle-State).

feat ADR-0006: GPU Aesthetic Overhaul - Migration des Renderings von CPU-basierten Pixel-Arrays auf eine GPU-Shader-Pipeline. - Implementierung von temporalen Trails ("Fossils") mittels Ping-Pong-Framebuffer-Technik. - Unterstützung für GLSL 330 (Desktop) und GLSL 100 (Web/WASM). - Effizienter Transfer des Gitterzustands als 1-Byte-Grayscale-Textur zur Minimierung der Bus-Bandbreite. - Vollständiges Ressourcen-Management und VRAM-Cleanup integriert.

06.05.2026:
feat ADR-0007: Competitive 1v1 USP Focus - Implementierung des "Catalyst Strike": Einmalige 10x10 Vernichtungsaktion pro Team pro Match. - Einführung der "Ignition Sequence": Dramatischer 3-Sekunden-Countdown vor Simulationsstart. - Blind-Draft Phase: Turn-based Editor (Rot vs. Blau) mit versteckter Gegnerseite. - Dynamische Telemetrie: Automatisch skalierende Populationsgraphen basierend auf dem Peak-Wert. - Navbar-Scoreboard: Zentrierte Echtzeit-Statistiken für bessere competitive Übersicht.

feat ADR-0008: Epic Scale Tournament Architecture - Erweiterung der Gitterlimits auf 5000x5000 für native Desktop-Builds. - Implementierung einer "Active Chunk" Heuristik zur drastischen Performance-Steigerung bei großen Spielfeldern (Überspringen toter Sektoren). - Neuer `STATE_OBSERVER` (Observer-Modus) mit spezialisierten Broadcast-Tools. - Integration einer 2D-Kamera (Raylib `Camera2D`) für stufenlosen Zoom und freies Panning. - Optimierte Randomisierungs-Logik: Gleichmäßige 3%-Verteilung über die gesamte Hemisphäre.

20.05.2026:
feat(gui): Enhanced Random Population Density - Increased the random start cell density from 3% to 37.5% per team for the [R]andom key in editor mode.

feat(wasm): Persistent Browser Storage (IDBFS) - Added automatic creation and mounting of the 'biotope_results/' directory to IndexedDB (IDBFS). - Ensured filesystem persistence so saved .bio files are retained after browser refreshes. - Added -lidbfs.js and -s FORCE_FILESYSTEM=1 to the WASM build flags.

fix(load): Simulation Logic After Load - Resolved a critical bug where loaded configurations failed to simulate (black screen). - Fixed the 'load_grid' function to properly initialize and update the 'chunk_map' (spatial partitioning) for loaded cells.

perf(build): Robust Docker Build Environment - Renamed 'Makefile.web' to 'Makefile.wasm' to avoid GNU Make naming conflicts with legacy 'tangle' rules. - Implemented automatic 'emcc' path detection in 'Makefile.wasm' to ensure the compiler is found even in non-interactive Docker shells.

21.05.2026:
feat ADR-0009: Multiplayer JSON Ecosystem (Hard Cut) - Vollständiger Austausch des veralteten `.bio` Textformats durch ein web-kompatibles `.json` Format für die Client-Server-Kommunikation. - Spezifikation von Ligen (Einsteiger, Rookie, Champions) mit individuellen Bounding-Boxen. - Einführung relativer Koordinaten zur Entkopplung von Mustern und Spielfeldpositionen. - Integration der quelloffenen `cJSON` Bibliothek in die C-Codebasis und Anpassung der Build-Systeme. - Komplettes Refactoring von `file_io.c` (`save_grid`, `load_grid`) zur Vermeidung von Format-Fragmentierung (Single Source of Truth).

22.05.2026:
feat ADR-0010: Headless Simulation Worker - Implementierung eines CLI-basierten C-Workers (`biotope_headless`) zur automatisierten Match-Simulation auf Servern. - Entkopplung der Simulationslogik von der GUI (Raylib). - Unterstützung für rich metadata (`player_id`, `nickname`) und flexible Zellformate. - Generierung strukturierter Ergebnis-JSONs für das Backend-Matchmaking.

feat ADR-0011: REST API & Server-Side Validation - Entwicklung eines robusten Backends mittels Python/FastAPI. - Implementierung des `POST /api/v1/submit_config` Endpunkts für Spieler-Einreichungen. - Einführung einer strikten serverseitigen Validierung (Fair Play): Max. 38% Biomasse (24 Zellen) und 8x8 Bounding-Box. - Automatisierte API-Dokumentation via Swagger/OpenAPI.

22.05.2026 (Evening):
feat ADR-0012: Automated Matchmaking & Elo System - Vollständige Integration von MongoDB Atlas als persistenter Datenspeicher für Spieler, Muster und Matches. - Implementierung des "Proximity Swiss" Algorithmus zur fairen Paarung von Gegnern basierend auf ihrem Elo-Rating. - Entwicklung eines autonomen Hintergrund-Workers (`matchmaker`), der Simulationen via `biotope_headless` orchestriert. - Einführung eines dynamischen Elo-Ranking-Systems mit variablem K-Faktor für schnelle Konvergenz. - Containerisierung des Matchmakers als dedizierter Docker-Service mit automatisierter Abhängigkeitsverwaltung.

feat ADR-0013: Mobile-First Web Draft Editor - Implementierung eines eigenständigen Web-Editors (`editor.html`) für die Erstellung und Übermittlung von Zell-Konfigurationen via Smartphone. - Optimiertes Touch-Interface mit 8x8 Grid und Echtzeit-Biomasse-Validierung (max. 24 Zellen). - Persistente Spieler-Identität (UUID) mittels Browser LocalStorage. - Integration in die Haupt-App (`gui.c`) über einen direkten Link-Button ("MOBILE EDITOR"). - Konfiguration von CORS im FastAPI-Backend zur Ermöglichung von Cross-Origin Submissions von Mobilgeräten.

23.05.2026:
feat ADR-0015: Architectural Consolidation and Modularization - Complete refactoring of the C codebase to eliminate monolithic structures and circular dependencies. - Separated `gui.c` into specialized modules: `renderer.c` (Raylib presentation) and `app_state_manager.c` (core logic and state machine). - Centralized magic numbers and parameters into a new `config.h`. - Consolidated all core data structures (`World`, `GameConfig`, `Team`, `AppState`) into a pure C header `core_types.h`. - Migrated all I/O and JSON parsing routines out of entry points and strictly into `file_io.c` with robust boundary checks. - Implemented dedicated C-based State of the Art `DEV_TEST` suites for every refactoring phase, including an end-to-end golden snapshot integration test to guarantee zero regressions.

24.05.2026:
refactor(gui): UI Clean-up and Mobile Editor Discovery - Removed redundant manual save button ([S] key) from Editor mode as auto-save is performed on simulation start. - Removed non-functional 'MOBILE EDITOR' button from the main simulation HUD. - Implemented a new 'Mobile Editor Discovery' section in the Tutorial screen (STATE_PUZZLE) featuring a QR-code placeholder and short-link (biotope.io/editor). - Updated test checklists and debugging reports to reflect the streamlined UI architecture.
fix(gui): Ignition Countdown State Leak - Resolved an issue where the ignition countdown would only display '1' in subsequent simulation runs. - Fixed by ensuring `ignitionStartTime` is reset to `0.0` when returning to `STATE_CONFIG` from either `STATE_FINISHED` or `STATE_GAME_OVER`.
feat(gui): Independent Pause and Observer Modes - Introduced a global `is_paused` flag in `GameConfig` to decouple simulation state from camera state. - Assigned `[LEERTASTE]` to toggle simulation pause across all active modes with clean visual feedback in the dynamic footer. - Updated `[O]` to toggle the Observer (free-camera) mode independently of the simulation's run state. - Removed redundant green status messages that overlapped with the scoreboard in the header. - Implemented a dynamic 'Status-First' footer showing "PAUSED" vs "RUNNING" and context-aware controls. - Ensured `is_paused` state and camera state are reset when returning to the configuration screen.
fix(gui): Observer Camera State Leak - Resolved an issue where camera zoom and pan settings persisted across different simulation runs. - Fixed by ensuring the `observer_camera` is reset to default values when returning to the configuration screen.
refactor(gameplay): Catalyst Feature Removal - Completely removed the 'Catalyst' mechanic (`apply_catalyst`) from the simulation logic and UI. - This change eliminates redundant interactions, prevents UI clutter (overlapping texts in the scoreboard), and simplifies the codebase without affecting the core competitive loop.

28.05.2026:
feat ADR-0017: C-Networking and API Integration - Implementierung einer asynchronen Netzwerkschicht in C mittels `libcurl` und `pthread` zur Anbindung des Backends. - Einführung von `network_io.c` zur Kapselung aller HTTP-Aufrufe gemäß Clean Architecture. - Implementierung von Hintergrund-Threads für Leaderboard- und Highlight-Fetching zur Vermeidung von UI-Stottern (60 FPS Garantie). - Erweiterung der Hyper-Workers um die Metrik "Aktivitäts-Summe" (Summe aller Zelländerungen) zur Identifikation visuell dynamischer Highlights. - Optimierung des Speicherbedarfs: Interne Speicherung von 8x8 Highlight-Seeds als `uint64_t` Bitboards. - Backend-Update: Vollständige Unterstützung für MongoDB Atlas mit konfigurierbaren Credentials via `.env` und automatischem Verbindungs-Check beim Startup. - Integration von Test-Triggern (`[L]` für Leaderboard, `[H]` für Highlights) zur Verifizierung der Datenübertragung.

feat ADR-0018: Multicam Render Context Architecture
- Refactored core simulation and renderer logic to decouple from global state, introducing `SimulationContext` and `RenderContext`.
- Enabled simultaneous multi-viewport rendering and parallel simulations.
- Implemented `BeginScissorMode` clipping to prevent quadrant rendering bleed.
- Added a 2x2 Splitscreen "Wusel-Multicam" PoC (Kiosk Mode) triggered via the `[K]` key.
- Verified memory safety (0 definitely lost leaks from our new logic) and successful texture management.

feat ADR-0019: Kiosk Mode State Machine and UI
- Implementierung der `STATE_KIOSK_MODE` State Machine für autonomen Ablauf der App.
- Hinzufügen von Leaderboard Rendering und 2x2 Multicam Splitscreen Rendering inklusive nahtlosem Wechsel alle 15/30 Sekunden.
- Click-to-Replay Funktion eingebaut: Bei Klick auf einen Multicam-Quadranten startet ein Replay der Simulation im Vollbild (inklusive Ignition Countdown).
- Global Failsafe Inactivity Timer: Rückkehr zum Kiosk Mode nach 60 Sekunden Inaktivität aus jeglichen Menüs oder Replays heraus.
- Speichersicherheit beim Beenden von Simulationen und Kiosk-Übergängen (Fixing von Segfaults und Memory Leaks bei World-Allokationen).

31.05.2026:
feat ADR-0021: Kiosk Mode Engagement Enhancement
- P1: Spielernamen pro Multicam-Quadrant — `MatchHighlight.participant_red/blue` wird jetzt nach `SimulationContext.participant_red/blue` kopiert (vorher verworfen) und als farbiger Name-Header (`THEME_RED` / `THEME_BLUE`) über jedem Quadrant gerendert.
- P2: Live-Populationsbalken pro Quadrant — `KioskController` erweitert um `quad_red_pop[4]` / `quad_blue_pop[4]`; die bisher verworfenen `dummy_red/dummy_blue`-Ausgaben von `update_generation_ctx` werden jetzt persistent gespeichert und als proportionaler Rot/Blau-Balken am unteren Rand jedes Quadranten gerendert.
- P3: Metric-Reason-Label — `MatchHighlight.metric_reason` (z.B. "LONGEST MATCH") wird rechts-bündig im Header-Strip jedes Quadranten angezeigt.
- P4: 8×8-Startmuster-Thumbnail — Zeigt das ursprüngliche Einsaat-Muster als kleines farbiges Raster in der oberen linken Ecke jedes Quadranten, um den Kontrast zwischen einfacher Eingabe und komplexem Ergebnis sichtbar zu machen.
- P5: QR-Code-Platzhalter + CTA auf dem Leaderboard-Screen — Stilisierter QR-Code und "Submit YOUR strategy!" Einladungstext für Messegäste.
- P6: Visueller Fortschrittsbalken ersetzt den Text-Timer "SWITCHING IN X SECONDS" in beiden Kiosk-Substates.

feat ADR-0022: Kiosk UI Architecture Refactor & UX Enhancement (Phase B)
- Architektur: Render-Blöcke aus monolithischem switch extrahiert in `draw_kiosk_leaderboard()` und `draw_kiosk_multicam()`.
- Skalierung: `viewport_bounds` wird jetzt jedes Frame aus `compute_kiosk_layout()` neu berechnet — Quadranten skalieren korrekt bei Fenstergrößenänderung und Vollbild.
- Interaktion: Maus-Click-Trigger entfernt; Tasten `[1]`–`[4]` starten Replay des gewählten Matches vom Startzustand (Seed), nicht vom laufenden Zustand.
- Leaderboard: Footer-Panel (feste Leiste hinter CTA + Fortschrittsbalken), proportionales Spalten-Layout (80 % Bildschirmbreite, 10 % Margin), Clip-to-fit mit `+N more`-Indikator, Top-3-Zeilenhighlights, Spalte "STAMINA" → "ENDURANCE", QR-Panel entfernt.
- Multicam: Titel "WUSEL-MULTICAM KIOSK MODE" → "LIVE BATTLES", proportionale Score-Bar (≥ 20 px), Quadrant-Separator-Linien, metric_reason als zentrierter Badge unter Spielernamen, Thumbnail-Zellgröße proportional (≈ 20 % Quadranthöhe), Footer-Panel.
- Cleanup (Boy Scout Rule): Ungenutzte KioskLayout-Felder `font_cta`, `font_name`, `font_score` entfernt.