## USER 🧑‍💻

This is the Gemini CLI. We are setting up the context for our chat.
Today's date is Samstag, 31. Januar 2026 (formatted according to the user's locale).
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
├───main_original
├───main_original.c
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
│   ├───gemini_protocol\
│   │   ├───biotop_260108.md
│   │   ├───biotop_260109.md
│   │   ├───biotop_260109a.md
│   │   ├───biotop_260109b.json
│   │   ├───biotop_260109b.md
│   │   ├───biotop_260109c.json
│   │   ├───biotop_260109c.md
│   │   ├───biotop_260117.json
│   │   ├───biotop_260117.md
│   │   ├───biotop_260123.json
│   │   └───biotop_260123.md
│   └───Präsentation\
│       ├───C_Performance_Architecture_Biotop.pdf
│       ├───Struktur.md
│       └───C_Performance_Architecture_Biotop (1).pdf\
│           ├───C_Performance_Architecture_Biotop-01.jpg
│           ├───C_Performance_Architecture_Biotop-02.jpg
│           ├───C_Performance_Architecture_Biotop-03.jpg
│           ├───C_Performance_Architecture_Biotop-04.jpg
│           ├───C_Performance_Architecture_Biotop-05.jpg
│           ├───C_Performance_Architecture_Biotop-06.jpg
│           ├───C_Performance_Architecture_Biotop-07.jpg
│           ├───C_Performance_Architecture_Biotop-08.jpg
│           ├───C_Performance_Architecture_Biotop-09.jpg
│           ├───C_Performance_Architecture_Biotop-10.jpg
│           ├───C_Performance_Architecture_Biotop-11.jpg
│           ├───C_Performance_Architecture_Biotop-12.jpg
│           ├───C_Performance_Architecture_Biotop-13.jpg
│           ├───C_Performance_Architecture_Biotop-14.jpg
│           └───C_Performance_Architecture_Biotop-15.jpg
└───utilities\

Reminder: Do not return an empty response when a tool call is required.

My setup is complete. I will provide my first command in the next turn.

---

## USER 🧑‍💻

Bitte erkläre // --- Theme Colors (Digital Lab) ---
// KI-Agent unterstützt: Sci-Fi / Retro Colors
const Color THEME_BG = { 20, 24, 32, 255 };        // Deep Dark Blue/Grey
const Color THEME_HUD = { 10, 12, 16, 230 };       // Semi-transparent Black
const Color THEME_GRID = { 40, 44, 52, 255 };      // Faint Grid Lines
const Color THEME_RED = { 255, 60, 100, 255 };     // Neon Red/Pink
const Color THEME_BLUE = { 0, 220, 255, 255 };     // Neon Cyan
const Color THEME_TEXT = { 220, 220, 220, 255 };   // Off-White
const Color THEME_HIGHLIGHT = { 255, 255, 255, 40 }; // Selection Glow
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
    InitWindow(screenWidth, screenHeight, "Biotope - Game of Life"); // Öffnet Startfenster - Header für Start-Window
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

    while (!WindowShouldClose()) {            // HIER geht's los! Game Loop/Hauptschleife "WindowShouldClose" = "x" oben-rechts in Win - Fenster
        // Update dynamic screen dimensions
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        
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

        EndDrawing();
    }

    if (gui_world) free_world(gui_world);
    CloseWindow();
}
--- End of content ---

---

## MODEL ✨

Absolut! Gerne, hier ist die Erklärung für den Code-Schnipsel, passend für einen Studienanfänger.

### 1. Was macht der Code?

Dieser Code-Block definiert eine feste Farbpalette für die grafische Benutzeroberfläche (GUI) des Programms. Man kann es sich wie einen digitalen "Farbkasten" vorstellen, bei dem jede Farbe einen festen Namen und einen Zweck hat. Anstatt an jeder Stelle im Programm die Farbwerte neu einzutippen, verwenden Programmierer diese vordefinierten Namen. Das sorgt für ein einheitliches Aussehen im gesamten Programm, hier im Stil eines "Sci-Fi / Retro" Designs.

### 2. Welche Schlüsselworte/Anweisungen enthält der Code?

