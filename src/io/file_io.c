#include <stdio.h>
#include <time.h>
#include <stdlib.h> // For malloc/free
#include <string.h>
#include <dirent.h>
#include "file_io.h"
#include "cJSON.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

// Helper to write string to file
static int write_string_to_file(const char *filename, const char *str) {
    FILE *f = fopen(filename, "w");
    if (!f) return 0;
    fputs(str, f);
    fclose(f);
    return 1;
}

// Helper to read file to string
static char* read_file_to_string(const char *filename) {
    FILE *f = fopen(filename, "r");
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

static int compare_protocol_info(const void *a, const void *b) {
    ProtocolInfo *pa = (ProtocolInfo *)a;
    ProtocolInfo *pb = (ProtocolInfo *)b;
    if (pb->timestamp > pa->timestamp) return 1;
    if (pb->timestamp < pa->timestamp) return -1;
    return 0;
}

// Format ISO 8601 string
static void get_iso8601_time(char *buf, size_t size, time_t t) {
    struct tm *tm_info = gmtime(&t);
    strftime(buf, size, "%Y-%m-%dT%H:%M:%SZ", tm_info);
}

// KI-Agent unterstützt: Unified JSON save logic (Hard Cut)
int save_grid(const char *filename, World *w, GameConfig *c) {
    cJSON *root = cJSON_CreateObject();
    
    // Metadata
    cJSON *metadata = cJSON_CreateObject();
    cJSON_AddStringToObject(metadata, "player_id", "local_user");
    cJSON_AddStringToObject(metadata, "nickname", "local");
    cJSON_AddStringToObject(metadata, "league", "local");
    
    time_t now = time(NULL);
    char time_str[64];
    get_iso8601_time(time_str, sizeof(time_str), now);
    cJSON_AddStringToObject(metadata, "timestamp", time_str);
    cJSON_AddNumberToObject(metadata, "unix_timestamp", (double)now);
    
    cJSON_AddItemToObject(root, "metadata", metadata);
    
    // Config
    cJSON *config = cJSON_CreateObject();
    cJSON_AddNumberToObject(config, "bounding_box_x", c->cols);
    cJSON_AddNumberToObject(config, "bounding_box_y", c->rows);
    cJSON_AddNumberToObject(config, "max_rounds", c->max_rounds);
    cJSON_AddNumberToObject(config, "max_population", c->max_population);
    cJSON_AddNumberToObject(config, "delay_ms", c->delay_ms);
    
    cJSON *cells = cJSON_CreateArray();
    int cell_count = 0;
    int stride = c->cols + 2;
    
    for(int r = 1; r <= c->rows; r++) {
        for(int col = 1; col <= c->cols; col++) {
            int idx = r * stride + col;
            if (w->grid[idx] != DEAD) {
                cJSON *cell = cJSON_CreateArray();
                cJSON_AddItemToArray(cell, cJSON_CreateNumber(col - 1)); // X
                cJSON_AddItemToArray(cell, cJSON_CreateNumber(r - 1));   // Y
                // Fallback team data for local preservation
                cJSON_AddItemToArray(cell, cJSON_CreateNumber(w->grid[idx])); 
                cJSON_AddItemToArray(cells, cell);
                cell_count++;
            }
        }
    }
    
    cJSON_AddNumberToObject(config, "cell_count", cell_count);
    cJSON_AddItemToObject(config, "cells", cells);
    cJSON_AddItemToObject(root, "config", config);
    
    char *json_string = cJSON_Print(root);
    int success = write_string_to_file(filename, json_string);
    
    free(json_string);
    cJSON_Delete(root);
    
    if (success) {
        printf("Saved JSON to %s\n", filename);
#if defined(PLATFORM_WEB)
        EM_ASM({
            FS.syncfs(false, function(err) {
                if (err) console.error("IndexedDB sync error:", err);
                else console.log("Biotope saved to IndexedDB");
            });
        });
#endif
    } else {
        printf("Error saving JSON to %s\n", filename);
    }
    
    return success;
}

// KI-Agent unterstützt: Unified JSON load logic (Hard Cut)
int load_grid(const char *filename, World *w, GameConfig *c) {
    char *json_string = read_file_to_string(filename);
    if (!json_string) {
        printf("Error opening file %s\n", filename);
        return 0;
    }
    
    cJSON *root = cJSON_Parse(json_string);
    free(json_string);
    
    if (!root) {
        printf("Error parsing JSON in %s\n", filename);
        return 0;
    }
    
    cJSON *config = cJSON_GetObjectItemCaseSensitive(root, "config");
    if (!config) {
        printf("Error: Invalid JSON schema (missing config)\n");
        cJSON_Delete(root);
        return 0;
    }
    
    int cols = cJSON_GetObjectItemCaseSensitive(config, "bounding_box_x")->valueint;
    int rows = cJSON_GetObjectItemCaseSensitive(config, "bounding_box_y")->valueint;
    
    // Check if loaded config matches current world size
    if (rows != c->rows || cols != c->cols) {
        printf("Resizing world from %dx%d to %dx%d...\n", c->rows, c->cols, rows, cols);
        free(w->grid);
        w->grid = (int*)calloc((rows + 2) * (cols + 2), sizeof(int));
        
        if (w->chunk_map) free(w->chunk_map);
        w->chunk_rows = (rows + CHUNK_SIZE - 1) / CHUNK_SIZE;
        w->chunk_cols = (cols + CHUNK_SIZE - 1) / CHUNK_SIZE;
        w->chunk_map = (unsigned char*)calloc(w->chunk_rows * w->chunk_cols, sizeof(unsigned char));

        w->rows = rows;
        w->cols = cols;
        c->rows = rows;
        c->cols = cols;
    }
    
    // Update config (with defaults if missing)
    cJSON *max_pop = cJSON_GetObjectItemCaseSensitive(config, "max_population");
    if (max_pop) c->max_population = max_pop->valueint;
    
    cJSON *max_rounds = cJSON_GetObjectItemCaseSensitive(config, "max_rounds");
    if (max_rounds) c->max_rounds = max_rounds->valueint;
    
    cJSON *delay_ms = cJSON_GetObjectItemCaseSensitive(config, "delay_ms");
    if (delay_ms) c->delay_ms = delay_ms->valueint;
    
    // Clear grid
    int stride = cols + 2;
    for(int i=0; i < (rows + 2) * (cols + 2); i++) w->grid[i] = DEAD;
    if (w->chunk_map) {
        for (int i = 0; i < w->chunk_rows * w->chunk_cols; i++) w->chunk_map[i] = 0;
    }

    c->current_blue_pop = 0;
    c->current_red_pop = 0;
    
    cJSON *cells = cJSON_GetObjectItemCaseSensitive(config, "cells");
    cJSON *cell = NULL;
    cJSON_ArrayForEach(cell, cells) {
        int x = cJSON_GetArrayItem(cell, 0)->valueint;
        int y = cJSON_GetArrayItem(cell, 1)->valueint;
        int team = TEAM_RED; // Default
        
        if (cJSON_GetArraySize(cell) > 2) {
            team = cJSON_GetArrayItem(cell, 2)->valueint;
        } else {
            // Server assigned color fallback for pure single-player drafting JSONs
            team = (x < cols / 2) ? TEAM_BLUE : TEAM_RED;
        }
        
        if (x >= 0 && x < cols && y >= 0 && y < rows) {
            int idx = (y + 1) * stride + (x + 1);
            w->grid[idx] = team;
            if (team == TEAM_RED) c->current_red_pop++;
            if (team == TEAM_BLUE) c->current_blue_pop++;
            activate_chunk_at(w, y, x);
        }
    }
    
    // Load History
    cJSON *history = cJSON_GetObjectItemCaseSensitive(root, "history");
    if (history) {
        cJSON *count = cJSON_GetObjectItemCaseSensitive(history, "count");
        if (count && count->valueint > 0) {
            int hCount = count->valueint;
            if (c->history_red_pop) free(c->history_red_pop);
            if (c->history_blue_pop) free(c->history_blue_pop);
            c->history_red_pop = malloc(c->max_rounds * sizeof(int));
            c->history_blue_pop = malloc(c->max_rounds * sizeof(int));
            c->history_count = 0;
            
            cJSON *red_arr = cJSON_GetObjectItemCaseSensitive(history, "red");
            cJSON *blue_arr = cJSON_GetObjectItemCaseSensitive(history, "blue");
            
            for (int i = 0; i < hCount && i < c->max_rounds; i++) {
                cJSON *r_val = cJSON_GetArrayItem(red_arr, i);
                cJSON *b_val = cJSON_GetArrayItem(blue_arr, i);
                if (r_val && b_val) {
                    c->history_red_pop[i] = r_val->valueint;
                    c->history_blue_pop[i] = b_val->valueint;
                    c->history_count++;
                }
            }
        }
    }
    
    cJSON_Delete(root);
    printf("Loaded JSON from %s\n", filename);
    return 1;
}

// KI-Agent unterstützt: Parse metadata for preview panel
int load_protocol_metadata(const char *filename, ProtocolInfo *info) {
    char *json_string = read_file_to_string(filename);
    if (!json_string) return 0;
    
    cJSON *root = cJSON_Parse(json_string);
    free(json_string);
    if (!root) return 0;
    
    info->has_results = 0;
    info->winner = 0;
    info->final_red = 0;
    info->final_blue = 0;
    info->timestamp = 0;
    
    cJSON *metadata = cJSON_GetObjectItemCaseSensitive(root, "metadata");
    if (metadata) {
        cJSON *ts = cJSON_GetObjectItemCaseSensitive(metadata, "unix_timestamp");
        if (ts) info->timestamp = (time_t)ts->valuedouble;
        
        cJSON *winner = cJSON_GetObjectItemCaseSensitive(metadata, "winner");
        if (winner) {
            info->has_results = 1;
            info->winner = winner->valueint;
            
            cJSON *final_red = cJSON_GetObjectItemCaseSensitive(metadata, "final_red");
            if (final_red) info->final_red = final_red->valueint;
            
            cJSON *final_blue = cJSON_GetObjectItemCaseSensitive(metadata, "final_blue");
            if (final_blue) info->final_blue = final_blue->valueint;
        }
    }
    
    cJSON *config = cJSON_GetObjectItemCaseSensitive(root, "config");
    if (config) {
        cJSON *rows = cJSON_GetObjectItemCaseSensitive(config, "bounding_box_y");
        if (rows) info->rows = rows->valueint;
        
        cJSON *cols = cJSON_GetObjectItemCaseSensitive(config, "bounding_box_x");
        if (cols) info->cols = cols->valueint;
        
        cJSON *max_pop = cJSON_GetObjectItemCaseSensitive(config, "max_population");
        if (max_pop) info->max_population = max_pop->valueint;
        
        cJSON *max_rounds = cJSON_GetObjectItemCaseSensitive(config, "max_rounds");
        if (max_rounds) info->max_rounds = max_rounds->valueint;
    }
    
    cJSON_Delete(root);
    return 1;
}

// KI-Agent unterstützt: Append results by parsing, updating, and saving JSON
void append_protocol_result(const char *filename, GameConfig *c, int winner) {
    char *json_string = read_file_to_string(filename);
    if (!json_string) return;
    
    cJSON *root = cJSON_Parse(json_string);
    free(json_string);
    if (!root) return;
    
    cJSON *metadata = cJSON_GetObjectItemCaseSensitive(root, "metadata");
    if (!metadata) {
        metadata = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "metadata", metadata);
    }
    
    cJSON_AddNumberToObject(metadata, "has_results", 1);
    cJSON_AddNumberToObject(metadata, "winner", winner);
    cJSON_AddNumberToObject(metadata, "final_red", c->current_red_pop);
    cJSON_AddNumberToObject(metadata, "final_blue", c->current_blue_pop);
    
    if (c->history_count > 0 && c->history_red_pop && c->history_blue_pop) {
        cJSON *history = cJSON_CreateObject();
        cJSON_AddNumberToObject(history, "count", c->history_count);
        
        cJSON *red_arr = cJSON_CreateArray();
        cJSON *blue_arr = cJSON_CreateArray();
        for (int i = 0; i < c->history_count; i++) {
            cJSON_AddItemToArray(red_arr, cJSON_CreateNumber(c->history_red_pop[i]));
            cJSON_AddItemToArray(blue_arr, cJSON_CreateNumber(c->history_blue_pop[i]));
        }
        cJSON_AddItemToObject(history, "red", red_arr);
        cJSON_AddItemToObject(history, "blue", blue_arr);
        cJSON_AddItemToObject(root, "history", history);
    }
    
    char *new_json = cJSON_Print(root);
    write_string_to_file(filename, new_json);
    
    free(new_json);
    cJSON_Delete(root);
    printf("Appended results and history to %s\n", filename);

#if defined(PLATFORM_WEB)
    EM_ASM({
        FS.syncfs(false, function(err) {
            if (err) console.error("IndexedDB sync error:", err);
        });
    });
#endif
}

