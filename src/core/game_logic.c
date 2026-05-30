#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#ifndef PLATFORM_WEB
#include <omp.h>
#endif
#include "game_logic.h"

// KI-Agent unterstützt
World* create_world(int rows, int cols) {
    World *w = malloc(sizeof(World));
    w->rows = rows;
    w->cols = cols;
    // PADDED GRID: (rows + 2) * (cols + 2)
    w->grid = calloc((rows + 2) * (cols + 2), sizeof(int)); 

    // CHUNKING (Epic Scale)
    w->chunk_rows = (rows + CHUNK_SIZE - 1) / CHUNK_SIZE;
    w->chunk_cols = (cols + CHUNK_SIZE - 1) / CHUNK_SIZE;
    w->chunk_map = calloc(w->chunk_rows * w->chunk_cols, sizeof(unsigned char));

    return w;
}

// KI-Agent unterstützt
void free_world(World *w) {
    if (w) {
        if (w->grid) free(w->grid);
        if (w->chunk_map) free(w->chunk_map);
        free(w);
    }
}

// KI-Agent unterstützt
void init_world(World *current_gen, int rows, int cols) {
    int stride = cols + 2;
    
    // Nested loops to skip the ghost borders (start at 1, end at rows/cols)
    for (int r = 1; r <= rows; r++) {
        for (int c = 1; c <= cols; c++) {
            int i = r * stride + c;
            
            int val = rand() % 100; 
            if (val < 10) {
                current_gen->grid[i] = TEAM_RED;
                activate_chunk_at(current_gen, r - 1, c - 1);
            } else if (val < 20) {
                current_gen->grid[i] = TEAM_BLUE;
                activate_chunk_at(current_gen, r - 1, c - 1);
            } else {
                current_gen->grid[i] = DEAD;
            }
        }
    }
}

// KI-Agent unterstützt: Synchronisiert die Ränder für unendliches Spielfeld (Wrapping)
void sync_ghost_borders(World *w) {
    int stride = w->cols + 2;

    // 1. Zeilen spiegeln (Oben <-> Unten)
    for (int c = 1; c <= w->cols; c++) {
        w->grid[0 * stride + c] = w->grid[w->rows * stride + c];           // Letzte echte Zeile -> Rahmen Oben
        w->grid[(w->rows + 1) * stride + c] = w->grid[1 * stride + c];     // Erste echte Zeile -> Rahmen Unten
    }

    // 2. Spalten spiegeln (Links <-> Rechts)
    for (int r = 1; r <= w->rows; r++) {
        w->grid[r * stride + 0] = w->grid[r * stride + w->cols];           // Rechteste echte Spalte -> Rahmen Links
        w->grid[r * stride + (w->cols + 1)] = w->grid[r * stride + 1];     // Linkeste echte Spalte -> Rahmen Rechts
    }

    // 3. Ecken spiegeln (Diagonal)
    w->grid[0 * stride + 0] = w->grid[w->rows * stride + w->cols];
    w->grid[0 * stride + (w->cols + 1)] = w->grid[w->rows * stride + 1];
    w->grid[(w->rows + 1) * stride + 0] = w->grid[1 * stride + w->cols];
    w->grid[(w->rows + 1) * stride + (w->cols + 1)] = w->grid[1 * stride + 1];
}


// Helper for update_generation: checks if a chunk or any of its 8 neighbors were active
static bool is_chunk_or_neighbors_active(World *w, int cr, int cc) {
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            int ncr = cr + dr;
            int ncc = cc + dc;
            // Wrapping chunk coordinates
            if (ncr < 0) ncr = w->chunk_rows - 1;
            else if (ncr >= w->chunk_rows) ncr = 0;
            if (ncc < 0) ncc = w->chunk_cols - 1;
            else if (ncc >= w->chunk_cols) ncc = 0;
            
            if (w->chunk_map[ncr * w->chunk_cols + ncc]) return true;
        }
    }
    return false;
}

