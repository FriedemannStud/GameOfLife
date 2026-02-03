#include <stdlib.h>
#include <time.h>
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


// KI-Agent unterstützt
void update_generation(World *current_gen, World *next_gen, int rows, int cols, int *red_pop, int *blue_pop) {
    // Reset Counters
    *red_pop = 0;
    *blue_pop = 0;

    // Vor der Berechnung: Geister-Ränder mit echten Daten füllen (Wrapping)
    sync_ghost_borders(current_gen);


    // Macro to check a neighbor index and increment counters
    // Using a macro avoids function call overhead in the tight loop
    #define COUNT_NEIGHBOR(idx) \
        if (current_gen->grid[idx] == TEAM_RED) red_neighbors++; \
        else if (current_gen->grid[idx] == TEAM_BLUE) blue_neighbors++;

    int stride = cols + 2;

    for (int r = 1; r <= rows; r++) {
        for (int c = 1; c <= cols; c++) {
            int i = r * stride +c;
             
            int red_neighbors = 0;
            int blue_neighbors = 0;

                COUNT_NEIGHBOR(i - stride -1);
                COUNT_NEIGHBOR(i - stride);
                COUNT_NEIGHBOR(i - stride + 1);
                COUNT_NEIGHBOR(i - 1);
                COUNT_NEIGHBOR(i + 1);
                COUNT_NEIGHBOR(i + stride - 1);
                COUNT_NEIGHBOR(i + stride);
                COUNT_NEIGHBOR(i + stride + 1);
            

            // --- Evolution Rules ---
            int total_neighbors = red_neighbors + blue_neighbors;
            int current_cell = current_gen->grid[i];
            
            int new_state = DEAD;

            if (current_cell != DEAD) {
                // SURVIVAL: 2 or 3 neighbors -> stay alive
                if (total_neighbors == 2 || total_neighbors == 3) {
                    new_state = current_cell;
                }
            }
            else {
                // BIRTH: exactly 3 neighbors -> become alive
                if (total_neighbors == 3) {
                    // Determine color by majority
                    if (red_neighbors > blue_neighbors) {
                        new_state = TEAM_RED;
                    }
                    else {
                        new_state = TEAM_BLUE;
                    }
                }
            }
            
            next_gen->grid[i] = new_state;
            
            // --- Integrated Counting ---
            if (new_state == TEAM_RED) (*red_pop)++;
            else if (new_state == TEAM_BLUE) (*blue_pop)++;
        }
    }
    #undef COUNT_NEIGHBOR
}