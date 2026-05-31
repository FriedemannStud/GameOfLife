#include "raylib.h"
#include "renderer.h"
#include "config.h"
#include "file_io.h" // KI-Agent unterstützt
#include "network_io.h"
#include "app_state_manager.h"
#include "game_logic.h"
#include <stdio.h>
#include <stdlib.h> // For abs
#include <time.h>   // For time()
#include <string.h> // For strncpy
#include <sys/stat.h> // For mkdir
#include <errno.h>    // For errno
#include <math.h>     // For sinf

#ifdef PLATFORM_WEB
    #include <emscripten/emscripten.h>
#endif

#ifndef PLATFORM_WEB
    #define MAX_GRID_SIZE 5000
#else
    #define MAX_GRID_SIZE 500
#endif

// Global World Pointer for GUI

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

// KI-Agent unterstützt: Single source of truth for all kiosk pixel geometry (ADR-0022)
// Pure function — no drawing, no Raylib calls, safe to call at any time.
// All HUD proportions are derived from screen size and match count so the layout
// scales correctly to any monitor resolution or match count.
KioskLayout compute_kiosk_layout(int screen_w, int screen_h, int match_count) {
    KioskLayout l = {0};
    if (match_count < 1) match_count = 1;

    // Near-square grid: ceil(sqrt(N)) columns, enough rows to fit all N matches.
    // Examples: 4 → 2×2,  6 → 3×2,  9 → 3×3,  11 → 4×3 (1 empty slot).
    l.grid_cols = (int)ceilf(sqrtf((float)match_count));
    l.grid_rows = (match_count + l.grid_cols - 1) / l.grid_cols;

    // Global chrome — fixed pixel heights (small enough for any ≥720p screen)
    l.top_bar_h      = 44;
    l.bottom_panel_h = 55;
    l.pad            = 12;
    l.separator_px   = 2;

    // Quadrant pixel size: fill available area after chrome and inter-quad padding.
    // Horizontal: pad on left, between every column, and on right → (cols+1) pads.
    // Vertical:   starts immediately below top bar, one pad between rows, no inner top pad.
    l.quad_w = (screen_w - l.pad * (l.grid_cols + 1)) / l.grid_cols;
    l.quad_h = (screen_h - l.top_bar_h - l.bottom_panel_h
                          - l.pad * (l.grid_rows - 1)) / l.grid_rows;

    // KI-Agent unterstützt: Proportional score bar — readable from 80 cm (ADR-0022 Phase B)
    l.score_bar_h = l.quad_h / 18;
    if (l.score_bar_h < 20) l.score_bar_h = 20;

    // KI-Agent unterstützt: Badge height for metric_reason label below name strip (ADR-0022 Phase B)
    // Compute badge first (based on fixed 26 px name row), then expand header_h.
    l.badge_h = (int)(26 * 0.85f);
    if (l.badge_h < 16) l.badge_h = 16;
    l.header_h = 26 + l.badge_h;  // combined: name row + badge row

    // KI-Agent unterstützt: Proportional thumbnail cell — visible from standing distance (ADR-0022 Phase B)
    // Divisor 40 → thumbnails occupy ~20% of quad height (8 cells × quad_h/40 = quad_h/5).
    l.seed_cell_px = l.quad_h / 40;
    if (l.seed_cell_px < 7) l.seed_cell_px = 7;

    // Font sizes
    l.font_name  = 16;
    l.font_badge = 12;
    l.font_score = 11;
    l.font_cta   = 20;

    return l;
}

// Helper to draw the grid (reused in multiple states)
// KI-Agent unterstützt: Optimized Texture-Based Rendering for VcXsrv performance
// --- NEW SHADER PIPELINE GLOBALS ---
static Shader biotopeShader;

// KI-Agent unterstützt: Initialize render context
void init_render_context(RenderContext *ctx, int cols, int rows, Rectangle bounds) {
    if (!ctx) return;
    ctx->viewport_bounds = bounds;
    ctx->tex_w = cols;
    ctx->tex_h = rows;
    ctx->last_draw_w = bounds.width;
    ctx->last_draw_h = bounds.height;
    
    ctx->pixel_buffer = (unsigned char*)malloc(cols * rows * sizeof(unsigned char));
    if (ctx->pixel_buffer) {
        memset(ctx->pixel_buffer, 0, cols * rows * sizeof(unsigned char));
    }
    
    Image img = GenImageColor(cols, rows, BLANK);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
    ctx->grid_texture = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(ctx->grid_texture, TEXTURE_FILTER_POINT);
    
    ctx->ping_pong_target[0] = LoadRenderTexture(bounds.width, bounds.height);
    ctx->ping_pong_target[1] = LoadRenderTexture(bounds.width, bounds.height);
    ctx->ping_pong_index = 0;
    
    // Theme Colors
    ctx->col_background = (Color){ 20, 24, 32, 255 };
    ctx->col_team_red = (Color){ 255, 60, 100, 255 };
    ctx->col_team_blue = (Color){ 0, 220, 255, 255 };
    
    BeginTextureMode(ctx->ping_pong_target[0]);
    ClearBackground(ctx->col_background);
    EndTextureMode();
    BeginTextureMode(ctx->ping_pong_target[1]);
    ClearBackground(ctx->col_background);
    EndTextureMode();
    
    ctx->loc_prev_frame = GetShaderLocation(biotopeShader, "previousFrame");
    ctx->loc_fade_rate = GetShaderLocation(biotopeShader, "fadeRate");
    ctx->use_metaballs = false;
    
    ctx->camera.zoom = 1.0f;
    ctx->camera.target = (Vector2){ 0, 0 };
    ctx->camera.offset = (Vector2){ 0, 0 };
    ctx->camera.rotation = 0.0f;
}

// KI-Agent unterstützt: Free render context resources safely
void free_render_context(RenderContext *ctx) {
    if (!ctx) return;
    if (ctx->grid_texture.id > 0) {
        UnloadTexture(ctx->grid_texture);
        ctx->grid_texture.id = 0;
    }
    if (ctx->pixel_buffer) {
        free(ctx->pixel_buffer);
        ctx->pixel_buffer = NULL;
    }
    if (ctx->ping_pong_target[0].id > 0) {
        UnloadRenderTexture(ctx->ping_pong_target[0]);
        ctx->ping_pong_target[0].id = 0;
    }
    if (ctx->ping_pong_target[1].id > 0) {
        UnloadRenderTexture(ctx->ping_pong_target[1]);
        ctx->ping_pong_target[1].id = 0;
    }
}

static RenderContext default_render_ctx;

// KI-Agent unterstützt: Clear accumulated shader trail from ping-pong buffers (ADR-0020)
// Called when returning to Kiosk mode to avoid stale fossil-trail artifacts
void clear_render_context_trail(RenderContext *ctx) {
    if (!ctx) return;
    if (ctx->ping_pong_target[0].id > 0) {
        BeginTextureMode(ctx->ping_pong_target[0]);
        ClearBackground(ctx->col_background);
        EndTextureMode();
    }
    if (ctx->ping_pong_target[1].id > 0) {
        BeginTextureMode(ctx->ping_pong_target[1]);
        ClearBackground(ctx->col_background);
        EndTextureMode();
    }
    ctx->ping_pong_index = 0;
}