// KI-Agent unterstützt: List only .json protocol files
int list_protocol_files(const char *dir_path, ProtocolInfo **out_list) {
    DIR *d = opendir(dir_path);
    if (!d) return 0;

    struct dirent *dir;
    int count = 0;
    int capacity = 10;
    ProtocolInfo *list = malloc(capacity * sizeof(ProtocolInfo));

    while ((dir = readdir(d)) != NULL) {
        char *ext = strrchr(dir->d_name, '.');
        // Search ONLY for .json files now! (Hard Cut)
        if (ext && strcmp(ext, ".json") == 0) {
            ProtocolInfo info;
            snprintf(info.filename, sizeof(info.filename), "%s", dir->d_name);
            snprintf(info.filepath, sizeof(info.filepath), "%s/%s", dir_path, dir->d_name);
            
            if (load_protocol_metadata(info.filepath, &info)) {
                if (count >= capacity) {
                    capacity *= 2;
                    list = realloc(list, capacity * sizeof(ProtocolInfo));
                }
                list[count++] = info;
            }
        }
    }
    closedir(d);

    if (count > 0) qsort(list, count, sizeof(ProtocolInfo), compare_protocol_info);

    *out_list = list;
    return count;
}

// Legacy markdown export remains for backward compatibility of external tools
void export_stats_md(const char *filename, GameConfig *c, int winner) {
    FILE *f = fopen(filename, "w");
    if (!f) return; 
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    fprintf(f, "# Biotope Game Results\n\n");
    fprintf(f, "**Date:** %02d.%02d.%04d %02d:%02d\n\n", 
            t->tm_mday, t->tm_mon+1, t->tm_year+1900, t->tm_hour, t->tm_min);
            
    fprintf(f, "## Configuration\n");
    fprintf(f, "- Grid: %dx%d\n", c->rows, c->cols);
    fprintf(f, "- Max Population: %d\n", c->max_population);
    fprintf(f, "- Max Rounds: %d\n\n", c->max_rounds);
    
    fprintf(f, "## Final Score\n");
    fprintf(f, "- **Red Team:** %d\n", c->current_red_pop);
    fprintf(f, "- **Blue Team:** %d\n\n", c->current_blue_pop);
    
    fprintf(f, "## Result\n");
    if (winner == TEAM_RED) fprintf(f, "**Winner: RED TEAM**\n");
    else if (winner == TEAM_BLUE) fprintf(f, "**Winner: BLUE TEAM**\n");
    else fprintf(f, "**DRAW**\n");
    
    fclose(f);
    printf("Stats exported to %s\n", filename);
}
// KI-Agent unterstützt: Load Config from JSON
bool load_config_from_json(const char* filepath, GameConfig* config) {
    char *json_str = read_file_to_string(filepath);
    if (!json_str) return false;
    cJSON *root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) return false;

    // Default or extract config values
    cJSON *cfg = cJSON_GetObjectItemCaseSensitive(root, "config");
    if (cfg) {
        cJSON *max_r = cJSON_GetObjectItemCaseSensitive(cfg, "max_rounds");
        if (cJSON_IsNumber(max_r)) config->max_rounds = max_r->valueint;
    }
    
    cJSON_Delete(root);
    return true;
}

