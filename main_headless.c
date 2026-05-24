#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "core_types.h"
#include "game_logic.h"
#include "file_io.h"

// KI-Agent unterstützt: Minimalist entry point for headless simulation.
typedef struct {
    char player_id[64];
    char nickname[64];
    int final_population;
} PlayerInfo;

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: ./biotope_headless <red_config.json> <blue_config.json> [output.json]\n");
        return 1;
    }

    const char *red_path = argv[1];
    const char *blue_path = argv[2];

    PlayerInfo red_info = { "unknown", "unknown", 0 };
    PlayerInfo blue_info = { "unknown", "unknown", 0 };

    // Phase 2.3: Initialize World
    int rows = LOCAL_GRID_SIZE;
    int cols = LOCAL_GRID_SIZE * 2;
    World *current_gen = create_world(rows, cols);
    World *next_gen = create_world(rows, cols);

    // Parse Red
    initialize_world_from_file(red_path, current_gen, TEAM_RED, red_info.player_id, red_info.nickname);

    // Parse Blue
    initialize_world_from_file(blue_path, current_gen, TEAM_BLUE, blue_info.player_id, blue_info.nickname);

    // Phase 3.1: The Simulation Loop (100 Generations)
    int rp = 0, bp = 0;
    for (int i = 0; i < 100; i++) {
        update_generation(current_gen, next_gen, rows, cols, &rp, &bp);
        World *temp = current_gen;
        current_gen = next_gen;
        next_gen = temp;
    }

    // Phase 3.2: Final Population Count
    for (int r = 1; r <= rows; r++) {
        for (int c = 1; c <= cols; c++) {
            int cell = current_gen->grid[r * (cols + 2) + c];
            if (cell == TEAM_RED) red_info.final_population++;
            else if (cell == TEAM_BLUE) blue_info.final_population++;
        }
    }

    const char *winner = "draw";
    if (red_info.final_population > blue_info.final_population) winner = "red";
    else if (blue_info.final_population > red_info.final_population) winner = "blue";

    // Phase 4.1: Generate Output JSON via file_io
    const char *output_path = (argc >= 4) ? argv[3] : NULL;
    save_headless_results(output_path, winner, 100, 
                          red_info.player_id, red_info.nickname, red_info.final_population,
                          blue_info.player_id, blue_info.nickname, blue_info.final_population);

    // Phase 4.3: Final Cleanup
    free_world(current_gen);
    free_world(next_gen);
    return 0;
}