// KI-Agent unterstützt: Draw grid and cells using RenderContext
// ADR-0020: Uses gui_world->rows/cols instead of config->rows/cols to ensure
// correct rendering when world dimensions differ from global config (e.g. Kiosk 50x50 vs. interactive 500x1000)
void DrawGridAndCellsCtx(RenderContext *r_ctx, const GameConfig *config, const World *gui_world, bool drawGridLines) {
    if (r_ctx == NULL || gui_world == NULL) return;
    (void)config; // KI-Agent unterstützt: config no longer used for dimensions (ADR-0020)

    int world_cols = gui_world->cols;
    int world_rows = gui_world->rows;

    int drawWidth = r_ctx->viewport_bounds.width;
    int drawHeight = r_ctx->viewport_bounds.height;
    int startX = r_ctx->viewport_bounds.x;
    int startY = r_ctx->viewport_bounds.y;
    
    float cellW = (float)drawWidth / world_cols;
    float cellH = (float)drawHeight / world_rows;
    
    // --- 1. Resource Management ---
    if (world_cols != r_ctx->tex_w || world_rows != r_ctx->tex_h || drawWidth != r_ctx->last_draw_w || drawHeight != r_ctx->last_draw_h) {
        // Cleanup old resources
        if (r_ctx->grid_texture.id > 0) UnloadTexture(r_ctx->grid_texture);
        if (r_ctx->pixel_buffer) free(r_ctx->pixel_buffer);
        if (r_ctx->ping_pong_target[0].id > 0) UnloadRenderTexture(r_ctx->ping_pong_target[0]);
        if (r_ctx->ping_pong_target[1].id > 0) UnloadRenderTexture(r_ctx->ping_pong_target[1]);
        
        // Update dimensions
        r_ctx->tex_w = world_cols;
        r_ctx->tex_h = world_rows;
        r_ctx->last_draw_w = drawWidth;
        r_ctx->last_draw_h = drawHeight;
        
        // Allocate new resources
        r_ctx->pixel_buffer = (unsigned char*)malloc(r_ctx->tex_w * r_ctx->tex_h * sizeof(unsigned char));
        if (r_ctx->pixel_buffer) {
            memset(r_ctx->pixel_buffer, 0, r_ctx->tex_w * r_ctx->tex_h * sizeof(unsigned char));
        }
        
        Image img = GenImageColor(r_ctx->tex_w, r_ctx->tex_h, BLANK);
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
        
        r_ctx->grid_texture = LoadTextureFromImage(img);
        UnloadImage(img);
        
        SetTextureFilter(r_ctx->grid_texture, TEXTURE_FILTER_POINT); 

        // Init Ping-Pong Targets
        r_ctx->ping_pong_target[0] = LoadRenderTexture(drawWidth, drawHeight);
        r_ctx->ping_pong_target[1] = LoadRenderTexture(drawWidth, drawHeight);

        // Clear both targets to background color
        BeginTextureMode(r_ctx->ping_pong_target[0]);
        ClearBackground(r_ctx->col_background);
        EndTextureMode();
        BeginTextureMode(r_ctx->ping_pong_target[1]);
        ClearBackground(r_ctx->col_background);
        EndTextureMode();

        // Get Shader Locations
        r_ctx->loc_prev_frame = GetShaderLocation(biotopeShader, "previousFrame");
        r_ctx->loc_fade_rate = GetShaderLocation(biotopeShader, "fadeRate");
    }
    
    // --- 2. Update Pixel Data (CPU side) ---
    int stride = world_cols + 2;
    for (int r = 1; r <= world_rows; r++) {
        for (int c = 1; c <= world_cols; c++) {
            int gridIdx = r * stride + c;
            int pixelIdx = (r - 1) * world_cols + (c - 1);

            if (gui_world->grid[gridIdx] == TEAM_BLUE) {
                r_ctx->pixel_buffer[pixelIdx] = 127;
            } else if (gui_world->grid[gridIdx] == TEAM_RED) {
                r_ctx->pixel_buffer[pixelIdx] = 255;
            } else {
                r_ctx->pixel_buffer[pixelIdx] = 0; 
            }
        }
    }
    
    // --- 3. Upload to GPU & Draw ---
    UpdateTexture(r_ctx->grid_texture, r_ctx->pixel_buffer);
    
    Rectangle source = { 0.0f, 0.0f, (float)r_ctx->tex_w, (float)r_ctx->tex_h };
    Rectangle fboDest = { 0.0f, 0.0f, (float)drawWidth, (float)drawHeight };
    Vector2 origin = { 0.0f, 0.0f };
    
    BeginTextureMode(r_ctx->ping_pong_target[r_ctx->ping_pong_index]);
        BeginShaderMode(biotopeShader);
        
        // Bind previous frame
        SetShaderValueTexture(biotopeShader, r_ctx->loc_prev_frame, r_ctx->ping_pong_target[1 - r_ctx->ping_pong_index].texture);
        
        // Set uniforms
        float fade = 0.95f;
        SetShaderValue(biotopeShader, r_ctx->loc_fade_rate, &fade, SHADER_UNIFORM_FLOAT);
        
        DrawTexturePro(r_ctx->grid_texture, source, fboDest, origin, 0.0f, WHITE);
        
        EndShaderMode();
    EndTextureMode();

    // Draw the current FBO to the screen
    Rectangle screenSource = { 0.0f, 0.0f, (float)drawWidth, -(float)drawHeight };
    Rectangle screenDest = { (float)startX, (float)startY, (float)drawWidth, (float)drawHeight };
    
    // Wrap rendering in Scissor Mode to prevent quadrant bleeding during pan/zoom
    BeginScissorMode(startX, startY, drawWidth, drawHeight);
    
    // Applying Camera2D for Observer Mode
    BeginMode2D(r_ctx->camera);
        DrawTexturePro(r_ctx->ping_pong_target[r_ctx->ping_pong_index].texture, screenSource, screenDest, origin, 0.0f, WHITE);
    EndMode2D();

    // Swap buffers
    r_ctx->ping_pong_index = 1 - r_ctx->ping_pong_index;

    // --- 4. Draw Grid Lines ---
    if (drawGridLines) {
        for (int i = 0; i <= world_cols; i++) DrawLine(startX + i * cellW, startY, startX + i * cellW, startY + drawHeight, THEME_GRID);
        for (int i = 0; i <= world_rows; i++) DrawLine(startX, startY + i * cellH, startX + drawWidth, startY + i * cellH, THEME_GRID);
    }
    
    // 5. Draw Hemisphere Separator
    int midCol = world_cols / 2;
    int midX = startX + midCol * cellW;
    BeginMode2D(r_ctx->camera);
        DrawLine(midX, startY, midX, startY + drawHeight, Fade(THEME_TEXT, 0.3f));
    EndMode2D();
    
    EndScissorMode();
}


// KI-Agent unterstützt: Wrapper for backward compatibility
void DrawGridAndCells(const GameConfig *config, const World *gui_world, int screenWidth, int screenHeight, bool drawGridLines) {
    const int headerHeight = 60;
    const int footerHeight = 40;
    const int margin = 20;
    int drawWidth = screenWidth - (margin * 2);
    int drawHeight = screenHeight - headerHeight - footerHeight - margin;
    
    default_render_ctx.viewport_bounds = (Rectangle){ margin, headerHeight, drawWidth, drawHeight };
    DrawGridAndCellsCtx(&default_render_ctx, config, gui_world, drawGridLines);
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



static char statusMsg[64] = "";
static float statusTimer = 0.0f;
// KI-Agent unterstützt: ignitionStartTime moved to app_state_manager.c (ADR-0020 Phase 1)

void init_renderer(int window_width, int window_height, const char* title) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(window_width, window_height, title);

    // KI-Agent unterstützt: Ensure results directory exists
#ifdef _WIN32
    mkdir("biotope_results");
#else
    mkdir("biotope_results", 0777);
#endif

#ifndef PLATFORM_WEB
    SetTargetFPS(120);
    biotopeShader = LoadShader(0, "assets/shaders/biotope_base.fs");
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

    biotopeShader = LoadShader(0, "assets/shaders/biotope_base_web.fs");
#endif

    // Initialize default fallback context
    init_render_context(&default_render_ctx, 50, 50, (Rectangle){ 20, 60, (float)window_width - 40, (float)window_height - 120 });
}

