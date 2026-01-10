#include "raylib.h"
#include "gui.h"
#include "file_io.h" // KI-Agent unterstützt
#include <stdio.h>
#include <stdlib.h> // For abs
#include <time.h>   // For time()
#include <string.h> // For strncpy

// Global World Pointer for GUI
World *gui_world = NULL;

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
void DrawGridAndCells(GameConfig *config, int screenWidth, int screenHeight, bool drawGridLines) {
    // ... (existing implementation) ...
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
    
    // 1. Draw Living Cells (with Padding for "Gap Grid" effect)
    // KI-Agent unterstützt: Optimization - Use padding instead of lines for simulation
    float pad = (cellW > 4 && cellH > 4) ? 1.0f : 0.0f; // Only pad if cells are big enough

    for (int r = 0; r < config->rows; r++) {
        for (int c = 0; c < config->cols; c++) {
            int index = r * config->cols + c;
            if (gui_world->grid[index] == TEAM_BLUE) {
                DrawRectangle(startX + c * cellW + pad, startY + r * cellH + pad, cellW - pad, cellH - pad, THEME_BLUE);
            } else if (gui_world->grid[index] == TEAM_RED) {
                DrawRectangle(startX + c * cellW + pad, startY + r * cellH + pad, cellW - pad, cellH - pad, THEME_RED);
            }
        }
    }

    // 2. Draw Grid Lines (Only if requested - e.g. Editor)
    if (drawGridLines) {
        for (int i = 0; i <= config->cols; i++) DrawLine(startX + i * cellW, startY, startX + i * cellW, startY + drawHeight, THEME_GRID);
        for (int i = 0; i <= config->rows; i++) DrawLine(startX, startY + i * cellH, startX + drawWidth, startY + i * cellH, THEME_GRID);
    }
    
    // 3. Draw Hemisphere Separator (Always visible but subtle)
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
                if (IsKeyPressed(KEY_RIGHT)) config.cols += 10;
                if (IsKeyPressed(KEY_LEFT) && config.cols > 10) config.cols -= 10;
                if (IsKeyPressed(KEY_UP)) config.rows += 10;
                if (IsKeyPressed(KEY_DOWN) && config.rows > 10) config.rows -= 10;
                
                // Interaction: Change Delay (incl. German Layout)
                if (IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_RIGHT_BRACKET)) 
                    config.delay_ms += 50;
                if ((IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_SLASH)) && config.delay_ms > 0) 
                    config.delay_ms -= 50;

                // Interaction: Change Max Rounds
                if (IsKeyPressed(KEY_PAGE_UP)) config.max_rounds += 100;
                if (IsKeyPressed(KEY_PAGE_DOWN) && config.max_rounds > 100) config.max_rounds -= 100;

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
                    
                    // Check if mouse is inside the grid area
                    if (mousePos.x >= startX && mousePos.x < startX + drawWidth &&
                        mousePos.y >= startY && mousePos.y < startY + drawHeight) {
                        
                        int col = (int)((mousePos.x - startX) / cellW);
                        int row = (int)((mousePos.y - startY) / cellH);
                        
                        // Handle Clicks (Single Cell)
                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            int index = row * config.cols + col;
                            int midCol = config.cols / 2;

                            // Check Hemispheres and Population Limits
                            if (col < midCol) {
                                if (gui_world->grid[index] == TEAM_BLUE) {
                                    gui_world->grid[index] = DEAD;
                                    config.current_blue_pop--;
                                } else if (gui_world->grid[index] == DEAD && config.current_blue_pop < config.max_population) {
                                    gui_world->grid[index] = TEAM_BLUE;
                                    config.current_blue_pop++;
                                }
                            } else {
                                if (gui_world->grid[index] == TEAM_RED) {
                                    gui_world->grid[index] = DEAD;
                                    config.current_red_pop--;
                                } else if (gui_world->grid[index] == DEAD && config.current_red_pop < config.max_population) {
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
                if (IsKeyPressed(KEY_L)) {
                    if (load_grid("setup.bio", gui_world, &config)) {
                        strcpy(statusMsg, "Loaded from setup.bio!");
                        statusTimer = 2.0f;
                    } else {
                        strcpy(statusMsg, "Load Failed");
                        statusTimer = 2.0f;
                    }
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
                
                // Transition: Start Simulation
                if (IsKeyPressed(KEY_ENTER)) {
                    state = STATE_RUNNING;
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
                     
                     char filename[64];
                     time_t t = time(NULL);
                     sprintf(filename, "biotope_results_%ld.md", t);
                     export_stats_md(filename, &config, winner);
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
                
                DrawGridAndCells(&config, screenWidth, screenHeight, true); // true = Draw Grid Lines

                // KI-Agent unterstützt: Updated Footer Menu Font Size to 14
                DrawText("[ENTER] RUN | [S] SAVE | [L] LOAD | [R] RANDOM | [G] GLIDER | [T] TRAVELER | [B] BLASTER", 
                         20, screenHeight - 28, 20, THEME_TEXT);
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