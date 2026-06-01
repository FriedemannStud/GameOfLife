# DEV_TASKS-0024b: Epoch-Reset und Named Constant für Highlight-Pool

Zwei Verbesserungen am bestehenden ADR-0024-Feature (Round-Robin Rotation):

1. **Punkt A** — `highlight_pool_index` nur bei echtem Epoch-Wechsel zurücksetzen (via `epoch_id` aus dem API-Response).
2. **Punkt B** — Magic Number `10` durch `MAX_HIGHLIGHT_MATCHES` ersetzen.

---

- [x] **B.1** `#define MAX_HIGHLIGHT_MATCHES 10` in `network_io.h` hinzufügen
- [x] **B.2** `matches[10]` → `matches[MAX_HIGHLIGHT_MATCHES]` in `network_io.h`
- [x] **B.3** `i < 10` → `i < MAX_HIGHLIGHT_MATCHES` in `network_io.c`
- [x] **A.1** `char epoch_id[64]` zu `HighlightData` in `network_io.h` hinzufügen
- [x] **A.2** `epoch_id` aus JSON parsen in `network_io.c`
- [x] **A.3** `char last_epoch_id[64]` zu `KioskController` in `app_state_manager.h` hinzufügen
- [x] **A.4** `last_epoch_id[0] = '\0'` in `init_kiosk_controller()` initialisieren
- [x] **A.5** Reset-Logik in `update_app_state()` auf `strcmp(epoch_id)` umstellen
- [x] **Build** `make` — zero warnings
