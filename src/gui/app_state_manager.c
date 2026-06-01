#include "app_state_manager.h"
#include "game_logic.h"
#include "network_io.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// KI-Agent unterstützt: App state manager (ADR-0020)
static double ignitionStartTime = 0.0;
static float kiosk_time_accumulator = 0.0f;
static float interactive_time_accumulator = 0.0f;

// KI-Agent unterstützt: Ignition time accessors — single source of truth (ADR-0020)
double get_ignition_start_time(void) { return ignitionStartTime; }
void set_ignition_start_time(double t) { ignitionStartTime = t; }

// KI-Agent unterstützt: Dynamic arrays replace static [4] buffers (ADR-0022)
// Resources are allocated in init_kiosk_controller and freed in free_kiosk_controller.
KioskController kiosk_ctrl = {
    .current_sub_state = KIOSK_SUB_LEADERBOARD,
    .state_timer       = 0.0f,
    .match_count       = 0,
    .renders           = NULL,
    .sims              = NULL,
    .initialized       = false,
    .quad_red_pop      = NULL,
    .quad_blue_pop     = NULL
};

// KI-Agent unterstützt: Free all GPU and simulation resources owned by the kiosk (ADR-0022)
// Safe to call only when ctrl->initialized == true (all arrays are valid).
void free_kiosk_controller(KioskController *ctrl) {
    if (!ctrl || !ctrl->initialized) return;

    for (int i = 0; i < ctrl->match_count; i++) {
        free_render_context(&ctrl->renders[i]);
        free_simulation_context(&ctrl->sims[i]);
    }

    free(ctrl->renders);        ctrl->renders       = NULL;
    free(ctrl->sims);           ctrl->sims          = NULL;
    free(ctrl->quad_red_pop);   ctrl->quad_red_pop  = NULL;
    free(ctrl->quad_blue_pop);  ctrl->quad_blue_pop = NULL;

    ctrl->match_count = 0;
    ctrl->initialized = false;
}

// KI-Agent unterstützt: Allocate and initialize all kiosk resources (ADR-0022)
// Viewport geometry is derived from compute_kiosk_layout — no magic pixel numbers here.
void init_kiosk_controller(KioskController *ctrl, int match_count,
                            int screen_w, int screen_h) {
    if (!ctrl) return;
    if (ctrl->initialized) free_kiosk_controller(ctrl);

    ctrl->match_count  = match_count;
    ctrl->sims         = (SimulationContext*)calloc(match_count, sizeof(SimulationContext));
    ctrl->renders      = (RenderContext*)calloc(match_count, sizeof(RenderContext));
    ctrl->quad_red_pop = (int*)calloc(match_count, sizeof(int));
    ctrl->quad_blue_pop = (int*)calloc(match_count, sizeof(int));

    // On partial allocation failure: clean up manually and bail out
    if (!ctrl->sims || !ctrl->renders || !ctrl->quad_red_pop || !ctrl->quad_blue_pop) {
        free(ctrl->sims);         ctrl->sims         = NULL;
        free(ctrl->renders);      ctrl->renders       = NULL;
        free(ctrl->quad_red_pop); ctrl->quad_red_pop  = NULL;
        free(ctrl->quad_blue_pop);ctrl->quad_blue_pop = NULL;
        ctrl->match_count = 0;
        return;
    }

    KioskLayout layout = compute_kiosk_layout(screen_w, screen_h, match_count);

    for (int i = 0; i < match_count; i++) {
        int col = i % layout.grid_cols;
        int row = i / layout.grid_cols;
        Rectangle vp = {
            (float)(layout.pad + col * (layout.quad_w + layout.pad)),
            (float)(layout.top_bar_h + row * (layout.quad_h + layout.pad)),
            (float)layout.quad_w,
            (float)layout.quad_h
        };
        init_simulation_context(&ctrl->sims[i], KIOSK_SIM_ROWS, KIOSK_SIM_COLS);
        init_render_context(&ctrl->renders[i],  KIOSK_SIM_COLS, KIOSK_SIM_ROWS, vp);
    }

    // KI-Agent unterstützt: Pool starts empty; populated on first API response (ADR-0024)
    ctrl->highlight_pool_index = 0;
    ctrl->highlight_pool_size  = 0;
    ctrl->last_epoch_id[0]     = '\0';   // empty string — any epoch_id triggers first reset
    ctrl->initialized = true;
}

