#include "app_state_manager.h"
#include "game_logic.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// KI-Agent unterstützt: App state manager
static double ignitionStartTime = 0.0;
static float timeAccumulator = 0.0f;

AppState update_app_state(AppState current_state, GameConfig* config, World** current_world, World** next_world, float delta_time, double current_time) {
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
            timeAccumulator += delta_time;
            if (timeAccumulator >= config->delay_ms / 1000.0f) {
                timeAccumulator = 0.0f;
                
                update_generation(*current_world, *next_world, config->rows, config->cols, &config->current_red_pop, &config->current_blue_pop);
                
                // Record Telemetry
                if (config->history_count < config->max_rounds) {
                    config->history_red_pop[config->history_count] = config->current_red_pop;
                    config->history_blue_pop[config->history_count] = config->current_blue_pop;
                    config->history_count++;
                }

                // Pointer Swap (Double Buffering)
                World *temp = *current_world;
                *current_world = *next_world;
                *next_world = temp;

                config->current_round++;
                
                if (config->current_round >= config->max_rounds ||
                    config->current_red_pop == 0 ||
                    config->current_blue_pop == 0) {
                    return STATE_FINISHED;
                }
            }
            break;
            
        default:
            break;
    }
    return current_state;
}