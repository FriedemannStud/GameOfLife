#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../core_types.h"
#include "../file_io.h"

// Simple test framework
#define TEST(name) void test_##name()
#define RUN_TEST(name) \
    printf("Running %s... ", #name); \
    test_##name(); \
    printf("PASSED\n");

// Helper to write string to file for testing
void write_mock_json(const char* filepath, const char* content) {
    FILE *f = fopen(filepath, "w");
    if (f) {
        fputs(content, f);
        fclose(f);
    }
}

TEST(corrupted_json) {
    write_mock_json("mock.json", "{ invalid_json: ");
    World *w = create_world(10, 10);
    bool success = initialize_world_from_file("mock.json", w, TEAM_RED, NULL, NULL);
    assert(success == false);
    free_world(w);
    remove("mock.json");
}

TEST(missing_fields) {
    write_mock_json("mock.json", "{\"config\": {}}");
    World *w = create_world(10, 10);
    char pid[64];
    bool success = initialize_world_from_file("mock.json", w, TEAM_RED, pid, NULL);
    assert(success == true);
    assert(strcmp(pid, "unknown") == 0);
    free_world(w);
    remove("mock.json");
}

TEST(out_of_bounds_arrays) {
    // Array with points outside LOCAL_GRID_SIZE (e.g., 99, 99)
    write_mock_json("mock.json", "{\"config\": {\"cells\": [[99, 99], [-1, 5], [0, 0]]}}");
    World *w = create_world(LOCAL_GRID_SIZE, LOCAL_GRID_SIZE * 2);
    bool success = initialize_world_from_file("mock.json", w, TEAM_RED, NULL, NULL);
    assert(success == true); // Parsed successfully, out-of-bounds points ignored
    
    // Check that [0, 0] was loaded correctly at the correct offset
    // TEAM_RED offset is 0. So grid cell (0+1) * (LOCAL_GRID_SIZE*2 + 2) + (0+1)
    int cell = w->grid[1 * (LOCAL_GRID_SIZE * 2 + 2) + 1];
    assert(cell == TEAM_RED);
    
    // Check that [99, 99] and [-1, 5] did not crash and were ignored.
    free_world(w);
    remove("mock.json");
}

TEST(invalid_types) {
    // Cells array containing strings instead of numbers
    write_mock_json("mock.json", "{\"config\": {\"cells\": [[\"a\", \"b\"]]}}");
    World *w = create_world(LOCAL_GRID_SIZE, LOCAL_GRID_SIZE * 2);
    bool success = initialize_world_from_file("mock.json", w, TEAM_RED, NULL, NULL);
    assert(success == true); // Handled gracefully, ignored invalid types
    free_world(w);
    remove("mock.json");
}

int main(void) {
    printf("=== Starting IO Logic DEV_TEST ===\n");
    RUN_TEST(corrupted_json);
    RUN_TEST(missing_fields);
    RUN_TEST(out_of_bounds_arrays);
    RUN_TEST(invalid_types);
    printf("=== All IO Logic DEV_TESTs PASSED ===\n");
    return 0;
}