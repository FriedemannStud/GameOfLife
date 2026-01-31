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
const Color THEME_BG = { 20, 24, 32, 255 };        // Deep Dark Blue/Grey //Datentyp color = von Raylib zur Verfügung gestellt.
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
    static Texture2D gridTex = { 0 }; //Raylib Datentyp: Texture2D wird in GPU geladen zum schnellen Darstellung werden Bilder vor Zeichnen in Texturen umgeandelt 
    static int texW = 0;
    static int texH = 0;
    static Color *pixels = NULL;
    
    // Check if grid size changed or not initialized
    if (config->cols != texW || config->rows != texH) {
        // Cleanup old resources
        if (gridTex.id > 0) UnloadTexture(gridTex); // Raylib Bild&Textur Management: UnloadTexture = Gibt Textur-Objekt Speicherplatz (GPU) wieder frei
        if (pixels) free(pixels);
        
        // Update dimensions
        texW = config->cols;
        texH = config->rows;
        
        // Allocate new resources
        pixels = (Color*)malloc(texW * texH * sizeof(Color));
        Image img = GenImageColor(texW, texH, BLANK); // Create empty image
            // Raylib Datentyp: Image = Array von Pixeldaten (CPU), die bearbeitet werden können
            // Raylib Bild&Textur Management: GenImageColor = Erzeugt Image-Objekt (CPU), ganz mit einer Farbe gefüllt.
            // Raylib Konstante: BLANK = transparentes Schwarz
        gridTex = LoadTextureFromImage(img);
            // Raylib Bild&Textur Management: LoadTextureFromImage = Lädt Image (CPU) in Textur (GPU) - schnelles Zeichnen 
        UnloadImage(img);
            // Raylib Bild&Text Management: UnloadImage = Gibt Image-Objekt Speicherplatz (CPU) wieder frei
        
        // IMPORTANT: Point filtering ensures sharp pixels when scaled up
        SetTextureFilter(gridTex, TEXTURE_FILTER_POINT); 
            // Raylib Bild&Texture Management: SetTextureFilter = Wendet Filterbibliothek auf Textur (gridTex) an. TEXTURE_FILTER_POINT = "pxeliger" Look
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
    UpdateTexture(gridTex, pixels); // Raylib Bild&Textur Mangement: UpdateTexture = Aktualisiert Textur (GPU) mit Pixeldaten von Image (CPU)
    
    Rectangle source = { 0.0f, 0.0f, (float)texW, (float)texH }; // Raylib Datentyp: Rectangle definiert Rechteck {x, y, Breite, Höhe}
    Rectangle dest = { (float)startX, (float)startY, (float)drawWidth, (float)drawHeight };
    Vector2 origin = { 0.0f, 0.0f };  //Raylib Datentyp: Vector2 stellt Punkt oder Vektor in 2D dar {x, y}
    
    DrawTexturePro(gridTex, source, dest, origin, 0.0f, WHITE); // Raylib Bild&Textur Management: DrawTexturePro = Zeichnet Textur

    // --- 4. Draw Grid Lines (Optional - Overhead is low for lines) ---
    if (drawGridLines) {
        for (int i = 0; i <= config->cols; i++) DrawLine(startX + i * cellW, startY, startX + i * cellW, startY + drawHeight, THEME_GRID); // Raylib Zeichenfunktion: Linie zeichnen
        for (int i = 0; i <= config->rows; i++) DrawLine(startX, startY + i * cellH, startX + drawWidth, startY + i * cellH, THEME_GRID);
    }
    
    // 5. Draw Hemisphere Separator
    DrawLine(startX + (config->cols / 2) * cellW, startY, 
             startX + (config->cols / 2) * cellW, startY + drawHeight, Fade(THEME_TEXT, 0.3f)); // Raylib Zeichenfunktion: Fade = gibt Farbe mit neuem Apha-Wert zurück
}

// KI-Agent unterstützt: Pattern Definitions
typedef struct { int r; int c; } Point;