*   `const`: Dies ist ein Schlüsselwort in C, das eine Variable als "Konstante" deklariert. Das bedeutet, dass der Wert der Variable nach ihrer ersten Zuweisung nicht mehr verändert werden kann. Das ist perfekt für Werte wie Design-Farben, die im gesamten Programmablauf gleich bleiben sollen.
*   `Color`: Dies ist der Datentyp. Es ist kein Standard-Typ wie `int` oder `char`, sondern eine benutzerdefinierte `struct` (Struktur), die von der Raylib-Bibliothek bereitgestellt wird. Eine solche `Color`-Struktur enthält typischerweise vier Werte: die Farbanteile für Rot, Grün, Blau und einen Alpha-Wert (Transparenz).
*   `= { ... };`: Dies ist die Initialisierung. Die Zahlen in den geschweiften Klammern werden den einzelnen Feldern der `Color`-Struktur zugewiesen, in der Regel in der Reihenfolge: `{ Rot, Grün, Blau, Alpha }`.

### 3. Welche Struktur besitzt der Code?

Der Code besteht aus einer Liste von Konstantendefinitionen. Jede Zeile folgt exakt dem gleichen Aufbau:
`const Datentyp VARIABLENNAME = { Wert1, Wert2, Wert3, Wert4 };`

Diese Gruppe von Definitionen wird durch einen Kommentar (`// --- Theme Colors ... ---`) eingeleitet, was die Lesbarkeit des Codes verbessert. Solche Konfigurationsblöcke stehen oft am Anfang einer Datei, damit man sie leicht finden und anpassen kann.

### 4. Was gibt es zu den Variablen zu sagen?

*   **Benennung:** Die Variablennamen wie `THEME_BG` oder `THEME_RED` sind in Großbuchstaben geschrieben. Das ist eine weit verbreitete Konvention, um sofort zu signalisieren, dass es sich um Konstanten handelt.
*   **Werte (RGBA):** Jede Variable speichert eine Farbe als eine Kombination aus vier Zahlenwerten von 0 bis 255.
    *   `THEME_RED = { 255, 60, 100, 255 }` bedeutet: viel Rot (255), wenig Grün (60), etwas Blau (100) und volle Deckkraft (255). Das ergibt den Neon-Rot/Pink-Farbton.
    *   `THEME_HIGHLIGHT = { 255, 255, 255, 40 }` ist interessant: Es ist Weiß (`255, 255, 255`), aber mit einem sehr niedrigen Alpha-Wert von `40`. Das macht die Farbe stark durchsichtig und erzeugt einen "Leuchteffekt", wenn sie über andere Elemente gezeichnet wird.

### 5. Was sonst noch zu erwähnen wäre ...

*   **Wartbarkeit:** Der größte Vorteil dieses Ansatzes ist die einfache Wartbarkeit. Wenn der Entwickler sich entscheidet, die Hintergrundfarbe zu ändern, muss er nur die eine Zeile `const Color THEME_BG = { ... };` anpassen. Die Änderung wirkt sich dann automatisch auf das gesamte Programm aus, ohne dass man dutzende Stellen im Code durchsuchen und ändern muss.
*   **Kommentare:** Die Kommentare (`// ...`) sind hier sehr nützlich. Sie beschreiben nicht nur, welche Farbe es ist (z.B. "Neon Cyan"), sondern auch ihren Zweck ("Faint Grid Lines" - schwache Gitterlinien). Das ist eine exzellente Programmierpraxis.
*   **KI-Hinweis:** Der Kommentar `// KI-Agent unterstützt:` deutet darauf hin, dass eine Künstliche Intelligenz bei der Auswahl oder Implementierung dieses Farbschemas geholfen hat.

---

## USER 🧑‍💻

Bitte erkläre alle Schlüsselworte, Funktionen, Methoden, Datentypen ud so weiter, die aus Raylib in @gui.c bereitgestellt werden.
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
    InitWindow(screenWidth, screenHeight, "Biotope - Game of Life"); // Öffnet Startfenster - Header für Start-Window
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

    while (!WindowShouldClose()) {            // HIER geht's los! Game Loop/Hauptschleife "WindowShouldClose" = "x" oben-rechts in Win - Fenster
        // Update dynamic screen dimensions
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        
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

        EndDrawing();
    }

    if (gui_world) free_world(gui_world);
    CloseWindow();
}
--- End of content ---

---

## MODEL ✨

Hallo! Na klar, hier ist eine detaillierte Aufschlüsselung aller Elemente aus der `raylib`-Bibliothek, die in der Datei `gui.c` verwendet werden. Die Erklärungen sind speziell für Studienanfänger aufbereitet.

### Grundlegende Struktur eines Raylib-Programms

Ein typisches Raylib-Programm hat eine einfache Struktur, die auch in `gui.c` zu sehen ist:

