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
static AppState state = STATE_PUZZLE;
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
static World *current_world = NULL;
static World *swap_world = NULL;

void MainLoopStep(void) {
    state = process_ui_events(state, &config, &current_world, &swap_world);
    state = update_app_state(state, &config, &current_world, &swap_world, GetFrameTime(), GetTime());
    draw_current_state(state, &config, current_world);
}

int main(int argc, char *argv[]) {
    // KI-Agent unterstützt: Explicitly ignore unused parameters
    (void)argc;
    (void)argv;
    
    printf("Starting Biotope GUI...\n"); 
    
    network_init();
    init_renderer(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, "Biotope - Game of Life"); 

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(MainLoopStep, 0, 1);
#else
    while (!WindowShouldClose()) {
        MainLoopStep();
    }
#endif

    close_renderer();
    network_cleanup();
    
    if (config.history_red_pop) free(config.history_red_pop);
    if (config.history_blue_pop) free(config.history_blue_pop);
    if (current_world) free_world(current_world);
    if (swap_world) free_world(swap_world);
    
    return 0;
}