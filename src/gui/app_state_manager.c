#include "app_state_manager.h"
#include "game_logic.h"
#include "network_io.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// KI-Agent unterstützt: App state manager
static double ignitionStartTime = 0.0;
static float timeAccumulator = 0.0f;

static SimulationContext kiosk_sims[4];
static RenderContext kiosk_renders[4];

KioskController kiosk_ctrl = {
    .current_sub_state = KIOSK_SUB_LEADERBOARD,
    .state_timer = 0.0f,
    .renders = kiosk_renders,
    .sims = kiosk_sims,
    .initialized = false
};

static double time_since_last_input = 0.0;

void reset_kiosk_timers(void) {
    kiosk_ctrl.state_timer = 0.0f;
    kiosk_ctrl.current_sub_state = KIOSK_SUB_LEADERBOARD;
    network_fetch_leaderboard_async();
}

void update_global_input(AppState* current_app_state) {
    if (IsKeyPressed(KEY_NULL) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || 
        IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || GetMouseDelta().x != 0 || GetMouseDelta().y != 0) {
        time_since_last_input = 0.0;
    } else {
        time_since_last_input += GetFrameTime();
    }
    
    if (time_since_last_input > 60.0 && *current_app_state != STATE_KIOSK_MODE) {
        *current_app_state = STATE_KIOSK_MODE;
        reset_kiosk_timers();
    }
}