1.  **Initialisierung:** Mit `InitWindow()` wird ein Fenster erzeugt. Davor kann man mit `SetConfigFlags()` bestimmte Eigenschaften des Fensters festlegen.
2.  **Hauptschleife (Game Loop):** Eine `while`-Schleife, die so lange läuft, wie das Fenster offen ist (`!WindowShouldClose()`). In jedem Durchlauf dieser Schleife wird die Logik aktualisiert, Benutzereingaben verarbeitet und alles neu gezeichnet.
3.  **Zeichnen:** Innerhalb der Schleife beginnt das Zeichnen immer mit `BeginDrawing()` und endet mit `EndDrawing()`. Dazwischen werden alle Zeichenbefehle aufgerufen.
4.  **Aufräumen:** Nach der Schleife wird mit `CloseWindow()` alles wieder sauber beendet.

---

### Datentypen (Structs)

Das sind benutzerdefinierte Datentypen, die mehrere zusammengehörige Werte bündeln.

*   `Color`: Eine Struktur zum Speichern einer Farbe. Sie enthält vier `unsigned char`-Werte (0-255): `r` (Rot), `g` (Grün), `b` (Blau) und `a` (Alpha/Transparenz).
*   `Vector2`: Stellt einen Punkt oder Vektor in einem 2D-Raum dar. Enthält zwei `float`-Werte: `x` und `y`. Wird z.B. für die Mausposition verwendet.
*   `Rectangle`: Definiert ein Rechteck. Enthält vier `float`-Werte: `x`, `y` (die Position der oberen linken Ecke), sowie `width` und `height`.
*   `Image`: Repräsentiert ein Bild, das im Hauptspeicher des Computers (CPU) liegt. Es ist im Grunde ein Array von Pixeldaten, die man direkt bearbeiten kann.
*   `Texture2D`: Repräsentiert eine Textur, die in den Speicher der Grafikkarte (GPU) geladen wurde. Das Zeichnen von Texturen ist extrem schnell, weshalb Bilder vor dem Zeichnen in Texturen umgewandelt werden.

---

### Fenster- und Kontextverwaltung

Diese Funktionen steuern das Programmfenster selbst.

*   `SetConfigFlags(flags)`: Setzt Konfigurationseinstellungen *bevor* das Fenster erstellt wird. Im Code wird `FLAG_WINDOW_RESIZABLE` verwendet, damit der Benutzer die Größe des Fensters mit der Maus ändern kann.
*   `InitWindow(breite, höhe, titel)`: Erstellt ein neues Fenster mit der angegebenen Breite, Höhe und einem Titel in der Titelleiste.
*   `WindowShouldClose()`: Gibt `true` zurück, wenn der Benutzer versucht, das Fenster zu schließen (z.B. durch Klick auf das 'X' oder Drücken von ALT+F4). Dies ist die Hauptbedingung der Game-Loop.
*   `CloseWindow()`: Schließt das Fenster und gibt alle von Raylib belegten Ressourcen frei.
*   `SetTargetFPS(fps)`: Legt die maximale Anzahl an "Frames per Second" (Bilder pro Sekunde) fest, mit der die Hauptschleife laufen soll. `60` ist ein gängiger Wert für flüssige Animationen.
*   `GetScreenWidth()` / `GetScreenHeight()`: Geben die aktuelle Breite bzw. Höhe des Fensters zurück. Nützlich, wenn die Fenstergröße veränderbar ist.

---

### Hauptschleife (Game Loop)

Diese Funktionen definieren den "Herzschlag" der Anwendung.

*   `BeginDrawing()`: Signalisiert den Beginn einer "Zeichenrunde". Raylib bereitet intern alles vor, um neue Grafiken zu zeichnen (leert z.B. den Bildschirm).
*   `EndDrawing()`: Beendet die Zeichenrunde. Das fertig gezeichnete Bild wird nun auf dem Bildschirm angezeigt.
*   `ClearBackground(farbe)`: Füllt das gesamte Fenster mit einer einzigen Farbe. Dies ist meist der erste Zeichenbefehl nach `BeginDrawing()`.

---

### Input-Verarbeitung (Tastatur & Maus)

Funktionen, um auf Benutzereingaben zu reagieren.

*   `IsKeyPressed(taste)`: Gibt `true` zurück, aber nur in *genau dem einen Frame*, in dem eine Taste *gerade eben* heruntergedrückt wurde. Ideal für einmalige Aktionen wie Menü-Navigation.
*   `IsKeyDown(taste)`: Gibt `true` zurück, solange eine Taste *gedrückt gehalten* wird.
*   `GetMousePosition()`: Gibt die aktuelle X- und Y-Position des Mauszeigers als `Vector2` zurück.
*   `IsMouseButtonPressed(taste)`: Äquivalent zu `IsKeyPressed` für Maustasten (z.B. `MOUSE_LEFT_BUTTON`).
*   `IsMouseButtonDown(taste)`: Äquivalent zu `IsKeyDown` für Maustasten.
*   `IsMouseButtonReleased(taste)`: Gibt `true` zurück, in dem Frame, in dem eine Maustaste *losgelassen* wird.

