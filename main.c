/*
 * Game of Life - Competitive Biotope Mode (Red vs Blue)
 * 
 * Rules:
 * - Standard Conway survival rules (2 or 3 neighbors).
 * - Birth rule (3 neighbors): Majority color of parents determines child color.
 * - Teams: Red (X) vs Blue (O).
 * - Goal: Highest population after MAX_ROUNDS.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h> // zur Initialisierung des Zufallszahlengenerators
#include <unistd.h> // zur Verlangsamung der Ausführung mit sleep()

#define DEAD 0
#define TEAM_RED 1
#define TEAM_BLUE 2
#define MAX_ROUNDS 1000

// Struktur definieren
typedef struct {
    int *grid; // Pointer auf Array
    int r;
    int c;
} World;

// Prototypen
void init_world(World *current_gen, int rows, int cols);
void print_world(World *current_gen, int rows, int cols);
void update_generation(World *current_gen, World *next_gen, int rows, int cols);


int main(int argc, char *argv[]) {
    printf("argc: %i\n", argc);
    if (argc < 2)
    {
        printf("./main <rows> <columns> <delay milli-sec>\n");
        return 1;
    }
    
    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);
    
    if (rows > 2000 || cols > 2000) {
        printf("Error: Grid too large (max 2000x2000)\n");
        return 1;
    }

    int delay_my = atoi(argv[3]) * 1000;
    // Dynamische Speicherverwaltung für zwei Gitter
    // Zwei Gitter, um neuen zustand berechnen zu können, ohne den aktuellen Zustand zu beeinflussen, analog zu "Bildbearbeituns-Übung"
    World *current_gen = malloc(sizeof(World));
    World *next_gen = malloc(sizeof(World));

    // Speicher für die Zellen (Arrays) reservieren
    current_gen->grid = malloc(rows * cols * sizeof(int));
    next_gen->grid = malloc(rows * cols * sizeof(int));

    // Initialisierung
    // Zufälliges Muster lebender Zellen
    init_world(current_gen, rows, cols);

    // Spiel-Schleife (Loop)
    int turns = 0;
    while (turns < MAX_ROUNDS)
     {
        // Aktuelle Population ausgeben
        print_world(current_gen, rows, cols);
        printf("Turn: %i / %d\n", turns, MAX_ROUNDS);
        usleep(delay_my);
        update_generation(current_gen, next_gen, rows, cols);

        // Die neu berechnete Generation wird zur aktuellen Generation (Pointer-Swap)
        World *temp = current_gen;
        current_gen = next_gen;
        next_gen = temp;
        
        turns++;
    }

    // Game Over - Determine Winner
    int final_red = 0;
    int final_blue = 0;
    for (int i = 0; i < rows * cols; i++) {
        if (current_gen->grid[i] == TEAM_RED) final_red++;
        else if (current_gen->grid[i] == TEAM_BLUE) final_blue++;
    }

    printf("\n--- GAME OVER ---\n");
    printf("Final Score:\nRed: %d\nBlue: %d\n", final_red, final_blue);
    if (final_red > final_blue) {
        printf("Winner: RED TEAM\n");
    } else if (final_blue > final_red) {
        printf("Winner: BLUE TEAM\n");
    } else {
        printf("Result: DRAW\n");
    }

    // Speicher freigeben (Vermeidung von Memory Leaks)
    free(current_gen->grid);
    free(next_gen->grid);
    free(current_gen);
    free(next_gen);

    return 0;
}

void init_world(World *current_gen, int rows, int cols)
{
    // Initialisiere den Zufallszahlengenerator mit der aktuellen Zeit
    srand(time(NULL));
    for (int i = 0; i < (rows * cols); i++)
    {
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

void print_world(World *current_gen, int rows, int cols)
// Zunächst ohne GUI als Zeichen-Matrix 1 und 0
{
    system("clear");
    int red_count = 0;
    int blue_count = 0;

    for (int i = 0; i < (rows * cols); i++)
    {
        if (current_gen->grid[i] == TEAM_RED) {
            printf("X"); // Could add ANSI colors later
            red_count++;
        } else if (current_gen->grid[i] == TEAM_BLUE) {
            printf("O");
            blue_count++;
        } else {
            printf(" ");
        }

        if ((i+1) % cols == 0)
        {
            printf("\n");
        }
    }
    printf("\nStats: Red: %d | Blue: %d\n", red_count, blue_count);
}

void update_generation(World *current_gen, World *next_gen, int rows, int cols)
{
    // Macro to check a neighbor index and increment counters
    // Using a macro avoids function call overhead in the tight loop
    #define COUNT_NEIGHBOR(idx) \
        if (current_gen->grid[idx] == TEAM_RED) red_neighbors++; \
        else if (current_gen->grid[idx] == TEAM_BLUE) blue_neighbors++;

    for (int i = 0; i < (rows * cols); i++)
    {
        int red_neighbors = 0;
        int blue_neighbors = 0;

        // --- Neighbor Counting (Unrolled for Edges) ---

        // Sonderfall: i = Eckzelle oben, links
        if (i == 0)
        {
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
        else if (i == (cols - 1))
        {
            COUNT_NEIGHBOR(i + (cols * (rows - 1)) - 1);     // oberhalb, links
            COUNT_NEIGHBOR(i + (cols * (rows - 1)));         // oberhalb, mittig
            COUNT_NEIGHBOR(i + (cols * (rows - 1)) - cols + 1); // oberhalb, rechts
            COUNT_NEIGHBOR(i - 1);                           // links
            COUNT_NEIGHBOR(i - (cols - 1));                  // rechts
            COUNT_NEIGHBOR(i + cols - 1);                    // unterhalb, links
            COUNT_NEIGHBOR(i + cols);                        // unterhalb, mittig
            COUNT_NEIGHBOR(i + 1);                           // unterhalb, rechts (Note: was i+1 in original)
        }
        // Sonderfall i = Eckzelle unten, rechts
        else if (i == (cols * rows - 1))
        {
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
        else if (i == (cols * (rows - 1)))
        {
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
        else if (i > 0 && i < cols)
        {
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
        else if (i > (cols * (rows - 1)))
        {
            COUNT_NEIGHBOR(i + cols - 1);
            COUNT_NEIGHBOR(i + cols);
            COUNT_NEIGHBOR(i + cols + 1);
            COUNT_NEIGHBOR(i - 1);
            COUNT_NEIGHBOR(i + 1);
            COUNT_NEIGHBOR(i - (cols * (rows - 1)) - 1);
            COUNT_NEIGHBOR(i - (cols * (rows - 1)));
            COUNT_NEIGHBOR(i - (cols * (rows - 1)) + 1);
        }
        // Sonderfall: i = Zelle am linken Rand
        else if (i != 0 && i % cols == 0)
        {
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
        else if ((i + 1) % cols == 0)
        {
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
        else
        {
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
        
        if (current_cell != DEAD) 
        {
            // SURVIVAL: 2 or 3 neighbors -> stay alive
            if (total_neighbors == 2 || total_neighbors == 3) {
                next_gen->grid[i] = current_cell;
            } else {
                next_gen->grid[i] = DEAD;
            }
        }
        else // Dead cell
        {
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