AppState update_app_state(AppState current_state, GameConfig* config, SimulationContext *sim_ctx, float delta_time, double current_time) {
    // KI-Agent unterstützt: Poll for network updates
    LeaderboardData lb;
    if (network_get_leaderboard(&lb)) {
        kiosk_ctrl.cached_lb = lb;
        printf("--- Leaderboard Received ---\n");
    }

    HighlightData hd;
    if (network_get_highlights(&hd)) {
        kiosk_ctrl.cached_highlights = hd;
        printf("--- Highlights Received: %d matches ---\n", hd.count);
        
        if (current_state == STATE_KIOSK_MODE) {
            for (int i = 0; i < 4 && i < hd.count; i++) {
                int stride = 50 + 2;
                World* w = kiosk_ctrl.sims[i].current_world;
                for (int k = 0; k < (50 + 2) * stride; k++) w->grid[k] = DEAD;
                if (w->chunk_map) memset(w->chunk_map, 0, w->chunk_rows * w->chunk_cols);
                for (int r = 0; r < 8; r++) {
                    for (int c = 0; c < 8; c++) {
                        if (hd.matches[i].seed_blue[r * 8 + c] == 1) {
                            w->grid[(r+21)*stride + (c+11)] = TEAM_BLUE;
                            activate_chunk_at(w, r+20, c+10);
                        }
                        if (hd.matches[i].seed_red[r * 8 + c] == 1) {
                            w->grid[(r+21)*stride + (c+31)] = TEAM_RED;
                            activate_chunk_at(w, r+20, c+30);
                        }
                    }
                }
            }
        }
    }

    switch (current_state) {
        case STATE_IGNITION:
            if (ignitionStartTime == 0.0) {
                ignitionStartTime = current_time;
                // Step 4.1: Allocate telemetry arrays if not allocated
                if (!config->history_red_pop) {
                    config->history_red_pop = (int*)malloc(config->max_rounds * sizeof(int));
                    config->history_blue_pop = (int*)malloc(config->max_rounds * sizeof(int));
                    config->history_count = 0;
                }
            }
            if (current_time - ignitionStartTime >= 3.0) {
                ignitionStartTime = 0.0;
                return STATE_RUNNING;
            }
            break;

        case STATE_RUNNING:
        case STATE_OBSERVER:
            if (config->is_paused) break; // Skip logic if paused

            timeAccumulator += delta_time;
            if (timeAccumulator >= config->delay_ms / 1000.0f) {
                timeAccumulator = 0.0f;
                
                update_generation_ctx(sim_ctx, &config->current_red_pop, &config->current_blue_pop);
                
                // Record Telemetry
                if (config->history_count < config->max_rounds) {
                    config->history_red_pop[config->history_count] = config->current_red_pop;
                    config->history_blue_pop[config->history_count] = config->current_blue_pop;
                    config->history_count++;
                }

                config->current_round++;
                
                if (config->current_round >= config->max_rounds ||
                    config->current_red_pop == 0 ||
                    config->current_blue_pop == 0) {
                    return STATE_FINISHED;
                }
            }
            break;
            
        case STATE_KIOSK_MODE:
            if (!kiosk_ctrl.initialized) {
                int w = GetScreenWidth();
                int h = GetScreenHeight();
                int pad = 12;
                int q_w = (w - pad * 3) / 2;
                int q_h = (h - pad * 3) / 2;
                for (int i = 0; i < 4; i++) {
                    int col = i % 2;
                    int row = i / 2;
                    float rx = pad + col * (q_w + pad);
                    float ry = pad + row * (q_h + pad);
                    init_simulation_context(&kiosk_ctrl.sims[i], 50, 50);
                    kiosk_ctrl.renders[i].viewport_bounds = (Rectangle){ rx, ry, (float)q_w, (float)q_h };
                    init_render_context(&kiosk_ctrl.renders[i], 50, 50, (Rectangle){ rx, ry, (float)q_w, (float)q_h });
                }
                kiosk_ctrl.initialized = true;
                reset_kiosk_timers();
            }
            
            kiosk_ctrl.state_timer += delta_time;
            
            if (kiosk_ctrl.current_sub_state == KIOSK_SUB_LEADERBOARD) {
                if (kiosk_ctrl.state_timer > 15.0f) {
                    kiosk_ctrl.current_sub_state = KIOSK_SUB_MULTICAM;
                    kiosk_ctrl.state_timer = 0.0f;
                    network_fetch_highlights_async();
                }
            } else if (kiosk_ctrl.current_sub_state == KIOSK_SUB_MULTICAM) {
                timeAccumulator += delta_time;
                if (timeAccumulator >= 0.1f) {
                    timeAccumulator = 0.0f;
                    int dummy_red, dummy_blue;
                    for (int i = 0; i < 4; i++) {
                        update_generation_ctx(&kiosk_ctrl.sims[i], &dummy_red, &dummy_blue);
                    }
                }
                
                if (kiosk_ctrl.state_timer > 30.0f) {
                    kiosk_ctrl.current_sub_state = KIOSK_SUB_LEADERBOARD;
                    kiosk_ctrl.state_timer = 0.0f;
                    network_fetch_leaderboard_async();
                }
                
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePos = GetMousePosition();
                    for (int i = 0; i < 4; i++) {
                        if (CheckCollisionPointRec(mousePos, kiosk_ctrl.renders[i].viewport_bounds)) {
                            // Check if world needs allocation (if we returned to menu via Q before)
                            if (sim_ctx->current_world == NULL) {
                                sim_ctx->current_world = create_world(config->rows, config->cols);
                            }
                            if (sim_ctx->next_world == NULL) {
                                sim_ctx->next_world = create_world(config->rows, config->cols);
                            }
                            
                            // Copy seed data into single-player ctx
                            int stride = config->cols + 2;
                            // Reset single-player world
                            for (int k = 0; k < (config->rows + 2) * stride; k++) {
                                sim_ctx->current_world->grid[k] = DEAD;
                            }
                            if (sim_ctx->current_world->chunk_map) {
                                memset(sim_ctx->current_world->chunk_map, 0, sim_ctx->current_world->chunk_rows * sim_ctx->current_world->chunk_cols);
                            }
                            
                            // To match Kiosk multicam, inject at center
                            int center_r = config->rows / 2 - 4;
                            int center_c_b = config->cols / 4 - 4;
                            int center_c_r = config->cols * 3 / 4 - 4;
                            
                            for (int r = 0; r < 8; r++) {
                                for (int c = 0; c < 8; c++) {
                                    int src_stride = 50 + 2;
                                    World* kiosk_w = kiosk_ctrl.sims[i].current_world;
                                    if (kiosk_w->grid[(r+21)*src_stride + (c+11)] == TEAM_BLUE) {
                                        sim_ctx->current_world->grid[(center_r + r + 1)*stride + (center_c_b + c + 1)] = TEAM_BLUE;
                                        activate_chunk_at(sim_ctx->current_world, center_r + r, center_c_b + c);
                                    }
                                    if (kiosk_w->grid[(r+21)*src_stride + (c+31)] == TEAM_RED) {
                                        sim_ctx->current_world->grid[(center_r + r + 1)*stride + (center_c_r + c + 1)] = TEAM_RED;
                                        activate_chunk_at(sim_ctx->current_world, center_r + r, center_c_r + c);
                                    }
                                }
                            }
                            
                            config->current_red_pop = 1;
                            config->current_blue_pop = 1;
                            config->current_round = 0;
                            config->is_paused = false;
                            time_since_last_input = 0.0;
                            return STATE_IGNITION;
                        }
                    }
                }
            }
            break;
            
        default:
            break;
    }
    return current_state;
}