#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../app_state_manager.h"
#include "../game_logic.h"

#define TEST(name) void test_##name()
#define RUN_TEST(name) \
    printf("Running %s... ", #name); \
    test_##name(); \
    printf("PASSED\n");

TEST(ignition_to_running) {
    GameConfig config = {0};
    World *current = create_world(10, 10);
    World *next = create_world(10, 10);
    
    // First call should initialize timer and return STATE_IGNITION
    AppState state = update_app_state(STATE_IGNITION, &config, &current, &next, 0.016f, 10.0);
    assert(state == STATE_IGNITION);
    
    // Call 2 seconds later (not enough time)
    state = update_app_state(STATE_IGNITION, &config, &current, &next, 0.016f, 12.0);
    assert(state == STATE_IGNITION);
    
    // Call 3 seconds later (enough time)
    state = update_app_state(STATE_IGNITION, &config, &current, &next, 0.016f, 13.0);
    assert(state == STATE_RUNNING);
    
    // Telemetry should be allocated
    assert(config.history_red_pop != NULL);
    assert(config.history_blue_pop != NULL);
    
    free(config.history_red_pop);
    free(config.history_blue_pop);
    free_world(current);
    free_world(next);
}

TEST(running_to_finished_max_rounds) {
    GameConfig config = {0};
    config.delay_ms = 100;
    config.rows = 10;
    config.cols = 10;
    config.max_rounds = 5;
    config.current_round = 0;
    config.current_red_pop = 10;
    config.current_blue_pop = 10;
    config.history_red_pop = calloc(5, sizeof(int));
    config.history_blue_pop = calloc(5, sizeof(int));

    World *current = create_world(10, 10);
    World *next = create_world(10, 10);

    // Place stable 2x2 block for red
    current->grid[2 * 12 + 2] = TEAM_RED;
    current->grid[2 * 12 + 3] = TEAM_RED;
    current->grid[3 * 12 + 2] = TEAM_RED;
    current->grid[3 * 12 + 3] = TEAM_RED;

    // Place stable 2x2 block for blue
    current->grid[2 * 12 + 8] = TEAM_BLUE;
    current->grid[2 * 12 + 9] = TEAM_BLUE;
    current->grid[3 * 12 + 8] = TEAM_BLUE;
    current->grid[3 * 12 + 9] = TEAM_BLUE;

    // Activate chunks
    activate_chunk_at(current, 1, 1);
    activate_chunk_at(current, 1, 7);

    // Call with 0.1s delta (enough to trigger a tick)
    AppState state = STATE_RUNNING;
    for (int i=0; i<5; i++) {
        state = update_app_state(state, &config, &current, &next, 0.1f, 0.0);
        printf("i=%d, red_pop=%d, blue_pop=%d, round=%d, state=%d\n", i, config.current_red_pop, config.current_blue_pop, config.current_round, state);
        if (i < 4) assert(state == STATE_RUNNING);
        else assert(state == STATE_FINISHED); // 5th tick finishes
    }
    
    free(config.history_red_pop);
    free(config.history_blue_pop);
    free_world(current);
    free_world(next);
}

TEST(running_to_finished_extinction) {
    GameConfig config = {0};
    config.delay_ms = 100;
    config.rows = 10;
    config.cols = 10;
    config.max_rounds = 100;
    config.current_round = 0;
    config.current_red_pop = 10;
    config.current_blue_pop = 10;
    config.history_red_pop = calloc(100, sizeof(int));
    config.history_blue_pop = calloc(100, sizeof(int));

    World *current = create_world(10, 10);
    World *next = create_world(10, 10);

    // Place stable 2x2 blocks so first tick survives
    current->grid[2 * 12 + 2] = TEAM_RED;
    current->grid[2 * 12 + 3] = TEAM_RED;
    current->grid[3 * 12 + 2] = TEAM_RED;
    current->grid[3 * 12 + 3] = TEAM_RED;

    current->grid[2 * 12 + 8] = TEAM_BLUE;
    current->grid[2 * 12 + 9] = TEAM_BLUE;
    current->grid[3 * 12 + 8] = TEAM_BLUE;
    current->grid[3 * 12 + 9] = TEAM_BLUE;
    activate_chunk_at(current, 1, 1);
    activate_chunk_at(current, 1, 7);

    // 1 tick, population still > 0
    AppState state = update_app_state(STATE_RUNNING, &config, &current, &next, 0.1f, 0.0);
    assert(state == STATE_RUNNING);
    
    // Simulate population dropping to 0 for red by wiping grid
    for(int i=0; i<12*12; i++) current->grid[i] = DEAD;
    activate_chunk_at(current, 1, 1);
    activate_chunk_at(current, 1, 7);

    state = update_app_state(STATE_RUNNING, &config, &current, &next, 0.1f, 0.0);
    assert(state == STATE_FINISHED);
    
    free(config.history_red_pop);
    free(config.history_blue_pop);
    free_world(current);
    free_world(next);
}

int main(void) {
    printf("=== Starting App State Manager DEV_TEST ===\n");
    RUN_TEST(ignition_to_running);
    RUN_TEST(running_to_finished_max_rounds);
    RUN_TEST(running_to_finished_extinction);
    printf("=== All App State Manager DEV_TESTs PASSED ===\n");
    return 0;
}