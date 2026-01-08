#include <stdio.h>
#include <stdlib.h>
#include <time.h> // zur Initialisierung des Zufallszahlengenerators

#define ROWS 5  
#define COLS 5

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
            current_gen->grid[i + (COLS * ROWS) - 1] + // oberhalb, links
            current_gen->grid[i + (COLS * (ROWS - 1))] + // oberhalb, mittig
            current_gen->grid[i + (COLS * (ROWS - 1)) + 1] + // oberhalt, rechts
            current_gen->grid[i + COLS - 1] + // links
            current_gen->grid[i + 1] +
            current_gen->grid[i + COLS + COLS - 1] + // unterhalb, links
            current_gen->grid[i + COLS] +
            current_gen->grid[i + COLS + 1];
            
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
            current_gen->grid[i + (COLS * (ROWS - 1)) - 1] + // oberhalb, links
            current_gen->grid[i + (COLS * (ROWS - 1))] + // oberhalb, mittig
            current_gen->grid[i + (COLS * (ROWS - 1)) - COLS + 1] + // oberhalb, rechts
            current_gen->grid[i - 1] +
            current_gen->grid[i - (COLS - 1)] + // rechts
            current_gen->grid[i + COLS - 1] +
            current_gen->grid[i + COLS] +
            current_gen->grid[i + 1]; // unterhalb, rechts
            
            printf("i: %i->i statt oberhalb, links: %i\n", i, i + (COLS * (ROWS - 1)) - 1);
            printf("i: %i->i statt oberhalb, mittig: %i\n", i, i + (COLS * (ROWS - 1)));
            printf("i: %i->i statt oberhalb, rechts: %i\n", i, i + (COLS * (ROWS - 1)) - COLS + 1);
            printf("i: %i->i statt rechts: %i\n", i, i - (COLS - 1));
            printf("i: %i->i statt unterhalb, rechts: %i\n", i, i + 1);
        }

        //wenn i am oberen Rand, muss Status der untersten Zeile als i-1 verwendet werden
        else if (i > 0 && i < COLS)
        {
            zellen = 
            // Index der Zellen der untersten Zeile berechnen
            current_gen->grid[i + (COLS * (ROWS - 1)) - 1] +
            current_gen->grid[i + (COLS * (ROWS - 1))] +
            current_gen->grid[i + (COLS * (ROWS - 1)) + 1] +
            current_gen->grid[i - 1] +
            current_gen->grid[i + 1] +
            current_gen->grid[i + COLS - 1] +
            current_gen->grid[i + COLS] +
            current_gen->grid[i + COLS + 1];
            
            printf("i: %i->i von letzter Reihe: %i\n", i, i+(COLS * (ROWS - 1)));
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
            // Index der Zellen der obersten Zeile berechnen
            current_gen->grid[i - (COLS * (ROWS - 1)) - 1] +
            current_gen->grid[i - (COLS * (ROWS - 1))] +
            current_gen->grid[i - (COLS * (ROWS - 1)) + 1];   
            
            printf("i: %i->i von erster Reihe: %i\n", i, i-(COLS * (ROWS - 1)));
        }

        //wenn i am linken Rand, muss Status der Zelle am rechten Rand als i-1 verwendet werden
        else if (i != 0 && i % COLS == 0)
        {
            zellen = 
            current_gen->grid[i - COLS + COLS - 1] + //linker Rand
            current_gen->grid[i - COLS] +
            current_gen->grid[i - COLS + 1] +
            current_gen->grid[i + COLS - 1] + // linker Rand
            current_gen->grid[i + 1] +
            current_gen->grid[i + COLS + COLS - 1] + // linker Rand
            current_gen->grid[i + COLS] +
            current_gen->grid[i + COLS + 1];   
            
            printf("i: %i->i von rechtem Rand (oben): %i\n", i, i - COLS + COLS - 1);
            printf("i: %i->i von rechtem Rand (mitte): %i\n", i, i + COLS - 1);
            printf("i: %i->i von rechtem Rand (unten): %i\n", i, i + COLS + COLS - 1);
        }

        //wenn i am rechten Rand, muss Status der Zelle am linken Rand als i+1 verwendet werden
        else if ((i + 1) % COLS == 0)
        {
            zellen = 
            current_gen->grid[i - COLS - 1] +
            current_gen->grid[i - COLS] +
            current_gen->grid[i - COLS + 1 - (COLS)] + // rechter Rand
            current_gen->grid[i - 1] +
            current_gen->grid[i + 1 - (COLS)] + // rechter Rand
            current_gen->grid[i + COLS - 1] +
            current_gen->grid[i + COLS] +
            current_gen->grid[i + COLS + 1 - (COLS)]; // rechter Rand   
            
            printf("i: %i->i von linkem Rand (oben): %i\n", i, i - COLS + 1 - (COLS));
            printf("i: %i->i von linkem Rand (mitte): %i\n", i, i + 1 - (COLS));
            printf("i: %i->i von linkem Rand (unten): %i\n", i, i + COLS + 1 - (COLS));
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