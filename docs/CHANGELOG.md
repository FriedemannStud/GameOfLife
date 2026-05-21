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