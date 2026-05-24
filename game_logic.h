#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "core_types.h"

#define CHUNK_SIZE 64

// KI-Agent unterstützt
World* create_world(int rows, int cols);
void free_world(World *w);
void init_world(World *current_gen, int rows, int cols);
void sync_ghost_borders(World *w);
void update_generation(World *current_gen, World *next_gen, int rows, int cols, int *red_pop, int *blue_pop);
void activate_chunk_at(World *w, int r, int c);

// KI-Agent unterstützt: Thread-safe match execution
MatchResult run_isolated_match(int left_cells[LOCAL_GRID_SIZE][LOCAL_GRID_SIZE], int right_cells[LOCAL_GRID_SIZE][LOCAL_GRID_SIZE], int max_gen);

#endif // GAME_LOGIC_H
