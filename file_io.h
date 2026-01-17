#ifndef FILE_IO_H
#define FILE_IO_H

#include "game_logic.h"
#include "gui.h" // For GameConfig struct

// Struct to hold file metadata for the browser
typedef struct {
    char filename[128]; // e.g., "run_20260117_120000.bio"
    char filepath[256]; // Full relative path
    long timestamp;     // For sorting
    
    // Preview Data
    int rows;
    int cols;
    int max_rounds;
    int max_population;
    
    // Result Data (Optional)
    int has_results;    // 1 if simulation finished and results are appended
    int winner;         // 0=Draw, 1=Red, 2=Blue
    int final_red;
    int final_blue;
} ProtocolInfo;

// KI-Agent unterstützt
int save_grid(const char *filename, World *w, GameConfig *c);
// KI-Agent unterstützt
int load_grid(const char *filename, World *w, GameConfig *c);
// KI-Agent unterstützt
void append_protocol_result(const char *filename, int winner, int red, int blue);
// KI-Agent unterstützt
void export_stats_md(const char *filename, GameConfig *c, int winner);

// Lists .bio files in a directory. Returns count.
// Caller must free the list.
int list_protocol_files(const char *dir_path, ProtocolInfo **out_list);

// Loads just the header to preview metadata
int load_protocol_metadata(const char *filename, ProtocolInfo *info);

#endif // FILE_IO_H
