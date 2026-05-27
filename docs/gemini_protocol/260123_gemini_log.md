## USER 🧑‍💻

This is the Gemini CLI. We are setting up the context for our chat.
Today's date is Freitag, 23. Januar 2026 (formatted according to the user's locale).
My operating system is: win32
The project's temporary directory is: C:\Users\Fried\.gemini\tmp\e28ee0484ec4cae024692ec5e73476d86df6b49984f53c9f0519d586dbfc0130
I'm currently working in the directory: C:\Users\Fried\OneDrive\Studium\Inf\INF-Einf-B\GameOfLife
Here is the folder structure of the current working directories:

Showing up to 200 items (files + folders). Folders or files indicated with ... contain more items not shown, were ignored, or the display limit (200 items) was reached.

C:\Users\Fried\OneDrive\Studium\Inf\INF-Einf-B\GameOfLife\
├───.gitignore
├───config.xlaunch
├───docker-compose.yml
├───Dockerfile
├───file_io.c
├───file_io.h
├───game_logic.c
├───game_logic.h
├───gemini.md
├───generate_setup_2von3.py
├───generate_setup.py
├───gui.c
├───gui.h
├───main.c
├───project-logfile.md
├───README.md
├───setup_bio_readme.md
├───setup.bio
├───test_logic.c
├───.git\...
├───biotope_results\
├───docs\
│   ├───ADR-0000-template.md
│   ├───ADR-0001-competitive-biotope-mode.md
│   ├───ADR-0002-biotope-ui-system.md
│   ├───ADR-0003-integrated-simulation-protocol.md
│   ├───DEV_SPEC-0000-template.md
│   ├───DEV_SPEC-0001-competitive-biotope-mode.md
│   ├───DEV_SPEC-0002-biotope-ui-system.md
│   ├───DEV_SPEC-0003-integrated-simulation-protocol.md
│   ├───DEV_TASKS-0000-template.md
│   ├───DEV_TASKS-0001-competitive-biotope-mode.md
│   ├───DEV_TASKS-0002-biotope-ui-system.md
│   ├───DEV_TASKS-0003-integrated-simulation-protocol.md
│   ├───DEV_TECH_DESIGN-0000-template.md
│   ├───DEV_TECH_DESIGN-0001-competitive-biotope-mode.md
│   ├───DEV_TECH_DESIGN-0002-biotope-ui-system.md
│   ├───DEV_TECH_DESIGN-0003-integrated-simulation-protocol.md
│   ├───PERFORMANCE_REPORT_UI.md
│   └───gemini_protocol\
│       ├───biotop_260108.md
│       ├───biotop_260109.md
│       ├───biotop_260109a.md
│       ├───biotop_260109b.json
│       ├───biotop_260109b.md
│       ├───biotop_260109c.json
│       ├───biotop_260109c.md
│       ├───biotop_260117.json
│       └───biotop_260117.md
└───utilities\

Reminder: Do not return an empty response when a tool call is required.

My setup is complete. I will provide my first command in the next turn.

---

## USER 🧑‍💻

@game_logic.c Bitte erkläre     #define COUNT_NEIGHBOR(idx) \
        if (current_gen->grid[idx] == TEAM_RED) red_neighbors++; \
        else if (current_gen->grid[idx] == TEAM_BLUE) blue_neighbors++;
--- Content from referenced files ---
Content from @game_logic.c:
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
--- End of content ---

---

## MODEL ✨

Dieser Code-Ausschnitt definiert ein Makro für den C-Präprozessor, das als Hilfsfunktion zur Nachbarschaftsanalyse in einer "Game of Life"-Simulation dient.

**1. Was macht der Code?**
Das Makro `COUNT_NEIGHBOR` prüft den Zustand einer Zelle an einer bestimmten Position (`idx`) im Spielfeld. Wenn die Zelle zum roten Team gehört, wird ein Zähler für rote Nachbarn erhöht; gehört sie zum blauen Team, wird der Zähler für blaue Nachbarn erhöht. Es automatisiert also das Zählen von Nachbarn unterschiedlicher Fraktionen für eine einzelne Position.

**2. Welche Schlüsselworte/Anweisungen enthält der Code?**
*   **`#define`**: Eine Präprozessor-Direktive, die dem Compiler sagt: "Ersetze überall im folgenden Code `COUNT_NEIGHBOR(idx)` durch diesen Textblock".
*   **`if` / `else if`**: Bedingte Anweisungen, die entscheiden, welcher Zähler erhöht wird.
*   **`==`**: Der Vergleichsoperator, um den Inhalt der Gitterzelle mit den Team-Konstanten zu vergleichen.
*   **`++`**: Der Inkrement-Operator, der den Wert der Zählvariablen um 1 erhöht.
*   **`\` (Backslash)**: Ein Zeilenfortsetzungszeichen. Da Makros normalerweise in einer Zeile stehen müssen, erlaubt der Backslash eine übersichtliche Schreibweise über mehrere Zeilen.

**3. Welche Struktur besitzt der Code?**
Es handelt sich um ein **parametrisiertes Makro**. Es ähnelt einer Funktion, wird aber vor der eigentlichen Kompilierung direkt als Text in den Quellcode kopiert ("Inlining"). Innerhalb des Makros befindet sich eine einfache zweiseitige Auswahl (if-else-Struktur).

**4. Was gibt es zu den Variablen zu sagen?**
*   **`idx`**: Der Platzhalter (Parameter) für den Index im Spielfeld-Array. Er wird beim Aufruf des Makros durch den tatsächlichen Wert ersetzt.
*   **`current_gen->grid[idx]`**: Greift auf das Array `grid` innerhalb der Struktur `current_gen` zu. Hier wird der aktuelle Zustand (TEAM_RED, TEAM_BLUE oder DEAD) der Zelle an Position `idx` gelesen.
*   **`red_neighbors` / `blue_neighbors`**: Diese Variablen müssen an der Stelle, an der das Makro aufgerufen wird, bereits deklariert sein. Das Makro geht davon aus, dass diese Zähler existieren und verändert sie direkt (Seiteneffekt).
*   **`TEAM_RED` / `TEAM_BLUE`**: Dies sind symbolische Konstanten (wahrscheinlich per `#define` oder `enum` definiert), die für bestimmte Zahlenwerte stehen.

**5. Was sonst noch zu erwähnen wäre ...**
*   **Effizienz**: Die Verwendung eines Makros spart hier den "Overhead" eines Funktionsaufrufs (Speichern von Registern, Sprungbefehle). Da dieser Code innerhalb einer Schleife für jede Zelle achtmal aufgerufen wird, ist dies eine gängige Optimierung.
*   **Gefahr**: Makros haben keinen eigenen Gültigkeitsbereich (Scope). Wenn die Variablen `red_neighbors` oder `blue_neighbors` am Aufort nicht existieren, meldet der Compiler einen Fehler.
*   **Wartbarkeit**: Am Ende der Funktion `update_generation` wird das Makro mit `#undef COUNT_NEIGHBOR` wieder gelöscht, damit es in anderen Teilen des Programms nicht versehentlich zu Namenskonflikten führt.

---

## USER 🧑‍💻

In der Codebase gibt es folgende Dateien.
file_io.c file_io.h
game_logic.c game_logic.h
gui.c gui.h
test_logic.c main.c
Wie hängen diese Dateien zusammen und bilden letztendlich das Programm in C? Gibt es für die Ausführung obsolete Dateien? Was ist der Unterschied zwischen einer .c und einer .h Datei?

---

## MODEL ✨

Gerne, das erkläre ich dir für einen Studenten im 1. Semester:

### Wie die Dateien zusammenhängen und das Programm bilden

In C-Projekten ist es üblich, den Code in mehrere `.c`-Dateien und `.h`-Dateien aufzuteilen. Jede `.c`-Datei (Quelldatei) implementiert bestimmte Funktionen, und die zugehörige `.h`-Datei (Header-Datei) deklariert diese Funktionen und Datenstrukturen.

So wirken deine Dateien zusammen, um das "Game of Life"-Programm zu bilden:

1.  **`main.c`**:
    *   Dies ist der Startpunkt deines Programms. Wenn du das Programm ausführst, beginnt die Ausführung in der `main`-Funktion in dieser Datei.
    *   `main.c` wird andere Header-Dateien wie `game_logic.h`, `file_io.h` und `gui.h` inkludieren (`#include`), um deren Funktionen nutzen zu können. Es ist der "Dirigent", der die verschiedenen Teile des Programms aufruft.

2.  **`game_logic.c` und `game_logic.h`**:
    *   **`game_logic.h`**: Deklariert alle Funktionen und Datenstrukturen, die die Spielregeln des "Game of Life" betreffen (z.B. wie eine neue Welt erstellt, initialisiert oder die nächste Generation berechnet wird).
    *   **`game_logic.c`**: Enthält die tatsächliche Implementierung (den "Code") dieser deklarierten Funktionen. Hier steht, *wie* die Spielregeln umgesetzt werden.
    *   `main.c` (und eventuell `gui.c`, wenn die GUI direkt auf Spiellogik zugreifen muss) wird `game_logic.h` einbinden, um die Logikfunktionen aufrufen zu können.

3.  **`file_io.c` und `file_io.h`**:
    *   **`file_io.h`**: Deklariert Funktionen für alle Dateioperationen (Input/Output). Das könnte das Laden eines Start-Setups für das Spielfeld aus einer Datei oder das Speichern des aktuellen Zustands sein.
    *   **`file_io.c`**: Implementiert diese Dateioperationsfunktionen.
    *   `main.c` oder `game_logic.c` könnten `file_io.h` einbinden, um Daten zu lesen oder zu schreiben.

4.  **`gui.c` und `gui.h`**:
    *   **`gui.h`**: Deklariert Funktionen, die für die grafische Benutzeroberfläche (Graphical User Interface) verantwortlich sind. Das beinhaltet zum Beispiel Funktionen zum Zeichnen des Spielfelds auf dem Bildschirm, zum Verarbeiten von Maus- oder Tastatureingaben.
    *   **`gui.c`**: Implementiert die eigentliche Grafikdarstellung und Interaktion.
    *   `main.c` wird `gui.h` einbinden, um die Spielwelt darzustellen und Benutzereingaben zu verarbeiten.

5.  **`test_logic.c`**:
    *   Diese Datei enthält wahrscheinlich Testfälle für die Spiellogik, die in `game_logic.c` implementiert ist. Sie wird `game_logic.h` einbinden, um die Spiellogikfunktionen zu testen.
    *   **Zusammenfassung der Programmbildung:**
        1.  Du schreibst den Code in `.c`-Dateien und deren Deklarationen in `.h`-Dateien.
        2.  Ein **Compiler** übersetzt jede `.c`-Datei einzeln in eine Objektdatei (`.o` oder `.obj`). Dabei werden die `#include`-Anweisungen im Grunde durch den Inhalt der Header-Dateien ersetzt.
        3.  Ein **Linker** nimmt alle Objektdateien und die notwendigen Bibliotheken (wie für Grafik oder Dateizugriff) und fügt sie zu einem einzigen ausführbaren Programm (`.exe` unter Windows) zusammen.

### Gibt es für die Ausführung obsolete Dateien?

Ja, für die *normale Ausführung* des fertigen "Game of Life"-Programms ist die Datei **`test_logic.c`** im Allgemeinen nicht notwendig.

*   `test_logic.c` wird verwendet, um sicherzustellen, dass die Spiellogik korrekt funktioniert, d.h. während der Entwicklung. Du würdest diese Datei kompilieren und ausführen, um Tests durchzuführen.
*   Das Endprodukt, das der Benutzer spielt, wird typischerweise ohne die Testcode-Datei kompiliert, da dieser Code zur Laufzeit nicht benötigt wird. Manchmal wird sie aber auch mitkompiliert, wenn die Tests direkt im Programm aufgerufen werden können (z.B. als Debugging-Modus).

