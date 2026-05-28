#ifndef APP_STATE_MANAGER_H
#define APP_STATE_MANAGER_H

#include "core_types.h"
#include "renderer.h"
#include "network_io.h"

// KI-Agent unterstützt: Kiosk Controller
typedef enum {
    KIOSK_SUB_LEADERBOARD,
    KIOSK_SUB_MULTICAM
} KioskSubState;

typedef struct {
    KioskSubState current_sub_state;
    float state_timer;
    
    RenderContext* renders; 
    SimulationContext* sims;
    bool initialized;
    LeaderboardData cached_lb;
    HighlightData cached_highlights;
} KioskController;

extern KioskController kiosk_ctrl;
void reset_kiosk_timers(void);
void update_global_input(AppState* current_app_state);

// KI-Agent unterstützt: App state manager decoupled from rendering
AppState update_app_state(AppState current_state, GameConfig* config, SimulationContext *sim_ctx, float delta_time, double current_time);

#endif // APP_STATE_MANAGER_H