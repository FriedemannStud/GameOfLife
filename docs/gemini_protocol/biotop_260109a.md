## USER 🧑‍💻

This is the Gemini CLI. We are setting up the context for our chat.
Today's date is Freitag, 9. Januar 2026 (formatted according to the user's locale).
My operating system is: win32
The project's temporary directory is: C:\Users\Fried\.gemini\tmp\e28ee0484ec4cae024692ec5e73476d86df6b49984f53c9f0519d586dbfc0130
I'm currently working in the directory: C:\Users\Fried\OneDrive\Studium\Inf\INF-Einf-B\GameOfLife
Here is the folder structure of the current working directories:

Showing up to 200 items (files + folders). Folders or files indicated with ... contain more items not shown, were ignored, or the display limit (200 items) was reached.

C:\Users\Fried\OneDrive\Studium\Inf\INF-Einf-B\GameOfLife\
├───biotop.md
├───biotop1.md
├───docker-compose.yml
├───Dockerfile
├───main
├───main.c
├───project-logfile.md
├───README.md
├───.git\...
└───docs\
    ├───ADR-0000-template.md
    ├───ADR-0001-competitive-biotope-mode.md
    ├───DEV_SPEC-0000-template.md
    ├───DEV_SPEC-0001-competitive-biotope-mode.md
    ├───DEV_TASKS-0000-template.md
    ├───DEV_TASKS-0001-competitive-biotope-mode.md
    ├───DEV_TECH_DESIGN-0000-template.md
    └───DEV_TECH_DESIGN-0001-competitive-biotope-mode.md

Reminder: Do not return an empty response when a tool call is required.

My setup is complete. I will provide my first command in the next turn.

---

## USER 🧑‍💻

Erstelle ein Architecture Decision Record (ADR)

Erstelle ein Architecture Decision Record (ADR), das die grundlegende Architekturentscheidung für ein neues Feature dokumentiert. Dieses Dokument wird die Basis für alle weiteren Entwicklungsschritte bilden.

**Briefing Dokument:** Feature-Beschreibung in Kpitel 6. UI für Biotop in @README.md 

**Zu erstellende Berichte:**
1. Architecture Decision Record (ADR) für das beschriebene Feature

**Deine Aufgabe:**
1. Analysiere das beschriebene Feature und seinen Kontext sorgfältig
2. Dokumentiere den Kontext und die Problemstellung klar und ausführlich
3. Beschreibe die getroffene Architekturentscheidung mit fundierter Begründung
4. Analysiere die positiven und negativen Konsequenzen dieser Entscheidung
5. Dokumentiere alternative Lösungsansätze, die in Betracht gezogen wurden

**Lieferumfang:**
- Ein vollständiges Architecture Decision Record (ADR) gemäß @/ADR-0000-template.md
- Status des ADR (in der Regel "proposed" für neue Features)
- Aktuelles Datum

Antworte in Englisch. Speichere das Dokument unter @docs/ADR-0002-title-of-feature.md, wobei xxxx eine fortlaufende Nummer ist und "title-of-feature" den Namen des Features widerspiegelt.
--- Content from referenced files ---
Content from @README.md:
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


## Ab hier Programmierung mit VIBE-CODING
Mittlerweile existieren von verschiedenen Anbietern sog. KI-Agenten, die behaupten, auf professionellem Niveau Software-Code schreiben zu können. 
Ziel dieses Projektes ist, eine funktionierende (von Hand in C geschriebene) Simulation von Conway's Game of Life mittels der Nutzung von einem KI-Agenten zu der im Folgenden beschriebenen Anwendung zu entwickeln.

### Vorgehensweise
Im ersten Schritt wird ein sog. **Architecture Decision Record (ADR)** erstellt, der die Anforderungen an das Projekt und die Grundlagen für die Entscheidung für das Programmkonzept enthält. In einem weiteren Schritt soll der KI-Agent eine **Anforderungsanalyse und Spezifikation** erstellen, in der auf Basis des ADR die Anforderungsspezifikation, User Stories, das Product Backlog und die Definition of Done formuliert werden. Im nächsten Schritt soll der KI-Agent dann das **technische Design und eine umsetzbare technische Architektur** entwickeln und formulieren. Basierend auf diesen Dokumenten wird der KI-Agent anschließend beauftragt, eine detaillierte **Aufgabenliste** zu erstellen, die dann verwendet wird, um den KI-Agenten präzise durch die Umsetzung und Entwicklung des Programmcodes zu führen. Dem KI-Agenten werden für die vorstehend genannten Dokumente **Templates** zur Verfügung gestellt, die eine Form und inhaltliche Struktur vorgeben.

Die zur Verfügung gestellten Templates und die von dem KI-Agenten erstellten Dokumente werden in einem separaten Verzeichnis docs/ innerhalb der Codebase abgelegt.


Für die Umsetzung wird der KI-Agent von Google Gemini-CLI verwendet. Ausschlaggebend für die Wahl dieses Anbieters war ausschließlich das große kostenfreie Token-Volumen für Studierende.


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

// 5. Auswertung (Siegbedingung)
int red_count = count_cells_by_team(current_gen, TEAM_RED);
int blue_count = count_cells_by_team(current_gen, TEAM_BLUE);

if (red_count > blue_count) {
    // Team Rot gewinnt
} else if (blue_count > red_count) {
    // Team Blau gewinnt
}

// 6. UI für Biotop
Über eine GUI können die Spieler folgende Eingaben vor Spielbeginn vornehmen.
6.1 Spielfeldgröße in Zeilen x Spalten (rows, cols). Überschreibbare Standardvorgabe: 100 x 100.
6.2 Verzögerung der Anzeige einer neu berechneten Generation in Millisekunden. Überschreibbare Standardvorgabe 500 Millisekunden.
6.3 Anzahl der zur Generation 0 maximal lebenden Zellen pro Team. Es können weniger als die maximale Anzahl lebender Zellen initial eingegeben werden. Überschreibbare Standardvorgabe: 100. Es kann jedoch immer nur maximal 50% der gesamten Spielfeldgröße pro Team initial belegt werden.
6.4 Anzahl der zu spielenden Runden (turns). Überschreibbare Standardvorgabe: 1000.
6.4 Eingabe der initialen Belegung der Population (lebende Zellen) pro Team mittels Mausklick auf eine Zelle in einem auf dem Bildschirm dargestellten graphischen Gitter. Der zu belegende Bereich ist quadratisch. Diese initiale Belegung wird in einem geeigneten Datenformat auf der Festplatte abgespeichert und für den Spielablauf von dem Programm eingelesen.
6.4.1 Historische initiale Belegungen können ausgewählt werden und erneut gestartet werden.
6.4.2 Die initiale Belegung von Team Rot wird mittig in der rechten Hemisphäre des Spielfeldes platziert. Die initiale Belegung von Team Blau wird mittig in der linken Hemisphäre des Spielfeldes platziert.
6.5 Das Endergebnis in Form einer statistischen Auswertung des Spielverlaufs wird als Datei im .md-Format auf der Festplatte abgespeichert.
--- End of content ---

---

## MODEL ✨

I will read the necessary files to understand the current state of the project and the template structure.
**Tool Command**:
```json
{
  "name": "read_file",
  "args": {
    "file_path": "docs/ADR-0000-template.md"
  }
}
```**Tool Command**:
```json
{
  "name": "read_file",
  "args": {
    "file_path": "docs/ADR-0001-competitive-biotope-mode.md"
  }
}
```**Tool Command**:
```json
{
  "name": "read_file",
  "args": {
    "file_path": "main.c"
  }
}
```**Tool Command**:
```json
{
  "name": "read_file",
  "args": {
    "file_path": "Dockerfile"
  }
}
```

---

## USER 🧑‍💻

