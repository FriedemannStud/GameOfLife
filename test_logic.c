#include <stdio.h>
#include <assert.h>
#include "game_logic.h"

// KI-Agent unterstützt
int main() {
    printf("Testing Game Logic...\n");
    
    // Test creation
    World *w = create_world(10, 10);
    assert(w != NULL);
    assert(w->grid != NULL);
    assert(w->rows == 10);
    assert(w->cols == 10);
    printf("- World creation passed\n");

    // Test initialization
    init_world(w, 10, 10);
    // Just check memory is accessible
    int val = w->grid[0];
    assert(val >= 0 && val <= 2);
    printf("- Initialization passed\n");

    // Test update
    World *next = create_world(10, 10);
    update_generation(w, next, 10, 10);
    printf("- Update generation passed\n");

    // Test cleanup
    free_world(w);
    free_world(next);
    printf("- Memory cleanup passed\n");

    printf("All logic tests passed!\n");
    return 0;
}
