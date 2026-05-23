#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "cJSON.h"
#include "game_logic.h"

// KI-Agent unterstützt: Competitor structure for batch processing
typedef struct {
    char player_id[64];
    int cells[8][8]; // 8x8 local grid for the starting pattern
    int living_cells;
} Competitor;

// KI-Agent unterstützt: Ranking result for a player
typedef struct {
    char player_id[64];
    double total_score;
    int matches_played;
    long long sum_stable_gen;
} RankingScore;

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

// KI-Agent unterstützt: Parse the batch JSON file
static int parse_batch_file(const char* filepath, Competitor** competitors, int* count, int* max_gen) {
    char *json_str = read_file_to_string(filepath);
    if (!json_str) {
        fprintf(stderr, "Error: Could not read file %s\n", filepath);
        return 0;
    }

    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        fprintf(stderr, "Error: Could not parse JSON in %s\n", filepath);
        free(json_str);
        return 0;
    }

    cJSON *max_gen_obj = cJSON_GetObjectItemCaseSensitive(root, "max_generations");
    if (cJSON_IsNumber(max_gen_obj)) {
        *max_gen = max_gen_obj->valueint;
    } else {
        *max_gen = 1000; // Default
    }

    cJSON *comps_array = cJSON_GetObjectItemCaseSensitive(root, "competitors");
    if (!cJSON_IsArray(comps_array)) {
        fprintf(stderr, "Error: 'competitors' array not found\n");
        cJSON_Delete(root);
        free(json_str);
        return 0;
    }

    *count = cJSON_GetArraySize(comps_array);
    *competitors = calloc(*count, sizeof(Competitor));

    for (int i = 0; i < *count; i++) {
        cJSON *item = cJSON_GetArrayItem(comps_array, i);
        cJSON *pid = cJSON_GetObjectItemCaseSensitive(item, "player_id");
        if (cJSON_IsString(pid)) {
            strncpy((*competitors)[i].player_id, pid->valuestring, 63);
        }

        cJSON *cells = cJSON_GetObjectItemCaseSensitive(item, "cells");
        if (cJSON_IsArray(cells)) {
            int cell_count = cJSON_GetArraySize(cells);
            (*competitors)[i].living_cells = 0;
            // Initialize cells array to 0
            for(int r=0; r<8; r++) for(int c=0; c<8; c++) (*competitors)[i].cells[r][c] = 0;
            
            for (int j = 0; j < cell_count; j++) {
                cJSON *cell = cJSON_GetArrayItem(cells, j);
                if (cJSON_IsArray(cell) && cJSON_GetArraySize(cell) >= 2) {
                    int x = cJSON_GetArrayItem(cell, 0)->valueint;
                    int y = cJSON_GetArrayItem(cell, 1)->valueint;
                    if (x >= 0 && x < 8 && y >= 0 && y < 8) {
                        (*competitors)[i].cells[y][x] = 1;
                        (*competitors)[i].living_cells++;
                    }
                }
            }
        }
    }

    cJSON_Delete(root);
    free(json_str);
    return 1;
}

