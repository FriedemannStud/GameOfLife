#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../renderer.h"

#define TEST(name) void test_##name()
#define RUN_TEST(name) \
    printf("Running %s... ", #name); \
    test_##name(); \
    printf("PASSED\n");

TEST(renderer_init_close) {
    // We do not actually initialize Raylib here because it might require an X server context
    // However, we can test state machine transitions that don't depend on actual user input
    // by manually testing process_ui_events with a known state, if possible.
    // Since process_ui_events depends heavily on IsKeyPressed(), testing it requires an active Raylib window.
    // For this DEV_TEST, we ensure it compiles and can handle boundary inputs safely.
    
    GameConfig config = {0};
    World *current = NULL;
    World *next = NULL;
    
    // Boundary test: process_ui_events should handle NULL worlds safely in states that expect them?
    // Actually, process_ui_events assumes valid pointers for the most part, but let's test puzzle state.
    // In puzzle state, it only checks IsKeyPressed(KEY_ENTER).
    // It shouldn't crash.
    AppState state = process_ui_events(STATE_PUZZLE, &config, &current, &next);
    assert(state == STATE_PUZZLE); // Assuming no key is pressed
}

int main(void) {
    printf("=== Starting Renderer DEV_TEST ===\n");
    // Since drawing/raylib is hard to test in a headless container, we focus on safe API boundaries.
    RUN_TEST(renderer_init_close);
    printf("=== All Renderer DEV_TESTs PASSED ===\n");
    return 0;
}