**Tool Response**:
```json
{
  "id": "read_file-1767949104888-7f64e6f1f40068",
  "name": "read_file",
  "response": {
    "output": "### **ADR-xxxx: Titel of ADR**\n\n**Status:**\n\n**Date:** JJJJ-MM-DD\n\n#### **1. Context and Problem Statement**\n\nText here ...\n\n#### **2. Decision**\n\nText here ...\n\n#### **3. Consequences of the Decision**\n\n**Positive Consequences (Advantages):**\n\nText here ...\n\n**Negative Consequences (Disadvantages):**\n\nText here ...\n\n\n\n#### **4. Alternatives Considered**\n\nText here ...\n\n"
  }
}
```**Tool Response**:
```json
{
  "id": "read_file-1767949105020-7e2a3054b2414",
  "name": "read_file",
  "response": {
    "output": "### ADR-0001: Competitive Biotope Mode (Red vs Blue)\n\n**Status:** Proposed\n\n**Date:** 2026-01-08\n\n#### 1. Context and Problem Statement\n\nThe current project implements a standard, zero-player version of Conway's Game of Life. A new requirement (\"Bonus: Interaktiver Wettbewerb um das nachhaltigste Zell-Biotop\") calls for a competitive, interactive variation of the game.\n\nIn this \"Biotope\" mode:\n- Two teams (Red and Blue) compete on the same infinite (toroidal) grid.\n- Players define the initial state.\n- Standard Conway rules apply for birth and death.\n- **New Rule:** When a new cell is born (3 neighbors), it adopts the \"team color\" of the majority of its 3 parents.\n- Winning condition is based on the count of living cells per team after a fixed number of generations.\n\nThe challenge is to integrate this logic into the existing high-performance C implementation without sacrificing efficiency or completely rewriting the core engine. We need to define how the grid state represents teams and how the evolution logic handles color inheritance.\n\n#### 2. Decision\n\nWe will extend the existing integer-based grid system to support multi-state values representing team affiliation.\n\n**Specifics:**\n1.  **Data Representation:** \n    - The `World` structure's `int *grid` will retain its format.\n    - Cell values will be defined as constants:\n        - `0`: DEAD\n        - `1`: TEAM_RED\n        - `2`: TEAM_BLUE\n    - This avoids the overhead of complex structs (`struct Cell { ... }`) and keeps memory access contiguous and cache-friendly.\n\n2.  **Core Logic Extension (`update_generation`):**\n    - The neighbor counting logic will be updated to count neighbors *per team* (e.g., `red_neighbors` and `blue_neighbors`).\n    - **Survival Logic:** Total neighbors (`red + blue`) determines survival (2 or 3). A surviving cell retains its current value (1 or 2).\n    - **Birth Logic:** If total neighbors == 3, a new cell is born. The new value is determined by comparing `red_neighbors` vs `blue_neighbors`.\n        - If `red_neighbors > blue_neighbors` -> New cell is `TEAM_RED`.\n        - Otherwise -> New cell is `TEAM_BLUE`.\n\n3.  **Visualization:**\n    - The `print_world` function will be updated to distinguish teams visually (e.g., using ANSI color codes or distinct characters like 'X' for Red and 'O' for Blue).\n\n4.  **Game Loop:**\n    - Will include a maximum turn limit (`MAX_ROUNDS`).\n    - Will implement a final tally of cell values to declare a winner.\n\n#### 3. Consequences of the Decision\n\n**Positive Consequences (Advantages):**\n*   **Performance:** Maintains the high-performance characteristics of the original C code by using primitive types and simple arithmetic. No object overhead.\n*   **Simplicity:** Reuses the existing memory management and grid traversal architecture.\n*   **Extensibility:** Easy to add more teams later (e.g., value `3` for Green) by just changing the comparison logic.\n\n**Negative Consequences (Disadvantages):**\n*   **Complexity in Neighbor Counting:** The unrolled loop for neighbor counting in `update_generation` becomes slightly more complex as we need to inspect the *value* of neighbors, not just their existence.\n*   **UI Dependency:** The console output must now reliably handle distinct representations for different teams, which might vary across terminal environments (though standard ASCII/ANSI is generally safe).\n\n#### 4. Alternatives Considered\n\n**Alternative A: Struct-based Cells**\n*   *Concept:* Change `int *grid` to `Cell *grid` where `typedef struct { bool is_alive; int team_id; } Cell;`.\n*   *Reason for Rejection:* Introduces memory alignment padding and potentially higher cache miss rates. For a grid simulation, \"Array of Structures\" (AoS) is often less performant than \"Structure of Arrays\" (SoA) or simple primitive arrays for this specific access pattern. The current `int` approach is leaner.\n\n**Alternative B: Separate Bitboards/Grids**\n*   *Concept:* Use two separate grids, one for Red positions and one for Blue.\n*   *Reason for Rejection:* Makes collision detection (ensuring a cell isn't occupied by both) and neighbor interaction logic cumbersome. A single \"Truth\" grid prevents inconsistent states where a cell is both Red and Blue.\n\n**Alternative C: Object-Oriented Approach (C++)**\n*   *Concept:* Rewrite in C++ with Cell objects.\n*   *Reason for Rejection:* Violation of the constraint to use the existing C codebase and maintaining the specific \"minimalist C\" aesthetic of the project.\n"
  }
}
```**Tool Response**:
```json
{
  "id": "read_file-1767949105066-f53077e8fd43e8",
  "name": "read_file",
  "response": {
    "output": "/*\r\n * Game of Life - Competitive Biotope Mode (Red vs Blue)\r\n * \r\n * Rules:\r\n * - Standard Conway survival rules (2 or 3 neighbors).\r\n * - Birth rule (3 neighbors): Majority color of parents determines child color.\r\n * - Teams: Red (X) vs Blue (O).\r\n * - Goal: Highest population after MAX_ROUNDS.\r\n * // KI-Agent unterstützt\r\n */\r\n\r\n#include <stdio.h>\r\n#include <stdlib.h>\r\n#include <time.h> // zur Initialisierung des Zufallszahlengenerators\r\n#include <unistd.h> // zur Verlangsamung der Ausführung mit sleep()\r\n\r\n#define DEAD 0 // KI-Agent unterstützt\r\n#define TEAM_RED 1 // KI-Agent unterstützt\r\n#define TEAM_BLUE 2 // KI-Agent unterstützt\r\n#define MAX_ROUNDS 1000 // KI-Agent unterstützt\r\n\r\n// Struktur definieren\r\ntypedef struct {\r\n    int *grid; // Pointer auf Array\r\n    int r;\r\n    int c;\r\n} World;\r\n\r\n// Prototypen\r\nvoid init_world(World *current_gen, int rows, int cols);\r\nvoid print_world(World *current_gen, int rows, int cols);\r\nvoid update_generation(World *current_gen, World *next_gen, int rows, int cols);\r\n\r\n\r\nint main(int argc, char *argv[]) {\r\n    printf(\"argc: %i\\n\", argc);\r\n    if (argc < 2)\r\n    {\r\n        printf(\"./main <rows> <columns> <delay milli-sec>\\n\");\r\n        return 1;\r\n    }\r\n    \r\n    int rows = atoi(argv[1]);\r\n    int cols = atoi(argv[2]);\r\n    \r\n    if (rows > 2000 || cols > 2000) { // KI-Agent unterstützt\r\n        printf(\"Error: Grid too large (max 2000x2000)\\n\");\r\n        return 1;\r\n    }\r\n\r\n    int delay_my = atoi(argv[3]) * 1000;\r\n    // Dynamische Speicherverwaltung für zwei Gitter\r\n    // Zwei Gitter, um neuen zustand berechnen zu können, ohne den aktuellen Zustand zu beeinflussen, analog zu \"Bildbearbeituns-Übung\"\r\n    World *current_gen = malloc(sizeof(World));\r\n    World *next_gen = malloc(sizeof(World));\r\n\r\n    // Speicher für die Zellen (Arrays) reservieren\r\n    current_gen->grid = malloc(rows * cols * sizeof(int));\r\n    next_gen->grid = malloc(rows * cols * sizeof(int));\r\n\r\n    // Initialisierung\r\n    // Zufälliges Muster lebender Zellen\r\n    init_world(current_gen, rows, cols);\r\n\r\n    // Spiel-Schleife (Loop)\r\n    int turns = 0;\r\n    while (turns < MAX_ROUNDS) // KI-Agent unterstützt\r\n     {\r\n        // Aktuelle Population ausgeben\r\n        print_world(current_gen, rows, cols);\r\n        printf(\"Turn: %i / %d\\n\", turns, MAX_ROUNDS); // KI-Agent unterstützt\r\n        usleep(delay_my);\r\n        update_generation(current_gen, next_gen, rows, cols);\r\n\r\n        // Die neu berechnete Generation wird zur aktuellen Generation (Pointer-Swap)\r\n        World *temp = current_gen;\r\n        current_gen = next_gen;\r\n        next_gen = temp;\r\n        \r\n        turns++;\r\n    }\r\n\r\n    // Game Over - Determine Winner\r\n    // KI-Agent unterstützt\r\n    int final_red = 0;\r\n    int final_blue = 0;\r\n    for (int i = 0; i < rows * cols; i++) {\r\n        if (current_gen->grid[i] == TEAM_RED) final_red++;\r\n        else if (current_gen->grid[i] == TEAM_BLUE) final_blue++;\r\n    }\r\n\r\n    printf(\"\\n--- GAME OVER ---\\n\");\r\n    printf(\"Final Score:\\nRed: %d\\nBlue: %d\\n\", final_red, final_blue);\r\n    if (final_red > final_blue) {\r\n        printf(\"Winner: RED TEAM\\n\");\r\n    } else if (final_blue > final_red) {\r\n        printf(\"Winner: BLUE TEAM\\n\");\r\n    } else {\r\n        printf(\"Result: DRAW\\n\");\r\n    }\r\n\r\n    // Speicher freigeben (Vermeidung von Memory Leaks)\r\n    free(current_gen->grid);\r\n    free(next_gen->grid);\r\n    free(current_gen);\r\n    free(next_gen);\r\n\r\n    return 0;\r\n}\r\n\r\nvoid init_world(World *current_gen, int rows, int cols)\r\n{\r\n    // Initialisiere den Zufallszahlengenerator mit der aktuellen Zeit\r\n    srand(time(NULL));\r\n    for (int i = 0; i < (rows * cols); i++)\r\n    {\r\n        // KI-Agent unterstützt\r\n        int val = rand() % 100;\r\n        if (val < 10) {\r\n            current_gen->grid[i] = TEAM_RED;\r\n        } else if (val < 20) {\r\n            current_gen->grid[i] = TEAM_BLUE;\r\n        } else {\r\n            current_gen->grid[i] = DEAD;\r\n        }\r\n    }\r\n}\r\n\r\nvoid print_world(World *current_gen, int rows, int cols)\r\n// Zunächst ohne GUI als Zeichen-Matrix 1 und 0\r\n{\r\n    system(\"clear\");\r\n    // KI-Agent unterstützt\r\n    int red_count = 0;\r\n    int blue_count = 0;\r\n\r\n    for (int i = 0; i < (rows * cols); i++)\r\n    {\r\n        if (current_gen->grid[i] == TEAM_RED) {\r\n            printf(\"X\"); // Could add ANSI colors later\r\n            red_count++;\r\n        } else if (current_gen->grid[i] == TEAM_BLUE) {\r\n            printf(\"O\");\r\n            blue_count++;\r\n        } else {\r\n            printf(\" \");\r\n        }\r\n\r\n        if ((i+1) % cols == 0)\r\n        {\r\n            printf(\"\\n\");\r\n        }\r\n    }\r\n    printf(\"\\nStats: Red: %d | Blue: %d\\n\", red_count, blue_count);\r\n}\r\n\r\nvoid update_generation(World *current_gen, World *next_gen, int rows, int cols)\r\n{\r\n    // Macro to check a neighbor index and increment counters\r\n    // Using a macro avoids function call overhead in the tight loop\r\n    // KI-Agent unterstützt\r\n    #define COUNT_NEIGHBOR(idx) \\\r\n        if (current_gen->grid[idx] == TEAM_RED) red_neighbors++; \\\r\n        else if (current_gen->grid[idx] == TEAM_BLUE) blue_neighbors++;\r\n\r\n    for (int i = 0; i < (rows * cols); i++)\r\n    {\r\n        int red_neighbors = 0;\r\n        int blue_neighbors = 0;\r\n\r\n        // --- Neighbor Counting (Unrolled for Edges) ---\r\n\r\n        // Sonderfall: i = Eckzelle oben, links\r\n        if (i == 0)\r\n        {\r\n            COUNT_NEIGHBOR(i + (cols * rows) - 1);           // oberhalb, links\r\n            COUNT_NEIGHBOR(i + (cols * (rows - 1)));         // oberhalb, mittig\r\n            COUNT_NEIGHBOR(i + (cols * (rows - 1)) + 1);     // oberhalb, rechts\r\n            COUNT_NEIGHBOR(i + cols - 1);                    // links\r\n            COUNT_NEIGHBOR(i + 1);                           // rechts\r\n            COUNT_NEIGHBOR(i + cols + cols - 1);             // unterhalb, links\r\n            COUNT_NEIGHBOR(i + cols);                        // unterhalb, mittig\r\n            COUNT_NEIGHBOR(i + cols + 1);                    // unterhalb, rechts\r\n        }\r\n        // Sonderfall: i = Eckzelle oben, rechts\r\n        else if (i == (cols - 1))\r\n        {\r\n            COUNT_NEIGHBOR(i + (cols * (rows - 1)) - 1);     // oberhalb, links\r\n            COUNT_NEIGHBOR(i + (cols * (rows - 1)));         // oberhalb, mittig\r\n            COUNT_NEIGHBOR(i + (cols * (rows - 1)) - cols + 1); // oberhalb, rechts\r\n            COUNT_NEIGHBOR(i - 1);                           // links\r\n            COUNT_NEIGHBOR(i - (cols - 1));                  // rechts\r\n            COUNT_NEIGHBOR(i + cols - 1);                    // unterhalb, links\r\n            COUNT_NEIGHBOR(i + cols);                        // unterhalb, mittig\r\n            COUNT_NEIGHBOR(i + 1);                           // unterhalb, rechts (Note: was i+1 in original)\r\n        }\r\n        // Sonderfall i = Eckzelle unten, rechts\r\n        else if (i == (cols * rows - 1))\r\n        {\r\n            COUNT_NEIGHBOR(i - cols - 1);                    // oberhalb, links\r\n            COUNT_NEIGHBOR(i - cols);                        // oberhalb, mittig\r\n            COUNT_NEIGHBOR(i - cols - (cols - 1));           // oberhalb, rechts\r\n            COUNT_NEIGHBOR(i - 1);                           // links\r\n            COUNT_NEIGHBOR(i - (cols - 1));                  // rechts\r\n            COUNT_NEIGHBOR(i - (cols * (rows - 1)) - 1);     // unterhalb, links\r\n            COUNT_NEIGHBOR(i - (cols * (rows - 1)));         // unterhalb, mittig\r\n            COUNT_NEIGHBOR(i - (cols * (rows)) + 1);         // unterhalb, rechts\r\n        }\r\n        // Sonderfall: i = Eckzelle unten, links\r\n        else if (i == (cols * (rows - 1)))\r\n        {\r\n            COUNT_NEIGHBOR(i - 1);                           // oberhalb, links\r\n            COUNT_NEIGHBOR(i - cols);                        // oberhalb, mittig\r\n            COUNT_NEIGHBOR(i - cols + 1);                    // oberhalb, rechts\r\n            COUNT_NEIGHBOR(i + (cols - 1));                  // links\r\n            COUNT_NEIGHBOR(i + 1);                           // rechts\r\n            COUNT_NEIGHBOR(i - (cols * (rows - 2)) - 1);     // unterhalb, links\r\n            COUNT_NEIGHBOR(i - (cols * (rows - 1)));         // unterhalb, mittig\r\n            COUNT_NEIGHBOR(i - (cols * (rows -1)) + 1);      // unterhalb, rechts\r\n        }\r\n        // Sonderfall: i = Zelle am oberen Rand\r\n        else if (i > 0 && i < cols)\r\n        {\r\n            COUNT_NEIGHBOR(i + (cols * (rows - 1)) - 1);\r\n            COUNT_NEIGHBOR(i + (cols * (rows - 1)));\r\n            COUNT_NEIGHBOR(i + (cols * (rows - 1)) + 1);\r\n            COUNT_NEIGHBOR(i - 1);\r\n            COUNT_NEIGHBOR(i + 1);\r\n            COUNT_NEIGHBOR(i + cols - 1);\r\n            COUNT_NEIGHBOR(i + cols);\r\n            COUNT_NEIGHBOR(i + cols + 1);\r\n        }\r\n        // Sonderfall: i = Zelle am unteren Rand\r\n        else if (i > (cols * (rows - 1)))\r\n        {\r\n            COUNT_NEIGHBOR(i + cols - 1);\r\n            COUNT_NEIGHBOR(i + cols);\r\n            COUNT_NEIGHBOR(i + cols + 1);\r\n            COUNT_NEIGHBOR(i - 1);\r\n            COUNT_NEIGHBOR(i + 1);\r\n            COUNT_NEIGHBOR(i - (cols * (rows - 1)) - 1);\r\n            COUNT_NEIGHBOR(i - (cols * (rows - 1)));\r\n            COUNT_NEIGHBOR(i - (cols * (rows - 1)) + 1);\r\n        }\r\n        // Sonderfall: i = Zelle am linken Rand\r\n        else if (i != 0 && i % cols == 0)\r\n        {\r\n            COUNT_NEIGHBOR(i - cols + cols - 1);\r\n            COUNT_NEIGHBOR(i - cols);\r\n            COUNT_NEIGHBOR(i - cols + 1);\r\n            COUNT_NEIGHBOR(i + cols - 1);\r\n            COUNT_NEIGHBOR(i + 1);\r\n            COUNT_NEIGHBOR(i + cols + cols - 1);\r\n            COUNT_NEIGHBOR(i + cols);\r\n            COUNT_NEIGHBOR(i + cols + 1);\r\n        }\r\n        // Sonderfall: i = Zelle am rechten Rand\r\n        else if ((i + 1) % cols == 0)\r\n        {\r\n            COUNT_NEIGHBOR(i - cols - 1);\r\n            COUNT_NEIGHBOR(i - cols);\r\n            COUNT_NEIGHBOR(i - cols + 1 - (cols));\r\n            COUNT_NEIGHBOR(i - 1);\r\n            COUNT_NEIGHBOR(i + 1 - (cols));\r\n            COUNT_NEIGHBOR(i + cols - 1);\r\n            COUNT_NEIGHBOR(i + cols);\r\n            COUNT_NEIGHBOR(i + cols + 1 - (cols));\r\n        }\r\n        // Zellen innerhalb Spielfeld\r\n        else\r\n        {\r\n            COUNT_NEIGHBOR(i - cols - 1);\r\n            COUNT_NEIGHBOR(i - cols);\r\n            COUNT_NEIGHBOR(i - cols + 1);\r\n            COUNT_NEIGHBOR(i - 1);\r\n            COUNT_NEIGHBOR(i + 1);\r\n            COUNT_NEIGHBOR(i + cols - 1);\r\n            COUNT_NEIGHBOR(i + cols);\r\n            COUNT_NEIGHBOR(i + cols + 1);\r\n        }\r\n\r\n        // --- Evolution Rules ---\r\n        // KI-Agent unterstützt\r\n        int total_neighbors = red_neighbors + blue_neighbors;\r\n        int current_cell = current_gen->grid[i];\r\n        \r\n        if (current_cell != DEAD) \r\n        {\r\n            // SURVIVAL: 2 or 3 neighbors -> stay alive\r\n            if (total_neighbors == 2 || total_neighbors == 3) {\r\n                next_gen->grid[i] = current_cell;\r\n            } else {\r\n                next_gen->grid[i] = DEAD;\r\n            }\r\n        }\r\n        else // Dead cell\r\n        {\r\n            // BIRTH: exactly 3 neighbors -> become alive\r\n            if (total_neighbors == 3) {\r\n                // Determine color by majority\r\n                if (red_neighbors > blue_neighbors) {\r\n                    next_gen->grid[i] = TEAM_RED;\r\n                } else {\r\n                    next_gen->grid[i] = TEAM_BLUE;\r\n                }\r\n            } else {\r\n                next_gen->grid[i] = DEAD;\r\n            }\r\n        }\r\n    }\r\n    \r\n    #undef COUNT_NEIGHBOR\r\n}"
  }
}
```**Tool Response**:
```json
{
  "id": "read_file-1767949105111-d9000ea434195",
  "name": "read_file",
  "response": {
    "output": "FROM gcc:latest\r\n\r\nWORKDIR /app\r\n\r\nCOPY . .\r\n\r\nRUN apt-get update && apt-get install -y \\\r\n    make \\\r\n    && rm -rf /var/lib/apt/lists/*\r\n\r\n# Beim Container-Start automatisch gcc ausführen\r\nCMD [\"bash\"]\r\n"
  }
}
```