// KI-Agent unterstützt: Comparator for sorting ranking scores (descending)
static int compare_rankings(const void *a, const void *b) {
    RankingScore *ra = (RankingScore *)a;
    RankingScore *rb = (RankingScore *)b;
    if (rb->total_score > ra->total_score) return 1;
    if (rb->total_score < ra->total_score) return -1;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: ./biotope_hyper_worker <input_batch.json> <output_results.json>\n");
        return 1;
    }

    const char *input_path = argv[1];
    const char *output_path = argv[2];

    Competitor *competitors = NULL;
    int competitor_count = 0;
    int max_generations = 0;

    if (!parse_batch_file(input_path, &competitors, &competitor_count, &max_generations)) {
        return 1;
    }

    printf("Successfully parsed %d competitors. Max Generations: %d\n", competitor_count, max_generations);
    
    RankingScore *scores = calloc(competitor_count, sizeof(RankingScore));
    for (int i = 0; i < competitor_count; i++) {
        strncpy(scores[i].player_id, competitors[i].player_id, 63);
        scores[i].total_score = 0;
        scores[i].matches_played = 0;
        scores[i].sum_stable_gen = 0;
    }

    clock_t start_time = clock();

    // KI-Agent unterstützt: O(N^2) Round-Robin with OpenMP Parallelization
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < competitor_count; i++) {
        for (int j = i + 1; j < competitor_count; j++) {
            // Match 1: i (Left) vs j (Right)
            MatchResult res1 = run_isolated_match(competitors[i].cells, competitors[j].cells, max_generations);
            
            // Match 2: j (Left) vs i (Right)
            MatchResult res2 = run_isolated_match(competitors[j].cells, competitors[i].cells, max_generations);

            #pragma omp atomic
            scores[i].total_score += (res1.winner == TEAM_RED ? 1.0 : (res1.winner == 0 ? 0.5 : 0.0));
            #pragma omp atomic
            scores[i].total_score += (res2.winner == TEAM_BLUE ? 1.0 : (res2.winner == 0 ? 0.5 : 0.0));
            #pragma omp atomic
            scores[i].matches_played += 2;
            #pragma omp atomic
            scores[i].sum_stable_gen += (res1.stable_at_generation ? res1.stable_at_generation : max_generations);
            #pragma omp atomic
            scores[i].sum_stable_gen += (res2.stable_at_generation ? res2.stable_at_generation : max_generations);

            #pragma omp atomic
            scores[j].total_score += (res1.winner == TEAM_BLUE ? 1.0 : (res1.winner == 0 ? 0.5 : 0.0));
            #pragma omp atomic
            scores[j].total_score += (res2.winner == TEAM_RED ? 1.0 : (res2.winner == 0 ? 0.5 : 0.0));
            #pragma omp atomic
            scores[j].matches_played += 2;
            #pragma omp atomic
            scores[j].sum_stable_gen += (res1.stable_at_generation ? res1.stable_at_generation : max_generations);
            #pragma omp atomic
            scores[j].sum_stable_gen += (res2.stable_at_generation ? res2.stable_at_generation : max_generations);
        }
    }

    clock_t end_time = clock();
    double cpu_time_used = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;

    // Sort rankings
    qsort(scores, competitor_count, sizeof(RankingScore), compare_rankings);

    // KI-Agent unterstützt: Generate output JSON
    cJSON *output_root = cJSON_CreateObject();
    cJSON_AddNumberToObject(output_root, "total_matches_played", (competitor_count * (competitor_count - 1)));
    cJSON_AddNumberToObject(output_root, "execution_time_cpu_s", cpu_time_used);
    
    cJSON *rankings_array = cJSON_CreateArray();
    for (int i = 0; i < competitor_count; i++) {
        cJSON *rank_item = cJSON_CreateObject();
        cJSON_AddStringToObject(rank_item, "player_id", scores[i].player_id);
        cJSON_AddNumberToObject(rank_item, "total_score", scores[i].total_score);
        cJSON_AddNumberToObject(rank_item, "matches_played", scores[i].matches_played);
        cJSON_AddNumberToObject(rank_item, "win_rate", scores[i].matches_played > 0 ? scores[i].total_score / scores[i].matches_played : 0);
        cJSON_AddNumberToObject(rank_item, "avg_stable_generation", scores[i].matches_played > 0 ? (double)scores[i].sum_stable_gen / scores[i].matches_played : 0);
        cJSON_AddItemToArray(rankings_array, rank_item);
    }
    cJSON_AddItemToObject(output_root, "rankings", rankings_array);

    char *output_str = cJSON_Print(output_root);
    FILE *out_f = fopen(output_path, "w");
    if (out_f) {
        fprintf(out_f, "%s\n", output_str);
        fclose(out_f);
        printf("Results written to %s\n", output_path);
    } else {
        fprintf(stderr, "Error: Could not open %s for writing\n", output_path);
    }

    // Cleanup
    free(output_str);
    cJSON_Delete(output_root);
    free(scores);
    free(competitors);

    return 0;
}
