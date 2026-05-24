#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include "../core_types.h"
#include "../config.h"

// Simple unit testing framework macro
#define TEST(name) void test_##name()
#define RUN_TEST(name) \
    printf("Running %s... ", #name); \
    test_##name(); \
    printf("PASSED\n");

TEST(struct_layout) {
    // Check sizes and offsets
    assert(sizeof(World) > 0);
    assert(sizeof(GameConfig) > 0);
    
    // Ensure GameConfig pointer types are initialized to NULL when using calloc
    GameConfig *gc = calloc(1, sizeof(GameConfig));
    assert(gc != NULL);
    assert(gc->history_red_pop == NULL);
    assert(gc->history_blue_pop == NULL);
    free(gc);
}

TEST(config_macros) {
    assert(DEFAULT_WINDOW_WIDTH >= 600 && DEFAULT_WINDOW_WIDTH <= 3840);
    assert(DEFAULT_WINDOW_HEIGHT >= 400 && DEFAULT_WINDOW_HEIGHT <= 2160);
    assert(LOCAL_GRID_SIZE > 0 && LOCAL_GRID_SIZE <= 128);
    assert(MAX_PATH_LENGTH >= 256);
    assert(MAX_FILENAME_LENGTH >= 64);
}

int main(void) {
    printf("=== Starting Foundation DEV_TEST ===\n");
    RUN_TEST(struct_layout);
    RUN_TEST(config_macros);
    printf("=== All Foundation DEV_TESTs PASSED ===\n");
    return 0;
}