void close_renderer(void) {
    // Shader & Texture Cleanup
    if (biotopeShader.id > 0) UnloadShader(biotopeShader);
    
    free_render_context(&default_render_ctx);
    
    CloseWindow();
}

// KI-Agent unterstützt: Refactored to use SimulationContext* (ADR-0020)
AppState process_ui_events(AppState state, GameConfig* config, SimulationContext *sim_ctx, RenderContext *r_ctx, SessionOrigin *session_origin) {
    World* gui_world = sim_ctx->current_world;
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
        screenWidth = GetScreenWidth(); // Raylib Fenstersteuerung: Gibt Fensterbreite zurück 
        screenHeight = GetScreenHeight(); // Raylib Fenstersteuerung: Gibt Fensterhöhe zurück 
        
        // Timer for status message
        if (statusTimer > 0) {
            statusTimer -= GetFrameTime();
            if (statusTimer <= 0) strcpy(statusMsg, "");
        }

        // --- Global Interactions ---
        if (IsKeyPressed(KEY_M)) {
            r_ctx->use_metaballs = !r_ctx->use_metaballs;
            if (r_ctx->use_metaballs) strcpy(statusMsg, "METABALLS: ON");
            else strcpy(statusMsg, "METABALLS: OFF");
            statusTimer = 2.0f;
        }

        // KI-Agent unterstützt: Network Test Triggers
        if (IsKeyPressed(KEY_L)) {
            network_fetch_leaderboard_async();
            strcpy(statusMsg, "FETCHING LEADERBOARD...");
            statusTimer = 2.0f;
        }
        if (IsKeyPressed(KEY_H)) {
            network_fetch_highlights_async();
            strcpy(statusMsg, "FETCHING HIGHLIGHTS...");
            statusTimer = 2.0f;
        }

        // KI-Agent unterstützt: Web Editor Link (Phase 4.1)
        
        
        // --- Logic per State ---
        switch (state) {                 // Zustandsmaschine - Wert von State gibt Code-Block-Ausführung vor 
            case STATE_PUZZLE:
                if (IsKeyPressed(KEY_ENTER)) {
                    state = STATE_CONFIG;
                }
                break;
            case STATE_CONFIG:           // Spieleinstellungen mit Tasten im Fenster Biotope Configuration
                // Interaction: Change Grid Size
                if (IsActionTriggered(KEY_RIGHT) && config->cols < MAX_GRID_SIZE) config->cols += 10;
                if (IsActionTriggered(KEY_LEFT) && config->cols > 10) config->cols -= 10;
                if (IsActionTriggered(KEY_UP) && config->rows < MAX_GRID_SIZE) config->rows += 10;
                if (IsActionTriggered(KEY_DOWN) && config->rows > 10) config->rows -= 10;
                
                // Extra Clamp for Presets or other logic
                if (config->cols > MAX_GRID_SIZE) config->cols = MAX_GRID_SIZE;
                if (config->rows > MAX_GRID_SIZE) config->rows = MAX_GRID_SIZE;
                
                // Interaction: Change Delay (incl. German Layout)
                if (IsActionTriggered(KEY_KP_ADD) || IsActionTriggered(KEY_EQUAL) || IsActionTriggered(KEY_RIGHT_BRACKET)) 
                    config->delay_ms += 50;
                if ((IsActionTriggered(KEY_KP_SUBTRACT) || IsActionTriggered(KEY_MINUS) || IsActionTriggered(KEY_SLASH)) && config->delay_ms > 0) 
                    config->delay_ms -= 50;

                // Interaction: Change Max Rounds
                if (IsActionTriggered(KEY_PAGE_UP)) config->max_rounds += 100;
                if (IsActionTriggered(KEY_PAGE_DOWN) && config->max_rounds > 100) config->max_rounds -= 100;

                // Interaction: Change Max Population
                int max_squad_cells = (config->rows * config->cols) / 2;
                // Clamp if grid size reduced below current max_pop
                if (config->max_population > max_squad_cells) config->max_population = max_squad_cells;

                if (IsActionTriggered(KEY_INSERT) && config->max_population < max_squad_cells) {
                    config->max_population += 10;
                    if (config->max_population > max_squad_cells) config->max_population = max_squad_cells;
                }
                if (IsActionTriggered(KEY_DELETE) && config->max_population > 10) config->max_population -= 10;

                // Presets 
                if (IsActionTriggered(KEY_ONE)) { // CONWAY'S CHESS
                    config->cols = 16;
                    config->rows = 8;
                    config->delay_ms = 500;
                    config->max_rounds = 50;
                    config->max_population = 30;
                }
                if (IsActionTriggered(KEY_TWO)) { // Outer Space Battle
                    config->cols = 400;
                    config->rows = 200;
                    config->delay_ms = 100;
                    config->max_rounds = 300;
                    config->max_population = 1000;
                }
                if (IsActionTriggered(KEY_THREE)) { // TURING SANDBOX
                    config->cols = 1000;
                    config->rows = 500;
                    config->delay_ms = 0;
                    config->max_rounds = 1000;
                    config->max_population = 5000;
                }

                // Transition: Start Setup (ADR-0020 Phase 3: via SimulationContext)
                if (IsKeyPressed(KEY_ENTER)) {
                    // KI-Agent unterstützt: Allocate worlds via SimulationContext (ADR-0020)
                    reset_simulation_context(sim_ctx, config->rows, config->cols);
                    gui_world = sim_ctx->current_world;
                    if (session_origin) *session_origin = ORIGIN_INTERACTIVE;
                    
                    config->current_blue_pop = 0;
                    config->current_red_pop = 0;
                    config->current_round = 0;
                    
                    state = STATE_EDIT_RED;
                }
                if (IsKeyPressed(KEY_K)) {
                    state = STATE_KIOSK_MODE;
                    reset_kiosk_timers();
                }
                break;

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
                    float cellW = (float)drawWidth / config->cols;
                    float cellH = (float)drawHeight / config->rows;
                    int midCol = config->cols / 2;
                    
                    static int editAction = 0; // 0:Idle, 1:Place, 2:Remove
                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) editAction = 0;

                    if (mousePos.x >= startX && mousePos.x < startX + drawWidth &&
                        mousePos.y >= startY && mousePos.y < startY + drawHeight) {
                        
                        int col = (int)((mousePos.x - startX) / cellW);
                        int row = (int)((mousePos.y - startY) / cellH);
                        int stride = config->cols + 2;
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
                                    config->current_red_pop--;
                                    activate_chunk_at(gui_world, row, col);
                                } else if (editAction == 1 && gui_world->grid[index] == DEAD && config->current_red_pop < config->max_population) {
                                    gui_world->grid[index] = TEAM_RED;
                                    config->current_red_pop++;
                                    activate_chunk_at(gui_world, row, col);
                                }
                            } else {
                                if (editAction == 2 && gui_world->grid[index] == TEAM_BLUE) {
                                    gui_world->grid[index] = DEAD;
                                    config->current_blue_pop--;
                                    activate_chunk_at(gui_world, row, col);
                                } else if (editAction == 1 && gui_world->grid[index] == DEAD && config->current_blue_pop < config->max_population) {
                                    gui_world->grid[index] = TEAM_BLUE;
                                    config->current_blue_pop++;
                                    activate_chunk_at(gui_world, row, col);
                                }
                            }
                        }
                        
                        if (isCorrectSide) {
                            if (IsKeyPressed(KEY_G)) PlacePattern(gui_world, config, row, col, 1);
                            if (IsKeyPressed(KEY_T)) PlacePattern(gui_world, config, row, col, 2);
                            if (IsKeyPressed(KEY_B)) PlacePattern(gui_world, config, row, col, 3);
                        }
                    }
                }
                
                if (IsKeyPressed(KEY_L)) {
                    fileCount = list_protocol_files("biotope_results", &fileList);
                    selectedFileIndex = 0;
                    state = STATE_LOAD;
                }
                
                if (IsKeyPressed(KEY_R)) {
                    // Randomize only current player's side
                    int stride = config->cols + 2;
                    int midCol = config->cols / 2;
                    srand(time(NULL));

                    // 1. Clear side
                    for(int r=0; r<config->rows; r++) {
                        for(int c=0; c<config->cols; c++) {
                            bool isCorrectSide = (state == STATE_EDIT_RED) ? (c >= midCol) : (c < midCol);
                            if (isCorrectSide) {
                                int idx = (r + 1) * stride + (c + 1);
                                if (gui_world->grid[idx] != DEAD) {
                                    if (gui_world->grid[idx] == TEAM_RED) config->current_red_pop--;
                                    else config->current_blue_pop--;
                                    gui_world->grid[idx] = DEAD;
                                    activate_chunk_at(gui_world, r, c);
                                }
                            }
                        }
                    }

                    // 2. Sprinkle cells: Exactly 37.5% of the total grid area
                    int totalCells = config->rows * config->cols;
                    int targetPop = (int)(totalCells * 0.375f);
                    if (targetPop < 1) targetPop = 1;

                    // Sync config max_population to this 3% for the UI counter
                    config->max_population = targetPop;

                    int *currentPop = (state == STATE_EDIT_RED) ? &config->current_red_pop : &config->current_blue_pop;
                    int team = (state == STATE_EDIT_RED) ? TEAM_RED : TEAM_BLUE;
                    int sideWidth = (state == STATE_EDIT_RED) ? (config->cols - midCol) : midCol;
                    int startCol = (state == STATE_EDIT_RED) ? midCol : 0;

                    // Safety: limit attempts
                    int attempts = 0;
                    int maxAttempts = targetPop * 10; 
                    while (*currentPop < targetPop && attempts < maxAttempts) {
                        int r = rand() % config->rows;
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
                        // Auto-Save and Start (ADR-0020 Phase 3: use sim_ctx->current_world)
                        char autoFilename[128];
                        time_t now = time(NULL);
                        strftime(autoFilename, sizeof(autoFilename), "biotope_results/run_%Y%m%d_%H%M%S.json", localtime(&now));
                        strcpy(currentProtocolFilename, autoFilename);
                        // KI-Agent unterstützt: Save via SimulationContext (ADR-0020)
                        save_grid(autoFilename, sim_ctx->current_world, config);
                        state = STATE_IGNITION;
                    }
                }
                break;

            case STATE_IGNITION:
                // KI-Agent unterstützt: Use accessor (ADR-0020)
                if (get_ignition_start_time() == 0.0) set_ignition_start_time(GetTime());
                break;

            case STATE_LOAD:   // Alte Spielkonfigurationen laden
                if (IsKeyPressed(KEY_UP) && selectedFileIndex > 0) selectedFileIndex--;
                if (IsKeyPressed(KEY_DOWN) && selectedFileIndex < fileCount - 1) selectedFileIndex++;
                
                if (IsKeyPressed(KEY_ENTER) && fileCount > 0) {
                    // KI-Agent unterstützt: Load via SimulationContext (ADR-0020 Phase 3.3)
                    if (load_grid(fileList[selectedFileIndex].filepath, sim_ctx->current_world, config)) {
                        strcpy(statusMsg, "Protocol Loaded!");
                        statusTimer = 2.0f;
                        
                        // Rebuild swap world via context
                        if (sim_ctx->world_b) free_world(sim_ctx->world_b);
                        sim_ctx->world_b = create_world(config->rows, config->cols);
                        sim_ctx->next_world = sim_ctx->world_b;
                        gui_world = sim_ctx->current_world;
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
                    // KI-Agent unterstützt: Session-origin-aware routing (ADR-0020 Phase 5)
                    cleanup_interactive_session(sim_ctx, config, r_ctx);
                    set_ignition_start_time(0.0);
                    if (session_origin && *session_origin == ORIGIN_KIOSK_REPLAY) {
                        state = STATE_KIOSK_MODE;
                        reset_kiosk_timers();
                    } else {
                        state = STATE_CONFIG;
                    }
                    if (session_origin) *session_origin = ORIGIN_NONE;
                    gui_world = NULL;
                    break;
                }

                // KI-Agent unterstützt: [K] always returns to Kiosk (ADR-0020 Phase 5)
                if (IsKeyPressed(KEY_K)) {
                    cleanup_interactive_session(sim_ctx, config, r_ctx);
                    set_ignition_start_time(0.0);
                    state = STATE_KIOSK_MODE;
                    reset_kiosk_timers();
                    if (session_origin) *session_origin = ORIGIN_NONE;
                    gui_world = NULL;
                    break;
                }

                // Toggle Pause (Independent of mode)
                if (IsKeyPressed(KEY_SPACE)) {
                    config->is_paused = !config->is_paused;
                }

                // Toggle Observer Mode (Camera only)
                if (IsKeyPressed(KEY_O)) {
                    state = STATE_OBSERVER;
                    r_ctx->camera.zoom = 1.0f;
                    r_ctx->camera.target = (Vector2){ 0, 0 };
                    r_ctx->camera.offset = (Vector2){ 0, 0 };
                    strcpy(statusMsg, "OBSERVER MODE: ON");
                    statusTimer = 2.0f;
                }

                break;

            case STATE_OBSERVER:
                // Toggle Pause (Still available in Observer mode)
                if (IsKeyPressed(KEY_SPACE)) {
                    config->is_paused = !config->is_paused;
                }

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
                    r_ctx->camera.target.x -= delta.x / r_ctx->camera.zoom;
                    r_ctx->camera.target.y -= delta.y / r_ctx->camera.zoom;
                }

                // Zoom: Mouse Wheel
                float wheel = GetMouseWheelMove();
                if (wheel != 0) {
                    Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), r_ctx->camera);
                    r_ctx->camera.offset = GetMousePosition();
                    r_ctx->camera.target = mouseWorldPos;
                    const float zoomIncrement = 0.125f;
                    r_ctx->camera.zoom += (wheel * zoomIncrement);
                    if (r_ctx->camera.zoom < 0.125f) r_ctx->camera.zoom = 0.125f;
                }

                // KI-Agent unterstützt: [K] always returns to Kiosk from Observer (ADR-0020)
                if (IsKeyPressed(KEY_K)) {
                    cleanup_interactive_session(sim_ctx, config, r_ctx);
                    set_ignition_start_time(0.0);
                    state = STATE_KIOSK_MODE;
                    reset_kiosk_timers();
                    if (session_origin) *session_origin = ORIGIN_NONE;
                    break;
                }

                break;
                
            case STATE_FINISHED:
                if (IsKeyPressed(KEY_ENTER)) {
                     state = STATE_GAME_OVER;
                     int winner = 0;
                     if (config->current_red_pop > config->current_blue_pop) winner = TEAM_RED;
                     else if (config->current_blue_pop > config->current_red_pop) winner = TEAM_BLUE;
                     
                     // Append to Protocol
                     if (strlen(currentProtocolFilename) > 0) {
                        append_protocol_result(currentProtocolFilename, config, winner);
                     }                }
                if (IsKeyPressed(KEY_Q)) {
                    // KI-Agent unterstützt: Session-origin-aware routing (ADR-0021 bugfix)
                    cleanup_interactive_session(sim_ctx, config, r_ctx);
                    set_ignition_start_time(0.0);
                    if (session_origin && *session_origin == ORIGIN_KIOSK_REPLAY) {
                        state = STATE_KIOSK_MODE;
                        reset_kiosk_timers();
                    } else {
                        state = STATE_CONFIG;
                    }
                    if (session_origin) *session_origin = ORIGIN_NONE;
                    gui_world = NULL;
                }
                break;

            case STATE_GAME_OVER:
                if (IsKeyPressed(KEY_ONE)) {
                    // KI-Agent unterstützt: Centralized cleanup (ADR-0020 Phase 4)
                    cleanup_interactive_session(sim_ctx, config, r_ctx);
                    set_ignition_start_time(0.0);
                    state = STATE_CONFIG;
                    gui_world = NULL;
                }
                break;
            case STATE_KIOSK_MODE:
                // KI-Agent unterstützt: [P] shortcut to enter interactive mode (ADR-0020 Phase 6)
                if (IsKeyPressed(KEY_P)) {
                    state = STATE_CONFIG;
                    if (session_origin) *session_origin = ORIGIN_NONE;
                }
                break;
        }

    // KI-Agent unterstützt: Sync gui_world pointer back into context (ADR-0020)
    if (sim_ctx->current_world != gui_world && gui_world != NULL) {
        sim_ctx->current_world = gui_world;
    }
    return state;
}

