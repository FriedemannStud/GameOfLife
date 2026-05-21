#include "raylib.h"
#include "gui.h"
#include "file_io.h" // KI-Agent unterstützt
#include <stdio.h>
#include <stdlib.h> // For abs
#include <time.h>   // For time()
#include <string.h> // For strncpy
#include <sys/stat.h> // For mkdir
#include <errno.h>    // For errno

#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

#ifndef PLATFORM_WEB
    #define MAX_GRID_SIZE 5000
#else
    #define MAX_GRID_SIZE 500
#endif

// Global World Pointer for GUI
World *gui_world = NULL;
World *swap_world = NULL;

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
const Color THEME_HINT = { 130, 140, 160, 255 };   // Muted Grey-Blue for hints (Arrows, +/-)
const Color THEME_ACCENT = { 180, 190, 210, 255 }; // Call to action / Secondary Header
const Color THEME_HIGHLIGHT = { 255, 255, 255, 40 }; // Selection Glow

// Helper to draw the grid (reused in multiple states)
// KI-Agent unterstützt: Optimized Texture-Based Rendering for VcXsrv performance
// --- NEW SHADER PIPELINE GLOBALS ---
static Shader biotopeShader;
static unsigned char *gpu_data_buffer = NULL;
static Camera2D observer_camera = { 0 };

// Texture Management (Moved to file scope for cleanup)
static Texture2D gridTex = { 0 };
static int texW = 0;
static int texH = 0;
static int lastDrawWidth = 0;
static int lastDrawHeight = 0;

