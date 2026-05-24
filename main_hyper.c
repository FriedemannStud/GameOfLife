#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "core_types.h"
#include "game_logic.h"
#include "file_io.h"

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

    // KI-Agent unterstützt: Generate output JSON using file_io
    if (save_batch_results(output_path, scores, competitor_count, cpu_time_used)) {
        printf("Results written to %s (CPU Time: %.2fs)\n", output_path, cpu_time_used);
    } else {
        fprintf(stderr, "Error: Could not save results to %s\n", output_path);
    }

    // Cleanup
    free(scores);
    free(competitors);

    return 0;
}