// KI-Agent unterstützt: Loads match_count sims from the pool starting at highlight_pool_index (ADR-0024)
// Uses modulo wrap-around so the pool is cycled continuously.
static void load_kiosk_sims_from_pool(void) {
    if (kiosk_ctrl.highlight_pool_size == 0) return;  // guard: no data yet
    for (int i = 0; i < kiosk_ctrl.match_count; i++) {
        // Modulo-Arithmetik: Index springt am Pool-Ende wieder auf 0
        int slot = (kiosk_ctrl.highlight_pool_index + i) % kiosk_ctrl.highlight_pool_size;
        MatchHighlight *m = &kiosk_ctrl.cached_highlights.matches[slot];

        int stride = KIOSK_SIM_COLS + 2;
        World *w   = kiosk_ctrl.sims[i].current_world;
        // Grid leeren bevor neues Muster geladen wird
        for (int k = 0; k < (KIOSK_SIM_ROWS + 2) * stride; k++) w->grid[k] = DEAD;
        if (w->chunk_map) memset(w->chunk_map, 0, w->chunk_rows * w->chunk_cols);

        for (int r = 0; r < LOCAL_GRID_SIZE; r++) {
            for (int c = 0; c < LOCAL_GRID_SIZE; c++) {
                if (m->seed_red[r * LOCAL_GRID_SIZE + c] == 1) {
                    w->grid[(r+1)*stride + (c+1)] = TEAM_RED;
                    activate_chunk_at(w, r, c);
                }
                if (m->seed_blue[r * LOCAL_GRID_SIZE + c] == 1) {
                    w->grid[(r+1)*stride + (c+LOCAL_GRID_SIZE+1)] = TEAM_BLUE;
                    activate_chunk_at(w, r, c + LOCAL_GRID_SIZE);
                }
            }
        }
        // Spielernamen in den SimulationContext übertragen (für HUD-Anzeige)
        strncpy(kiosk_ctrl.sims[i].participant_red,  m->participant_red,
                sizeof(kiosk_ctrl.sims[i].participant_red) - 1);
        kiosk_ctrl.sims[i].participant_red[sizeof(kiosk_ctrl.sims[i].participant_red) - 1] = '\0';
        strncpy(kiosk_ctrl.sims[i].participant_blue, m->participant_blue,
                sizeof(kiosk_ctrl.sims[i].participant_blue) - 1);
        kiosk_ctrl.sims[i].participant_blue[sizeof(kiosk_ctrl.sims[i].participant_blue) - 1] = '\0';
    }
}

static double time_since_last_input = 0.0;

void reset_kiosk_timers(void) {
    kiosk_ctrl.state_timer = 0.0f;
    kiosk_ctrl.current_sub_state = KIOSK_SUB_LEADERBOARD;
    // KI-Agent unterstützt: Clear stale shader trail buffers to avoid visual artifacts (ADR-0020)
    if (kiosk_ctrl.initialized) {
        for (int i = 0; i < kiosk_ctrl.match_count; i++) {
            clear_render_context_trail(&kiosk_ctrl.renders[i]);
        }
    }
    network_fetch_leaderboard_async();
}