### Was ist der Unterschied zwischen einer `.c` und einer `.h` Datei?

Stell dir das wie folgt vor:

*   **`.h`-Datei (Header-Datei): Die Speisekarte / Das Inhaltsverzeichnis**
    *   Sie enthält **Deklarationen**: Das ist wie eine Liste der Funktionen, die in den zugehörigen `.c`-Dateien existieren und verwendet werden können. Es sagt dir nur den Namen der Funktion, welche Eingaben sie erwartet und welchen Typ von Ergebnis sie liefert – aber nicht *wie* sie das macht.
    *   Sie definiert auch Datenstrukturen (wie `struct World`), die du verwenden kannst.
    *   Der Zweck ist, anderen `.c`-Dateien zu sagen, "was" verfügbar ist, ohne ihnen alle Implementierungsdetails zu zeigen.

*   **`.c`-Datei (Quelldatei): Das Rezeptbuch / Die Implementierung**
    *   Sie enthält **Definitionen**: Das ist der eigentliche Code, der beschreibt, *wie* jede Funktion funktioniert. Hier sind die Anweisungen Schritt für Schritt aufgeführt.
    *   Jede `.c`-Datei **inkludiert** ihre eigene `.h`-Datei (und die `.h`-Dateien anderer Module, deren Funktionen sie nutzen möchte), um sicherzustellen, dass die Deklarationen bekannt sind, bevor die Funktionen definiert oder aufgerufen werden.

Dieser Ansatz hilft dabei, große Programme zu organisieren, Abhängigkeiten zu kontrollieren und die Kompilierungszeiten zu verkürzen, da nicht bei jeder kleinen Änderung der gesamte Code neu übersetzt werden muss.

---

## USER 🧑‍💻

Bitte erkläre 
    // Initial state
    AppState state = STATE_CONFIG;
    GameConfig config = {
        .rows = 50, 
        .cols = 50, 
        .delay_ms = 100, 
        .max_population = 100, 
        .max_rounds = 1000,
        .current_red_pop = 0,
        .current_blue_pop = 0,
        .current_round = 0
    };
in @gui.c
--- Content from referenced files ---
Content from @gui.c:
#include "raylib.h"
#include "gui.h"
#include "file_io.h" // KI-Agent unterstützt
#include <stdio.h>
#include <stdlib.h> // For abs
#include <time.h>   // For time()
#include <string.h> // For strncpy

// Global World Pointer for GUI
World *gui_world = NULL;

// Protocol Archive State
static ProtocolInfo *fileList = NULL;
static int fileCount = 0;
static int selectedFileIndex = 0;
// Current Run Tracking
static char currentProtocolFilename[256] = "";

// --- Theme Colors (Digital Lab) ---
// KI-Agent unterstützt: Sci-Fi / Retro Colors
const Color THEME_BG = { 20, 24, 32, 255 };        // Deep Dark Blue/Grey
const Color THEME_HUD = { 10, 12, 16, 230 };       // Semi-transparent Black
const Color THEME_GRID = { 40, 44, 52, 255 };      // Faint Grid Lines
const Color THEME_RED = { 255, 60, 100, 255 };     // Neon Red/Pink
const Color THEME_BLUE = { 0, 220, 255, 255 };     // Neon Cyan
const Color THEME_TEXT = { 220, 220, 220, 255 };   // Off-White
const Color THEME_HIGHLIGHT = { 255, 255, 255, 40 }; // Selection Glow

// Helper to draw the grid (reused in multiple states)
// KI-Agent unterstützt: Optimized Texture-Based Rendering for VcXsrv performance
void DrawGridAndCells(GameConfig *config, int screenWidth, int screenHeight, bool drawGridLines) {
    if (!gui_world) return;

    // Layout Constants
    const int headerHeight = 60;
    const int footerHeight = 40;
    const int margin = 20;
    
    int drawWidth = screenWidth - (margin * 2);
    int drawHeight = screenHeight - headerHeight - footerHeight - (margin * 1); // margin bottom handled by footer
    int startX = margin;
    int startY = headerHeight;
    
    float cellW = (float)drawWidth / config->cols;
    float cellH = (float)drawHeight / config->rows;
    
    // --- 1. Texture Management (Static to persist across frames) ---
    static Texture2D gridTex = { 0 };
    static int texW = 0;
    static int texH = 0;
    static Color *pixels = NULL;
    
    // Check if grid size changed or not initialized
    if (config->cols != texW || config->rows != texH) {
        // Cleanup old resources
        if (gridTex.id > 0) UnloadTexture(gridTex);
        if (pixels) free(pixels);
        
        // Update dimensions
        texW = config->cols;
        texH = config->rows;
        
        // Allocate new resources
        pixels = (Color*)malloc(texW * texH * sizeof(Color));
        Image img = GenImageColor(texW, texH, BLANK); // Create empty image
        gridTex = LoadTextureFromImage(img);
        UnloadImage(img);
        
        // IMPORTANT: Point filtering ensures sharp pixels when scaled up
        SetTextureFilter(gridTex, TEXTURE_FILTER_POINT); 
    }
    
    // --- 2. Update Pixel Data (CPU side) ---
    // Instead of thousands of DrawRectangle calls, we update a single buffer.
    for (int i = 0; i < texW * texH; i++) {
        if (gui_world->grid[i] == TEAM_BLUE) {
            pixels[i] = THEME_BLUE;
        } else if (gui_world->grid[i] == TEAM_RED) {
            pixels[i] = THEME_RED;
        } else {
            pixels[i] = BLANK; // Transparent, so background shows through
        }
    }
    
    // --- 3. Upload to GPU & Draw ---
    UpdateTexture(gridTex, pixels);
    
    Rectangle source = { 0.0f, 0.0f, (float)texW, (float)texH };
    Rectangle dest = { (float)startX, (float)startY, (float)drawWidth, (float)drawHeight };
    Vector2 origin = { 0.0f, 0.0f };
    
    DrawTexturePro(gridTex, source, dest, origin, 0.0f, WHITE);

    // --- 4. Draw Grid Lines (Optional - Overhead is low for lines) ---
    if (drawGridLines) {
        for (int i = 0; i <= config->cols; i++) DrawLine(startX + i * cellW, startY, startX + i * cellW, startY + drawHeight, THEME_GRID);
        for (int i = 0; i <= config->rows; i++) DrawLine(startX, startY + i * cellH, startX + drawWidth, startY + i * cellH, THEME_GRID);
    }
    
    // 5. Draw Hemisphere Separator
    DrawLine(startX + (config->cols / 2) * cellW, startY, 
             startX + (config->cols / 2) * cellW, startY + drawHeight, Fade(THEME_TEXT, 0.3f));
}

// KI-Agent unterstützt: Pattern Definitions
typedef struct { int r; int c; } Point;

void PlacePattern(World *w, GameConfig *c, int startR, int startC, int type) {
    int team;
    int midCol = c->cols / 2;
    int *current_pop;
    
    // Determine Team based on Mouse Cursor (Start Position)
    if (startC < midCol) {
        team = TEAM_BLUE;
        current_pop = &c->current_blue_pop;
    } else {
        team = TEAM_RED;
        current_pop = &c->current_red_pop;
    }

    // Pattern Data
    // Glider (3x3)
    Point p_glider[] = {{0,1}, {1,2}, {2,0}, {2,1}, {2,2}};
    // HWSS (Traveler) (5x7)
    Point p_traveler[] = {{0,3}, {0,4}, {1,1}, {1,6}, {2,0}, {3,0}, {3,6}, {4,0}, {4,1}, {4,2}, {4,3}, {4,4}, {4,5}};
    // Gosper Glider Gun (Blaster) (9x36)
    Point p_blaster[] = {
        {4,0}, {5,0}, {4,1}, {5,1}, // Left Block
        {4,10}, {5,10}, {6,10}, {3,11}, {7,11}, {2,12}, {8,12}, {2,13}, {8,13}, {5,14}, {3,15}, {7,15}, {4,16}, {5,16}, {6,16}, {5,17}, // Left Mech
        {2,20}, {3,20}, {4,20}, {2,21}, {3,21}, {4,21}, {1,22}, {5,22}, {0,24}, {1,24}, {5,24}, {6,24}, // Right Mech
        {2,34}, {3,34}, {2,35}, {3,35} // Right Block
    };

    Point *cells = NULL;
    int count = 0;

    if (type == 1) { cells = p_glider; count = 5; }
    else if (type == 2) { cells = p_traveler; count = 13; }
    else if (type == 3) { cells = p_blaster; count = 36; }

    for (int i = 0; i < count; i++) {
        // Check Population Limit
        if (*current_pop >= c->max_population) break;

        // Calculate Wrap-around Coordinates
        int r = (startR + cells[i].r) % c->rows;
        int col = (startC + cells[i].c) % c->cols;
        
        // Handle negative modulo (if logic ever allows negative offsets)
        if (r < 0) r += c->rows;
        if (col < 0) col += c->cols;

        // Check Border Crossing (Clipping Rule)
        bool valid = false;
        if (team == TEAM_BLUE && col < midCol) valid = true;
        if (team == TEAM_RED && col >= midCol) valid = true;

        if (valid) {
            int idx = r * c->cols + col;
            if (w->grid[idx] == DEAD) {
                w->grid[idx] = team;
                (*current_pop)++;
            }
        }
    }
}

// Helper for continuous input handling (Key Repeat)
bool IsActionTriggered(int key) {
    static int activeKey = -1;
    static float timer = 0.0f;
    const float INITIAL_DELAY = 0.5f;
    const float REPEAT_INTERVAL = 0.05f; 

    if (IsKeyPressed(key)) {
        activeKey = key;
        timer = 0.0f;
        return true;
    }

    if (IsKeyDown(key)) {
        if (activeKey == key) {
            timer += GetFrameTime();
            if (timer >= INITIAL_DELAY + REPEAT_INTERVAL) {
                timer = INITIAL_DELAY; 
                return true;
            }
        }
    } else {
        if (activeKey == key) {
            activeKey = -1;
            timer = 0.0f;
        }
    }
    return false;
}

