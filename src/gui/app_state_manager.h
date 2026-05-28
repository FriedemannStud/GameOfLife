#ifndef APP_STATE_MANAGER_H
#define APP_STATE_MANAGER_H

#include "core_types.h"

// KI-Agent unterstützt: App state manager decoupled from rendering
AppState update_app_state(AppState current_state, GameConfig* config, SimulationContext *sim_ctx, float delta_time, double current_time);

#endif // APP_STATE_MANAGER_H