---

## MODEL ✨

I will create the ADR document detailing the decision to use **Raylib** for the interactive UI, as it best meets the requirements for a graphical grid, mouse interaction, and simple configuration widgets within a C environment.
**Tool Command**:
```json
{
  "name": "write_file",
  "args": {
    "content": "# ADR-0002: Interactive UI System for Biotope Mode\n\n**Status:** Proposed\n\n**Date:** 2026-01-09\n\n## 1. Context and Problem Statement\n\nThe \"Competitive Biotope Mode\" (described in the README, Chapter 6) requires a significant expansion of the current command-line interface (CLI). The current implementation (`main.c`) runs the simulation non-interactively with random initialization. \n\n**New Requirements (from Chapter 6):**\n*   **Interactive Setup:** Players must be able to set the initial state of the grid (Red vs. Blue cells) by clicking with a mouse on a graphical grid.\n*   **Configuration:** Users need to input simulation parameters before starting:\n    *   Grid size (e.g., 100x100).\n    *   Update delay (e.g., 500ms).\n    *   Max initial population per team.\n    *   Max rounds.\n*   **Visualization:** The simulation must show the grid in real-time, distinguishing Red and Blue teams.\n*   **File I/O:** Functionality to save/load initial configurations and export statistical results (`.md` files).\n*   **Usability:** Needs a \"GUI\" with specific layout requirements (Red on right, Blue on left).\n\n**Problem:** Standard C libraries (`stdio.h`) cannot provide mouse interaction, high-resolution graphical rendering, or GUI widgets (text boxes, buttons). We need a library or framework to handle the presentation layer while maintaining the C codebase.\n\n## 2. Decision\n\nWe will implement the User Interface using the **Raylib** library, supplemented by **raygui** for UI controls.\n\n**Architecture Overview:**\n1.  **Library Choice:** **Raylib** is a lightweight, open-source C library for video game programming. It is chosen over others (like SDL2 or GTK) because it is extremely self-contained, has zero external dependencies on Windows (static linking), and includes a \"Game Loop\" architecture that fits the simulation perfectly.\n2.  **UI Widgets:** We will use **raygui**, an immediate-mode GUI module for Raylib, to implement the configuration screens (Input boxes for Rows/Cols, Sliders for Speed). This avoids writing custom button/text-input logic.\n3.  **Application States:** The application will be refactored into a state machine:\n    *   `STATE_CONFIG`: Show input fields for settings (Section 6.1 - 6.4).\n    *   `STATE_EDIT`: Display the empty grid. Allow users to place cells via Mouse Click. Enforce \"Hemisphere\" constraints (Red right, Blue left).\n    *   `STATE_RUNNING`: The main simulation loop (rendering the grid and calling the core `update_generation` logic).\n    *   `STATE_GAME_OVER`: Show statistics and save results.\n4.  **Integration:** The existing core logic (arrays, update rules) in `main.c` will be separated into a \"pure logic\" module (e.g., `simulation.c`) to be called by the Raylib application.\n\n## 3. Consequences of the Decision\n\n**Positive Consequences (Advantages):**\n*   **Rapid Development:** Raylib provides high-level functions for drawing grids (`DrawRectangle`), handling input (`IsMouseButtonPressed`), and managing windows, drastically reducing boilerplate code compared to raw SDL2 or OpenGL.\n*   **Built-in GUI:** `raygui` solves the problem of \"how to enter numbers\" (configuration) without needing a complex widget toolkit.\n*   **Performance:** Raylib is hardware-accelerated (OpenGL), ensuring smooth rendering of large grids (100x100+) which would flicker or lag in a standard terminal.\n*   **Portability:** The same C code can compile for Windows, Linux, and even Web (WASM), satisfying the cross-platform nature of the project.\n\n**Negative Consequences (Disadvantages):**\n*   **Dependency Management:** The project now requires `raylib` (and `raygui`) to be installed or included in the build process. The `Dockerfile` will need updating to support a build environment for a graphical app (though running the GUI inside Docker is complex; the primary target for the GUI is likely the host Windows machine).\n*   **Code Restructuring:** The current `main.c` mixes logic and CLI output. This must be refactored to separate the \"Model\" (Grid Data) from the \"View/Controller\" (Raylib Input/Output).\n\n## 4. Alternatives Considered\n\n**Alternative A: SDL2 (Simple DirectMedia Layer)**\n*   *Concept:* Use SDL2 for windowing and rendering.\n*   *Reason for Rejection:* SDL2 is lower-level. Implementing text input fields and buttons (for the configuration menu) requires significantly more code or third-party add-ons (like Dear ImGui). Raylib + raygui offers a more \"all-in-one\" solution for C.\n\n**Alternative B: Web-Based Interface (Backend in C)**\n*   *Concept:* Run the C simulation as a backend server and use a Browser for the UI.\n*   *Reason for Rejection:* Adds complexity (HTTP server, JSON serialization, JavaScript frontend). The requirements implies a single application (\"C-Umgebung\"), and a native Desktop App provides the most responsive \"Game Loop\" experience.\n\n**Alternative C: Console TUI (Text User Interface) with ncurses**\n*   *Concept:* Advanced terminal UI with mouse support.\n*   *Reason for Rejection:* \"Graphic grid\" requirement is hard to satisfy visually in a terminal (pixels are square, characters are rectangular). Resolution is limited. Visually less appealing for a \"Biotope\" competition.\n",
    "file_path": "docs/ADR-0002-biotope-ui-system.md"
  }
}
```

---

## USER 🧑‍💻

**Tool Response**:
```json
{
  "id": "write_file-1767949143695-22ad22135afdc",
  "name": "write_file",
  "response": {
    "output": "Successfully created and wrote to new file: C:\\Users\\Fried\\OneDrive\\Studium\\Inf\\INF-Einf-B\\GameOfLife\\docs\\ADR-0002-biotope-ui-system.md."
  }
}
```

---

## MODEL ✨

I have created the Architecture Decision Record (ADR) for the Interactive UI system.

**Summary of Action:**
- Analyzed the requirements for the "UI for Biotop" (Chapter 6 of README) and the existing project context (`main.c`, `ADR-0001`).
- Created `docs/ADR-0002-biotope-ui-system.md` adhering to the provided template.
- Documented the decision to use **Raylib** with **raygui** to satisfy the requirements for a graphical grid, mouse interaction, and configuration widgets within a C environment.

You can view the document at: `docs/ADR-0002-biotope-ui-system.md`

---

## USER 🧑‍💻

Erstelle eine umfassende Anforderungsanalyse und Spezifikation
Briefing Dokument: @docs/ADR-0002-biotope-ui-system.md 
Zu erstellende Berichte
1. Detaillierte Anforderungsspezifikation auf Basis des ADR erstellen
2. User Stories mit klaren Akzeptanzkriterien formulieren
3. Priorisierung und Abhängigkeitsanalyse der Funktionen
4. Erstellung eines Product Backlogs

**Lieferumfang:**
- Vollständige Feature-Spezifikation mit Akzeptanzkriterien
- Priorisierter Product Backlog
- Definition of Done (DoD)
 Antworte in Englisch.
 Nutze @docs/DEV_SPEC-0000-template.md als Template und speichere das Dokument in @docs/DEV_SPEC-xxxx-title-of-feature.md
--- Content from referenced files ---
Content from @docs/ADR-0002-biotope-ui-system.md:
# ADR-0002: Interactive UI System for Biotope Mode

**Status:** Proposed

**Date:** 2026-01-09

## 1. Context and Problem Statement

The "Competitive Biotope Mode" (described in the README, Chapter 6) requires a significant expansion of the current command-line interface (CLI). The current implementation (`main.c`) runs the simulation non-interactively with random initialization. 

**New Requirements (from Chapter 6):**
*   **Interactive Setup:** Players must be able to set the initial state of the grid (Red vs. Blue cells) by clicking with a mouse on a graphical grid.
*   **Configuration:** Users need to input simulation parameters before starting:
    *   Grid size (e.g., 100x100).
    *   Update delay (e.g., 500ms).
    *   Max initial population per team.
    *   Max rounds.
*   **Visualization:** The simulation must show the grid in real-time, distinguishing Red and Blue teams.
*   **File I/O:** Functionality to save/load initial configurations and export statistical results (`.md` files).
*   **Usability:** Needs a "GUI" with specific layout requirements (Red on right, Blue on left).

**Problem:** Standard C libraries (`stdio.h`) cannot provide mouse interaction, high-resolution graphical rendering, or GUI widgets (text boxes, buttons). We need a library or framework to handle the presentation layer while maintaining the C codebase.

## 2. Decision

We will implement the User Interface using the **Raylib** library, supplemented by **raygui** for UI controls.

**Architecture Overview:**
1.  **Library Choice:** **Raylib** is a lightweight, open-source C library for video game programming. It is chosen over others (like SDL2 or GTK) because it is extremely self-contained, has zero external dependencies on Windows (static linking), and includes a "Game Loop" architecture that fits the simulation perfectly.
2.  **UI Widgets:** We will use **raygui**, an immediate-mode GUI module for Raylib, to implement the configuration screens (Input boxes for Rows/Cols, Sliders for Speed). This avoids writing custom button/text-input logic.
3.  **Application States:** The application will be refactored into a state machine:
    *   `STATE_CONFIG`: Show input fields for settings (Section 6.1 - 6.4).
    *   `STATE_EDIT`: Display the empty grid. Allow users to place cells via Mouse Click. Enforce "Hemisphere" constraints (Red right, Blue left).
    *   `STATE_RUNNING`: The main simulation loop (rendering the grid and calling the core `update_generation` logic).
    *   `STATE_GAME_OVER`: Show statistics and save results.
4.  **Integration:** The existing core logic (arrays, update rules) in `main.c` will be separated into a "pure logic" module (e.g., `simulation.c`) to be called by the Raylib application.

## 3. Consequences of the Decision

**Positive Consequences (Advantages):**
*   **Rapid Development:** Raylib provides high-level functions for drawing grids (`DrawRectangle`), handling input (`IsMouseButtonPressed`), and managing windows, drastically reducing boilerplate code compared to raw SDL2 or OpenGL.
*   **Built-in GUI:** `raygui` solves the problem of "how to enter numbers" (configuration) without needing a complex widget toolkit.
*   **Performance:** Raylib is hardware-accelerated (OpenGL), ensuring smooth rendering of large grids (100x100+) which would flicker or lag in a standard terminal.
*   **Portability:** The same C code can compile for Windows, Linux, and even Web (WASM), satisfying the cross-platform nature of the project.

**Negative Consequences (Disadvantages):**
*   **Dependency Management:** The project now requires `raylib` (and `raygui`) to be installed or included in the build process. The `Dockerfile` will need updating to support a build environment for a graphical app (though running the GUI inside Docker is complex; the primary target for the GUI is likely the host Windows machine).
*   **Code Restructuring:** The current `main.c` mixes logic and CLI output. This must be refactored to separate the "Model" (Grid Data) from the "View/Controller" (Raylib Input/Output).

## 4. Alternatives Considered

**Alternative A: SDL2 (Simple DirectMedia Layer)**
*   *Concept:* Use SDL2 for windowing and rendering.
*   *Reason for Rejection:* SDL2 is lower-level. Implementing text input fields and buttons (for the configuration menu) requires significantly more code or third-party add-ons (like Dear ImGui). Raylib + raygui offers a more "all-in-one" solution for C.

**Alternative B: Web-Based Interface (Backend in C)**
*   *Concept:* Run the C simulation as a backend server and use a Browser for the UI.
*   *Reason for Rejection:* Adds complexity (HTTP server, JSON serialization, JavaScript frontend). The requirements implies a single application ("C-Umgebung"), and a native Desktop App provides the most responsive "Game Loop" experience.

**Alternative C: Console TUI (Text User Interface) with ncurses**
*   *Concept:* Advanced terminal UI with mouse support.
*   *Reason for Rejection:* "Graphic grid" requirement is hard to satisfy visually in a terminal (pixels are square, characters are rectangular). Resolution is limited. Visually less appealing for a "Biotope" competition.
Content from @docs/DEV_SPEC-0000-template.md:
# Requirements Analysis & Specification: Title here ...

This document details the requirements for ..., as described in **ADR-0015**.

---

