

  📝 Interaktionstest-Checkliste: Biotope GUI

  1. Start & Onboarding (STATE_PUZZLE)
   - [ ] App startet ohne Absturz (./biotope).
   - [ ] Tutorial/Willkommens-Text wird mittig angezeigt.
   - [ ] Aktion: [ENTER] drücken wechselt zum Konfigurations-Bildschirm.

  2. Konfiguration (STATE_CONFIG)
   - [ ] Aktuell ausgewählter Parameter wird optisch hervorgehoben (Glühen/Highlight).
   - [ ] Aktion: Mit den Pfeiltasten [LINKS] und [RECHTS] zwischen den Parametern (Grid
     Size, Delay, Max Rounds, Target Pop) wechseln.
   - [ ] Aktion: Mit [OBEN] den Wert des ausgewählten Parameters erhöhen (gedrückt halten
     für schnelles Hochzählen).
   - [ ] Aktion: Mit [UNTEN] den Wert des ausgewählten Parameters verringern (gedrückt
     halten für schnelles Runterzählen).
   - [ ] Aktion: [ENTER] drücken wechselt in den Editor-Modus für Team Rot.

  3. Editor Modus: Team Rot (STATE_EDIT_RED)
   - [ ] Die rechte Bildschirmhälfte ist abgedunkelt/gesperrt.
   - [ ] Maus-Aktion: Linksklick auf eine leere Zelle in der linken Hälfte platziert eine
     rote Zelle.
   - [ ] Maus-Aktion: Linksklick auf eine vorhandene rote Zelle löscht diese wieder.
   - [ ] Maus-Aktion (Drag-to-Paint): Maustaste gedrückt halten und über das Gitter ziehen,
     um flüssig mehrere Zellen zu zeichnen/löschen.
   - [ ] Aktion: [R] drücken füllt die rote Seite zufällig mit 37,5% Zellen.
   - [ ] Aktion: [C] drücken löscht alle roten Zellen auf dem Feld.
   - [ ] Muster platzieren: Maus über das Gitter bewegen und [G] (Glider), [T] (Toad) oder
     [B] (Block) drücken.
   - [ ] UI-Button: Auf den Button "MOBILE EDITOR" oben rechts klicken (sollte den
     Standard-Webbrowser mit editor.html öffnen).
   - [ ] Die aktuelle Populationszahl ("Target Pop") aktualisiert sich korrekt beim
     Setzen/Löschen.
   - [ ] Aktion: [ENTER] drücken wechselt zum Editor für Team Blau.

  4. Editor Modus: Team Blau (STATE_EDIT_BLUE)
   - [ ] Die linke Bildschirmhälfte (Rot) ist nun gesperrt, bestehende rote Zellen sind
     sichtbar, aber nicht bearbeitbar.
   - [ ] Alle Maus-Aktionen (Klicken, Drag-to-Paint) und Muster-Tasten ([G], [T], [B])
     funktionieren nun für die blaue (rechte) Seite.
   - [ ] Aktion: [R] drücken füllt die blaue Seite zufällig (Random).
   - [ ] Aktion: [C] drücken leert die blaue Seite (Clear).
   - [ ] Aktion: [L] drücken öffnet das Archiv-Menü (Load State).
   - [ ] Aktion: [ENTER] drücken startet die Ignition-Sequenz (Zündung).

  5. Protokoll-Archiv (STATE_LOAD) - Optional, falls [L] gedrückt wurde
   - [ ] Eine Liste der vergangenen .json-Protokolle aus biotope_results/ wird angezeigt.
   - [ ] Aktion: Mit [OBEN] und [UNTEN] durch die Liste scrollen.
   - [ ] Metadaten (Gewinner, Generationen, Spieler) der ausgewählten Datei werden im
     Vorschau-Panel angezeigt.
   - [ ] Aktion: [Q] drücken bricht ab und kehrt zum Editor zurück.
   - [ ] Aktion: [ENTER] lädt das ausgewählte Gitter und kehrt zum Editor zurück.

  6. Countdown & Simulation (STATE_IGNITION & STATE_RUNNING)
   - [ ] Zündung: Ein dramatischer Countdown (3 Sekunden) läuft ab. Keine Eingaben möglich.
   - [ ] Simulation: Das Spiel beginnt automatisch, rote und blaue Zellen interagieren.
   - [ ] Die Generationen-Anzeige (Round) und Populations-Zähler zählen korrekt hoch.
   - [ ] Aktion: [LEERTASTE] drücken pausiert die Simulation. Erneutes
     Drücken lässt sie weiterlaufen.

  7. Observer Modus (STATE_OBSERVER / Pausiert)
   - [ ] Das Wort "PAUSED" blinkt auf dem Bildschirm.
   - [ ] Aktion (Kamera-Pan): Die [ALT]-Taste gedrückt halten und mit der Maus ziehen, um
     das Spielfeld zu verschieben.
   - [ ] Aktion (Kamera-Zoom): Die [ALT]-Taste gedrückt halten und das Mausrad drehen, um
     herein- oder herauszuzoomen.

  8. Globale Effekte (Überall verfügbar)
   - [ ] Aktion: [M] drücken schaltet den Metaball-Shader/Filter ein und aus (Statusmeldung
     "METABALLS: ON/OFF" erscheint).

  9. Spielende (STATE_FINISHED)
   - [ ] Die Simulation stoppt automatisch, wenn die Max Rounds erreicht sind ODER ein Team
     komplett ausstirbt.
   - [ ] Ein "Game Over" Screen wird angezeigt.
   - [ ] Der Gewinner (Rot, Blau oder Unentschieden) wird korrekt ausgerufen.
   - [ ] Ein Liniendiagramm (Telemetry Graph) zeigt den historischen Verlauf der roten und
     blauen Population.
   - [ ] Aktion: [1] drücken setzt das System vollständig zurück und man landet wieder im
     Tutorial/Konfigurations-Bildschirm.
