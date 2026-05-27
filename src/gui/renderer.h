#ifndef RENDERER_H
#define RENDERER_H

#include <stdbool.h>
#include "core_types.h"

// KI-Agent unterstützt: Renderer API
void init_renderer(int window_width, int window_height, const char* title);
AppState process_ui_events(AppState current_state, GameConfig* config, World** current_world, World** swap_world);
void draw_current_state(AppState state, const GameConfig* config, const World* world);
void close_renderer(void);

#endif // RENDERER_H