// KI-Agent unterstützt
void run_gui_app() {
    // Initial window size
    int screenWidth = 800;
    int screenHeight = 600;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE); 
    InitWindow(screenWidth, screenHeight, "Biotope - Game of Life");
    SetTargetFPS(60);

    // Initial state
    AppState state = STATE_CONFIG;
    GameConfig config = {
        .rows = 50, 
        .cols = 50, 
        .delay_ms = 100, 
        .max_population = 100, 
        .max_rounds = 1000,
        .current_red_pop = 0,
        .current_blue_pop = 0,
        .current_round = 0
    };
    
    // Feedback Message System
    char statusMsg[64] = "";
    float statusTimer = 0.0f;

    while (!WindowShouldClose()) {
        // Update dynamic screen dimensions
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        
        // Timer for status message
        if (statusTimer > 0) {
            statusTimer -= GetFrameTime();
            if (statusTimer <= 0) strcpy(statusMsg, "");
        }
        
        // --- Logic per State ---
        switch (state) {
            case STATE_CONFIG:
                // Interaction: Change Grid Size
                if (IsActionTriggered(KEY_RIGHT)) config.cols += 10;
                if (IsActionTriggered(KEY_LEFT) && config.cols > 10) config.cols -= 10;
                if (IsActionTriggered(KEY_UP)) config.rows += 10;
                if (IsActionTriggered(KEY_DOWN) && config.rows > 10) config.rows -= 10;
                
                // Interaction: Change Delay (incl. German Layout)
                if (IsActionTriggered(KEY_KP_ADD) || IsActionTriggered(KEY_EQUAL) || IsActionTriggered(KEY_RIGHT_BRACKET)) 
                    config.delay_ms += 50;
                if ((IsActionTriggered(KEY_KP_SUBTRACT) || IsActionTriggered(KEY_MINUS) || IsActionTriggered(KEY_SLASH)) && config.delay_ms > 0) 
                    config.delay_ms -= 50;

                // Interaction: Change Max Rounds
                if (IsActionTriggered(KEY_PAGE_UP)) config.max_rounds += 100;
                if (IsActionTriggered(KEY_PAGE_DOWN) && config.max_rounds > 100) config.max_rounds -= 100;

                // Interaction: Change Max Population
                int max_squad_cells = (config.rows * config.cols) / 2;
                // Clamp if grid size reduced below current max_pop
                if (config.max_population > max_squad_cells) config.max_population = max_squad_cells;

                if (IsActionTriggered(KEY_INSERT) && config.max_population < max_squad_cells) {
                    config.max_population += 10;
                    if (config.max_population > max_squad_cells) config.max_population = max_squad_cells;
                }
                if (IsActionTriggered(KEY_DELETE) && config.max_population > 10) config.max_population -= 10;

                // Transition: Start Setup
                if (IsKeyPressed(KEY_ENTER)) {
                    if (gui_world) free_world(gui_world);
                    gui_world = create_world(config.rows, config.cols);
                    // Initialize empty
                    for(int i=0; i<config.rows*config.cols; i++) gui_world->grid[i] = DEAD;
                    
                    config.current_blue_pop = 0;
                    config.current_red_pop = 0;
                    config.current_round = 0;
                    
                    state = STATE_EDIT;
                }
                break;

            case STATE_EDIT:
                // --- Mouse & Pattern Interaction ---
                {
                    Vector2 mousePos = GetMousePosition();
                    
                    // Constants must match DrawGridAndCells layout
                    const int headerHeight = 60;
                    const int footerHeight = 40;
                    const int margin = 20;
                    int drawWidth = screenWidth - (margin * 2);
                    int drawHeight = screenHeight - headerHeight - footerHeight - margin;
                    int startX = margin;
                    int startY = headerHeight;
                    
                    float cellW = (float)drawWidth / config.cols;
                    float cellH = (float)drawHeight / config.rows;
                    
                    // State for Drag-and-Paint interaction
                    static int editAction = 0; // 0:Idle, 1:Place, 2:Remove
                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) editAction = 0;

                    // Check if mouse is inside the grid area
                    if (mousePos.x >= startX && mousePos.x < startX + drawWidth &&
                        mousePos.y >= startY && mousePos.y < startY + drawHeight) {
                        
                        int col = (int)((mousePos.x - startX) / cellW);
                        int row = (int)((mousePos.y - startY) / cellH);
                        
                        // Handle Clicks (Single Cell) & Drag
                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            int index = row * config.cols + col;
                            // Determine action based on initial cell state: Place (1) or Remove (2)
                            if (gui_world->grid[index] == DEAD) editAction = 1;
                            else editAction = 2;
                        }

                        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && editAction != 0) {
                            int index = row * config.cols + col;
                            int midCol = config.cols / 2;

                            // Check Hemispheres and Population Limits
                            if (col < midCol) {
                                if (editAction == 2 && gui_world->grid[index] == TEAM_BLUE) {
                                    gui_world->grid[index] = DEAD;
                                    config.current_blue_pop--;
                                } else if (editAction == 1 && gui_world->grid[index] == DEAD && config.current_blue_pop < config.max_population) {
                                    gui_world->grid[index] = TEAM_BLUE;
                                    config.current_blue_pop++;
                                }
                            } else {
                                if (editAction == 2 && gui_world->grid[index] == TEAM_RED) {
                                    gui_world->grid[index] = DEAD;
                                    config.current_red_pop--;
                                } else if (editAction == 1 && gui_world->grid[index] == DEAD && config.current_red_pop < config.max_population) {
                                    gui_world->grid[index] = TEAM_RED;
                                    config.current_red_pop++;
                                }
                            }
                        }
                        
                        // Handle Patterns
                        if (IsKeyPressed(KEY_G)) {
                            PlacePattern(gui_world, &config, row, col, 1); // 1 = Glider
                            strcpy(statusMsg, "Deployed: GLIDER");
                            statusTimer = 2.0f;
                        }
                        if (IsKeyPressed(KEY_T)) {
                            PlacePattern(gui_world, &config, row, col, 2); // 2 = Traveler
                            strcpy(statusMsg, "Deployed: TRAVELER");
                            statusTimer = 2.0f;
                        }
                        if (IsKeyPressed(KEY_B)) {
                            PlacePattern(gui_world, &config, row, col, 3); // 3 = Blaster
                            strcpy(statusMsg, "Deployed: BLASTER");
                            statusTimer = 2.0f;
                        }
                    }
                }
                
                // File I/O
                if (IsKeyPressed(KEY_S)) {
                    if (save_grid("setup.bio", gui_world, &config)) {
                        strcpy(statusMsg, "Saved to setup.bio!");
                        statusTimer = 2.0f;
                    } else {
                        strcpy(statusMsg, "Save Failed!");
                        statusTimer = 2.0f;
                    }
                }
                
                // NEW: Load State Transition
                if (IsKeyPressed(KEY_L)) {
                    fileCount = list_protocol_files("biotope_results", &fileList);
                    selectedFileIndex = 0;
                    state = STATE_LOAD;
                }
                
                // KI-Agent unterstützt: Random Placement Logic
                if (IsKeyPressed(KEY_R)) {
                    // Reset grid
                    for(int i=0; i<config.rows*config.cols; i++) gui_world->grid[i] = DEAD;
                    config.current_blue_pop = 0;
                    config.current_red_pop = 0;
                    
                    int midCol = config.cols / 2;
                    // Seed random
                    srand(time(NULL));
                    
                    // Iterate and randomly fill
                    for(int r=0; r<config.rows; r++) {
                        for(int c=0; c<config.cols; c++) {
                            int idx = r * config.cols + c;
                            // 20% chance to be alive
                            if ((rand() % 100) < 20) {
                                if (c < midCol) {
                                    if (config.current_blue_pop < config.max_population) {
                                        gui_world->grid[idx] = TEAM_BLUE;
                                        config.current_blue_pop++;
                                    }
                                } else {
                                    if (config.current_red_pop < config.max_population) {
                                        gui_world->grid[idx] = TEAM_RED;
                                        config.current_red_pop++;
                                    }
                                }
                            }
                        }
                    }
                    strcpy(statusMsg, "Randomized Grid!");
                    statusTimer = 2.0f;
                }
                
                // Transition: Start Simulation with Auto-Save
                if (IsKeyPressed(KEY_ENTER)) {
                    // Phase 3: Auto-Save on Start
                    char autoFilename[128];
                    time_t now = time(NULL);
                    struct tm *t = localtime(&now);
                    strftime(autoFilename, sizeof(autoFilename), "biotope_results/run_%Y%m%d_%H%M%S.bio", t);
                    
                    // Store for result appending later
                    strcpy(currentProtocolFilename, autoFilename);
                    
                    if (save_grid(autoFilename, gui_world, &config)) {
                        printf("Auto-save successful: %s\n", autoFilename);
                    } else {
                        printf("Auto-save failed!\n");
                    }
                    
                    state = STATE_RUNNING;
                }
                break;

            case STATE_LOAD:
                if (IsKeyPressed(KEY_UP) && selectedFileIndex > 0) selectedFileIndex--;
                if (IsKeyPressed(KEY_DOWN) && selectedFileIndex < fileCount - 1) selectedFileIndex++;
                
                if (IsKeyPressed(KEY_ENTER) && fileCount > 0) {
                    if (load_grid(fileList[selectedFileIndex].filepath, gui_world, &config)) {
                        strcpy(statusMsg, "Protocol Loaded!");
                        statusTimer = 2.0f;
                    }
                    if (fileList) free(fileList);
                    fileList = NULL;
                    state = STATE_EDIT;
                }
                
                if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_ESCAPE)) {
                    if (fileList) free(fileList);
                    fileList = NULL;
                    state = STATE_EDIT;
                }
                break;
            
            case STATE_RUNNING:
                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_Q)) {
                    if (gui_world) free_world(gui_world);
                    gui_world = NULL;
                    state = STATE_CONFIG;
                    break;
                }

                // --- Simulation Logic ---
                static float timeAccumulator = 0.0f;
                timeAccumulator += GetFrameTime();
                
                if (timeAccumulator >= config.delay_ms / 1000.0f) {
                    timeAccumulator = 0.0f;
                    
                    World *next_gen = create_world(config.rows, config.cols);
                    update_generation(gui_world, next_gen, config.rows, config.cols);
                    free_world(gui_world);
                    gui_world = next_gen;
                    
                    config.current_round++;
                    
                    config.current_red_pop = 0;
                    config.current_blue_pop = 0;
                    for(int i=0; i<config.rows*config.cols; i++) {
                        if (gui_world->grid[i] == TEAM_RED) config.current_red_pop++;
                        if (gui_world->grid[i] == TEAM_BLUE) config.current_blue_pop++;
                    }
                    
                    if (config.current_round >= config.max_rounds || 
                        config.current_red_pop == 0 || 
                        config.current_blue_pop == 0) {
                        state = STATE_FINISHED;
                    }
                }
                break;
                
            case STATE_FINISHED:
                if (IsKeyPressed(KEY_ENTER)) {
                     state = STATE_GAME_OVER;
                     int winner = 0;
                     if (config.current_red_pop > config.current_blue_pop) winner = TEAM_RED;
                     else if (config.current_blue_pop > config.current_red_pop) winner = TEAM_BLUE;
                     
                     // Append to Protocol
                     if (strlen(currentProtocolFilename) > 0) {
                        append_protocol_result(currentProtocolFilename, winner, config.current_red_pop, config.current_blue_pop);
                     }
                }
                if (IsKeyPressed(KEY_Q)) {
                    if (gui_world) free_world(gui_world);
                    gui_world = NULL;
                    state = STATE_CONFIG;
                }
                break;

            case STATE_GAME_OVER:
                if (IsKeyPressed(KEY_ONE)) {
                     if (gui_world) free_world(gui_world);
                     gui_world = NULL;
                     state = STATE_CONFIG;
                }
                break;
        }

        // --- Drawing ---
        BeginDrawing();
        ClearBackground(THEME_BG);

        // Draw HUD Backgrounds (Header & Footer)
        DrawRectangle(0, 0, screenWidth, 60, THEME_HUD); // Header
        DrawRectangle(0, screenHeight - 40, screenWidth, 40, THEME_HUD); // Footer

        // Draw Status Message Overlay
        if (statusTimer > 0) {
            // KI-Agent unterstützt: Center status message to avoid collision with counters
            DrawText(statusMsg, screenWidth/2 - MeasureText(statusMsg, 20)/2, 20, 20, GREEN);
        }

        switch (state) {
            case STATE_CONFIG:
                DrawText("BIOTOPE CONFIGURATION", 20, 15, 30, THEME_TEXT);
                
                char buf[64];
                sprintf(buf, "GRID SIZE:  %03d x %03d", config.rows, config.cols);
                DrawText(buf, 40, 100, 20, THEME_BLUE);
                DrawText("(Arrows)", 300, 100, 18, DARKGRAY);
                
                sprintf(buf, "DELAY:      %04d ms", config.delay_ms);
                DrawText(buf, 40, 140, 20, THEME_RED);
                DrawText("(+/-)", 300, 140, 18, DARKGRAY);
                
                sprintf(buf, "MAX ROUNDS: %04d", config.max_rounds);
                DrawText(buf, 40, 180, 20, THEME_BLUE);
                DrawText("(PageUp/PageDown)", 300, 180, 18, DARKGRAY);
                
                sprintf(buf, "MAX INIT POP:    %04d", config.max_population);
                DrawText(buf, 40, 220, 20, THEME_RED);
                DrawText("(Insert/Delete)", 300, 220, 18, DARKGRAY);
                
                // KI-Agent unterstützt: Mission Protocol (Rules Display)
                int rulesX = screenWidth / 2 + 40;
                DrawLine(rulesX - 20, 100, rulesX - 20, 240, Fade(THEME_TEXT, 0.3f)); // Vertical Separator
                
                DrawText("CONWAY'S MISSION PROTOCOL", rulesX, 100, 20, THEME_HIGHLIGHT);
                DrawText("- SURVIVAL: 2 or 3 neighbors", rulesX, 135, 20, THEME_TEXT);
                DrawText("- BIRTH: 3 neighbors (Majority Rule of parents)", rulesX, 160, 20, THEME_TEXT);
                DrawText("- TEAMS: RED vs BLUE", rulesX, 185, 20, THEME_TEXT);
                DrawText("- GOAL: Max Population after timeout", rulesX, 210, 20, THEME_TEXT);

                DrawText("PRESS [ENTER] TO INITIALIZE SYSTEM", 40, 300, 20, THEME_HIGHLIGHT);
                break;

            case STATE_EDIT:
                DrawText("EDITOR MODE", 20, 18, 24, THEME_BLUE);
                // KI-Agent unterstützt: Increased font size to 16 for better readability
                DrawText("LEFT: BLUE SQUAD  |  RIGHT: RED SQUAD", 220, 24, 16, DARKGRAY);
                
                // Draw Population Counters
                char popBuf[64];
                sprintf(popBuf, "BLUE: %03d/%03d", config.current_blue_pop, config.max_population);
                DrawText(popBuf, screenWidth - 300, 20, 20, THEME_BLUE);
                sprintf(popBuf, "RED: %03d/%03d", config.current_red_pop, config.max_population);
                DrawText(popBuf, screenWidth - 140, 20, 20, THEME_RED);
                
                // Ghost Cursor (Visual Polish)
                Vector2 mousePos = GetMousePosition();
                // ... Re-calculate grid metrics for ghost cursor ...
                {
                     const int headerHeight = 60;
                    const int footerHeight = 40;
                    const int margin = 20;
                    int drawWidth = screenWidth - (margin * 2);
                    int drawHeight = screenHeight - headerHeight - footerHeight - margin;
                    int startX = margin;
                    int startY = headerHeight;
                    float cellW = (float)drawWidth / config.cols;
                    float cellH = (float)drawHeight / config.rows;
                    
                    if (mousePos.x >= startX && mousePos.x < startX + drawWidth &&
                        mousePos.y >= startY && mousePos.y < startY + drawHeight) {
                        int col = (int)((mousePos.x - startX) / cellW);
                        int row = (int)((mousePos.y - startY) / cellH);
                        // Draw Ghost
                        Color ghostColor = (col < config.cols/2) ? Fade(THEME_BLUE, 0.2f) : Fade(THEME_RED, 0.2f);
                        DrawRectangle(startX + col * cellW, startY + row * cellH, cellW, cellH, ghostColor);
                    }
                }
                
                // KI-Agent unterstützt: Draw grid lines only if grid is not too dense (> 150)
                bool showLines = (config.rows <= 150 && config.cols <= 150);
                DrawGridAndCells(&config, screenWidth, screenHeight, showLines); 

                // KI-Agent unterstützt: Updated Footer Menu Font Size to 14
                DrawText("[ENTER] RUN | [S] SAVE | [L] LOAD | [R] RANDOM | [G] GLIDER | [T] TRAVELER | [B] BLASTER", 
                         20, screenHeight - 28, 20, THEME_TEXT);
                break;

            case STATE_LOAD:
                DrawText("PROTOCOL ARCHIVE", 20, 15, 30, THEME_TEXT);
                DrawText("SELECT A SIMULATION RUN TO REPLAY", 400, 24, 16, DARKGRAY);

                if (fileCount == 0) {
                    DrawText("NO PROTOCOLS FOUND IN 'biotope_results/'", 40, 100, 20, THEME_RED);
                } else {
                    // Draw List
                    int startY = 100;
                    int itemHeight = 30;
                    int visibleItems = (screenHeight - 150) / itemHeight;
                    
                    // Simple scrolling view
                    int scrollOffset = 0;
                    if (selectedFileIndex >= visibleItems) scrollOffset = selectedFileIndex - visibleItems + 1;

                    for (int i = 0; i < visibleItems && (i + scrollOffset) < fileCount; i++) {
                        int idx = i + scrollOffset;
                        Color col = (idx == selectedFileIndex) ? THEME_BLUE : THEME_TEXT;
                        if (idx == selectedFileIndex) {
                            DrawRectangle(30, startY + i * itemHeight - 5, 400, itemHeight, THEME_HIGHLIGHT);
                            DrawText(">", 15, startY + i * itemHeight, 20, THEME_BLUE);
                        }
                        DrawText(fileList[idx].filename, 40, startY + i * itemHeight, 20, col);
                    }

                    // Draw Preview Panel
                    int previewX = 460;
                    DrawLine(previewX - 20, 100, previewX - 20, screenHeight - 60, Fade(THEME_TEXT, 0.3f));
                    
                    DrawText("PROTOCOL PREVIEW", previewX, 100, 20, THEME_HIGHLIGHT);
                    
                    ProtocolInfo *sel = &fileList[selectedFileIndex];
                    char infoBuf[128];
                    
                    if (sel->timestamp > 0) {
                        struct tm *t = localtime(&sel->timestamp);
                        strftime(infoBuf, sizeof(infoBuf), "DATE: %d.%m.%Y %H:%M:%S", t);
                        DrawText(infoBuf, previewX, 140, 20, THEME_TEXT);
                    } else {
                        DrawText("DATE: LEGACY FORMAT", previewX, 140, 20, DARKGRAY);
                    }
                    
                    sprintf(infoBuf, "GRID: %d x %d", sel->rows, sel->cols);
                    DrawText(infoBuf, previewX, 170, 20, THEME_TEXT);
                    
                    sprintf(infoBuf, "MAX ROUNDS: %d", sel->max_rounds);
                    DrawText(infoBuf, previewX, 200, 20, THEME_TEXT);
                    
                    sprintf(infoBuf, "MAX POPULATION: %d", sel->max_population);
                    DrawText(infoBuf, previewX, 230, 20, THEME_TEXT);
                    
                    if (sel->has_results) {
                        DrawLine(previewX - 10, 260, previewX + 250, 260, Fade(THEME_TEXT, 0.3f));
                        DrawText("RESULTS:", previewX, 270, 20, THEME_HIGHLIGHT);
                        
                        if (sel->winner == 1) DrawText("WINNER: RED", previewX, 300, 20, THEME_RED);
                        else if (sel->winner == 2) DrawText("WINNER: BLUE", previewX, 300, 20, THEME_BLUE);
                        else DrawText("WINNER: DRAW", previewX, 300, 20, DARKGRAY);
                        
                        char scoreBuf[64];
                        sprintf(scoreBuf, "R:%d  B:%d", sel->final_red, sel->final_blue);
                        DrawText(scoreBuf, previewX, 330, 20, THEME_TEXT);
                    } else {
                        DrawText("NO RESULTS YET", previewX, 270, 18, DARKGRAY);
                    }

                    DrawText("PRESS [ENTER] TO LOAD", previewX, 380, 20, GREEN);
                }

                DrawText("[UP/DOWN] NAVIGATE  |  [ENTER] LOAD  |  [Q/ESC] CANCEL", 20, screenHeight - 28, 20, THEME_TEXT);
                break;

            case STATE_RUNNING:
                DrawText("SIMULATION ACTIVE", 20, 18, 24, THEME_RED);
                
                // KI-Agent unterstützt: Stable positioning for Label and Counter
                const char* labelText = "ECO-BLOOM CYCLE:";
                char roundBuf[32];
                sprintf(roundBuf, "%04d / %04d", config.current_round, config.max_rounds);
                
                // Calculate widths based on a "worst-case" wide string to prevent jitter
                int maxCounterWidth = MeasureText("0000 / 0000", 20); 
                int labelWidth = MeasureText(labelText, 20);
                int gap = 10;
                int rightMargin = 20;
                
                // Draw Label (Fixed position relative to right edge)
                DrawText(labelText, screenWidth - rightMargin - maxCounterWidth - gap - labelWidth, 20, 20, THEME_TEXT);
                
                // Draw Counter (Fixed start position)
                DrawText(roundBuf, screenWidth - rightMargin - maxCounterWidth, 20, 20, THEME_TEXT);
                
                DrawGridAndCells(&config, screenWidth, screenHeight, false); // false = No Grid Lines (Performance!)
                
                DrawText("[Q] ABORT SIMULATION", 20, screenHeight - 30, 20, DARKGRAY);
                break;
                
            case STATE_FINISHED:
                DrawText("SIMULATION COMPLETED", 20, 18, 24, THEME_BLUE);
                
                DrawGridAndCells(&config, screenWidth, screenHeight, false);
                
                DrawText("[ENTER] VIEW RESULTS  |  [Q] MENU", 20, screenHeight - 30, 20, THEME_HIGHLIGHT);
                break;

            case STATE_GAME_OVER:
                DrawText("MISSION REPORT", screenWidth/2 - 100, 100, 30, THEME_TEXT);
                
                char resultBuf[128];
                Color winnerColor = THEME_TEXT;
                if (config.current_red_pop > config.current_blue_pop) {
                    sprintf(resultBuf, "WINNER: RED TEAM");
                    winnerColor = THEME_RED;
                } else if (config.current_blue_pop > config.current_red_pop) {
                    sprintf(resultBuf, "WINNER: BLUE TEAM");
                    winnerColor = THEME_BLUE;
                } else {
                    sprintf(resultBuf, "RESULT: DRAW");
                }
                
                DrawText(resultBuf, screenWidth/2 - MeasureText(resultBuf, 40)/2, 200, 40, winnerColor);
                
                sprintf(buf, "RED: %d  vs  BLUE: %d", config.current_red_pop, config.current_blue_pop);
                DrawText(buf, screenWidth/2 - MeasureText(buf, 20)/2, 260, 20, GRAY);
                
                DrawText("Stats exported to file.", screenWidth/2 - MeasureText("Stats exported to file.", 20)/2, 400, 20, DARKGRAY);
                DrawText("PRESS [1] TO RESTART SYSTEM", screenWidth/2 - MeasureText("PRESS [1] TO RESTART SYSTEM", 20)/2, 500, 20, THEME_HIGHLIGHT);
                break;
        }

        EndDrawing();
    }

    if (gui_world) free_world(gui_world);
    CloseWindow();
}
--- End of content ---

