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
void draw_current_state(AppState state, const GameConfig* config, const World* gui_world, RenderContext *r_ctx);
void close_renderer(void);

// Refactored grid rendering using instance context
void DrawGridAndCellsCtx(RenderContext *r_ctx, const GameConfig *config, const World *gui_world, bool drawGridLines);

// KI-Agent unterstützt: Clear accumulated shader trail from ping-pong buffers (ADR-0020)
void clear_render_context_trail(RenderContext *ctx);

#endif // RENDERER_H