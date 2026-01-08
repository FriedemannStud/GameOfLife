# Conway's Game of Life in einer C-Umgebung - Bonus: Interaktiver Wettbewerb um das nachhaltigste Zell-Biotop
#### Author1: Friedemann Decker / 2203777
#### Video: <URL> oder "VC"
#### Description:

##### Spielregeln
 aus Gewinnen: Strategien für math. Spiele Bd. 4, Elwyn R. Berlekamp, John H. Conway, Richard K. Guy, Vieweg, 1985, Seite 123

"LIFE" wird auf einem unendlichen Schachbrett gespielt. Ein Spiel-Zustand ist dadurch gegeben, dass man sagt, welche Quadrate oder Zellen **leben** und welche **tod** sind. Den Anfangszustand zur Zeit 0 können Sie sich aussuchen. Danach haben Sie sich freundlichst zurückzulehnen und nichts mehr zu tun, denn was dann kommt, ergibt sich in strenger Zwangsläufigkeit aus den Spielregeln:
**Geburt**: Eine zur Zeit t tote Zelle wird zum Zeitpunkt *t+1* genau dann lebendig, wenn zur Zeit *t* genau 3 von ihren  Nachbarn lebendig waren.
**Tod durch Überbevölkerung**: Eine Zelle, die zur Zeit *t* lebt, aber zugleich noch 4 oder mehr lebende Nachbarn hat, ist zur Zeit *t+1* tot.
**Tod durch Einsamkeit**: Eine Zelle, die zur Zeit *t* lebt, aber nur einen oder keinen lebendigen Nachbarn hat, ist zur Zeit *t+1* tot.

Damit haben wir alle Todesursachen abgehandelt, also lassen wir das traurige Thema und reden wir von der Regel für das
**Überleben**: Eine Zelle, die zur Zeit *t* lebt, ist auch zur Zeit *t+1* am Leben, wenn sie 2 oder 3 zur Zeit *t* lebende Nachbarn hat.
ZITAT Ende


Umsetzung der Spielidee in ein C-Programm

##### Unendliches Spielfeld
Das unendliche Spielfeld wird dadurch realisiert, dass die gegüberliegenden Ränder als Nachbarfelder modelliert werden. Dadurch entsteht quasi ein unendliches Spielfeld in Form der Oberfläche eines Torus.

##### Programmier-Konzept
Die Kern-Algorithmen (Erstellung des Spielfelds, Berechnung der Zustände, Speicherverwaltung) werden eigenständig programmiert. Die periphere Komponenten (Eingabe Startkonfiguration, Ausgabe auf Bildschirm, ...) werden mittels eines selbst entwickelten Vibe-Coding Prompts erstellt.

##### Prseudocode Kern-Algorithmen
// Einbindung benötigter Header
#include <stdio.h>
#include <stdlib.h>

// 1. Konstanten festlegen
// Eine geeignete Anzahl Zeilen/Spalten für den Betrieb wird später festgelegt.
// Für die Entwicklung wird 20/20 gewählt
#define ROWS 20  
#define COLS 20

// 2. Struktur definieren
typedef struct {
    int *grid; // Pointer auf Array
    int r;
    int c;
} World;

int main(void) {
    // 3. Dynamische Speicherverwaltung für zwei Gitter
    // Zwei Gitter, um neuen zustand berechnen zu können, ohne den aktuellen Zustand zu beeinflussen, analog zu "Bildbearbeituns-Übung"
    World *current_gen = malloc(sizeof(World));
    World *next_gen = malloc(sizeof(World));

    // Speicher für die Zellen (Arrays) reservieren
    current_gen->grid = malloc(ROWS * COLS * sizeof(int));
    next_gen->grid = malloc(ROWS * COLS * sizeof(int));

    // 4. Initialisierung
    // Zufälliges Muster lebender Zellen
    init_world(current_gen);

    // 5. Spiel-Schleife (Loop)
    while (true) {
        print_world(current_gen);
        update_generation(current_gen, next_gen);

        // Tausche die Gitter (Pointer-Zuweisung)
        World *temp = current_gen;
        current_gen = next_gen;
        next_gen = temp;

        // Abbruchbedingung oder Pause noch einfügen
    }

    // 6. Speicher freigeben (Vermeidung von Memory Leaks)
    free(current_gen->grid);
    free(next_gen->grid);
    free(current_gen);
    free(next_gen);

    return 0;
}