---

## USER 🧑‍💻

A

---

## MODEL ✨

Dieser Code-Abschnitt bereitet die grundlegenden Einstellungen (die "Konfiguration") für das Programm vor, bevor das eigentliche Fenster oder die Simulation startet.

**1. Was macht der Code?**
Der Code legt zwei Dinge fest:
1.  In welchem Modus (Zustand) die Anwendung starten soll (hier: im Konfigurationsmenü).
2.  Welche Standardwerte für das Spiel gelten sollen (wie groß ist das Spielfeld, wie schnell läuft es, etc.).

**2. Welche Schlüsselworte/Anweisungen enthält der Code?**
*   **`AppState` / `GameConfig`**: Das sind benutzerdefinierte Datentypen. `AppState` ist wahrscheinlich ein `enum` (eine Aufzählung von Zuständen), und `GameConfig` ist ein `struct` (eine Struktur, die mehrere verschiedene Werte zusammenfasst).
*   **`=` (Zuweisungsoperator)**: Damit werden die Werte in die Variablen geschrieben.
*   **`{ ... }` (Initialisierungsliste)**: Alles, was in den geschweiften Klammern steht, gehört zusammen zur Struktur `config`.
*   **`.rows = 50` (Designated Initializer)**: Dies ist eine moderne C-Schreibweise (seit C99). Sie sagt explizit: "Suche in der Struktur das Feld `rows` und setze es auf 50". Das ist sicherer und lesbarer als nur die Zahlen der Reihe nach aufzuschreiben.