// KI-Agent unterstützt: Forward declarations for dedicated kiosk render functions (ADR-0022 Phase B)
static void draw_kiosk_leaderboard(const KioskController *ctrl, int screen_w, int screen_h);
static void draw_kiosk_multicam(KioskController *ctrl, int screen_w, int screen_h);

void draw_current_state(AppState state, const GameConfig* config, const World* gui_world, RenderContext *r_ctx) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    // --- Drawing ---
        BeginDrawing(); // Raylib Anzeigesteuerung: Beginn einer neuen "Zeichenrunde"
        ClearBackground(THEME_BG); // Raylib Anzeigesteuerung: Gesamtes Fenster wird mit THEME_BG gefüllt

        // Draw HUD Backgrounds (Header & Footer)
        DrawRectangle(0, 0, screenWidth, 60, THEME_HUD); // Header // Raylib Zeichenfunktion: Rechteck zeichnen
        DrawRectangle(0, screenHeight - 40, screenWidth, 40, THEME_HUD); // Footer

        // Draw Status Message Overlay
        if (strlen(statusMsg) > 0) {
            // KI-Agent unterstützt: Center status message to avoid collision with counters
            DrawText(statusMsg, screenWidth/2 - MeasureText(statusMsg, 20)/2, 20, 20, GREEN); // Raylib Zeichenfunktion: Text zeichnen
        }

        switch (state) {
            case STATE_PUZZLE:
                DrawText("Tutorial Level 1", screenWidth/2 - MeasureText("Tutorial Level 1", 40)/2, screenHeight/2 - 120, 40, THEME_BLUE);
                DrawText("PRESS [ENTER] TO CONTINUE", screenWidth/2 - MeasureText("PRESS [ENTER] TO CONTINUE", 20)/2, screenHeight/2 + 150, 20, THEME_TEXT);

                // KI-Agent unterstützt: Mobile Editor Discovery (QR Placeholder + Link)
                int qrSize = 100;
                int qrX = screenWidth/2 - qrSize/2;
                int qrY = screenHeight/2 - 40;
                DrawRectangle(qrX - 5, qrY - 5, qrSize + 10, qrSize + 10, THEME_TEXT); // Border
                DrawRectangle(qrX, qrY, qrSize, qrSize, BLACK); // Background
                // Stylized QR pattern using dots
                for (int i = 0; i < 5; i++) {
                    for (int j = 0; j < 5; j++) {
                        if ((i + j) % 2 == 0) DrawRectangle(qrX + i*20 + 5, qrY + j*20 + 5, 10, 10, THEME_BLUE);
                    }
                }
                DrawText("MOBILE 8x8 EDITOR:", screenWidth/2 - MeasureText("MOBILE 8x8 EDITOR:", 16)/2, qrY + qrSize + 15, 16, THEME_HINT);
                DrawText("biotope.io/editor", screenWidth/2 - MeasureText("biotope.io/editor", 18)/2, qrY + qrSize + 35, 18, THEME_BLUE);
                break;
            case STATE_CONFIG:
                DrawText("BIOTOPE CONFIGURATION", 20, 15, 30, THEME_TEXT);
                
                char buf[64];
                sprintf(buf, "GRID SIZE:  %03d x %03d", config->rows, config->cols);
                DrawText(buf, 40, 100, 20, THEME_BLUE);
                DrawText("(Arrows)", 300, 100, 18, THEME_HINT);
                
                sprintf(buf, "DELAY:      %04d ms", config->delay_ms);
                DrawText(buf, 40, 140, 20, THEME_RED);
                DrawText("(+/-)", 300, 140, 18, THEME_HINT);
                
                sprintf(buf, "MAX ROUNDS: %04d", config->max_rounds);
                DrawText(buf, 40, 180, 20, THEME_BLUE);
                DrawText("(PageUp/PageDown)", 300, 180, 18, THEME_HINT);
                
                sprintf(buf, "MAX INIT POP:    %04d", config->max_population);
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
                DrawText("PRESS [ENTER] TO INITIALIZE SYSTEM", 40, 460, 20, THEME_ACCENT);
                DrawText("PRESS [K] TO ENTER 2x2 KIOSK MODE", 40, 495, 20, THEME_BLUE);
                break;

            case STATE_EDIT_RED:
            case STATE_EDIT_BLUE:
                DrawText("EDITOR MODE", 20, 18, 24, THEME_BLUE);
                
                // Centered Scoreboard
                char bluePopBuf[64], redPopBuf[64];
                sprintf(bluePopBuf, "BLUE: %03d/%03d", config->current_blue_pop, config->max_population);
                sprintf(redPopBuf, "RED: %03d/%03d", config->current_red_pop, config->max_population);
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
                    float cellW = (float)drawWidth / config->cols;
                    float cellH = (float)drawHeight / config->rows;
                    
                    if (mousePos.x >= startX && mousePos.x < startX + drawWidth &&
                        mousePos.y >= startY && mousePos.y < startY + drawHeight) {
                        int col = (int)((mousePos.x - startX) / cellW);
                        int row = (int)((mousePos.y - startY) / cellH);
                        // Draw Ghost only if on correct side
                        bool isCorrectSide = (state == STATE_EDIT_RED) ? (col >= config->cols/2) : (col < config->cols/2);
                        if (isCorrectSide) {
                            Color ghostColor = (state == STATE_EDIT_BLUE) ? Fade(THEME_BLUE, 0.2f) : Fade(THEME_RED, 0.2f);
                            DrawRectangle(startX + col * cellW, startY + row * cellH, cellW, cellH, ghostColor);
                        }
                    }
                }
                
                bool showLines = (config->rows <= 150 && config->cols <= 150);
                r_ctx->viewport_bounds = (Rectangle){ 20, 60, (float)screenWidth - 40, (float)screenHeight - 120 };
                DrawGridAndCellsCtx(r_ctx, config, gui_world, showLines); 

                // KI-Agent unterstützt: Dynamic Status-First Footer
                char footerBuf[256];
                if (config->is_paused) {
                    sprintf(footerBuf, "PAUSED | [SPACE] CONTINUE SIM | [O] OBSERVER | [Q] ABORT");
                } else {
                    sprintf(footerBuf, "RUNNING | [SPACE] PAUSE SIM | [O] OBSERVER | [Q] ABORT");
                }
                DrawText(footerBuf, 20, screenHeight - 28, 20, THEME_TEXT);
                break;

            case STATE_IGNITION:
                DrawText("SYSTEM IGNITION", 20, 18, 24, THEME_RED);
                r_ctx->viewport_bounds = (Rectangle){ 20, 60, (float)screenWidth - 40, (float)screenHeight - 120 };
                DrawGridAndCellsCtx(r_ctx, config, gui_world, false);
                {
                    double elapsed = GetTime() - get_ignition_start_time();
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
                sprintf(bluePopRun, "BLUE: %d", config->current_blue_pop);
                sprintf(redPopRun, "RED: %d", config->current_red_pop);
                int blueWRun = MeasureText(bluePopRun, 20);
                DrawText(bluePopRun, screenWidth/2 - blueWRun - 20, 20, 20, THEME_BLUE);
                DrawText(redPopRun, screenWidth/2 + 20, 20, 20, THEME_RED);

                // Right-aligned Round Counter (Compact)
                char roundBuf[32];
                sprintf(roundBuf, "CYCLE: %04d/%04d", config->current_round, config->max_rounds);
                int roundW = MeasureText(roundBuf, 20);
                DrawText(roundBuf, screenWidth - roundW - 20, 20, 20, THEME_TEXT);
                
                
                r_ctx->viewport_bounds = (Rectangle){ 20, 60, (float)screenWidth - 40, (float)screenHeight - 120 };
                DrawGridAndCellsCtx(r_ctx, config, gui_world, false); // false = No Grid Lines (Performance!)
                
                // KI-Agent unterstützt: Dynamic Status-First Footer for Running/Observer
                char simFooter[256];
                if (config->is_paused) {
                    if (state == STATE_RUNNING) {
                        // KI-Agent unterstützt: [K] KIOSK hint (ADR-0020 Phase 6)
                        sprintf(simFooter, "PAUSED | [SPACE] CONTINUE SIM | [O] OBSERVER | [Q] ABORT | [K] KIOSK");
                    } else {
                        sprintf(simFooter, "PAUSED | [SPACE] CONTINUE SIM | [O] EXIT | [MOUSE RIGHT] PAN | [WHEEL] ZOOM | [K] KIOSK");
                    }
                } else {
                    if (state == STATE_RUNNING) {
                        sprintf(simFooter, "RUNNING | [SPACE] PAUSE SIM | [O] OBSERVER | [Q] ABORT | [K] KIOSK");
                    } else {
                        sprintf(simFooter, "RUNNING | [SPACE] PAUSE SIM | [O] EXIT | [MOUSE RIGHT] PAN | [WHEEL] ZOOM | [K] KIOSK");
                    }
                }
                DrawText(simFooter, 20, screenHeight - 28, 20, THEME_TEXT);
                break;
                
            case STATE_FINISHED:
                DrawText("SIMULATION COMPLETED", 20, 18, 24, THEME_BLUE);
                
                r_ctx->viewport_bounds = (Rectangle){ 20, 60, (float)screenWidth - 40, (float)screenHeight - 120 };
                DrawGridAndCellsCtx(r_ctx, config, gui_world, false);
                
                DrawText("[ENTER] VIEW RESULTS  |  [Q] MENU", 20, screenHeight - 30, 20, THEME_ACCENT);
                break;

            case STATE_GAME_OVER:
                DrawText("MISSION REPORT", screenWidth/2 - 100, 100, 30, THEME_TEXT);
                
                char resultBuf[128];
                Color winnerColor = THEME_TEXT;
                if (config->current_red_pop > config->current_blue_pop) {
                    sprintf(resultBuf, "WINNER: RED TEAM");
                    winnerColor = THEME_RED;
                } else if (config->current_blue_pop > config->current_red_pop) {
                    sprintf(resultBuf, "WINNER: BLUE TEAM");
                    winnerColor = THEME_BLUE;
                } else {
                    sprintf(resultBuf, "RESULT: DRAW");
                }
                
                DrawText(resultBuf, screenWidth/2 - MeasureText(resultBuf, 40)/2, 200, 40, winnerColor);
                
                sprintf(buf, "RED: %d  vs  BLUE: %d", config->current_red_pop, config->current_blue_pop);
                DrawText(buf, screenWidth/2 - MeasureText(buf, 20)/2, 260, 20, GRAY);
                
                // --- Step 4.3: Telemetry Graph ---
                int graphW = 400;
                int graphH = 100;
                int graphX = screenWidth/2 - graphW/2;
                int graphY = 300;
                DrawRectangle(graphX, graphY, graphW, graphH, THEME_HUD);
                DrawRectangleLines(graphX, graphY, graphW, graphH, THEME_HINT);

                if (config->history_count > 1) {
                    // Find Peak Population for Dynamic Scaling
                    int peakPop = 0;
                    for (int i = 0; i < config->history_count; i++) {
                        if (config->history_red_pop[i] > peakPop) peakPop = config->history_red_pop[i];
                        if (config->history_blue_pop[i] > peakPop) peakPop = config->history_blue_pop[i];
                    }
                    
                    // Safety: Avoid division by zero and add 10% margin
                    float yMax = (peakPop > 0) ? (float)peakPop * 1.1f : (float)(config->rows * config->cols);

                    for (int i = 0; i < config->history_count - 1; i++) {
                        float x1 = graphX + ((float)i / config->max_rounds) * graphW;
                        float x2 = graphX + ((float)(i + 1) / config->max_rounds) * graphW;
                        
                        // Scale Y using the dynamic peak
                        float y1_red = graphY + graphH - ((float)config->history_red_pop[i] / yMax) * graphH;
                        float y2_red = graphY + graphH - ((float)config->history_red_pop[i+1] / yMax) * graphH;
                        
                        float y1_blue = graphY + graphH - ((float)config->history_blue_pop[i] / yMax) * graphH;
                        float y2_blue = graphY + graphH - ((float)config->history_blue_pop[i+1] / yMax) * graphH;
                        
                        DrawLine(x1, y1_red, x2, y2_red, THEME_RED);
                        DrawLine(x1, y1_blue, x2, y2_blue, THEME_BLUE);
                    }
                }

                DrawText("Stats exported to file.", screenWidth/2 - MeasureText("Stats exported to file.", 20)/2, 420, 20, THEME_HINT);
                DrawText("PRESS [1] TO RESTART SYSTEM", screenWidth/2 - MeasureText("PRESS [1] TO RESTART SYSTEM", 20)/2, 500, 20, THEME_ACCENT);
                break;
            case STATE_KIOSK_MODE:
                // KI-Agent unterstützt: Dispatches to dedicated render functions (ADR-0022 Phase B)
                if (kiosk_ctrl.current_sub_state == KIOSK_SUB_LEADERBOARD)
                    draw_kiosk_leaderboard(&kiosk_ctrl, screenWidth, screenHeight);
                else
                    draw_kiosk_multicam(&kiosk_ctrl, screenWidth, screenHeight);
                break;
        }

        EndDrawing(); // Raylib Anzeigesteuerung: Ende der "Zeichenrunde". Fertig gezeichnetes Bild wird im Fenster angezeigt.
}