### 1. Detailed Requirements Specification

Text here ...

---

### 2. User Stories & Acceptance Criteria

**Epic: Manage and Revisit Conversation History**

*   **User Story 1: Title ...**
    *   **As a user,** ..., **so that** ...
    *   **Acceptance Criteria:**
        *   ...
        *   ... 
        *   ... 

*   **User Story 2: Title ...**
    *   **As a user,** ... , **so that** ... 
    *   **Acceptance Criteria:**
        *   ... 
        *   ... 

*   **User Story xx: Titel**
    ...

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   ...
        *   ... 
    *   **Should-Have:**
        *   ... 
        *   ...
    *   **Could-Have:**
        *   ...
        *   ... 
        *   ... 
    *   **Won't-Have (in this increment):**
        *   ... 
        *   ... 

*   **Dependencies:**
    1.  **Topic:** ... 
    2.  **Topic:** ... 
    3.  **Topic:** ...

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |

---

### 5. Definition of Done (DoD)

A Product Backlog Item (e.g., a User Story or a Task) is considered "Done" when all of the following criteria are met:

*   **Code Quality:** The code is written and formatted according to the guidelines in `docs/CODING_STYLE.md` (`black .`, `ruff check .`).
*   **Tests:**
    *   All new backend functions are covered by unit tests.
    *   The end-to-end functionality is verified by an integration test.
    *   All existing tests continue to pass (no regressions).
*   **Acceptance Criteria:** All acceptance criteria defined for the story have been met and manually verified in the frontend.
*   **Code Review:** The code has been reviewed by at least one other team member (or is in a reviewable state in a pull request).
*   **Merge:** The code has been successfully merged into the main development branch (e.g., `main` or `develop`).
*   **Documentation:** Necessary changes to technical documentation (e.g., new ADRs, if required) have been made.
--- End of content ---

---

## MODEL ✨

I will first read the provided template and the related ADR document again to ensure I capture all necessary details for the specification.
Then, I will draft the "Requirements Analysis & Specification" document based on the `ADR-0002` and `README.md` content, focusing on the new interactive features, user stories, and acceptance criteria.
Finally, I will write the generated content to `docs/DEV_SPEC-0002-biotope-ui-system.md`.

(I have already read the file content in the prompt, so I will proceed directly to creating the file.)
**Tool Command**:
```json
{
  "name": "write_file",
  "args": {
    "file_path": "docs/DEV_SPEC-0002-biotope-ui-system.md",
    "content": "# Requirements Analysis & Specification: Biotope UI System\n\nThis document details the requirements for the \"Competitive Biotope Mode\" interactive user interface, as described in **ADR-0002**.\n\n---\n\n### 1. Detailed Requirements Specification\n\nThe system requires a graphical user interface (GUI) to facilitate the \"Biotope\" game mode, where two players (Red vs. Blue) compete on a shared Conway's Game of Life grid. The application must transition from a pure CLI-based simulation to an interactive Raylib-based application.\n\n**Functional Requirements:**\n1.  **Configuration Screen:**\n    *   Allow users to input grid dimensions (Rows, Columns). Default: 100x100.\n    *   Allow users to set the update delay (in ms). Default: 500ms.\n    *   Allow users to set the maximum initial population per team. Default: 100.\n    *   Allow users to set the maximum number of rounds. Default: 1000.\n    *   Validate inputs (e.g., max grid size, non-negative delay).\n2.  **Grid Editor (Setup Phase):**\n    *   Display an empty grid based on configured dimensions.\n    *   Divide the screen visually into two hemispheres: Left (Blue Team) and Right (Red Team).\n    *   Allow players to toggle cells by clicking.\n    *   **Constraint:** Blue cells can only be placed in the left hemisphere; Red cells only in the right.\n    *   **Constraint:** Enforce the maximum initial population limit per team.\n    *   Provide a \"Save Configuration\" button to save the current pattern to a file.\n    *   Provide a \"Load Configuration\" button to load a pattern from a file.\n    *   Provide a \"Start Game\" button.\n3.  **Simulation View (Running Phase):**\n    *   Render the grid in real-time.\n    *   Differentiate cells by color: Red (Team 1), Blue (Team 2).\n    *   Display current statistics: Round number, Red Count, Blue Count.\n    *   Handle game loop timing based on the configured delay.\n    *   Automatically stop when `MAX_ROUNDS` is reached or one team is eliminated.\n4.  **Game Over Screen:**\n    *   Display the winner and final scores.\n    *   Automatically export statistics to a Markdown file (`.md`).\n    *   Allow restarting the game or exiting.\n\n**Non-Functional Requirements:**\n*   **Performance:** The grid rendering must be smooth (60 FPS) for grids up to 200x200.\n*   **Usability:** UI controls (sliders, buttons) must be intuitive and clearly labeled.\n*   **Portability:** Code must compile on Windows (primary) and Linux.\n\n---\n\n### 2. User Stories & Acceptance Criteria\n\n**Epic: Game Configuration**\n\n*   **User Story 1: Configure Simulation Parameters**\n    *   **As a** player,\n    *   **I want to** adjust the grid size, speed, and turn limit before the game starts,\n    *   **so that** I can customize the match duration and difficulty.\n    *   **Acceptance Criteria:**\n        *   The app launches into a \"Configuration\" screen.\n        *   Input fields exist for Rows, Cols, Delay, Max Population, and Max Rounds.\n        *   Default values (100x100, 500ms, 100 pop, 1000 rounds) are pre-filled.\n        *   Clicking \"Next\" validates inputs (e.g., prevent negative numbers) and proceeds to the Grid Editor.\n\n**Epic: Interactive Setup (Grid Editor)**\n\n*   **User Story 2: Place Initial Cells**\n    *   **As a** player (Red or Blue),\n    *   **I want to** click on grid cells to set my starting population,\n    *   **so that** I can strategically position my colony.\n    *   **Acceptance Criteria:**\n        *   Clicking a dead cell toggles it to alive (and vice-versa).\n        *   Clicking on the left half creates a BLUE cell.\n        *   Clicking on the right half creates a RED cell.\n        *   Attempting to place a cell in the opponent's hemisphere is blocked (no action).\n        *   A counter shows \"Remaining Cells\" for each team. Placing a cell decreases this count.\n\n*   **User Story 3: Save/Load Setup**\n    *   **As a** player,\n    *   **I want to** save my meticulously created pattern to a file and load it later,\n    *   **so that** I can replay specific scenarios or pause setup.\n    *   **Acceptance Criteria:**\n        *   \"Save\" button writes the current grid state (positions and colors) to a file.\n        *   \"Load\" button reads a file and restores the grid state.\n        *   Loading overwrites the current grid.\n\n**Epic: Simulation & Visualization**\n\n*   **User Story 4: View Live Match**\n    *   **As a** spectator,\n    *   **I want to** see the Red and Blue cells evolve and fight in real-time,\n    *   **so that** I can follow the progress of the game.\n    *   **Acceptance Criteria:**\n        *   The grid updates automatically according to the \"Delay\" setting.\n        *   Red cells are rendered as Red squares, Blue as Blue squares.\n        *   A visible counter shows the current Round Number.\n\n**Epic: Game Over & Analysis**\n\n*   **User Story 5: Result Export**\n    *   **As a** data analyst,\n    *   **I want** the game results (winner, final counts) to be saved to a file,\n    *   **so that** I can keep a history of match outcomes.\n    *   **Acceptance Criteria:**\n        *   When the game ends, a `biotope_results_<timestamp>.md` file is created.\n        *   The file contains the date, settings used, final scores, and the winner.\n\n---\n\n### 3. Prioritization and Dependency Analysis\n\n*   **Prioritization (MoSCoW Method):**\n    *   **Must-Have (MVP):**\n        *   Refactoring core logic to be separate from CLI `main`.\n        *   Raylib window initialization.\n        *   State machine implementation (Config -> Edit -> Run -> End).\n        *   Mouse interaction for cell placement (Red/Blue split).\n        *   Visual rendering of the grid.\n        *   Winning logic and Game Over screen.\n    *   **Should-Have:**\n        *   `raygui` widgets for inputs (can use keyboard shortcuts or simple text rendering initially if GUI is complex).\n        *   File Save/Load functionality.\n        *   Detailed statistics export.\n    *   **Could-Have:**\n        *   Zoom/Pan controls for the grid (Camera2D).\n        *   Fancy particle effects for cell death/birth.\n    *   **Won't-Have (in this increment):**\n        *   Networked multiplayer (playing over LAN/Internet).\n\n*   **Dependencies:**\n    1.  **Raylib Integration:** All UI stories depend on successfully linking and running Raylib.\n    2.  **Core Refactoring:** The interactive loop depends on `update_generation` being callable from the Raylib loop, not a `while(true)` sleep loop.\n\n---\n\n### 4. Product Backlog\n\n| ID | Epic | User Story / Task | Priority |\n| :-- | :--- | :--- | :--- |\n| PB-001 | Infrastructure | **Task:** Refactor `main.c` core logic into `game_logic.c/h` (Simulation State separate from UI). | High |\n| PB-002 | UI Framework | **Task:** Set up Raylib build environment (Makefile/Task) and verify \"Hello World\" window. | High |\n| PB-003 | Configuration | **User Story 1:** Implement \"Config State\" with input handling for Rows/Cols/Delay. | Medium |\n| PB-004 | Editor | **User Story 2:** Implement \"Edit State\" with mouse clicking and hemisphere constraints. | High |\n| PB-005 | Editor | **Task:** Implement \"Max Population\" check in Edit State. | Medium |\n| PB-006 | Simulation | **User Story 4:** Implement \"Run State\" connecting the Game Loop to `update_generation`. | High |\n| PB-007 | Simulation | **Task:** Visualize Red/Blue cells with distinct colors. | High |\n| PB-008 | Data | **User Story 3:** Implement File Save/Load for initial grid patterns. | Medium |\n| PB-009 | Analysis | **User Story 5:** Implement Game Over logic and Markdown export. | Low |\n\n---\n\n### 5. Definition of Done (DoD)\n\nA Product Backlog Item is considered \"Done\" when:\n\n*   **Code Quality:** The code compiles without warnings. Naming conventions match existing C style.\n*   **Functionality:** The feature works as described in the Acceptance Criteria within the Raylib window.\n*   **Stability:** The application does not crash on invalid inputs (e.g., negative grid size) or boundary clicks.\n*   **Verification:** Verified manually by running the application on the local Windows machine.\n*   **Documentation:** Functions are commented. Changes are reflected in the project `README.md` if user-facing instructions change.\n"
  }
}
```

---

## USER 🧑‍💻

**Tool Response**:
```json
{
  "id": "write_file-1767949567921-ed58722c79f16",
  "name": "write_file",
  "response": {
    "output": "Successfully created and wrote to new file: C:\\Users\\Fried\\OneDrive\\Studium\\Inf\\INF-Einf-B\\GameOfLife\\docs\\DEV_SPEC-0002-biotope-ui-system.md."
  }
}
```

---

## MODEL ✨

I have created the Requirements Analysis & Specification document for the Biotope UI System.

**Summary of Action:**
- Created `docs/DEV_SPEC-0002-biotope-ui-system.md` based on the template and `ADR-0002`.
- Detailed the Functional and Non-Functional Requirements.
- Formulated User Stories with Acceptance Criteria for Configuration, Setup, Simulation, and Results.
- Prioritized tasks using the MoSCoW method and created a Product Backlog.
- Defined the Definition of Done (DoD).

You can view the document at: `docs/DEV_SPEC-0002-biotope-ui-system.md`

---

## USER 🧑‍💻

Erstelle das Technische Design und entwickle eine umsetzbare Architektur
  Briefing Dokument: @docs/DEV_SPEC-0002-biotope-ui-system.md 
  **Deine Aufgabe:**
  1. Detaillierte technische Spezifikation basierend auf dem DEV_SPEC erstellen
  2. Systemkomponenten und deren Interaktionen definieren
  3. Datenmodelle und API-Schnittstellen spezifizieren
  4. Sicherheits- und Performance-Aspekte berücksichtigen
  **Lieferumfang:**
  - Technische Spezifikation
  - Architekturdiagramme (Komponenten, Sequenz, etc.)
  - API-Dokumentation
  - Datenmodelle
  Berücksichtige dabei die bereits vorhandenen Strukturen und nutze Synergien, wo möglich und sinnvoll.
  Antworte in Englisch.
  Nutze @/docs/DEV_TECH_DESIGN-0000-template.md als Template und speichere das Dokument in /docs/DEV_TECH_DESIGN-xxxx-title-of-feature.md
--- Content from referenced files ---
Content from @docs/DEV_SPEC-0002-biotope-ui-system.md:
# Requirements Analysis & Specification: Biotope UI System

This document details the requirements for the "Competitive Biotope Mode" interactive user interface, as described in **ADR-0002**.

---

### 1. Detailed Requirements Specification

The system requires a graphical user interface (GUI) to facilitate the "Biotope" game mode, where two players (Red vs. Blue) compete on a shared Conway's Game of Life grid. The application must transition from a pure CLI-based simulation to an interactive Raylib-based application.

**Functional Requirements:**
1.  **Configuration Screen:**
    *   Allow users to input grid dimensions (Rows, Columns). Default: 100x100.
    *   Allow users to set the update delay (in ms). Default: 500ms.
    *   Allow users to set the maximum initial population per team. Default: 100.
    *   Allow users to set the maximum number of rounds. Default: 1000.
    *   Validate inputs (e.g., max grid size, non-negative delay).