**3. Welche Struktur besitzt der Code?**
Es handelt sich um zwei **Variablendeklarationen mit gleichzeitiger Initialisierung**:
*   Zuerst eine einfache Variable (`state`).
*   Dann eine komplexe Strukturvariable (`config`), die aus vielen Unter-Variablen besteht.

**4. Was gibt es zu den Variablen zu sagen?**
*   **`state`**: Diese Variable steuert die Programmlogik. Mit `STATE_CONFIG` weiß das Programm: "Zeige zuerst das Einstellungsmenü an". Später wird dieser Wert auf `STATE_EDIT` oder `STATE_RUNNING` geändert.
*   **`config`**: Dies ist der "Werkzeugkasten" mit allen Regeln:
    *   `rows` / `cols`: Das Spielfeld ist 50x50 Felder groß.
    *   `delay_ms`: Zwischen zwei Schritten liegen 100 Millisekunden (die Geschwindigkeit).
    *   `max_population`: Jedes Team darf maximal 100 Zellen gleichzeitig haben.
    *   `max_rounds`: Nach 1000 Schritten stoppt die Simulation automatisch.
    *   `current_...`: Diese Werte stehen auf 0, weil zu Beginn noch keine Runde gespielt wurde und noch keine Zellen auf dem Feld sind.

**5. Was sonst noch zu erwähnen wäre ...**
Durch diese zentrale Stelle am Anfang der Funktion `run_gui_app` kann man das Verhalten des Spiels sehr leicht ändern. Wenn du möchtest, dass das Spiel langsamer läuft, müsstest du nur die `100` bei `delay_ms` in einen höheren Wert ändern. Es ist gute Programmierpraxis, solche "Standardwerte" gesammelt an einer Stelle zu definieren, statt sie später im Code zu verstecken.

---

## USER 🧑‍💻