---

### Zeit-Funktionen

*   `GetFrameTime()`: Gibt die Zeit in Sekunden zurück, die für die Berechnung und das Zeichnen des letzten Frames benötigt wurde (z.B. `0.01666` bei 60 FPS). Nützlich für zeitbasierte Animationen und Physik.

---

### Bild- & Textur-Management

Diese Funktionen sind Teil der optimierten Gitter-Darstellung.

*   `GenImageColor(breite, höhe, farbe)`: Erzeugt ein neues `Image`-Objekt im CPU-Speicher, das vollständig mit einer Farbe gefüllt ist.
*   `LoadTextureFromImage(image)`: Lädt die Daten aus einem `Image` (CPU) in eine `Texture2D` (GPU), damit sie schnell gezeichnet werden kann.
*   `UnloadImage(image)`: Gibt den Speicher eines `Image`-Objekts auf der CPU wieder frei.
*   `UnloadTexture(textur)`: Gibt den Speicher einer `Texture2D` auf der GPU wieder frei.
*   `SetTextureFilter(textur, filter)`: Legt fest, wie eine Textur aussieht, wenn sie vergrößert oder verkleinert wird. `TEXTURE_FILTER_POINT` sorgt für einen "pixeligen" Look ohne verschwommene Kanten, was für Pixel-Art-Spiele ideal ist.
*   `UpdateTexture(textur, daten)`: Aktualisiert eine bestehende Textur auf der GPU mit neuen Pixeldaten (einem `Color`-Array) von der CPU. Das ist sehr effizient, um Animationen darzustellen, ohne ständig neue Texturen erstellen zu müssen.
*   `DrawTexturePro(textur, sourceRect, destRect, ...)`: Eine sehr mächtige Funktion, um Texturen zu zeichnen. Sie erlaubt es, nur einen bestimmten Ausschnitt (`sourceRect`) einer Textur auf einen bestimmten Bereich des Bildschirms (`destRect`) zu zeichnen und dabei zu skalieren, zu drehen und einzufärben.

---

### Zeichen-Funktionen

Die Werkzeuge, um Formen und Text auf den Bildschirm zu bringen.

*   `DrawRectangle(x, y, breite, höhe, farbe)`: Zeichnet ein ausgefülltes Rechteck.
*   `DrawLine(startX, startY, endX, endY, farbe)`: Zeichnet eine einfache Linie.
*   `DrawText(text, x, y, schriftgröße, farbe)`: Zeichnet einen Text an der angegebenen Position.
*   `MeasureText(text, schriftgröße)`: Misst die Breite eines Textes in Pixeln, *ohne* ihn zu zeichnen. Sehr nützlich, um Text zu zentrieren.
*   `Fade(farbe, alpha)`: Nimmt eine bestehende Farbe und gibt eine neue `Color` zurück, bei der der Alpha-Wert (Transparenz) auf einen neuen Wert (0.0 bis 1.0) gesetzt ist.

---

### Vordefinierte Konstanten

Raylib bietet viele nützliche, vordefinierte Konstanten.

*   **Farben:** `BLANK` (transparentes Schwarz), `WHITE`, `GREEN`, `DARKGRAY`, `GRAY` sind vordefinierte `Color`-Werte.
*   **Tastatur-Codes:** `KEY_RIGHT`, `KEY_ENTER`, `KEY_G` etc. sind einfach Zahlen, die jeder Taste auf der Tastatur zugeordnet sind und die Lesbarkeit des Codes enorm verbessern.

---

## USER 🧑‍💻

