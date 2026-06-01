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

// Maximum string lengths
#define MAX_PATH_LENGTH 512
#define MAX_FILENAME_LENGTH 256
#define MAX_HINT_LENGTH 256

#endif // CONFIG_H