Bitte erkläre
// --- Logic per State ---
        switch (state) {
            case STATE_CONFIG:
in @gui.c
--- Content from referenced files ---
Content from @gui.c:
#include "raylib.h"
#include "gui.h"
#include "file_io.h" // KI-Agent unterstützt
#include <stdio.h>
#include <stdlib.h> // For abs
#include <time.h>   // For time()
#include <string.h> // For strncpy

// Global World Pointer for GUI
World *gui_world = NULL;

// Protocol Archive State
static ProtocolInfo *fileList = NULL;
static int fileCount = 0;
static int selectedFileIndex = 0;
// Current Run Tracking
static char currentProtocolFilename[256] = "";

// --- Theme Colors (Digital Lab) ---
// KI-Agent unterstützt: Sci-Fi / Retro Colors
const Color THEME_BG = { 20, 24, 32, 255 };        // Deep Dark Blue/Grey
const Color THEME_HUD = { 10, 12, 16, 230 };       // Semi-transparent Black
const Color THEME_GRID = { 40, 44, 52, 255 };      // Faint Grid Lines
const Color THEME_RED = { 255, 60, 100, 255 };     // Neon Red/Pink
const Color THEME_BLUE = { 0, 220, 255, 255 };     // Neon Cyan
const Color THEME_TEXT = { 220, 220, 220, 255 };   // Off-White
const Color THEME_HIGHLIGHT = { 255, 255, 255, 40 }; // Selection Glow

// Helper to draw the grid (reused in multiple states)
// KI-Agent unterstützt: Optimized Texture-Based Rendering for VcXsrv performance
void DrawGridAndCells(GameConfig *config, int screenWidth, int screenHeight, bool drawGridLines) {
    if (!gui_world) return;

    // Layout Constants
    const int headerHeight = 60;
    const int footerHeight = 40;
    const int margin = 20;
    
    int drawWidth = screenWidth - (margin * 2);
    int drawHeight = screenHeight - headerHeight - footerHeight - (margin * 1); // margin bottom handled by footer
    int startX = margin;
    int startY = headerHeight;
    
    float cellW = (float)drawWidth / config->cols;
    float cellH = (float)drawHeight / config->rows;
    
    // --- 1. Texture Management (Static to persist across frames) ---
    static Texture2D gridTex = { 0 };
    static int texW = 0;
    static int texH = 0;
    static Color *pixels = NULL;
    
    // Check if grid size changed or not initialized
    if (config->cols != texW || config->rows != texH) {
        // Cleanup old resources
        if (gridTex.id > 0) UnloadTexture(gridTex);
        if (pixels) free(pixels);
        
        // Update dimensions
        texW = config->cols;
        texH = config->rows;
        
        // Allocate new resources
        pixels = (Color*)malloc(texW * texH * sizeof(Color));
        Image img = GenImageColor(texW, texH, BLANK); // Create empty image
        gridTex = LoadTextureFromImage(img);
        UnloadImage(img);
        
        // IMPORTANT: Point filtering ensures sharp pixels when scaled up
        SetTextureFilter(gridTex, TEXTURE_FILTER_POINT); 
    }
    
    // --- 2. Update Pixel Data (CPU side) ---
    // Instead of thousands of DrawRectangle calls, we update a single buffer.
    for (int i = 0; i < texW * texH; i++) {
        if (gui_world->grid[i] == TEAM_BLUE) {
            pixels[i] = THEME_BLUE;
        } else if (gui_world->grid[i] == TEAM_RED) {
            pixels[i] = THEME_RED;
        } else {
            pixels[i] = BLANK; // Transparent, so background shows through
        }
    }
    
    // --- 3. Upload to GPU & Draw ---
    UpdateTexture(gridTex, pixels);
    
    Rectangle source = { 0.0f, 0.0f, (float)texW, (float)texH };
    Rectangle dest = { (float)startX, (float)startY, (float)drawWidth, (float)drawHeight };
    Vector2 origin = { 0.0f, 0.0f };
    
    DrawTexturePro(gridTex, source, dest, origin, 0.0f, WHITE);

    // --- 4. Draw Grid Lines (Optional - Overhead is low for lines) ---
    if (drawGridLines) {
        for (int i = 0; i <= config->cols; i++) DrawLine(startX + i * cellW, startY, startX + i * cellW, startY + drawHeight, THEME_GRID);
        for (int i = 0; i <= config->rows; i++) DrawLine(startX, startY + i * cellH, startX + drawWidth, startY + i * cellH, THEME_GRID);
    }
    
    // 5. Draw Hemisphere Separator
    DrawLine(startX + (config->cols / 2) * cellW, startY, 
             startX + (config->cols / 2) * cellW, startY + drawHeight, Fade(THEME_TEXT, 0.3f));
}

// KI-Agent unterstützt: Pattern Definitions
typedef struct { int r; int c; } Point;

void PlacePattern(World *w, GameConfig *c, int startR, int startC, int type) {
    int team;
    int midCol = c->cols / 2;
    int *current_pop;
    
    // Determine Team based on Mouse Cursor (Start Position)
    if (startC < midCol) {
        team = TEAM_BLUE;
        current_pop = &c->current_blue_pop;
    } else {
        team = TEAM_RED;
        current_pop = &c->current_red_pop;
    }

    // Pattern Data
    // Glider (3x3)
    Point p_glider[] = {{0,1}, {1,2}, {2,0}, {2,1}, {2,2}};
    // HWSS (Traveler) (5x7)
    Point p_traveler[] = {{0,3}, {0,4}, {1,1}, {1,6}, {2,0}, {3,0}, {3,6}, {4,0}, {4,1}, {4,2}, {4,3}, {4,4}, {4,5}};
    // Gosper Glider Gun (Blaster) (9x36)
    Point p_blaster[] = {
        {4,0}, {5,0}, {4,1}, {5,1}, // Left Block
        {4,10}, {5,10}, {6,10}, {3,11}, {7,11}, {2,12}, {8,12}, {2,13}, {8,13}, {5,14}, {3,15}, {7,15}, {4,16}, {5,16}, {6,16}, {5,17}, // Left Mech
        {2,20}, {3,20}, {4,20}, {2,21}, {3,21}, {4,21}, {1,22}, {5,22}, {0,24}, {1,24}, {5,24}, {6,24}, // Right Mech
        {2,34}, {3,34}, {2,35}, {3,35} // Right Block
    };

    Point *cells = NULL;
    int count = 0;

    if (type == 1) { cells = p_glider; count = 5; }
    else if (type == 2) { cells = p_traveler; count = 13; }
    else if (type == 3) { cells = p_blaster; count = 36; }

    for (int i = 0; i < count; i++) {
        // Check Population Limit
        if (*current_pop >= c->max_population) break;

        // Calculate Wrap-around Coordinates
        int r = (startR + cells[i].r) % c->rows;
        int col = (startC + cells[i].c) % c->cols;
        
        // Handle negative modulo (if logic ever allows negative offsets)
        if (r < 0) r += c->rows;
        if (col < 0) col += c->cols;

        // Check Border Crossing (Clipping Rule)
        bool valid = false;
        if (team == TEAM_BLUE && col < midCol) valid = true;
        if (team == TEAM_RED && col >= midCol) valid = true;

        if (valid) {
            int idx = r * c->cols + col;
            if (w->grid[idx] == DEAD) {
                w->grid[idx] = team;
                (*current_pop)++;
            }
        }
    }
}

// Helper for continuous input handling (Key Repeat)
bool IsActionTriggered(int key) {
    static int activeKey = -1;
    static float timer = 0.0f;
    const float INITIAL_DELAY = 0.5f;
    const float REPEAT_INTERVAL = 0.05f; 

    if (IsKeyPressed(key)) {
        activeKey = key;
        timer = 0.0f;
        return true;
    }

    if (IsKeyDown(key)) {
        if (activeKey == key) {
            timer += GetFrameTime();
            if (timer >= INITIAL_DELAY + REPEAT_INTERVAL) {
                timer = INITIAL_DELAY; 
                return true;
            }
        }
    } else {
        if (activeKey == key) {
            activeKey = -1;
            timer = 0.0f;
        }
    }
    return false;
}

// KI-Agent unterstützt
void run_gui_app() {
    // Initial window size
    int screenWidth = 800;
    int screenHeight = 600;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE); 
    InitWindow(screenWidth, screenHeight, "Biotope - Game of Life");
    SetTargetFPS(60);

    // Initial state
    AppState state = STATE_CONFIG;
    GameConfig config = {
        .rows = 50, 
        .cols = 50, 
        .delay_ms = 100, 
        .max_population = 100, 
        .max_rounds = 1000,
        .current_red_pop = 0,
        .current_blue_pop = 0,
        .current_round = 0
    };
    
    // Feedback Message System
    char statusMsg[64] = "";
    float statusTimer = 0.0f;

    while (!WindowShouldClose()) {
        // Update dynamic screen dimensions
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        
        // Timer for status message
        if (statusTimer > 0) {
            statusTimer -= GetFrameTime();
            if (statusTimer <= 0) strcpy(statusMsg, "");
        }
        
        // --- Logic per State ---
        switch (state) {
            case STATE_CONFIG:
                // Interaction: Change Grid Size
                if (IsActionTriggered(KEY_RIGHT)) config.cols += 10;
                if (IsActionTriggered(KEY_LEFT) && config.cols > 10) config.cols -= 10;
                if (IsActionTriggered(KEY_UP)) config.rows += 10;
                if (IsActionTriggered(KEY_DOWN) && config.rows > 10) config.rows -= 10;
                
                // Interaction: Change Delay (incl. German Layout)
                if (IsActionTriggered(KEY_KP_ADD) || IsActionTriggered(KEY_EQUAL) || IsActionTriggered(KEY_RIGHT_BRACKET)) 
                    config.delay_ms += 50;
                if ((IsActionTriggered(KEY_KP_SUBTRACT) || IsActionTriggered(KEY_MINUS) || IsActionTriggered(KEY_SLASH)) && config.delay_ms > 0) 
                    config.delay_ms -= 50;

                // Interaction: Change Max Rounds
                if (IsActionTriggered(KEY_PAGE_UP)) config.max_rounds += 100;
                if (IsActionTriggered(KEY_PAGE_DOWN) && config.max_rounds > 100) config.max_rounds -= 100;

                // Interaction: Change Max Population
                int max_squad_cells = (config.rows * config.cols) / 2;
                // Clamp if grid size reduced below current max_pop
                if (config.max_population > max_squad_cells) config.max_population = max_squad_cells;

                if (IsActionTriggered(KEY_INSERT) && config.max_population < max_squad_cells) {
                    config.max_population += 10;
                    if (config.max_population > max_squad_cells) config.max_population = max_squad_cells;
                }
                if (IsActionTriggered(KEY_DELETE) && config.max_population > 10) config.max_population -= 10;

                // Transition: Start Setup
                if (IsKeyPressed(KEY_ENTER)) {
                    if (gui_world) free_world(gui_world);
                    gui_world = create_world(config.rows, config.cols);
                    // Initialize empty
                    for(int i=0; i<config.rows*config.cols; i++) gui_world->grid[i] = DEAD;
                    
                    config.current_blue_pop = 0;
                    config.current_red_pop = 0;
                    config.current_round = 0;
                    
                    state = STATE_EDIT;
                }
                break;

            case STATE_EDIT:
                // --- Mouse & Pattern Interaction ---
                {
                    Vector2 mousePos = GetMousePosition();
                    
                    // Constants must match DrawGridAndCells layout
                    const int headerHeight = 60;
                    const int footerHeight = 40;
                    const int margin = 20;
                    int drawWidth = screenWidth - (margin * 2);
                    int drawHeight = screenHeight - headerHeight - footerHeight - margin;
                    int startX = margin;
                    int startY = headerHeight;
                    
                    float cellW = (float)drawWidth / config.cols;
                    float cellH = (float)drawHeight / config.rows;
                    
                    // State for Drag-and-Paint interaction
                    static int editAction = 0; // 0:Idle, 1:Place, 2:Remove
                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) editAction = 0;

                    // Check if mouse is inside the grid area
                    if (mousePos.x >= startX && mousePos.x < startX + drawWidth &&
                        mousePos.y >= startY && mousePos.y < startY + drawHeight) {
                        
                        int col = (int)((mousePos.x - startX) / cellW);
                        int row = (int)((mousePos.y - startY) / cellH);
                        
                        // Handle Clicks (Single Cell) & Drag
                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            int index = row * config.cols + col;
                            // Determine action based on initial cell state: Place (1) or Remove (2)
                            if (gui_world->grid[index] == DEAD) editAction = 1;
                            else editAction = 2;
                        }

                        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && editAction != 0) {
                            int index = row * config.cols + col;
                            int midCol = config.cols / 2;

                            // Check Hemispheres and Population Limits
                            if (col < midCol) {
                                if (editAction == 2 && gui_world->grid[index] == TEAM_BLUE) {
                                    gui_world->grid[index] = DEAD;
                                    config.current_blue_pop--;
                                } else if (editAction == 1 && gui_world->grid[index] == DEAD && config.current_blue_pop < config.max_population) {
                                    gui_world->grid[index] = TEAM_BLUE;
                                    config.current_blue_pop++;
                                }
                            } else {
                                if (editAction == 2 && gui_world->grid[index] == TEAM_RED) {
                                    gui_world->grid[index] = DEAD;
                                    config.current_red_pop--;
                                } else if (editAction == 1 && gui_world->grid[index] == DEAD && config.current_red_pop < config.max_population) {
                                    gui_world->grid[index] = TEAM_RED;
                                    config.current_red_pop++;
                                }
                            }
                        }
                        
                        // Handle Patterns
                        if (IsKeyPressed(KEY_G)) {
                            PlacePattern(gui_world, &config, row, col, 1); // 1 = Glider
                            strcpy(statusMsg, "Deployed: GLIDER");
                            statusTimer = 2.0f;
                        }
                        if (IsKeyPressed(KEY_T)) {
                            PlacePattern(gui_world, &config, row, col, 2); // 2 = Traveler
                            strcpy(statusMsg, "Deployed: TRAVELER");
                            statusTimer = 2.0f;
                        }
                        if (IsKeyPressed(KEY_B)) {
                            PlacePattern(gui_world, &config, row, col, 3); // 3 = Blaster
                            strcpy(statusMsg, "Deployed: BLASTER");
                            statusTimer = 2.0f;
                        }
                    }
                }
                
                // File I/O
                if (IsKeyPressed(KEY_S)) {
                    if (save_grid("setup.bio", gui_world, &config)) {
                        strcpy(statusMsg, "Saved to setup.bio!");
                        statusTimer = 2.0f;
                    } else {
                        strcpy(statusMsg, "Save Failed!");
                        statusTimer = 2.0f;
                    }
                }
                
                // NEW: Load State Transition
                if (IsKeyPressed(KEY_L)) {
                    fileCount = list_protocol_files("biotope_results", &fileList);
                    selectedFileIndex = 0;
                    state = STATE_LOAD;
                }
                
                // KI-Agent unterstützt: Random Placement Logic
                if (IsKeyPressed(KEY_R)) {
                    // Reset grid
                    for(int i=0; i<config.rows*config.cols; i++) gui_world->grid[i] = DEAD;
                    config.current_blue_pop = 0;
                    config.current_red_pop = 0;
                    
                    int midCol = config.cols / 2;
                    // Seed random
                    srand(time(NULL));
                    
                    // Iterate and randomly fill
                    for(int r=0; r<config.rows; r++) {
                        for(int c=0; c<config.cols; c++) {
                            int idx = r * config.cols + c;
                            // 20% chance to be alive
                            if ((rand() % 100) < 20) {
                                if (c < midCol) {
                                    if (config.current_blue_pop < config.max_population) {
                                        gui_world->grid[idx] = TEAM_BLUE;
                                        config.current_blue_pop++;
                                    }
                                } else {
                                    if (config.current_red_pop < config.max_population) {
                                        gui_world->grid[idx] = TEAM_RED;
                                        config.current_red_pop++;
                                    }
                                }
                            }
                        }
                    }
                    strcpy(statusMsg, "Randomized Grid!");
                    statusTimer = 2.0f;
                }
                
                // Transition: Start Simulation with Auto-Save
                if (IsKeyPressed(KEY_ENTER)) {
                    // Phase 3: Auto-Save on Start
                    char autoFilename[128];
                    time_t now = time(NULL);
                    struct tm *t = localtime(&now);
                    strftime(autoFilename, sizeof(autoFilename), "biotope_results/run_%Y%m%d_%H%M%S.bio", t);
                    
                    // Store for result appending later
                    strcpy(currentProtocolFilename, autoFilename);
                    
                    if (save_grid(autoFilename, gui_world, &config)) {
                        printf("Auto-save successful: %s\n", autoFilename);
                    } else {
                        printf("Auto-save failed!\n");
                    }
                    
                    state = STATE_RUNNING;
                }
                break;

            case STATE_LOAD:
                if (IsKeyPressed(KEY_UP) && selectedFileIndex > 0) selectedFileIndex--;
                if (IsKeyPressed(KEY_DOWN) && selectedFileIndex < fileCount - 1) selectedFileIndex++;
                
                if (IsKeyPressed(KEY_ENTER) && fileCount > 0) {
                    if (load_grid(fileList[selectedFileIndex].filepath, gui_world, &config)) {
                        strcpy(statusMsg, "Protocol Loaded!");
                        statusTimer = 2.0f;
                    }
                    if (fileList) free(fileList);
                    fileList = NULL;
                    state = STATE_EDIT;
                }
                
                if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_ESCAPE)) {
                    if (fileList) free(fileList);
                    fileList = NULL;
                    state = STATE_EDIT;
                }
                break;
            
            case STATE_RUNNING:
                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_Q)) {
                    if (gui_world) free_world(gui_world);
                    gui_world = NULL;
                    state = STATE_CONFIG;
                    break;
                }

                // --- Simulation Logic ---
                static float timeAccumulator = 0.0f;
                timeAccumulator += GetFrameTime();
                
                if (timeAccumulator >= config.delay_ms / 1000.0f) {
                    timeAccumulator = 0.0f;
                    
                    World *next_gen = create_world(config.rows, config.cols);
                    update_generation(gui_world, next_gen, config.rows, config.cols);
                    free_world(gui_world);
                    gui_world = next_gen;
                    
                    config.current_round++;
                    
                    config.current_red_pop = 0;
                    config.current_blue_pop = 0;
                    for(int i=0; i<config.rows*config.cols; i++) {
                        if (gui_world->grid[i] == TEAM_RED) config.current_red_pop++;
                        if (gui_world->grid[i] == TEAM_BLUE) config.current_blue_pop++;
                    }
                    
                    if (config.current_round >= config.max_rounds || 
                        config.current_red_pop == 0 || 
                        config.current_blue_pop == 0) {
                        state = STATE_FINISHED;
                    }
                }
                break;
                
            case STATE_FINISHED:
                if (IsKeyPressed(KEY_ENTER)) {
                     state = STATE_GAME_OVER;
                     int winner = 0;
                     if (config.current_red_pop > config.current_blue_pop) winner = TEAM_RED;
                     else if (config.current_blue_pop > config.current_red_pop) winner = TEAM_BLUE;
                     
                     // Append to Protocol
                     if (strlen(currentProtocolFilename) > 0) {
                        append_protocol_result(currentProtocolFilename, winner, config.current_red_pop, config.current_blue_pop);
                     }
                }
                if (IsKeyPressed(KEY_Q)) {
                    if (gui_world) free_world(gui_world);
                    gui_world = NULL;
                    state = STATE_CONFIG;
                }
                break;

            case STATE_GAME_OVER:
                if (IsKeyPressed(KEY_ONE)) {
                     if (gui_world) free_world(gui_world);
                     gui_world = NULL;
                     state = STATE_CONFIG;
                }
                break;
        }

        // --- Drawing ---
        BeginDrawing();
        ClearBackground(THEME_BG);

        // Draw HUD Backgrounds (Header & Footer)
        DrawRectangle(0, 0, screenWidth, 60, THEME_HUD); // Header
        DrawRectangle(0, screenHeight - 40, screenWidth, 40, THEME_HUD); // Footer

        // Draw Status Message Overlay
        if (statusTimer > 0) {
            // KI-Agent unterstützt: Center status message to avoid collision with counters
            DrawText(statusMsg, screenWidth/2 - MeasureText(statusMsg, 20)/2, 20, 20, GREEN);
        }

        switch (state) {
            case STATE_CONFIG:
                DrawText("BIOTOPE CONFIGURATION", 20, 15, 30, THEME_TEXT);
                
                char buf[64];
                sprintf(buf, "GRID SIZE:  %03d x %03d", config.rows, config.cols);
                DrawText(buf, 40, 100, 20, THEME_BLUE);
                DrawText("(Arrows)", 300, 100, 18, DARKGRAY);
                
                sprintf(buf, "DELAY:      %04d ms", config.delay_ms);
                DrawText(buf, 40, 140, 20, THEME_RED);
                DrawText("(+/-)", 300, 140, 18, DARKGRAY);
                
                sprintf(buf, "MAX ROUNDS: %04d", config.max_rounds);
                DrawText(buf, 40, 180, 20, THEME_BLUE);
                DrawText("(PageUp/PageDown)", 300, 180, 18, DARKGRAY);
                
                sprintf(buf, "MAX INIT POP:    %04d", config.max_population);
                DrawText(buf, 40, 220, 20, THEME_RED);
                DrawText("(Insert/Delete)", 300, 220, 18, DARKGRAY);
                
                // KI-Agent unterstützt: Mission Protocol (Rules Display)
                int rulesX = screenWidth / 2 + 40;
                DrawLine(rulesX - 20, 100, rulesX - 20, 240, Fade(THEME_TEXT, 0.3f)); // Vertical Separator
                
                DrawText("CONWAY'S MISSION PROTOCOL", rulesX, 100, 20, THEME_HIGHLIGHT);
                DrawText("- SURVIVAL: 2 or 3 neighbors", rulesX, 135, 20, THEME_TEXT);
                DrawText("- BIRTH: 3 neighbors (Majority Rule of parents)", rulesX, 160, 20, THEME_TEXT);
                DrawText("- TEAMS: RED vs BLUE", rulesX, 185, 20, THEME_TEXT);
                DrawText("- GOAL: Max Population after timeout", rulesX, 210, 20, THEME_TEXT);

                DrawText("PRESS [ENTER] TO INITIALIZE SYSTEM", 40, 300, 20, THEME_HIGHLIGHT);
                break;

            case STATE_EDIT:
                DrawText("EDITOR MODE", 20, 18, 24, THEME_BLUE);
                // KI-Agent unterstützt: Increased font size to 16 for better readability
                DrawText("LEFT: BLUE SQUAD  |  RIGHT: RED SQUAD", 220, 24, 16, DARKGRAY);
                
                // Draw Population Counters
                char popBuf[64];
                sprintf(popBuf, "BLUE: %03d/%03d", config.current_blue_pop, config.max_population);
                DrawText(popBuf, screenWidth - 300, 20, 20, THEME_BLUE);
                sprintf(popBuf, "RED: %03d/%03d", config.current_red_pop, config.max_population);
                DrawText(popBuf, screenWidth - 140, 20, 20, THEME_RED);
                
                // Ghost Cursor (Visual Polish)
                Vector2 mousePos = GetMousePosition();
                // ... Re-calculate grid metrics for ghost cursor ...
                {
                     const int headerHeight = 60;
                    const int footerHeight = 40;
                    const int margin = 20;
                    int drawWidth = screenWidth - (margin * 2);
                    int drawHeight = screenHeight - headerHeight - footerHeight - margin;
                    int startX = margin;
                    int startY = headerHeight;
                    float cellW = (float)drawWidth / config.cols;
                    float cellH = (float)drawHeight / config.rows;
                    
                    if (mousePos.x >= startX && mousePos.x < startX + drawWidth &&
                        mousePos.y >= startY && mousePos.y < startY + drawHeight) {
                        int col = (int)((mousePos.x - startX) / cellW);
                        int row = (int)((mousePos.y - startY) / cellH);
                        // Draw Ghost
                        Color ghostColor = (col < config.cols/2) ? Fade(THEME_BLUE, 0.2f) : Fade(THEME_RED, 0.2f);
                        DrawRectangle(startX + col * cellW, startY + row * cellH, cellW, cellH, ghostColor);
                    }
                }
                
                // KI-Agent unterstützt: Draw grid lines only if grid is not too dense (> 150)
                bool showLines = (config.rows <= 150 && config.cols <= 150);
                DrawGridAndCells(&config, screenWidth, screenHeight, showLines); 

                // KI-Agent unterstützt: Updated Footer Menu Font Size to 14
                DrawText("[ENTER] RUN | [S] SAVE | [L] LOAD | [R] RANDOM | [G] GLIDER | [T] TRAVELER | [B] BLASTER", 
                         20, screenHeight - 28, 20, THEME_TEXT);
                break;

            case STATE_LOAD:
                DrawText("PROTOCOL ARCHIVE", 20, 15, 30, THEME_TEXT);
                DrawText("SELECT A SIMULATION RUN TO REPLAY", 400, 24, 16, DARKGRAY);

                if (fileCount == 0) {
                    DrawText("NO PROTOCOLS FOUND IN 'biotope_results/'", 40, 100, 20, THEME_RED);
                } else {
                    // Draw List
                    int startY = 100;
                    int itemHeight = 30;
                    int visibleItems = (screenHeight - 150) / itemHeight;
                    
                    // Simple scrolling view
                    int scrollOffset = 0;
                    if (selectedFileIndex >= visibleItems) scrollOffset = selectedFileIndex - visibleItems + 1;

                    for (int i = 0; i < visibleItems && (i + scrollOffset) < fileCount; i++) {
                        int idx = i + scrollOffset;
                        Color col = (idx == selectedFileIndex) ? THEME_BLUE : THEME_TEXT;
                        if (idx == selectedFileIndex) {
                            DrawRectangle(30, startY + i * itemHeight - 5, 400, itemHeight, THEME_HIGHLIGHT);
                            DrawText(">", 15, startY + i * itemHeight, 20, THEME_BLUE);
                        }
                        DrawText(fileList[idx].filename, 40, startY + i * itemHeight, 20, col);
                    }

                    // Draw Preview Panel
                    int previewX = 460;
                    DrawLine(previewX - 20, 100, previewX - 20, screenHeight - 60, Fade(THEME_TEXT, 0.3f));
                    
                    DrawText("PROTOCOL PREVIEW", previewX, 100, 20, THEME_HIGHLIGHT);
                    
                    ProtocolInfo *sel = &fileList[selectedFileIndex];
                    char infoBuf[128];
                    
                    if (sel->timestamp > 0) {
                        struct tm *t = localtime(&sel->timestamp);
                        strftime(infoBuf, sizeof(infoBuf), "DATE: %d.%m.%Y %H:%M:%S", t);
                        DrawText(infoBuf, previewX, 140, 20, THEME_TEXT);
                    } else {
                        DrawText("DATE: LEGACY FORMAT", previewX, 140, 20, DARKGRAY);
                    }
                    
                    sprintf(infoBuf, "GRID: %d x %d", sel->rows, sel->cols);
                    DrawText(infoBuf, previewX, 170, 20, THEME_TEXT);
                    
                    sprintf(infoBuf, "MAX ROUNDS: %d", sel->max_rounds);
                    DrawText(infoBuf, previewX, 200, 20, THEME_TEXT);
                    
                    sprintf(infoBuf, "MAX POPULATION: %d", sel->max_population);
                    DrawText(infoBuf, previewX, 230, 20, THEME_TEXT);
                    
                    if (sel->has_results) {
                        DrawLine(previewX - 10, 260, previewX + 250, 260, Fade(THEME_TEXT, 0.3f));
                        DrawText("RESULTS:", previewX, 270, 20, THEME_HIGHLIGHT);
                        
                        if (sel->winner == 1) DrawText("WINNER: RED", previewX, 300, 20, THEME_RED);
                        else if (sel->winner == 2) DrawText("WINNER: BLUE", previewX, 300, 20, THEME_BLUE);
                        else DrawText("WINNER: DRAW", previewX, 300, 20, DARKGRAY);
                        
                        char scoreBuf[64];
                        sprintf(scoreBuf, "R:%d  B:%d", sel->final_red, sel->final_blue);
                        DrawText(scoreBuf, previewX, 330, 20, THEME_TEXT);
                    } else {
                        DrawText("NO RESULTS YET", previewX, 270, 18, DARKGRAY);
                    }

                    DrawText("PRESS [ENTER] TO LOAD", previewX, 380, 20, GREEN);
                }

                DrawText("[UP/DOWN] NAVIGATE  |  [ENTER] LOAD  |  [Q/ESC] CANCEL", 20, screenHeight - 28, 20, THEME_TEXT);
                break;

            case STATE_RUNNING:
                DrawText("SIMULATION ACTIVE", 20, 18, 24, THEME_RED);
                
                // KI-Agent unterstützt: Stable positioning for Label and Counter
                const char* labelText = "ECO-BLOOM CYCLE:";
                char roundBuf[32];
                sprintf(roundBuf, "%04d / %04d", config.current_round, config.max_rounds);
                
                // Calculate widths based on a "worst-case" wide string to prevent jitter
                int maxCounterWidth = MeasureText("0000 / 0000", 20); 
                int labelWidth = MeasureText(labelText, 20);
                int gap = 10;
                int rightMargin = 20;
                
                // Draw Label (Fixed position relative to right edge)
                DrawText(labelText, screenWidth - rightMargin - maxCounterWidth - gap - labelWidth, 20, 20, THEME_TEXT);
                
                // Draw Counter (Fixed start position)
                DrawText(roundBuf, screenWidth - rightMargin - maxCounterWidth, 20, 20, THEME_TEXT);
                
                DrawGridAndCells(&config, screenWidth, screenHeight, false); // false = No Grid Lines (Performance!)
                
                DrawText("[Q] ABORT SIMULATION", 20, screenHeight - 30, 20, DARKGRAY);
                break;
                
            case STATE_FINISHED:
                DrawText("SIMULATION COMPLETED", 20, 18, 24, THEME_BLUE);
                
                DrawGridAndCells(&config, screenWidth, screenHeight, false);
                
                DrawText("[ENTER] VIEW RESULTS  |  [Q] MENU", 20, screenHeight - 30, 20, THEME_HIGHLIGHT);
                break;

            case STATE_GAME_OVER:
                DrawText("MISSION REPORT", screenWidth/2 - 100, 100, 30, THEME_TEXT);
                
                char resultBuf[128];
                Color winnerColor = THEME_TEXT;
                if (config.current_red_pop > config.current_blue_pop) {
                    sprintf(resultBuf, "WINNER: RED TEAM");
                    winnerColor = THEME_RED;
                } else if (config.current_blue_pop > config.current_red_pop) {
                    sprintf(resultBuf, "WINNER: BLUE TEAM");
                    winnerColor = THEME_BLUE;
                } else {
                    sprintf(resultBuf, "RESULT: DRAW");
                }
                
                DrawText(resultBuf, screenWidth/2 - MeasureText(resultBuf, 40)/2, 200, 40, winnerColor);
                
                sprintf(buf, "RED: %d  vs  BLUE: %d", config.current_red_pop, config.current_blue_pop);
                DrawText(buf, screenWidth/2 - MeasureText(buf, 20)/2, 260, 20, GRAY);
                
                DrawText("Stats exported to file.", screenWidth/2 - MeasureText("Stats exported to file.", 20)/2, 400, 20, DARKGRAY);
                DrawText("PRESS [1] TO RESTART SYSTEM", screenWidth/2 - MeasureText("PRESS [1] TO RESTART SYSTEM", 20)/2, 500, 20, THEME_HIGHLIGHT);
                break;
        }

        EndDrawing();
    }

    if (gui_world) free_world(gui_world);
    CloseWindow();
}
--- End of content ---