// =============================================================================
// Kiosk render functions — extracted from draw_current_state() (ADR-0022 Phase B)
// Each function owns exactly one sub-state view and calls compute_kiosk_layout()
// at its top to derive all pixel values — no magic numbers anywhere below.
// =============================================================================

// KI-Agent unterstützt: Dedicated leaderboard render function extracted from draw_current_state (ADR-0022)
static void draw_kiosk_leaderboard(const KioskController *ctrl, int screen_w, int screen_h) {
    KioskLayout layout = compute_kiosk_layout(screen_w, screen_h, ctrl->match_count);

    // B.3: Solid footer panel — anchors CTA and progress bar visually (ADR-0022)
    int footer_y = screen_h - layout.bottom_panel_h;
    DrawRectangle(0, footer_y, screen_w, layout.bottom_panel_h, THEME_HUD);

    DrawText("GLOBAL LEADERBOARD",
             screen_w / 2 - MeasureText("GLOBAL LEADERBOARD", 30) / 2,
             80, 30, THEME_BLUE);

    int startY = 160;

    // KI-Agent unterstützt: Clip-to-fit — footer panel is always protected (ADR-0022)
    // rowHeight targets ~10 visible rows; clamp ensures readability.
    // show_count is capped so all drawn rows + optional "+N more" line fit above footer_y.
    int entries_area_h = footer_y - (startY + 40);
    int rowHeight      = entries_area_h / 10;
    if (rowHeight < 28) rowHeight = 28;

    int total_entries = ctrl->cached_lb.count;
    if (total_entries > MAX_LEADERBOARD_ENTRIES) total_entries = MAX_LEADERBOARD_ENTRIES;
    int max_fit      = entries_area_h / rowHeight;
    int show_count   = total_entries;
    int hidden_count = 0;
    if (total_entries > max_fit) {
        show_count   = max_fit - 1;  // reserve last slot for "+N more" line
        hidden_count = total_entries - show_count;
    }

    // B.6: Column header — "STAMINA" renamed to "ENDURANCE" (ADR-0022)
    DrawText("RANK",      screen_w / 2 - 230, startY, 20, THEME_HINT);
    DrawText("PLAYER",    screen_w / 2 - 120, startY, 20, THEME_HINT);
    DrawText("WIN RATE",  screen_w / 2 +  40, startY, 20, THEME_HINT);
    DrawText("W / D / L", screen_w / 2 + 150, startY, 20, THEME_HINT);
    DrawText("ENDURANCE", screen_w / 2 + 260, startY, 20, THEME_HINT);

    DrawLine(screen_w / 2 - 240, startY + 25, screen_w / 2 + 370, startY + 25, THEME_GRID);

    if (total_entries > 0) {
        for (int i = 0; i < show_count; i++) {
            int y = startY + 40 + i * rowHeight;
            char rankBuf[8];
            char winBuf[16];
            char wdlBuf[16];
            char asgBuf[16];

            sprintf(rankBuf, "#%d", i + 1);
            sprintf(winBuf,  "%.1f%%", ctrl->cached_lb.entries[i].win_rate);
            sprintf(wdlBuf,  "%d / %d / %d",
                    ctrl->cached_lb.entries[i].wins,
                    ctrl->cached_lb.entries[i].draws,
                    ctrl->cached_lb.entries[i].losses);
            sprintf(asgBuf,  "%.0f Gen", ctrl->cached_lb.entries[i].avg_stable_generation);

            Color rankCol = THEME_TEXT;
            if      (i == 0) rankCol = THEME_RED;
            else if (i == 1) rankCol = THEME_BLUE;
            else if (i == 2) rankCol = THEME_ACCENT;

            // B.5: Subtle coloured background for top-3 entries (ADR-0022)
            if (i < 3) {
                Color row_bg = (i == 0) ? Fade(THEME_RED,    0.12f)
                             : (i == 1) ? Fade(THEME_BLUE,   0.12f)
                                        : Fade(THEME_ACCENT,  0.10f);
                DrawRectangle(screen_w / 2 - 240, y - 4, 610, rowHeight, row_bg);
            }

            DrawText(rankBuf,                          screen_w / 2 - 230, y, 20, rankCol);
            DrawText(ctrl->cached_lb.entries[i].name,  screen_w / 2 - 120, y, 20, THEME_TEXT);
            DrawText(winBuf,                           screen_w / 2 +  40, y, 20, THEME_ACCENT);
            DrawText(wdlBuf,                           screen_w / 2 + 150, y, 20, THEME_TEXT);
            DrawText(asgBuf,                           screen_w / 2 + 260, y, 20, THEME_HINT);
        }

        // "+N more" indicator — only shown when entries were clipped
        if (hidden_count > 0) {
            char more_buf[32];
            sprintf(more_buf, "+ %d more", hidden_count);
            int more_y = startY + 40 + show_count * rowHeight + 4;
            DrawText(more_buf, screen_w / 2 - 230, more_y, 16, THEME_HINT);
        }
    } else {
        DrawText("LOADING DATA...",
                 screen_w / 2 - MeasureText("LOADING DATA...", 20) / 2,
                 startY + 60, 20, THEME_HINT);
    }

    // B.4: QR panel — proportional to screen width (ADR-0022)
    // Replaces the fixed 90x90 corner orphan with a clearly bordered, invitation panel.
    {
        int panel_w = screen_w / 4;
        int panel_h = (int)(screen_h * 0.42f);
        int panel_x = screen_w - panel_w - 20;
        int panel_y = (screen_h - layout.bottom_panel_h - panel_h) / 2;
        int qr_size = (int)(panel_w * 0.55f);
        if (qr_size < 140) qr_size = 140;
        int qr_x    = panel_x + (panel_w - qr_size) / 2;
        int qr_y    = panel_y + 20;
        int font_cta = layout.font_cta;
        int font_url = font_cta - 2;

        DrawRectangle(panel_x - 2, panel_y - 2, panel_w + 4, panel_h + 4, Fade(THEME_ACCENT, 0.25f));
        DrawRectangle(panel_x, panel_y, panel_w, panel_h, THEME_HUD);

        DrawRectangle(qr_x - 4, qr_y - 4, qr_size + 8, qr_size + 8, THEME_TEXT);
        DrawRectangle(qr_x, qr_y, qr_size, qr_size, BLACK);
        int cell = qr_size / 10;
        for (int qi = 0; qi < 9; qi++) {
            for (int qj = 0; qj < 9; qj++) {
                if ((qi + qj) % 2 == 0)
                    DrawRectangle(qr_x + qi * cell + 2, qr_y + qj * cell + 2,
                                  cell - 2, cell - 2, THEME_BLUE);
            }
        }

        const char *cta1  = "Submit YOUR strategy!";
        const char *cta2  = "editor.biotope.io";
        int          cta1_w = MeasureText(cta1, font_cta);
        int          cta2_w = MeasureText(cta2, font_url);
        DrawText(cta1, panel_x + (panel_w - cta1_w) / 2,
                 qr_y + qr_size + 14, font_cta, THEME_ACCENT);
        DrawText(cta2, panel_x + (panel_w - cta2_w) / 2,
                 qr_y + qr_size + 14 + font_cta + 6, font_url, THEME_BLUE);
    }

    // B.3: Progress bar anchored to footer panel (ADR-0022)
    {
        int bar_w = 240;
        int bar_x = screen_w / 2 - bar_w / 2;
        int bar_y = footer_y + layout.bottom_panel_h - 14;
        float progress = ctrl->state_timer / 15.0f;
        if (progress > 1.0f) progress = 1.0f;
        DrawRectangle(bar_x, bar_y, bar_w, 6, Fade(THEME_HINT, 0.3f));
        DrawRectangle(bar_x, bar_y, (int)(bar_w * progress), 6, THEME_ACCENT);
    }

    DrawText("PRESS [P] TO PLAY",
             screen_w / 2 - MeasureText("PRESS [P] TO PLAY", 20) / 2,
             footer_y + 8, 20, THEME_ACCENT);
}