// KI-Agent unterstützt: Initialize World from File
bool initialize_world_from_file(const char* filepath, World* world, Team team, char* out_player_id, char* out_nickname) {
    char *json_str = read_file_to_string(filepath);
    if (!json_str) return false;
    cJSON *root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) return false;

    if (out_player_id) strcpy(out_player_id, "unknown");
    if (out_nickname) strcpy(out_nickname, "unknown");

    cJSON *meta = cJSON_GetObjectItemCaseSensitive(root, "metadata");
    if (meta) {
        cJSON *pid = cJSON_GetObjectItemCaseSensitive(meta, "player_id");
        cJSON *nick = cJSON_GetObjectItemCaseSensitive(meta, "nickname");
        if (pid && cJSON_IsString(pid) && out_player_id) strncpy(out_player_id, pid->valuestring, 63);
        if (nick && cJSON_IsString(nick) && out_nickname) strncpy(out_nickname, nick->valuestring, 63);
    }
    
    cJSON *config = cJSON_GetObjectItemCaseSensitive(root, "config");
    cJSON *cells = cJSON_GetObjectItemCaseSensitive(config, "cells");
    if (cJSON_IsArray(cells)) {
        cJSON *cell;
        cJSON_ArrayForEach(cell, cells) {
            if (cJSON_IsArray(cell) && cJSON_GetArraySize(cell) >= 2) {
                cJSON *cx = cJSON_GetArrayItem(cell, 0);
                cJSON *cy = cJSON_GetArrayItem(cell, 1);
                if (cJSON_IsNumber(cx) && cJSON_IsNumber(cy)) {
                    int x = cx->valueint;
                    int y = cy->valueint;
                    if (x >= 0 && x < LOCAL_GRID_SIZE && y >= 0 && y < LOCAL_GRID_SIZE) {
                        int offset_x = (team == TEAM_BLUE) ? LOCAL_GRID_SIZE : 0;
                        world->grid[(y + 1) * (world->cols + 2) + (x + offset_x + 1)] = team;
                    }
                }
            }
        }
    }
    cJSON_Delete(root);
    return true;
}