2.  **Grid Editor (Setup Phase):**
    *   Display an empty grid based on configured dimensions.
    *   Divide the screen visually into two hemispheres: Left (Blue Team) and Right (Red Team).
    *   Allow players to toggle cells by clicking.
    *   **Constraint:** Blue cells can only be placed in the left hemisphere; Red cells only in the right.
    *   **Constraint:** Enforce the maximum initial population limit per team.
    *   Provide a "Save Configuration" button to save the current pattern to a file.
    *   Provide a "Load Configuration" button to load a pattern from a file.
    *   Provide a "Start Game" button.
3.  **Simulation View (Running Phase):**
    *   Render the grid in real-time.
    *   Differentiate cells by color: Red (Team 1), Blue (Team 2).
    *   Display current statistics: Round number, Red Count, Blue Count.
    *   Handle game loop timing based on the configured delay.
    *   Automatically stop when `MAX_ROUNDS` is reached or one team is eliminated.
4.  **Game Over Screen:**
    *   Display the winner and final scores.
    *   Automatically export statistics to a Markdown file (`.md`).
    *   Allow restarting the game or exiting.

**Non-Functional Requirements:**
*   **Performance:** The grid rendering must be smooth (60 FPS) for grids up to 200x200.
*   **Usability:** UI controls (sliders, buttons) must be intuitive and clearly labeled.
*   **Portability:** Code must compile on Windows (primary) and Linux.

---

### 2. User Stories & Acceptance Criteria

**Epic: Game Configuration**

*   **User Story 1: Configure Simulation Parameters**
    *   **As a** player,
    *   **I want to** adjust the grid size, speed, and turn limit before the game starts,
    *   **so that** I can customize the match duration and difficulty.
    *   **Acceptance Criteria:**
        *   The app launches into a "Configuration" screen.
        *   Input fields exist for Rows, Cols, Delay, Max Population, and Max Rounds.
        *   Default values (100x100, 500ms, 100 pop, 1000 rounds) are pre-filled.
        *   Clicking "Next" validates inputs (e.g., prevent negative numbers) and proceeds to the Grid Editor.

**Epic: Interactive Setup (Grid Editor)**

*   **User Story 2: Place Initial Cells**
    *   **As a** player (Red or Blue),
    *   **I want to** click on grid cells to set my starting population,
    *   **so that** I can strategically position my colony.
    *   **Acceptance Criteria:**
        *   Clicking a dead cell toggles it to alive (and vice-versa).
        *   Clicking on the left half creates a BLUE cell.
        *   Clicking on the right half creates a RED cell.
        *   Attempting to place a cell in the opponent's hemisphere is blocked (no action).
        *   A counter shows "Remaining Cells" for each team. Placing a cell decreases this count.

*   **User Story 3: Save/Load Setup**
    *   **As a** player,
    *   **I want to** save my meticulously created pattern to a file and load it later,
    *   **so that** I can replay specific scenarios or pause setup.
    *   **Acceptance Criteria:**
        *   "Save" button writes the current grid state (positions and colors) to a file.
        *   "Load" button reads a file and restores the grid state.
        *   Loading overwrites the current grid.

**Epic: Simulation & Visualization**

*   **User Story 4: View Live Match**
    *   **As a** spectator,
    *   **I want to** see the Red and Blue cells evolve and fight in real-time,
    *   **so that** I can follow the progress of the game.
    *   **Acceptance Criteria:**
        *   The grid updates automatically according to the "Delay" setting.
        *   Red cells are rendered as Red squares, Blue as Blue squares.
        *   A visible counter shows the current Round Number.

**Epic: Game Over & Analysis**

*   **User Story 5: Result Export**
    *   **As a** data analyst,
    *   **I want** the game results (winner, final counts) to be saved to a file,
    *   **so that** I can keep a history of match outcomes.
    *   **Acceptance Criteria:**
        *   When the game ends, a `biotope_results_<timestamp>.md` file is created.
        *   The file contains the date, settings used, final scores, and the winner.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Refactoring core logic to be separate from CLI `main`.
        *   Raylib window initialization.
        *   State machine implementation (Config -> Edit -> Run -> End).
        *   Mouse interaction for cell placement (Red/Blue split).
        *   Visual rendering of the grid.
        *   Winning logic and Game Over screen.
    *   **Should-Have:**
        *   `raygui` widgets for inputs (can use keyboard shortcuts or simple text rendering initially if GUI is complex).
        *   File Save/Load functionality.
        *   Detailed statistics export.
    *   **Could-Have:**
        *   Zoom/Pan controls for the grid (Camera2D).
        *   Fancy particle effects for cell death/birth.
    *   **Won't-Have (in this increment):**
        *   Networked multiplayer (playing over LAN/Internet).

*   **Dependencies:**
    1.  **Raylib Integration:** All UI stories depend on successfully linking and running Raylib.
    2.  **Core Refactoring:** The interactive loop depends on `update_generation` being callable from the Raylib loop, not a `while(true)` sleep loop.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| PB-001 | Infrastructure | **Task:** Refactor `main.c` core logic into `game_logic.c/h` (Simulation State separate from UI). | High |
| PB-002 | UI Framework | **Task:** Set up Raylib build environment (Makefile/Task) and verify "Hello World" window. | High |
| PB-003 | Configuration | **User Story 1:** Implement "Config State" with input handling for Rows/Cols/Delay. | Medium |
| PB-004 | Editor | **User Story 2:** Implement "Edit State" with mouse clicking and hemisphere constraints. | High |
| PB-005 | Editor | **Task:** Implement "Max Population" check in Edit State. | Medium |
| PB-006 | Simulation | **User Story 4:** Implement "Run State" connecting the Game Loop to `update_generation`. | High |
| PB-007 | Simulation | **Task:** Visualize Red/Blue cells with distinct colors. | High |
| PB-008 | Data | **User Story 3:** Implement File Save/Load for initial grid patterns. | Medium |
| PB-009 | Analysis | **User Story 5:** Implement Game Over logic and Markdown export. | Low |

---

### 5. Definition of Done (DoD)

A Product Backlog Item is considered "Done" when:

*   **Code Quality:** The code compiles without warnings. Naming conventions match existing C style.
*   **Functionality:** The feature works as described in the Acceptance Criteria within the Raylib window.
*   **Stability:** The application does not crash on invalid inputs (e.g., negative grid size) or boundary clicks.
*   **Verification:** Verified manually by running the application on the local Windows machine.
*   **Documentation:** Functions are commented. Changes are reflected in the project `README.md` if user-facing instructions change.
--- End of content ---

---

## MODEL ✨

I will read the provided template and review the context again to ensure the technical design is robust and aligns with the requirements.
Then, I will draft the "Technical Design Specification" document, defining the system architecture, components, data models, and API interfaces for the Biotope UI system using Raylib.
Finally, I will write the generated content to `docs/DEV_TECH_DESIGN-0002-biotope-ui-system.md`.
**Tool Command**:
```json
{
  "name": "read_file",
  "args": {
    "file_path": "docs/DEV_TECH_DESIGN-0000-template.md"
  }
}
```

---

## USER 🧑‍💻

**Tool Response**:
```json
{
  "id": "read_file-1767951376975-c02db0eb324af",
  "name": "read_file",
  "response": {
    "output": "# Technical Design: Title ...\n\n**Version:** x.x\n**Date:** JJJJ-MM-DD\n**Author:** Gemini\n**Related Documents:** [ADR-xxxx](docs/adr/xxxx-document-name.md), [DEV_SPEC-xxxx](docs/tasks/DEV_SPEC-xxxx-document-name.md)\n\n---\n\n### 1. Introduction\n\nThis document provides a detailed technical design for the \"Title ...\" feature. It translates the requirements defined in DEV_SPEC-xxxx into a concrete implementation plan, specifying the architecture, components, data models, and APIs. The goal is to create a robust, secure, and performant solution that integrates seamlessly with the existing application structure.\n\n---\n\n### 2. System Architecture and Components\n\nText here ...\n\n#### 2.1. Component Overview\n\n*   **Frontend:**\n    *   Text here ...\n    *   Text here ...\n    ...\n\n*   **Backend:**\n    *   Text here ...\n    *   Text here ...\n\n*   **Database:**\n    *   Text here ...\n\nand so on\n\n#### 2.2. Component Interaction Diagram\n\nThis diagram illustrates the high-level flow of information between the major components.\n\n```mermaid\n...\n```\n\n---\n\n### 3. Data Model Specification\n\nText here ...\n\n### 4. Backend Specification\n\n#### 4.1. API Endpoints\n\nText here ...\n\n#### 4.2. Service Layer (`conversation_service.py`)\n\nText here ...\n---\n\n### 5. Frontend Specification\n\n#### 5.1. Title ...\n\nText here ...\n\n#### 5.2. HTMTeitle ...\n\nText here ...\n\n\n#### 5.3. Sequence Diagram: Loading a Conversation\n\nmermaid\nText here ...\n\n---\n\n### 6. Security Considerations\n\nText here ...\n---\n\n### 7. Performance Considerations\n\nText here ...\r\n"
  }
}
```

---

## MODEL ✨

