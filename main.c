

#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <unistd.h> 
#include "game_logic.h" 
#include "gui.h" // KI-Agent unterstützt - holt run_gui_app()


int main(int argc, char *argv[]) {
    // KI-Agent unterstützt: Switching to GUI mode
    printf("Starting Biotope GUI...\n"); // siehe Docker-Terminal
    run_gui_app(); // Init-Protokoll in Docker-Terminal
    return 0;


}
