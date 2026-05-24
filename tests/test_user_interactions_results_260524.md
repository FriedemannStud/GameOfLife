## Addendum: Debugging Report (Refactoring Regression Analysis)

**Date:** 2026-05-23
**Context:** This report analyzes the regressions identified during the manual interactive test following the architectural consolidation (ADR-0015).

### 1. Editor Mode (`STATE_EDIT_RED` / `STATE_EDIT_BLUE`)
**Observations:**
- Pressing `[S]` - **REMOVED**: This feature was removed as it is redundant due to auto-save on simulation start.
- 'MOBILE EDITOR' button fails to open the browser.
**Developer Hints for Debugging:**
- **Location (`[S]` Save):** `renderer.c`, `process_ui_events`, around `if (IsKeyPressed(KEY_S)) save_grid...`. The `statusMsg` and `statusTimer` are not being updated when `save_grid` is called. Add `strcpy(statusMsg, "Gitter gespeichert!"); statusTimer = 2.0f;`.
- **Location (Mobile Editor):** `renderer.c`, `draw_current_state` (or `process_ui_events`). The `OpenURL("editor.html")` call is likely failing because the file path is relative to the binary, but `OpenURL` often requires an absolute `file://` URI or `http://` depending on the OS. **Before you touch this Observation:** Discuss with the user if this feature is realy required.

### 2. Countdown & Simulation (`STATE_IGNITION` & `STATE_RUNNING`)
**Observations:**
- During ignition, the countdown is broken ('Bei [2] und [3] wird nur die Ziffer 1 drei Sekunden lang angezeigt'). - **RESOLVED**: Fixed state leak in `renderer.c`.
**Root Cause Analysis:**
The `static double ignitionStartTime` in `renderer.c` was only set to `GetTime()` if it was `0.0`, but it was never reset to `0.0` when a simulation ended naturally (`STATE_GAME_OVER`) or was manually finished (`STATE_FINISHED`). Consequently, subsequent runs used the timestamp from the first simulation of the session, causing the countdown calculation `3 - (GetTime() - ignitionStartTime)` to result in negative values, which were clamped to `1`.
**Fix:**
Implemented `ignitionStartTime = 0.0;` in all transitions leading back to `STATE_CONFIG` in `renderer.c`.

- Spacebar `[LEERTASTE]` does not pause the simulation. - **RESOLVED**: Implemented independent `is_paused` flag in `GameConfig`.
- In `STATE_OBSERVER`, the 'PAUSED' text overlaps with the red/blue population counters. - **RESOLVED**: Added a centered, pulsing 'PAUSED' overlay that is independent of HUD counters.

### 3. Observer Mode & Camera
**Observations:**
- Camera zoom/pan settings persist across different simulations if observer mode was active. - **RESOLVED**: Fixed camera state leak in `renderer.c`.
**Root Cause Analysis:**
The `static Camera2D observer_camera` in `renderer.c` retained its transformation state (zoom and target coordinates) because it was only initialized at application startup. Transitions back to `STATE_CONFIG` did not reset the camera.
**Fix:**
Implemented `observer_camera` reset (zoom 1.0, target 0,0) in all transitions leading back to `STATE_CONFIG` in `renderer.c`.