Bitte erkläre 
// --- Simulation Logic ---
                static float timeAccumulator = 0.0f;
                timeAccumulator += GetFrameTime(); // Raylib Zeitsteuerung: Benötigte Zeit in Sek. für Berechnung u. Zeichnen  d. letzten Frames   
                
                if (timeAccumulator >= config.delay_ms / 1000.0f) {
                    timeAccumulator = 0.0f;

---

## MODEL ✨

Gerne, hier ist die Erklärung zu diesem wichtigen Code-Abschnitt.

### 1. Was macht der Code?

Dieser Code-Teil ist eine **zeitgesteuerte Bremse** für die Simulationslogik.

Stell dir vor, dein Computer ist so schnell, dass er tausende von Spielrunden pro Sekunde berechnen könnte. Das wäre für das menschliche Auge nur ein Flimmern. Das Ziel dieses Codes ist es, die Simulation auf eine kontrollierbare Geschwindigkeit zu drosseln (z.B. eine neue Generation alle 100 Millisekunden).

Er funktioniert wie eine Stoppuhr: Er misst die Zeit, die vergeht, und immer wenn ein bestimmtes Zeitintervall (die `delay_ms` aus der Konfiguration) erreicht ist, wird die nächste Generation des "Game of Life" berechnet und die Stoppuhr zurückgesetzt.

### 2. Welche Schlüsselworte/Anweisungen enthält der Code?

*   `static float timeAccumulator = 0.0f;`
    *   `static`: Dies ist hier das wichtigste Schlüsselwort. Es sorgt dafür, dass die Variable `timeAccumulator` ihren Wert zwischen den einzelnen Durchläufen der Hauptschleife **behält**. Sie wird nur ein einziges Mal am Anfang auf `0.0f` gesetzt und "erinnert" sich dann an ihren letzten Wert. Ohne `static` würde sie in jedem Frame neu auf 0 gesetzt und der Code würde nie funktionieren.
    *   `float`: Ein Datentyp für eine Gleitkommazahl (Dezimalzahl).
    *   `timeAccumulator`: Der Name bedeutet "Zeit-Sammler", was seine Aufgabe perfekt beschreibt.

*   `timeAccumulator += GetFrameTime();`
    *   `GetFrameTime()`: Dies ist eine Raylib-Funktion, die die exakte Zeit in Sekunden zurückgibt, die für den letzten Frame (einen Durchlauf der Hauptschleife) benötigt wurde. Das ist meist eine sehr kleine Zahl (z.B. 0.016 Sekunden bei 60 Frames pro Sekunde).
    *   `+=`: Dieser Operator addiert den kleinen Zeitwert von `GetFrameTime()` in jedem Frame auf den `timeAccumulator` drauf. Der "Zeit-Sammler" füllt sich also langsam auf.

*   `if (timeAccumulator >= config.delay_ms / 1000.0f)`
    *   Dies ist die entscheidende Abfrage. Sie prüft: "Ist die gesammelte Zeit (`timeAccumulator`) größer oder gleich der gewünschten Verzögerung?"
    *   `config.delay_ms / 1000.0f`: Die Variable `delay_ms` ist in Millisekunden. `GetFrameTime()` arbeitet aber in Sekunden. Hier wird der Wert aus Millisekunden in Sekunden umgerechnet (z.B. 100ms -> 0.1s).

*   `timeAccumulator = 0.0f;`
    *   Wenn die `if`-Bedingung wahr ist (also genug Zeit vergangen ist), wird der Code für die nächste Generation ausgeführt und der "Zeit-Sammler" wird wieder auf Null zurückgesetzt, um für das nächste Intervall von vorne zu zählen.

### 3. Welche Struktur besitzt der Code?

Es ist ein klassisches und sehr verbreitetes Muster in der Spiele- und Anwendungsentwicklung, das man "Akkumulieren und Prüfen" nennen könnte.
1.  **Akkumulieren:** In jedem Schleifendurchlauf wird ein kleiner Wert (hier: Zeit) auf eine Zählervariable addiert.
2.  **Prüfen:** In jedem Schleifendurchlauf wird geprüft, ob der Zähler einen Schwellenwert erreicht hat.
3.  **Aktion & Reset:** Wenn der Schwellenwert erreicht ist, wird eine Aktion ausgeführt und der Zähler zurückgesetzt.

### 4. Was gibt es zu den Variablen zu sagen?

*   `timeAccumulator`: Die zentrale Variable in diesem Mechanismus. Ihre einzige Aufgabe ist es, die Zeit zu sammeln. Das `static`-Attribut ist ihr wichtigstes Merkmal, da es ihr ein "Gedächtnis" über die Frames hinweg verleiht.
*   `config.delay_ms`: Eine externe Variable aus der Spielkonfiguration, die es dem Benutzer erlaubt, die Simulationsgeschwindigkeit dynamisch einzustellen.

### 5. Was sonst noch zu erwähnen wäre ...

Dies ist ein Beispiel für eine **"nicht-blockierende" Zeitsteuerung**. Das ist ein extrem wichtiges Konzept. Eine einfache Alternative wäre, eine Funktion wie `sleep(100)` aufzurufen. Das würde das *gesamte Programm* für 100 Millisekunden anhalten – es könnte in der Zeit keine Mauseingaben verarbeiten, nichts zeichnen und würde sich "einfroren" anfühlen.

Der hier gezeigte Ansatz ist viel eleganter: Das Programm läuft mit voller Geschwindigkeit weiter (z.B. 60 FPS), ist immer reaktiv, und die Simulationslogik wird einfach nur dann ausgeführt, wenn ihr "Zeitfenster" gekommen ist.

---

## USER 🧑‍💻

Bitte erkläre 
World* create_world(int rows, int cols) {
    World *w = malloc(sizeof(World));
    w->rows = rows;
    w->cols = cols;
    w->grid = malloc(rows * cols * sizeof(int));
    return w;
}
in @game_logic.c
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
        int val = rand() % 100; // Ganzzahlige Zufallszahlen zw. 0 und 100
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

Absolut! Diese Funktion ist ein Kernstück der dynamischen Speicherverwaltung in C. Hier ist die Erklärung:

### 1. Was macht der Code?

Diese Funktion ist eine "Fabrik" (oder ein "Konstruktor") für ein `World`-Objekt. Ihre Aufgabe ist es, alle notwendigen Datenstrukturen für eine neue Spielwelt im Speicher des Computers zu erstellen und zu initialisieren.

Man übergibt die gewünschte Größe (Anzahl der Zeilen und Spalten), und die Funktion liefert eine komplett vorbereitete, aber noch "leere" Welt zurück. Sie baut quasi das Gerüst für das Spielfeld.

### 2. Welche Schlüsselworte/Anweisungen enthält der Code?

*   `World*`: Dies ist der Rückgabetyp der Funktion. Das Sternchen `*` bedeutet, dass die Funktion nicht das `World`-Objekt selbst zurückgibt, sondern einen **Pointer** (Zeiger) darauf – also die *Speicheradresse*, unter der das Objekt zu finden ist.
*   `malloc()`: Steht für "memory allocation" (Speicher anfordern). Dies ist eine Standard-C-Funktion, die ein Stück rohen, uninitialisierten Speicher vom Betriebssystem anfordert. Der Speicher wird vom sogenannten **"Heap"** genommen, einem Speicherbereich für Daten, die auch nach dem Ende der Funktion weiterexistieren sollen.
*   `sizeof()`: Ein Operator, der die Größe eines Datentyps oder einer Variable in Bytes berechnet. `sizeof(World)` ermittelt also, wie viel Speicherplatz die `World`-Struktur selbst benötigt.
*   `w->rows = rows;`: Der Pfeil-Operator `->` wird verwendet, um auf ein Mitglied (eine Variable) einer Struktur zuzugreifen, wenn man nur einen Pointer auf die Struktur hat. Es ist eine Kurzform für `(*w).rows`. Hier werden die übergebenen `rows` und `cols` in der neu erstellten `World`-Struktur gespeichert.
*   `return w;`: Gibt den Pointer `w` (also die Speicheradresse der neuen Welt) an die aufrufende Funktion zurück.

### 3. Welche Struktur besitzt der Code?

Die Funktion folgt einem sehr typischen, zweistufigen Prozess zur Erstellung dynamischer Datenstrukturen in C:

1.  **Container anlegen:** Zuerst wird mit `malloc(sizeof(World))` Speicher für die "Hülle" bzw. den Container angelegt. Die `World`-Struktur enthält ja nur die Metadaten (`rows`, `cols`) und einen *Pointer* auf das eigentliche Spielfeld (`grid`).
2.  **Inhalt anlegen:** Danach wird mit `malloc(rows * cols * sizeof(int))` der Speicher für das eigentliche Spielfeld (ein riesiges Array von `int`s) angelegt. Die Größe wird dynamisch basierend auf den `rows` und `cols` berechnet.
3.  **Verknüpfen:** Die Adresse des neu angelegten Spielfelds wird in der `World`-Struktur gespeichert (`w->grid = ...`).
4.  **Zurückgeben:** Der Pointer auf den fertig konfigurierten Container wird zurückgegeben.

### 4. Was gibt es zu den Variablen zu sagen?

*   `World *w`: Eine lokale Pointer-Variable, die nur innerhalb der Funktion existiert. Sie dient als "Arbeitskopie", um die neue `World`-Struktur im Speicher zu bauen. Ihr Wert (die Speicheradresse) wird am Ende zurückgegeben.
*   `rows`, `cols`: Das sind die Eingabeparameter, die die Größe der zu erstellenden Welt bestimmen.

### 5. Was sonst noch zu erwähnen wäre ...

*   **Dynamische Speicherverwaltung:** Dies ist ein Paradebeispiel dafür. Die Größe der Welt ist nicht beim Kompilieren des Programms bekannt, sondern wird zur Laufzeit festgelegt. `malloc` ist das Werkzeug, um diesen zur Laufzeit benötigten Speicher anzufordern.
*   **Verantwortung & Memory Leaks (Speicherlecks):** Das ist extrem wichtig! Speicher, der mit `malloc` angefordert wird, wird **nicht** automatisch freigegeben. Die Funktion, die `create_world` aufruft, erhält die Verantwortung für diesen Speicher. Sie **muss** irgendwann die Gegen-Funktion `free_world(w)` aufrufen, um den Speicher wieder an das System zurückzugeben. Vergisst man das, entsteht ein **Speicherleck**: Das Programm belegt immer mehr Speicher, ohne ihn je freizugeben, und kann im schlimmsten Fall abstürzen. `create_world` und `free_world` sind also ein untrennbares Paar.

---

## USER 🧑‍💻

Bitte erkläre
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
in @gui.c. Berichte detailliert darüber, wie die aktualisierte Anzeige von gui_world auf dem Bildschirm erzeugt wird. Das erschließt sich mir nicht.
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
    
    Rectangle source = { 0.0f, 0.0f, (float)texW, (float)texH }; // Raylib Datentyp: Rectangle definiert Rechteck {x, y, Breite, Höhe}
    Rectangle dest = { (float)startX, (float)startY, (float)drawWidth, (float)drawHeight };
    Vector2 origin = { 0.0f, 0.0f };  //Raylib Datentyp: Vector2 stellt Punkt oder Vektor in 2D dar {x, y}
    
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
    InitWindow(screenWidth, screenHeight, "Biotope - Game of Life"); // Öffnet Startfenster - Header für Start-Window
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

    while (!WindowShouldClose()) {            // HIER geht's los! Game Loop/Hauptschleife "WindowShouldClose" = "x" oben-rechts in Win - Fenster
        // Update dynamic screen dimensions
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        
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

        EndDrawing();
    }

    if (gui_world) free_world(gui_world);
    CloseWindow();
}
--- End of content ---

