#include <stdio.h>
#include <stdlib.h>
#include <time.h> // zur Initialisierung des Zufallszahlengenerators
#include <unistd.h> // zur Verlangsamung der Ausführung mit sleep()

//#define rows 40
// #define cols 50

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
        printf("./main_original <rows> <columns> <delay milli-sec>\n Schach: ./main_original 30 120 500 Zoom: [Strg] + [+]\n Space Battle: ./main_original 30 120 100\n Von Neumann: ./main_original 2000 2000 0 Zoom: [Strg] + [+]\n");
        return 1;
    }
    
    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);
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
    while (turns < 1000)
     {
        // Aktuelle Population ausgeben
        print_world(current_gen, rows, cols);
        printf("turns: %i\n", turns);
        usleep(delay_my);
        update_generation(current_gen, next_gen, rows, cols);

        // Die neu berechnete Generation wird zur aktuellen Generation (Pointer-Swap)
        World *temp = current_gen;
        current_gen = next_gen;
        next_gen = temp;
        
        // Abbruchbedingung oder Pause noch einfügen
        // Abbruch, wenn stabile Population erreicht,
        // d.h. wenn next_gen = current_gen, unter Berücksichtigung periodisch alternierender Muster  
        turns++;
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
        current_gen->grid[i] = rand() % 2; // Zufällige 0 oder 1
    }
}

