#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

// KI-Agent unterstützt
#define DEAD 0
#define TEAM_RED 1
#define TEAM_BLUE 2
#define MAX_ROUNDS 1000

#define CHUNK_SIZE 64

// KI-Agent unterstützt
typedef struct {
    int *grid; // Pointer to flat array: row-major order
    int rows;
    int cols;

    // Spatial Partitioning (NEW for Epic Scale)
    unsigned char *chunk_map; // 1D array: 1 = active, 0 = dead
    int chunk_rows;
    int chunk_cols;
} World;

// KI-Agent unterstützt
World* create_world(int rows, int cols);
void free_world(World *w);
void init_world(World *current_gen, int rows, int cols);
void sync_ghost_borders(World *w);
void update_generation(World *current_gen, World *next_gen, int rows, int cols, int *red_pop, int *blue_pop);
void apply_catalyst(World *w, int center_r, int center_c);
void activate_chunk_at(World *w, int r, int c);

#endif // GAME_LOGIC_H