I will now create the Technical Design Specification document, adhering to the structure and providing detailed architectural plans, including component breakdown, data structures, and state machine logic for the Raylib implementation.
**Tool Command**:
```json
{
  "name": "write_file",
  "args": {
    "content": "# Technical Design: Biotope UI System\n\n**Version:** 1.0\n**Date:** 2026-01-09\n**Author:** Gemini\n**Related Documents:** [ADR-0002](docs/ADR-0002-biotope-ui-system.md), [DEV_SPEC-0002](docs/DEV_SPEC-0002-biotope-ui-system.md)\n\n---\n\n### 1. Introduction\n\nThis document provides a detailed technical design for the \"Biotope UI System\" feature. It translates the requirements defined in DEV_SPEC-0002 into a concrete implementation plan, specifying the architecture, components, data models, and APIs. The goal is to create a robust, secure, and performant solution that integrates seamlessly with the existing application structure, leveraging **Raylib** for the graphical interface.\n\n---\n\n### 2. System Architecture and Components\n\nThe architecture follows a modular C approach, separating the simulation logic (Model) from the visualization and user interaction (View/Controller).\n\n#### 2.1. Component Overview\n\n*   **Logic Layer (`game_logic.c/h`):**\n    *   **Responsibility:** Handles the core \"Game of Life\" rules, grid state management, and memory allocation. It is completely decoupled from Raylib, ensuring the simulation remains testable and portable.\n    *   **Key Functions:** `update_generation`, `init_world_struct`, `count_neighbors`.\n\n*   **UI Layer (`main.c` + `gui.c/h`):**\n    *   **Responsibility:** Manages the Raylib window, the main application loop, input handling, and rendering.\n    *   **State Machine:** Orchestrates transitions between Configuration, Editor, Simulation, and Game Over states.\n    *   **Modules:**\n        *   `ui_config`: Renders text boxes and sliders for settings.\n        *   `ui_editor`: Handles mouse clicks for initial cell placement.\n        *   `ui_simulation`: Renders the grid loop and handles timing.\n\n*   **Data Persistence (`file_io.c/h`):**\n    *   **Responsibility:** Handles saving/loading initial patterns and exporting game statistics to Markdown files.\n\n#### 2.2. Component Interaction Diagram\n\n```mermaid\ngraph TD\n    User((User)) -->|Input (Mouse/Keyboard)| RaylibApp[Raylib Application]\n    \n    subgraph UI_Layer\n        RaylibApp --> StateMachine[State Machine]\n        StateMachine -->|Render/Input| UI_Config[Config Screen]\n        StateMachine -->|Render/Input| UI_Editor[Editor Screen]\n        StateMachine -->|Render| UI_Sim[Simulation View]\n    end\n    \n    subgraph Logic_Layer\n        UI_Sim -->|Get Grid Data| WorldData[World Struct]\n        UI_Sim -->|Update| GameLogic[Game Logic Engine]\n        UI_Editor -->|Modify| WorldData\n        GameLogic -->|Read/Write| WorldData\n    end\n\n    subgraph Storage\n        UI_Editor -->|Save/Load| FileIO[File I/O]\n        StateMachine -->|Export Stats| FileIO\n    end\n```\n\n---\n\n### 3. Data Model Specification\n\nThe core data structures are defined in `game_logic.h` (refactored from `main.c`).\n\n#### 3.1. World Structure\nReused from `main.c` but formalized in a header.\n\n```c\n// game_logic.h\n\n#define DEAD 0\n#define TEAM_RED 1\n#define TEAM_BLUE 2\n\ntypedef struct {\n    int *grid; // Pointer to flat array: row-major order\n    int rows;\n    int cols;\n} World;\n```\n\n#### 3.2. Game Configuration Structure\nHolds the settings for the current session.\n\n```c\ntypedef struct {\n    int rows;           // Default: 100\n    int cols;           // Default: 100\n    int delay_ms;       // Default: 500\n    int max_population; // Default: 100 per team\n    int max_rounds;     // Default: 1000\n    int current_red_pop;\n    int current_blue_pop;\n} GameConfig;\n```\n\n#### 3.3. Application State Enum\n\n```c\ntypedef enum {\n    STATE_CONFIG,\n    STATE_EDIT,\n    STATE_RUNNING,\n    STATE_GAME_OVER\n} AppState;\n```\n\n---\n\n### 4. Logic Layer Specification (`game_logic.c`)\n\nThis module encapsulates the core algorithms.\n\n#### 4.1. API Functions\n\n*   `World* create_world(int rows, int cols);`\n    *   Allocates memory for a new World struct and its grid.\n*   `void free_world(World *w);`\n    *   Frees memory.\n*   `void update_generation_colored(World *current, World *next);`\n    *   Implementation of the Biotope rules (Red/Blue birth logic).\n*   `int count_team_population(World *w, int team_id);`\n    *   Helper to count cells for victory conditions/limits.\n\n---\n\n### 5. UI Layer Specification\n\n#### 5.1. Main Loop (`main.c`)\nThe `main` function initializes Raylib and enters the `while (!WindowShouldClose())` loop. It acts as the context switcher.\n\n```c\n// Pseudo-code\nvoid main() {\n    InitWindow(800, 600, \"Biotope Game of Life\");\n    AppState state = STATE_CONFIG;\n    GameConfig config = {100, 100, 500, 100, 1000};\n    \n    while (!WindowShouldClose()) {\n        BeginDrawing();\n        ClearBackground(RAYWHITE);\n        \n        switch (state) {\n            case STATE_CONFIG: draw_config_screen(&state, &config); break;\n            case STATE_EDIT:   draw_editor_screen(&state, &config, world); break;\n            case STATE_RUNNING: draw_simulation_screen(&state, &config, world); break;\n            case STATE_GAME_OVER: draw_game_over_screen(&state, &config, world); break;\n        }\n        \n        EndDrawing();\n    }\n    CloseWindow();\n}\n```\n\n#### 5.2. Editor Screen Logic\n*   **Coordinate Mapping:** Mouse `(x, y)` pixels -> Grid `(r, c)`.\n    *   `cell_size = screen_width / cols;`\n    *   `r = mouse_y / cell_size;`\n    *   `c = mouse_x / cell_size;`\n*   **Hemisphere Check:**\n    *   `mid_col = cols / 2;`\n    *   If `c < mid_col` -> Left Hemisphere (Blue).\n    *   If `c >= mid_col` -> Right Hemisphere (Red).\n*   **Population Check:**\n    *   Before setting a cell to alive, check `if (current_team_pop < max_pop)`.\n\n#### 5.3. Simulation Screen Logic\n*   **Timer:** Uses `GetTime()` from Raylib to track delta time. Only calls `update_generation_colored` when `timer >= delay_ms`.\n*   **Rendering:** Iterates `World->grid`.\n    *   `value == TEAM_RED`: `DrawRectangle(..., RED)`\n    *   `value == TEAM_BLUE`: `DrawRectangle(..., BLUE)`\n    *   `value == DEAD`: `DrawRectangleLines(...)` (Grid lines)\n\n---\n\n### 6. File I/O Specification (`file_io.c`)\n\n#### 6.1. Save/Load Format (Custom .bio)\nSimple text format to store configuration and active cells.\n\n```text\n# Header\nROWS 100\nCOLS 100\nMAX_POP 100\n\n# Data (r, c, team)\n10 5 2\n10 6 2\n50 80 1\n...\n```\n\n#### 6.2. Statistics Export (Markdown)\nWrites to `biotope_results_YYYYMMDD_HHMMSS.md`.\n\n---\n\n### 7. Performance Considerations\n\n*   **Rendering Optimization:** Drawing 10,000 rectangles (100x100) per frame in Raylib is generally fast enough (batching is handled internally). If performance drops on large grids (e.g., 500x500), we will switch to drawing a single `Image` (texture manipulation) instead of thousands of `DrawRectangle` calls.\n*   **Memory:** 1000x1000 integers is ~4MB, which is negligible.\n*   **Update Logic:** The `O(N)` update complexity is acceptable for N=10,000 to 1,000,000.\n\n---\n\n### 8. Security Considerations\n\n*   **Input Validation:** The Configuration screen must prevent allocating excessively large grids (e.g., >2000x2000) to avoid DoS (memory exhaustion) on the user's machine.\n*   **File I/O:** The file loader should be robust against malformed `.bio` files to prevent buffer overflows or crashes (standard `fscanf` safety). It only writes to the local execution directory.\n",
    "file_path": "docs/DEV_TECH_DESIGN-0002-biotope-ui-system.md"
  }
}
```

---

## USER 🧑‍💻

**Tool Response**:
```json
{
  "id": "write_file-1767951428599-5654b0957b9b78",
  "name": "write_file",
  "response": {
    "output": "Successfully created and wrote to new file: C:\\Users\\Fried\\OneDrive\\Studium\\Inf\\INF-Einf-B\\GameOfLife\\docs\\DEV_TECH_DESIGN-0002-biotope-ui-system.md."
  }
}
```

---

## MODEL ✨

I have created the Technical Design Specification for the Biotope UI System.

**Summary of Action:**
- Created `docs/DEV_TECH_DESIGN-0002-biotope-ui-system.md` based on `DEV_SPEC-0002` and the provided template.
- Defined the modular architecture: `game_logic` (Model), `main`+`gui` (View/Controller), and `file_io` (Persistence).
- Detailed the component interactions, data models (`World`, `GameConfig`, `AppState`), and API functions.
- Specified the State Machine logic for Configuration, Editor, Simulation, and Game Over states.
- Addressed Performance (batch rendering vs. texture) and Security (input validation) considerations.

You can view the document at: `docs/DEV_TECH_DESIGN-0002-biotope-ui-system.md`

---

## USER 🧑‍💻

Aufgabenliste erstellen
Erstelle einen sehr detaillierten Aufgabenplan gemäß
 @docs/ADR-0002-biotope-ui-system.md @docs/DEV_SPEC-0002-biotope-ui-system.md @docs/DEV_TECH_DESIGN-0002-biotope-ui-system.md 
, der es einem Full-Stack-Entwickler ermöglicht, den Aufgabenplan umzusetzen. Achte dabei darauf, dass Qualität vor Geschwindigkeit geht. Das heißt, formuliere lieber kleine Schritte, die präzise und verständliche Anweisungen enthalten. Baue engmaschig interaktive Funktionstest ein. Das heißt, du weist den Anwender an, Aktionen durchzuführen. Der Anwender berichtet das Ergebnis. Gliedere die Aufgabenliste so, dass auch Unterbrechungen während der Entwicklung möglich sind. Achtung! Jede durchgeführte Codeänderung muss mit "// KI-Agent unterstützt" im Code dokumentiert werden.
Schreibe den Aufgabenplan in
 @docs/DEV_TASKS-xxxx-titel-of-feature.md
und falls zielführend, führe auch das Schreiben in das DEV_TASK Dokument in Teilschritten durch. Berücksichtige @docs/CODING_STYLE.md.
Nutze @docs/DEV_TASKS-0000-template.md als Format-Template.
 Antworte in Englisch.
--- Content from referenced files ---
Content from @docs/ADR-0002-biotope-ui-system.md:
# ADR-0002: Interactive UI System for Biotope Mode

**Status:** Proposed

**Date:** 2026-01-09

## 1. Context and Problem Statement

The "Competitive Biotope Mode" (described in the README, Chapter 6) requires a significant expansion of the current command-line interface (CLI). The current implementation (`main.c`) runs the simulation non-interactively with random initialization. 

**New Requirements (from Chapter 6):**
*   **Interactive Setup:** Players must be able to set the initial state of the grid (Red vs. Blue cells) by clicking with a mouse on a graphical grid.
*   **Configuration:** Users need to input simulation parameters before starting:
    *   Grid size (e.g., 100x100).
    *   Update delay (e.g., 500ms).
    *   Max initial population per team.
    *   Max rounds.
*   **Visualization:** The simulation must show the grid in real-time, distinguishing Red and Blue teams.
*   **File I/O:** Functionality to save/load initial configurations and export statistical results (`.md` files).
*   **Usability:** Needs a "GUI" with specific layout requirements (Red on right, Blue on left).

**Problem:** Standard C libraries (`stdio.h`) cannot provide mouse interaction, high-resolution graphical rendering, or GUI widgets (text boxes, buttons). We need a library or framework to handle the presentation layer while maintaining the C codebase.

## 2. Decision

We will implement the User Interface using the **Raylib** library, supplemented by **raygui** for UI controls.

**Architecture Overview:**
1.  **Library Choice:** **Raylib** is a lightweight, open-source C library for video game programming. It is chosen over others (like SDL2 or GTK) because it is extremely self-contained, has zero external dependencies on Windows (static linking), and includes a "Game Loop" architecture that fits the simulation perfectly.
2.  **UI Widgets:** We will use **raygui**, an immediate-mode GUI module for Raylib, to implement the configuration screens (Input boxes for Rows/Cols, Sliders for Speed). This avoids writing custom button/text-input logic.
3.  **Application States:** The application will be refactored into a state machine:
    *   `STATE_CONFIG`: Show input fields for settings (Section 6.1 - 6.4).
    *   `STATE_EDIT`: Display the empty grid. Allow users to place cells via Mouse Click. Enforce "Hemisphere" constraints (Red right, Blue left).
    *   `STATE_RUNNING`: The main simulation loop (rendering the grid and calling the core `update_generation` logic).
    *   `STATE_GAME_OVER`: Show statistics and save results.
4.  **Integration:** The existing core logic (arrays, update rules) in `main.c` will be separated into a "pure logic" module (e.g., `simulation.c`) to be called by the Raylib application.

## 3. Consequences of the Decision

**Positive Consequences (Advantages):**
*   **Rapid Development:** Raylib provides high-level functions for drawing grids (`DrawRectangle`), handling input (`IsMouseButtonPressed`), and managing windows, drastically reducing boilerplate code compared to raw SDL2 or OpenGL.
*   **Built-in GUI:** `raygui` solves the problem of "how to enter numbers" (configuration) without needing a complex widget toolkit.
*   **Performance:** Raylib is hardware-accelerated (OpenGL), ensuring smooth rendering of large grids (100x100+) which would flicker or lag in a standard terminal.
*   **Portability:** The same C code can compile for Windows, Linux, and even Web (WASM), satisfying the cross-platform nature of the project.

**Negative Consequences (Disadvantages):**
*   **Dependency Management:** The project now requires `raylib` (and `raygui`) to be installed or included in the build process. The `Dockerfile` will need updating to support a build environment for a graphical app (though running the GUI inside Docker is complex; the primary target for the GUI is likely the host Windows machine).
*   **Code Restructuring:** The current `main.c` mixes logic and CLI output. This must be refactored to separate the "Model" (Grid Data) from the "View/Controller" (Raylib Input/Output).

## 4. Alternatives Considered

**Alternative A: SDL2 (Simple DirectMedia Layer)**
*   *Concept:* Use SDL2 for windowing and rendering.
*   *Reason for Rejection:* SDL2 is lower-level. Implementing text input fields and buttons (for the configuration menu) requires significantly more code or third-party add-ons (like Dear ImGui). Raylib + raygui offers a more "all-in-one" solution for C.

**Alternative B: Web-Based Interface (Backend in C)**
*   *Concept:* Run the C simulation as a backend server and use a Browser for the UI.
*   *Reason for Rejection:* Adds complexity (HTTP server, JSON serialization, JavaScript frontend). The requirements implies a single application ("C-Umgebung"), and a native Desktop App provides the most responsive "Game Loop" experience.

**Alternative C: Console TUI (Text User Interface) with ncurses**
*   *Concept:* Advanced terminal UI with mouse support.
*   *Reason for Rejection:* "Graphic grid" requirement is hard to satisfy visually in a terminal (pixels are square, characters are rectangular). Resolution is limited. Visually less appealing for a "Biotope" competition.
Content from @docs/DEV_SPEC-0002-biotope-ui-system.md:
# Requirements Analysis & Specification: Biotope UI System

This document details the requirements for the "Competitive Biotope Mode" interactive user interface, as described in **ADR-0002**.

---

### 1. Detailed Requirements Specification

The system requires a graphical user interface (GUI) to facilitate the "Biotope" game mode, where two players (Red vs. Blue) compete on a shared Conway's Game of Life grid. The application must transition from a pure CLI-based simulation to an interactive Raylib-based application.

**Functional Requirements:**
1.  **Configuration Screen:**
    *   Allow users to input grid dimensions (Rows, Columns). Default: 100x100.
    *   Allow users to set the update delay (in ms). Default: 500ms.
    *   Allow users to set the maximum initial population per team. Default: 100.
    *   Allow users to set the maximum number of rounds. Default: 1000.
    *   Validate inputs (e.g., max grid size, non-negative delay).
2.  **Grid Editor (Setup Phase):**
    *   Display an empty grid based on configured dimensions.
    *   Divide the screen visually into two hemispheres: Left (Blue Team) and Right (Red Team).
    *   Allow players to toggle cells by clicking.
    *   **Constraint:** Blue cells can only be placed in the left hemisphere; Red cells only in the right.
    *   **Constraint:** Enforce the maximum initial population limit per team.
    *   Provide a "Save Configuration" button to save the current pattern to a file.
    *   Provide a "Load Configuration" button to load a pattern from a file.
    *   Provide a "Start Game" button.
3.  **Simulation View (Running Phase):**
    *   Render the grid in real-time.
    *   Differentiate cells by color: Red (Team 1), Blue (Team 2).
    *   Display current statistics: Round number, Red Count, Blue Count.
    *   Handle game loop timing based on the configured delay.
    *   Automatically stop when `MAX_ROUNDS` is reached or one team is eliminated.
4.  **Game Over Screen:**
    *   Display the winner and final scores.
    *   Automatically export statistics to a Markdown file (`.md`).
    *   Allow restarting the game or exiting.

**Non-Functional Requirements:**
*   **Performance:** The grid rendering must be smooth (60 FPS) for grids up to 200x200.
*   **Usability:** UI controls (sliders, buttons) must be intuitive and clearly labeled.
*   **Portability:** Code must compile on Windows (primary) and Linux.

---

### 2. User Stories & Acceptance Criteria

**Epic: Game Configuration**

*   **User Story 1: Configure Simulation Parameters**
    *   **As a** player,
    *   **I want to** adjust the grid size, speed, and turn limit before the game starts,
    *   **so that** I can customize the match duration and difficulty.
    *   **Acceptance Criteria:**
        *   The app launches into a "Configuration" screen.
        *   Input fields exist for Rows, Cols, Delay, Max Population, and Max Rounds.
        *   Default values (100x100, 500ms, 100 pop, 1000 rounds) are pre-filled.
        *   Clicking "Next" validates inputs (e.g., prevent negative numbers) and proceeds to the Grid Editor.

**Epic: Interactive Setup (Grid Editor)**

*   **User Story 2: Place Initial Cells**
    *   **As a** player (Red or Blue),
    *   **I want to** click on grid cells to set my starting population,
    *   **so that** I can strategically position my colony.
    *   **Acceptance Criteria:**
        *   Clicking a dead cell toggles it to alive (and vice-versa).
        *   Clicking on the left half creates a BLUE cell.
        *   Clicking on the right half creates a RED cell.
        *   Attempting to place a cell in the opponent's hemisphere is blocked (no action).
        *   A counter shows "Remaining Cells" for each team. Placing a cell decreases this count.

*   **User Story 3: Save/Load Setup**
    *   **As a** player,
    *   **I want to** save my meticulously created pattern to a file and load it later,
    *   **so that** I can replay specific scenarios or pause setup.
    *   **Acceptance Criteria:**
        *   "Save" button writes the current grid state (positions and colors) to a file.
        *   "Load" button reads a file and restores the grid state.
        *   Loading overwrites the current grid.

**Epic: Simulation & Visualization**

*   **User Story 4: View Live Match**
    *   **As a** spectator,
    *   **I want to** see the Red and Blue cells evolve and fight in real-time,
    *   **so that** I can follow the progress of the game.
    *   **Acceptance Criteria:**
        *   The grid updates automatically according to the "Delay" setting.
        *   Red cells are rendered as Red squares, Blue as Blue squares.
        *   A visible counter shows the current Round Number.

**Epic: Game Over & Analysis**

*   **User Story 5: Result Export**
    *   **As a** data analyst,
    *   **I want** the game results (winner, final counts) to be saved to a file,
    *   **so that** I can keep a history of match outcomes.
    *   **Acceptance Criteria:**
        *   When the game ends, a `biotope_results_<timestamp>.md` file is created.
        *   The file contains the date, settings used, final scores, and the winner.

---

### 3. Prioritization and Dependency Analysis

*   **Prioritization (MoSCoW Method):**
    *   **Must-Have (MVP):**
        *   Refactoring core logic to be separate from CLI `main`.
        *   Raylib window initialization.
        *   State machine implementation (Config -> Edit -> Run -> End).
        *   Mouse interaction for cell placement (Red/Blue split).
        *   Visual rendering of the grid.
        *   Winning logic and Game Over screen.
    *   **Should-Have:**
        *   `raygui` widgets for inputs (can use keyboard shortcuts or simple text rendering initially if GUI is complex).
        *   File Save/Load functionality.
        *   Detailed statistics export.
    *   **Could-Have:**
        *   Zoom/Pan controls for the grid (Camera2D).
        *   Fancy particle effects for cell death/birth.
    *   **Won't-Have (in this increment):**
        *   Networked multiplayer (playing over LAN/Internet).

*   **Dependencies:**
    1.  **Raylib Integration:** All UI stories depend on successfully linking and running Raylib.
    2.  **Core Refactoring:** The interactive loop depends on `update_generation` being callable from the Raylib loop, not a `while(true)` sleep loop.

---

### 4. Product Backlog

| ID | Epic | User Story / Task | Priority |
| :-- | :--- | :--- | :--- |
| PB-001 | Infrastructure | **Task:** Refactor `main.c` core logic into `game_logic.c/h` (Simulation State separate from UI). | High |
| PB-002 | UI Framework | **Task:** Set up Raylib build environment (Makefile/Task) and verify "Hello World" window. | High |
| PB-003 | Configuration | **User Story 1:** Implement "Config State" with input handling for Rows/Cols/Delay. | Medium |
| PB-004 | Editor | **User Story 2:** Implement "Edit State" with mouse clicking and hemisphere constraints. | High |
| PB-005 | Editor | **Task:** Implement "Max Population" check in Edit State. | Medium |
| PB-006 | Simulation | **User Story 4:** Implement "Run State" connecting the Game Loop to `update_generation`. | High |
| PB-007 | Simulation | **Task:** Visualize Red/Blue cells with distinct colors. | High |
| PB-008 | Data | **User Story 3:** Implement File Save/Load for initial grid patterns. | Medium |
| PB-009 | Analysis | **User Story 5:** Implement Game Over logic and Markdown export. | Low |

---

### 5. Definition of Done (DoD)

A Product Backlog Item is considered "Done" when:

*   **Code Quality:** The code compiles without warnings. Naming conventions match existing C style.
*   **Functionality:** The feature works as described in the Acceptance Criteria within the Raylib window.
*   **Stability:** The application does not crash on invalid inputs (e.g., negative grid size) or boundary clicks.
*   **Verification:** Verified manually by running the application on the local Windows machine.
*   **Documentation:** Functions are commented. Changes are reflected in the project `README.md` if user-facing instructions change.
Content from @docs/DEV_TASKS-0000-template.md:
# DEV_TASKS-xxxx: Title here ... 

Text here ... 

**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each "Verification" step, report the outcome. This iterative process is crucial for maintaining quality.

**Briefing Documents:**
*   [ADR-xxxx: Title of ADR](../../docs/adr/xxxx-document-name.md)
*   [DEV_SPEC-xxxx: Title of DEV-SPEC](../../docs/tasks/DEV_SPEC-xxxx-document-name.md)
*   [DEV_TECH_DESIGN-xxxx: Technical Specification](../../docs/tasks/DEV_TECH_DESIGN-xxxx-document-name.md)

---

## Phase 1: Title of Phase 1

*Goal: Text here ...*

- [ ] **Step 1.1: Title ...**
    - [ ] **Action:** Text here ...
    - [ ] **Verification (Interactive Test):**
        1.  Text here ...
        2.  Text here ...
        3.  Text here ...
        4.  **Expected Result:** Text here ...

- [ ] **Step 1.2: Title**
    - [ ] **Action:** Text here ...
    - [ ] **Action:** Text here ...
    - [ ] **Verification:** Text here ...

- [ ] **Step 1.3: Title ...**
    - [ ] **Action:** Text here ...
    - [ ] **Action:** Text here ...
    - [ ] **Action:** Text here ...
    - [ ] **Verification (Interactive Test):**
        1.  Text here ...
        2.  Text here ...
        3.  Text here ...
        4.  Text here ...
        5.  **Expected Result:** Text here ...

- [ ] **Step x.x: Title ...**
    - [ ] **Action:** Text here ...
    ...
    - [ ] **Verification:** Text here ...


## Phase 2: Title ...

*Goal: Text here ...

- [ ] **Step 2.1: Title ...**
    - [ ] **Action:** Text here ...
    - [x] **Action:** Text here ...
      
 and so on
Content from @docs/DEV_TECH_DESIGN-0002-biotope-ui-system.md:
# Technical Design: Biotope UI System

**Version:** 1.0
**Date:** 2026-01-09
**Author:** Gemini
**Related Documents:** [ADR-0002](docs/ADR-0002-biotope-ui-system.md), [DEV_SPEC-0002](docs/DEV_SPEC-0002-biotope-ui-system.md)

---

### 1. Introduction

This document provides a detailed technical design for the "Biotope UI System" feature. It translates the requirements defined in DEV_SPEC-0002 into a concrete implementation plan, specifying the architecture, components, data models, and APIs. The goal is to create a robust, secure, and performant solution that integrates seamlessly with the existing application structure, leveraging **Raylib** for the graphical interface.

---

### 2. System Architecture and Components

The architecture follows a modular C approach, separating the simulation logic (Model) from the visualization and user interaction (View/Controller).

#### 2.1. Component Overview

*   **Logic Layer (`game_logic.c/h`):**
    *   **Responsibility:** Handles the core "Game of Life" rules, grid state management, and memory allocation. It is completely decoupled from Raylib, ensuring the simulation remains testable and portable.
    *   **Key Functions:** `update_generation`, `init_world_struct`, `count_neighbors`.

*   **UI Layer (`main.c` + `gui.c/h`):**
    *   **Responsibility:** Manages the Raylib window, the main application loop, input handling, and rendering.
    *   **State Machine:** Orchestrates transitions between Configuration, Editor, Simulation, and Game Over states.
    *   **Modules:**
        *   `ui_config`: Renders text boxes and sliders for settings.
        *   `ui_editor`: Handles mouse clicks for initial cell placement.
        *   `ui_simulation`: Renders the grid loop and handles timing.

*   **Data Persistence (`file_io.c/h`):**
    *   **Responsibility:** Handles saving/loading initial patterns and exporting game statistics to Markdown files.

#### 2.2. Component Interaction Diagram

```mermaid
graph TD
    User((User)) -->|Input (Mouse/Keyboard)| RaylibApp[Raylib Application]
    
    subgraph UI_Layer
        RaylibApp --> StateMachine[State Machine]
        StateMachine -->|Render/Input| UI_Config[Config Screen]
        StateMachine -->|Render/Input| UI_Editor[Editor Screen]
        StateMachine -->|Render| UI_Sim[Simulation View]
    end
    
    subgraph Logic_Layer
        UI_Sim -->|Get Grid Data| WorldData[World Struct]
        UI_Sim -->|Update| GameLogic[Game Logic Engine]
        UI_Editor -->|Modify| WorldData
        GameLogic -->|Read/Write| WorldData
    end

    subgraph Storage
        UI_Editor -->|Save/Load| FileIO[File I/O]
        StateMachine -->|Export Stats| FileIO
    end
