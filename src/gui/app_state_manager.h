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

// KI-Agent unterstützt: Ignition time accessors — single source of truth (ADR-0020)
double get_ignition_start_time(void);
void set_ignition_start_time(double t);

// KI-Agent unterstützt: Centralized cleanup for interactive sessions (ADR-0020)
void cleanup_interactive_session(SimulationContext *sim, GameConfig *config, RenderContext *r_ctx);

// KI-Agent unterstützt: update_global_input with full context for cleanup on timeout
void update_global_input(AppState* current_app_state, SimulationContext *sim,
                         GameConfig *config, RenderContext *r_ctx);

// KI-Agent unterstützt: App state manager decoupled from rendering
AppState update_app_state(AppState current_state, GameConfig* config,
                          SimulationContext *sim_ctx, float delta_time,
                          double current_time, SessionOrigin *session_origin);

#endif // APP_STATE_MANAGER_H