// Ping-Pong Targets
static RenderTexture2D pingPongTarget[2] = { 0 };
static int pingPongIndex = 0;
static int locPrevFrame = -1;
static int locFadeRate = -1;
static bool useMetaballs = false;

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
    
    // --- 1. Resource Management ---
    
    // Check if grid size changed or not initialized
    if (config->cols != texW || config->rows != texH || drawWidth != lastDrawWidth || drawHeight != lastDrawHeight) {
        // Cleanup old resources
        if (gridTex.id > 0) UnloadTexture(gridTex);
        if (gpu_data_buffer) free(gpu_data_buffer);
        if (pingPongTarget[0].id > 0) UnloadRenderTexture(pingPongTarget[0]);
        if (pingPongTarget[1].id > 0) UnloadRenderTexture(pingPongTarget[1]);
        
        // Update dimensions
        texW = config->cols;
        texH = config->rows;
        lastDrawWidth = drawWidth;
        lastDrawHeight = drawHeight;
        
        // Allocate new resources
        gpu_data_buffer = (unsigned char*)malloc(texW * texH * sizeof(unsigned char));
        Image img = GenImageColor(texW, texH, BLANK); // Create empty image
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE); // Force 1-byte grayscale
        
        gridTex = LoadTextureFromImage(img);
        UnloadImage(img);
        
        // IMPORTANT: Point filtering ensures sharp pixels when scaled up
        SetTextureFilter(gridTex, TEXTURE_FILTER_POINT); 

        // Init Ping-Pong Targets
        pingPongTarget[0] = LoadRenderTexture(drawWidth, drawHeight);
        pingPongTarget[1] = LoadRenderTexture(drawWidth, drawHeight);

        // Clear both targets to background color
        BeginTextureMode(pingPongTarget[0]);
        ClearBackground(THEME_BG);
        EndTextureMode();
        BeginTextureMode(pingPongTarget[1]);
        ClearBackground(THEME_BG);
        EndTextureMode();

        // Get Shader Locations
        locPrevFrame = GetShaderLocation(biotopeShader, "previousFrame");
        locFadeRate = GetShaderLocation(biotopeShader, "fadeRate");
    }
    
    // --- 2. Update Pixel Data (CPU side) ---
    int stride = config->cols + 2;
    for (int r = 1; r <= config->rows; r++) {
        for (int c = 1; c <= config->cols; c++) {
            int gridIdx = r * stride + c;
            int pixelIdx = (r - 1) * config->cols + (c - 1);

            if (gui_world->grid[gridIdx] == TEAM_BLUE) {
                gpu_data_buffer[pixelIdx] = 127;
            } else if (gui_world->grid[gridIdx] == TEAM_RED) {
                gpu_data_buffer[pixelIdx] = 255;
            } else {
                gpu_data_buffer[pixelIdx] = 0; 
            }
        }
    }
    
    // --- 3. Upload to GPU & Draw ---
    UpdateTexture(gridTex, gpu_data_buffer);
    
    Rectangle source = { 0.0f, 0.0f, (float)texW, (float)texH };
    // We draw to the FBO at 0,0 with full width/height
    Rectangle fboDest = { 0.0f, 0.0f, (float)drawWidth, (float)drawHeight };
    Vector2 origin = { 0.0f, 0.0f };
    
    BeginTextureMode(pingPongTarget[pingPongIndex]);
        BeginShaderMode(biotopeShader);
        
        // Bind previous frame
        SetShaderValueTexture(biotopeShader, locPrevFrame, pingPongTarget[1 - pingPongIndex].texture);
        
        // Set uniforms
        float fade = 0.95f;
        SetShaderValue(biotopeShader, locFadeRate, &fade, SHADER_UNIFORM_FLOAT);
        
        DrawTexturePro(gridTex, source, fboDest, origin, 0.0f, WHITE);
        
        EndShaderMode();
    EndTextureMode();

    // Draw the current FBO to the screen
    // Note: y-axis is flipped in OpenGL textures when rendered to FBO
    Rectangle screenSource = { 0.0f, 0.0f, (float)drawWidth, -(float)drawHeight };
    Rectangle screenDest = { (float)startX, (float)startY, (float)drawWidth, (float)drawHeight };
    
    // Applying Camera2D for Observer Mode
    BeginMode2D(observer_camera);
        DrawTexturePro(pingPongTarget[pingPongIndex].texture, screenSource, screenDest, origin, 0.0f, WHITE);
    EndMode2D();

    // Swap buffers
    pingPongIndex = 1 - pingPongIndex;

    // --- 4. Draw Grid Lines (Optional - Overhead is low for lines) ---
    if (drawGridLines) {
        for (int i = 0; i <= config->cols; i++) DrawLine(startX + i * cellW, startY, startX + i * cellW, startY + drawHeight, THEME_GRID);
        for (int i = 0; i <= config->rows; i++) DrawLine(startX, startY + i * cellH, startX + drawWidth, startY + i * cellH, THEME_GRID);
    }
    
    // 5. Draw Hemisphere Separator
    int midCol = config->cols / 2;
    int midX = startX + midCol * cellW;
    BeginMode2D(observer_camera);
        DrawLine(midX, startY, midX, startY + drawHeight, Fade(THEME_TEXT, 0.3f));
    EndMode2D();
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
            int stride = c->cols + 2;
            int idx = (r + 1) * stride + (col + 1);
            if (w->grid[idx] == DEAD) {
                w->grid[idx] = team;
                (*current_pop)++;
                activate_chunk_at(w, r, col);
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

// Global state
static int screenWidth = 800;
static int screenHeight = 600;
static AppState state = STATE_PUZZLE;
static GameConfig config = {
    .rows = 50, 
    .cols = 50, 
    .delay_ms = 100, 
    .max_population = 100, 
    .max_rounds = 1000,
    .current_red_pop = 0,
    .current_blue_pop = 0,
    .current_round = 0,
    .history_red_pop = NULL,
    .history_blue_pop = NULL,
    .history_count = 0
};
static char statusMsg[64] = "";
static float statusTimer = 0.0f;
static double ignitionStartTime = 0.0;

void init_gui_app(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Biotope - Game of Life");

    // KI-Agent unterstützt: Ensure results directory exists
#ifdef _WIN32
    mkdir("biotope_results");
#else
    mkdir("biotope_results", 0777);
#endif

    // Observer Camera Init
    observer_camera.zoom = 1.0f;
    observer_camera.target = (Vector2){ 0, 0 };
    observer_camera.offset = (Vector2){ 0, 0 };
    observer_camera.rotation = 0.0f;

#ifndef PLATFORM_WEB
    SetTargetFPS(120);
    biotopeShader = LoadShader(0, "resources/shaders/biotope_base.fs");
#else
    // KI-Agent unterstützt: WASM Persistent Storage Setup
    EM_ASM({
        try {
            FS.mkdir('/biotope_results');
        } catch (e) {
            // Directory might already exist, ignore
        }
        FS.mount(FS.filesystems.IDBFS, {}, '/biotope_results');
        FS.syncfs(true, function(err) {
            if (err) console.error("IDBFS Sync Error:", err);
            else console.log("Biotope Archive Synced from IndexedDB");
        });
    });

    biotopeShader = LoadShader(0, "resources/shaders/biotope_base_web.fs");
#endif
}

void close_gui_app(void) {
    if (gui_world) free_world(gui_world);
    if (swap_world) free_world(swap_world);
    
    // Shader & Texture Cleanup
    if (biotopeShader.id > 0) UnloadShader(biotopeShader);
    if (gridTex.id > 0) UnloadTexture(gridTex);
    if (pingPongTarget[0].id > 0) UnloadRenderTexture(pingPongTarget[0]);
    if (pingPongTarget[1].id > 0) UnloadRenderTexture(pingPongTarget[1]);
    
    if (gpu_data_buffer) {
        free(gpu_data_buffer);
        gpu_data_buffer = NULL;
    }
    
    CloseWindow();
}

void UpdateDrawFrame(void) {
        screenWidth = GetScreenWidth(); // Raylib Fenstersteuerung: Gibt Fensterbreite zurück 
        screenHeight = GetScreenHeight(); // Raylib Fenstersteuerung: Gibt Fensterhöhe zurück
        
        // Timer for status message
        if (statusTimer > 0) {
            statusTimer -= GetFrameTime();
            if (statusTimer <= 0) strcpy(statusMsg, "");
        }

        // --- Global Interactions ---
        if (IsKeyPressed(KEY_M)) {
            useMetaballs = !useMetaballs;
            if (useMetaballs) strcpy(statusMsg, "METABALLS: ON");
            else strcpy(statusMsg, "METABALLS: OFF");
            statusTimer = 2.0f;
        }
        
        // --- Logic per State ---
        switch (state) {                 // Zustandsmaschine - Wert von State gibt Code-Block-Ausführung vor 
            case STATE_PUZZLE:
                if (IsKeyPressed(KEY_ENTER)) {
                    state = STATE_CONFIG;
                }
                break;
            case STATE_CONFIG:           // Spieleinstellungen mit Tasten im Fenster Biotope Configuration
                // Interaction: Change Grid Size
                if (IsActionTriggered(KEY_RIGHT) && config.cols < MAX_GRID_SIZE) config.cols += 10;
                if (IsActionTriggered(KEY_LEFT) && config.cols > 10) config.cols -= 10;
                if (IsActionTriggered(KEY_UP) && config.rows < MAX_GRID_SIZE) config.rows += 10;
                if (IsActionTriggered(KEY_DOWN) && config.rows > 10) config.rows -= 10;
                
                // Extra Clamp for Presets or other logic
                if (config.cols > MAX_GRID_SIZE) config.cols = MAX_GRID_SIZE;
                if (config.rows > MAX_GRID_SIZE) config.rows = MAX_GRID_SIZE;
                
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
                if (IsActionTriggered(KEY_ONE)) { // CONWAY'S CHESS
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
                if (IsActionTriggered(KEY_THREE)) { // TURING SANDBOX
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
                    if (swap_world) free_world(swap_world);
                    swap_world = create_world(config.rows, config.cols);
                    // Initialize empty (Corrected for ghost borders)
                    int stride = config.cols + 2;
                    for(int r=0; r < config.rows + 2; r++) {
                        for(int c=0; c < config.cols + 2; c++) {
                            gui_world->grid[r * stride + c] = DEAD;
                        }
                    }
                    
                    config.current_blue_pop = 0;
                    config.current_red_pop = 0;
                    config.current_round = 0;
                    config.red_catalyst_used = false;
                    config.blue_catalyst_used = false;
                    
                    state = STATE_EDIT_RED;
                }                break;

            case STATE_EDIT_RED:
            case STATE_EDIT_BLUE:
                {
                    Vector2 mousePos = GetMousePosition();
                    const int headerHeight = 60;
                    const int footerHeight = 40;
                    const int margin = 20;
                    int drawWidth = screenWidth - (margin * 2);
                    int drawHeight = screenHeight - headerHeight - footerHeight - margin;
                    int startX = margin;
                    int startY = headerHeight;
                    float cellW = (float)drawWidth / config.cols;
                    float cellH = (float)drawHeight / config.rows;
                    int midCol = config.cols / 2;
                    
                    static int editAction = 0; // 0:Idle, 1:Place, 2:Remove
                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) editAction = 0;

                    if (mousePos.x >= startX && mousePos.x < startX + drawWidth &&
                        mousePos.y >= startY && mousePos.y < startY + drawHeight) {
                        
                        int col = (int)((mousePos.x - startX) / cellW);
                        int row = (int)((mousePos.y - startY) / cellH);
                        int stride = config.cols + 2;
                        int index = (row + 1) * stride + (col + 1);
                        
                        bool isCorrectSide = (state == STATE_EDIT_RED) ? (col >= midCol) : (col < midCol);

                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && isCorrectSide) {
                            if (gui_world->grid[index] == DEAD) editAction = 1;
                            else editAction = 2;
                        }

                        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && editAction != 0 && isCorrectSide) {
                            if (state == STATE_EDIT_RED) {
                                if (editAction == 2 && gui_world->grid[index] == TEAM_RED) {
                                    gui_world->grid[index] = DEAD;
                                    config.current_red_pop--;
                                    activate_chunk_at(gui_world, row, col);
                                } else if (editAction == 1 && gui_world->grid[index] == DEAD && config.current_red_pop < config.max_population) {
                                    gui_world->grid[index] = TEAM_RED;
                                    config.current_red_pop++;
                                    activate_chunk_at(gui_world, row, col);
                                }
                            } else {
                                if (editAction == 2 && gui_world->grid[index] == TEAM_BLUE) {
                                    gui_world->grid[index] = DEAD;
                                    config.current_blue_pop--;
                                    activate_chunk_at(gui_world, row, col);
                                } else if (editAction == 1 && gui_world->grid[index] == DEAD && config.current_blue_pop < config.max_population) {
                                    gui_world->grid[index] = TEAM_BLUE;
                                    config.current_blue_pop++;
                                    activate_chunk_at(gui_world, row, col);
                                }
                            }
                        }
                        
                        if (isCorrectSide) {
                            if (IsKeyPressed(KEY_G)) PlacePattern(gui_world, &config, row, col, 1);
                            if (IsKeyPressed(KEY_T)) PlacePattern(gui_world, &config, row, col, 2);
                            if (IsKeyPressed(KEY_B)) PlacePattern(gui_world, &config, row, col, 3);
                        }
                    }
                }
                
                if (IsKeyPressed(KEY_S)) save_grid("setup.bio", gui_world, &config);
                
                if (IsKeyPressed(KEY_L)) {
                    fileCount = list_protocol_files("biotope_results", &fileList);
                    selectedFileIndex = 0;
                    state = STATE_LOAD;
                }
                
                if (IsKeyPressed(KEY_R)) {
                    // Randomize only current player's side
                    int stride = config.cols + 2;
                    int midCol = config.cols / 2;
                    srand(time(NULL));

                    // 1. Clear side
                    for(int r=0; r<config.rows; r++) {
                        for(int c=0; c<config.cols; c++) {
                            bool isCorrectSide = (state == STATE_EDIT_RED) ? (c >= midCol) : (c < midCol);
                            if (isCorrectSide) {
                                int idx = (r + 1) * stride + (c + 1);
                                if (gui_world->grid[idx] != DEAD) {
                                    if (gui_world->grid[idx] == TEAM_RED) config.current_red_pop--;
                                    else config.current_blue_pop--;
                                    gui_world->grid[idx] = DEAD;
                                    activate_chunk_at(gui_world, r, c);
                                }
                            }
                        }
                    }

                    // 2. Sprinkle cells: Exactly 37.5% of the total grid area
                    int totalCells = config.rows * config.cols;
                    int targetPop = (int)(totalCells * 0.375f);
                    if (targetPop < 1) targetPop = 1;

                    // Sync config max_population to this 3% for the UI counter
                    config.max_population = targetPop;

                    int *currentPop = (state == STATE_EDIT_RED) ? &config.current_red_pop : &config.current_blue_pop;
                    int team = (state == STATE_EDIT_RED) ? TEAM_RED : TEAM_BLUE;
                    int sideWidth = (state == STATE_EDIT_RED) ? (config.cols - midCol) : midCol;
                    int startCol = (state == STATE_EDIT_RED) ? midCol : 0;

                    // Safety: limit attempts
                    int attempts = 0;
                    int maxAttempts = targetPop * 10; 
                    while (*currentPop < targetPop && attempts < maxAttempts) {
                        int r = rand() % config.rows;
                        int c = startCol + (rand() % sideWidth);
                        int idx = (r + 1) * stride + (c + 1);
                        if (gui_world->grid[idx] == DEAD) {
                            gui_world->grid[idx] = team;
                            (*currentPop)++;
                            activate_chunk_at(gui_world, r, c);
                        }
                        attempts++;
                    }
                }
                
                if (IsKeyPressed(KEY_ENTER)) {
                    if (state == STATE_EDIT_RED) {
                        state = STATE_EDIT_BLUE;
                    } else {
                        // Auto-Save and Start
                        char autoFilename[128];
                        time_t now = time(NULL);
                        strftime(autoFilename, sizeof(autoFilename), "biotope_results/run_%Y%m%d_%H%M%S.bio", localtime(&now));
                        strcpy(currentProtocolFilename, autoFilename);
                        save_grid(autoFilename, gui_world, &config);
                        state = STATE_IGNITION;   
                    }
                }
                break;

            case STATE_IGNITION:
                {
                    if (ignitionStartTime == 0.0) {
                        ignitionStartTime = GetTime();
                        // Step 4.1: Allocate telemetry arrays
                        config.history_red_pop = (int*)malloc(config.max_rounds * sizeof(int));
                        config.history_blue_pop = (int*)malloc(config.max_rounds * sizeof(int));
                        config.history_count = 0;
                    }
                    
                    if (GetTime() - ignitionStartTime >= 3.0) {
                        ignitionStartTime = 0.0;
                        state = STATE_RUNNING;
                    }
                }
                break;

            case STATE_LOAD:   // Alte Spielkonfigurationen laden
                if (IsKeyPressed(KEY_UP) && selectedFileIndex > 0) selectedFileIndex--;
                if (IsKeyPressed(KEY_DOWN) && selectedFileIndex < fileCount - 1) selectedFileIndex++;
                
                if (IsKeyPressed(KEY_ENTER) && fileCount > 0) {
                    if (load_grid(fileList[selectedFileIndex].filepath, gui_world, &config)) {
                        strcpy(statusMsg, "Protocol Loaded!");
                        statusTimer = 2.0f;
                        
                        // FIX: Ensure swap_world matches the new dimensions!
                        // Otherwise -> Heap Corruption / Buffer Overflow in update_generation
                        if (swap_world) free_world(swap_world);
                        swap_world = create_world(config.rows, config.cols);
                        // Initialize swap_world to valid empty state (including borders)
                        int stride = config.cols + 2;
                        for(int i=0; i < (config.rows + 2) * stride; i++) swap_world->grid[i] = DEAD;
                    }
                    if (fileList) free(fileList);
                    fileList = NULL;
                    state = STATE_EDIT_RED;
                }
                
                if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_ESCAPE)) {
                    if (fileList) free(fileList);
                    fileList = NULL;
                    state = STATE_EDIT_RED;   
                }
                break;
            
            case STATE_RUNNING:  // Hier zurücklehnen und zuschauen
                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_Q)) {
                    if (gui_world) free_world(gui_world);
                    if (swap_world) free_world(swap_world);
                    gui_world = NULL;
                    swap_world = NULL;
                    state = STATE_CONFIG;
                    ignitionStartTime = 0.0;
                    break;
                }

                // Toggle Observer Mode
                if (IsKeyPressed(KEY_O)) {
                    state = STATE_OBSERVER;
                    observer_camera.zoom = 1.0f;
                    observer_camera.target = (Vector2){ 0, 0 };
                    observer_camera.offset = (Vector2){ 0, 0 };
                    strcpy(statusMsg, "OBSERVER MODE: ON");
                    statusTimer = 2.0f;
                }

                // --- Catalyst Interaction ---
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePos = GetMousePosition();
                    const int headerHeight = 60;
                    const int footerHeight = 40;
                    const int margin = 20;
                    int drawWidth = screenWidth - (margin * 2);
                    int drawHeight = screenHeight - headerHeight - footerHeight - margin;
                    int startX = margin;
                    int startY = headerHeight;
                    
                    if (mousePos.x >= startX && mousePos.x < startX + drawWidth &&
                        mousePos.y >= startY && mousePos.y < startY + drawHeight) {
                        
                        float cellW = (float)drawWidth / config.cols;
                        float cellH = (float)drawHeight / config.rows;
                        int col = (int)((mousePos.x - startX) / cellW);
                        int row = (int)((mousePos.y - startY) / cellH);
                        int midCol = config.cols / 2;

                        if (col >= midCol && !config.red_catalyst_used) {
                            apply_catalyst(gui_world, row + 1, col + 1);
                            config.red_catalyst_used = true;
                            strcpy(statusMsg, "RED CATALYST ACTIVATED!");
                            statusTimer = 2.0f;
                        } else if (col < midCol && !config.blue_catalyst_used) {
                            apply_catalyst(gui_world, row + 1, col + 1);
                            config.blue_catalyst_used = true;
                            strcpy(statusMsg, "BLUE CATALYST ACTIVATED!");
                            statusTimer = 2.0f;
                        }
                    }
                }

                // --- Simulation Logic ---
                static float timeAccumulator = 0.0f;
                timeAccumulator += GetFrameTime(); // Raylib Zeitsteuerung: Benötigte Zeit in Sek. für Berechnung u. Zeichnen  d. letzten Frames   
                // "Sammelt" die pro Frame verbrauchte Zeit, bis die in delay_ms vorgegebene Wartezeit angesammelt wurde, dann weiter.  
                if (timeAccumulator >= config.delay_ms / 1000.0f) {
                    timeAccumulator = 0.0f;
                    
                    update_generation(gui_world, swap_world, config.rows, config.cols, &config.current_red_pop, &config.current_blue_pop);
                    
                    // Step 4.2: Record Telemetry
                    if (config.history_count < config.max_rounds) {
                        config.history_red_pop[config.history_count] = config.current_red_pop;
                        config.history_blue_pop[config.history_count] = config.current_blue_pop;
                        config.history_count++;
                    }

                    // Pointer Swap (Double Buffering)
                    World *temp = gui_world;
                    gui_world = swap_world;
                    swap_world = temp;

                    config.current_round++;
                    
                    if (config.current_round >= config.max_rounds || // Fertig, wenn max_rounds erreicht
                        config.current_red_pop == 0 ||               // Fertig, wenn keine roten Zellen mehr 
                        config.current_blue_pop == 0) {              // Fertig, wenn keine blauen Zellen mehr
                        state = STATE_FINISHED;
                    }
                }
                break;

            case STATE_OBSERVER:
                // Exit Observer Mode
                if (IsKeyPressed(KEY_O) || IsKeyPressed(KEY_ESCAPE)) {
                    state = STATE_RUNNING;
                    strcpy(statusMsg, "OBSERVER MODE: OFF");
                    statusTimer = 2.0f;
                }

                // --- Camera Controls ---
                
                // Pan: Right Click + Drag
                if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
                    Vector2 delta = GetMouseDelta();
                    observer_camera.target.x -= delta.x / observer_camera.zoom;
                    observer_camera.target.y -= delta.y / observer_camera.zoom;
                }

                // Zoom: Mouse Wheel
                float wheel = GetMouseWheelMove();
                if (wheel != 0) {
                    Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), observer_camera);
                    observer_camera.offset = GetMousePosition();
                    observer_camera.target = mouseWorldPos;
                    const float zoomIncrement = 0.125f;
                    observer_camera.zoom += (wheel * zoomIncrement);
                    if (observer_camera.zoom < 0.125f) observer_camera.zoom = 0.125f;
                }

                // --- Simulation Logic (Shared with RUNNING) ---
                static float obsTimeAccumulator = 0.0f;
                obsTimeAccumulator += GetFrameTime();
                if (obsTimeAccumulator >= config.delay_ms / 1000.0f) {
                    obsTimeAccumulator = 0.0f;
                    
                    update_generation(gui_world, swap_world, config.rows, config.cols, &config.current_red_pop, &config.current_blue_pop);
                    
                    if (config.history_count < config.max_rounds) {
                        config.history_red_pop[config.history_count] = config.current_red_pop;
                        config.history_blue_pop[config.history_count] = config.current_blue_pop;
                        config.history_count++;
                    }

                    World *temp = gui_world;
                    gui_world = swap_world;
                    swap_world = temp;

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
                        append_protocol_result(currentProtocolFilename, &config, winner);
                     }                }
                if (IsKeyPressed(KEY_Q)) {
                    if (gui_world) free_world(gui_world);
                    gui_world = NULL;
                    // Step 4.1 Cleanup
                    if (config.history_red_pop) { free(config.history_red_pop); config.history_red_pop = NULL; }
                    if (config.history_blue_pop) { free(config.history_blue_pop); config.history_blue_pop = NULL; }
                    state = STATE_CONFIG;
                }
                break;

            case STATE_GAME_OVER:
                if (IsKeyPressed(KEY_ONE)) {
                     if (gui_world) free_world(gui_world);
                     gui_world = NULL;
                     // Step 4.1 Cleanup
                     if (config.history_red_pop) { free(config.history_red_pop); config.history_red_pop = NULL; }
                     if (config.history_blue_pop) { free(config.history_blue_pop); config.history_blue_pop = NULL; }
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
            case STATE_PUZZLE:
                DrawText("Tutorial Level 1", screenWidth/2 - MeasureText("Tutorial Level 1", 40)/2, screenHeight/2 - 20, 40, THEME_BLUE);
                DrawText("PRESS [ENTER] TO CONTINUE", screenWidth/2 - MeasureText("PRESS [ENTER] TO CONTINUE", 20)/2, screenHeight/2 + 40, 20, THEME_TEXT);
                break;
            case STATE_CONFIG:
                DrawText("BIOTOPE CONFIGURATION", 20, 15, 30, THEME_TEXT);
                
                char buf[64];
                sprintf(buf, "GRID SIZE:  %03d x %03d", config.rows, config.cols);
                DrawText(buf, 40, 100, 20, THEME_BLUE);
                DrawText("(Arrows)", 300, 100, 18, THEME_HINT);
                
                sprintf(buf, "DELAY:      %04d ms", config.delay_ms);
                DrawText(buf, 40, 140, 20, THEME_RED);
                DrawText("(+/-)", 300, 140, 18, THEME_HINT);
                
                sprintf(buf, "MAX ROUNDS: %04d", config.max_rounds);
                DrawText(buf, 40, 180, 20, THEME_BLUE);
                DrawText("(PageUp/PageDown)", 300, 180, 18, THEME_HINT);
                
                sprintf(buf, "MAX INIT POP:    %04d", config.max_population);
                DrawText(buf, 40, 220, 20, THEME_RED);
                DrawText("(Insert/Delete)", 300, 220, 18, THEME_HINT);


                
                // KI-Agent unterstützt: Mission Protocol (Rules Display)
                int rulesX = screenWidth / 2 + 40;
                DrawLine(rulesX - 20, 100, rulesX - 20, 240, Fade(THEME_TEXT, 0.3f)); // Vertical Separator
                
                DrawText("CONWAY'S MISSION PROTOCOL", rulesX, 100, 20, THEME_ACCENT);
                DrawText("- SURVIVAL: 2 or 3 neighbors", rulesX, 135, 20, THEME_TEXT);
                DrawText("- BIRTH: 3 neighbors (Majority Rule of parents)", rulesX, 160, 20, THEME_TEXT);
                DrawText("- TEAMS: RED vs BLUE", rulesX, 185, 20, THEME_TEXT);
                DrawText("- GOAL: Max Population after timeout", rulesX, 210, 20, THEME_TEXT);


                DrawText("PRESET", 40, 300, 20, THEME_ACCENT);
                DrawText("[1] CONWAY'S CHESS", 40, 335, 20, THEME_ACCENT);
                DrawText("[2] OUTER SPACE BATTLE", 40, 370, 20, THEME_ACCENT);
                DrawText("[3] TURING SANDBOX", 40, 405, 20, THEME_ACCENT);
                DrawText("PRESS [ENTER] TO INITIALIZE SYSTEM", 40, 475, 20, THEME_ACCENT);
                break;

            case STATE_EDIT_RED:
            case STATE_EDIT_BLUE:
                DrawText("EDITOR MODE", 20, 18, 24, THEME_BLUE);
                
                // Centered Scoreboard
                char bluePopBuf[64], redPopBuf[64];
                sprintf(bluePopBuf, "BLUE: %03d/%03d", config.current_blue_pop, config.max_population);
                sprintf(redPopBuf, "RED: %03d/%03d", config.current_red_pop, config.max_population);
                int blueW = MeasureText(bluePopBuf, 20);
                DrawText(bluePopBuf, screenWidth/2 - blueW - 20, 20, 20, THEME_BLUE);
                DrawText(redPopBuf, screenWidth/2 + 20, 20, 20, THEME_RED);

                // Right-aligned Instruction
                const char* inst = (state == STATE_EDIT_RED) ? "P1: RED SQUAD (RIGHT)" : "P2: BLUE SQUAD (LEFT)";
                DrawText(inst, screenWidth - MeasureText(inst, 16) - 20, 24, 16, (state == STATE_EDIT_RED) ? THEME_RED : THEME_BLUE);
                
                
                // Ghost Cursor (Visual Polish)
                Vector2 mousePos = GetMousePosition();
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
                        // Draw Ghost only if on correct side
                        bool isCorrectSide = (state == STATE_EDIT_RED) ? (col >= config.cols/2) : (col < config.cols/2);
                        if (isCorrectSide) {
                            Color ghostColor = (state == STATE_EDIT_BLUE) ? Fade(THEME_BLUE, 0.2f) : Fade(THEME_RED, 0.2f);
                            DrawRectangle(startX + col * cellW, startY + row * cellH, cellW, cellH, ghostColor);
                        }
                    }
                }
                
                bool showLines = (config.rows <= 150 && config.cols <= 150);
                DrawGridAndCells(&config, screenWidth, screenHeight, showLines); 

                DrawText("[ENTER] NEXT/DONE | [S] SAVE | [L] LOAD | [R] RANDOM | [G] GLIDER | [T] TRAVELER | [B] BLASTER", 
                         20, screenHeight - 28, 20, THEME_TEXT);
                break;

            case STATE_IGNITION:
                DrawText("SYSTEM IGNITION", 20, 18, 24, THEME_RED);
                DrawGridAndCells(&config, screenWidth, screenHeight, false);
                {
                    double elapsed = GetTime() - ignitionStartTime;
                    int countdown = 3 - (int)elapsed;
                    if (countdown < 1) countdown = 1;
                    char countBuf[16];
                    sprintf(countBuf, "%d", countdown);
                    DrawText(countBuf, screenWidth/2 - MeasureText(countBuf, 120)/2, screenHeight/2 - 60, 120, THEME_ACCENT);
                    DrawText("REVEALING BIOTOPE...", screenWidth/2 - MeasureText("REVEALING BIOTOPE...", 20)/2, screenHeight/2 + 60, 20, THEME_TEXT);
                }
                break;

            case STATE_LOAD:
                DrawText("PROTOCOL ARCHIVE", 20, 15, 30, THEME_TEXT);
                DrawText("SELECT A SIMULATION RUN TO REPLAY", 400, 24, 16, THEME_HINT);

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
                    
                    DrawText("PROTOCOL PREVIEW", previewX, 100, 20, THEME_ACCENT);
                    
                    ProtocolInfo *sel = &fileList[selectedFileIndex];
                    char infoBuf[128];
                    
                    if (sel->timestamp > 0) {
                        struct tm *t = localtime(&sel->timestamp);
                        strftime(infoBuf, sizeof(infoBuf), "DATE: %d.%m.%Y %H:%M:%S", t);
                        DrawText(infoBuf, previewX, 140, 20, THEME_TEXT);
                    } else {
                        DrawText("DATE: LEGACY FORMAT", previewX, 140, 20, THEME_HINT);
                    }
                    
                    sprintf(infoBuf, "GRID: %d x %d", sel->rows, sel->cols);
                    DrawText(infoBuf, previewX, 170, 20, THEME_TEXT);
                    
                    sprintf(infoBuf, "MAX ROUNDS: %d", sel->max_rounds);
                    DrawText(infoBuf, previewX, 200, 20, THEME_TEXT);
                    
                    sprintf(infoBuf, "MAX POPULATION: %d", sel->max_population);
                    DrawText(infoBuf, previewX, 230, 20, THEME_TEXT);
                    
                    if (sel->has_results) {
                        DrawLine(previewX - 10, 260, previewX + 250, 260, Fade(THEME_TEXT, 0.3f));
                        DrawText("RESULTS:", previewX, 270, 20, THEME_ACCENT);
                        
                        if (sel->winner == 1) DrawText("WINNER: RED", previewX, 300, 20, THEME_RED);
                        else if (sel->winner == 2) DrawText("WINNER: BLUE", previewX, 300, 20, THEME_BLUE);
                        else DrawText("WINNER: DRAW", previewX, 300, 20, THEME_HINT);
                        
                        char scoreBuf[64];
                        sprintf(scoreBuf, "R:%d  B:%d", sel->final_red, sel->final_blue);
                        DrawText(scoreBuf, previewX, 330, 20, THEME_TEXT);
                    } else {
                        DrawText("NO RESULTS YET", previewX, 270, 18, THEME_HINT);
                    }

                    DrawText("PRESS [ENTER] TO LOAD", previewX, 380, 20, GREEN);
                }

                DrawText("[UP/DOWN] NAVIGATE  |  [ENTER] LOAD  |  [Q/ESC] CANCEL", 20, screenHeight - 28, 20, THEME_TEXT);
                break;

            case STATE_OBSERVER:
            case STATE_RUNNING:
                DrawText("SIMULATION ACTIVE", 20, 18, 24, THEME_RED);
                
                if (state == STATE_OBSERVER) {
                    DrawText("(OBSERVER MODE)", 230, 22, 18, THEME_HINT);
                }

                // Centered Scoreboard (Vital for competitive feedback)
                char bluePopRun[32], redPopRun[32];
                sprintf(bluePopRun, "BLUE: %d", config.current_blue_pop);
                sprintf(redPopRun, "RED: %d", config.current_red_pop);
                int blueWRun = MeasureText(bluePopRun, 20);
                DrawText(bluePopRun, screenWidth/2 - blueWRun - 20, 20, 20, THEME_BLUE);
                DrawText(redPopRun, screenWidth/2 + 20, 20, 20, THEME_RED);

                // Right-aligned Round Counter (Compact)
                char roundBuf[32];
                sprintf(roundBuf, "CYCLE: %04d/%04d", config.current_round, config.max_rounds);
                int roundW = MeasureText(roundBuf, 20);
                DrawText(roundBuf, screenWidth - roundW - 20, 20, 20, THEME_TEXT);
                
                
                DrawGridAndCells(&config, screenWidth, screenHeight, false); // false = No Grid Lines (Performance!)
                
                // Catalyst Indicators
                DrawText("CATALYST:", 20, screenHeight - 65, 18, THEME_HINT);
                DrawText("BLUE", 120, screenHeight - 65, 18, config.blue_catalyst_used ? THEME_HINT : THEME_BLUE);
                DrawText("RED", 180, screenHeight - 65, 18, config.red_catalyst_used ? THEME_HINT : THEME_RED);

                if (state == STATE_RUNNING) {
                    DrawText("[Q] ABORT  |  [O] OBSERVER MODE", 20, screenHeight - 30, 20, THEME_HINT);
                } else {
                    DrawText("[MOUSE RIGHT] PAN  |  [WHEEL] ZOOM  |  [O] EXIT OBSERVER", 20, screenHeight - 30, 20, THEME_HINT);
                }
                break;
                
            case STATE_FINISHED:
                DrawText("SIMULATION COMPLETED", 20, 18, 24, THEME_BLUE);
                
                DrawGridAndCells(&config, screenWidth, screenHeight, false);
                
                DrawText("[ENTER] VIEW RESULTS  |  [Q] MENU", 20, screenHeight - 30, 20, THEME_ACCENT);
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
                
                // --- Step 4.3: Telemetry Graph ---
                int graphW = 400;
                int graphH = 100;
                int graphX = screenWidth/2 - graphW/2;
                int graphY = 300;
                DrawRectangle(graphX, graphY, graphW, graphH, THEME_HUD);
                DrawRectangleLines(graphX, graphY, graphW, graphH, THEME_HINT);

                if (config.history_count > 1) {
                    // Find Peak Population for Dynamic Scaling
                    int peakPop = 0;
                    for (int i = 0; i < config.history_count; i++) {
                        if (config.history_red_pop[i] > peakPop) peakPop = config.history_red_pop[i];
                        if (config.history_blue_pop[i] > peakPop) peakPop = config.history_blue_pop[i];
                    }
                    
                    // Safety: Avoid division by zero and add 10% margin
                    float yMax = (peakPop > 0) ? (float)peakPop * 1.1f : (float)(config.rows * config.cols);

                    for (int i = 0; i < config.history_count - 1; i++) {
                        float x1 = graphX + ((float)i / config.max_rounds) * graphW;
                        float x2 = graphX + ((float)(i + 1) / config.max_rounds) * graphW;
                        
                        // Scale Y using the dynamic peak
                        float y1_red = graphY + graphH - ((float)config.history_red_pop[i] / yMax) * graphH;
                        float y2_red = graphY + graphH - ((float)config.history_red_pop[i+1] / yMax) * graphH;
                        
                        float y1_blue = graphY + graphH - ((float)config.history_blue_pop[i] / yMax) * graphH;
                        float y2_blue = graphY + graphH - ((float)config.history_blue_pop[i+1] / yMax) * graphH;
                        
                        DrawLine(x1, y1_red, x2, y2_red, THEME_RED);
                        DrawLine(x1, y1_blue, x2, y2_blue, THEME_BLUE);
                    }
                }

                DrawText("Stats exported to file.", screenWidth/2 - MeasureText("Stats exported to file.", 20)/2, 420, 20, THEME_HINT);
                DrawText("PRESS [1] TO RESTART SYSTEM", screenWidth/2 - MeasureText("PRESS [1] TO RESTART SYSTEM", 20)/2, 500, 20, THEME_ACCENT);
                break;
        }

        EndDrawing(); // Raylib Anzeigesteuerung: Ende der "Zeichenrunde". Fertig gezeichnetes Bild wird im Fenster angezeigt.
}
