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

    // KI-Agent unterstützt: Thread-local highlight storage to avoid mutexes
    int max_threads = 1;
#ifdef _OPENMP
    max_threads = omp_get_max_threads();
#endif
    HighlightEntry *thread_highlights = calloc(max_threads * 10, sizeof(HighlightEntry));

    // KI-Agent unterstützt: O(N^2) Round-Robin with OpenMP Parallelization
    #pragma omp parallel
    {
        int tid = 0;
#ifdef _OPENMP
        tid = omp_get_thread_num();
#endif
        HighlightEntry *my_highlights = &thread_highlights[tid * 10];
        int my_h_count = 0;

        #pragma omp for schedule(dynamic)
        for (int i = 0; i < competitor_count; i++) {
            for (int j = i + 1; j < competitor_count; j++) {
                // KI-Agent unterstützt: Single match per pair (ADR-0026).
                // A second "swapped" round is mathematically redundant on a torus with a
                // colour-symmetric rule (births only at n=3, odd). T = shift-8-cols ∘ swap(RED↔BLUE)
                // is an exact symmetry of Φ, so round 2 would always produce the same winner
                // and identical per-player populations — only rankings would be multiplied by 2.
                // TRIPWIRE: if topology changes (bounded grid) or birth rule gains an even-n case,
                // re-evaluate and potentially restore the second call. See ADR-0026 and
                // DEV_TECH_DESIGN-0026 for the full proof.
                MatchResult res = run_isolated_match(competitors[i].cells, competitors[j].cells, max_generations);

                #pragma omp atomic
                scores[i].total_score += (res.winner == TEAM_RED ? 1.0 : (res.winner == 0 ? 0.5 : 0.0));
                #pragma omp atomic
                scores[i].matches_played += 1;
                #pragma omp atomic
                scores[i].wins += (res.winner == TEAM_RED ? 1 : 0);
                #pragma omp atomic
                scores[i].draws += (res.winner == 0 ? 1 : 0);
                #pragma omp atomic
                scores[i].losses += (res.winner == TEAM_BLUE ? 1 : 0);
                #pragma omp atomic
                scores[i].sum_stable_gen += (res.stable_at_generation ? res.stable_at_generation : max_generations);

                #pragma omp atomic
                scores[j].total_score += (res.winner == TEAM_BLUE ? 1.0 : (res.winner == 0 ? 0.5 : 0.0));
                #pragma omp atomic
                scores[j].matches_played += 1;
                #pragma omp atomic
                scores[j].wins += (res.winner == TEAM_BLUE ? 1 : 0);
                #pragma omp atomic
                scores[j].draws += (res.winner == 0 ? 1 : 0);
                #pragma omp atomic
                scores[j].losses += (res.winner == TEAM_RED ? 1 : 0);
                #pragma omp atomic
                scores[j].sum_stable_gen += (res.stable_at_generation ? res.stable_at_generation : max_generations);

                // KI-Agent unterstützt: Update local highlights (Activity Sum)
                if (my_h_count < 10 || (double)res.activity_sum > my_highlights[9].score) {
                    int pos = (my_h_count < 10) ? my_h_count : 9;
                    while (pos > 0 && (double)res.activity_sum > my_highlights[pos-1].score) {
                        my_highlights[pos] = my_highlights[pos-1];
                        pos--;
                    }
                    my_highlights[pos].score = (double)res.activity_sum;
                    my_highlights[pos].seed_red = grid_to_bitboard(competitors[i].cells);
                    my_highlights[pos].seed_blue = grid_to_bitboard(competitors[j].cells);
                    snprintf(my_highlights[pos].player_id_red, sizeof(my_highlights[pos].player_id_red), "%s", competitors[i].player_id);
                    snprintf(my_highlights[pos].player_id_blue, sizeof(my_highlights[pos].player_id_blue), "%s", competitors[j].player_id);
                    if (my_h_count < 10) my_h_count++;
                }
            }
        }
    }

    // Merge Thread Highlights
    HighlightEntry final_highlights[10];
    int final_h_count = 0;
    for (int t = 0; t < max_threads; t++) {
        for (int h = 0; h < 10; h++) {
            HighlightEntry *entry = &thread_highlights[t * 10 + h];
            if (entry->score == 0) continue;

            if (final_h_count < 10 || entry->score > final_highlights[9].score) {
                int pos = (final_h_count < 10) ? final_h_count : 9;
                while (pos > 0 && entry->score > final_highlights[pos-1].score) {
                    final_highlights[pos] = final_highlights[pos-1];
                    pos--;
                }
                final_highlights[pos] = *entry;
                if (final_h_count < 10) final_h_count++;
            }
        }
    }

    clock_t end_time = clock();
    double cpu_time_used = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;

    // Sort rankings
    qsort(scores, competitor_count, sizeof(RankingScore), compare_rankings);

    // KI-Agent unterstützt: Generate output JSON using file_io
    if (save_batch_results(output_path, scores, competitor_count, cpu_time_used, final_highlights, final_h_count)) {
        printf("Results written to %s (CPU Time: %.2fs)\n", output_path, cpu_time_used);
    } else {
        fprintf(stderr, "Error: Could not save results to %s\n", output_path);
    }

    // Cleanup
    free(thread_highlights);
    free(scores);
    free(competitors);

    return 0;
}