// KI-Agent unterstützt: Centralized cleanup for interactive sessions (ADR-0020)
void cleanup_interactive_session(SimulationContext *sim, GameConfig *config, RenderContext *r_ctx) {
    // 1. Free simulation worlds (keep SimulationContext struct alive)
    if (sim->world_a) { free_world(sim->world_a); sim->world_a = NULL; }
    if (sim->world_b) { free_world(sim->world_b); sim->world_b = NULL; }
    sim->current_world = NULL;
    sim->next_world = NULL;
    sim->current_generation = 0;
    sim->is_active = false;

    // 2. Free telemetry arrays
    if (config->history_red_pop) { free(config->history_red_pop); config->history_red_pop = NULL; }
    if (config->history_blue_pop) { free(config->history_blue_pop); config->history_blue_pop = NULL; }
    config->history_count = 0;

    // 3. Reset config counters
    config->current_red_pop = 0;
    config->current_blue_pop = 0;
    config->current_round = 0;
    config->is_paused = false;

    // 4. Reset camera
    if (r_ctx) {
        r_ctx->camera.zoom = 1.0f;
        r_ctx->camera.target = (Vector2){ 0, 0 };
        r_ctx->camera.offset = (Vector2){ 0, 0 };
    }
}

// KI-Agent unterstützt: Restore main-game grid (50x50) after a kiosk replay (ADR-0024)
// Only acts when config was changed to 8x16 for the kiosk replay.
void restore_main_game_context(GameConfig *config, RenderContext *r_ctx) {
    if (config->rows != DEFAULT_GRID_ROWS || config->cols != DEFAULT_GRID_COLS) {
        config->rows = DEFAULT_GRID_ROWS;
        config->cols = DEFAULT_GRID_COLS;
        free_render_context(r_ctx);
        Rectangle vp = { 20, 60,
                         (float)(GetScreenWidth()  - 40),
                         (float)(GetScreenHeight() - 120) };
        init_render_context(r_ctx, DEFAULT_GRID_COLS, DEFAULT_GRID_ROWS, vp);
    }
}

void update_global_input(AppState* current_app_state, SimulationContext *sim,
                         GameConfig *config, RenderContext *r_ctx) {
    if (IsKeyPressed(KEY_NULL) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || 
        IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || GetMouseDelta().x != 0 || GetMouseDelta().y != 0) {
        time_since_last_input = 0.0;
    } else {
        time_since_last_input += GetFrameTime();
    }
    
    if (time_since_last_input > 60.0 && *current_app_state != STATE_KIOSK_MODE) {
        // KI-Agent unterstützt: Clean up interactive session before forced kiosk return (ADR-0020)
        restore_main_game_context(config, r_ctx);  // no-op if not a kiosk replay
        cleanup_interactive_session(sim, config, r_ctx);
        set_ignition_start_time(0.0);
        interactive_time_accumulator = 0.0f;
        *current_app_state = STATE_KIOSK_MODE;
        reset_kiosk_timers();
    }
}