// KI-Agent unterstützt: Parallelized update returns total activity (births + deaths)
int update_generation(World *current_gen, World *next_gen, int rows, int cols, int *red_pop, int *blue_pop) {
    sync_ghost_borders(current_gen);

    int stride = cols + 2;
    int total_red = 0;
    int total_blue = 0;
    int total_activity = 0;

    // 1. Clear the NEXT generation's chunk map
    if (next_gen->chunk_map) {
        for (int i = 0; i < next_gen->chunk_rows * next_gen->chunk_cols; i++) {
            next_gen->chunk_map[i] = 0;
        }
    }

    // 2. Iterate over CHUNKS
#ifndef PLATFORM_WEB
    #pragma omp parallel for reduction(+:total_red, total_blue, total_activity)
#endif
    for (int cr = 0; cr < current_gen->chunk_rows; cr++) {
        for (int cc = 0; cc < current_gen->chunk_cols; cc++) {
            int chunk_idx = cr * current_gen->chunk_cols + cc;

            if (current_gen->chunk_map && !is_chunk_or_neighbors_active(current_gen, cr, cc)) {
                continue;
            }

            int start_r = cr * CHUNK_SIZE + 1;
            int end_r = (cr + 1) * CHUNK_SIZE;
            if (end_r > rows) end_r = rows;

            int start_c = cc * CHUNK_SIZE + 1;
            int end_c = (cc + 1) * CHUNK_SIZE;
            if (end_c > cols) end_c = cols;

            bool chunk_has_life = false;

            for (int r = start_r; r <= end_r; r++) {
                for (int c = start_c; c <= end_c; c++) {
                    int i = r * stride + c;
                    
                    int red_neighbors = 0;
                    int blue_neighbors = 0;

                    int n_indices[8] = {
                        i - stride - 1, i - stride, i - stride + 1,
                        i - 1,                      i + 1,
                        i + stride - 1, i + stride, i + stride + 1
                    };

                    for (int k = 0; k < 8; k++) {
                        int val = current_gen->grid[n_indices[k]];
                        if (val == TEAM_RED) red_neighbors++;
                        else if (val == TEAM_BLUE) blue_neighbors++;
                    }

                    int total_neighbors = red_neighbors + blue_neighbors;
                    int current_cell = current_gen->grid[i];
                    int new_state = DEAD;

                    if (current_cell != DEAD) {
                        if (total_neighbors == 2 || total_neighbors == 3) new_state = current_cell;
                    } else {
                        if (total_neighbors == 3) {
                            new_state = (red_neighbors > blue_neighbors) ? TEAM_RED : TEAM_BLUE;
                        }
                    }
                    
                    if (new_state != current_cell) total_activity++;

                    next_gen->grid[i] = new_state;
                    
                    if (new_state != DEAD) {
                        chunk_has_life = true;
                        if (new_state == TEAM_RED) total_red++;
                        else total_blue++;
                    }
                }
            }

            if (chunk_has_life && next_gen->chunk_map) {
                next_gen->chunk_map[chunk_idx] = 1;
            }
        }
    }

    *red_pop = total_red;
    *blue_pop = total_blue;
    return total_activity;
}

// KI-Agent unterstützt: Bitboard helpers
uint64_t grid_to_bitboard(int cells[LOCAL_GRID_SIZE][LOCAL_GRID_SIZE]) {
    uint64_t bb = 0;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (cells[r][c]) bb |= (1ULL << (r * 8 + c));
        }
    }
    return bb;
}

void bitboard_to_grid(uint64_t bb, int cells[LOCAL_GRID_SIZE][LOCAL_GRID_SIZE]) {
    for (int i = 0; i < 64; i++) {
        cells[i / 8][i % 8] = (bb & (1ULL << i)) ? 1 : 0;
    }
}

// KI-Agent unterstützt: Thread-safe match execution with activity tracking
MatchResult run_isolated_match(int left_cells[LOCAL_GRID_SIZE][LOCAL_GRID_SIZE], int right_cells[LOCAL_GRID_SIZE][LOCAL_GRID_SIZE], int max_gen) {
    MatchResult result = { 0, 0, 0, 0, 0, 0 };
    int rows = LOCAL_GRID_SIZE;
    int cols = LOCAL_GRID_SIZE * 2;
    int r_pop = 0, b_pop = 0;

    World *current = create_world(rows, cols);
    World *next = create_world(rows, cols);

    for (int r = 0; r < LOCAL_GRID_SIZE; r++) {
        for (int c = 0; c < LOCAL_GRID_SIZE; c++) {
            if (left_cells[r][c]) {
                current->grid[(r + 1) * (cols + 2) + (c + 1)] = TEAM_RED;
                activate_chunk_at(current, r, c);
            }
            if (right_cells[r][c]) {
                current->grid[(r + 1) * (cols + 2) + (c + LOCAL_GRID_SIZE + 1)] = TEAM_BLUE;
                activate_chunk_at(current, r, c + LOCAL_GRID_SIZE);
            }
        }
    }

    for (int gen = 1; gen <= max_gen; gen++) {
        result.activity_sum += update_generation(current, next, rows, cols, &r_pop, &b_pop);
        result.total_generations = gen;

        if (memcmp(current->grid, next->grid, sizeof(int) * (rows + 2) * (cols + 2)) == 0) {
            result.stable_at_generation = gen;
            break;
        }

        World *temp = current;
        current = next;
        next = temp;
    }

    result.red_final_pop = r_pop;
    result.blue_final_pop = b_pop;
    if (r_pop > b_pop) result.winner = TEAM_RED;
    else if (b_pop > r_pop) result.winner = TEAM_BLUE;
    else result.winner = 0;

    free_world(current);
    free_world(next);
    return result;
}

