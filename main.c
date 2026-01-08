#include <stdio.h>
#include <stdlib.h>
#include <time.h> // zur Initialisierung des Zufallszahlengenerators

#define ROWS 10 
#define COLS 10

// Struktur definieren
typedef struct {
    int *grid; // Pointer auf Array
    int r;
    int c;
} World;

// Prototypen
void init_world(World *current_gen);
void print_world(World *current_gen);
void update_generation(World *current_gen, World *next_gen);


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

    // Spiel-Schleife (Loop)
    int turns = 0;
    while (turns < 10)
     {
        // Aktuelle Population ausgeben
        printf("turns: %i\n", turns);
        printf("current_gen:\n");
        print_world(current_gen);
        update_generation(current_gen, next_gen);
        // printf("next_gen:\n");
        // print_world(next_gen);

        // Die neu berechnete Generation wird zur aktuellen Generation (Pointer-Zuweisung)
        current_gen = next_gen;
        
        // Abbruchbedingung oder Pause noch einfügen
        turns++;
    }

    // Speicher freigeben (Vermeidung von Memory Leaks)
    free(current_gen->grid);
    free(next_gen->grid);
    free(current_gen);
    free(next_gen);

    return 0;
}

void init_world(World *current_gen)
{
    // Initialisiere den Zufallszahlengenerator mit der aktuellen Zeit
    srand(time(NULL));
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
        printf("%i  ", current_gen->grid[i]);
        if ((i+1) % COLS == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
}

void update_generation(World *current_gen, World *next_gen)
{
    int zellen = 0;
    for (int i = 0; i < (ROWS * COLS); i++)
    {
        //next_generation Zellenweise berechnen
        //Anzahl lebender Zellen zählen

        //wenn i oben, linke Eckzelle, muss Status der Zellen ... (beschreiben)
        if (i == 0)
        {
            zellen = 
            current_gen->grid[i + (COLS * ROWS) - 1] + // oberhalb, links -> letzte Zeile, letzte Spalte
            current_gen->grid[i + (COLS * (ROWS - 1))] + // oberhalb, mittig -> letzte Zeile, erste Spalte
            current_gen->grid[i + (COLS * (ROWS - 1)) + 1] + // oberhalt, rechts -> letzte Zeile, zweite Spalte
            current_gen->grid[i + COLS - 1] + // links -> erste Zeile, letzte Spalte
            current_gen->grid[i + 1] +
            current_gen->grid[i + COLS + COLS - 1] + // unterhalb, links -> zweite Zeile, letzte Spalte
            current_gen->grid[i + COLS] +
            current_gen->grid[i + COLS + 1];
            
            printf(" Eckzelle oben, links\n");
            printf("i: %i->i statt oberhalb, links: %i\n", i, i + (COLS * ROWS) - 1);
            printf("i: %i->i statt oberhalb, mittig: %i\n", i, i + (COLS * (ROWS - 1)));
            printf("i: %i->i statt oberhalb, rechts: %i\n", i, i + (COLS * (ROWS - 1)) + 1);
            printf("i: %i->i statt links: %i\n", i, i + COLS - 1);
            printf("i: %i->i statt unterhalb, links: %i\n", i, i + COLS + COLS - 1);
        }


        //wenn i oben, rechte Eckzelle, muss Status der Zellen ... (beschreiben)
        else if (i == (COLS - 1))
        {
            zellen = 
            current_gen->grid[i + (COLS * (ROWS - 1)) - 1] + // oberhalb, links -> letzte Zeile, vorletzte Spalte
            current_gen->grid[i + (COLS * (ROWS - 1))] + // oberhalb, mittig -> letzte Zeile, letzte Spalte
            current_gen->grid[i + (COLS * (ROWS - 1)) - COLS + 1] + // oberhalb, rechts -> letzte Zeile, erste Spalte
            current_gen->grid[i - 1] +
            current_gen->grid[i - (COLS - 1)] + // rechts -> erste Zeile, erste Spalte
            current_gen->grid[i + COLS - 1] +
            current_gen->grid[i + COLS] +
            current_gen->grid[i + 1]; // unterhalb, rechts -> zweite Zeile, erste Spalte
            
            printf(" Eckzelle oben, rechts\n");
            printf("i: %i->i statt oberhalb, links: %i\n", i, i + (COLS * (ROWS - 1)) - 1);
            printf("i: %i->i statt oberhalb, mittig: %i\n", i, i + (COLS * (ROWS - 1)));
            printf("i: %i->i statt oberhalb, rechts: %i\n", i, i + (COLS * (ROWS - 1)) - COLS + 1);
            printf("i: %i->i statt rechts: %i\n", i, i - (COLS - 1));
            printf("i: %i->i statt unterhalb, rechts: %i\n", i, i + 1);
        }

        //wenn i unten, rechte Eckzelle, muss Status der Zellen ... (beschreiben)
        else if (i == (COLS * ROWS - 1))
        {
            zellen = 
            current_gen->grid[i - COLS - 1] +
            current_gen->grid[i - COLS] +
            current_gen->grid[i - COLS - (COLS - 1)] + // oberhalb, rechts -> vorletzte Zeile, erste Spalte
            current_gen->grid[i - 1] +
            current_gen->grid[i - (COLS - 1)] + // rechts -> letzte Zeile, erste Spalte
            current_gen->grid[i - (COLS * (ROWS - 1)) - 1] + // unterhalb, links -> erste Zeile, vorletzte Spalte
            current_gen->grid[i - (COLS * (ROWS - 1))] + // unterhalb, mittig -> erste Zeile, letzte Spalte
            current_gen->grid[i - (COLS * (ROWS)) + 1]; // unterhalb, rechts -> erste Zeile, erste Spalte
            
            printf(" Eckzelle unten, rechts\n");
            printf("i: %i->i statt oberhalb, rechts: %i\n", i, i - COLS - (COLS - 1));
            printf("i: %i->i statt rechts: %i\n", i, i - (COLS - 1));
            printf("i: %i->i statt unterhalb, links: %i\n", i, i - (COLS * (ROWS - 1)) - 1);
            printf("i: %i->i statt unterhalb, mittig: %i\n", i, i - (COLS * (ROWS - 1)));
            printf("i: %i->i statt unterhalb, rechts: %i\n", i, i - (COLS * (ROWS)) + 1);
        }

        //wenn i unten, linke Eckzelle, muss Status der Zellen ... (beschreiben)
        else if (i == (COLS * (ROWS - 1)))
        {
            zellen = 
            current_gen->grid[i - 1] + // oberhalb, links -> vorletzte Zeile, letzte Spalte
            current_gen->grid[i - COLS] +
            current_gen->grid[i - COLS + 1] +
            current_gen->grid[i + (COLS - 1)] + // links -> letzte Zeile, letzte Spalte
            current_gen->grid[i + 1] +
            current_gen->grid[i - (COLS * (ROWS - 2)) - 1] + // unterhalb, links -> erste Zeile, letzte Spalte
            current_gen->grid[i - (COLS * (ROWS - 1))] + // unterhalb, mittig -> erste Zeile, erste Spalte
            current_gen->grid[i - (COLS * (ROWS -1)) + 1]; // unterhalb, rechts -> erste Zeile, zweite Spalte
            
            printf(" Eckzelle unten, links\n");
            printf("i: %i->i statt oberhalb, links: %i\n", i, i - 1);
            printf("i: %i->i statt links: %i\n", i, i + COLS - 1);
            printf("i: %i->i statt unterhalb, links: %i\n", i, i - (COLS * (ROWS - 2)) - 1);
            printf("i: %i->i statt unterhalb, mittig: %i\n", i, i - (COLS * (ROWS - 1)));
            printf("i: %i->i statt unterhalb, rechts: %i\n", i, i - (COLS * (ROWS -1)) + 1);
        }

        //wenn i am oberen Rand, muss Status der untersten Zeile verwendet werden
        else if (i > 0 && i < COLS)
        {
            zellen = 
            current_gen->grid[i + (COLS * (ROWS - 1)) - 1] + // oberhalb links -> letzte Zeile, eine Spalte links
            current_gen->grid[i + (COLS * (ROWS - 1))] + // oberhalb, mittig -> letzte Zeile, selbe Spalte
            current_gen->grid[i + (COLS * (ROWS - 1)) + 1] + // oberhalb rechts -> letzte Zeile, eine Spalte rechts
            current_gen->grid[i - 1] +
            current_gen->grid[i + 1] +
            current_gen->grid[i + COLS - 1] +
            current_gen->grid[i + COLS] +
            current_gen->grid[i + COLS + 1];
            
            printf(" Randzelle oben\n");
            printf("i: %i->i statt oberhalb, links: %i\n", i, i + (COLS * (ROWS - 1)) - 1);
            printf("i: %i->i statt oberhalb: %i\n", i, i + (COLS * (ROWS - 1)));
            printf("i: %i->i statt oberhalb, rechts: %i\n", i, i + (COLS * (ROWS - 1)) + 1);
        }

        //wenn i am unteren Rand, muss Status der obersten Zeile als i+1 verwendet werden
        else if (i > (COLS * (ROWS - 1)))
        {
            zellen = 
            current_gen->grid[i + COLS - 1] +
            current_gen->grid[i + COLS] +
            current_gen->grid[i + COLS + 1] +
            current_gen->grid[i - 1] +
            current_gen->grid[i + 1] +
            current_gen->grid[i - (COLS * (ROWS - 1)) - 1] + // unterhalb, links -> erste Zeile, eine Spalte links
            current_gen->grid[i - (COLS * (ROWS - 1))] + // unterhalb, mittig -> erste Zeile, selbe Spalte
            current_gen->grid[i - (COLS * (ROWS - 1)) + 1]; // unterhalb, rechts -> erste Zeile, eine Spalte rechts
            
            printf(" Randzelle unten\n");
            printf("i: %i->i statt unterhalb, links: %i\n", i, i - (COLS * (ROWS - 1)) - 1);
            printf("i: %i->i statt unterhalb: %i\n", i, i - (COLS * (ROWS - 1)));
            printf("i: %i->i statt unterhalb, rechts: %i\n", i, i - (COLS * (ROWS - 1)) + 1);
        }

        //wenn i am linken Rand, muss Status der Zelle am rechten Rand als i-1 verwendet werden
        else if (i != 0 && i % COLS == 0)
        {
            zellen = 
            current_gen->grid[i - COLS + COLS - 1] + //linker Rand -> eine Zeile nach oben, letzte Spalte
            current_gen->grid[i - COLS] +
            current_gen->grid[i - COLS + 1] +
            current_gen->grid[i + COLS - 1] + // linker Rand -> selbe Zeile, letzte Spalte
            current_gen->grid[i + 1] +
            current_gen->grid[i + COLS + COLS - 1] + // linker Rand -> eine Zeile nach unten, letzte Spalte
            current_gen->grid[i + COLS] +
            current_gen->grid[i + COLS + 1];   
            
            printf(" Randzelle links\n");
            printf("i: %i->i statt oberhalb, links: %i\n", i, i - COLS + COLS - 1);
            printf("i: %i->i statt links: %i\n", i, i + COLS - 1);
            printf("i: %i->i statt unterhalb, links: %i\n", i, i + COLS + COLS - 1);
        }

        //wenn i am rechten Rand, muss Status der Zelle am linken Rand als i+1 verwendet werden
        else if ((i + 1) % COLS == 0)
        {
            zellen = 
            current_gen->grid[i - COLS - 1] +
            current_gen->grid[i - COLS] +
            current_gen->grid[i - COLS + 1 - (COLS)] + // rechter Rand -> eine Zeile nach oben, erste Spalte
            current_gen->grid[i - 1] +
            current_gen->grid[i + 1 - (COLS)] + // rechter Rand -> selbe Zeile, erste Spalte
            current_gen->grid[i + COLS - 1] +
            current_gen->grid[i + COLS] +
            current_gen->grid[i + COLS + 1 - (COLS)]; // rechter Rand -> eine Zeile nach unten, erste Spalte   
            
            printf(" Randzelle rechts\n");
            printf("i: %i->i statt oberhalb, rechts: %i\n", i, i - COLS + 1 - (COLS));
            printf("i: %i->i statt rechts: %i\n", i, i + 1 - (COLS));
            printf("i: %i->i statt unterhalb, rechts: %i\n", i, i + COLS + 1 - (COLS));
        }


        else
        {
            zellen = 
            current_gen->grid[i - COLS - 1] +
            current_gen->grid[i - COLS] +
            current_gen->grid[i - COLS + 1] +
            current_gen->grid[i - 1] +
            current_gen->grid[i + 1] +
            current_gen->grid[i + COLS - 1] +
            current_gen->grid[i + COLS] +
            current_gen->grid[i + COLS + 1];

            printf("i: %i->i im Feld\n", i);
        }

        if (zellen == 2 || zellen == 3)
        {
            next_gen->grid[i] = 1;
        }
        else
        {
            next_gen->grid[i] = 0;
        }
    }
}