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
static SimulationContext global_sim;
static RenderContext global_render;

static SimulationContext kiosk_sims[4];
static RenderContext kiosk_renders[4];
static bool kiosk_initialized = false;

void MainLoopStep(void) {
    if (state == STATE_KIOSK_MODE) {
        if (!kiosk_initialized) {
            int w = GetScreenWidth();
            int h = GetScreenHeight();
            int pad = 12;
            int q_w = (w - pad * 3) / 2;
            int q_h = (h - pad * 3) / 2;
            
            // Quad 0: Top-Left
            init_simulation_context(&kiosk_sims[0], 50, 50);
            init_world(kiosk_sims[0].current_world, 50, 50);
            init_render_context(&kiosk_renders[0], 50, 50, (Rectangle){ (float)pad, (float)pad, (float)q_w, (float)q_h });
            
            // Quad 1: Top-Right
            init_simulation_context(&kiosk_sims[1], 50, 50);
            init_world(kiosk_sims[1].current_world, 50, 50);
            init_render_context(&kiosk_renders[1], 50, 50, (Rectangle){ (float)(q_w + pad * 2), (float)pad, (float)q_w, (float)q_h });
            
            // Quad 2: Bottom-Left
            init_simulation_context(&kiosk_sims[2], 50, 50);
            init_world(kiosk_sims[2].current_world, 50, 50);
            init_render_context(&kiosk_renders[2], 50, 50, (Rectangle){ (float)pad, (float)(q_h + pad * 2), (float)q_w, (float)q_h });
            
            // Quad 3: Bottom-Right
            init_simulation_context(&kiosk_sims[3], 50, 50);
            init_world(kiosk_sims[3].current_world, 50, 50);
            init_render_context(&kiosk_renders[3], 50, 50, (Rectangle){ (float)(q_w + pad * 2), (float)(q_h + pad * 2), (float)q_w, (float)q_h });
            
            kiosk_initialized = true;
        }
        
        // Handle input to exit Kiosk Mode
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q)) {
            for (int i = 0; i < 4; i++) {
                free_render_context(&kiosk_renders[i]);
                free_simulation_context(&kiosk_sims[i]);
            }
            kiosk_initialized = false;
            state = STATE_CONFIG;
            return;
        }
        
        // Update loop: simulate at 10 FPS
        static float kiosk_timer = 0.0f;
        kiosk_timer += GetFrameTime();
        if (kiosk_timer >= 0.1f) {
            kiosk_timer = 0.0f;
            int dummy_red, dummy_blue;
            for (int i = 0; i < 4; i++) {
                update_generation_ctx(&kiosk_sims[i], &dummy_red, &dummy_blue);
            }
        }
        
        // Draw loop
        BeginDrawing();
        ClearBackground((Color){ 20, 24, 32, 255 });
        
        for (int i = 0; i < 4; i++) {
            DrawGridAndCellsCtx(&kiosk_renders[i], &config, kiosk_sims[i].current_world, false);
        }
        
        // Header HUD overlay (transparent black)
        DrawRectangle(0, 0, GetScreenWidth(), 40, (Color){ 10, 12, 16, 200 });
        DrawText("WUSEL-MULTICAM KIOSK MODE PoC (2x2)", 20, 10, 20, (Color){ 0, 220, 255, 255 });
        DrawText("PRESS [Q] / [ESC] TO RETURN TO CONFIG", GetScreenWidth() - MeasureText("PRESS [Q] / [ESC] TO RETURN TO CONFIG", 16) - 20, 12, 16, (Color){ 255, 60, 100, 255 });
        
        EndDrawing();
        
    } else {
        state = process_ui_events(state, &config, &global_sim.current_world, &global_sim.next_world, &global_render);
        state = update_app_state(state, &config, &global_sim, GetFrameTime(), GetTime());
        draw_current_state(state, &config, global_sim.current_world, &global_render);
    }
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
    
    if (kiosk_initialized) {
        for (int i = 0; i < 4; i++) {
            free_render_context(&kiosk_renders[i]);
            free_simulation_context(&kiosk_sims[i]);
        }
    }
    
    close_renderer();
    network_cleanup();
    
    if (config.history_red_pop) free(config.history_red_pop);
    if (config.history_blue_pop) free(config.history_blue_pop);
    
    return 0;
}