// KI-Agent unterstützt: Parse the batch JSON file
int parse_batch_file(const char* filepath, Competitor** competitors, int* count, int* max_gen) {
    char *json_str = read_file_to_string(filepath);
    if (!json_str) return 0;
    cJSON *root = cJSON_Parse(json_str);
    if (!root) { free(json_str); return 0; }

    cJSON *max_gen_obj = cJSON_GetObjectItemCaseSensitive(root, "max_generations");
    *max_gen = (cJSON_IsNumber(max_gen_obj)) ? max_gen_obj->valueint : 1000;

    cJSON *comps_array = cJSON_GetObjectItemCaseSensitive(root, "competitors");
    if (!cJSON_IsArray(comps_array)) {
        cJSON_Delete(root); free(json_str); return 0;
    }

    *count = cJSON_GetArraySize(comps_array);
    *competitors = calloc(*count, sizeof(Competitor));

    for (int i = 0; i < *count; i++) {
        cJSON *item = cJSON_GetArrayItem(comps_array, i);
        cJSON *pid = cJSON_GetObjectItemCaseSensitive(item, "player_id");
        if (cJSON_IsString(pid)) strncpy((*competitors)[i].player_id, pid->valuestring, 63);

        cJSON *cells = cJSON_GetObjectItemCaseSensitive(item, "cells");
        if (cJSON_IsArray(cells)) {
            int cell_count = cJSON_GetArraySize(cells);
            (*competitors)[i].living_cells = 0;
            for(int r=0; r<LOCAL_GRID_SIZE; r++) for(int c=0; c<LOCAL_GRID_SIZE; c++) (*competitors)[i].cells[r][c] = 0;
            
            for (int j = 0; j < cell_count; j++) {
                cJSON *cell = cJSON_GetArrayItem(cells, j);
                if (cJSON_IsArray(cell) && cJSON_GetArraySize(cell) >= 2) {
                    int x = cJSON_GetArrayItem(cell, 0)->valueint;
                    int y = cJSON_GetArrayItem(cell, 1)->valueint;
                    if (x >= 0 && x < LOCAL_GRID_SIZE && y >= 0 && y < LOCAL_GRID_SIZE) {
                        (*competitors)[i].cells[y][x] = 1;
                        (*competitors)[i].living_cells++;
                    }
                }
            }
        }
    }
    cJSON_Delete(root); free(json_str);
    return 1;
}