void print_world(World *current_gen, int rows, int cols)
// Zunächst ohne GUI als Zeichen-Matrix 1 und 0
{
    system("clear");
    for (int i = 0; i < (rows * cols); i++)
    {
        printf("%c", current_gen->grid[i] + 32);
        if ((i+1) % cols == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
}

void update_generation(World *current_gen, World *next_gen, int rows, int cols)
{
    int zellen = 0;
    for (int i = 0; i < (rows * cols); i++)
    {
        //next_generation Zellenweise berechnen
        //Anzahl lebender Zellen in Nachbarschaft von i zählen
        // Sonderfall: i = Eckzelle oben, links
        if (i == 0)
        {
            zellen = 
            current_gen->grid[i + (cols * rows) - 1] + // oberhalb, links -> letzte Zeile, letzte Spalte
            current_gen->grid[i + (cols * (rows - 1))] + // oberhalb, mittig -> letzte Zeile, erste Spalte
            current_gen->grid[i + (cols * (rows - 1)) + 1] + // oberhalt, rechts -> letzte Zeile, zweite Spalte
            current_gen->grid[i + cols - 1] + // links -> erste Zeile, letzte Spalte
            current_gen->grid[i + 1] +
            current_gen->grid[i + cols + cols - 1] + // unterhalb, links -> zweite Zeile, letzte Spalte
            current_gen->grid[i + cols] +
            current_gen->grid[i + cols + 1];
            /*Debug-print
            printf(" Eckzelle oben, links\n");
            printf("i: %i->i statt oberhalb, links: %i\n", i, i + (cols * rows) - 1);
            printf("i: %i->i statt oberhalb, mittig: %i\n", i, i + (cols * (rows - 1)));
            printf("i: %i->i statt oberhalb, rechts: %i\n", i, i + (cols * (rows - 1)) + 1);
            printf("i: %i->i statt links: %i\n", i, i + cols - 1);
            printf("i: %i->i statt unterhalb, links: %i\n", i, i + cols + cols - 1);
            */
        }
        // Sonderfall: i = Eckzelle oben, rechts
        else if (i == (cols - 1))
        {
            zellen = 
            current_gen->grid[i + (cols * (rows - 1)) - 1] + // oberhalb, links -> letzte Zeile, vorletzte Spalte
            current_gen->grid[i + (cols * (rows - 1))] + // oberhalb, mittig -> letzte Zeile, letzte Spalte
            current_gen->grid[i + (cols * (rows - 1)) - cols + 1] + // oberhalb, rechts -> letzte Zeile, erste Spalte
            current_gen->grid[i - 1] +
            current_gen->grid[i - (cols - 1)] + // rechts -> erste Zeile, erste Spalte
            current_gen->grid[i + cols - 1] +
            current_gen->grid[i + cols] +
            current_gen->grid[i + 1]; // unterhalb, rechts -> zweite Zeile, erste Spalte
            /*Debug-print
            printf(" Eckzelle oben, rechts\n");
            printf("i: %i->i statt oberhalb, links: %i\n", i, i + (cols * (rows - 1)) - 1);
            printf("i: %i->i statt oberhalb, mittig: %i\n", i, i + (cols * (rows - 1)));
            printf("i: %i->i statt oberhalb, rechts: %i\n", i, i + (cols * (rows - 1)) - cols + 1);
            printf("i: %i->i statt rechts: %i\n", i, i - (cols - 1));
            printf("i: %i->i statt unterhalb, rechts: %i\n", i, i + 1);
            */
        }
        // Sonderfall i = Eckzelle unten, rechts
        else if (i == (cols * rows - 1))
        {
            zellen = 
            current_gen->grid[i - cols - 1] +
            current_gen->grid[i - cols] +
            current_gen->grid[i - cols - (cols - 1)] + // oberhalb, rechts -> vorletzte Zeile, erste Spalte
            current_gen->grid[i - 1] +
            current_gen->grid[i - (cols - 1)] + // rechts -> letzte Zeile, erste Spalte
            current_gen->grid[i - (cols * (rows - 1)) - 1] + // unterhalb, links -> erste Zeile, vorletzte Spalte
            current_gen->grid[i - (cols * (rows - 1))] + // unterhalb, mittig -> erste Zeile, letzte Spalte
            current_gen->grid[i - (cols * (rows)) + 1]; // unterhalb, rechts -> erste Zeile, erste Spalte
            /*Debug-print
            printf(" Eckzelle unten, rechts\n");
            printf("i: %i->i statt oberhalb, rechts: %i\n", i, i - cols - (cols - 1));
            printf("i: %i->i statt rechts: %i\n", i, i - (cols - 1));
            printf("i: %i->i statt unterhalb, links: %i\n", i, i - (cols * (rows - 1)) - 1);
            printf("i: %i->i statt unterhalb, mittig: %i\n", i, i - (cols * (rows - 1)));
            printf("i: %i->i statt unterhalb, rechts: %i\n", i, i - (cols * (rows)) + 1);
            */
        }
        // Sonderfall: i = Eckzelle unten, links
        else if (i == (cols * (rows - 1)))
        {
            zellen = 
            current_gen->grid[i - 1] + // oberhalb, links -> vorletzte Zeile, letzte Spalte
            current_gen->grid[i - cols] +
            current_gen->grid[i - cols + 1] +
            current_gen->grid[i + (cols - 1)] + // links -> letzte Zeile, letzte Spalte
            current_gen->grid[i + 1] +
            current_gen->grid[i - (cols * (rows - 2)) - 1] + // unterhalb, links -> erste Zeile, letzte Spalte
            current_gen->grid[i - (cols * (rows - 1))] + // unterhalb, mittig -> erste Zeile, erste Spalte
            current_gen->grid[i - (cols * (rows -1)) + 1]; // unterhalb, rechts -> erste Zeile, zweite Spalte
            /*Debug-print
            printf(" Eckzelle unten, links\n");
            printf("i: %i->i statt oberhalb, links: %i\n", i, i - 1);
            printf("i: %i->i statt links: %i\n", i, i + cols - 1);
            printf("i: %i->i statt unterhalb, links: %i\n", i, i - (cols * (rows - 2)) - 1);
            printf("i: %i->i statt unterhalb, mittig: %i\n", i, i - (cols * (rows - 1)));
            printf("i: %i->i statt unterhalb, rechts: %i\n", i, i - (cols * (rows -1)) + 1);
            */
        }
        // Sonderfall: i = Zelle am oberen Rand
        else if (i > 0 && i < cols)
        {
            zellen = 
            current_gen->grid[i + (cols * (rows - 1)) - 1] + // oberhalb links -> letzte Zeile, eine Spalte links
            current_gen->grid[i + (cols * (rows - 1))] + // oberhalb, mittig -> letzte Zeile, selbe Spalte
            current_gen->grid[i + (cols * (rows - 1)) + 1] + // oberhalb rechts -> letzte Zeile, eine Spalte rechts
            current_gen->grid[i - 1] +
            current_gen->grid[i + 1] +
            current_gen->grid[i + cols - 1] +
            current_gen->grid[i + cols] +
            current_gen->grid[i + cols + 1];
            /*Debug-print
            printf(" Randzelle oben\n");
            printf("i: %i->i statt oberhalb, links: %i\n", i, i + (cols * (rows - 1)) - 1);
            printf("i: %i->i statt oberhalb: %i\n", i, i + (cols * (rows - 1)));
            printf("i: %i->i statt oberhalb, rechts: %i\n", i, i + (cols * (rows - 1)) + 1);
            */
        }
        // Sonderfall: i = Zelle am unteren Rand
        else if (i > (cols * (rows - 1)))
        {
            zellen = 
            current_gen->grid[i + cols - 1] +
            current_gen->grid[i + cols] +
            current_gen->grid[i + cols + 1] +
            current_gen->grid[i - 1] +
            current_gen->grid[i + 1] +
            current_gen->grid[i - (cols * (rows - 1)) - 1] + // unterhalb, links -> erste Zeile, eine Spalte links
            current_gen->grid[i - (cols * (rows - 1))] + // unterhalb, mittig -> erste Zeile, selbe Spalte
            current_gen->grid[i - (cols * (rows - 1)) + 1]; // unterhalb, rechts -> erste Zeile, eine Spalte rechts
            /*Debug-print
            printf(" Randzelle unten\n");
            printf("i: %i->i statt unterhalb, links: %i\n", i, i - (cols * (rows - 1)) - 1);
            printf("i: %i->i statt unterhalb: %i\n", i, i - (cols * (rows - 1)));
            printf("i: %i->i statt unterhalb, rechts: %i\n", i, i - (cols * (rows - 1)) + 1);
            */
        }
        // Sonderfall: i = Zelle am linken Rand
        else if (i != 0 && i % cols == 0)
        {
            zellen = 
            current_gen->grid[i - cols + cols - 1] + //linker Rand -> eine Zeile nach oben, letzte Spalte
            current_gen->grid[i - cols] +
            current_gen->grid[i - cols + 1] +
            current_gen->grid[i + cols - 1] + // linker Rand -> selbe Zeile, letzte Spalte
            current_gen->grid[i + 1] +
            current_gen->grid[i + cols + cols - 1] + // linker Rand -> eine Zeile nach unten, letzte Spalte
            current_gen->grid[i + cols] +
            current_gen->grid[i + cols + 1];   
            /*Debug-print
            printf(" Randzelle links\n");
            printf("i: %i->i statt oberhalb, links: %i\n", i, i - cols + cols - 1);
            printf("i: %i->i statt links: %i\n", i, i + cols - 1);
            printf("i: %i->i statt unterhalb, links: %i\n", i, i + cols + cols - 1);
            */
        }
        // Sonderfall: i = Zelle am rechten Rand
        else if ((i + 1) % cols == 0)
        {
            zellen = 
            current_gen->grid[i - cols - 1] +
            current_gen->grid[i - cols] +
            current_gen->grid[i - cols + 1 - (cols)] + // rechter Rand -> eine Zeile nach oben, erste Spalte
            current_gen->grid[i - 1] +
            current_gen->grid[i + 1 - (cols)] + // rechter Rand -> selbe Zeile, erste Spalte
            current_gen->grid[i + cols - 1] +
            current_gen->grid[i + cols] +
            current_gen->grid[i + cols + 1 - (cols)]; // rechter Rand -> eine Zeile nach unten, erste Spalte   
            /*Debug-print
            printf(" Randzelle rechts\n");
            printf("i: %i->i statt oberhalb, rechts: %i\n", i, i - cols + 1 - (cols));
            printf("i: %i->i statt rechts: %i\n", i, i + 1 - (cols));
            printf("i: %i->i statt unterhalb, rechts: %i\n", i, i + cols + 1 - (cols));
            */
        }
        // Zellen innerhalb Spielfeld
        else
        {
            zellen = 
            current_gen->grid[i - cols - 1] +
            current_gen->grid[i - cols] +
            current_gen->grid[i - cols + 1] +
            current_gen->grid[i - 1] +
            current_gen->grid[i + 1] +
            current_gen->grid[i + cols - 1] +
            current_gen->grid[i + cols] +
            current_gen->grid[i + cols + 1];
            /*Debug-print
            printf("i: %i->i im Feld\n", i);
            */
        }
        // 
        if (current_gen->grid[i] == 1 && zellen == 2 || zellen == 3)
        {
            next_gen->grid[i] = 1;
        }
        else
        {
            next_gen->grid[i] = 0;
        }
    }
}