##### Pseudocode Biotop
LIFE bleibt immer noch ein 0-Personen-Spiel, jedoch wird der Zustand des initialen Spielfelds von zwei unterschiedlichen Personen (Teams "rot" vs "blau") festgelegt. Dann wird das Spiel gestartet. Nach einer festgelegten Anzahl Zyklen wird gezählt, von welcher Farbe mehr Zellen existieren. Gewonnen hat der Spieler mit den meisten lebenden Zellen. 
Man kann sich das das Spielfeld wie ein **Biotop** vorstellen: Die Regeln von Conway bestimmen, wo neues Leben entstehen kann, aber die Teamfarben der Nachbarn entscheiden, welche  "Lebensform" das neue Leben annimmt. So entsteht ein rundenbasierter Wettstreit auf einem mathematischen Raster.

Um die interaktive Spielidee „Rote Zellen vs Blaue Zellen“ auf Basis des vorhandenen Entwurfs für Conway's Game of Life umzusetzen, muss der Pseudocode in der **Zustandsverwaltung** und der **Geburtslogiklogik** erweitert werden. Während das klassische Modell nur „lebendig“ oder „tot“ kennt, wird hier eine Differenzierung nach Zellfarben eingeführt.

Entwurf für die Erweiterung des Pseudocodes:

1. Erweiterung der Datenstruktur und Konstanten
Die Zustände für die verschiedenen Teams werden definiert.

#define DEAD 0
#define TEAM_RED 1
#define TEAM_BLUE 2
#define MAX_ROUNDS 100 // Beispielhafte Begrenzung der Runden

In der Struktur `World` bleibt der Pointer auf das Gitter gleich, aber die darin gespeicherten Ganzzahlen (`int`) repräsentieren nun diese drei Zustände (`0`, `1` oder `2`).

2. Anpassung der Initialisierung (`init_world`)

Anstatt das Gitter zufällig zu füllen, wird eine Funktion benötigt, die es den Spielern ermöglicht, ihre **Startaufstellung** zu definieren.

• **Spieler 1 (Rot)** wählt Koordinaten für seine Zellen.

• **Spieler 2 (Blau)** wählt Koordinaten für seine Zellen.

3. Erweiterte Logik der Evolution (`update_generation`)

Die wichtigste Änderung betrifft die Regeln für Geburt und Überleben. Jede Zelle muss nicht nur die Anzahl der Nachbarn prüfen, sondern auch deren **Zugehörigkeit**.

**Logik-Erweiterung:**

**Nachbarn zählen:** Die Funktion `count_neighbors` muss nun die Anzahl der roten und blauen Nachbarn separat erfassen.

**Geburt:** Wenn eine tote Zelle genau **drei lebende Nachbarn** hat, wird sie geboren
Zusatzregel: Die neue Zelle erhält die Farbe des Teams, das die **Mehrheit** unter diesen drei Nachbarn stellt (z. B. 2 rote und 1 blaue Zelle -> die neue Zelle wird rot).

**Überleben:** Eine lebende Zelle bleibt am Leben, wenn sie **2 oder 3 Nachbarn** hat (unabhängig von deren Farbe). Sie behält dabei ihre ursprüngliche Teamfarbe bei.

**Tod:** Zellen sterben weiterhin durch Einsamkeit (<2) oder Überbevölkerung (>3).

4. Spielschleife und Siegbedingung

Die `while`-Schleife im Hauptprogramm wird um einen **Rundenzähler** und eine **Auswertung** am Ende ergänzt.

int rounds = 0;
while (rounds < MAX_ROUNDS) {
    print_world(current_gen); // Visualisierung der roten und blauen Zellen [4]
    update_generation(current_gen, next_gen);
    
    // Gitter tauschen wie gehabt
    World *temp = current_gen;
    current_gen = next_gen;
    next_gen = temp;

    rounds++;
}

// 6. Auswertung (Siegbedingung)
int red_count = count_cells_by_team(current_gen, TEAM_RED);
int blue_count = count_cells_by_team(current_gen, TEAM_BLUE);

if (red_count > blue_count) {
    // Team Rot gewinnt
} else if (blue_count > red_count) {
    // Team Blau gewinnt
}
