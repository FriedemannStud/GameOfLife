#ifndef CONFIG_H
#define CONFIG_H

// KI-Agent unterstützt: Centralized configuration macros

// GUI Configuration
#define DEFAULT_WINDOW_WIDTH  800
#define DEFAULT_WINDOW_HEIGHT 600

// KI-Agent unterstützt: Default main-game grid dimensions (ADR-0024 restore)
#define DEFAULT_GRID_ROWS 50
#define DEFAULT_GRID_COLS 50

// Worker / Grid Configuration
#define LOCAL_GRID_SIZE 8

// Kiosk Mode — simulation timing
#define KIOSK_SIM_GEN_INTERVAL_S  0.4f   // 1/xf = Generationen/s im Live-Battles-Fenster

// KI-Agent unterstützt: Kiosk render budget (ADR-0033)
#define KIOSK_TARGET_FPS          15      // per-device tuning knob (FR-1); 15 for low-heat trade-show operation, 30 for cool office-laptop, 60 for smoother motion
#define KIOSK_FADE_REF_FPS        120.0f  // reference fps the 0.95 fade was tuned at (FR-2)
#define KIOSK_FADE_PER_REF_FRAME  0.95f   // fossil fade per frame @ reference fps (FR-2)
#define KIOSK_FADE_DT_MAX         0.1f    // clamp frame-time spikes so a stall can't wipe trails (FR-2)
#define KIOSK_MAX_FBO_HEIGHT      1080    // cap for ping-pong FBO height; no-op at ≤1080p (FR-3)

// Maximum string lengths
#define MAX_PATH_LENGTH 512
#define MAX_FILENAME_LENGTH 256
#define MAX_HINT_LENGTH 256

#endif // CONFIG_H