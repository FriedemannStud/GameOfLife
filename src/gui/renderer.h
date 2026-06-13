#ifndef RENDERER_H
#define RENDERER_H

#include <stdbool.h>
#include "raylib.h"
#include "core_types.h"

// KI-Agent unterstützt: RenderContext encapsulating dynamic viewport GPU resources
typedef struct {
    Texture2D grid_texture;
    unsigned char* pixel_buffer; // CPU-side pixel buffer before GPU upload
    Rectangle viewport_bounds;
    Camera2D camera;

    int tex_w;
    int tex_h;
    int last_draw_w;
    int last_draw_h;

    RenderTexture2D ping_pong_target[2];
    int ping_pong_index;
    int loc_prev_frame;
    int loc_fade_rate;
    bool use_metaballs;

    Color col_background;
    Color col_team_red;
    Color col_team_blue;
} RenderContext;

void init_render_context(RenderContext *ctx, int cols, int rows, Rectangle bounds);
void free_render_context(RenderContext *ctx);

// KI-Agent unterstützt: Renderer API
void init_renderer(int window_width, int window_height, const char* title);
// KI-Agent unterstützt: Refactored to use SimulationContext* instead of raw World** (ADR-0020)
AppState process_ui_events(AppState current_state, GameConfig* config, SimulationContext *sim_ctx, RenderContext *r_ctx, SessionOrigin *session_origin);
// KI-Agent unterstützt: sim_ctx added to display participant names in KIOSK_REPLAY (ADR-0024)
void draw_current_state(AppState state, const GameConfig* config, const World* gui_world,
                        RenderContext *r_ctx, const SimulationContext *sim_ctx);
void close_renderer(void);

// Refactored grid rendering using instance context
void DrawGridAndCellsCtx(RenderContext *r_ctx, const GameConfig *config, const World *gui_world, bool drawGridLines);

// KI-Agent unterstützt: Clear accumulated shader trail from ping-pong buffers (ADR-0020)
void clear_render_context_trail(RenderContext *ctx);

// KI-Agent unterstützt: Proportional layout descriptor for the Kiosk multi-match grid (ADR-0022)
// All pixel values are derived from screen size and match count — no magic numbers in draw code.
typedef struct {
    // Grid geometry — computed from match_count and screen dimensions
    int grid_cols;       // columns in the match grid (e.g. 2 for 4 matches, 3 for 9)
    int grid_rows;       // rows in the match grid
    int quad_w;          // quadrant pixel width
    int quad_h;          // quadrant pixel height
    int pad;             // uniform padding between and around quadrants
    int separator_px;    // cross-line thickness drawn between quadrants

    // Per-quadrant HUD proportions — all derived from quad_h / quad_w
    int header_h;        // name strip height at top of each quadrant
    int score_bar_h;     // score bar height at bottom of each quadrant
    int badge_h;         // metric-reason badge height (used from Phase B onward)
    int seed_cell_px;     // pixel size of each cell in the 8x8 seed thumbnail (multicam)
    int lb_icon_cell_px;  // pixel size of each cell in the 8x8 seed thumbnail (leaderboard)

    // KI-Agent unterstützt: Scaled fonts — proportional to screen_h (reference 1080p)
    int font_badge;       // metric reason badge
    int font_title;       // leaderboard section title
    int font_header;      // column headers + table rows
    int font_small;       // secondary labels ("+N more", "LOADING DATA...")
    int font_cta;         // footer call-to-action text
    int font_name;        // player names in multicam header
    int font_vs;          // "vs" separator in multicam header
    int font_topbar;      // top bar primary label
    int font_topbar_cta;  // top bar key-hint label

    // Global chrome areas
    int top_bar_h;        // global top bar height
    int bottom_panel_h;   // footer panel height (holds CTA + progress bar)

    // Leaderboard layout — Y positions follow chrome + font heights
    int lb_title_y;       // Y of the "GLOBAL LEADERBOARD" title
    int lb_table_y;       // Y of the column header row
    int progress_bar_w;   // width of the countdown progress bar
} KioskLayout;

// Pure function: derives all pixel values from screen size and match count.
// No drawing, no Raylib calls, no side effects — safe to call any time.
KioskLayout compute_kiosk_layout(int screen_w, int screen_h, int match_count);

#endif // RENDERER_H