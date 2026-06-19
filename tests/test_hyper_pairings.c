// KI-Agent unterstützt: Unit tests for parse_batch_file explicit-pairings mode
// (ADR-0032 Stage 2).
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "core_types.h"
#include "file_io.h"

#define TEST(name) void test_##name(void)
#define RUN_TEST(name) \
    printf("Running %s... ", #name); \
    test_##name(); \
    printf("PASSED\n");

// Writes `content` to a fixed temp file and returns its path. Each test writes
// then removes it, so reusing one path across sequential tests is fine.
static const char *write_temp(const char *content) {
    static const char *path = "/tmp/test_hyper_pairings.json";
    FILE *f = fopen(path, "w");
    assert(f);
    fputs(content, f);
    fclose(f);
    return path;
}

static const char *BATCH_NO_PAIRINGS =
    "{\"max_generations\":500,\"competitors\":["
    "{\"player_id\":\"P0\",\"cells\":[[1,1],[2,1]]},"
    "{\"player_id\":\"P1\",\"cells\":[[0,0],[1,1]]},"
    "{\"player_id\":\"P2\",\"cells\":[[3,3]]}]}";

static const char *BATCH_WITH_PAIRINGS =
    "{\"max_generations\":500,\"competitors\":["
    "{\"player_id\":\"P0\",\"cells\":[[1,1],[2,1]]},"
    "{\"player_id\":\"P1\",\"cells\":[[0,0],[1,1]]},"
    "{\"player_id\":\"P2\",\"cells\":[[3,3]]}],"
    "\"pairings\":[[0,1],[0,2]]}";

// Pairings with an out-of-range index that must be skipped defensively.
static const char *BATCH_BAD_PAIRINGS =
    "{\"max_generations\":500,\"competitors\":["
    "{\"player_id\":\"P0\",\"cells\":[[1,1]]},"
    "{\"player_id\":\"P1\",\"cells\":[[0,0]]}],"
    "\"pairings\":[[0,1],[0,9],[-1,0]]}";

TEST(no_pairings_means_full_round_robin) {
    Competitor *comp = NULL;
    int count = 0, max_gen = 0, pcount = 99;
    Pairing *pairings = (Pairing *)0x1; // sentinel; must be reset to NULL
    const char *path = write_temp(BATCH_NO_PAIRINGS);

    int ok = parse_batch_file(path, &comp, &count, &max_gen, &pairings, &pcount);
    assert(ok);
    assert(count == 3);
    assert(max_gen == 500);
    assert(pcount == -1);        // signals full round-robin
    assert(pairings == NULL);

    remove(path);
    free(comp);
    free(pairings);
}

TEST(explicit_pairings_parsed) {
    Competitor *comp = NULL;
    int count = 0, max_gen = 0, pcount = 0;
    Pairing *pairings = NULL;
    const char *path = write_temp(BATCH_WITH_PAIRINGS);

    int ok = parse_batch_file(path, &comp, &count, &max_gen, &pairings, &pcount);
    assert(ok);
    assert(count == 3);
    assert(pcount == 2);
    assert(pairings != NULL);
    assert(pairings[0].a == 0 && pairings[0].b == 1);
    assert(pairings[1].a == 0 && pairings[1].b == 2);

    remove(path);
    free(comp);
    free(pairings);
}

TEST(out_of_range_pairings_skipped) {
    Competitor *comp = NULL;
    int count = 0, max_gen = 0, pcount = 0;
    Pairing *pairings = NULL;
    const char *path = write_temp(BATCH_BAD_PAIRINGS);

    int ok = parse_batch_file(path, &comp, &count, &max_gen, &pairings, &pcount);
    assert(ok);
    assert(count == 2);
    assert(pcount == 1);          // only [0,1] is valid
    assert(pairings[0].a == 0 && pairings[0].b == 1);

    remove(path);
    free(comp);
    free(pairings);
}

int main(void) {
    printf("=== Starting Hyper Pairings DEV_TEST ===\n");
    RUN_TEST(no_pairings_means_full_round_robin);
    RUN_TEST(explicit_pairings_parsed);
    RUN_TEST(out_of_range_pairings_skipped);
    printf("=== All Hyper Pairings DEV_TESTs PASSED ===\n");
    return 0;
}