void PlacePattern(World *w, GameConfig *c, int startR, int startC, int type) {  
    // Platziert vordefinierte Muster auf World "w"
    // startR, startC = Startposition für Muster kommt von Mauspos
    // type = welches Muster platziert werden soll
    int team;
    int midCol = c->cols / 2; // Mittellinie
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

    if (IsKeyPressed(key)) { // Raylib Input-Steuerung: TRUE, wenn Taste 1 x gedrückt
        activeKey = key;
        timer = 0.0f;
        return true;
    }

    if (IsKeyDown(key)) {  // Raylib Input-Steuerung: TRUE, solange Taste gedrückt
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

    SetConfigFlags(FLAG_WINDOW_RESIZABLE); // Raylib Fenstersteuerung: Param. FLAG_WINDOW_REZISABLE -> Fenstergröße veränderbar
    InitWindow(screenWidth, screenHeight, "Biotope - Game of Life"); // Raylib Fenstersteuerung: Öffnet Startfenster - Header für Start-Window
    SetTargetFPS(60); // Raylib Fenstersteuerung: Legt Frames Per Second fest 

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

    while (!WindowShouldClose()) {      // HIER geht's los! Game Loop/Hauptschleife 
            //Raylib Fenstersteuerung "WindowShouldClose" = "x" oben-rechts in Win - Fenster
            // Update dynamic screen dimensions
        screenWidth = GetScreenWidth(); // Raylib Fenstersteuerung: Gibt Fensterbreite zurück 
        screenHeight = GetScreenHeight(); // Raylib Fenstersteuerung: Gibt Fensterhöhe zurück
        
        // Timer for status message
        if (statusTimer > 0) {
            statusTimer -= GetFrameTime();
            if (statusTimer <= 0) strcpy(statusMsg, "");
        }
        
        // --- Logic per State ---
        switch (state) {                 // Zustandsmaschine - Wert von State gibt Code-Block-Ausführung vor 
            case STATE_CONFIG:           // Spieleinstellungen mit Tasten im Fenster Biotope Configuration
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

                // Presets 
                if (IsActionTriggered(KEY_ONE)) { // Schach Modus
                    config.cols = 16;
                    config.rows = 8;
                    config.delay_ms = 500;
                    config.max_rounds = 50;
                    config.max_population = 30;
                }
                if (IsActionTriggered(KEY_TWO)) { // Outer Space Battle
                    config.cols = 400;
                    config.rows = 200;
                    config.delay_ms = 100;
                    config.max_rounds = 300;
                    config.max_population = 1000;
                }
                if (IsActionTriggered(KEY_THREE)) { // Von Neumann
                    config.cols = 1000;
                    config.rows = 500;
                    config.delay_ms = 0;
                    config.max_rounds = 1000;
                    config.max_population = 5000;
                }

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

            case STATE_EDIT:    // Startkonfiguration für Simulation auf Screen mit Maus setzen
                // --- Mouse & Pattern Interaction ---
                {
                    Vector2 mousePos = GetMousePosition();  // Raylib Input-Steuerung: Gibt aktuelle x,y-Mauspos. als Vector2 zurück 
                    
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
                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) editAction = 0; // Raylib Input-Steuerung: TRUE in dem Frame, in dem Maustaste losgelassen wird

                    // Check if mouse is inside the grid area
                    if (mousePos.x >= startX && mousePos.x < startX + drawWidth &&
                        mousePos.y >= startY && mousePos.y < startY + drawHeight) {
                        
                        int col = (int)((mousePos.x - startX) / cellW);
                        int row = (int)((mousePos.y - startY) / cellH);
                        
                        // Handle Clicks (Single Cell) & Drag
                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {  // Raylib Input-Steuerung: TRUE in dem Frame, in dem Maustaste gedrückt wird.
                            int index = row * config.cols + col;
                            // Determine action based on initial cell state: Place (1) or Remove (2)
                            if (gui_world->grid[index] == DEAD) editAction = 1;
                            else editAction = 2;
                        }

                        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && editAction != 0) {  // Raylib Input-Steuerung: TRUE, solange Maustaste gedrückt wird.
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
                    state = STATE_LOAD;  // Absprung alte Spiele laden
                }
                
                // KI-Agent unterstützt: Random Placement Logic
                if (IsKeyPressed(KEY_R)) {    // Spielfeld wird mit Zufallsmuster gefüllt
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
                
                // Transition: Start Simulation initiale Belegung wird gespeichert "....bio"
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

            case STATE_LOAD:   // Alte Spielkonfigurationen laden
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
            
            case STATE_RUNNING:  // Hier zurücklehnen und zuschauen
                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_Q)) {
                    if (gui_world) free_world(gui_world);
                    gui_world = NULL;
                    state = STATE_CONFIG;
                    break;
                }

                // --- Simulation Logic ---
                static float timeAccumulator = 0.0f;
                timeAccumulator += GetFrameTime(); // Raylib Zeitsteuerung: Benötigte Zeit in Sek. für Berechnung u. Zeichnen  d. letzten Frames   
                // "Sammelt" die pro Frame verbrauchte Zeit, bis die in delay_ms vorgegebene Wartezeit angesammelt wurde, dann weiter.  
                if (timeAccumulator >= config.delay_ms / 1000.0f) {
                    timeAccumulator = 0.0f;
                    
                    World *next_gen = create_world(config.rows, config.cols); // Def von create_world() in game_logic.c
                    update_generation(gui_world, next_gen, config.rows, config.cols);
                    free_world(gui_world);
                    gui_world = next_gen;
                    
                    config.current_round++;
                    
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
        BeginDrawing(); // Raylib Anzeigesteuerung: Beginn einer neuen "Zeichenrunde"
        ClearBackground(THEME_BG); // Raylib Anzeigesteuerung: Gesamtes Fenster wird mit THEME_BG gefüllt

        // Draw HUD Backgrounds (Header & Footer)
        DrawRectangle(0, 0, screenWidth, 60, THEME_HUD); // Header // Raylib Zeichenfunktion: Rechteck zeichnen
        DrawRectangle(0, screenHeight - 40, screenWidth, 40, THEME_HUD); // Footer

        // Draw Status Message Overlay
        if (statusTimer > 0) {
            // KI-Agent unterstützt: Center status message to avoid collision with counters
            DrawText(statusMsg, screenWidth/2 - MeasureText(statusMsg, 20)/2, 20, 20, GREEN); // Raylib Zeichenfunktion: Text zeichnen
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


                DrawText("PRESET", 40, 300, 20, THEME_HIGHLIGHT);
                DrawText("[1] CHESS", 40, 335, 20, THEME_HIGHLIGHT);
                DrawText("[2] OUTER SPACE BATTLE", 40, 370, 20, THEME_HIGHLIGHT);
                DrawText("[3] VON NEUMANN", 40, 405, 20, THEME_HIGHLIGHT);
                DrawText("PRESS [ENTER] TO INITIALIZE SYSTEM", 40, 475, 20, THEME_HIGHLIGHT);
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

        EndDrawing(); // Raylib Anzeigesteuerung: Ende der "Zeichenrunde". Fertig gezeichnetes Bild wird im Fenster angezeigt.
    }

    if (gui_world) free_world(gui_world);
    CloseWindow(); // Raylib Fenstersteuerung: Schließt Fenster und gibt alle Ressourcen frei
}
