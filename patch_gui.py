import re

with open('gui.h', 'r') as f:
    content = f.read()

content = content.replace('void run_gui_app();', 'void init_gui_app(void);\nvoid UpdateDrawFrame(void);\nvoid close_gui_app(void);')

with open('gui.h', 'w') as f:
    f.write(content)


with open('gui.c', 'r') as f:
    content = f.read()

# Replace run_gui_app with init_gui_app, UpdateDrawFrame, close_gui_app

# First, define global states
globals_decl = """// Global state for single-frame loop
static int screenWidth = 800;
static int screenHeight = 600;
static AppState state = STATE_CONFIG;
static GameConfig config = {
    .rows = 50, 
    .cols = 50, 
    .delay_ms = 100, 
    .max_population = 100, 
    .max_rounds = 1000,
    .current_red_pop = 0,
    .current_blue_pop = 0,
    .current_round = 0
};
static char statusMsg[64] = "";
static float statusTimer = 0.0f;
static float timeAccumulator = 0.0f;
static int editAction = 0;

void init_gui_app(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Biotope - Game of Life");
    // Removed SetTargetFPS(120); because emscripten handles pacing
}

void close_gui_app(void) {
    if (gui_world) free_world(gui_world);
    if (swap_world) free_world(swap_world);
    CloseWindow();
}

void UpdateDrawFrame(void) {
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();
    
    if (statusTimer > 0) {
        statusTimer -= GetFrameTime();
        if (statusTimer <= 0) strcpy(statusMsg, "");
    }
"""

run_gui_start = content.find('void run_gui_app() {')
if run_gui_start == -1:
    print("Could not find run_gui_app")
    exit(1)

# Find the start of the switch statement
switch_start = content.find('switch (state) {', run_gui_start)
while_start = content.find('while (!WindowShouldClose()) {', run_gui_start)

# Replace from run_gui_app() to the switch statement
content = content[:run_gui_start] + globals_decl + content[switch_start-8:content.rfind('if (gui_world) free_world(gui_world);')].replace('        // --- Logic per State ---', '    // --- Logic per State ---').replace('    while (!WindowShouldClose()) {      // HIER geht\\'s los! Game Loop/Hauptschleife ', '')

# Fix indentation and missing closing braces
content = content.replace('static int editAction = 0; // 0:Idle, 1:Place, 2:Remove', '')
content = content.replace('static float timeAccumulator = 0.0f;', '')

# Because we removed the while loop, we need to remove its closing brace.
content = content[:content.rfind('}')] + '\n}'

with open('gui.c', 'w') as f:
    f.write(content)

print("Patch applied to gui.c and gui.h")
