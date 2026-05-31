#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <unistd.h> 
#include <raylib.h>
#include "core_types.h"
#include "game_logic.h" 
#include "renderer.h"
#include "app_state_manager.h"
#include "config.h"
#include "network_io.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

// KI-Agent unterstützt: Shared Main Loop State
static AppState state = STATE_KIOSK_MODE;
static GameConfig config = {
    .rows = 50, 
    .cols = 50, 
    .delay_ms = 100, 
    .max_population = 100, 
    .max_rounds = 1000,
    .current_red_pop = 0,
    .current_blue_pop = 0,
    .current_round = 0,
    .history_red_pop = NULL,
    .history_blue_pop = NULL,
    .history_count = 0
};
static SimulationContext global_sim;
static RenderContext global_render;
static SessionOrigin session_origin = ORIGIN_NONE; // KI-Agent unterstützt: ADR-0020

void MainLoopStep(void) {
    update_global_input(&state, &global_sim, &config, &global_render);
    
    state = process_ui_events(state, &config, &global_sim, &global_render, &session_origin);
    state = update_app_state(state, &config, &global_sim, GetFrameTime(), GetTime(), &session_origin);
    draw_current_state(state, &config, global_sim.current_world, &global_render);
}

int main(int argc, char *argv[]) {
    // KI-Agent unterstützt: Explicitly ignore unused parameters
    (void)argc;
    (void)argv;
    srand(time(NULL));
    
    printf("Starting Biotope GUI...\n"); 
    
    network_init();
    init_renderer(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, "Biotope - Game of Life"); 
    
    init_simulation_context(&global_sim, config.rows, config.cols);
    init_render_context(&global_render, config.cols, config.rows, (Rectangle){ 20, 60, DEFAULT_WINDOW_WIDTH - 40, DEFAULT_WINDOW_HEIGHT - 120 });

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(MainLoopStep, 0, 1);
#else
    while (!WindowShouldClose()) {
        MainLoopStep();
    }
#endif

    free_render_context(&global_render);
    free_simulation_context(&global_sim);
    // KI-Agent unterstützt: Explicit kiosk lifecycle cleanup (ADR-0022)
    free_kiosk_controller(&kiosk_ctrl);
    close_renderer();
    network_cleanup();
    
    if (config.history_red_pop) free(config.history_red_pop);
    if (config.history_blue_pop) free(config.history_blue_pop);
    
    return 0;
}