AppState update_app_state(AppState current_state, GameConfig* config, SimulationContext *sim_ctx,
                          RenderContext *r_ctx, float delta_time, double current_time,
                          SessionOrigin *session_origin) {
    // KI-Agent unterstützt: Poll for network updates
    LeaderboardData lb;
    if (network_get_leaderboard(&lb)) {
        kiosk_ctrl.cached_lb = lb;
        printf("--- Leaderboard Received ---\n");
    }

    HighlightData hd;
    if (network_get_highlights(&hd)) {
        // KI-Agent unterstützt: Pool resets only when epoch_id changes — preserves rotation
        // within an epoch; resets correctly on epoch boundary or first load (ADR-0024)
        bool new_epoch = (strcmp(hd.epoch_id, kiosk_ctrl.last_epoch_id) != 0);
        kiosk_ctrl.cached_highlights   = hd;
        kiosk_ctrl.highlight_pool_size = hd.count;
        if (new_epoch) {
            kiosk_ctrl.highlight_pool_index = 0;
            strncpy(kiosk_ctrl.last_epoch_id, hd.epoch_id, sizeof(kiosk_ctrl.last_epoch_id) - 1);
            kiosk_ctrl.last_epoch_id[sizeof(kiosk_ctrl.last_epoch_id) - 1] = '\0';
            printf("--- Highlights: %d matches (new epoch '%s', pool reset) ---\n",
                   hd.count, hd.epoch_id);
            if (current_state == STATE_KIOSK_MODE) {
                load_kiosk_sims_from_pool();
            }
        } else {
            printf("--- Highlights: %d matches (same epoch, index=%d kept) ---\n",
                   hd.count, kiosk_ctrl.highlight_pool_index);
        }
    }

    switch (current_state) {
        case STATE_IGNITION:
            if (ignitionStartTime == 0.0) {
                ignitionStartTime = current_time;
            }
            // KI-Agent unterstützt: Allocate telemetry separately from timer init (ADR-0020)
            // Fix: process_ui_events sets ignitionStartTime first, so the == 0.0 check above
            // would be skipped on the first frame — telemetry must be checked independently.
            if (!config->history_red_pop) {
                config->history_red_pop = (int*)malloc(config->max_rounds * sizeof(int));
                config->history_blue_pop = (int*)malloc(config->max_rounds * sizeof(int));
                config->history_count = 0;
            }
            if (current_time - ignitionStartTime >= 3.0) {
                ignitionStartTime = 0.0;
                return STATE_RUNNING;
            }
            break;

        case STATE_RUNNING:
        case STATE_OBSERVER:
            if (config->is_paused) break; // Skip logic if paused

            interactive_time_accumulator += delta_time;
            if (interactive_time_accumulator >= config->delay_ms / 1000.0f) {
                interactive_time_accumulator = 0.0f;
                
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
                // KI-Agent unterstützt: Replaced inline geometry with init_kiosk_controller (ADR-0022)
                init_kiosk_controller(&kiosk_ctrl, KIOSK_DEFAULT_MATCH_COUNT,
                                      GetScreenWidth(), GetScreenHeight());
                reset_kiosk_timers();
            }
            
            kiosk_ctrl.state_timer += delta_time;
            
            if (kiosk_ctrl.current_sub_state == KIOSK_SUB_LEADERBOARD) {
                if (kiosk_ctrl.state_timer > 15.0f) {
                    kiosk_ctrl.current_sub_state = KIOSK_SUB_MULTICAM;
                    kiosk_ctrl.state_timer = 0.0f;
                    network_fetch_highlights_async();
                    // KI-Agent unterstützt: Load the next rotation window from the pool (ADR-0024)
                    load_kiosk_sims_from_pool();
                }
            } else if (kiosk_ctrl.current_sub_state == KIOSK_SUB_MULTICAM) {
                kiosk_time_accumulator += delta_time;
                if (kiosk_time_accumulator >= KIOSK_SIM_GEN_INTERVAL_S) {
                    kiosk_time_accumulator = 0.0f;
                    // KI-Agent unterstützt: Store per-quadrant pop for score bar (ADR-0021)
                    for (int i = 0; i < kiosk_ctrl.match_count; i++) {
                        update_generation_ctx(&kiosk_ctrl.sims[i],
                                              &kiosk_ctrl.quad_red_pop[i],
                                              &kiosk_ctrl.quad_blue_pop[i]);
                    }
                }
                
                if (kiosk_ctrl.state_timer > 30.0f) {
                    // KI-Agent unterstützt: Advance round-robin index before switching away (ADR-0024)
                    if (kiosk_ctrl.highlight_pool_size > 0) {
                        kiosk_ctrl.highlight_pool_index =
                            (kiosk_ctrl.highlight_pool_index + kiosk_ctrl.match_count)
                            % kiosk_ctrl.highlight_pool_size;
                    }
                    kiosk_ctrl.current_sub_state = KIOSK_SUB_LEADERBOARD;
                    kiosk_ctrl.state_timer = 0.0f;
                    network_fetch_leaderboard_async();
                }
                
                // KI-Agent unterstützt: Keyboard match selection replaces mouse click (ADR-0022)
                // KEY_ONE + i maps to keys 1, 2, 3, 4 ... (Raylib digit key constants are sequential).
                for (int i = 0; i < kiosk_ctrl.match_count; i++) {
                    if (IsKeyPressed(KEY_ONE + i)) {
                        // KI-Agent unterstützt: Resolve quadrant index to current pool slot (ADR-0024)
                        int slot = (kiosk_ctrl.highlight_pool_size > 0)
                            ? (kiosk_ctrl.highlight_pool_index + i) % kiosk_ctrl.highlight_pool_size
                            : i;
                        // Namen aus dem Pool-Slot in den Replay-Context kopieren (für STATE_RUNNING HUD)
                        strncpy(sim_ctx->participant_red,
                                kiosk_ctrl.cached_highlights.matches[slot].participant_red,
                                sizeof(sim_ctx->participant_red) - 1);
                        sim_ctx->participant_red[sizeof(sim_ctx->participant_red) - 1] = '\0';
                        strncpy(sim_ctx->participant_blue,
                                kiosk_ctrl.cached_highlights.matches[slot].participant_blue,
                                sizeof(sim_ctx->participant_blue) - 1);
                        sim_ctx->participant_blue[sizeof(sim_ctx->participant_blue) - 1] = '\0';
                        // KI-Agent unterstützt: Run replay on the same 8x16 grid as the tournament (ADR-0024)
                        // 1. Resize config so DrawGridAndCellsCtx and update_generation use 8x16.
                        config->rows = KIOSK_SIM_ROWS;
                        config->cols = KIOSK_SIM_COLS;
                        // 2. Reinitialise GPU render context for 8x16 (pixel buffer & texture must match world).
                        free_render_context(r_ctx);
                        Rectangle kiosk_vp = { 20, 60,
                                               (float)(GetScreenWidth()  - 40),
                                               (float)(GetScreenHeight() - 120) };
                        init_render_context(r_ctx, KIOSK_SIM_COLS, KIOSK_SIM_ROWS, kiosk_vp);
                        // 3. Reset sim to 8x16.
                        reset_simulation_context(sim_ctx, KIOSK_SIM_ROWS, KIOSK_SIM_COLS);

                        int stride = config->cols + 2;  // 18
                        for (int k = 0; k < (config->rows + 2) * stride; k++) {
                            sim_ctx->current_world->grid[k] = DEAD;
                        }
                        if (sim_ctx->current_world->chunk_map) {
                            memset(sim_ctx->current_world->chunk_map, 0,
                                   sim_ctx->current_world->chunk_rows * sim_ctx->current_world->chunk_cols);
                        }

                        // Centering math reduces to (0, 0, 8) for 8x16 — same layout as kiosk sims.
                        int center_r   = config->rows / 2 - 4;       // 0
                        int center_c_b = config->cols / 4 - 4;        // 0  (blue: left half)
                        int center_c_r = config->cols * 3 / 4 - 4;   // 8  (red:  right half)

                        // KI-Agent unterstützt: Read original seed — not the live simulation state (ADR-0022 fix)
                        // cached_highlights holds the 8x8 start pattern; current_world is mid-game.
                        for (int r = 0; r < 8; r++) {
                            for (int c = 0; c < 8; c++) {
                                if (kiosk_ctrl.cached_highlights.matches[slot].seed_blue[r * 8 + c]) {
                                    sim_ctx->current_world->grid[(center_r + r + 1)*stride + (center_c_b + c + 1)] = TEAM_BLUE;
                                    activate_chunk_at(sim_ctx->current_world, center_r + r, center_c_b + c);
                                }
                                if (kiosk_ctrl.cached_highlights.matches[slot].seed_red[r * 8 + c]) {
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
                        // KI-Agent unterstützt: Mark this session as a Kiosk replay (ADR-0020)
                        if (session_origin) *session_origin = ORIGIN_KIOSK_REPLAY;
                        interactive_time_accumulator = 0.0f;
                        return STATE_IGNITION;
                    }
                }
            }
            break;
            
        default:
            break;
    }
    return current_state;
}