## Addendum: Debugging Report (Refactoring Regression Analysis)

**Date:** 2026-05-23
**Context:** This report analyzes the regressions identified during the manual interactive test following the architectural consolidation (ADR-0015).

### 1. Configuration Screen (`STATE_CONFIG`)
**Observations:**
- Arrow keys (LEFT/RIGHT) do not switch between parameters.
- No visual highlighting of a selected parameter.
- UP/DOWN arrow keys do not increment/decrement a 'selected' parameter. Instead, parameters seem to have individual hardcoded keys.
**Developer Hints for Debugging:**
- **Location:** `renderer.c`, in `process_ui_events` under `case STATE_CONFIG:` and in `draw_current_state` under `case STATE_CONFIG:`.
- **Analysis:** During the refactoring or in a previous iteration, the 'selected parameter index' logic seems to have been lost or replaced by hardcoded keybindings for each specific configuration value. The drawing logic is missing the `THEME_HIGHLIGHT` rectangle that should be drawn behind the actively selected configuration row.
- **Action:** Introduce a `static int selected_config_idx = 0;` variable in `renderer.c` to track which parameter is active (0=Grid Size, 1=Delay, 2=Max Rounds, 3=Target Pop). Update `process_ui_events` to use LEFT/RIGHT to change this index, and UP/DOWN to modify `config` values based on this index.

### 2. Editor Mode (`STATE_EDIT_RED` / `STATE_EDIT_BLUE`)
**Observations:**
- Pressing `[C]` does not clear the board.
- Pressing `[S]` does not show the 'Gitter gespeichert' (Grid saved) status message.
- 'MOBILE EDITOR' button fails to open the browser.
**Developer Hints for Debugging:**
- **Location (`[C]` Clear):** `renderer.c`, `process_ui_events`. Search for `IsKeyPressed(KEY_C)`. It appears this logic was lost during the split. It needs to loop through the grid and set `TEAM_NONE` (DEAD) for the respective side.
- **Location (`[S]` Save):** `renderer.c`, `process_ui_events`, around `if (IsKeyPressed(KEY_S)) save_grid...`. The `statusMsg` and `statusTimer` are not being updated when `save_grid` is called. Add `strcpy(statusMsg, "Gitter gespeichert!"); statusTimer = 2.0f;`.
- **Location (Mobile Editor):** `renderer.c`, `draw_current_state` (or `process_ui_events`). The `OpenURL("editor.html")` call is likely failing because the file path is relative to the binary, but `OpenURL` often requires an absolute `file://` URI or `http://` depending on the OS.

### 3. Countdown & Simulation (`STATE_IGNITION` & `STATE_RUNNING`)
**Observations:**
- During ignition, the countdown is broken ('Bei [2] und [3] wird nur die Ziffer 1 drei Sekunden lang angezeigt').
- Spacebar `[LEERTASTE]` does not pause the simulation.
- In `STATE_OBSERVER`, the 'PAUSED' text overlaps with the red/blue population counters.
**Developer Hints for Debugging:**
- **Location (Countdown):** `renderer.c`, `draw_current_state`, under `case STATE_IGNITION:`. The math calculating the remaining seconds is likely flawed. *Check:* `double elapsed = GetTime() - ignitionStartTime;` If `ignitionStartTime` is not accessible in `renderer.c`, the math fails. Note that `ignitionStartTime` was moved to `app_state_manager.c`. `renderer.c` needs a way to track the start time for the UI.
- **Location (Pause / `[LEERTASTE]`):** `renderer.c`, `process_ui_events`. Search for `case STATE_RUNNING:` and `case STATE_OBSERVER:`. The logic `if (IsKeyPressed(KEY_SPACE)) state = STATE_OBSERVER;` is missing.
- **Location (Overlap):** `renderer.c`, `draw_current_state`. Adjust the `y` coordinate in `DrawText("PAUSED", ...)` to push it lower (e.g., `screenHeight/2 - 50`).

