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

// KI-Agent unterstützt: Named constants replacing magic numbers (ADR-0022)
// KI-Agent unterstützt: Matches run_isolated_match() world exactly (rows=8, cols=16)
#define KIOSK_SIM_ROWS  LOCAL_GRID_SIZE        //  8 rows  — single source of truth: config.h
#define KIOSK_SIM_COLS  (LOCAL_GRID_SIZE * 2)  // 16 cols  — same 1:2 ratio as hyper-worker
#define KIOSK_DEFAULT_MATCH_COUNT  4  // number of simultaneous matches shown

typedef struct {
    KioskSubState current_sub_state;
    float state_timer;
    int   match_count;               // single source of truth for all kiosk loops (ADR-0022)

    RenderContext    *renders;       // dynamically allocated array (match_count elements)
    SimulationContext *sims;         // dynamically allocated array (match_count elements)
    bool initialized;
    LeaderboardData   cached_lb;
    HighlightData     cached_highlights;

    // KI-Agent unterstützt: Per-quadrant live population — dynamic arrays (ADR-0022)
    int *quad_red_pop;               // length == match_count
    int *quad_blue_pop;              // length == match_count
} KioskController;

extern KioskController kiosk_ctrl;
void reset_kiosk_timers(void);

// KI-Agent unterstützt: Explicit lifecycle for kiosk GPU and simulation resources (ADR-0022)
void init_kiosk_controller(KioskController *ctrl, int match_count, int screen_w, int screen_h);
void free_kiosk_controller(KioskController *ctrl);

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