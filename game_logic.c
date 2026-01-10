#include <stdlib.h>
#include <time.h>
#include "game_logic.h"

// KI-Agent unterstützt
World* create_world(int rows, int cols) {
    World *w = malloc(sizeof(World));
    w->rows = rows;
    w->cols = cols;
    w->grid = malloc(rows * cols * sizeof(int));
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
    for (int i = 0; i < (rows * cols); i++) {
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

// KI-Agent unterstützt
void update_generation(World *current_gen, World *next_gen, int rows, int cols) {
    // Macro to check a neighbor index and increment counters
    // Using a macro avoids function call overhead in the tight loop
    #define COUNT_NEIGHBOR(idx) \
        if (current_gen->grid[idx] == TEAM_RED) red_neighbors++; \
        else if (current_gen->grid[idx] == TEAM_BLUE) blue_neighbors++;

    for (int i = 0; i < (rows * cols); i++) {
        int red_neighbors = 0;
        int blue_neighbors = 0;

        // --- Neighbor Counting (Unrolled for Edges) ---

        // Sonderfall: i = Eckzelle oben, links
        if (i == 0) {
            COUNT_NEIGHBOR(i + (cols * rows) - 1);           // oberhalb, links
            COUNT_NEIGHBOR(i + (cols * (rows - 1)));         // oberhalb, mittig
            COUNT_NEIGHBOR(i + (cols * (rows - 1)) + 1);     // oberhalb, rechts
            COUNT_NEIGHBOR(i + cols - 1);                    // links
            COUNT_NEIGHBOR(i + 1);                           // rechts
            COUNT_NEIGHBOR(i + cols + cols - 1);             // unterhalb, links
            COUNT_NEIGHBOR(i + cols);                        // unterhalb, mittig
            COUNT_NEIGHBOR(i + cols + 1);                    // unterhalb, rechts
        }
        // Sonderfall: i = Eckzelle oben, rechts
        else if (i == (cols - 1)) {
            COUNT_NEIGHBOR(i + (cols * (rows - 1)) - 1);     // oberhalb, links
            COUNT_NEIGHBOR(i + (cols * (rows - 1)));         // oberhalb, mittig
            COUNT_NEIGHBOR(i + (cols * (rows - 1)) - cols + 1); // oberhalb, rechts
            COUNT_NEIGHBOR(i - 1);                           // links
            COUNT_NEIGHBOR(i - (cols - 1));                  // rechts
            COUNT_NEIGHBOR(i + cols - 1);                    // unterhalb, links
            COUNT_NEIGHBOR(i + cols);                        // unterhalb, mittig
            COUNT_NEIGHBOR(i + 1);                           // unterhalb, rechts
        }
        // Sonderfall i = Eckzelle unten, rechts
        else if (i == (cols * rows - 1)) {
            COUNT_NEIGHBOR(i - cols - 1);                    // oberhalb, links
            COUNT_NEIGHBOR(i - cols);                        // oberhalb, mittig
            COUNT_NEIGHBOR(i - cols - (cols - 1));           // oberhalb, rechts
            COUNT_NEIGHBOR(i - 1);                           // links
            COUNT_NEIGHBOR(i - (cols - 1));                  // rechts
            COUNT_NEIGHBOR(i - (cols * (rows - 1)) - 1);     // unterhalb, links
            COUNT_NEIGHBOR(i - (cols * (rows - 1)));         // unterhalb, mittig
            COUNT_NEIGHBOR(i - (cols * (rows)) + 1);         // unterhalb, rechts
        }
        // Sonderfall: i = Eckzelle unten, links
        else if (i == (cols * (rows - 1))) {
            COUNT_NEIGHBOR(i - 1);                           // oberhalb, links
            COUNT_NEIGHBOR(i - cols);                        // oberhalb, mittig
            COUNT_NEIGHBOR(i - cols + 1);                    // oberhalb, rechts
            COUNT_NEIGHBOR(i + (cols - 1));                  // links
            COUNT_NEIGHBOR(i + 1);                           // rechts
            COUNT_NEIGHBOR(i - (cols * (rows - 2)) - 1);     // unterhalb, links
            COUNT_NEIGHBOR(i - (cols * (rows - 1)));         // unterhalb, mittig
            COUNT_NEIGHBOR(i - (cols * (rows -1)) + 1);      // unterhalb, rechts
        }
        // Sonderfall: i = Zelle am oberen Rand
        else if (i > 0 && i < cols) {
            COUNT_NEIGHBOR(i + (cols * (rows - 1)) - 1);
            COUNT_NEIGHBOR(i + (cols * (rows - 1)));
            COUNT_NEIGHBOR(i + (cols * (rows - 1)) + 1);
            COUNT_NEIGHBOR(i - 1);
            COUNT_NEIGHBOR(i + 1);
            COUNT_NEIGHBOR(i + cols - 1);
            COUNT_NEIGHBOR(i + cols);
            COUNT_NEIGHBOR(i + cols + 1);
        }
        // Sonderfall: i = Zelle am unteren Rand
        else if (i > (cols * (rows - 1))) {
            // Nachbarn OBEN (ganz normal)
            COUNT_NEIGHBOR(i - cols - 1);
            COUNT_NEIGHBOR(i - cols);
            COUNT_NEIGHBOR(i - cols + 1);
            
            // Nachbarn SEITE
            COUNT_NEIGHBOR(i - 1);
            COUNT_NEIGHBOR(i + 1);
            
            // Nachbarn UNTEN (Wrap to Top Row)
            int top_row_idx = i - (cols * (rows - 1));
            COUNT_NEIGHBOR(top_row_idx - 1);
            COUNT_NEIGHBOR(top_row_idx);
            COUNT_NEIGHBOR(top_row_idx + 1);
        }
        // Sonderfall: i = Zelle am linken Rand
        else if (i != 0 && i % cols == 0) {
            COUNT_NEIGHBOR(i - cols + cols - 1);
            COUNT_NEIGHBOR(i - cols);
            COUNT_NEIGHBOR(i - cols + 1);
            COUNT_NEIGHBOR(i + cols - 1);
            COUNT_NEIGHBOR(i + 1);
            COUNT_NEIGHBOR(i + cols + cols - 1);
            COUNT_NEIGHBOR(i + cols);
            COUNT_NEIGHBOR(i + cols + 1);
        }
        // Sonderfall: i = Zelle am rechten Rand
        else if ((i + 1) % cols == 0) {
            COUNT_NEIGHBOR(i - cols - 1);
            COUNT_NEIGHBOR(i - cols);
            COUNT_NEIGHBOR(i - cols + 1 - (cols));
            COUNT_NEIGHBOR(i - 1);
            COUNT_NEIGHBOR(i + 1 - (cols));
            COUNT_NEIGHBOR(i + cols - 1);
            COUNT_NEIGHBOR(i + cols);
            COUNT_NEIGHBOR(i + cols + 1 - (cols));
        }
        // Zellen innerhalb Spielfeld
        else {
            COUNT_NEIGHBOR(i - cols - 1);
            COUNT_NEIGHBOR(i - cols);
            COUNT_NEIGHBOR(i - cols + 1);
            COUNT_NEIGHBOR(i - 1);
            COUNT_NEIGHBOR(i + 1);
            COUNT_NEIGHBOR(i + cols - 1);
            COUNT_NEIGHBOR(i + cols);
            COUNT_NEIGHBOR(i + cols + 1);
        }

        // --- Evolution Rules ---
        int total_neighbors = red_neighbors + blue_neighbors;
        int current_cell = current_gen->grid[i];
        
        if (current_cell != DEAD) {
            // SURVIVAL: 2 or 3 neighbors -> stay alive
            if (total_neighbors == 2 || total_neighbors == 3) {
                next_gen->grid[i] = current_cell;
            } else {
                next_gen->grid[i] = DEAD;
            }
        }
        else {
            // BIRTH: exactly 3 neighbors -> become alive
            if (total_neighbors == 3) {
                // Determine color by majority
                if (red_neighbors > blue_neighbors) {
                    next_gen->grid[i] = TEAM_RED;
                } else {
                    next_gen->grid[i] = TEAM_BLUE;
                }
            } else {
                next_gen->grid[i] = DEAD;
            }
        }
    }
    #undef COUNT_NEIGHBOR
}
