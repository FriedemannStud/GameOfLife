#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <unistd.h> 
#include <raylib.h>
#include "game_logic.h" 
#include "gui.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

int main(int argc, char *argv[]) {
    // KI-Agent unterstützt: Explicitly ignore unused parameters
    (void)argc;
    (void)argv;
    
    // KI-Agent unterstützt: Switching to GUI mode
    printf("Starting Biotope GUI...\n"); 
    
    init_gui_app(); 

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
#endif

    close_gui_app();
    return 0;
}