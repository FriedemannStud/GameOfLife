#include <stdio.h>
#include <stdlib.h>

#define ROWS 35  
#define COLS 150

// Struktur definieren
typedef struct {
    int *grid; // Pointer auf Array
    int r;
    int c;
} World;

// Prototypen
void init_world(World *current_gen);
void print_world(World *current_gen);
// void update_generation(World current_gen, World next_gen);


int main(void) {
    // Dynamische Speicherverwaltung für zwei Gitter
    // Zwei Gitter, um neuen zustand berechnen zu können, ohne den aktuellen Zustand zu beeinflussen, analog zu "Bildbearbeituns-Übung"
    World *current_gen = malloc(sizeof(World));
    World *next_gen = malloc(sizeof(World));

    // Speicher für die Zellen (Arrays) reservieren
    current_gen->grid = malloc(ROWS * COLS * sizeof(int));
    next_gen->grid = malloc(ROWS * COLS * sizeof(int));

    // Initialisierung
    // Zufälliges Muster lebender Zellen
    init_world(current_gen);


    // Aktuelle Population ausgeben
    print_world(current_gen);

    // Spiel-Schleife (Loop)
    //while (true) {
    //    print_world(current_gen);
    //    update_generation(current_gen, next_gen);

        // Tausche die Gitter (Pointer-Zuweisung)
    //    World *temp = current_gen;
    //    current_gen = next_gen;
    //    next_gen = temp;

        // Abbruchbedingung oder Pause noch einfügen
    //}

    // Speicher freigeben (Vermeidung von Memory Leaks)
    free(current_gen->grid);
//    free(next_gen->grid);
    free(current_gen);
//    free(next_gen);

    return 0;
}

void init_world(World *current_gen)
{
    for (int i = 0; i < (ROWS * COLS); i++)
    {
        current_gen->grid[i] = rand() % 2; // Zufällige 0 oder 1
    }
}

void print_world(World *current_gen)
// Zunächst ohne GUI als Zeichen-Matrix 1 und 0
{
    for (int i = 0; i < (ROWS * COLS); i++)
    {
        printf("%i", current_gen->grid[i]);
        if ((i+1) % COLS == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
}