---

## MODEL ✨

Selbstverständlich! Hier ist die detaillierte Erklärung für den `STATE_RUNNING`-Block, mit einem besonderen Fokus auf deine Frage, wie die `gui_world` auf den Bildschirm kommt.

### Allgemeine Erklärung: `case STATE_RUNNING`

Dieser Code-Block wird in jedem Frame ausgeführt, solange sich das Programm im Zustand `STATE_RUNNING` befindet – also, während die Simulation aktiv läuft.

Seine Aufgaben sind:

1.  **Titel anzeigen:** Mit `DrawText("SIMULATION ACTIVE", ...)` wird der Text oben links gezeichnet, um dem Benutzer zu zeigen, was gerade passiert.
2.  **Rundenzähler anzeigen:** Es wird ein Text wie "ECO-BLOOM CYCLE: 0015 / 1000" oben rechts angezeigt.
3.  **Das Spielfeld zeichnen:** Die Funktion `DrawGridAndCells(...)` wird aufgerufen. **Dies ist der entscheidende Teil für deine Frage.**
4.  **Hinweis anzeigen:** Unten wird ein Hinweis gezeichnet, wie man die Simulation abbrechen kann (`[Q] ABORT SIMULATION`).

### Das Problem des "flackernden" Zählers und seine Lösung

