#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

// KI-Agent unterstützt
#define DEAD 0
#define TEAM_RED 1
#define TEAM_BLUE 2
#define MAX_ROUNDS 1000

// KI-Agent unterstützt
typedef struct {
    int *grid; // Pointer to flat array: row-major order
    int rows;
    int cols;
} World;

// KI-Agent unterstützt
World* create_world(int rows, int cols);
void free_world(World *w);
void init_world(World *current_gen, int rows, int cols);
void update_generation(World *current_gen, World *next_gen, int rows, int cols);

#endif // GAME_LOGIC_H