// KI-Agent unterstützt: Generate output JSON for batch results including highlights
int save_batch_results(const char* filepath, RankingScore* scores, int count, double cpu_time_used, HighlightEntry* highlights, int highlight_count) {
    cJSON *output_root = cJSON_CreateObject();
    cJSON_AddNumberToObject(output_root, "total_matches_played", (long long)count * (count - 1));
    cJSON_AddNumberToObject(output_root, "execution_time_cpu_s", cpu_time_used);
    
    cJSON *rankings_array = cJSON_CreateArray();
    for (int i = 0; i < count; i++) {
        cJSON *rank_item = cJSON_CreateObject();
        cJSON_AddStringToObject(rank_item, "player_id", scores[i].player_id);
        cJSON_AddNumberToObject(rank_item, "total_score", scores[i].total_score);
        cJSON_AddNumberToObject(rank_item, "matches_played", scores[i].matches_played);
        cJSON_AddNumberToObject(rank_item, "win_rate", scores[i].matches_played > 0 ? scores[i].total_score / scores[i].matches_played : 0);
        cJSON_AddNumberToObject(rank_item, "avg_stable_generation", scores[i].matches_played > 0 ? (double)scores[i].sum_stable_gen / scores[i].matches_played : 0);
        cJSON_AddItemToArray(rankings_array, rank_item);
    }
    cJSON_AddItemToObject(output_root, "rankings", rankings_array);

    // Add Highlights
    cJSON *highlights_array = cJSON_CreateArray();
    for (int i = 0; i < highlight_count; i++) {
        cJSON *h_item = cJSON_CreateObject();
        cJSON_AddStringToObject(h_item, "red_name", highlights[i].player_id_red);
        cJSON_AddStringToObject(h_item, "blue_name", highlights[i].player_id_blue);
        cJSON_AddNumberToObject(h_item, "metric_value", highlights[i].score);
        
        // Convert Bitboards to Arrays
        cJSON *red_seed = cJSON_CreateArray();
        cJSON *blue_seed = cJSON_CreateArray();
        for (int b = 0; b < 64; b++) {
            cJSON_AddItemToArray(red_seed, cJSON_CreateNumber((highlights[i].seed_red & (1ULL << b)) ? 1 : 0));
            cJSON_AddItemToArray(blue_seed, cJSON_CreateNumber((highlights[i].seed_blue & (1ULL << b)) ? 1 : 0));
        }
        cJSON_AddItemToObject(h_item, "red_seed", red_seed);
        cJSON_AddItemToObject(h_item, "blue_seed", blue_seed);
        cJSON_AddItemToArray(highlights_array, h_item);
    }
    cJSON_AddItemToObject(output_root, "highlights", highlights_array);

    char *output_str = cJSON_Print(output_root);
    FILE *out_f = fopen(filepath, "w");
    if (out_f) {
        fprintf(out_f, "%s\n", output_str);
        fclose(out_f);
    }
    free(output_str);
    cJSON_Delete(output_root);
    return out_f ? 1 : 0;
}