// KI-Agent unterstützt: Helper to activate a chunk given a 0-based grid coordinate
void activate_chunk_at(World *w, int r, int c) {
    if (!w || !w->chunk_map) return;
    int cr = r / CHUNK_SIZE;
    int cc = c / CHUNK_SIZE;
    if (cr >= 0 && cr < w->chunk_rows && cc >= 0 && cc < w->chunk_cols) {
        w->chunk_map[cr * w->chunk_cols + cc] = 1;
    }
}

// KI-Agent unterstützt: Initialize simulation context
void init_simulation_context(SimulationContext *ctx, int rows, int cols) {
    if (!ctx) return;
    ctx->rows = rows;
    ctx->cols = cols;
    ctx->current_generation = 0;
    ctx->max_generations = MAX_ROUNDS;
    ctx->world_a = create_world(rows, cols);
    ctx->world_b = create_world(rows, cols);
    ctx->current_world = ctx->world_a;
    ctx->next_world = ctx->world_b;
    memset(ctx->participant_red, 0, sizeof(ctx->participant_red));
    memset(ctx->participant_blue, 0, sizeof(ctx->participant_blue));
    ctx->is_active = false;
}

// KI-Agent unterstützt: Free simulation context resources safely
void free_simulation_context(SimulationContext *ctx) {
    if (!ctx) return;
    if (ctx->world_a) {
        free_world(ctx->world_a);
        ctx->world_a = NULL;
    }
    if (ctx->world_b) {
        free_world(ctx->world_b);
        ctx->world_b = NULL;
    }
    ctx->current_world = NULL;
    ctx->next_world = NULL;
    ctx->is_active = false;
}

// KI-Agent unterstützt: Reset simulation context for a new interactive session (ADR-0020)
void reset_simulation_context(SimulationContext *ctx, int rows, int cols) {
    if (!ctx) return;

    // If dimensions match, just clear the grids (fast path — avoids malloc/free)
    if (ctx->world_a && ctx->world_a->rows == rows && ctx->world_a->cols == cols) {
        int stride = cols + 2;
        int total = (rows + 2) * stride;
        memset(ctx->world_a->grid, 0, total * sizeof(int));
        memset(ctx->world_b->grid, 0, total * sizeof(int));
        if (ctx->world_a->chunk_map)
            memset(ctx->world_a->chunk_map, 0, ctx->world_a->chunk_rows * ctx->world_a->chunk_cols);
        if (ctx->world_b->chunk_map)
            memset(ctx->world_b->chunk_map, 0, ctx->world_b->chunk_rows * ctx->world_b->chunk_cols);
    } else {
        // Dimensions changed: full teardown + rebuild
        if (ctx->world_a) free_world(ctx->world_a);
        if (ctx->world_b) free_world(ctx->world_b);
        ctx->world_a = create_world(rows, cols);
        ctx->world_b = create_world(rows, cols);
    }

    ctx->rows = rows;
    ctx->cols = cols;
    ctx->current_generation = 0;
    ctx->max_generations = MAX_ROUNDS;
    ctx->current_world = ctx->world_a;
    ctx->next_world = ctx->world_b;
    ctx->is_active = false;
}

// KI-Agent unterstützt: Advance the simulation state on the context
int update_generation_ctx(SimulationContext *ctx, int *red_pop, int *blue_pop) {
    if (!ctx || !ctx->current_world || !ctx->next_world) return 0;
    
    int activity = update_generation(ctx->current_world, ctx->next_world, ctx->rows, ctx->cols, red_pop, blue_pop);
    
    // Swap front and back double buffers
    World *temp = ctx->current_world;
    ctx->current_world = ctx->next_world;
    ctx->next_world = temp;
    
    ctx->current_generation++;
    return activity;
}
