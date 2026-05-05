#include <stdlib.h>
#include <time.h>
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
    // Correct parenthesis and 2 arguments for calloc(count, size)
    w->grid = calloc((rows + 2) * (cols + 2), sizeof(int)); // calloc() belegt den Speicher explizit mit 0.
    return w;
}

// KI-Agent unterstützt
void free_world(World *w) {
    if (w) {
        if (w->grid) free(w->grid);
        free(w);
    }
}

// KI-Agent unterstützt
void init_world(World *current_gen, int rows, int cols) {
    // Initialisiere den Zufallszahlengenerator mit der aktuellen Zeit
    srand(time(NULL));
    
    int stride = cols + 2;
    
    // Nested loops to skip the ghost borders (start at 1, end at rows/cols)
    for (int r = 1; r <= rows; r++) {
        for (int c = 1; c <= cols; c++) {
            int i = r * stride + c;
            
            int val = rand() % 100; 
            if (val < 10) {
                current_gen->grid[i] = TEAM_RED;
            } else if (val < 20) {
                current_gen->grid[i] = TEAM_BLUE;
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


// KI-Agent unterstützt: Parallelized update using OpenMP
void update_generation(World *current_gen, World *next_gen, int rows, int cols, int *red_pop, int *blue_pop) {
    // Vor der Berechnung: Geister-Ränder mit echten Daten füllen (Wrapping)
    sync_ghost_borders(current_gen);

    int stride = cols + 2;
    int local_red = 0;
    int local_blue = 0;

    // OpenMP Parallelization: Split the outer loop across CPU cores
    // reduction(+:local_red, local_blue) ensures each thread counts safely
#ifndef PLATFORM_WEB
    #pragma omp parallel for reduction(+:local_red, local_blue)
#endif
    for (int r = 1; r <= rows; r++) {
        for (int c = 1; c <= cols; c++) {
            int i = r * stride + c;
            
            int red_neighbors = 0;
            int blue_neighbors = 0;

            // Manual neighbor check (Top row)
            if (current_gen->grid[i - stride - 1] == TEAM_RED) red_neighbors++;
            else if (current_gen->grid[i - stride - 1] == TEAM_BLUE) blue_neighbors++;
            if (current_gen->grid[i - stride] == TEAM_RED) red_neighbors++;
            else if (current_gen->grid[i - stride] == TEAM_BLUE) blue_neighbors++;
            if (current_gen->grid[i - stride + 1] == TEAM_RED) red_neighbors++;
            else if (current_gen->grid[i - stride + 1] == TEAM_BLUE) blue_neighbors++;
            
            // Middle row
            if (current_gen->grid[i - 1] == TEAM_RED) red_neighbors++;
            else if (current_gen->grid[i - 1] == TEAM_BLUE) blue_neighbors++;
            if (current_gen->grid[i + 1] == TEAM_RED) red_neighbors++;
            else if (current_gen->grid[i + 1] == TEAM_BLUE) blue_neighbors++;
            
            // Bottom row
            if (current_gen->grid[i + stride - 1] == TEAM_RED) red_neighbors++;
            else if (current_gen->grid[i + stride - 1] == TEAM_BLUE) blue_neighbors++;
            if (current_gen->grid[i + stride] == TEAM_RED) red_neighbors++;
            else if (current_gen->grid[i + stride] == TEAM_BLUE) blue_neighbors++;
            if (current_gen->grid[i + stride + 1] == TEAM_RED) red_neighbors++;
            else if (current_gen->grid[i + stride + 1] == TEAM_BLUE) blue_neighbors++;

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
            
            next_gen->grid[i] = new_state;
            
            if (new_state == TEAM_RED) local_red++;
            else if (new_state == TEAM_BLUE) local_blue++;
        }
    }

    // Write final totals back
    *red_pop = local_red;
    *blue_pop = local_blue;
}