---

## MODEL ✨

Dieser Code-Schnipsel ist das Herzstück der Programmsteuerung innerhalb der Hauptschleife (Game Loop). Er entscheidet 60 Mal pro Sekunde, welcher Teil des Programms gerade aktiv sein soll.

**1. Was macht der Code?**
Er prüft den Inhalt der Variable `state` und verzweigt dann in den entsprechenden Code-Block.
*   Wenn `state` gerade auf `STATE_CONFIG` steht, wird der Code ausgeführt, der das Einstellungsmenü steuert (Tastatureingaben für Gittergröße, Geschwindigkeit usw.).
*   Wäre `state` z.B. auf `STATE_RUNNING`, würde ein anderer Block (weiter unten im Code) ausgeführt, der die Simulation berechnet.
Man nennt dies eine **State Machine (Zustandsautomat)**. Das Programm befindet sich immer in genau einem Zustand (z.B. Menü, Spiel, Pause) und verhält sich dementsprechend.

**2. Welche Schlüsselworte/Anweisungen enthält der Code?**
*   **`switch (...)`**: Eine Kontrollstruktur, die eine Variable auf verschiedene mögliche Werte prüft. Sie ist oft übersichtlicher als viele verschachtelte `if` / `else if` Anweisungen.
*   **`case ... :`**: Markiert den Einstiegspunkt für einen bestimmten Wert. Hier: "Im Fall, dass `state` gleich `STATE_CONFIG` ist, mach hier weiter".
*   (Nicht im Schnipsel, aber dazugehörig) **`break;`**: Am Ende eines `case`-Blocks steht meist ein `break`, um den `switch`-Block zu verlassen und nicht versehentlich in den nächsten Fall "hineinzufallen".