// KI-Agent unterstützt: Generate output JSON for headless results
void save_headless_results(const char* filepath, const char* winner, int gens, const char* rp_id, const char* rp_nick, int rp_pop, const char* bp_id, const char* bp_nick, int bp_pop) {
    cJSON *result_root = cJSON_CreateObject();
    cJSON_AddStringToObject(result_root, "winner", winner);
    cJSON_AddNumberToObject(result_root, "generations", gens);
    
    char timestamp[64];
    get_iso8601_time(timestamp, sizeof(timestamp), time(NULL));
    cJSON_AddStringToObject(result_root, "timestamp", timestamp);

    cJSON *red_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(red_obj, "player_id", rp_id);
    cJSON_AddStringToObject(red_obj, "nickname", rp_nick);
    cJSON_AddNumberToObject(red_obj, "population", rp_pop);
    cJSON_AddItemToObject(result_root, "red", red_obj);

    cJSON *blue_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(blue_obj, "player_id", bp_id);
    cJSON_AddStringToObject(blue_obj, "nickname", bp_nick);
    cJSON_AddNumberToObject(blue_obj, "population", bp_pop);
    cJSON_AddItemToObject(result_root, "blue", blue_obj);

    char *json_output = cJSON_Print(result_root);
    if (filepath) {
        FILE *f = fopen(filepath, "w");
        if (f) {
            fputs(json_output, f);
            fclose(f);
            printf("Results saved to: %s\n", filepath);
        } else {
            fprintf(stderr, "Error: Could not open output file %s\n", filepath);
            printf("%s\n", json_output); // Fallback to stdout
        }
    } else {
        printf("%s\n", json_output);
    }

    free(json_output);
    cJSON_Delete(result_root);
}
