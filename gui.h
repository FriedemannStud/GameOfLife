#ifndef GUI_H
#define GUI_H

#include "game_logic.h"

// KI-Agent unterstützt
typedef enum {
    STATE_PUZZLE,   // NEW: Tutorial/Onboarding
    STATE_CONFIG,
    STATE_EDIT,
    STATE_LOAD,     // NEW: For browsing protocol archive
    STATE_RUNNING,
    STATE_FINISHED, 
    STATE_GAME_OVER
} AppState;

// KI-Agent unterstützt
typedef struct {
    int level;
    int target_pop;
    char hint[256];
} PuzzleConfig;

// KI-Agent unterstützt
typedef struct {
    int rows;
    int cols;
    int delay_ms;
    int max_population;
    int max_rounds;
    // Current counters needed for UI display
    int current_red_pop;
    int current_blue_pop;
    int current_round;
} GameConfig;

// KI-Agent unterstützt
void init_gui_app(void);
void UpdateDrawFrame(void);
void close_gui_app(void);

#endif // GUI_H