Bevor wir zum Spielfeld kommen, ein cleveres Detail bei der Anzeige des Rundenzählers:

*   **Das Problem:** Stell dir vor, der Zähler springt von Runde 9 zu Runde 10. Der Text "0009 / 1000" ist schmaler als "0010 / 1000". Wenn man den Text einfach rechtsbündig an den Bildschirmrand setzt, würde die Startposition des Textes bei jedem Sprung (z.B. von 9 auf 10, von 99 auf 100) leicht nach links "springen". Das nennt man "Jitter" und es sieht unprofessionell aus.
*   **Die Lösung:**
    1.  `int maxCounterWidth = MeasureText("0000 / 0000", 20);`
        Diese Zeile misst die Pixel-Breite des *breitestmöglichen* Textes, den der Zähler jemals anzeigen kann (hier wurde "0000 / 0000" als Referenz für einen 4-stelligen Zähler genommen). Das Ergebnis ist eine feste Pixelzahl, z.B. 120 Pixel.
    2.  `DrawText(labelText, screenWidth - rightMargin - maxCounterWidth - gap - labelWidth, ...)`
        Die Position des Labels ("ECO-BLOOM CYCLE:") und des Zählers wird nun basierend auf dieser *fixen maximalen Breite* berechnet.
    3.  **Ergebnis:** Egal ob "9" oder "1000" angezeigt wird, der für den Zähler reservierte Platz ist immer gleich breit. Dadurch bleiben das Label und der Zähler absolut stabil an ihrer Position, ohne zu flackern.

