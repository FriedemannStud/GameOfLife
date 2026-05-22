#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "game_logic.h"
#include "cJSON.h"

// KI-Agent unterstützt: Helper to format ISO 8601 time string
static void get_iso8601_time(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = gmtime(&now);
    strftime(buf, size, "%Y-%m-%dT%H:%M:%SZ", tm_info);
}

// KI-Agent unterstützt: Helper to read a file into a string
static char* read_file_to_string(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = malloc(length + 1);
    if (buffer) {
        size_t read_bytes = fread(buffer, 1, length, f);
        buffer[read_bytes] = '\0';
    }
    fclose(f);
    return buffer;
}

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
    int rows = 8;
    int cols = 16;
    World *current_gen = create_world(rows, cols);
    World *next_gen = create_world(rows, cols);

    // Parse Red
    char *red_json_str = read_file_to_string(red_path);
    if (red_json_str) {
        cJSON *root = cJSON_Parse(red_json_str);
        if (root) {
            cJSON *meta = cJSON_GetObjectItemCaseSensitive(root, "metadata");
            if (meta) {
                cJSON *pid = cJSON_GetObjectItemCaseSensitive(meta, "player_id");
                cJSON *nick = cJSON_GetObjectItemCaseSensitive(meta, "nickname");
                if (pid) strncpy(red_info.player_id, pid->valuestring, 63);
                if (nick) strncpy(red_info.nickname, nick->valuestring, 63);
            }
            cJSON *config = cJSON_GetObjectItemCaseSensitive(root, "config");
            cJSON *cells = cJSON_GetObjectItemCaseSensitive(config, "cells");
            cJSON *cell;
            cJSON_ArrayForEach(cell, cells) {
                // Support both [x, y] and [x, y, team]
                int x = cJSON_GetArrayItem(cell, 0)->valueint;
                int y = cJSON_GetArrayItem(cell, 1)->valueint;
                if (x >= 0 && x < 8 && y >= 0 && y < 8) {
                    current_gen->grid[(y + 1) * (cols + 2) + (x + 1)] = TEAM_RED;
                }
            }
            cJSON_Delete(root);
        }
        free(red_json_str);
    }

    // Parse Blue
    char *blue_json_str = read_file_to_string(blue_path);
    if (blue_json_str) {
        cJSON *root = cJSON_Parse(blue_json_str);
        if (root) {
            cJSON *meta = cJSON_GetObjectItemCaseSensitive(root, "metadata");
            if (meta) {
                cJSON *pid = cJSON_GetObjectItemCaseSensitive(meta, "player_id");
                cJSON *nick = cJSON_GetObjectItemCaseSensitive(meta, "nickname");
                if (pid) strncpy(blue_info.player_id, pid->valuestring, 63);
                if (nick) strncpy(blue_info.nickname, nick->valuestring, 63);
            }
            cJSON *config = cJSON_GetObjectItemCaseSensitive(root, "config");
            cJSON *cells = cJSON_GetObjectItemCaseSensitive(config, "cells");
            cJSON *cell;
            cJSON_ArrayForEach(cell, cells) {
                int x = cJSON_GetArrayItem(cell, 0)->valueint;
                int y = cJSON_GetArrayItem(cell, 1)->valueint;
                if (x >= 0 && x < 8 && y >= 0 && y < 8) {
                    current_gen->grid[(y + 1) * (cols + 2) + (x + 8 + 1)] = TEAM_BLUE;
                }
            }
            cJSON_Delete(root);
        }
        free(blue_json_str);
    }

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

    // Phase 4.1: Generate Output JSON
    cJSON *result_root = cJSON_CreateObject();
    cJSON_AddStringToObject(result_root, "winner", winner);
    cJSON_AddNumberToObject(result_root, "generations", 100);
    
    char timestamp[64];
    get_iso8601_time(timestamp, sizeof(timestamp));
    cJSON_AddStringToObject(result_root, "timestamp", timestamp);

    cJSON *red_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(red_obj, "player_id", red_info.player_id);
    cJSON_AddStringToObject(red_obj, "nickname", red_info.nickname);
    cJSON_AddNumberToObject(red_obj, "population", red_info.final_population);
    cJSON_AddItemToObject(result_root, "red", red_obj);

    cJSON *blue_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(blue_obj, "player_id", blue_info.player_id);
    cJSON_AddStringToObject(blue_obj, "nickname", blue_info.nickname);
    cJSON_AddNumberToObject(blue_obj, "population", blue_info.final_population);
    cJSON_AddItemToObject(result_root, "blue", blue_obj);

    char *json_output = cJSON_Print(result_root);

    // Phase 4.2: Write to Destination
    const char *output_path = (argc >= 4) ? argv[3] : NULL;
    if (output_path) {
        FILE *f = fopen(output_path, "w");
        if (f) {
            fputs(json_output, f);
            fclose(f);
            printf("Results saved to: %s\n", output_path);
        } else {
            fprintf(stderr, "Error: Could not open output file %s\n", output_path);
            printf("%s\n", json_output); // Fallback to stdout
        }
    } else {
        printf("%s\n", json_output);
    }

    // Phase 4.3: Final Cleanup
    free(json_output);
    cJSON_Delete(result_root);
    free_world(current_gen);
    free_world(next_gen);
    return 0;
}