```

---

### 3. Data Model Specification

The core data structures are defined in `game_logic.h` (refactored from `main.c`).

#### 3.1. World Structure
Reused from `main.c` but formalized in a header.

```c
// game_logic.h

#define DEAD 0
#define TEAM_RED 1
#define TEAM_BLUE 2

typedef struct {
    int *grid; // Pointer to flat array: row-major order
    int rows;
    int cols;
} World;
```

#### 3.2. Game Configuration Structure
Holds the settings for the current session.

```c
typedef struct {
    int rows;           // Default: 100
    int cols;           // Default: 100
    int delay_ms;       // Default: 500
    int max_population; // Default: 100 per team
    int max_rounds;     // Default: 1000
    int current_red_pop;
    int current_blue_pop;
} GameConfig;
```

#### 3.3. Application State Enum

```c
typedef enum {
    STATE_CONFIG,
    STATE_EDIT,
    STATE_RUNNING,
    STATE_GAME_OVER
} AppState;
```

---

### 4. Logic Layer Specification (`game_logic.c`)

This module encapsulates the core algorithms.

#### 4.1. API Functions

*   `World* create_world(int rows, int cols);`
    *   Allocates memory for a new World struct and its grid.
*   `void free_world(World *w);`
    *   Frees memory.
*   `void update_generation_colored(World *current, World *next);`
    *   Implementation of the Biotope rules (Red/Blue birth logic).
*   `int count_team_population(World *w, int team_id);`
    *   Helper to count cells for victory conditions/limits.

---

### 5. UI Layer Specification

#### 5.1. Main Loop (`main.c`)
The `main` function initializes Raylib and enters the `while (!WindowShouldClose())` loop. It acts as the context switcher.

```c
// Pseudo-code
void main() {
    InitWindow(800, 600, "Biotope Game of Life");
    AppState state = STATE_CONFIG;
    GameConfig config = {100, 100, 500, 100, 1000};
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        switch (state) {
            case STATE_CONFIG: draw_config_screen(&state, &config); break;
            case STATE_EDIT:   draw_editor_screen(&state, &config, world); break;
            case STATE_RUNNING: draw_simulation_screen(&state, &config, world); break;
            case STATE_GAME_OVER: draw_game_over_screen(&state, &config, world); break;
        }
        
        EndDrawing();
    }
    CloseWindow();
}
```

#### 5.2. Editor Screen Logic
*   **Coordinate Mapping:** Mouse `(x, y)` pixels -> Grid `(r, c)`.
    *   `cell_size = screen_width / cols;`
    *   `r = mouse_y / cell_size;`
    *   `c = mouse_x / cell_size;`
*   **Hemisphere Check:**
    *   `mid_col = cols / 2;`
    *   If `c < mid_col` -> Left Hemisphere (Blue).
    *   If `c >= mid_col` -> Right Hemisphere (Red).
*   **Population Check:**
    *   Before setting a cell to alive, check `if (current_team_pop < max_pop)`.

#### 5.3. Simulation Screen Logic
*   **Timer:** Uses `GetTime()` from Raylib to track delta time. Only calls `update_generation_colored` when `timer >= delay_ms`.
*   **Rendering:** Iterates `World->grid`.
    *   `value == TEAM_RED`: `DrawRectangle(..., RED)`
    *   `value == TEAM_BLUE`: `DrawRectangle(..., BLUE)`
    *   `value == DEAD`: `DrawRectangleLines(...)` (Grid lines)

---

### 6. File I/O Specification (`file_io.c`)

#### 6.1. Save/Load Format (Custom .bio)
Simple text format to store configuration and active cells.

```text
# Header
ROWS 100
COLS 100
MAX_POP 100