---

### Detailliert: Wie `gui_world` auf den Bildschirm kommt (via `DrawGridAndCells`)

Das ist der Kern deiner Frage und eine sehr clevere Optimierung. Ein naiver Ansatz wäre, für jede lebende Zelle ein `DrawRectangle()` aufzurufen. Bei einem großen Gitter (z.B. 1000x500 Zellen) wären das bis zu 500.000 einzelne Zeichenbefehle pro Frame – extrem langsam!

Stattdessen wird hier eine professionelle Technik namens **Texture-Based Rendering** verwendet. Der Prozess lässt sich in 3 Schritte unterteilen:

#### Schritt 1: Das Bild im CPU-Speicher "malen"

*   In `DrawGridAndCells` gibt es eine `static Color *pixels;` Variable. Das ist ein riesiges Array von `Color`-Werten im Hauptspeicher (CPU-Speicher). Die Größe des Arrays ist `rows * cols`, also genau ein Farb-Pixel für jede Zelle im Spielfeld.
*   Eine `for`-Schleife geht nun durch **jede einzelne Zelle** des `gui_world->grid`.
*   Für jede Zelle wird geprüft: Ist sie `TEAM_BLUE`, `TEAM_RED` oder `DEAD`?
*   Basierend auf dem Zustand wird die entsprechende Farbe (`THEME_BLUE`, `THEME_RED` oder `BLANK` für transparente, tote Zellen) an die passende Stelle im `pixels`-Array geschrieben.
*   **Wichtig:** Bis hierhin wurde noch *nichts* auf dem Bildschirm gezeichnet! Es wurde nur ein Bild in Form eines Farb-Arrays im CPU-Speicher vorbereitet.

#### Schritt 2: Das Bild zur Grafikkarte hochladen

*   In der Funktion gibt es auch eine `static Texture2D gridTex;`. Eine `Texture2D` ist ein spezielles Bildobjekt, das im extrem schnellen Speicher der Grafikkarte (GPU) lebt.
*   Jetzt kommt der Befehl `UpdateTexture(gridTex, pixels);`.
*   Dieser Befehl nimmt das gesamte, im CPU-Speicher vorbereitete `pixels`-Array und kopiert es in **einer einzigen, schnellen Operation** auf die Grafikkarte, wo es in der `gridTex`-Textur gespeichert wird.

#### Schritt 3: Die Textur auf den Bildschirm zeichnen

*   Zuletzt wird `DrawTexturePro(gridTex, source, dest, ...)` aufgerufen.
*   Dies ist der **einzige Zeichenbefehl** für das gesamte Spielfeld. Er sagt der Grafikkarte: "Nimm die Textur `gridTex`, die du jetzt in deinem Speicher hast, und zeichne sie auf den Bildschirm. Strecke oder stauche sie so, dass sie genau in den Zielbereich `dest` passt."
*   Die Grafikkarte ist auf genau diese Aufgabe (Texturen zeichnen) spezialisiert und erledigt das blitzschnell.

### Zusammenfassung des Tricks

Statt der Grafikkarte tausende von kleinen Befehlen ("zeichne hier ein kleines Rechteck", "zeichne dort ein kleines Rechteck") zu schicken, was sehr langsam ist, wird hier so gearbeitet:

1.  **CPU:** Bereitet das gesamte Bild des Spielfelds in einem Rutsch in einem einfachen Array vor.
2.  **CPU -> GPU:** Schickt das fertige Bild in einem großen Datenpaket an die Grafikkarte (`UpdateTexture`).
3.  **GPU:** Zeichnet dieses eine große Bild auf den Bildschirm (`DrawTexturePro`).

Dieser "Bulk-Transfer" ist um Größenordnungen performanter und der Grund, warum die Simulation auch bei großen Spielfeldern flüssig läuft. Der Parameter `false` in `DrawGridAndCells(..., false)` sorgt zusätzlich dafür, dass während der laufenden Simulation die Gitterlinien nicht gezeichnet werden, was noch ein kleines bisschen mehr Leistung spart.