// KI-Agent unterstützt: Dedicated multicam render function extracted from draw_current_state (ADR-0022)
static void draw_kiosk_multicam(KioskController *ctrl, int screen_w, int screen_h) {
    KioskLayout layout = compute_kiosk_layout(screen_w, screen_h, ctrl->match_count);

    // B.11 (footer): Solid footer panel — same pattern as leaderboard (ADR-0022)
    int footer_y = screen_h - layout.bottom_panel_h;
    DrawRectangle(0, footer_y, screen_w, layout.bottom_panel_h, THEME_HUD);

    // KI-Agent unterstützt: Update viewport bounds every frame so quadrants scale with the window (ADR-0022)
    // DrawGridAndCellsCtx detects the size change and reallocates GPU resources automatically.
    for (int i = 0; i < ctrl->match_count; i++) {
        int col = i % layout.grid_cols;
        int row = i / layout.grid_cols;
        ctrl->renders[i].viewport_bounds = (Rectangle){
            (float)(layout.pad + col * (layout.quad_w + layout.pad)),
            (float)(layout.top_bar_h + row * (layout.quad_h + layout.pad)),
            (float)layout.quad_w,
            (float)layout.quad_h
        };
    }

    // Render simulation grids into their respective viewports
    for (int i = 0; i < ctrl->match_count; i++) {
        DrawGridAndCellsCtx(&ctrl->renders[i], NULL, ctrl->sims[i].current_world, false);
    }

    // Per-quadrant HUD overlay: header, badge, thumbnails, score bar
    // KI-Agent unterstützt: Per-quadrant HUD overlay — names, badge, score bar, thumbnail (ADR-0021/ADR-0022)
    for (int i = 0; i < ctrl->match_count; i++) {
        Rectangle vp = ctrl->renders[i].viewport_bounds;
        int rx = (int)vp.x;
        int ry = (int)vp.y;
        int qw = (int)vp.width;
        int qh = (int)vp.height;

        // B.10: Two-row header — name strip + badge strip (ADR-0022)
        DrawRectangle(rx, ry, qw, layout.header_h - layout.badge_h, Fade(THEME_HUD, 0.88f));
        DrawRectangle(rx, ry + layout.header_h - layout.badge_h,
                      qw, layout.badge_h, Fade(THEME_HUD, 0.70f));

        const char *name_red  = ctrl->sims[i].participant_red;
        const char *name_blue = ctrl->sims[i].participant_blue;
        DrawText(name_red, rx + 6, ry + 5, 16, THEME_RED);
        int vs_x   = rx + 6 + MeasureText(name_red, 16) + 5;
        DrawText("vs", vs_x, ry + 7, 13, THEME_HINT);
        int blue_x = vs_x + MeasureText("vs", 13) + 5;
        DrawText(name_blue, blue_x, ry + 5, 16, THEME_BLUE);

        // B.10: metric_reason centred in badge strip (ADR-0022)
        const char *reason = ctrl->cached_highlights.matches[i].metric_reason;
        if (strlen(reason) > 0) {
            int rw = MeasureText(reason, layout.font_badge);
            DrawText(reason,
                     rx + (qw - rw) / 2,
                     ry + layout.header_h - layout.badge_h
                         + (layout.badge_h - layout.font_badge) / 2,
                     layout.font_badge, THEME_ACCENT);
        }

        // B.11: 8x8 seed thumbnails — cell size from layout.seed_cell_px (ADR-0022)
        // KI-Agent unterstützt: Side-by-side thumbnails correcting overlap bug (ADR-0021 bugfix)
        int thumb_cell = layout.seed_cell_px;
        int thumb_x    = rx + 4;
        int thumb_y    = ry + layout.header_h + 4;
        int thumb_w    = 8 * thumb_cell;
        int thumb_gap  = 3;
        DrawRectangle(thumb_x - 1, thumb_y - 1,
                      thumb_w * 2 + thumb_gap + 2, 8 * thumb_cell + 2,
                      Fade(BLACK, 0.65f));
        for (int tr = 0; tr < 8; tr++) {
            for (int tc = 0; tc < 8; tc++) {
                if (ctrl->cached_highlights.matches[i].seed_blue[tr * 8 + tc])
                    DrawRectangle(thumb_x + tc * thumb_cell,
                                  thumb_y + tr * thumb_cell,
                                  thumb_cell - 1, thumb_cell - 1,
                                  Fade(THEME_BLUE, 0.9f));
            }
        }
        int thumb_x_red = thumb_x + thumb_w + thumb_gap;
        for (int tr = 0; tr < 8; tr++) {
            for (int tc = 0; tc < 8; tc++) {
                if (ctrl->cached_highlights.matches[i].seed_red[tr * 8 + tc])
                    DrawRectangle(thumb_x_red + tc * thumb_cell,
                                  thumb_y + tr * thumb_cell,
                                  thumb_cell - 1, thumb_cell - 1,
                                  Fade(THEME_RED, 0.9f));
            }
        }

        // B.7: Score bar — height proportional to quad (ADR-0022)
        int bar_y   = ry + qh - layout.score_bar_h;
        DrawRectangle(rx, bar_y, qw, layout.score_bar_h, Fade(THEME_HUD, 0.88f));
        int red_pop  = ctrl->quad_red_pop[i];
        int blue_pop = ctrl->quad_blue_pop[i];
        int total    = red_pop + blue_pop;
        if (total > 0) {
            int red_w = (int)((float)red_pop / total * qw);
            DrawRectangle(rx,          bar_y, red_w,          layout.score_bar_h, Fade(THEME_RED,  0.75f));
            DrawRectangle(rx + red_w,  bar_y, qw - red_w,     layout.score_bar_h, Fade(THEME_BLUE, 0.75f));
        }
        int score_font = (int)(layout.score_bar_h * 0.72f);
        if (score_font < 14) score_font = 14;
        char pop_buf[16];
        sprintf(pop_buf, "%d", red_pop);
        DrawText(pop_buf, rx + 4, bar_y + 1, score_font, WHITE);
        sprintf(pop_buf, "%d", blue_pop);
        int pw = MeasureText(pop_buf, score_font);
        DrawText(pop_buf, rx + qw - pw - 4, bar_y + 1, score_font, WHITE);
    }

    // B.9: Cross-line separators between quadrants (ADR-0022)
    // Positions derived from stored viewport bounds — not hardcoded.
    if (ctrl->match_count > 1) {
        int line_top    = (int)ctrl->renders[0].viewport_bounds.y;
        int line_bottom = footer_y;

        for (int col = 1; col < layout.grid_cols; col++) {
            int sep_x = (int)(ctrl->renders[col - 1].viewport_bounds.x
                            + ctrl->renders[col - 1].viewport_bounds.width)
                        + layout.pad / 2;
            DrawRectangle(sep_x - layout.separator_px / 2, line_top,
                          layout.separator_px, line_bottom - line_top,
                          Fade(THEME_GRID, 0.8f));
        }

        for (int row = 1; row < layout.grid_rows; row++) {
            int first_in_row = row * layout.grid_cols;
            if (first_in_row >= ctrl->match_count) break;
            int sep_y = (int)(ctrl->renders[first_in_row].viewport_bounds.y)
                        - layout.pad / 2;
            DrawRectangle(0, sep_y - layout.separator_px / 2,
                          screen_w, layout.separator_px,
                          Fade(THEME_GRID, 0.8f));
        }
    }

    // B.8: Screen title — professional name replaces internal codename (ADR-0022)
    DrawRectangle(0, 0, screen_w, layout.top_bar_h, THEME_HUD);
    DrawText("LIVE BATTLES", 20, 10, 20, THEME_BLUE);

    // B.2.3: CTA text reflects keyboard interaction model (ADR-0022)
    DrawText("[1-4] WATCH MATCH  |  [P] PLAY",
             screen_w - MeasureText("[1-4] WATCH MATCH  |  [P] PLAY", 16) - 20,
             12, 16, THEME_ACCENT);

    // Progress bar anchored to footer panel (ADR-0021 / ADR-0022)
    {
        int bar_w  = 240;
        int bar_x  = screen_w / 2 - bar_w / 2;
        int bar_y2 = footer_y + layout.bottom_panel_h - 14;
        float progress = ctrl->state_timer / 30.0f;
        if (progress > 1.0f) progress = 1.0f;
        DrawRectangle(bar_x, bar_y2, bar_w, 6, Fade(THEME_HINT, 0.3f));
        DrawRectangle(bar_x, bar_y2, (int)(bar_w * progress), 6, THEME_ACCENT);
    }

    DrawText("PRESS [P] TO PLAY",
             screen_w / 2 - MeasureText("PRESS [P] TO PLAY", 20) / 2,
             footer_y + 8, 20, THEME_ACCENT);
}
