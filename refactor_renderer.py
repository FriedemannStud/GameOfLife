import re

with open("renderer.c", "r") as f:
    text = f.read()

# Replace includes
text = text.replace('#include "gui.h"', '#include "renderer.h"')

# Remove globals
text = re.sub(r'static int screenWidth.*?;', '', text)
text = re.sub(r'static int screenHeight.*?;', '', text)
text = re.sub(r'static AppState state.*?;', '', text)
text = re.sub(r'static GameConfig config.*?\n    \.max_rounds = 1000\n};\n', '', text, flags=re.DOTALL)
text = text.replace('World *gui_world = NULL;\nWorld *swap_world = NULL;\n', '')
text = text.replace('World *gui_world = NULL;', '')
text = text.replace('World *swap_world = NULL;', '')

# Rename init_gui_app and close_gui_app
text = text.replace('void init_gui_app(void)', 'void init_renderer(int window_width, int window_height, const char* title)')
text = text.replace('InitWindow(screenWidth, screenHeight, "Biotope - Game of Life");', 'InitWindow(window_width, window_height, title);')
text = text.replace('void close_gui_app(void)', 'void close_renderer(void)')

# Remove free_world from close_renderer
text = re.sub(r'if \(gui_world\) free_world\(gui_world\);', '', text)
text = re.sub(r'if \(swap_world\) free_world\(swap_world\);', '', text)

# DrawGridAndCells modification
text = text.replace('void DrawGridAndCells(GameConfig *config, int screenWidth, int screenHeight, bool drawGridLines)', 'void DrawGridAndCells(const GameConfig *config, const World *gui_world, int screenWidth, int screenHeight, bool drawGridLines)')
text = text.replace('DrawGridAndCells(&config, screenWidth, screenHeight, showLines);', 'DrawGridAndCells(config, gui_world, screenWidth, screenHeight, showLines);')
text = text.replace('DrawGridAndCells(&config, screenWidth, screenHeight, false);', 'DrawGridAndCells(config, gui_world, screenWidth, screenHeight, false);')

# Split UpdateDrawFrame
split_idx = text.find('// --- Drawing ---')

logic_part = text[:split_idx]
draw_part = text[split_idx:]

# Logic Part (process_ui_events)
logic_part = logic_part.replace('void UpdateDrawFrame(void) {', 'AppState process_ui_events(AppState state, GameConfig* config, World** p_current_world, World** p_swap_world) {\n    World* gui_world = *p_current_world;\n    World* swap_world = *p_swap_world;\n    int screenWidth = GetScreenWidth();\n    int screenHeight = GetScreenHeight();')
logic_part = logic_part.replace('config.', 'config->')
logic_part = logic_part.replace('&config->', 'config')
logic_part = logic_part.replace('&config->current_red_pop', '&config->current_red_pop') # config is ptr, so &config->current_red_pop is right. Wait, previously it was &config.current_red_pop, which became &config->current_red_pop. Correct!
logic_part = logic_part.replace('!gui_world', 'gui_world == NULL')

# Remove simulation logic from process_ui_events
logic_part = re.sub(r'// --- Simulation Logic ---.*?break;', 'break;', logic_part, flags=re.DOTALL)
logic_part = re.sub(r'// --- Simulation Logic \(Shared with RUNNING\) ---.*?break;', 'break;', logic_part, flags=re.DOTALL)
logic_part = re.sub(r'case STATE_IGNITION:.*?break;', 'case STATE_IGNITION:\n                if (ignitionStartTime == 0.0) ignitionStartTime = GetTime();\n                break;', logic_part, flags=re.DOTALL)

# Also remove editorBtnRec from logic_part
logic_part = re.sub(r'Rectangle editorBtnRec = .*?OpenURL\("editor.html"\);\n        }', '', logic_part, flags=re.DOTALL)

logic_part += "    *p_current_world = gui_world;\n    *p_swap_world = swap_world;\n    return state;\n}\n\n"


# Draw Part (draw_current_state)
draw_part = draw_part.replace('// --- Drawing ---', 'void draw_current_state(AppState state, const GameConfig* config, const World* gui_world) {\n    int screenWidth = GetScreenWidth();\n    int screenHeight = GetScreenHeight();\n    Rectangle editorBtnRec = { (float)screenWidth - 220, 15, 200, 30 };\n    bool hoverEditor = (state != STATE_PUZZLE) && CheckCollisionPointRec(GetMousePosition(), editorBtnRec);\n    // --- Drawing ---')

draw_part = draw_part.replace('config.', 'config->')

# Fix EndDrawing block. 
# draw_part ends with:
#        EndDrawing(); // ...
# }
# void close_gui_app(void) {
# ...
draw_part = draw_part.replace('void close_gui_app(void)', 'void close_renderer(void)')

with open("renderer.c", "w") as f:
    f.write(logic_part + draw_part)
