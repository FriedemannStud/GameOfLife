#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "core_types.h"
#include "game_logic.h"
#include "file_io.h"
#include "cJSON.h"

// KI-Agent unterstützt: Minimalist entry point for headless simulation.
typedef struct {
    char player_id[64];
    char nickname[64];
    int final_population;
} PlayerInfo;

// KI-Agent unterstützt: capture the current board as a compact "0/1/2" string
// (row-major over the rows x cols playfield, '0'=dead, '1'=red, '2'=blue) so the
// duel front-end can replay the deterministic match frame by frame (ADR-0028 §2.2).
static cJSON *capture_frame(const World *world, int rows, int cols) {
    char buf[LOCAL_GRID_SIZE * LOCAL_GRID_SIZE * 2 + 1]; // rows*cols (8*16) + NUL
    int stride = cols + 2;                                // grid has a 1-cell ghost border
    int n = 0;
    for (int r = 1; r <= rows; r++) {
        for (int c = 1; c <= cols; c++) {
            int cell = world->grid[r * stride + c];
            buf[n++] = (char)('0' + cell);
        }
    }
    buf[n] = '\0';
    return cJSON_CreateString(buf);
}

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

    // KI-Agent unterstützt: optional per-generation frame capture for the duel
    // replay. Enabled by an extra positional argument (argv[4] = frames output
    // path); when absent, behaviour is unchanged (backward compatible).
    const char *frames_path = (argc >= 5) ? argv[4] : NULL;
    cJSON *frames_arr = frames_path ? cJSON_CreateArray() : NULL;
    if (frames_arr) {
        cJSON_AddItemToArray(frames_arr, capture_frame(current_gen, rows, cols)); // gen 0
    }

    // Phase 3.1: The Simulation Loop (100 Generations)
    int rp = 0, bp = 0;
    for (int i = 0; i < 100; i++) {
        update_generation(current_gen, next_gen, rows, cols, &rp, &bp);
        World *temp = current_gen;
        current_gen = next_gen;
        next_gen = temp;
        if (frames_arr) {
            cJSON_AddItemToArray(frames_arr, capture_frame(current_gen, rows, cols));
        }
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

    // KI-Agent unterstützt: write the captured frames as a separate file so the
    // stable result schema stays untouched. Format: {rows, cols, frames:[str,...]}.
    if (frames_arr) {
        cJSON *frames_root = cJSON_CreateObject();
        cJSON_AddNumberToObject(frames_root, "rows", rows);
        cJSON_AddNumberToObject(frames_root, "cols", cols);
        cJSON_AddItemToObject(frames_root, "frames", frames_arr); // ownership transferred
        char *frames_json = cJSON_PrintUnformatted(frames_root);
        if (frames_json) {
            FILE *ff = fopen(frames_path, "w");
            if (ff) {
                fputs(frames_json, ff);
                fclose(ff);
            } else {
                fprintf(stderr, "Error: Could not open frames file %s\n", frames_path);
            }
            free(frames_json);
        }
        cJSON_Delete(frames_root); // also frees frames_arr
    }

    // Phase 4.3: Final Cleanup
    free_world(current_gen);
    free_world(next_gen);
    return 0;
}