**3. Welche Struktur besitzt der Code?**
Der Code ist Teil einer **Mehrfachauswahl** innerhalb der `while`-Schleife.
Strukturell sieht das Ganze so aus:
1.  Prüfe Zustand (`switch`).
2.  Führe Logik für diesen Zustand aus (Tasten drücken, Werte ändern).
3.  Zeichne das Bild für diesen Zustand (weiter unten im Code beim `BeginDrawing`).

**4. Was gibt es zu den Variablen zu sagen?**
*   **`state`**: Diese Variable fungiert als das "Gedächtnis" des Programms. Sie merkt sich, wo sich der Benutzer gerade befindet.
*   **`STATE_CONFIG`**: Das ist eine Konstante (wahrscheinlich aus einem `enum` in `gui.h`), die für den Zustand "Konfigurationsmenü" steht. Durch solche sprechenden Namen wird der Code viel lesbarer als wenn dort einfach eine Zahl wie `0` oder `1` stünde.

**5. Was sonst noch zu erwähnen wäre ...**
Diese Trennung ist extrem wichtig für sauberen Code in der Spieleprogrammierung. Ohne diesen `switch` hättest du einen riesigen Haufen Code, in dem sich Logik für das Menü mit der Logik für die Simulation vermischt. So ist alles schön in "Schubladen" (Cases) sortiert.

---

## USER 🧑‍💻

Danke. Deine letzte Erklärung war sehr hilfreich. Ein Gedanke zur Optimierung der Ausführungsgeschwindigkeit in form einer Frage. Ist es nicht Zeitverschwendung, 60 mal in einer Sekunde alle möglichen 'state's zu prüfen, obwohl es klar ist, dass wenn das Spiel sich im state = STATE_RUNNING befindet es erst wieder einen anderen state geben kann, wenn eine bestimmte Bedingung für das Ende von STATE_RUNNING erreicht ist?

---

## MODEL ✨

Das ist eine **exzellente Frage**! Sie zeigt, dass du dir Gedanken über Effizienz machst – genau das, was ein Informatiker tun sollte.

Die kurze Antwort ist: **Nein, in diesem Fall ist es keine Zeitverschwendung, sondern architektonisch sogar notwendig.**

Hier ist die Erklärung für einen Erstsemester-Studenten, warum wir das so machen:

### 1. Das "Game Loop"-Prinzip (Die Illusion der Bewegung)
Ein Spiel (oder eine grafische Anwendung wie diese) funktioniert wie ein Daumenkino. Damit das Bild flüssig wirkt und das Fenster auf Eingaben (Maus, Tastatur) reagiert, **muss** die Schleife etwa 60 Mal pro Sekunde komplett durchlaufen werden.

Würden wir sagen: *"Bleib im `STATE_RUNNING` und mach dort eine eigene Endlosschleife, bis wir fertig sind"*, würde folgendes passieren:
*   Das Programm würde "einfrieren".
*   Das Betriebssystem (Windows) würde denken, das Programm sei abgestürzt ("Keine Rückmeldung"), weil die Hauptschleife keine Nachrichten mehr verarbeitet.
*   Du könntest das Spiel nicht abbrechen (z.B. mit 'Q'), weil die Tastaturabfrage nur einmal pro Frame ganz kurz passiert.

Wir **müssen** also jeden Frame kurz prüfen: *"Was ist zu tun?"*, es tun (z.B. einen Simulationsschritt rechnen oder nur zeichnen), und dann sofort die Kontrolle zurückgeben, damit das Bild neu gemalt werden kann.

### 2. Wie ein `switch` funktioniert (Compiler-Magie)
Du hast recht, dass viele `if-else`-Abfragen langsam wären. Aber ein `switch`-Statement in C ist sehr schlau optimiert:

*   **Der naive Weg (If-Else):**
    *   Ist es 1? Nein.
    *   Ist es 2? Nein.
    *   Ist es 3? Ja -> Führe Code aus.
*   **Der Switch-Weg (Sprungtabelle):**
    Der Compiler baut eine sogenannte **Jump Table**. Stell dir das wie ein Inhaltsverzeichnis vor. Der Prozessor schaut auf den Wert von `state` (z.B. 3) und springt **sofort** an die richtige Speicheradresse.
    Es ist also fast egal, ob du 5 oder 100 States hast – der Sprung dauert immer fast gleich kurz (wenige Nanosekunden). Das fällt bei 60 Frames pro Sekunde (16,6 Millisekunden pro Frame) überhaupt nicht ins Gewicht.

### 3. Zusammenfassung
Wir opfern diese winzigen Nanosekunden für die Prüfung des `state`, gewinnen dafür aber:
1.  **Reaktionsfähigkeit:** Das Programm kann jederzeit auf 'Q' oder Mausbewegungen reagieren.
2.  **Saubere Struktur:** Jeder Frame ist ein neues "Blatt Papier", das basierend auf dem aktuellen Zustand neu gemalt wird.

**Fazit:**
Deine Intuition ist richtig: *Unnötige Checks vermeiden*.
Aber in der **Ereignis-basierten Programmierung** (GUI, Spiele) ist der Zustandcheck der Preis, den wir für ein flüssiges, reagierendes Fenster zahlen.

---

## USER 🧑‍💻

Bitte erkläre
// --- Simulation Logic ---
                static float timeAccumulator = 0.0f;
                timeAccumulator += GetFrameTime();
                
                if (timeAccumulator >= config.delay_ms / 1000.0f) {
                    timeAccumulator = 0.0f;
                    
                    World *next_gen = create_world(config.rows, config.cols);
                    update_generation(gui_world, next_gen, config.rows, config.cols);
                    free_world(gui_world);
                    gui_world = next_gen;
                    
                    config.current_round++;
                    
                    config.current_red_pop = 0;
                    config.current_blue_pop = 0;
                    for(int i=0; i<config.rows*config.cols; i++) {
                        if (gui_world->grid[i] == TEAM_RED) config.current_red_pop++;
                        if (gui_world->grid[i] == TEAM_BLUE) config.current_blue_pop++;
                    }
                    
                    if (config.current_round >= config.max_rounds || 
                        config.current_red_pop == 0 || 
                        config.current_blue_pop == 0) {
                        state = STATE_FINISHED;
                    }
Ich verstehe, wie update_generation() funktioniert. Was mir nicht klar ist, ist wie die neu berechneten Werte aus next_gen in die aktuelle zu zeichnende Generation gelangt. Geschieht das hier in diesem Snippet in STATE_RUNNING, oder anderswo?

---

## MODEL ✨

Du hast den entscheidenden Punkt genau erfasst! Die Antwort lautet: **Ja, es passiert genau in diesem Snippet**, und zwar durch eine Technik, die man **Pointer-Swapping** nennt.

Hier ist die Erklärung für einen Erstsemester-Studenten:

### 1. Was macht der Code?
Dieser Code steuert den "Herzschlag" der Simulation. Er sorgt dafür, dass nicht bei jedem der 60 Grafik-Frames pro Sekunde eine neue Generation berechnet wird (das wäre viel zu schnell), sondern nur nach einer bestimmten Wartezeit (`delay_ms`). Wenn diese Zeit abgelaufen ist, wird die Welt aktualisiert.

### 2. Welche Schlüsselworte/Anweisungen enthält der Code?


---

## USER 🧑‍💻

A

---

## MODEL ✨

Hier ist die Fortsetzung der Erklärung für das Snippet in `STATE_RUNNING`. Deine Vermutung ist absolut richtig: Der "Transfer" passiert genau hier durch einen geschickten Trick mit Zeigern (Pointern).

**1. Was macht der Code?**
Der Code fungiert als Zeitwächter und Logistik-Manager. Er zählt die vergangene Zeit (`timeAccumulator`). Sobald genug Zeit verstrichen ist (das `delay_ms`), beauftragt er die Berechnung der nächsten Generation, löscht die alte Welt und macht die neue Welt zur "aktuellen" Welt.

**2. Welche Schlüsselworte/Anweisungen enthält der Code?**
*   **`static`**: Das ist extrem wichtig! Eine `static`-Variable innerhalb einer Funktion "vergisst" ihren Wert nicht, wenn die Funktion endet. Sie bleibt im Speicher bestehen. Ohne `static` würde der `timeAccumulator` 60 Mal pro Sekunde wieder bei 0.0 starten.
*   **`GetFrameTime()`**: Eine Funktion der Grafik-Bibliothek (raylib), die uns sagt, wie viele Millisekunden der letzte Frame (der letzte Durchlauf der Schleife) gedauert hat.
*   **`free_world(...)`**: Gibt den Speicher der alten Generation frei, um "Memory Leaks" (Speicherfresser) zu verhindern.

**3. Welche Struktur besitzt der Code?**
Es ist ein **Timer-gesteuerter Ablauf**:
1. Zeit aufsummieren.
2. Prüfen, ob das Limit erreicht ist.
3. Wenn ja: Berechnen -> Altes löschen -> **Pointer umbiegen** -> Statistiken zählen -> Prüfen, ob das Spiel vorbei ist.

**4. Was gibt es zu den Variablen zu sagen?**
*   **`timeAccumulator`**: Sammelt die Sekundenbruchteile, bis sie groß genug für einen Simulationsschritt sind.
*   **`next_gen`**: Ein lokaler Zeiger, der temporär die frisch berechnete Welt hält.
*   **`gui_world`**: Das ist die **globale Variable** (außerhalb der Funktion definiert). Sie ist der "Anker" für die grafische Darstellung.

**5. Die Antwort auf deine Frage: Das "Pointer-Swapping"**
Hier passiert die "Magie", wie die neuen Werte in die Anzeige gelangen:
1. `update_generation(gui_world, next_gen, ...)` berechnet aus der alten Welt (`gui_world`) die neue Welt (`next_gen`).
2. `free_world(gui_world)` löscht die alte Welt aus dem Arbeitsspeicher.
3. **`gui_world = next_gen;`**: Das ist die entscheidende Zeile. Der Zeiger `gui_world` (den die Zeichenfunktion `DrawGridAndCells` benutzt) zeigt jetzt einfach auf die Speicheradresse, an der die neuen Daten liegen.

**Warum funktioniert das?**
In der Hauptschleife wird nach dem `switch(state)`-Block immer `BeginDrawing()` und dann `DrawGridAndCells(...)` aufgerufen. Da du den Zeiger `gui_world` in dem Moment umgebogen hast, greift die Zeichenfunktion im nächsten Moment automatisch auf die neuen Daten zu. Das ist extrem effizient, weil keine großen Datenmengen kopiert werden müssen – man ändert nur eine Adresse im Speicher.

**Zusammenfassend:** Ja, der Transfer findet genau hier statt, indem du der grafischen Oberfläche sagst: "Ab jetzt ist das hier dein Spielfeld!".