# Data (r, c, team)
10 5 2
10 6 2
50 80 1
...
```

#### 6.2. Statistics Export (Markdown)
Writes to `biotope_results_YYYYMMDD_HHMMSS.md`.

---

### 7. Performance Considerations

*   **Rendering Optimization:** Drawing 10,000 rectangles (100x100) per frame in Raylib is generally fast enough (batching is handled internally). If performance drops on large grids (e.g., 500x500), we will switch to drawing a single `Image` (texture manipulation) instead of thousands of `DrawRectangle` calls.
*   **Memory:** 1000x1000 integers is ~4MB, which is negligible.
*   **Update Logic:** The `O(N)` update complexity is acceptable for N=10,000 to 1,000,000.

---

### 8. Security Considerations

*   **Input Validation:** The Configuration screen must prevent allocating excessively large grids (e.g., >2000x2000) to avoid DoS (memory exhaustion) on the user's machine.
*   **File I/O:** The file loader should be robust against malformed `.bio` files to prevent buffer overflows or crashes (standard `fscanf` safety). It only writes to the local execution directory.
--- End of content ---

---

## MODEL ✨

I will carefully read the provided documents again, specifically the `DEV_SPEC` and `DEV_TECH_DESIGN`, to ensure the task list covers all functional and technical requirements.
Then, I will create the `docs/DEV_TASKS-0002-biotope-ui-system.md` file, structuring the plan into phases: "Infrastructure & Refactoring", "UI Foundation", "Interactive Features", "Simulation Loop", and "Polish & Data".
Each task will be broken down into small, verifiable steps with clear actions and interactive verification instructions for the developer.
I will ensure that the requirement to document code changes with "// KI-Agent unterstützt" is explicitly mentioned in the instructions.

(I have already read the file content in the prompt, so I will proceed directly to creating the file.)
**Tool Command**:
```json
{
  "name": "write_file",
  "args": {
    "content": "# DEV_TASKS-0002: Biotope UI System Implementation\n\nThis task list details the implementation steps for the interactive \"Biotope\" game mode using Raylib, as defined in **DEV_SPEC-0002** and **DEV_TECH_DESIGN-0002**.\n\n**Developer:** Please follow these steps precisely. The plan is broken into phases and small steps to allow for interruptions and ensure stability. After each \"Verification\" step, report the outcome. This iterative process is crucial for maintaining quality.\n\n**Important:** Every single code change (new lines or modifications) must be commented with `// KI-Agent unterstützt`.\n\n**Briefing Documents:**\n*   [ADR-0002: Interactive UI System](../../docs/ADR-0002-biotope-ui-system.md)\n*   [DEV_SPEC-0002: Requirements Specification](../../docs/DEV_SPEC-0002-biotope-ui-system.md)\n*   [DEV_TECH_DESIGN-0002: Technical Specification](../../docs/DEV_TECH_DESIGN-0002-biotope-ui-system.md)\n\n---\n\n## Phase 1: Infrastructure & Core Refactoring\n\n*Goal: Decouple the existing simulation logic from `main.c` into a reusable module (`game_logic.c`), paving the way for the GUI integration.*\n\n- [ ] **Step 1.1: Create Logic Module Headers**\n    - [ ] **Action:** Create `game_logic.h`.\n    - [ ] **Action:** Define the constants `DEAD`, `TEAM_RED`, `TEAM_BLUE` in `game_logic.h`.\n    - [ ] **Action:** Define the `World` struct in `game_logic.h` (move it from `main.c`).\n    - [ ] **Action:** Declare function prototypes: `create_world`, `free_world`, `init_world`, `update_generation`, `count_neighbors` (if needed publicly).\n    - [ ] **Verification:** Compile `main.c` (it should fail now or need adjustment if you only moved structs). Ideally, just check that the header file exists and looks correct.\n\n- [ ] **Step 1.2: Implement Logic Module Source**\n    - [ ] **Action:** Create `game_logic.c`.\n    - [ ] **Action:** Move the implementations of `init_world` and `update_generation` (and the `update_generation_colored` logic from ADR-0001) from `main.c` to `game_logic.c`.\n    - [ ] **Action:** Implement `create_world` (mallocs) and `free_world` (frees) in `game_logic.c`.\n    - [ ] **Action:** Ensure all functions are commented with `// KI-Agent unterstützt`.\n    - [ ] **Verification:** Create a temporary test file `test_logic.c` that imports `game_logic.h`, creates a world, runs one update, and frees it. Compile and run it to ensure no memory leaks or crashes.\n\n- [ ] **Step 1.3: Clean up `main.c` (CLI Legacy)**\n    - [ ] **Action:** Modify `main.c` to include `game_logic.h`.\n    - [ ] **Action:** Remove the struct definitions and helper functions that were moved.\n    - [ ] **Action:** Update `main` to use `create_world` and `free_world`.\n    - [ ] **Verification (Interactive Test):**\n        1.  Run the existing CLI build command (e.g., `gcc main.c game_logic.c -o game`).\n        2.  Execute `./game 20 20 100`.\n        3.  **Expected Result:** The CLI version should still work exactly as before (displaying the grid in the terminal).\n\n---\n\n## Phase 2: Raylib Setup & State Machine\n\n*Goal: Initialize the graphical window and establish the basic application flow (State Machine).*\n\n- [ ] **Step 2.1: Raylib Environment Setup**\n    - [ ] **Action:** Update `Makefile` (or build script) to link against `raylib`, `gdi32`, `winmm` (for Windows).\n    - [ ] **Action:** Create `gui.c` and `gui.h`.\n    - [ ] **Action:** In `gui.c`, create a function `void run_gui_app()`.\n    - [ ] **Action:** Inside `run_gui_app`, add `InitWindow(800, 600, \"Biotope\");` and a basic `while (!WindowShouldClose())` loop with `BeginDrawing(); ClearBackground(RAYWHITE); EndDrawing();`.\n    - [ ] **Verification (Interactive Test):**\n        1.  Modify `main.c` to call `run_gui_app()` instead of the CLI logic (comment out CLI parts for now).\n        2.  Compile and run.\n        3.  **Expected Result:** A blank white window titled \"Biotope\" appears.\n\n- [ ] **Step 2.2: State Machine Implementation**\n    - [ ] **Action:** Define `AppState` enum (`STATE_CONFIG`, `STATE_EDIT`, `STATE_RUNNING`, `STATE_GAME_OVER`) in `gui.h`.\n    - [ ] **Action:** Define `GameConfig` struct in `gui.h` (rows, cols, delay, etc.).\n    - [ ] **Action:** In `run_gui_app`, declare a variable `AppState state = STATE_CONFIG;`.\n    - [ ] **Action:** Implement a `switch(state)` inside the drawing loop. For now, just draw different text for each state (e.g., \"CONFIG SCREEN\", \"EDITOR\", \"RUNNING\").\n    - [ ] **Action:** Add temporary key presses to switch states (e.g., Press '1' for Config, '2' for Edit...).\n    - [ ] **Verification (Interactive Test):**\n        1.  Run the app.\n        2.  Press '1', '2', '3'.\n        3.  **Expected Result:** The text on the screen changes corresponding to the state.\n\n---\n\n## Phase 3: Configuration & Editor (Interactive Setup)\n\n*Goal: Allow users to define the grid and place initial cells.*\n\n- [ ] **Step 3.1: Configuration Screen UI**\n    - [ ] **Action:** In `STATE_CONFIG`, implement a simple UI (using `raygui` or simple `DrawText`/`CheckCollisionPointRec` if preferred) to set:\n        *   Grid Rows/Cols (Text Input or predefined buttons like \"Small\", \"Medium\", \"Large\").\n        *   Game Speed (Slider or buttons).\n    - [ ] **Action:** Add a \"START SETUP\" button.\n    - [ ] **Action:** When \"START SETUP\" is clicked:\n        *   Validate inputs.\n        *   Call `create_world` with the chosen size.\n        *   Transition to `STATE_EDIT`.\n    - [ ] **Verification (Interactive Test):**\n        1.  Run the app.\n        2.  Click \"START SETUP\".\n        3.  **Expected Result:** State changes to Editor.\n\n- [ ] **Step 3.2: Grid Rendering (Static)**\n    - [ ] **Action:** In `STATE_EDIT`, implement a loop to draw the grid lines based on `World` dimensions.\n    - [ ] **Action:** Calculate `cell_width = screen_width / cols` and `cell_height`.\n    - [ ] **Action:** Draw a vertical line in the middle to separate Red/Blue hemispheres.\n    - [ ] **Verification:** Visually confirm the grid looks correct and is split in half.\n\n- [ ] **Step 3.3: Mouse Interaction (Cell Placement)**\n    - [ ] **Action:** In `STATE_EDIT`, detect mouse clicks (`IsMouseButtonPressed(MOUSE_LEFT_BUTTON)`).\n    - [ ] **Action:** Calculate which cell `(r, c)` was clicked.\n    - [ ] **Action:** Implement Logic:\n        *   If `c < cols / 2`: Set cell to `TEAM_BLUE` (if DEAD) or `DEAD` (if BLUE). **Block** if trying to place RED.\n        *   If `c >= cols / 2`: Set cell to `TEAM_RED` (if DEAD) or `DEAD` (if RED). **Block** if trying to place BLUE.\n    - [ ] **Action:** Draw the living cells (Blue rects on left, Red rects on right).\n    - [ ] **Verification (Interactive Test):**\n        1.  Click on the left side -> Blue cell appears.\n        2.  Click on the right side -> Red cell appears.\n        3.  Try to place Blue on right -> Nothing happens.\n        4.  Click an existing cell -> It disappears.\n\n- [ ] **Step 3.4: Max Population Constraint**\n    - [ ] **Action:** Add `current_red_pop` and `current_blue_pop` counters.\n    - [ ] **Action:** Before placing a cell, check if `count < max_pop`.\n    - [ ] **Action:** Display these counters on screen (e.g., \"Blue: 5/100\").\n    - [ ] **Action:** Add a \"START GAME\" button that transitions to `STATE_RUNNING`.\n    - [ ] **Verification (Interactive Test):**\n        1.  Place cells until the limit is reached.\n        2.  Try to place one more -> Should fail.\n        3.  Click \"START GAME\".\n\n---\n\n## Phase 4: Simulation & Game Loop\n\n*Goal: Run the actual game logic and visualize the battle.*\n\n- [ ] **Step 4.1: Simulation State Logic**\n    - [ ] **Action:** In `STATE_RUNNING`, track time using `GetTime()`.\n    - [ ] **Action:** When `time_accumulator > delay`, call `update_generation_colored(current, next)` and swap pointers.\n    - [ ] **Action:** Render the grid (reuse the rendering logic from Step 3.2/3.3 but read-only).\n    - [ ] **Verification (Interactive Test):**\n        1.  Setup a simple pattern (e.g., a \"Blinker\" or simple block).\n        2.  Start Game.\n        3.  **Expected Result:** The pattern evolves automatically.\n\n- [ ] **Step 4.2: Game Over Conditions**\n    - [ ] **Action:** Track `rounds_played`.\n    - [ ] **Action:** Check victory conditions (Max rounds reached OR one team has 0 population).\n    - [ ] **Action:** If condition met, transition to `STATE_GAME_OVER`.\n    - [ ] **Verification (Interactive Test):**\n        1.  Set \"Max Rounds\" to 5.\n        2.  Start Game.\n        3.  **Expected Result:** After 5 updates, the screen changes to Game Over state.\n\n---\n\n## Phase 5: Polish, Persistence & Data\n\n*Goal: Save/Load functionality and nice-to-have features.*\n\n- [ ] **Step 5.1: File I/O (Save/Load Setup)**\n    - [ ] **Action:** Create `file_io.c`.\n    - [ ] **Action:** Implement `save_grid(filename, world)` and `load_grid(filename, world)`.\n    - [ ] **Action:** Add \"SAVE\" and \"LOAD\" buttons in `STATE_EDIT`.\n    - [ ] **Verification (Interactive Test):**\n        1.  Draw a pattern.\n        2.  Click Save.\n        3.  Clear grid (restart app).\n        4.  Click Load.\n        5.  **Expected Result:** The pattern reappears.\n\n- [ ] **Step 5.2: Game Over Screen & Export**\n    - [ ] **Action:** In `STATE_GAME_OVER`, display the Winner and Final Scores.\n    - [ ] **Action:** Implement `export_stats_md(filename, stats)` in `file_io.c`.\n    - [ ] **Action:** Call this export function automatically when entering `STATE_GAME_OVER`.\n    - [ ] **Verification:** Check the project folder for the `.md` file after a game.\n\n- [ ] **Step 5.3: Cleanup**\n    - [ ] **Action:** Ensure `free_world` is called before closing.\n    - [ ] **Action:** Code review to ensure `// KI-Agent unterstützt` is everywhere.\n    - [ ] **Verification:** Final full playthrough.\n",
    "file_path": "docs/DEV_TASKS-0002-biotope-ui-system.md"
  }
}
```

---

## USER 🧑‍💻

**Tool Response**:
```json
{
  "id": "write_file-1767951764486-afa4c2b15f2e5",
  "name": "write_file",
  "response": {
    "output": "Successfully created and wrote to new file: C:\\Users\\Fried\\OneDrive\\Studium\\Inf\\INF-Einf-B\\GameOfLife\\docs\\DEV_TASKS-0002-biotope-ui-system.md."
  }
}
```

---

## MODEL ✨

I have created the detailed Task List for the Biotope UI System.

**Summary of Action:**
- Created `docs/DEV_TASKS-0002-biotope-ui-system.md` based on `DEV_SPEC-0002` and `DEV_TECH_DESIGN-0002`.
- Structured the plan into 5 distinct phases:
    1.  **Infrastructure & Core Refactoring:** Separating logic from CLI.
    2.  **Raylib Setup & State Machine:** Basic window and app flow.
    3.  **Configuration & Editor:** Interactive setup and cell placement.
    4.  **Simulation & Game Loop:** Real-time evolution.
    5.  **Polish, Persistence & Data:** Save/Load and Stats export.
- Included precise actions and interactive verification steps for each task.
- Added the mandatory instruction to comment code changes with `// KI-Agent unterstützt`.

You can view the document at: `docs/DEV_TASKS-0002-biotope-ui-system.md`